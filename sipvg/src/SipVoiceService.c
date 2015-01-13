#include "SipVoiceService.h"
#include "mediastreamer2/msfilter.h"
#include "ortp/ortp.h"
#include "mschannel.h"
#include "mediastreamer2/msfilerec.h"
//回调
extern SalCallbacks sal_callbacks;
//数据回调入口
static void OnChannelDataNotify(channel_t* c,void* im)
{
	sip_voice_service_t* s=(sip_voice_service_t*)channel_get_user_data(c);
	mblk_t* om=(mblk_t*)im;
	if(s->cb)
		s->cb(s->ud,om->b_rptr,om->b_wptr-om->b_rptr);
}

static int find_codec_rank(const char *mime, int clock_rate)
{
	int i;

#ifdef __arm__
	/*hack for opus, that needs to be disabed by default on ARM single processor, otherwise there is no cpu left for video processing*/
	if (strcasecmp(mime,"opus")==0){
		if (ms_get_cpu_count()==1) return RANK_END;
	}
#endif
	for(i=0;codec_pref_order[i].name!=NULL;++i){
		if (strcasecmp(codec_pref_order[i].name,mime)==0 && clock_rate==codec_pref_order[i].rate)
			return i;
	}
	return RANK_END;
}
static int codec_compare(const PayloadType *a, const PayloadType *b)
{
	int ra,rb;
	ra=find_codec_rank(a->mime_type,a->clock_rate);
	rb=find_codec_rank(b->mime_type,b->clock_rate);
	if (ra>rb) return 1;
	if (ra<rb) return -1;
	return 0;
}
static void assign_payload_type(sip_voice_service_t *lc, PayloadType *const_pt, int number, const char *recv_fmtp)
{
	PayloadType *pt;
	
#ifdef ANDROID
	if (const_pt->channels==2){
		ms_message("Stereo %s codec not supported on this platform.",const_pt->mime_type);
		return;
	}
#endif
	
	pt=payload_type_clone(const_pt);
	if (number==-1){
		/*look for a free number */
		MSList *elem;
		int i;
		for(i=lc->dyn_pt;i<RTP_PROFILE_MAX_PAYLOADS;++i){
			bool_t already_assigned=FALSE;
			for(elem=lc->payload_types;elem!=NULL;elem=elem->next){
				PayloadType *it=(PayloadType*)elem->data;
				if (payload_type_get_number(it)==i){
					already_assigned=TRUE;
					break;
				}
			}
			if (!already_assigned){
				number=i;
				lc->dyn_pt=i+1;
				break;
			}
		}
		if (number==-1){
			ms_fatal("FIXME: too many codecs, no more free numbers.");
		}
	}
	ms_message("assigning %s/%i payload type number %i",pt->mime_type,pt->clock_rate,number);
	payload_type_set_number(pt,number);
	if (recv_fmtp!=NULL) payload_type_set_recv_fmtp(pt,recv_fmtp);
	rtp_profile_set_payload(lc->default_profile,number,pt);
	lc->payload_types=ms_list_append(lc->payload_types,pt);
}
static MSList *add_missing_codecs(sip_voice_service_t *lc, SalStreamType mtype, MSList *l)
{
	int i;
	for(i=0;i<RTP_PROFILE_MAX_PAYLOADS;++i){
		PayloadType *pt=rtp_profile_get_payload(lc->default_profile,i);
		if (pt){
			if (mtype==SalVideo && pt->type!=PAYLOAD_VIDEO)
				pt=NULL;
			else if (mtype==SalAudio && (pt->type!=PAYLOAD_AUDIO_PACKETIZED
			    && pt->type!=PAYLOAD_AUDIO_CONTINUOUS)){
				pt=NULL;
			}
			if (mtype==SalH239 && pt->type!=PAYLOAD_VIDEO)
				pt=NULL;
			if (pt && ms_filter_codec_supported(pt->mime_type)){
				if (ms_list_find(l,pt)==NULL){
					/*unranked codecs are disabled by default*/
					if (find_codec_rank(pt->mime_type, pt->clock_rate)!=RANK_END){
						payload_type_set_flag(pt,PAYLOAD_TYPE_ENABLED);
					}
					ms_message("Adding new codec %s/%i with fmtp %s",
					    pt->mime_type,pt->clock_rate,pt->recv_fmtp ? pt->recv_fmtp : "");
					l=ms_list_insert_sorted(l,pt,(int (*)(const void *, const void *))codec_compare);
				}
			}
		}
	}
	return l;
}
//Window睡眠函数
#ifdef WIN32
void WinSleep(int usec){
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0,1)){
    	TranslateMessage(&msg);
    	DispatchMessage(&msg);
	}
	if(usec)Sleep(usec);
}
#else
#define WinSleep ms_usleep
#endif
//监听函数
static void* sip_message_listen_fun(void* arg)
{
	sip_voice_service_t* s=(sip_voice_service_t*)arg;
	while(s->running){
		//开始监听服务
		sal_iterate(s->sal);
		WinSleep(5);
	}	
	return NULL;
}
//设置语音数据
void sip_voice_service_set_audio_data(sip_voice_service_t* s,uint8_t* data,int len)
{
	if(s->channel){
		mblk_t* om=allocb(len,0);
		memcpy(om->b_wptr,data,len);
		om->b_wptr+=len;
		
		if(s->debug)
			printf("playing pcm audio data: %d\n",len);
		channel_dispatch(s->channel,om);
	        //ms_filter_call_method(s->m_call->stream->source,MS_FILTER_SET_SOURCE_CAHNNEL_MSG,om);
	}
}
//设置语音数据回调
void sip_voice_service_set_audio_data_cb(sip_voice_service_t* s,on_audio_data_cb cb,on_session_close_cb close_cb,on_session_created_cb call_cb)
{
	s->cb=cb;
	s->close_cb=close_cb;
	s->call_cb=call_cb;
}
//新建
sip_voice_service_t* sip_voice_service_new(void* ud)
{
	sip_voice_service_t* s=ms_new0(sip_voice_service_t,1);
	s->auto_answer=1;
	s->ud=ud;
	//ortp 初始化
	ortp_init();
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	s->dyn_pt=96;
	s->default_profile=rtp_profile_new("default profile");

	assign_payload_type(s,&payload_type_pcmu8000,0,NULL);
	assign_payload_type(s,&payload_type_gsm,3,NULL);
	assign_payload_type(s,&payload_type_pcma8000,8,NULL);
	assign_payload_type(s,&payload_type_speex_nb,110,"vbr=on");
	assign_payload_type(s,&payload_type_speex_wb,111,"vbr=on");
	assign_payload_type(s,&payload_type_speex_uwb,112,"vbr=on");
	assign_payload_type(s,&payload_type_telephone_event,101,"0-11");
	assign_payload_type(s,&payload_type_g722,9,NULL);

	//初始化 mediastreamer2
	ms_init();
	//channels init
	mschannel_init();

	//创建sal
	s->sal=sal_init();
	sal_set_user_pointer(s->sal,s);
	//设置回调
	sal_set_callbacks(s->sal,&sal_callbacks);

	sal_enable_auto_contacts(s->sal,1);
	//启动监听
	s->udp_port=5060;
	sal_unlisten_ports(s->sal);
	if (sal_listen_port (s->sal,"0.0.0.0",s->udp_port,SalTransportUDP,FALSE)!=0){
		sip_voice_service_destroy(s);		
		printf("can't listenning on port 5060\n");
		return NULL;
	}
	
	//编码列表
	s->codecs=add_missing_codecs(s,SalAudio,s->codecs);

	s->channel=channel_new(get_default_channel_manager(0),s,1);
	channel_set_notify_cb(s->channel,OnChannelDataNotify);
	//启动监听
	s->running=1;
	ms_thread_create(&s->_th,NULL,sip_message_listen_fun,s);
	return s;
}
//销毁
void sip_voice_service_destroy(sip_voice_service_t* s)
{
	if(s->m_call)terminate_call(s);
	s->running=0;
	ms_thread_join(s->_th,NULL);
	//关闭端口
	sal_unlisten_ports(s->sal);
	//销毁profiles
	rtp_profile_clear_all(s->default_profile);
	rtp_profile_destroy(s->default_profile);
	ms_list_for_each(s->payload_types,(void (*)(void*))payload_type_destroy);
	ms_list_free(s->payload_types);
	s->payload_types=NULL;

	//通道销毁
	if(s->channel)channel_destroy(s->channel);

	//释放
	sal_uninit(s->sal);
	//退出ortp
	ortp_exit();

	//释放内存
	ms_free(s);
}

//for testing
void sip_voice_service_set_rec_filename(sip_voice_service_t* s,const char* filename)
{
	if(s->m_call){
		ms_filter_call_method(s->m_call->stream->record,MS_FILE_REC_OPEN,(void*)filename);
	}
}
//start recording
void sip_voice_service_rec_start(sip_voice_service_t* s)
{
	if(s->m_call){
		ms_filter_call_method(s->m_call->stream->record,MS_FILE_REC_START,NULL);
	}
}
//stop recording;
void sip_voice_service_rec_stop(sip_voice_service_t* s)
{
	if(s->m_call){
		ms_filter_call_method(s->m_call->stream->record,MS_FILE_REC_STOP,NULL);
		ms_filter_call_method(s->m_call->stream->record,MS_FILE_REC_CLOSE,NULL);
	}
}
