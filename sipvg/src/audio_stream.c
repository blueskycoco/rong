#include "audio_stream.h"
#include "mediastreamer2/mscodecutils.h"
#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/mssndcard.h"
#include "mschannel.h"
#define RECORD_TO_FILE 0
#define CARD_D "plughw:0,1"

static void disable_checksums(ortp_socket_t sock) {
#if defined(DISABLE_CHECKSUMS) && defined(SO_NO_CHECK)
	int option = 1;
	if (setsockopt(sock, SOL_SOCKET, SO_NO_CHECK, &option, sizeof(option)) == -1) {
		ms_warning("Could not disable udp checksum: %s", strerror(errno));
	}
#endif
}
static RtpSession * create_duplex_rtpsession(int loc_rtp_port, int loc_rtcp_port, bool_t ipv6) {
	RtpSession *rtpr;

	rtpr = rtp_session_new(RTP_SESSION_SENDRECV);
	rtp_session_set_recv_buf_size(rtpr, MAX(ms_get_mtu() , MS_MINIMAL_MTU));
	rtp_session_set_scheduling_mode(rtpr, 0);
	rtp_session_set_blocking_mode(rtpr, 0);
	rtp_session_enable_adaptive_jitter_compensation(rtpr, TRUE);
	rtp_session_set_symmetric_rtp(rtpr, TRUE);
	rtp_session_set_local_addr(rtpr, ipv6 ? "::" : "0.0.0.0", loc_rtp_port, loc_rtcp_port);
	rtp_session_signal_connect(rtpr, "timestamp_jump", (RtpCallback)rtp_session_resync, (long)NULL);
	rtp_session_signal_connect(rtpr, "ssrc_changed", (RtpCallback)rtp_session_resync, (long)NULL);
	rtp_session_set_ssrc_changed_threshold(rtpr, 0);
	rtp_session_set_rtcp_report_interval(rtpr, 2500);	/* At the beginning of the session send more reports. */
	disable_checksums(rtp_session_get_rtp_socket(rtpr));

	return rtpr;
}

