#ifndef __AUDIO_STREAM__INC__HH__
#define __AUDIO_STREAM__INC__HH__

#define MS_MINIMAL_MTU 1500 

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/msticker.h"
#include "mediastreamer2/qualityindicator.h"
#include "ortp/ortp.h"
#include <ortp/event.h>
//audio media stream
typedef struct audio_stream_t{
	//rtp连接
	RtpSession *session;
	OrtpEvQueue *evq;
	MSQualityIndicator *qi;
	

	MSTicker*	ticker,*ticker_r;
	//接收流
	MSFilter*	rtprecv;
	MSFilter*   decoder;
	MSFilter*	output;
	MSFilter*	tee;
	MSFilter*	record;

	//发送流
	MSFilter*   source;
	MSFilter*	encoder;
	MSFilter*	rtpsend;
}audio_stream_t;

//新建
audio_stream_t* audio_stream_new(int loc_rtp_port, int loc_rtcp_port);
//销毁
void audio_stream_destroy(audio_stream_t* s);
//start
int audio_stream_start(audio_stream_t *stream, RtpProfile *profile, const char *rem_rtp_ip,int rem_rtp_port,
	const char *rem_rtcp_ip, int rem_rtcp_port, int jitt_comp,int payload);
//stop audio
void audio_stream_stop(audio_stream_t *s);
#endif
