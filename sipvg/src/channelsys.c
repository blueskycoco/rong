#include "channelsys.h"
#include "mediastreamer2/mscommon.h"

//调度者
typedef struct dispatcher_t{
	//调度
	//调度队列
	queue_t	_rq;
	//队列锁
	ms_mutex_t	_rq_lock;
	//调度线程
	ms_thread_t	_th;
	//调度条件
	ms_cond_t	_rq_cond;
	//线程控制
	int running;
	channel_t* ch;
}dispatcher_t;

//通道定义
typedef struct channel_t{
	//通过id还标识该通道
	int channel_id;
	//保存管理者
	channel_manager_t* manager;
	//锁
	ms_mutex_t	_subs_lock,_atts_lock;
	//订阅者列表
	MSList* _subs;
	//参考列表
	MSList* _attaches;
	//通道打开状态
	int opened;

	//数据回调
	on_channel_notify_cb _data_cb;
	//额外数据
	void* ud;
	//用户自己负责调度
	int self_dispatch;
	//调度
	dispatcher_t* dispather;
}channel_t;
void channel_clean(channel_t* c){
    c->_subs=NULL;
    c->_attaches=NULL;
    c->opened=0;
}
//Window睡眠函数
#ifdef WIN32
static void WinSleep(int usec){
	MSG msg;
	while (PeekMessage(&msg, NULL, 0, 0,1)){
    	TranslateMessage(&msg);
    	DispatchMessage(&msg);
	}
	Sleep(usec);
}
#else
#define WinSleep ms_usleep
#endif
//线程函数
void * dispatch_msg_run(void *arg){
	dispatcher_t* d=(dispatcher_t*)arg;
	channel_t* c=d->ch;
	int ret=0;
	mblk_t* m=NULL;
	while(d->running){
		//ms_cond_wait(&d->_rq_cond,&d->_rq_lock);
		ms_mutex_lock(&d->_rq_lock);
		while((m=getq(&d->_rq))!=NULL){

			ms_mutex_lock(&c->_subs_lock);
			MSList* l=c->_subs;
			for(;l;l=l->next){
				channel_t* sub=(channel_t*)l->data;
				//控制派发
				if(sub->_data_cb&&c->opened){
					sub->_data_cb(sub,dupmsg(m));	
				}
			}
			ms_mutex_unlock(&c->_subs_lock);
		}
		ms_mutex_unlock(&d->_rq_lock);		
		freemsg(m);
		ms_usleep(5);
	}
	ms_thread_exit(&ret);
	return NULL;
}
//调度
dispatcher_t* dispatcher_new(channel_t* ch){
	dispatcher_t* d=ms_new0(dispatcher_t,1);
	d->ch=ch;
	qinit(&d->_rq);
	ms_mutex_init(&d->_rq_lock,NULL);
	ms_cond_init(&d->_rq_cond,NULL);
	//创建线程序
	d->running=1;
	ms_thread_create(&d->_th,NULL,dispatch_msg_run,d);
	return d;
}
//调度消息
void dispatch_msg(dispatcher_t* d,void * m){
	ms_mutex_lock(&d->_rq_lock);
	putq(&d->_rq,(mblk_t*)m);
	ms_mutex_unlock(&d->_rq_lock);
	//ms_cond_signal(&d->_rq_cond);
}
//销毁
void dispatcher_destroy(dispatcher_t* d){
	//退出线程序
	d->running=0;
	ms_mutex_lock(&d->_rq_lock);
	flushq(&d->_rq,0);
	ms_mutex_unlock(&d->_rq_lock);
	
	//退出线程序
	/* bug */
	if(d->_th){
		ms_cond_signal(&d->_rq_cond);
		ms_thread_join(d->_th,NULL);
	}

	ms_mutex_destroy(&d->_rq_lock);
	ms_cond_destroy(&d->_rq_cond);
	d->_th=NULL;
}

//通道消息
typedef struct channel_msg_t{
	//发送通道
	channel_t* from;
	//被发送的消息
	mblk_t* im;
}channel_msg_t;

