#include "stdint.h"
#include "SipVoiceService.h"
#include "test_audio.h"
#include "mediastreamer2/mscodecutils.h"
#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/mssndcard.h"

int g_debug=0;
void on_sip_audio_data(void* ud,uint8_t* data,int len)
{
   if(g_debug)
   printf("received audio data len: %d\n",len);
}
static test_audio_file_t* ta=NULL;
static int file_idx=0;
static int recording=0;
typedef struct audio_path_t{
	MSFilter *f1_r,*f1_w,*f2_r,*f2_w;
	MSTicker *ticker1,*ticker2;
}audio_path_t;

void on_call_hang(sip_voice_service_t* s)
{
   
    /*if(ta)
	{
		test_audio_file_stop(ta);
		test_audio_file_destroy(ta);
		ta=NULL;
    }
    if(recording)
	{
	   	sip_voice_service_rec_stop(s);
		recording=0;
    }*/
    ms_warning("remote hang up\r\n");
	if(ta)
	{
		test_audio_file_stop(ta);
		test_audio_file_destroy(ta);
		ta=NULL;
	}
}
int on_call_in(sip_voice_service_t* s,char* caller)
{
	ms_warning("caller %s\r\b",caller);
	if(strncmp(":toto",strchr(caller,':'),5)==0)
	{
		accept_call(s,s->m_call);
		if(ta==NULL){
		ta=test_audio_file_new(s,NULL);
		test_audio_file_new(s,NULL);
		test_audio_file_start(ta);
			}
	}
	return 0;
}
/*1 for 3g1 , 2 for 3g2 ,3 for pstn ,4 for voip*/
int connect_audio_path(audio_path_t* audio_path,int source, int dest,bool connect)
{
	//static MSFilter *f1_r,*f1_w,*f2_r,*f2_w;
	MSSndCard *card_capture1,*card_capture2;
	MSSndCard *card_playback1,*card_playback2;
	//static MSTicker *ticker1,*ticker2;
	//char *capt_card1=NULL,*play_card1=NULL,*capt_card2=NULL,*play_card2=NULL;
	int rate = 8000;
	char plughw_source[17],plughw_dest[17],plughw[11];
	if(connect)
	{
		memset(plughw_source,'\0',17);
		memset(plughw,'\0',11);
		memset(plughw_dest,'\0',17);
		sprintf(plughw_source,"ALSA: plughw:0,%d",source);
		sprintf(plughw_dest,"ALSA: plughw:0,%d",dest);
		sprintf(plughw,"plughw:0,%d",source);
		if(ms_snd_card_manager_get_card(ms_snd_card_manager_get(),plughw_source)==NULL)
			ms_snd_card_manager_add_card(ms_snd_card_manager_get(),ms_alsa_card_new_custom(plughw,plughw));
		sprintf(plughw,"plughw:0,%d",dest);
		if(ms_snd_card_manager_get_card(ms_snd_card_manager_get(),plughw_dest)==NULL)
			ms_snd_card_manager_add_card(ms_snd_card_manager_get(),ms_alsa_card_new_custom(plughw,plughw));

		card_capture1 = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),plughw_source);
		card_playback1 = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),plughw_source);
		card_capture2 = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),plughw_dest);
		card_playback2 = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),plughw_dest);
		
		if (card_playback1==NULL || card_capture1==NULL ||card_playback2==NULL || card_capture2==NULL)
		{
			if(card_playback1==NULL)
			ms_error("No card. card_playback1 %s",plughw_source);
			if(card_capture1==NULL)
			ms_error("No card. card_capture1 %s",plughw_source);
			if(card_playback2==NULL)
			ms_error("No card. card_playback2 %s",plughw_dest);
			if(card_capture2==NULL)
			ms_error("No card. card_capture2 %s",plughw_dest);
			return -1;
		}
		audio_path->f1_r=ms_snd_card_create_reader(card_capture1);
		audio_path->f2_w=ms_snd_card_create_writer(card_playback2);
		audio_path->f1_w=ms_snd_card_create_reader(card_playback1);
		audio_path->f2_r=ms_snd_card_create_writer(card_capture2);
		if(audio_path->f1_r!=NULL&&audio_path->f1_w!=NULL&&audio_path->f2_r!=NULL&&audio_path->f2_w!=NULL)
		{
			ms_filter_call_method (audio_path->f1_r, MS_FILTER_SET_SAMPLE_RATE,	&rate);
			ms_filter_call_method (audio_path->f2_r, MS_FILTER_SET_SAMPLE_RATE,	&rate);
			ms_filter_call_method (audio_path->f1_w, MS_FILTER_SET_SAMPLE_RATE,	&rate);
			ms_filter_call_method (audio_path->f2_w, MS_FILTER_SET_SAMPLE_RATE,	&rate);

			audio_path->ticker1=ms_ticker_new();
			audio_path->ticker2=ms_ticker_new();
			ms_filter_link(audio_path->f1_r,0,audio_path->f2_w,0);
			ms_filter_link(audio_path->f2_r,0,audio_path->f1_w,0);
			ms_ticker_attach(audio_path->ticker1,audio_path->f1_r);
			ms_ticker_attach(audio_path->ticker2,audio_path->f2_r);
			//while(run)
				//ms_sleep(1);
			}
		else
			ms_error("f1_r,f1_w,f2_r,f2_w create failed\r\n");
	}
	else
	{
		if(audio_path->ticker1 && audio_path->f1_r) ms_ticker_detach(audio_path->ticker1,audio_path->f1_r);
		if(audio_path->ticker2 && audio_path->f2_r) ms_ticker_detach(audio_path->ticker2,audio_path->f2_r);
		if(audio_path->ticker1) ms_ticker_destroy(audio_path->ticker1);
		if(audio_path->ticker2) ms_ticker_destroy(audio_path->ticker2);
		if(audio_path->f1_r && audio_path->f2_w) ms_filter_unlink(audio_path->f1_r,0,audio_path->f2_w,0);
		if(audio_path->f2_r && audio_path->f1_w) ms_filter_unlink(audio_path->f2_r,0,audio_path->f1_w,0);
		if(audio_path->f1_r) ms_filter_destroy(audio_path->f1_r);
		if(audio_path->f2_r) ms_filter_destroy(audio_path->f2_r);
		if(audio_path->f1_w) ms_filter_destroy(audio_path->f1_w);
		if(audio_path->f2_w) ms_filter_destroy(audio_path->f2_w);
	}
	return 0;
}

