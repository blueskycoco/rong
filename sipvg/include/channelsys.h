#ifndef __CHANNEL__SYS__INC__HH__
#define __CHANNEL__SYS__INC__HH__

#ifdef CHANNELSYS_EXPORTS
#define __CHANNEL_SYS_API__	__declspec(dllexport)
#else
#define __CHANNEL_SYS_API__	
#endif

#ifdef __cplusplus
extern "C" {
#endif
struct channel_t;
typedef struct channel_t channel_t;

struct channel_manager_t;
typedef struct channel_manager_t channel_manager_t;

//数据回调入口
typedef void (*on_channel_notify_cb)(channel_t* c,void* im);

//新建立
__CHANNEL_SYS_API__ channel_t* channel_new(channel_manager_t* m,void* ud,int self_dispatch);
//获取名称
__CHANNEL_SYS_API__ int  channel_get_id(channel_t* c);
//获取额外数据
__CHANNEL_SYS_API__ void* channel_get_user_data(channel_t* c);
//设置回调函数
__CHANNEL_SYS_API__ void channel_set_notify_cb(channel_t* c,on_channel_notify_cb cb);
//消息调度
__CHANNEL_SYS_API__ void channel_dispatch(channel_t* c,void * im);
//通道连接
__CHANNEL_SYS_API__ void channel_link(channel_t* from,channel_t* to,int mode);
//通道断开
__CHANNEL_SYS_API__ void channel_unlink(channel_t* from,channel_t* to,int mode);
//销毁
__CHANNEL_SYS_API__ void channel_destroy(void* c);
__CHANNEL_SYS_API__  void channel_clean(channel_t* c);

//获取默认通道管理器
__CHANNEL_SYS_API__ channel_manager_t* get_default_channel_manager(int del);
__CHANNEL_SYS_API__ void channel_manager_clear(channel_manager_t* m);
//新建
__CHANNEL_SYS_API__ channel_manager_t* channel_manager_new();
//获取外步数据
__CHANNEL_SYS_API__ void* channel_manager_get_user_data(channel_manager_t* m);
//销毁
__CHANNEL_SYS_API__ void channel_manager_destroy(channel_manager_t* m);
//查找通道
__CHANNEL_SYS_API__ channel_t* channel_manager_find(channel_manager_t* m,int id);
//添加通道
__CHANNEL_SYS_API__ void channel_manager_add(channel_manager_t* m,channel_t* ch);
//删除通道
__CHANNEL_SYS_API__ void channel_manager_remove(channel_manager_t* m,channel_t* ch);
//通道连接
__CHANNEL_SYS_API__ void channel_manager_link(channel_manager_t* m,int mode,int from,int to);
//通道断开
__CHANNEL_SYS_API__ void channel_manager_unlink(channel_manager_t* m,int mode,int from,int to);

#ifdef __cplusplus
}
#endif


#endif
