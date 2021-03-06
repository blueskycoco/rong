#include "sal/sal.h"
#include "SipVoiceService.h"

static RtpProfile *make_profile(call_t *call, const SalMediaDescription *md, const SalStreamDescription *desc, int *used_pt){
	const MSList *elem;
	RtpProfile *prof=rtp_profile_new("Call profile");
	bool_t first=TRUE;
	int remote_bw=0;
	sip_voice_service_t *lc=call->core;
	int up_ptime=0;
	
	*used_pt=-1;

	for(elem=desc->payloads;elem!=NULL;elem=elem->next){
		PayloadType *pt=(PayloadType*)elem->data;
		int number;

		if ((pt->flags & PAYLOAD_TYPE_FLAG_CAN_SEND) && first) {
			*used_pt=payload_type_get_number(pt);
			first=FALSE;
		}
		if (desc->bandwidth>0) remote_bw=desc->bandwidth;
		else if (md->bandwidth>0) {
			/*case where b=AS is given globally, not per stream*/
			remote_bw=md->bandwidth;
		}
		if (desc->type==SalAudio){
			pt->normal_bitrate=-1;
		}
		if (desc->ptime>0){
			up_ptime=desc->ptime;
		}
		if (up_ptime>0){
			char tmp[40];
			snprintf(tmp,sizeof(tmp),"ptime=%i",up_ptime);
			payload_type_append_send_fmtp(pt,tmp);
		}
		number=payload_type_get_number(pt);
		if (rtp_profile_get_payload(prof,number)!=NULL){
			ms_warning("A payload type with number %i already exists in profile !",number);
		}else
			rtp_profile_set_payload(prof,number,pt);
	}
	return prof;
}

//更新会话
void update_remote_session_id_and_ver(call_t *call) {
	SalMediaDescription *remote_desc = sal_call_get_remote_media_description(call->op);
	if (remote_desc) {
		call->remote_session_id = remote_desc->session_id;
		call->remote_session_ver = remote_desc->session_ver;
	}
}
//start audio stream
static void call_start_audio_stream(call_t *call){
	const char *cname=ms_strdup_printf("sip:999@%s",call->localip);
	const SalStreamDescription *stream=sal_media_description_find_stream(call->resultdesc,
		SalProtoRtpAvp,SalAudio);
	int used_pt=-1;
	if (stream && stream->dir!=SalStreamInactive && stream->rtp_port!=0){
		call->profile=make_profile(call,call->resultdesc,stream,&used_pt);
		if (used_pt!=-1){
			call->codec = rtp_profile_get_payload(call->profile, used_pt);
			//connect channels
			connect_channels(call);
			audio_stream_start(call->stream,call->profile,
				stream->rtp_addr[0]!='\0' ? stream->rtp_addr : call->resultdesc->addr,
				stream->rtp_port,
				stream->rtcp_addr[0]!='\0' ? stream->rtcp_addr : call->resultdesc->addr,
				stream->rtcp_port,
				0,used_pt);
		}
	}	
	ms_free((void*)cname);
}
void call_stop_audio_stream(call_t *call) {
	audio_stream_stop(call->stream);
	if(call->profile){
		rtp_profile_clear_all(call->profile);
		rtp_profile_destroy(call->profile);
		call->profile=NULL;
	}
	audio_stream_destroy(call->stream);
	call->stream=NULL;
}
//media stream update
void update_streams(sip_voice_service_t *lc, call_t *call, SalMediaDescription *new_md){
	SalMediaDescription *oldmd=call->resultdesc;
	if (!new_md)return;
	sal_media_description_ref(new_md);
	if(oldmd)sal_media_description_unref(oldmd);
	call->resultdesc=new_md;
	if(!call->stream){
		call->stream=audio_stream_new(call->port,call->port+1);
	}
	call_start_audio_stream(call);
	
}

static void call_received(SalOp *h){
	sip_voice_service_t* s=(sip_voice_service_t *)sal_get_user_pointer(sal_op_get_sal(h));
	//1.if already in call, decline
	if(s->m_call){
		sal_call_decline(h,SalReasonBusy,NULL);
	}
	s->m_call=call_new_incomming(s,h);
	int ret=0;
        if(s->call_cb)
	{
		const char* from=sal_op_get_from(h);
		ret=s->call_cb(s,(char*)from);
	}
	//自动应答
	if(s->auto_answer&&ret>=0){
		accept_call(s,s->m_call);
	}
	else  return;
}
//call ring
static void call_ringing(SalOp *h){	
	int i=0;
}
//call accept
static void call_accepted(SalOp *op){
	sip_voice_service_t *lc=(sip_voice_service_t *)sal_get_user_pointer(sal_op_get_sal(op));
	call_t *call=(call_t*)sal_op_get_user_pointer(op);
	SalMediaDescription *md;
	if (call==NULL){
		ms_warning("No call to accept.");
		return ;
	}
	//最终媒体流
	md=sal_call_get_final_media_description(op);
	if (md && !sal_media_description_empty(md)){
		update_remote_session_id_and_ver(call);
		//更新媒体流
		update_streams (lc,call,md);
	}
}
static void call_ack(SalOp *op){
	sip_voice_service_t *lc=(sip_voice_service_t *)sal_get_user_pointer(sal_op_get_sal(op));
	call_t *call=(call_t*)sal_op_get_user_pointer(op);
	if (call==NULL){
		ms_warning("No call to be ACK'd");
		return ;
	}	
	SalMediaDescription *md=sal_call_get_final_media_description(op);
	if (md && !sal_media_description_empty(md)){
		update_remote_session_id_and_ver(call);
		//更新媒体流
		update_streams (lc,call,md);
	}
}
static void call_terminated(SalOp *op, const char *from){
	sip_voice_service_t *lc=(sip_voice_service_t *)sal_get_user_pointer(sal_op_get_sal(op));
	call_t *call=(call_t*)sal_op_get_user_pointer(op);
	if(lc->close_cb)lc->close_cb(lc);
	call_stop_audio_stream(call);
	call_destroy(call);	
	sal_op_release(op);
	channel_clean(lc->channel);
	lc->m_call=NULL;
}
static void call_failure(SalOp *op, SalError error, SalReason sr, const char *details, int code){
	sip_voice_service_t *lc=(sip_voice_service_t *)sal_get_user_pointer(sal_op_get_sal(op));
	call_t *call=(call_t*)sal_op_get_user_pointer(op);
	if(lc->close_cb)lc->close_cb(lc);
	call_stop_audio_stream(call);
	call_destroy(call);
	sal_op_release(op);
	channel_clean(lc->channel);
	lc->m_call=NULL;
}
static void call_released(SalOp *op){
	sip_voice_service_t *lc=(sip_voice_service_t *)sal_get_user_pointer(sal_op_get_sal(op));
	call_t *call=(call_t*)sal_op_get_user_pointer(op);
	if(lc->close_cb)lc->close_cb(lc);
	if(call){
		call_stop_audio_stream(call);
		call_destroy(call);
	}	
	sal_op_release(op);
	channel_clean(lc->channel);
	lc->m_call=NULL;
}

SalCallbacks sal_callbacks={
	call_received,
	call_ringing,
	call_accepted,
	call_ack,
	NULL,
	call_terminated,
	call_failure,
	call_released,
	NULL
};