void* usage(){
   printf("\n");
   printf("h------hangup the session\n");
   printf("r------connect 3g1 to 3g2\n");
   printf("t------connect pstn to 3g2\n");
   printf("s------set playing file for the session\n");
   printf("b------begin playing pcm file\n");
   printf("e------end  playing pcm file\n");
   printf("d------set debug flag\n");
   printf("q------quit the demo program\n");
   printf("-->>");
}
#define DEFAULT_PCM_FILE  "share/test_vg.wav"
/*1 for 3g1 , 2 for 3g2 ,3 for pstn ,4 for voip*/

int main(int argc,const char** argv)
{
	audio_path_t* audio_path=ms_new0(audio_path_t,1);
	sip_voice_service_t* vg=sip_voice_service_new(NULL);
	sip_voice_service_set_audio_data_cb(vg,on_sip_audio_data,on_call_hang,on_call_in);  
	while(1)
	{
		usage();
		char ch=getchar();
		if(ch=='h')
		{
			printf("terminate the session\n");
			if(ta)
			{
				test_audio_file_stop(ta);
				test_audio_file_destroy(ta);
				ta=NULL;
			}
			if(recording)
			{
				sip_voice_service_rec_stop(vg);
				recording=0;
			}
			terminate_call(vg);
		}
		else if(ch=='r')
		{
			printf("connect 3g1 to 3g2\n");
			/*char filename[100]={0};
			sprintf(filename,"%d.wav",file_idx);
			sip_voice_service_set_rec_filename(vg,filename);
			sip_voice_service_rec_start(vg);
			recording=1;*/
			terminate_call(vg);
			connect_audio_path(audio_path,0,0,false);
			connect_audio_path(audio_path,1,2,true);
		}
		else if(ch=='t')
		{
			printf("connect pstn to 3g2\n");
			/*sip_voice_service_rec_stop(vg);
			file_idx++;
			recording=0;*/			
			terminate_call(vg);
			connect_audio_path(audio_path,0,0,false);
			connect_audio_path(audio_path,3,2,true);
		}
		else if(ch=='s')
		{
			//printf("please input the record filename:");
			/*char filename[100]={0};
			scanf("%s",filename);
			if(strcmp(filename,"")==0)
				strcpy(filename,DEFAULT_PCM_FILE);
			if(ta)
			{
				test_audio_file_stop(ta);
				test_audio_file_destroy(ta);
			}
			ta=test_audio_file_new(vg,filename);*/
		}
		else if(ch=='b')
		{
			//if(!ta)
			//	printf("please set playing pcm file first\n");
			//else
			//	test_audio_file_start(ta);
		}
		else if(ch=='e')
		{
			//if(!ta)
			//	printf("please set playing pcm file first\n");
			///
			//else
			//	test_audio_file_stop(ta);
		}
		else if(ch=='q')
		{
			printf("quitting....\n");
			break;
		}
		else if(ch=='d')
		{
			g_debug=!g_debug;
			vg->debug=g_debug;
		}
	}
	//if(recording)
	//	sip_voice_service_rec_stop(vg);
	//if(ta)
	//{
	//	test_audio_file_stop(ta);
	//	test_audio_file_destroy(ta);
	//}
	sip_voice_service_destroy(vg);
	printf("finished\n");
}
