#ifndef __SIP_VOICE__INC__HH__
#define __SIP_VOICE__INC__HH__

# define __BEGIN_DECLS
# define __END_DECLS

#include "sal/sal.h"
#include "call_session.h"
#include "channelsys.h"

//数据回调入口
typedef void (*on_audio_data_cb)(void* ud,uint8_t* data,int len);
typedef void (*on_session_close_cb)(sip_voice_service_t* s);
typedef int (*on_session_created_cb)(sip_voice_service_t* s,char* caller);

struct call_t;
typedef struct call_t call_t;

#define RANK_END 10000
#define PAYLOAD_TYPE_ENABLED	PAYLOAD_TYPE_USER_FLAG_0

typedef struct codec_desc{
	const char *name;
	int rate;
}codec_desc_t;

static codec_desc_t codec_pref_order[]={
	{"pcmu",8000},
	{"pcma",8000},
	{NULL,0}
};


//语音服务
typedef struct sip_voice_service_t{
	Sal *sal;
	RtpProfile *default_profile;
	int dyn_pt;
	MSList *payload_types;

	//语音编码列表
	MSList *codecs;
	//当前呼叫
	call_t *m_call;
	
	//信令监听端口,5060
	int udp_port;

	//自动应答
	int auto_answer;

	//监听线程
	int running;
	ms_thread_t _th;

	//通道
	channel_t* channel;

	//数据回调
	on_audio_data_cb cb;
	on_session_close_cb close_cb;
        on_session_created_cb call_cb;
	int debug;
	void* ud;
}sip_voice_service_t;

//新建
sip_voice_service_t* sip_voice_service_new(void* ud);
//设置语音数据回调
void sip_voice_service_set_audio_data_cb(sip_voice_service_t* s,on_audio_data_cb cb,on_session_close_cb close_cb,on_session_created_cb call_cb);
//设置语音数据
void sip_voice_service_set_audio_data(sip_voice_service_t* s,uint8_t* data,int len);
//销毁
void sip_voice_service_destroy(sip_voice_service_t* s);

//for testing
void sip_voice_service_set_rec_filename(sip_voice_service_t* s,const char* filename);
//start recording
void sip_voice_service_rec_start(sip_voice_service_t* s);
//stop recording;
void sip_voice_service_rec_stop(sip_voice_service_t* s);
#endif
