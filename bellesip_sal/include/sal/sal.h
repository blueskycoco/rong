/*
linphone
Copyright (C) 2010  Simon MORLAT (simon.morlat@free.fr)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/** 
 This header files defines the Signaling Abstraction Layer.
 The purpose of this layer is too allow experiment different call signaling 
 protocols and implementations under linphone, for example SIP, JINGLE...
**/

#ifndef sal_h
#define sal_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#ifdef __cplusplus
extern "C"
{
#endif

#include "mediastreamer2/mscommon.h"
#include "ortp/ortp_srtp.h"

#ifndef LINPHONE_PUBLIC
	#define LINPHONE_PUBLIC MS2_PUBLIC
#endif

/*Dirty hack, keep in sync with mediastreamer2/include/mediastream.h */
#ifndef PAYLOAD_TYPE_FLAG_CAN_RECV
#define PAYLOAD_TYPE_FLAG_CAN_RECV	PAYLOAD_TYPE_USER_FLAG_1
#define PAYLOAD_TYPE_FLAG_CAN_SEND	PAYLOAD_TYPE_USER_FLAG_2
#endif
struct Sal;

typedef struct Sal Sal;

struct SalOp;

typedef struct SalOp SalOp;

struct SalAddress;

typedef struct SalAddress SalAddress;

struct SalCustomHeader;

typedef struct SalCustomHeader SalCustomHeader;

struct addrinfo;

typedef enum {
	SalTransportUDP, /*UDP*/
	SalTransportTCP, /*TCP*/
	SalTransportTLS, /*TLS*/
	SalTransportDTLS /*DTLS*/
}SalTransport;

#define SAL_MEDIA_DESCRIPTION_UNCHANGED		0x00
#define SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED	0x01
#define SAL_MEDIA_DESCRIPTION_CODEC_CHANGED	0x02
#define SAL_MEDIA_DESCRIPTION_CRYPTO_CHANGED	0x04
#define SAL_MEDIA_DESCRIPTION_CHANGED		(SAL_MEDIA_DESCRIPTION_NETWORK_CHANGED | SAL_MEDIA_DESCRIPTION_CODEC_CHANGED | SAL_MEDIA_DESCRIPTION_CRYPTO_CHANGED)

MS2_PUBLIC const char* sal_transport_to_string(SalTransport transport);
MS2_PUBLIC SalTransport sal_transport_parse(const char*);
/* Address manipulation API*/
MS2_PUBLIC SalAddress * sal_address_new(const char *uri);
MS2_PUBLIC SalAddress * sal_address_clone(const SalAddress *addr);
MS2_PUBLIC SalAddress * sal_address_ref(SalAddress *addr);
MS2_PUBLIC void sal_address_unref(SalAddress *addr);
MS2_PUBLIC const char *sal_address_get_scheme(const SalAddress *addr);
MS2_PUBLIC const char *sal_address_get_display_name(const SalAddress* addr);
MS2_PUBLIC const char *sal_address_get_display_name_unquoted(const SalAddress *addr);
MS2_PUBLIC const char *sal_address_get_username(const SalAddress *addr);
MS2_PUBLIC const char *sal_address_get_domain(const SalAddress *addr);
#ifdef USE_BELLESIP
MS2_PUBLIC int sal_address_get_port(const SalAddress *addr);
#else
const char * sal_address_get_port(const SalAddress *addr);
int sal_address_get_port_int(const SalAddress *addr);
#endif
MS2_PUBLIC SalTransport sal_address_get_transport(const SalAddress* addr);
MS2_PUBLIC const char* sal_address_get_transport_name(const SalAddress* addr);

MS2_PUBLIC void sal_address_set_display_name(SalAddress *addr, const char *display_name);
MS2_PUBLIC void sal_address_set_username(SalAddress *addr, const char *username);
MS2_PUBLIC void sal_address_set_domain(SalAddress *addr, const char *host);
#ifdef USE_BELLESIP
MS2_PUBLIC void sal_address_set_port(SalAddress *uri, int port);
#else
void sal_address_set_port(SalAddress *addr, const char *port);
void sal_address_set_port_int(SalAddress *uri, int port);
#endif
MS2_PUBLIC void sal_address_clean(SalAddress *addr);
MS2_PUBLIC char *sal_address_as_string(const SalAddress *u);
MS2_PUBLIC char *sal_address_as_string_uri_only(const SalAddress *u);
MS2_PUBLIC void sal_address_destroy(SalAddress *u);
MS2_PUBLIC void sal_address_set_param(SalAddress *u,const char* name,const char* value);
MS2_PUBLIC void sal_address_set_transport(SalAddress* addr,SalTransport transport);
MS2_PUBLIC void sal_address_set_transport_name(SalAddress* addr,const char* transport);

MS2_PUBLIC Sal * sal_init();
MS2_PUBLIC void sal_uninit(Sal* sal);
MS2_PUBLIC void sal_set_user_pointer(Sal *sal, void *user_data);
MS2_PUBLIC void *sal_get_user_pointer(const Sal *sal);


typedef enum {
	SalAudio,
	SalVideo,
	SalH239,
	SalOther
} SalStreamType;
const char* sal_stream_type_to_string(SalStreamType type);

typedef enum{
	SalProtoUnknown,
	SalProtoRtpAvp,
	SalProtoRtpSavp
}SalMediaProto;
const char* sal_media_proto_to_string(SalMediaProto type);

typedef enum{
	SalStreamSendRecv,
	SalStreamSendOnly,
	SalStreamRecvOnly,
	SalStreamInactive
}SalStreamDir;
const char* sal_stream_dir_to_string(SalStreamDir type);


#define SAL_ENDPOINT_CANDIDATE_MAX 2

#define SAL_MEDIA_DESCRIPTION_MAX_ICE_ADDR_LEN 64
#define SAL_MEDIA_DESCRIPTION_MAX_ICE_FOUNDATION_LEN 32
#define SAL_MEDIA_DESCRIPTION_MAX_ICE_TYPE_LEN 6

typedef struct SalIceCandidate {
	char addr[SAL_MEDIA_DESCRIPTION_MAX_ICE_ADDR_LEN];
	char raddr[SAL_MEDIA_DESCRIPTION_MAX_ICE_ADDR_LEN];
	char foundation[SAL_MEDIA_DESCRIPTION_MAX_ICE_FOUNDATION_LEN];
	char type[SAL_MEDIA_DESCRIPTION_MAX_ICE_TYPE_LEN];
	unsigned int componentID;
	unsigned int priority;
	int port;
	int rport;
} SalIceCandidate;

#define SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES 10

typedef struct SalIceRemoteCandidate {
	char addr[SAL_MEDIA_DESCRIPTION_MAX_ICE_ADDR_LEN];
	int port;
} SalIceRemoteCandidate;

#define SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES 2

#define SAL_MEDIA_DESCRIPTION_MAX_ICE_UFRAG_LEN 256
#define SAL_MEDIA_DESCRIPTION_MAX_ICE_PWD_LEN 256

typedef struct SalSrtpCryptoAlgo {
	unsigned int tag;
	enum ortp_srtp_crypto_suite_t algo;
	/* 41= 40 max(key_length for all algo) + '\0' */
	char master_key[41];
} SalSrtpCryptoAlgo;

#define SAL_CRYPTO_ALGO_MAX 4

typedef struct SalStreamDescription{
	SalMediaProto proto;
	SalStreamType type;
	char typeother[32];
	char rtp_addr[64];
	char rtcp_addr[64];
	int rtp_port;
	int rtcp_port;
	MSList *payloads; //<list of PayloadType
	int bandwidth;
	int ptime;
	SalStreamDir dir;
	SalSrtpCryptoAlgo crypto[SAL_CRYPTO_ALGO_MAX];
	unsigned int crypto_local_tag;
	int max_rate;
	SalIceCandidate ice_candidates[SAL_MEDIA_DESCRIPTION_MAX_ICE_CANDIDATES];
	SalIceRemoteCandidate ice_remote_candidates[SAL_MEDIA_DESCRIPTION_MAX_ICE_REMOTE_CANDIDATES];
	char ice_ufrag[SAL_MEDIA_DESCRIPTION_MAX_ICE_UFRAG_LEN];
	char ice_pwd[SAL_MEDIA_DESCRIPTION_MAX_ICE_PWD_LEN];
	bool_t ice_mismatch;
	bool_t ice_completed;
} SalStreamDescription;

#define SAL_MEDIA_DESCRIPTION_MAX_STREAMS 4

typedef struct SalMediaDescription{
	int refcount;
	char addr[64];
	char username[64];
	int n_active_streams;
	int n_total_streams;
	int bandwidth;
	unsigned int session_ver;
	unsigned int session_id;
	SalStreamDescription streams[SAL_MEDIA_DESCRIPTION_MAX_STREAMS];
	char ice_ufrag[SAL_MEDIA_DESCRIPTION_MAX_ICE_UFRAG_LEN];
	char ice_pwd[SAL_MEDIA_DESCRIPTION_MAX_ICE_PWD_LEN];
	bool_t ice_lite;
	bool_t ice_completed;
} SalMediaDescription;

typedef struct SalMessage{
	const char *from;
	const char *text;
	const char *url;
	const char *message_id;
	time_t time;
}SalMessage;

#define SAL_MEDIA_DESCRIPTION_MAX_MESSAGE_ATTRIBUTES 5

MS2_PUBLIC SalMediaDescription *sal_media_description_new();
MS2_PUBLIC void sal_media_description_ref(SalMediaDescription *md);
MS2_PUBLIC void sal_media_description_unref(SalMediaDescription *md);
MS2_PUBLIC bool_t sal_media_description_empty(const SalMediaDescription *md);
MS2_PUBLIC int sal_media_description_equals(const SalMediaDescription *md1, const SalMediaDescription *md2);
MS2_PUBLIC bool_t sal_media_description_has_dir(const SalMediaDescription *md, SalStreamDir dir);
MS2_PUBLIC SalStreamDescription *sal_media_description_find_stream(SalMediaDescription *md,
    SalMediaProto proto, SalStreamType type);
MS2_PUBLIC void sal_media_description_set_dir(SalMediaDescription *md, SalStreamDir stream_dir);

/*this structure must be at the first byte of the SalOp structure defined by implementors*/
typedef struct SalOpBase{
	Sal *root;
	char *route; /*or request-uri for REGISTER*/
	MSList* route_addresses; /*list of SalAddress* */
#ifndef USE_BELLESIP
	char *contact;
#else
	SalAddress* contact_address;
#endif
	char *from;
	SalAddress* from_address;
	char *to;
	SalAddress* to_address;
	char *origin;
	SalAddress* origin_address;
	char *remote_ua;
	SalMediaDescription *local_media;
	SalMediaDescription *remote_media;
	void *user_pointer;
	const char* call_id;
	char *remote_contact;
	SalAddress* service_route; /*as defined by rfc3608, might be a list*/
	SalCustomHeader *sent_custom_headers;
	SalCustomHeader *recv_custom_headers;
} SalOpBase;


typedef enum SalError{
	SalErrorNone,
	SalErrorNoResponse,
	SalErrorProtocol,
	SalErrorFailure, /* see SalReason for more details */
	SalErrorUnknown
} SalError;

typedef enum SalReason{
	SalReasonDeclined,
	SalReasonBusy,
	SalReasonRedirect,
	SalReasonTemporarilyUnavailable,
	SalReasonNotFound,
	SalReasonDoNotDisturb,
	SalReasonMedia,
	SalReasonForbidden,
	SalReasonUnknown,
	SalReasonServiceUnavailable,
	SalReasonRequestPending,
	SalReasonUnauthorized,
	SalReasonNotAcceptable
}SalReason;

const char* sal_reason_to_string(const SalReason reason);

typedef enum SalPresenceStatus{
	SalPresenceOffline,
	SalPresenceOnline,
	SalPresenceBusy,
	SalPresenceBerightback,
	SalPresenceAway,
	SalPresenceOnthephone,
	SalPresenceOuttolunch,
	SalPresenceDonotdisturb,
	SalPresenceMoved,
	SalPresenceAltService,
	SalPresenceOnVacation
}SalPresenceStatus;

struct _SalPresenceModel;
typedef struct _SalPresenceModel SalPresenceModel;

const char* sal_presence_status_to_string(const SalPresenceStatus status);

typedef enum SalReferStatus{
	SalReferTrying,
	SalReferSuccess,
	SalReferFailed
}SalReferStatus;

typedef enum SalSubscribeStatus{
	SalSubscribeNone,
	SalSubscribePending,
	SalSubscribeActive,
	SalSubscribeTerminated
}SalSubscribeStatus;

typedef enum SalTextDeliveryStatus{
	SalTextDeliveryInProgress,
	SalTextDeliveryDone,
	SalTextDeliveryFailed
}SalTextDeliveryStatus;

typedef struct SalAuthInfo{
	char *username;
	char *userid;
	char *password;
	char *realm;
	char *domain;
	char *ha1;
}SalAuthInfo;

typedef struct SalBody{
	const char *type;
	const char *subtype;
	const void *data;
	size_t size;
}SalBody;

typedef void (*SalOnCallReceived)(SalOp *op);
typedef void (*SalOnCallRinging)(SalOp *op);
typedef void (*SalOnCallAccepted)(SalOp *op);
typedef void (*SalOnCallAck)(SalOp *op);
typedef void (*SalOnCallUpdating)(SalOp *op);/*< Called when a reINVITE is received*/
typedef void (*SalOnCallTerminated)(SalOp *op, const char *from);
typedef void (*SalOnCallFailure)(SalOp *op, SalError error, SalReason reason, const char *details, int code);
typedef void (*SalOnCallReleased)(SalOp *salop);
typedef void (*SalOnAuthRequestedLegacy)(SalOp *op, const char *realm, const char *username);
typedef bool_t (*SalOnAuthRequested)(Sal *sal,SalAuthInfo* info);
typedef void (*SalOnAuthFailure)(SalOp *op, SalAuthInfo* info);
typedef void (*SalOnRegisterSuccess)(SalOp *op, bool_t registered);
typedef void (*SalOnRegisterFailure)(SalOp *op, SalError error, SalReason reason, const char *details);
typedef void (*SalOnVfuRequest)(SalOp *op);
typedef void (*SalOnDtmfReceived)(SalOp *op, char dtmf);
typedef void (*SalOnRefer)(Sal *sal, SalOp *op, const char *referto);
typedef void (*SalOnTextReceived)(SalOp *op, const SalMessage *msg);
typedef void (*SalOnTextDeliveryUpdate)(SalOp *op, SalTextDeliveryStatus status);
typedef void (*SalOnNotifyRefer)(SalOp *op, SalReferStatus state);
typedef void (*SalOnSubscribeResponse)(SalOp *op, SalSubscribeStatus status, SalError error, SalReason reason);
typedef void (*SalOnNotify)(SalOp *op, SalSubscribeStatus status, const char *event, const SalBody *body);
typedef void (*SalOnSubscribeReceived)(SalOp *salop, const char *event, const SalBody *body);
typedef void (*SalOnSubscribeClosed)(SalOp *salop);
typedef void (*SalOnParsePresenceRequested)(SalOp *salop, const char *content_type, const char *content_subtype, const char *content, SalPresenceModel **result);
typedef void (*SalOnConvertPresenceToXMLRequested)(SalOp *salop, SalPresenceModel *presence, const char *contact, char **content);
typedef void (*SalOnNotifyPresence)(SalOp *op, SalSubscribeStatus ss, SalPresenceModel *model, const char *msg);
typedef void (*SalOnSubscribePresenceReceived)(SalOp *salop, const char *from);
typedef void (*SalOnSubscribePresenceClosed)(SalOp *salop, const char *from);
typedef void (*SalOnPingReply)(SalOp *salop);
typedef void (*SalOnInfoReceived)(SalOp *salop, const SalBody *body);
typedef void (*SalOnPublishResponse)(SalOp *salop, SalError error, SalReason reason);
typedef void (*SalOnExpire)(SalOp *salop);
/*allows sal implementation to access auth info if available, return TRUE if found*/
//白板支持 accepted=1表示接受,0--表示拒绝
typedef void (*SalOnPresRequestAnswered)(SalOp *salop,int accepted);
//白板请求
typedef void (*SalOnPresRequest)(SalOp *salop,int req);


typedef struct SalCallbacks{
	SalOnCallReceived call_received;
	SalOnCallRinging call_ringing;
	SalOnCallAccepted call_accepted;
	SalOnCallAck call_ack;
	SalOnCallUpdating call_updating;
	SalOnCallTerminated call_terminated;
	SalOnCallFailure call_failure;
	SalOnCallReleased call_released;
	SalOnAuthFailure auth_failure;
	SalOnRegisterSuccess register_success;
	SalOnRegisterFailure register_failure;
	SalOnVfuRequest vfu_request;
	SalOnDtmfReceived dtmf_received;
	SalOnRefer refer_received;
	SalOnTextReceived text_received;
	SalOnTextDeliveryUpdate text_delivery_update;
	SalOnNotifyRefer notify_refer;
	SalOnSubscribeReceived subscribe_received;
	SalOnSubscribeClosed subscribe_closed;
	SalOnSubscribeResponse subscribe_response;
	SalOnNotify notify;
	SalOnSubscribePresenceReceived subscribe_presence_received;
	SalOnSubscribePresenceClosed subscribe_presence_closed;
	SalOnParsePresenceRequested parse_presence_requested;
	SalOnConvertPresenceToXMLRequested convert_presence_to_xml_requested;
	SalOnNotifyPresence notify_presence;
	SalOnPingReply ping_reply;
	SalOnAuthRequested auth_requested;
	SalOnInfoReceived info_received;
	SalOnPublishResponse on_publish_response;
	SalOnExpire on_expire;
	//pre support
	SalOnPresRequestAnswered on_pres_req_answered;
	SalOnPresRequest on_pres_reqest;
}SalCallbacks;



MS2_PUBLIC SalAuthInfo* sal_auth_info_new();
MS2_PUBLIC SalAuthInfo* sal_auth_info_clone(const SalAuthInfo* auth_info);
MS2_PUBLIC void sal_auth_info_delete(SalAuthInfo* auth_info);
MS2_PUBLIC int sal_auth_compute_ha1(const char* userid,const char* realm,const char* password, char ha1[33]);

MS2_PUBLIC void sal_set_callbacks(Sal *ctx, const SalCallbacks *cbs);
MS2_PUBLIC int sal_listen_port(Sal *ctx, const char *addr, int port, SalTransport tr, int is_secure);
MS2_PUBLIC int sal_unlisten_ports(Sal *ctx);
MS2_PUBLIC void sal_set_dscp(Sal *ctx, int dscp);
MS2_PUBLIC int sal_reset_transports(Sal *ctx);
MS2_PUBLIC ortp_socket_t sal_get_socket(Sal *ctx);
MS2_PUBLIC void sal_set_user_agent(Sal *ctx, const char *user_agent);
MS2_PUBLIC void sal_append_stack_string_to_user_agent(Sal *ctx);
/*keepalive period in ms*/
MS2_PUBLIC void sal_set_keepalive_period(Sal *ctx,unsigned int value);
MS2_PUBLIC void sal_use_tcp_tls_keepalive(Sal *ctx, bool_t enabled);
MS2_PUBLIC int sal_enable_tunnel(Sal *ctx, void *tunnelclient);
MS2_PUBLIC void sal_disable_tunnel(Sal *ctx);
/**
 * returns keepalive period in ms
 * 0 desactiaved
 * */
MS2_PUBLIC unsigned int sal_get_keepalive_period(Sal *ctx);
MS2_PUBLIC void sal_use_session_timers(Sal *ctx, int expires);
MS2_PUBLIC void sal_use_dates(Sal *ctx, bool_t enabled);
MS2_PUBLIC void sal_use_one_matching_codec_policy(Sal *ctx, bool_t one_matching_codec);
MS2_PUBLIC void sal_use_rport(Sal *ctx, bool_t use_rports);
MS2_PUBLIC void sal_enable_auto_contacts(Sal *ctx, bool_t enabled);
MS2_PUBLIC void sal_set_root_ca(Sal* ctx, const char* rootCa);
MS2_PUBLIC const char *sal_get_root_ca(Sal* ctx);
MS2_PUBLIC void sal_verify_server_certificates(Sal *ctx, bool_t verify);
MS2_PUBLIC void sal_verify_server_cn(Sal *ctx, bool_t verify);
MS2_PUBLIC void sal_set_uuid(Sal*ctx, const char *uuid);
MS2_PUBLIC int sal_create_uuid(Sal*ctx, char *uuid, size_t len);
MS2_PUBLIC void sal_enable_test_features(Sal*ctx, bool_t enabled);

MS2_PUBLIC int sal_iterate(Sal *sal);
MS2_PUBLIC MSList * sal_get_pending_auths(Sal *sal);

/*create an operation */
MS2_PUBLIC SalOp * sal_op_new(Sal *sal);

/*generic SalOp API, working for all operations */
MS2_PUBLIC Sal *sal_op_get_sal(const SalOp *op);
#ifndef USE_BELLESIP
MS2_PUBLIC void sal_op_set_contact(SalOp *op, const char *contact);
#else
#define sal_op_set_contact sal_op_set_contact_address /*for liblinphone compatibility*/
MS2_PUBLIC void sal_op_set_contact_address(SalOp *op, const SalAddress* address);
#endif
MS2_PUBLIC void sal_op_set_route(SalOp *op, const char *route);
MS2_PUBLIC void sal_op_set_route_address(SalOp *op, const SalAddress* address);
MS2_PUBLIC void sal_op_add_route_address(SalOp *op, const SalAddress* address);
MS2_PUBLIC void sal_op_set_from(SalOp *op, const char *from);
MS2_PUBLIC void sal_op_set_from_address(SalOp *op, const SalAddress *from);
MS2_PUBLIC void sal_op_set_to(SalOp *op, const char *to);
MS2_PUBLIC void sal_op_set_to_address(SalOp *op, const SalAddress *to);
MS2_PUBLIC SalOp *sal_op_ref(SalOp* h);
MS2_PUBLIC void sal_op_release(SalOp *h);
MS2_PUBLIC void sal_op_authenticate(SalOp *h, const SalAuthInfo *info);
MS2_PUBLIC void sal_op_cancel_authentication(SalOp *h);
MS2_PUBLIC void sal_op_set_user_pointer(SalOp *h, void *up);
MS2_PUBLIC SalAuthInfo * sal_op_get_auth_requested(SalOp *h);
MS2_PUBLIC const char *sal_op_get_from(const SalOp *op);
MS2_PUBLIC const SalAddress *sal_op_get_from_address(const SalOp *op);
MS2_PUBLIC const char *sal_op_get_to(const SalOp *op);
MS2_PUBLIC const SalAddress *sal_op_get_to_address(const SalOp *op);
#ifndef USE_BELLESIP
MS2_PUBLIC const char *sal_op_get_contact(const SalOp *op);
#else
MS2_PUBLIC const SalAddress *sal_op_get_contact_address(const SalOp *op);
#define sal_op_get_contact sal_op_get_contact_address /*for liblinphone compatibility*/
#endif
MS2_PUBLIC const char *sal_op_get_route(const SalOp *op);
MS2_PUBLIC const MSList* sal_op_get_route_addresses(const SalOp *op);
MS2_PUBLIC const char *sal_op_get_proxy(const SalOp *op);
MS2_PUBLIC const char *sal_op_get_remote_contact(const SalOp *op);
/*for incoming requests, returns the origin of the packet as a sip uri*/
MS2_PUBLIC const char *sal_op_get_network_origin(const SalOp *op);
MS2_PUBLIC const SalAddress *sal_op_get_network_origin_address(const SalOp *op);
/*returns far-end "User-Agent" string */
MS2_PUBLIC const char *sal_op_get_remote_ua(const SalOp *op);
MS2_PUBLIC void *sal_op_get_user_pointer(const SalOp *op);
MS2_PUBLIC const char* sal_op_get_call_id(const SalOp *op);

MS2_PUBLIC const SalAddress* sal_op_get_service_route(const SalOp *op);
MS2_PUBLIC void sal_op_set_service_route(SalOp *op,const SalAddress* service_route);

MS2_PUBLIC void sal_op_set_manual_refresher_mode(SalOp *op, bool_t enabled);

/*Call API*/
MS2_PUBLIC int sal_call_set_local_media_description(SalOp *h, SalMediaDescription *desc);
MS2_PUBLIC int sal_call(SalOp *h, const char *from, const char *to);
MS2_PUBLIC int sal_call_notify_ringing(SalOp *h, bool_t early_media);
/*accept an incoming call or, during a call accept a reINVITE*/
MS2_PUBLIC int sal_call_accept(SalOp*h);
MS2_PUBLIC int sal_call_decline(SalOp *h, SalReason reason, const char *redirection /*optional*/);
MS2_PUBLIC int sal_call_update(SalOp *h, const char *subject);
MS2_PUBLIC SalMediaDescription * sal_call_get_remote_media_description(SalOp *h);
MS2_PUBLIC SalMediaDescription * sal_call_get_final_media_description(SalOp *h);
MS2_PUBLIC int sal_call_refer(SalOp *h, const char *refer_to);
MS2_PUBLIC int sal_call_refer_with_replaces(SalOp *h, SalOp *other_call_h);
MS2_PUBLIC int sal_call_accept_refer(SalOp *h);
/*informs this call is consecutive to an incoming refer */
MS2_PUBLIC int sal_call_set_referer(SalOp *h, SalOp *refered_call);
/* returns the SalOp of a call that should be replaced by h, if any */
MS2_PUBLIC SalOp *sal_call_get_replaces(SalOp *h);
MS2_PUBLIC int sal_call_send_dtmf(SalOp *h, char dtmf);
MS2_PUBLIC int sal_call_terminate(SalOp *h);
MS2_PUBLIC bool_t sal_call_autoanswer_asked(SalOp *op);
MS2_PUBLIC void sal_call_send_vfu_request(SalOp *h);
MS2_PUBLIC int sal_call_is_offerer(const SalOp *h);
MS2_PUBLIC int sal_call_notify_refer_state(SalOp *h, SalOp *newcall);

/*Registration*/
MS2_PUBLIC int sal_register(SalOp *op, const char *proxy, const char *from, int expires);
/*refresh a register, -1 mean use the last known value*/
MS2_PUBLIC int sal_register_refresh(SalOp *op, int expires);
MS2_PUBLIC int sal_unregister(SalOp *h);

/*Messaging */
MS2_PUBLIC int sal_text_send(SalOp *op, const char *from, const char *to, const char *text);
MS2_PUBLIC int sal_message_send(SalOp *op, const char *from, const char *to, const char* content_type, const char *msg);
MS2_PUBLIC int sal_conf_query_message_send(SalOp *op, const char *from, const char *to, const char* comtype,const char* subject);

/*presence Subscribe/notify*/
MS2_PUBLIC int sal_subscribe_presence(SalOp *op, const char *from, const char *to, int expires);
MS2_PUBLIC int sal_notify_presence(SalOp *op, SalPresenceModel *presence);
MS2_PUBLIC int sal_notify_presence_close(SalOp *op);

/*presence publish */
MS2_PUBLIC int sal_publish_presence(SalOp *op, const char *from, const char *to, int expires, SalPresenceModel *presence);


/*ping: main purpose is to obtain its own contact address behind firewalls*/
MS2_PUBLIC int sal_ping(SalOp *op, const char *from, const char *to);

/*info messages*/
MS2_PUBLIC int sal_send_info(SalOp *op, const char *from, const char *to, const SalBody *body);

/*generic subscribe/notify/publish api*/
MS2_PUBLIC int sal_subscribe(SalOp *op, const char *from, const char *to, const char *eventname, int expires, const SalBody *body);
MS2_PUBLIC int sal_unsubscribe(SalOp *op);
MS2_PUBLIC int sal_subscribe_accept(SalOp *op);
MS2_PUBLIC int sal_subscribe_decline(SalOp *op, SalReason reason);
MS2_PUBLIC int sal_notify(SalOp *op, const SalBody *body);
MS2_PUBLIC int sal_notify_close(SalOp *op);
MS2_PUBLIC int sal_publish(SalOp *op, const char *from, const char *to, const char*event_name, int expires, const SalBody *body);

/*privacy, must be in sync with LinphonePrivacyMask*/
typedef enum _SalPrivacy {
	SalPrivacyNone=0x0,
	SalPrivacyUser=0x1,
	SalPrivacyHeader=0x2,
	SalPrivacySession=0x4,
	SalPrivacyId=0x8,
	SalPrivacyCritical=0x10,
	SalPrivacyDefault=0x8000
} SalPrivacy;
typedef  unsigned int SalPrivacyMask;

MS2_PUBLIC const char* sal_privacy_to_string(SalPrivacy  privacy);
MS2_PUBLIC void sal_op_set_privacy(SalOp* op,SalPrivacy privacy);
MS2_PUBLIC SalPrivacy sal_op_get_privacy(const SalOp* op);



#define payload_type_set_number(pt,n)		(pt)->user_data=(void*)((long)n);
#define payload_type_get_number(pt)		((int)(long)(pt)->user_data)

/*misc*/
MS2_PUBLIC void sal_get_default_local_ip(Sal *sal, int address_family, char *ip, size_t iplen);

typedef void (*SalResolverCallback)(void *data, const char *name, struct addrinfo *ai_list);

MS2_PUBLIC unsigned long sal_resolve_a(Sal* sal, const char *name, int port, int family, SalResolverCallback cb, void *data);
MS2_PUBLIC void sal_resolve_cancel(Sal *sal, unsigned long id);

MS2_PUBLIC SalCustomHeader *sal_custom_header_append(SalCustomHeader *ch, const char *name, const char *value);
MS2_PUBLIC const char *sal_custom_header_find(const SalCustomHeader *ch, const char *name);
MS2_PUBLIC void sal_custom_header_free(SalCustomHeader *ch);
MS2_PUBLIC SalCustomHeader *sal_custom_header_clone(const SalCustomHeader *ch);

MS2_PUBLIC const SalCustomHeader *sal_op_get_recv_custom_header(SalOp *op);

MS2_PUBLIC void sal_op_set_sent_custom_header(SalOp *op, SalCustomHeader* ch);

MS2_PUBLIC void sal_enable_logs();
MS2_PUBLIC void sal_disable_logs();

/*internal API */
void __sal_op_init(SalOp *b, Sal *sal);
void __sal_op_set_network_origin(SalOp *op, const char *origin /*a sip uri*/);
void __sal_op_set_network_origin_address(SalOp *op, SalAddress *origin);
void __sal_op_set_remote_contact(SalOp *op, const char *ct);
void __sal_op_free(SalOp *b);

/*test api*/
/*0 for no error*/
LINPHONE_PUBLIC	void sal_set_send_error(Sal *sal,int value);
/*1 for no error*/
LINPHONE_PUBLIC	void sal_set_recv_error(Sal *sal,int value);

/*always answer 480 if value=true*/
LINPHONE_PUBLIC	void sal_enable_unconditional_answer(Sal *sal,int value);

/*refresher retry after value in ms*/
LINPHONE_PUBLIC	void sal_set_refresher_retry_after(Sal *sal,int value);
LINPHONE_PUBLIC	int sal_get_refresher_retry_after(const Sal *sal);
/*enable contact fixing*/
MS2_PUBLIC void sal_nat_helper_enable(Sal *sal,bool_t enable);
MS2_PUBLIC bool_t sal_nat_helper_enabled(Sal *sal);

LINPHONE_PUBLIC	void sal_set_dns_timeout(Sal* sal,int timeout);
LINPHONE_PUBLIC int sal_get_dns_timeout(const Sal* sal);
LINPHONE_PUBLIC void sal_set_dns_user_hosts_file(Sal *sal, const char *hosts_file);
LINPHONE_PUBLIC const char *sal_get_dns_user_hosts_file(const Sal *sal);

#ifdef __cplusplus
}
#endif

#endif
