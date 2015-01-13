#include "SipVoiceService.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msticker.h"
#include "mschannel.h"
#define PLAY_FROM_FILE 0
#define CARD_S "plughw:0,1"
typedef struct test_audio_file_t
{
	sip_voice_service_t* s;
	
	MSTicker* ticker;
	MSFilter* player;
	MSFilter* output;

	channel_t* channel;
}test_audio_file_t;

//播放数据
void OnPlayerData(channel_t* c,void* im)
{
	mblk_t* m=(mblk_t*)im;
	test_audio_file_t* t=(test_audio_file_t*)channel_get_user_data(c);
	sip_voice_service_set_audio_data(t->s,m->b_rptr,m->b_wptr-m->b_rptr);
}
//新建立
test_audio_file_t* test_audio_file_new(sip_voice_service_t* s,const char* filename)
{
	test_audio_file_t* t=ms_new0(test_audio_file_t,1);
	t->s=s;
	t->ticker=ms_ticker_new();
	#if PLAY_FROM_FILE
	t->player=ms_filter_new(MS_FILE_PLAYER_ID);
	#else
	ms_snd_card_manager_add_card(ms_snd_card_manager_get(),ms_alsa_card_new_custom(CARD_S,CARD_S));
	MSSndCard *card_capture = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),CARD_S);
	t->player=ms_snd_card_create_reader(card_capture);
	int rate = 8000;
	ms_filter_call_method (t->player, MS_FILTER_SET_SAMPLE_RATE,	&rate);
	#endif
	t->output=ms_filter_new((MSFilterId)MS_OUTPUT_CHANNEL_ID);
	t->channel=channel_new(get_default_channel_manager(0),t,0);

	//设置回调
	channel_set_notify_cb(t->channel,OnPlayerData);

	channel_t* ch=NULL;
	ms_filter_call_method(t->output,MS_FILTER_GET_OUTPUT_CAHNNEL,&ch);
	channel_link(ch,t->channel,1);
	#if PLAY_FROM_FILE
	int loop=1;
	ms_filter_call_method(t->player,MS_FILE_PLAYER_LOOP,&loop);
	ms_filter_call_method(t->player,MS_FILE_PLAYER_OPEN,(void*)filename);
	#endif
	return t;
}
//销毁
void test_audio_file_destroy(test_audio_file_t* t)
{
	#if PLAY_FROM_FILE
	ms_filter_call_method(t->player,MS_FILE_PLAYER_CLOSE,NULL);
	#endif
	channel_t* ch=NULL;
	ms_filter_call_method(t->output,MS_FILTER_GET_OUTPUT_CAHNNEL,&ch);
	channel_unlink(ch,t->channel,1);
	if(t->ticker)ms_ticker_destroy(t->ticker);
	if(t->output)ms_filter_destroy(t->output);
	if(t->player)ms_filter_destroy(t->player);
	if(t->channel)channel_destroy(t->channel);
}
//开始播放
void test_audio_file_start(test_audio_file_t* t)
{
    MSConnectionHelper h;
	ms_connection_helper_start(&h);
	ms_connection_helper_link(&h,t->player,-1,0);
	ms_connection_helper_link(&h,t->output,0,-1);
	ms_ticker_attach(t->ticker,t->player);
	#if PLAY_FROM_FILE
	ms_filter_call_method(t->player,MS_FILE_PLAYER_START,NULL);
	#endif
}
//开始播放
void test_audio_file_stop(test_audio_file_t* t)
{
	ms_ticker_detach(t->ticker,t->player);
	#if PLAY_FROM_FILE
	ms_filter_call_method(t->player,MS_FILE_PLAYER_STOP,NULL);
	#endif
	MSConnectionHelper h;
	ms_connection_helper_start(&h);
	ms_connection_helper_unlink(&h,t->player,-1,0);
	ms_connection_helper_unlink(&h,t->output,0,-1);
}
