#ifndef __TEST__AUDIO__INC__HH__
#define __TEST__AUDIO__INC__HH__

struct test_audio_file_t;
typedef struct test_audio_file_t test_audio_file_t;

//新建立
test_audio_file_t* test_audio_file_new(sip_voice_service_t* s,const char* filename);
//销毁
void test_audio_file_destroy(test_audio_file_t* t);
//开始播放
void test_audio_file_start(test_audio_file_t* t);
//停止播放
void test_audio_file_stop(test_audio_file_t* t);

#endif