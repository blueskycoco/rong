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
}
int on_call_in(sip_voice_service_t* s,char* caller)
{
	ms_warning("caller %s\r\b",caller);
	if(strncmp(":toto",strchr(caller,':'),5)==0)
	{
		accept_call(s,s->m_call);
		if(ta==NULL){
			connect_audio_path(audio_path,0,0,false);
		ta=test_audio_file_new(s,NULL);
		test_audio_file_new(s,NULL);
		test_audio_file_start(ta);
		phone_process(g_fd_1,0,"13581546361");
			}
	}
	return 0;
}

int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
	struct termios newtio,oldtio;
	if  ( tcgetattr( fd,&oldtio)  !=  0) { 
		perror("SetupSerial 1");
		return -1;
	}
	bzero( &newtio, sizeof( newtio ) );
	newtio.c_cflag  |=  CLOCAL | CREAD; 
	newtio.c_cflag &= ~CSIZE; 

	switch( nBits )
	{
		case 7:
			newtio.c_cflag |= CS7;
			break;
		case 8:
			newtio.c_cflag |= CS8;
			break;
	}

	switch( nEvent )
	{
		case 'O':
			newtio.c_cflag |= PARENB;
			newtio.c_cflag |= PARODD;
			newtio.c_iflag |= (INPCK | ISTRIP);
			break;
		case 'E': 
			newtio.c_iflag |= (INPCK | ISTRIP);
			newtio.c_cflag |= PARENB;
			newtio.c_cflag &= ~PARODD;
			break;
		case 'N':  
			newtio.c_cflag &= ~PARENB;
			break;
	}

	switch( nSpeed )
	{
		case 2400:
			cfsetispeed(&newtio, B2400);
			cfsetospeed(&newtio, B2400);
			break;
		case 4800:
			cfsetispeed(&newtio, B4800);
			cfsetospeed(&newtio, B4800);
			break;
		case 9600:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
		case 115200:
			cfsetispeed(&newtio, B115200);
			cfsetospeed(&newtio, B115200);
			break;
		default:
			cfsetispeed(&newtio, B9600);
			cfsetospeed(&newtio, B9600);
			break;
	}
	if( nStop == 1 )
		newtio.c_cflag &=  ~CSTOPB;
	else if ( nStop == 2 )
		newtio.c_cflag |=  CSTOPB;
	newtio.c_cc[VTIME]  = 0;
	newtio.c_cc[VMIN] = 0;
	tcflush(fd,TCIFLUSH);
	if((tcsetattr(fd,TCSANOW,&newtio))!=0)
	{
		perror("com set error");
		return -1;
	}
	printf("set done!\n");
	return 0;
}

int open_port(int comport)
{
	int fd;
	long  vdisable;
	if (comport==0)
	{	
		fd = open( "/dev/ttyS0", O_RDWR|O_NOCTTY|O_NDELAY);
		if (-1 == fd){
			perror("Can't Open Serial Port0");
			return(-1);
		}
		else 
			printf("open tts/0 .....\n");
	}
	else if(comport==1)
	{	
		fd = open( "/dev/ttyS1", O_RDWR/*|O_NOCTTY|O_NDELAY*/);
		if (-1 == fd){
			perror("Can't Open Serial Port2");
			return(-1);
		}
		else 
			printf("open tts/1 .....\n");
	}
	else if (comport==2)
	{
		fd = open( "/dev/ttyS2", O_RDWR/*|O_NOCTTY|O_NDELAY*/);
		if (-1 == fd){
			perror("Can't Open Serial Port3");
			return(-1);
		}
		else 
			printf("open tts/2 .....\n");
	}
	if(fcntl(fd, F_SETFL, FNDELAY)<0)
		printf("fcntl failed!\n");
	else
		printf("fcntl=%d\n",fcntl(fd, F_SETFL,FNDELAY));
	if(isatty(STDIN_FILENO)==0)

		printf("standard input is not a terminal device\n");
	else
		printf("isatty success!\n");
	printf("fd-open=%d\n",fd);
	return fd;
}
int phone_process(int fd,int type,char *phone_number)
{
	int nread,nwrite,i,j;
	char buff0[4]="ATD";
	char buff1[4]="ATA";
	char buff2[8]="AT+CHUP";
	char buff3[10]="AT+CLIP=1";
	char end='\r';
	char end_sent=';';
	if(type==0)//call out
	{
		printf("call Phone %s through serial fd %d\r\n",phone_number,fd);	
		write(fd,buff0,3);
		write(fd,phone_number,strlen(phone_number));
		write(fd,&end_sent,1);
		write(fd,&end,1);
	}
	else if(type==1)//open call in display
	{
		printf("open incoming phone number display on fd %d\r\n",fd);	
		write(fd,buff3,9);
		write(fd,&end,1);
	}
	else if(type==2)//reject call in
	{
		printf("reject incoming call on fd %d\r\n",fd);	
		write(fd,buff2,7);
		write(fd,&end,1);
	}
	else if(type==3)//accept call in
	{
		printf("accept incoming call on fd %d\r\n",fd);	
		write(fd,buff1,3);
		write(fd,&end,1);
	}
	return 0;
}
void print_system_status(int status)
{
	printf("status = %d\n",status);
	if(WIFEXITED(status))
	{
		printf("normal termination,exit status = %d\n",WEXITSTATUS(status));
	}
	else if(WIFSIGNALED(status))
	{
		printf("abnormal termination,signal number =%d%s\n",
		WTERMSIG(status),
		WCOREDUMP(status)?"core file generated" : "");
	}
}

int open_record()
{
	char command[256];
	int result;
	sprintf(command,"%s","/usr/local/bin/bin/amixer -c 0 sset \'Analog Right Sub Mic\' cap");
	printf("open_record %s\r\n",command);
	print_system_status(system(command));
	return result;
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
	audio_path=ms_new0(audio_path_t,1);
	sip_voice_service_t* vg=sip_voice_service_new(NULL);
	sip_voice_service_set_audio_data_cb(vg,on_sip_audio_data,on_call_hang,on_call_in);  
	open_record();
	if((g_fd_1=open_port(1))<0){
		perror("open_port error 1");
		return -1;
	}
	if(set_opt(g_fd_1,115200,8,'N',1)<0){
		perror("set_opt error 1");
		return -1;
	}
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
	sip_voice_service_destroy(vg);
	printf("finished\n");
}
