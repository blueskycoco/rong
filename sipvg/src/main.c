#include "stdint.h"
#include "SipVoiceService.h"
#include "test_audio.h"
#include "mediastreamer2/mscodecutils.h"
#include "mediastreamer2/msrtp.h"
#include "mediastreamer2/msfileplayer.h"
#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/mssndcard.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <dirent.h>


int g_fd_1,g_fd_2;
int pipe_fd_w;
#define LINE_ALL_OFF 0
#define LINE_3G1_3G2 1
#define LINE_PSTN_3G2 2
#define LINE_VOIP_3G2 3

int Line_on=LINE_ALL_OFF;
static int run=1;
char phone_in[100][20];
char phone_out[100][20];
int g_phone_len;


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
audio_path_t* audio_path;

int connect_audio_path(audio_path_t* audio_path,int source, int dest,bool connect);
int phone_process(int fd,int type,char *phone_number);

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
	terminate_call(s);
	char cmd[2];
	cmd[0]=0;
	cmd[1]='\0';
	write(pipe_fd_w,cmd,sizeof(char));
	Line_on=LINE_ALL_OFF;
}
int process_phone(char *phone,char out)
{
	int i,result=0;
	for(i=0;i<g_phone_len;i++)
	{
		printf("to compare %s<==>%s\r\n",phone_in[i],phone);
		if(strncmp(phone_in[i],phone,strlen(phone_in[i]))==0)
		{
			printf("%d accept %s call, transfer to %s 3G2 out\r\n",i,phone,phone_out[i]);
			char *cmd=(char *)malloc(strlen(phone_out[i])+2);
			memset(cmd,'\0',strlen(phone_out[i])+2);
			cmd[0]=out;
			strcpy(&cmd[1],phone_out[i]);
			write(pipe_fd_w,cmd,strlen(cmd));
			free(cmd);
			if(out==1)
				Line_on=LINE_3G1_3G2;
			else if(out==2)
				Line_on=LINE_PSTN_3G2;
			else if(out==3)
				Line_on=LINE_VOIP_3G2;
			result=1;
			break;
		}
	}
	if(result!=1)
		{
		char cmd[2];
					cmd[0]=0;cmd[1]='\0';
					write(pipe_fd_w,cmd,sizeof(char));
					Line_on=LINE_ALL_OFF;
		}
	return result;
}
int on_call_in(sip_voice_service_t* s,char* caller)
{
	ms_warning("caller %s\r\b",caller);
	if(Line_on==LINE_ALL_OFF)
	{
		char *ptr=strchr(caller,':');
		if(process_phone((char *)(ptr+1),3)==1)
		{
			accept_call(s,s->m_call);
			if(ta==NULL)
			{
				ta=test_audio_file_new(s,NULL);
				test_audio_file_new(s,NULL);
				test_audio_file_start(ta);
			}
		}
		else
			terminate_call(s);
	}
	else
	{
		printf("we are busy , reject this %s voip call",caller);
		terminate_call(s);
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
		audio_path->f1_w=ms_snd_card_create_writer(card_playback1);
		audio_path->f2_r=ms_snd_card_create_reader(card_capture2);
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
		if(audio_path->ticker1 && audio_path->f1_r){
			ms_warning("detach ticker1\r\n");
			ms_ticker_detach(audio_path->ticker1,audio_path->f1_r);
			}
		if(audio_path->ticker2 && audio_path->f2_r){
			ms_warning("detach ticker2\r\n");
			ms_ticker_detach(audio_path->ticker2,audio_path->f2_r);
			}
		if(audio_path->ticker1){
			ms_warning("destroy ticker1\r\n");
			ms_ticker_destroy(audio_path->ticker1);
			audio_path->ticker1=NULL;
			}
		if(audio_path->ticker2){
			ms_warning("destory ticker2\r\n");
			ms_ticker_destroy(audio_path->ticker2);
			audio_path->ticker2=NULL;
			}
		if(audio_path->f1_r && audio_path->f2_w){
			ms_warning("unlink f1_r from f2_w\r\n");
			ms_filter_unlink(audio_path->f1_r,0,audio_path->f2_w,0);
			}
		if(audio_path->f2_r && audio_path->f1_w){
			ms_warning("unlink f2_r from f1_w\r\n");
			ms_filter_unlink(audio_path->f2_r,0,audio_path->f1_w,0);
			}
		if(audio_path->f1_r){
			ms_warning("destroy f2_r\r\n");
			ms_filter_destroy(audio_path->f1_r);
			audio_path->f1_r=NULL;
			}
		if(audio_path->f2_r){
			ms_warning("destory f2_r\r\n");
			ms_filter_destroy(audio_path->f2_r);
			audio_path->f2_r=NULL;
			}
		if(audio_path->f1_w){
			ms_warning("destory f1_w\r\n");
			ms_filter_destroy(audio_path->f1_w);
			audio_path->f1_w=NULL;
			}
		if(audio_path->f2_w){
			ms_warning("destory f2_w\r\n");
			ms_filter_destroy(audio_path->f2_w);
			audio_path->f2_w=NULL;
			}
	}
	return 0;
}