//新建
audio_stream_t* audio_stream_new(int loc_rtp_port, int loc_rtcp_port){
	audio_stream_t* s=ms_new0(audio_stream_t,1);
	s->session=create_duplex_rtpsession(loc_rtp_port,loc_rtcp_port,false);
	s->qi=ms_quality_indicator_new(s->session);
	s->evq=ortp_ev_queue_new();
	rtp_session_register_event_queue(s->session,s->evq);

	s->rtpsend=ms_filter_new(MS_RTP_SEND_ID);
	s->rtprecv=ms_filter_new(MS_RTP_RECV_ID);
	s->source=ms_filter_new((MSFilterId)MS_SOURCE_CHANNEL_ID);
	s->output=ms_filter_new((MSFilterId)MS_OUTPUT_CHANNEL_ID);
	s->tee=ms_filter_new(MS_TEE_ID);
	#if RECORD_TO_FILE
	s->record=ms_filter_new(MS_FILE_REC_ID);
	#else
	
	MSSndCard *card_playback = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),CARD_D);
	if(card_playback==NULL)
	{
		ms_snd_card_manager_add_card(ms_snd_card_manager_get(),ms_alsa_card_new_custom(CARD_D,CARD_D));
		card_playback = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),CARD_D);
	}
	s->record=ms_snd_card_create_writer(card_playback);
	int rate = 8000;
	ms_filter_call_method (s->record, MS_FILTER_SET_SAMPLE_RATE,	&rate);
	#endif
	s->ticker=ms_ticker_new();
	ms_ticker_set_name(s->ticker,"audio send ticker");

	s->ticker_r=ms_ticker_new();
	ms_ticker_set_name(s->ticker_r,"audio recv ticker");
	return s;
}
//销毁
void audio_stream_destroy(audio_stream_t* s){
	if (s->session != NULL) {
		rtp_session_unregister_event_queue(s->session, s->evq);
		rtp_session_destroy(s->session);
	}
	if(s->evq)ortp_ev_queue_destroy(s->evq);
	if(s->qi)ms_quality_indicator_destroy(s->qi);

	if(s->ticker)ms_ticker_destroy(s->ticker);

	if(s->source)ms_filter_destroy(s->source);	
	if(s->encoder)ms_filter_destroy(s->encoder);
	if(s->rtpsend)ms_filter_destroy(s->rtpsend);	

	if(s->rtprecv)ms_filter_destroy(s->rtprecv);	
	if(s->decoder)ms_filter_destroy(s->decoder);
	if (s->output)ms_filter_destroy(s->output);
	if(s->tee)ms_filter_destroy(s->tee);
	if(s->record)ms_filter_destroy(s->record);
}
/*invoked from FEC capable filters*/
static  mblk_t* audio_stream_payload_picker(MSRtpPayloadPickerContext* context,unsigned int sequence_number) {
	return rtp_session_pick_with_cseq(((audio_stream_t*)(context->filter_graph_manager))->session, sequence_number);
}
int audio_stream_start(audio_stream_t *s, RtpProfile *profile, const char *rem_rtp_ip,int rem_rtp_port,
		const char *rem_rtcp_ip, int rem_rtcp_port, int jitt_comp,int payload){
	RtpSession *rtps=s->session;
	PayloadType *pt=NULL;
	MSRtpPayloadPickerContext picker_context;
	int sample_rate;
	//设置rtp
	rtp_session_set_profile(rtps,profile);
	if (rem_rtp_port>0) rtp_session_set_remote_addr_full(rtps,rem_rtp_ip,rem_rtp_port,rem_rtcp_ip,rem_rtcp_port);
	if (rem_rtcp_port<=0){
		rtp_session_enable_rtcp(rtps,FALSE);
	}
	rtp_session_set_payload_type(rtps,payload);
	rtp_session_set_jitter_compensation(rtps,jitt_comp);
	if (rem_rtp_port>0)
		ms_filter_call_method(s->rtpsend,MS_RTP_SEND_SET_SESSION,rtps);
	ms_filter_call_method(s->rtprecv,MS_RTP_RECV_SET_SESSION,rtps);
	//获取媒体流
	pt=rtp_profile_get_payload(profile,payload);
	if (pt==NULL){
		ms_error("audiostream.c: undefined payload type.");
		return -1;
	}
	//获取采样率
	if (ms_filter_call_method(s->rtpsend,MS_FILTER_GET_SAMPLE_RATE,&sample_rate)!=0){
		ms_error("Sample rate is unknown for RTP side !");
		return -1;
	}
	//编解码器
	if(s->encoder)ms_filter_destroy(s->encoder);
	s->encoder=ms_filter_create_encoder(pt->mime_type);
	if(s->decoder)ms_filter_destroy(s->decoder);	
	s->decoder=ms_filter_create_decoder(pt->mime_type);

	if (ms_filter_has_method(s->decoder, MS_FILTER_SET_RTP_PAYLOAD_PICKER)) {
		ms_message(" decoder has FEC capabilities");
		picker_context.filter_graph_manager=s;
		picker_context.picker=&audio_stream_payload_picker;
		ms_filter_call_method(s->decoder,MS_FILTER_SET_RTP_PAYLOAD_PICKER, &picker_context);
	}

	/* give the encoder/decoder some parameters*/
	ms_filter_call_method(s->encoder,MS_FILTER_SET_SAMPLE_RATE,&sample_rate);
	ms_filter_call_method(s->encoder,MS_FILTER_SET_NCHANNELS,&pt->channels);

	ms_filter_call_method(s->decoder,MS_FILTER_SET_SAMPLE_RATE,&sample_rate);
	ms_filter_call_method(s->decoder,MS_FILTER_SET_NCHANNELS,&pt->channels);

	if (pt->send_fmtp!=NULL) {
		char value[16]={0};
		int ptime;
		if (ms_filter_has_method(s->encoder,MS_AUDIO_ENCODER_SET_PTIME)){
			if (fmtp_get_value(pt->send_fmtp,"ptime",value,sizeof(value)-1)){
				ptime=atoi(value);
				ms_filter_call_method(s->encoder,MS_AUDIO_ENCODER_SET_PTIME,&ptime);
			}
		}
		ms_filter_call_method(s->encoder,MS_FILTER_ADD_FMTP, (void*)pt->send_fmtp);
	}
	if (pt->recv_fmtp!=NULL) ms_filter_call_method(s->decoder,MS_FILTER_ADD_FMTP,(void*)pt->recv_fmtp);

	/*configure resampler if needed*/
	ms_filter_call_method(s->rtpsend, MS_FILTER_SET_NCHANNELS, &pt->channels);
	ms_filter_call_method(s->rtprecv, MS_FILTER_SET_NCHANNELS, &pt->channels);

	MSConnectionHelper h;
	ms_connection_helper_start(&h);
	ms_connection_helper_link(&h,s->rtprecv,-1,0);
	ms_connection_helper_link(&h,s->decoder,0,0);
	ms_connection_helper_link(&h,s->tee,0,0);
	ms_connection_helper_link(&h,s->output,0,-1);

	ms_filter_link(s->tee,1,s->record,0);

	ms_connection_helper_start(&h);
	ms_connection_helper_link(&h,s->source,-1,0);
	ms_connection_helper_link(&h,s->encoder,0,0);
	ms_connection_helper_link(&h,s->rtpsend,0,-1);

	ms_ticker_attach(s->ticker_r,s->rtprecv);
	ms_ticker_attach(s->ticker,s->source);
	return 0;
}
//stop audio
void audio_stream_stop(audio_stream_t *s){
	ms_ticker_detach(s->ticker_r,s->rtprecv);
	ms_ticker_detach(s->ticker,s->source);

	MSConnectionHelper h;
	ms_connection_helper_start(&h);
	ms_connection_helper_unlink(&h,s->rtprecv,-1,0);
	ms_connection_helper_unlink(&h,s->decoder,0,0);
	ms_connection_helper_unlink(&h,s->tee,0,0);
	ms_connection_helper_unlink(&h,s->output,0,-1);

	ms_filter_unlink(s->tee,1,s->record,0);

	ms_connection_helper_start(&h);
	ms_connection_helper_unlink(&h,s->source,-1,0);
	ms_connection_helper_unlink(&h,s->encoder,0,0);
	ms_connection_helper_unlink(&h,s->rtpsend,0,-1);
}
