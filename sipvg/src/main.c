#include "stdint.h"
#include "SipVoiceService.h"
#include "test_audio.h"
int g_debug=0;
void on_sip_audio_data(void* ud,uint8_t* data,int len)
{
   if(g_debug)
   printf("received audio data len: %d\n",len);
}
static test_audio_file_t* ta=NULL;
static int file_idx=0;
static int recording=0;

void on_call_hang(sip_voice_service_t* s)
{
   
    if(ta)
	{
		test_audio_file_stop(ta);
		test_audio_file_destroy(ta);
		ta=NULL;
    }
    if(recording)
	{
	   	sip_voice_service_rec_stop(s);
		recording=0;
    }
}
int on_call_in(sip_voice_service_t* s,char* caller)
{
	return 0;
}

void* usage(){
   printf("\n");
   printf("h------hangup the session\n");
   printf("r------record the session to file\n");
   printf("t------stop recording the session\n");
   printf("s------set playing file for the session\n");
   printf("b------begin playing pcm file\n");
   printf("e------end  playing pcm file\n");
   printf("d------set debug flag\n");
   printf("q------quit the demo program\n");
   printf("-->>");
}
#define DEFAULT_PCM_FILE  "share/test_vg.wav"

int main(int argc,const char** argv)
{
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
			printf("record file command....\n");
			char filename[100]={0};
			sprintf(filename,"%d.wav",file_idx);
			sip_voice_service_set_rec_filename(vg,filename);
			sip_voice_service_rec_start(vg);
			recording=1;
		}
		else if(ch=='t')
		{
			printf("stopping recording...\n");
			sip_voice_service_rec_stop(vg);
			file_idx++;
			recording=0;
		}
		else if(ch=='s')
		{
			printf("please input the record filename:");
			char filename[100]={0};
			scanf("%s",filename);
			if(strcmp(filename,"")==0)
				strcpy(filename,DEFAULT_PCM_FILE);
			if(ta)
			{
				test_audio_file_stop(ta);
				test_audio_file_destroy(ta);
			}
			ta=test_audio_file_new(vg,filename);
		}
		else if(ch=='b')
		{
			if(!ta)
				printf("please set playing pcm file first\n");
			else
				test_audio_file_start(ta);
		}
		else if(ch=='e')
		{
			if(!ta)
				printf("please set playing pcm file first\n");
			else
				test_audio_file_stop(ta);
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
	if(recording)
		sip_voice_service_rec_stop(vg);
	if(ta)
	{
		test_audio_file_stop(ta);
		test_audio_file_destroy(ta);
	}
	sip_voice_service_destroy(vg);
	printf("finished\n");
}