void* usage(){
   printf("\n");
   printf("h------hangup the session\n");
   printf("r------connect 3g1 to 3g2\n");
   printf("t------connect pstn to 3g2\n");
   printf("s------disconnect 3g1 from 3g2\n");
   printf("b------disconnect pstn from 3g2\n");
   printf("e------end  playing pcm file\n");
   printf("d------set debug flag\n");
   printf("q------quit the demo program\n");
   printf("-->>");
}
#define DEFAULT_PCM_FILE  "share/test_vg.wav"
/*1 for 3g1 , 2 for 3g2 ,3 for pstn ,4 for voip*/

int main(int argc,const char** argv)
{
	int i=0,k=0,m=0,next_write=0;
	char ch;
	const char *fifo_name_w = "/tmp/from_sipvg";
	const char *fifo_name_r = "/tmp/to_sipvg";
	if(access(fifo_name_w, F_OK) == -1)  
	{          
        int res = mkfifo(fifo_name_w, 0777);  
        if(res != 0)  
        {  
            fprintf(stderr, "sipvg Could not create fifo %s\n",fifo_name_w);  
            exit(-1);  
        }  
	}  
	if(access(fifo_name_r, F_OK) == -1)  
	{          
        int res = mkfifo(fifo_name_r, 0777);  
        if(res != 0)  
        {  
            fprintf(stderr, "sipvg Could not create fifo %s\n",fifo_name_r);  
            exit(-1);  
        }  
	}  
	pipe_fd_w = open(fifo_name_w, O_WRONLY/*|O_NONBLOCK*/);
	int pipe_fd_r = open(fifo_name_r, O_RDONLY/*|O_NONBLOCK*/);
	printf("sipvg  open fifo wr over\r\n");
	char command[PIPE_BUF+1],read_bytes;
	audio_path=ms_new0(audio_path_t,1);
	sip_voice_service_t* vg=sip_voice_service_new(NULL);
	sip_voice_service_set_audio_data_cb(vg,on_sip_audio_data,on_call_hang,on_call_in);  
	memset(phone_in,'\0',sizeof(phone_in));
	memset(phone_out,'\0',sizeof(phone_out));
	FILE *fp=fopen("./phone.txt","r");
	if(fp<0)
	{
		perror("open phone.txt failed\r\n");
		return -1;
	}
	while(fread(&ch,sizeof(char),1,fp)==1)
	{
		if(ch==',')
		{/*next is route out phone number*/
			next_write=1;
			m=0;
		}
		else if(ch=='\n')
		{/*next is call in phone number*/
			next_write=0;
			k++;
			m=0;
		}
		else
		{/*store in phone_map*/
			if(next_write==1)
			{
				phone_out[k][m++]=ch;
			}
			else
			{
				phone_in[k][m++]=ch;
			}
		}
	}
	fclose(fp);
	k=k-1;
	g_phone_len=k;
	for(i=0;i<k;i++)
	{
		printf("Phone[%d] in: %s <==> Phone[%d] out : %s\r\n",i,phone_in[i],i,phone_out[i]); 
	}
	while(run)
	{
		char *ptr;
		 memset(command,'\0',sizeof(command));
		 ptr=command;
		 read_bytes = read(pipe_fd_r, ptr, PIPE_BUF);
		// printf("sipvg read_bytes %d\r\n",read_bytes);
		 if(read_bytes>0)
		 	{
		 	printf("sipvg read_bytes %d,%s\r\n",read_bytes,ptr);
		 switch(*ptr)
		 {
			case '1'://3g1 incomming call with phone num
			{
				printf("3G1 incomming call with phone num %s\r\n",(char *)(ptr+1));
				if(process_phone((char *)(ptr+1),1)==1)
				{
					connect_audio_path(audio_path,1,2,true);
				}
			}
			break;
			case '2'://pstn incomming call with phone num
			{
				printf("pstn incomming call with phone num %s\r\n",(char *)(ptr+1));
				if(process_phone((char *)(ptr+1),2)==1)
				{
					connect_audio_path(audio_path,3,2,true);
				}
			}
			break;
			case '3'://3g1 hang up
			{
				printf("3g1 hang up\r\n");
				if(Line_on==LINE_3G1_3G2)
				{
					connect_audio_path(audio_path,0,0,false);
					char cmd[2];
					cmd[0]=0;
					cmd[1]='\0';
					write(pipe_fd_w,cmd,sizeof(char));
					Line_on=LINE_ALL_OFF;
				}
			}
			break;
			case '4'://3g2 hang up
			{
				printf("3g2 hang up\r\n");
				if(Line_on==LINE_3G1_3G2 ||Line_on==LINE_PSTN_3G2 ||Line_on==LINE_VOIP_3G2)
				{
					connect_audio_path(audio_path,0,0,false);
					if(ta)
					{
						test_audio_file_stop(ta);
						test_audio_file_destroy(ta);
						ta=NULL;
					
					terminate_call(vg);
					}
					char cmd[2];
					cmd[0]=0;cmd[1]='\0';
					write(pipe_fd_w,cmd,sizeof(char));
					Line_on=LINE_ALL_OFF;
				}
			}
			break;
			case '5'://exit 
			{
				printf("sipvg exit\r\n");
				run=0;
			}
			break;
		 }
		 	}
	}
#if 0
	while(1)
	{
		usage();
		char ch=getchar();
		if(ch=='h')
		{
			printf("terminate the session\n");
			/*if(ta)
			{
				test_audio_file_stop(ta);
				test_audio_file_destroy(ta);
				ta=NULL;
			}
			if(recording)
			{
				sip_voice_service_rec_stop(vg);
				recording=0;
			}*/
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
			//terminate_call(vg);
			//connect_audio_path(audio_path,0,0,false);
			connect_audio_path(audio_path,1,2,true);
		}
		else if(ch=='t')
		{
			printf("connect pstn to 3g2\n");
			/*sip_voice_service_rec_stop(vg);
			file_idx++;
			recording=0;*/			
			//terminate_call(vg);
			//connect_audio_path(audio_path,0,0,false);
			connect_audio_path(audio_path,3,2,true);
		}
		else if(ch=='s')
		{
			printf("disconnect 3g1 from 3g2\r\n");
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
			connect_audio_path(audio_path,0,0,false);
		}
		else if(ch=='b')
		{
			//if(!ta)
			//	printf("please set playing pcm file first\n");
			//else
			//	test_audio_file_start(ta);
			printf("disconnect pstn from 3g2\r\n");
			connect_audio_path(audio_path,0,0,false);
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
	#endif
	close(pipe_fd_w);
	close(pipe_fd_r);
	sip_voice_service_destroy(vg);
	printf("finished\n");
}
