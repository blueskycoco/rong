#include "mschannel.h"
#include <mediastreamer2/msfilter.h>
#include <mediastreamer2/msticker.h>
#include "channelsys.h"

typedef struct channel_filter_t{
	channel_t* ch;
	MSFilter* f;
	//0--source,1-output
	int type;
}channel_filter_t;


//转码通道,传递数据
static void OnTranscodeChannelData(channel_t* c,void* im){
	channel_filter_t* t=(channel_filter_t*)channel_get_user_data(c);
	/*
	mblk_t* m=(mblk_t*)im;
	uint32_t timestamp=(uint32_t)(t->f->ticker->time*90LL);
	mblk_set_timestamp_info(m,timestamp);
	*/
	if(t->f->outputs[0])
		ms_queue_put(t->f->outputs[0],(mblk_t*)im);
}
//初始化
void channel_filter_init(MSFilter* f,int type){
	channel_filter_t* c=ms_new0(channel_filter_t,1);
	c->f=f;
	c->type=type;
	c->ch=channel_new(get_default_channel_manager(0),c,1);
	f->data=c;
	if(type==0){//需要被灌入数据
		channel_set_notify_cb(c->ch,OnTranscodeChannelData);
	}
}
void channel_source_filter_init(MSFilter* f){
	channel_filter_init(f,0);
}
void channel_output_filter_init(MSFilter* f){
	channel_filter_init(f,1);
}
//销毁
void channel_filter_uninit(MSFilter* f){
	channel_filter_t* c=(channel_filter_t*)f->data;
	if(c->ch)channel_destroy(c->ch);
	ms_free(c);
}
void channel_filter_process(MSFilter* f){
	channel_filter_t* t=(channel_filter_t*)f->data;
	if(t->type==1){
		mblk_t* im=NULL;
		while((im=ms_queue_get(f->inputs[0]))!=NULL){
			//printf("received  data....\n");	
			channel_dispatch(t->ch,dupmsg(im));
			freemsg(im);		
		}
	}
	
}
//获取通道
static int channel_filter_get_channel(MSFilter* f,void* arg){
	channel_filter_t* t=(channel_filter_t*)f->data;
	*(channel_t**)arg=t->ch;
	return 0;
}
static int channel_filter_set_msg(MSFilter* f,void* arg){
	if(f->inputs[0]){
		ms_queue_put(f->inputs[0],(mblk_t*)arg);
	}	
}
//方法
static MSFilterMethod methods_source[]=
{
	{ MS_FILTER_GET_SOURCE_CAHNNEL, channel_filter_get_channel },
	{ MS_FILTER_SET_SOURCE_CAHNNEL_MSG, channel_filter_set_msg },
	{ 0, NULL }
};
//方法
static MSFilterMethod methods_output[]=
{
	{ MS_FILTER_GET_OUTPUT_CAHNNEL, channel_filter_get_channel },
	{ 0, NULL }
};


MSFilterDesc ms_channel_source_desc={
	(MSFilterId)MS_SOURCE_CHANNEL_ID,
	"MSDummyChannelSource",
	"MSDummyChannelSource filter",
	MS_FILTER_OTHER,
	NULL,
	0,
	1,
	channel_source_filter_init,
	NULL,
	channel_filter_process,
	NULL,
	channel_filter_uninit,
	methods_source,
	0
};
MSFilterDesc ms_channel_output_desc={
	(MSFilterId)MS_OUTPUT_CHANNEL_ID,
	"MSChannelDummyOutput",
	"MSChannelDummyOutput filter",
	MS_FILTER_OTHER,
	NULL,
	1,
	0,
	channel_output_filter_init,
	NULL,
	channel_filter_process,
	NULL,
	channel_filter_uninit,
	methods_output,
	0
};
void mschannel_init(void){
	ms_filter_register(&ms_channel_source_desc);
	ms_filter_register(&ms_channel_output_desc);
}
