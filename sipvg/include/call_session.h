#ifndef __CALL_SESSION__INC__HH__
#define __CALL_SESSION__INC__HH__
#include "sal/sal.h"
#include "audio_stream.h"

typedef enum  {CallOutgoing,CallIncoming} CallDir;
struct sip_voice_service_t;

#define CALL_IPADDR_SIZE 50

typedef struct StunCandidate{
	char addr[64];
	int port;
}StunCandidate;
//КєНа
typedef struct call_t{
	//service
	sip_voice_service_t *core;
	//local media
	SalMediaDescription *localdesc;
	//result media
	SalMediaDescription *resultdesc;

	//audio stream
	audio_stream_t* stream;
	//call dir
	CallDir dir;
	char localip[CALL_IPADDR_SIZE]; /* our best guess for local ipaddress for this call */
	//profile
	struct _RtpProfile *profile;
	PayloadType *codec;
	//call op
	SalOp *op;
	//media port
	int port;
	//call start time
	time_t start_time;
	unsigned int remote_session_id;
	unsigned int remote_session_ver;
	StunCandidate ac,vc;
}call_t;

//connect channel
void connect_channels(call_t* call);
void disconnect_channels(call_t* call);
//new call
call_t* call_new_incomming(sip_voice_service_t* s,SalOp* op);
//destroy
void call_destroy(call_t* call);
//accept call
int accept_call(sip_voice_service_t *lc, call_t *call);
//terminate call
void terminate_call(sip_voice_service_t *lc);
#endif