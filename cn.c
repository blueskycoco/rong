/*
mediastreamer2 library - modular sound and video processing and streaming
Copyright (C) 2006  Simon MORLAT (simon.morlat@linphone.org)

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

#ifdef HAVE_CONFIG_H
#include "mediastreamer-config.h"
#endif

#include "mediastreamer2/msfilter.h"
#include "mediastreamer2/mssndcard.h"
#include "mediastreamer2/msticker.h"
#include <ortp/ortp.h>

#include <signal.h>

static int run=1;

static void stop(int signum)
{
	run=0;
	printf("to stop exe\r\n");
}

static void print_usage(void)
{
	printf("echo\t\t[--card1 <sound card1 name>]\n"
	       "\t\t[--card2 <sound card2 name>]\n");
	exit(-1);
}

int main(int argc, char *argv[])
{
	MSFilter *f1_r,*f1_w,*f2_r,*f2_w;
	MSSndCard *card_capture1,*card_capture2;
	MSSndCard *card_playback1,*card_playback2;
	MSTicker *ticker1,*ticker2;
	char *capt_card1=NULL,*play_card1=NULL,*capt_card2=NULL,*play_card2=NULL;
	char alsa[256];
	int rate = 8000;
	int i;
	const char *alsadev=NULL;
	printf("main 1\r\n");
	ortp_init();
	printf("main 2\r\n");
	ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);
	printf("main 3\r\n");
	ms_init();
	printf("ms_init over\r\n");
	signal(SIGINT,stop);

	
	for(i=1;i<argc;++i)
	{
		if (strcmp(argv[i],"--help")==0)
		{
			print_usage();
		}
		else if (strcmp(argv[i],"--card1")==0)
		{
			i++;
			capt_card1=play_card1=argv[i];
		}
		else if (strcmp(argv[i],"--card2")==0)
		{
			i++;
			capt_card2=play_card2=argv[i];
		}
	}
	ms_snd_card_manager_add_card(ms_snd_card_manager_get(),ms_alsa_card_new_custom(capt_card1,capt_card1));
	ms_snd_card_manager_add_card(ms_snd_card_manager_get(),ms_alsa_card_new_custom(capt_card2,capt_card2));
	memset(alsa,'\0',256);
	sprintf(alsa,"ALSA: %s",capt_card1);
	card_capture1 = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),alsa);
	card_playback1 = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),alsa);
	memset(alsa,'\0',256);
	sprintf(alsa,"ALSA: %s",capt_card2);
	card_capture2 = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),alsa);
	card_playback2 = ms_snd_card_manager_get_card(ms_snd_card_manager_get(),alsa);
	
	if (card_playback1==NULL || card_capture1==NULL ||card_playback2==NULL || card_capture2==NULL)
	{
		if(card_playback1==NULL)
		ms_error("No card. card_playback1 %s",capt_card1);
		if(card_capture1==NULL)
		ms_error("No card. card_capture1 %s",capt_card1);
		if(card_playback2==NULL)
		ms_error("No card. card_playback2 %s",capt_card2);
		if(card_capture2==NULL)
		ms_error("No card. card_capture2 %s",capt_card2);
		return -1;
	}
	else
	{
		ms_warning("card_playback1 %s|%s|%s|%d|%d|%d",card_playback1->name,card_playback1->id,card_playback1->desc->driver_type,
			card_playback1->capabilities,card_playback1->latency,card_playback1->preferred_sample_rate);
		ms_warning("card_playback2 %s|%s|%s|%d|%d|%d",card_playback2->name,card_playback2->id,card_playback2->desc->driver_type,
			card_playback2->capabilities,card_playback2->latency,card_playback2->preferred_sample_rate);
	}
	
	f1_r=ms_snd_card_create_reader(card_capture1);
	f2_w=ms_snd_card_create_writer(card_playback2);
	f1_w=ms_snd_card_create_writer(card_playback1);
	f2_r=ms_snd_card_create_reader(card_capture2);
	if(f1_r!=NULL&&f1_w!=NULL&&f2_r!=NULL&&f2_w!=NULL)
	{
		ms_filter_call_method (f1_r, MS_FILTER_SET_SAMPLE_RATE,	&rate);
		ms_filter_call_method (f2_r, MS_FILTER_SET_SAMPLE_RATE,	&rate);
		ms_filter_call_method (f1_w, MS_FILTER_SET_SAMPLE_RATE,	&rate);
		ms_filter_call_method (f2_w, MS_FILTER_SET_SAMPLE_RATE,	&rate);

		ticker1=ms_ticker_new();
		ms_ticker_set_name(ticker1,"card1 to card2");
		ticker2=ms_ticker_new();
		ms_ticker_set_name(ticker2,"card2 to card1");
		ms_filter_link(f1_r,0,f2_w,0);	 	
		ms_filter_link(f2_r,0,f1_w,0);
		ms_ticker_attach(ticker1,f1_r);
		ms_ticker_attach(ticker2,f2_r);		
		//while(run)
			ms_sleep(3);
		//if(ticker1) ms_ticker_detach(ticker1,f1_r);
		if(f1_r&&f2_w) ms_filter_unlink(f1_r,0,f2_w,0);
		//if(ticker1) ms_ticker_destroy(ticker1);
		if(f1_r) ms_filter_destroy(f1_r);
		if(f2_w) ms_filter_destroy(f2_w);
		
		//if(ticker2) ms_ticker_detach(ticker2,f2_r);
		if(f2_r&&f1_w)ms_filter_unlink(f2_r,0,f1_w,0);
		//if(ticker2) ms_ticker_destroy(ticker2);
		if(f2_r) ms_filter_destroy(f2_r);
		if(f1_w) ms_filter_destroy(f1_w);
	}
	else
		ms_error("f1_r,f1_w,f2_r,f2_w create failed\r\n");
	return 0;
}