//新构造消息
channel_msg_t* new_channel_msg(channel_t* from,void* m){
	channel_msg_t* msg=ms_new0(channel_msg_t,1);
	mblk_t* im=(mblk_t*)m;
	msg->from=from;
	msg->im=im;
	return msg;
}
//新建立
channel_t* channel_new(channel_manager_t* m,void* ud,int self_dispatch){
	static int __g_channel_id=1;
	channel_t* c=ms_new0(channel_t,1);
	c->channel_id=__g_channel_id++;
	c->manager=m;
	c->ud=ud;
	c->self_dispatch=self_dispatch;
	ms_mutex_init(&c->_atts_lock,NULL);
	ms_mutex_init(&c->_subs_lock,NULL);
	//添加到管理器 
	channel_manager_add(c->manager,c);
	//自己调度
	if(c->self_dispatch){
		c->dispather=dispatcher_new(c);
	}
	return c;
}
//获取额外数据
void* channel_get_user_data(channel_t* c){
	return c->ud;
}
int channel_get_id(channel_t* c){
	return c->channel_id;
}
//设置回调函数
void channel_set_notify_cb(channel_t* c,on_channel_notify_cb cb){
	c->_data_cb=cb;
}
//加付,成为to的订阅者
void channel_subs_add(channel_t* c,channel_t* sub){
	//成为订阅者
	ms_mutex_lock(&c->_subs_lock);
	if(!ms_list_find(c->_subs,sub)){
		c->_subs=ms_list_append(c->_subs,sub);
	}
	ms_mutex_unlock(&c->_subs_lock);

	ms_mutex_lock(&sub->_atts_lock);
	if(!ms_list_find(sub->_attaches,c)){
		//订阅者本身需要增加索引
		sub->_attaches=ms_list_append(sub->_attaches,c);
	}
	ms_mutex_unlock(&sub->_atts_lock);
}
//解除订阅
void channel_subs_remove(void* c1,void* sub1){
	channel_t* c=(channel_t*)c1;
	channel_t* sub=(channel_t*)sub1;

	MSList* l=NULL;
	ms_mutex_lock(&c->_subs_lock);
	l=c->_subs;
	for(;l;l=l->next){
		if(l->data==sub){
			c->_subs=ms_list_remove_link(c->_subs,l);
			break;
		}
	}
	ms_mutex_unlock(&c->_subs_lock);

	//解决
	ms_mutex_lock(&sub->_atts_lock);
	l=sub->_attaches;
	for(;l;l=l->next){
		if(l->data==c){
			sub->_attaches=ms_list_remove_link(sub->_attaches,l);
			break;
		}
	}
	ms_mutex_unlock(&sub->_atts_lock);
}

//打开通道
void channel_open(channel_t* c){
	c->opened=1;
	
}
//关闭通道
void channel_close(channel_t* c){
	//清空对列
	c->opened=0;
}
//通道数据通知
void channel_notify(channel_t* c,void* im){
	MSList* l=NULL;
	if(!c->opened)return;
	if(!c->dispather){
		ms_mutex_lock(&c->_subs_lock);
		l=c->_subs;
		for(;l;l=l->next){
			channel_t* sub=(channel_t*)l->data;
			//控制派发
			if(sub->_data_cb)
				sub->_data_cb(sub,im);		
		}
		ms_mutex_unlock(&c->_subs_lock);
	}
}

//通道连接
void channel_link(channel_t* from,channel_t* to,int mode){
	//互相定阅
	if(mode&0x01)
		channel_subs_add(from,to);
	if(mode&0x02)
		channel_subs_add(to,from);	
	channel_open(from);
	channel_open(to);
}

//通道断开
void channel_unlink(channel_t* from,channel_t* to,int mode){
	channel_close(from);
	channel_close(to);
	//互相定阅
	if(mode&0x01)
		channel_subs_remove(from,to);
	if(mode&0x02)
		channel_subs_remove(to,from);	
}

//销毁
void channel_destroy(void* c1){	
	channel_t* c=(channel_t*)c1;
	//关闭通道
	channel_close(c);
	ms_mutex_lock(&c->_subs_lock);
	if(c->_subs){
		ms_list_for_each2(c->_subs,channel_subs_remove,c);
	}
	c->_subs=ms_list_free(c->_subs);
	ms_mutex_unlock(&c->_subs_lock);
	//所有解付
	ms_mutex_destroy(&c->_subs_lock);
	ms_mutex_destroy(&c->_atts_lock);

	//移出管理器
	channel_manager_remove(c->manager,c);

	if(c->dispather)dispatcher_destroy(c->dispather);
	
	ms_free(c);
}
//渠道管理者
typedef struct channel_manager_t{
	//订阅者列表
	MSList* _channels;
	//待调度消息对列
	MSList* _rq;
	//调度锁
	ms_mutex_t	_rq_lock;
	//调度线程
	ms_thread_t	_th;
	//运行状态
	int running;
	//外部数据
	void* ud;
}channel_manager_t;



