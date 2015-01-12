#include "SipVoiceService.h"
#include "call_session.h"
#include "mediastreamer2/msfilter.h"
#include "mschannel.h"

extern "C" size_t b64_encode(void const *src, size_t srcSize, char *dest, size_t destLen);
//创建列表
static MSList *make_codec_list(sip_voice_service_t *lc, const MSList *codecs, int* max_sample_rate, int nb_codecs_limit){
	MSList *l=NULL;
	const MSList *it;
	int nb = 0;
	if (max_sample_rate) *max_sample_rate=0;
	for(it=codecs;it!=NULL;it=it->next){
		PayloadType *pt=(PayloadType*)it->data;
		if (pt->flags & PAYLOAD_TYPE_ENABLED){
			l=ms_list_append(l,payload_type_clone(pt));
			nb++;
			if (max_sample_rate && payload_type_get_rate(pt)>*max_sample_rate) *max_sample_rate=payload_type_get_rate(pt);
		}
		if ((nb_codecs_limit > 0) && (nb >= nb_codecs_limit)) break;
	}
	return l;
}
static bool_t generate_b64_crypto_key(int key_length, char* key_out) {
	int b64_size;
	uint8_t* tmp = (uint8_t*) malloc(key_length);			
	if (ortp_crypto_get_random(tmp, key_length)!=0) {
		ms_error("Failed to generate random key");
		free(tmp);
		return FALSE;
	}
	
	b64_size = b64_encode((const char*)tmp, key_length, NULL, 0);
	if (b64_size == 0) {
		ms_error("Failed to b64 encode key");
		free(tmp);
		return FALSE;
	}
	key_out[b64_size] = '\0';
	b64_encode((const char*)tmp, key_length, key_out, 40);
	free(tmp);
	return TRUE;
}
static void update_media_description_from_stun(SalMediaDescription *md, const StunCandidate *ac, const StunCandidate *vc){
	int i;
	for (i = 0; i < md->n_active_streams; i++) {
		if ((md->streams[i].type == SalAudio) && (ac->port != 0)) {
			strcpy(md->streams[0].rtp_addr,ac->addr);
			md->streams[0].rtp_port=ac->port;
			if ((ac->addr[0]!='\0' && vc->addr[0]!='\0' && strcmp(ac->addr,vc->addr)==0) || md->n_active_streams==1){
				strcpy(md->addr,ac->addr);
			}
		}
		if ((md->streams[i].type == SalVideo) && (vc->port != 0)) {
			strcpy(md->streams[1].rtp_addr,vc->addr);
			md->streams[1].rtp_port=vc->port;
		}
		if ((md->streams[i].type == SalH239) && (vc->port != 0)) {
			strcpy(md->streams[1].rtp_addr,vc->addr);
			md->streams[2].rtp_port=vc->port;
		}
	}
}
//new call
call_t* call_new_incomming(sip_voice_service_t* s,SalOp* op){
	call_t* call=ms_new0(call_t,1);
	call->dir=CallIncoming;
	call->op=op;
	call->core=s;
	call->start_time=time(NULL);	
	//get local ip
	sal_get_default_local_ip(s->sal,AF_INET,call->localip,CALL_IPADDR_SIZE);
	call->port=10241;
	sal_op_set_user_pointer(op,call);
	return call;
}
//destroy
void call_destroy(call_t* call){
	if(call->profile){
		rtp_profile_clear_all(call->profile);
		rtp_profile_destroy(call->profile);
		call->profile=NULL;
	}
	ms_free(call);
}
void make_local_media_description(sip_voice_service_t *lc, call_t *call){
	MSList *l;
	PayloadType *pt;
	SalMediaDescription *old_md=call->localdesc;
	int i;
	
	SalMediaDescription *md=sal_media_description_new();
	SalAddress *addr;
	bool_t keep_srtp_keys=0;
	char* local_ip=call->localip;
	//本地联系人
	const char *me=ms_strdup_printf("sip:999@%s",local_ip);
	addr=sal_address_new(me);
	
	
	md->session_id=(old_md ? old_md->session_id : (rand() & 0xfff));
	md->session_ver=(old_md ? (old_md->session_ver+1) : (rand() & 0xfff));
	md->n_total_streams=(old_md ? old_md->n_total_streams : 1);
	md->n_active_streams=1;
	strncpy(md->addr,local_ip,sizeof(md->addr));
	strncpy(md->username,sal_address_get_username(addr),sizeof(md->username));
	/*set audio capabilities */
	strncpy(md->streams[0].rtp_addr,local_ip,sizeof(md->streams[0].rtp_addr));
	strncpy(md->streams[0].rtcp_addr,local_ip,sizeof(md->streams[0].rtcp_addr));
	md->streams[0].rtp_port=call->port;
	md->streams[0].rtcp_port=call->port+1;
	md->streams[0].proto=SalProtoRtpAvp;
	md->streams[0].type=SalAudio;
	l=make_codec_list(lc,lc->codecs,&md->streams[0].max_rate,-1);
	pt=payload_type_clone(rtp_profile_get_payload_from_mime(lc->default_profile,"telephone-event"));
	l=ms_list_append(l,pt);
	md->streams[0].payloads=l;

	if (md->n_total_streams < md->n_active_streams)
		md->n_total_streams = md->n_active_streams;
	/* Deactivate inactive streams. */
	for (i = md->n_active_streams; i < md->n_total_streams; i++) {
		md->streams[i].rtp_port = 0;
		md->streams[i].rtcp_port = 0;
		md->streams[i].proto = SalProtoRtpAvp;
		md->streams[i].type = old_md->streams[i].type;
		md->streams[i].dir = SalStreamInactive;
		l = make_codec_list(lc, NULL, NULL, 1);
		md->streams[i].payloads = l;
	}

	for(i=0; i<md->n_active_streams; i++) {
		if (md->streams[i].proto == SalProtoRtpSavp) {
			if (keep_srtp_keys && old_md && old_md->streams[i].proto==SalProtoRtpSavp){
				int j;
				for(j=0;j<SAL_CRYPTO_ALGO_MAX;++j){
					memcpy(&md->streams[i].crypto[j],&old_md->streams[i].crypto[j],sizeof(SalSrtpCryptoAlgo));
				}
			}else{
				md->streams[i].crypto[0].tag = 1;
				md->streams[i].crypto[0].algo = AES_128_SHA1_80;
				if (!generate_b64_crypto_key(30, md->streams[i].crypto[0].master_key))
					md->streams[i].crypto[0].algo = (ortp_srtp_crypto_suite_t)0;
				md->streams[i].crypto[1].tag = 2;
				md->streams[i].crypto[1].algo = AES_128_SHA1_32;
				if (!generate_b64_crypto_key(30, md->streams[i].crypto[1].master_key))
					md->streams[i].crypto[1].algo =(ortp_srtp_crypto_suite_t)0;
				md->streams[i].crypto[2].algo = (ortp_srtp_crypto_suite_t)0;
			}
		}
	}
	update_media_description_from_stun(md,&call->ac,&call->vc);
	
	sal_address_destroy(addr);
	call->localdesc=md;
	if (old_md) sal_media_description_unref(old_md);
	ms_free((void*)me);
}
//
int accept_call(sip_voice_service_t *lc, call_t *call)
{
	SalOp *replaced;
	SalMediaDescription *new_md;
	bool_t was_ringing=FALSE;
	//create local media
	make_local_media_description(lc,call);
	sal_call_set_local_media_description(call->op,call->localdesc);	
	sal_call_accept(call->op);
	return 0;
}
//terminate call
void terminate_call(sip_voice_service_t *lc){
	if(lc->m_call)
		sal_call_terminate(lc->m_call->op);
}
//connect channel
void connect_channels(call_t* call){
	//连接通道
	channel_t* source_ch=NULL,*output_ch=NULL;
	ms_filter_call_method(call->stream->source,MS_FILTER_GET_SOURCE_CAHNNEL,&source_ch);
	ms_filter_call_method(call->stream->output,MS_FILTER_GET_OUTPUT_CAHNNEL,&output_ch);
	channel_link(call->core->channel,source_ch,1);
	channel_link(call->core->channel,output_ch,2);
}
void disconnect_channels(call_t* call){
	//连接通道
	channel_t* source_ch=NULL,*output_ch=NULL;
	ms_filter_call_method(call->stream->source,MS_FILTER_GET_SOURCE_CAHNNEL,&source_ch);
	ms_filter_call_method(call->stream->output,MS_FILTER_GET_OUTPUT_CAHNNEL,&output_ch);
	channel_unlink(call->core->channel,source_ch,1);
	channel_unlink(call->core->channel,output_ch,2);
}
