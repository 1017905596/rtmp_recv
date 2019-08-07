#ifndef __RTMP_LIB_H__
#define __RTMP_LIB_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "Socket.h"
#include <fcntl.h>
#include "Pthread.h"
#include "rtmp_amf0.h"
#include "rtmp_tots.h"

#define MAX_BUF_LEN  1024*64
#define MAX_CHUNK_ID 64

typedef enum rtmp_status_s
{
    rtmp_status_init = 0,
    rtmp_status_error,
    rtmp_status_send_c0c1,
    rtmp_status_wait_s0s1,
    rtmp_status_send_c2,
    rtmp_status_wait_s2,
    rtmp_status_wait_chunk,
    rtmp_status_wait_connect,
    rtmp_status_wait_create_stream,
    rtmp_status_wait_play,
    rtmp_status_wait_data,
}rtmp_status_t;


typedef struct rtmp_chunk_s{
    unsigned int message_size;
	unsigned int last_message_size;
    unsigned int message_stream_id;
    unsigned int message_type;
	unsigned int  time_stamp;
    unsigned char *message_buf;
	unsigned long message_buf_len;
	unsigned long message_buf_pos;
	unsigned char  rtmp_fmt0_buf[12];
}rtmp_chunk_t;

typedef struct rtmp_metadata_s{
    unsigned char server[128];
	double width;
    double height;
    double displayWidth;
	double displayHeight;
    double duration;
	double framerate;
	double fps;
	double videodatarate;
	double videocodecid;
	double audiodatarate;
	double audiocodecid;
	unsigned char profile[128];
	unsigned char level[128];
}rtmp_metadata_t;

typedef struct flv_tag_videodata_h_s
{
    unsigned char codec_id:4;                /* 视频编码ID */
    unsigned char frame_type:4;            /* 帧类型 */     
}flv_tag_videodata_h_t;

typedef struct flv_tag_avcvideopacket_s{
    unsigned char avc_packet_type;            /* 关键帧标志 */
    unsigned char composition_time[3];        
}flv_tag_avcvideopacket_t;
typedef struct flv_tag_audiodata_h_s
{
    unsigned char sound_type:1;            /* 音频类型，0 = mono，1 = stereo */        
    unsigned char sound_size:1;            /* 采样精度，0 = 8bits，1 = 16bits */        
    unsigned char sound_rate:2;            /* 第5-6位的数值表示采样率，0 = 5.5 kHz，1 = 11 kHz，2 = 22 kHz，3 = 44 kHz */    
    unsigned char sound_format:4;            /* 音频数据格式 */
}flv_tag_audiodata_h_t;

class rtmp_client{
public:
	rtmp_client();
	virtual ~rtmp_client();

public:
	//连接相关
	int rtmp_setupurl(char *url ,int UrlLen);
	//开启播放(连接、信令交互、数据存储)
	int rtmp_start_play();
	//开启publish 占时未实现
	int rtmp_start_publish();
protected:
	//内部处理
	int rtmp_parse_cur_url(char *url ,int UrlLen);
	//信令
	int rtmp_send_to_server_c0c1();
	int rtmp_send_to_server_c2();
	int rtmp_send_to_server_connect();
	int rtmp_send_to_server_win_ack_size(char *pag_buf,int pag_len);
	int rtmp_send_to_server_create_stream(char *pag_buf,int pag_len);
	int rtmp_send_to_server_checkbw(char *pag_buf,int pag_len);
	int rtmp_send_to_server_get_stream_length();
	int rtmp_send_to_server_play();
	int rtmp_send_to_server_Set_buffer_length();

	//收据接收
	int rtmp_process_chunk(unsigned char *Bufdata,int len);
	int rtmp_process_message(rtmp_chunk_t *chunk,unsigned char *messsge_data,int data_len);
	int rtmp_process_cmd_0x14(unsigned char *messsge_data,int data_len);
	int rtmp_process_cmd_0x14_connect_result();
	int rtmp_process_cmd_0x14_create_stream_result(unsigned char *messsge_data,int data_len);
#ifdef SAVE_FLV
	int rtmp_process_cmd_0x12_onmetadata_result(rtmp_metadata_t *data,unsigned char *messsge_data,int data_len);
	int rtmp_process_metadata_to_flv(rtmp_chunk_t *chunk,rtmp_metadata_t data);
	int rtmp_process_data_to_flv(rtmp_chunk_t *chunk,unsigned char *messsge_data,int data_len);
#else
	int rtmp_process_video(rtmp_chunk_t *chunk,unsigned char *messsge_data,int data_len);
	int rtmp_process_audio(rtmp_chunk_t *chunk,unsigned char *messsge_data,int data_len);
#endif
protected:
	static Dthr_ret WINAPI rtmp_recv_thread(LPVOID lParam);
	void rtmp_recv_data();
protected:
	Socket         rtmp_socket;
	rtmp_amf0      amf0_process;
	HANDLE         rtmp_thread;
	rtmp_status_t  rtmp_status;

	char           rtmp_url[512];
	char           rtmp_host[512];
	char           rtmp_app[32];
	char           rtmp_stream[128];
	int            rtmp_port;

	unsigned int   rtmp_last_video_timestamp;
	unsigned int   rtmp_last_audio_timestamp;
	unsigned int   rtmp_last_message_type;
	double         rtmp_now_stream_id;
	unsigned int   rtmp_chunk_max_size;
	unsigned int   acknowledgement_size;
	unsigned int   bandwidth;

	unsigned char  *m_pBuffer;
	int            m_iParsePos;
    //flv
	HANDLE         flv_fp;
	rtmp_chunk_t   rtmp_chunk[MAX_CHUNK_ID];
	//ts
	rtmp_tots      rtmp_savetots;
	unsigned char  video_head_data[1024];
	int            video_head_data_len;
	unsigned char  audio_head_data[1024];
	int            audio_head_data_len;
};
#endif