//调度线程
static void* __channel_manager_dispatcher_run(void* arg){
	channel_manager_t* mgr=(channel_manager_t*)arg;
	while(mgr->running){
		ms_mutex_lock(&mgr->_rq_lock);
		channel_msg_t* msg=NULL;
		if(mgr->_rq){
			//获取头消息			
			msg=(channel_msg_t*)mgr->_rq->data;
			mgr->_rq=ms_list_remove_link(mgr->_rq,mgr->_rq);
		}
		ms_mutex_unlock(&mgr->_rq_lock);
		if(msg){	
			ms_mutex_lock(&msg->from->_subs_lock);
			//通知处理
			if(msg->from->_subs)
				channel_notify(msg->from,msg->im);
			ms_mutex_unlock(&msg->from->_subs_lock);
			//释放消息
			ms_free(msg);
		}
		//每10毫秒调度一次
		WinSleep(10);
	}
	return NULL;
}

//消息调度
void channel_dispatch(channel_t* c,void * im){
	if(c->dispather){
		dispatch_msg(c->dispather,im);
	}else{
		ms_mutex_lock(&c->_subs_lock);
		if(c->_subs){
			channel_msg_t* msg=new_channel_msg(c,im);
			ms_mutex_lock(&c->manager->_rq_lock);
			c->manager->_rq=ms_list_append(c->manager->_rq,msg);
			ms_mutex_unlock(&c->manager->_rq_lock);
		}else
			freemsg((mblk_t*)im);
		ms_mutex_unlock(&c->_subs_lock);
	}
}

//新建
channel_manager_t* channel_manager_new(){
	channel_manager_t* m=ms_new0(channel_manager_t,1);
	m->running=1;
	//初始化创建
	ms_mutex_init(&m->_rq_lock,NULL);
	//创建调度线程
	ms_thread_create(&m->_th,NULL,__channel_manager_dispatcher_run,m);
	return m;
}
//销毁
void channel_manager_destroy(channel_manager_t* m){
	//释放缓冲数据
	channel_manager_clear(m);
	ms_mutex_destroy(&m->_rq_lock);
	m->_channels=ms_list_free(m->_channels);
}
void channel_manager_clear(channel_manager_t* m){
	ms_mutex_lock(&m->_rq_lock);
	MSList* l=m->_rq;
	for(;l;l->next){
		channel_msg_t* msg=(channel_msg_t*)l->data;
		freemsg(msg->im);
		ms_free(msg);
	}
	m->_rq=ms_list_free(m->_rq);
	ms_mutex_unlock(&m->_rq_lock);
}
//获取默认通道管理器
static channel_manager_t* _default_mgr=channel_manager_new();
channel_manager_t* get_default_channel_manager(int del){
	if(del)channel_manager_destroy(_default_mgr);
	_default_mgr=channel_manager_new();
	return _default_mgr;
}
//获取外步数据
void* channel_manager_get_user_data(channel_manager_t* m){
	return m->ud;
}
//查找通道
channel_t* channel_manager_find(channel_manager_t* m,int id){
	MSList* _l=m->_channels;
	for(;_l;_l=_l->next){
		channel_t* c=(channel_t*)_l->data;
		if(c->channel_id==id){
			return c;
		}
	}	
	return NULL;
}
//添加通道
void channel_manager_add(channel_manager_t* m,channel_t* ch){
	if(ms_list_find(m->_channels,ch))return;
	m->_channels=ms_list_append(m->_channels,ch);
}
//删除通道
void channel_manager_remove(channel_manager_t* m,channel_t* ch){
	MSList* _l=ms_list_find(m->_channels,ch);
	if(_l)m->_channels=ms_list_remove_link(m->_channels,_l);
}
//通道连接
void channel_manager_link(channel_manager_t* m,int mode,int from,int to){
	channel_t* fromChannel=channel_manager_find(m,from);
	channel_t* toChannel=channel_manager_find(m,to);
	channel_link(fromChannel,toChannel,mode);	
}
//通道断开
void channel_manager_unlink(channel_manager_t* m,int mode,int from,int to){
	channel_t* fromChannel=channel_manager_find(m,from);
	channel_t* toChannel=channel_manager_find(m,to);
	channel_unlink(fromChannel,toChannel,mode);
}
