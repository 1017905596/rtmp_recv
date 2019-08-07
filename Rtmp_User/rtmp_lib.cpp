#include "rtmp_lib.h"
#include <time.h>

rtmp_client::rtmp_client(){
	memset(rtmp_url,0,sizeof(rtmp_url));
	memset(rtmp_app,0,sizeof(rtmp_app));
	memset(rtmp_host,0,sizeof(rtmp_host));
	memset(rtmp_stream,0,sizeof(rtmp_stream));
	rtmp_port = 0;
	flv_fp = INVALID_HANDLE_VALUE;

	m_pBuffer = new unsigned char[MAX_BUF_LEN];
	memset(m_pBuffer,0,MAX_BUF_LEN);

	memset(rtmp_chunk,0,sizeof(rtmp_chunk_t)*MAX_CHUNK_ID);
}

rtmp_client::~rtmp_client(){

}
/***************************************************************
//解析当前url
//
***************************************************************/
int rtmp_client::rtmp_parse_cur_url(char *Url, int UrlLen)
{
	char serverinfo[1024] = {0};
	char serverport[32] = {0};
	char serverhost[512] = {0};
	char *HeadPos = Url;
	int HeadLend = 7;// rtmp://
	
	if(HeadPos == NULL){
		printf("url is error :%s\n",HeadPos);
		return -1;
	}
	char *PosEnd = strstr(HeadPos + HeadLend,"/");
	int Poslen = 0;
	if(PosEnd == NULL){
		Poslen = UrlLen - HeadLend;
	}else{
		Poslen = (PosEnd - HeadPos) - HeadLend;
		//获取app
		char *PosApp = strstr(PosEnd + 1,"/");
		if(PosApp != NULL){
			strncpy(rtmp_app,PosEnd + 1,(PosApp - PosEnd - 1));
			if(strstr(PosApp + 1,"mp4") != NULL){
				if(strstr(PosApp + 1,":") != NULL){
					strncpy(rtmp_stream,PosApp + 1,sizeof(rtmp_stream));
				}else{
					sprintf(rtmp_stream,"mp4:%s",PosApp + 1);
				}
			}else{
				strncpy(rtmp_stream,PosApp + 1,sizeof(rtmp_stream));
			}
		}else{
			strcpy(rtmp_app,"live");
			strcpy(rtmp_stream,"");
		}

		printf("rtmp:%s %s\n",rtmp_app,rtmp_stream);
	}
	strncpy(serverinfo,HeadPos + HeadLend,Poslen);
	//截取ip以及端口
	char *PosPort = strstr(serverinfo,":");
	int Port_len = 0;
	if(PosPort != NULL){
		Port_len = PosPort-serverinfo;
		strncpy(serverport,(serverinfo + Port_len + 1),(Poslen - Port_len));
		strncpy(serverhost,serverinfo,Port_len);
	}else{
		strcpy(serverport,"1935");
		strncpy(serverhost,serverinfo,Poslen);
	}	
	rtmp_port = atoi(serverport);
	strcpy(rtmp_host,serverhost);
	return 0;
}
/***************************************************************
//设置播放串
//
***************************************************************/
int rtmp_client::rtmp_setupurl(char *url ,int UrlLen){
	strncpy(rtmp_url,url,sizeof(rtmp_url));
	rtmp_parse_cur_url(url,UrlLen);
	
	printf("Host:%s,Port:%d\n",rtmp_host,rtmp_port);
	return 0;
}

/***************************************************************
//建立连接，并播放
//
***************************************************************/
int rtmp_client::rtmp_start_play(){
	if(rtmp_socket.Create(rtmp_host,SOCK_STREAM) == -1){
		printf("tcp socket create error\n");
		return -1;
	}

	if(rtmp_socket.Connect(rtmp_port,20000) < 0){
		printf("socket connect error\n");
		return -1;
	}
	//默认128个字节
	rtmp_chunk_max_size = 128;
	//发送c0c1
	rtmp_send_to_server_c0c1();
	//创建线程处理
	CPthread *thread = new CPthread();
	thread->Create(rtmp_recv_thread,this);
	return 0;
}
/***************************************************************
//开启线程
//
***************************************************************/
Dthr_ret WINAPI rtmp_client::rtmp_recv_thread(LPVOID lParam){
	((rtmp_client *)lParam)->rtmp_recv_data();
	return NULL;
}
/***************************************************************
//发送c0c1
//
***************************************************************/
int rtmp_client::rtmp_send_to_server_c0c1(){
	int ret = -1;
	char c0c1[1537] = {0};//按照协议为1537个字节
	unsigned int time_now = (unsigned int)time(NULL);
	
	rtmp_status = rtmp_status_send_c0c1;
	c0c1[0] = 0x03;
	memcpy(c0c1 + 5,&time_now,sizeof(time_now));
	
	printf("strat c1!!!\n");
	ret = rtmp_socket.Send(c0c1,sizeof(c0c1));
	if(ret < 0){
		printf("strat c1 error!!!\n");
	}else{
		rtmp_status = rtmp_status_wait_s0s1;
	}
	return ret;
}
/***************************************************************
//发送c2
//
***************************************************************/
int rtmp_client::rtmp_send_to_server_c2(){
	int ret = -1;
	char c2[1536] = {0};
	unsigned int time_now = (unsigned int)time(NULL);

	rtmp_status = rtmp_status_send_c2;
	memcpy(c2 + 4,&time_now,sizeof(time_now));
	printf("strat c2!!!\n");

	ret = rtmp_socket.Send(c2,sizeof(c2));
	if(ret < 0){
		printf("start c2 error!!!\n");
	}else{
		rtmp_status = rtmp_status_wait_s2;
	}
	return ret;
}
/***************************************************************
//发送connect
//
***************************************************************/
int rtmp_client::rtmp_send_to_server_connect(){
	int ret = -1;
	char pag_buf[1024*5] = {0};
	int  amf0_ret = 0;
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + sizeof(pag_buf);
	int message_len = 0;
	int send_len = 0;
	const char header_len = 12;
	//fmt+csid
	*data_ptr = 0x03;
	data_ptr ++;
	//timestamp:3
	data_ptr += 3;
	//message len :3
	data_ptr += 3;
	//type id:1
	*data_ptr = 0x14;
	data_ptr ++;
	//stream id:4
	data_ptr += 4;
	//String connect
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"connect");
	data_ptr += amf0_ret;
	//Number 1 
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,1);
	data_ptr += amf0_ret;
	//object header
	amf0_ret = amf0_process.rtmp_amf0_push_object_header(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;
//app
	//object 子目录 name
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"app");
	data_ptr += amf0_ret;
	//object 子目录 string
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,rtmp_app);
	data_ptr += amf0_ret;
//flashVer
	//object 子目录 name
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"flashVer");
	data_ptr += amf0_ret;
	//object 子目录 string
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"LNX 9,0,124,2");
	data_ptr += amf0_ret;
//tcUrl
	//object 子目录 name
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"tcUrl");
	data_ptr += amf0_ret;
	//object 子目录 string
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,rtmp_url);
	data_ptr += amf0_ret;
//fpad
	//object 子目录 name
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"fpad");
	data_ptr += amf0_ret;
	//object 子目录 string
	amf0_ret = amf0_process.rtmp_amf0_push_bool(data_ptr,data_end-data_ptr,0);
	data_ptr += amf0_ret;
//capabilities
	//object 子目录 name
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"capabilities");
	data_ptr += amf0_ret;
	//object 子目录 string
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,15);
	data_ptr += amf0_ret;
//audioCodecs
	//object 子目录 name
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"audioCodecs");
	data_ptr += amf0_ret;
	//object 子目录 string
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,4071);
	data_ptr += amf0_ret;
//videoCodecs
	//object 子目录 name
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"videoCodecs");
	data_ptr += amf0_ret;
	//object 子目录 string
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,252);
	data_ptr += amf0_ret;
//videoFunction
	//object 子目录 name
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name(data_ptr,data_end-data_ptr,"videoFunction");
	data_ptr += amf0_ret;
	//object 子目录 string
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,1);
	data_ptr += amf0_ret;
	//object end
	amf0_ret = amf0_process.rtmp_amf0_push_object_ender(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;
	//添加message len
	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
    memcpy( pag_buf + 4, ((char *)(&message_len)) + 1, 3 );
	//0xc3表示第二个chunk的头
	send_len = amf0_process.rtmp_amf0_splite_pack(0xc3,header_len,pag_buf,data_ptr - pag_buf);
	
	printf("strat connect %d\n",send_len);
	
	ret = rtmp_socket.Send(pag_buf,send_len);
	if(ret < 0){
		printf("strat connect error !!!\n");
	}else{
		rtmp_status = rtmp_status_wait_connect;
	}
	return ret; 
}
/***************************************************************
//发送set win ack size
//
***************************************************************/
int rtmp_client::rtmp_send_to_server_win_ack_size(char *pag_buf,int pag_len){
	unsigned int win_size = acknowledgement_size;
	int message_len;
	const char header_len = 12;
	char *data_ptr = pag_buf;

	if(pag_len < (header_len + 32)){
		return -1;
	}
	//fmt 2 1个字节
	*data_ptr = 0x02;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x05;
	data_ptr ++;
	//stream id 4个字节
	data_ptr += 4;
	//body
	win_size = SWAP_4BYTE(win_size);
	memcpy(data_ptr, &win_size, 4 );
    data_ptr += 4;

	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
	memcpy( pag_buf+4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;
}
/***************************************************************
//发送create stream
//
***************************************************************/
int rtmp_client::rtmp_send_to_server_create_stream(char *pag_buf,int pag_len){
	int message_len = 0;
	int  amf0_ret = 0;
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + pag_len;
	const char header_len = 8;

	if(pag_len < header_len + 32){
		return -1;
	}
	//fmt 1个字节  0100 0011
	*data_ptr = 0x43;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x14;
	data_ptr ++;
	//createStream
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"createStream");
	data_ptr += amf0_ret;
	//number
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,2);
	data_ptr += amf0_ret;
	//NULL
	amf0_ret = amf0_process.rtmp_amf0_push_null(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;

	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
	memcpy( pag_buf+4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;
}
/***************************************************************
//发送checkbw
//
***************************************************************/
int rtmp_client::rtmp_send_to_server_checkbw(char *pag_buf,int pag_len){
	int message_len = 0;
	int  amf0_ret = 0;
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + pag_len;
	const char header_len = 8;

	if(pag_len < header_len + 32){
		return -1;
	}
	//fmt 1个字节  0100 0011
	*data_ptr = 0x43;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x14;
	data_ptr ++;
	//createStream
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"_checkbw");
	data_ptr += amf0_ret;
	//number
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,3);
	data_ptr += amf0_ret;
	//NULL
	amf0_ret = amf0_process.rtmp_amf0_push_null(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;

	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
	memcpy( pag_buf+4, ((char *)(&message_len)) + 1, 3 );

	return data_ptr - pag_buf;
}
/***************************************************************
//发送get stream length
//
***************************************************************/
int rtmp_client::rtmp_send_to_server_get_stream_length(){
	int message_len = 0;
	int  amf0_ret = 0;
	char pag_buf[1024] = {0};
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + sizeof(pag_buf);
	const char header_len = 12;

	//fmt 1个字节  0000 1000
	*data_ptr = 0x08;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x14;
	data_ptr ++;
	//stream id 4个字节
	data_ptr += 4;
	//getStreamLength
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"getStreamLength");
	data_ptr += amf0_ret;
	//number
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,4);
	data_ptr += amf0_ret;
	//NULL
	amf0_ret = amf0_process.rtmp_amf0_push_null(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;
	//getStreamLength
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,rtmp_stream);
	data_ptr += amf0_ret;

	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
	memcpy( pag_buf+4, ((char *)(&message_len)) + 1, 3 );

	return  rtmp_socket.Send(pag_buf,data_ptr - pag_buf);
}
/***************************************************************
//发送play
//
***************************************************************/
int rtmp_client::rtmp_send_to_server_play(){
	int message_len = 0;
	int  amf0_ret = 0;
	char pag_buf[1024] = {0};
	char *data_ptr = pag_buf;
	char *data_end = pag_buf + sizeof(pag_buf);
	const char header_len = 12;

	//fmt 1个字节  0000 1000
	*data_ptr = 0x08;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x14;
	data_ptr ++;
	//stream id 4个字节,play时需要发送当前播放的stream id
	*data_ptr = (int)rtmp_now_stream_id;
	data_ptr += 4;
	//play
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,"play");
	data_ptr += amf0_ret;
	//number
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,5);
	data_ptr += amf0_ret;
	//NULL
	amf0_ret = amf0_process.rtmp_amf0_push_null(data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;
	//play
	amf0_ret = amf0_process.rtmp_amf0_push_string(data_ptr,data_end-data_ptr,rtmp_stream);
	data_ptr += amf0_ret;
	//number
	amf0_ret = amf0_process.rtmp_amf0_push_number(data_ptr,data_end-data_ptr,-2000.0);
	data_ptr += amf0_ret;

	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
	memcpy( pag_buf+4, ((char *)(&message_len)) + 1, 3 );

	return  rtmp_socket.Send(pag_buf,data_ptr - pag_buf);
}
/***************************************************************
//发送set buffer length
//
***************************************************************/
int rtmp_client::rtmp_send_to_server_Set_buffer_length(){
	int message_len = 0;
	char pag_buf[1024] = {0};
	char *data_ptr = pag_buf;
	const char header_len = 8;

	//fmt 1个字节  0100 0010
	*data_ptr = 0x42;
	data_ptr ++;
	//timestamp 3个字节
	data_ptr += 3;
	//size 
	data_ptr += 3;
	//type id 
	*data_ptr = 0x04;
	data_ptr ++;
	//event type
	data_ptr[1] = 0x03;
	data_ptr += 2;
	//stream id
	data_ptr[3] = 0x01;
	data_ptr += 4;
	//buffer length
    int v = 3000;
	v = SWAP_4BYTE(v);
	memcpy(data_ptr, &v, 4 );
    data_ptr += 4;

	message_len = data_ptr - pag_buf - header_len;
	message_len = SWAP_4BYTE(message_len);
	memcpy( pag_buf+4, ((char *)(&message_len)) + 1, 3 );

	return  rtmp_socket.Send(pag_buf,data_ptr - pag_buf);
}

/***************************************************************
//0x14 命令消息处理
//
***************************************************************/
int rtmp_client::rtmp_process_cmd_0x14(unsigned char *messsge_data,int data_len){
	char command_name[256]={0};
    int ret = 0;

	ret = amf0_process.rtmp_amf0_pop_string(command_name,sizeof(command_name),messsge_data,data_len);
	if(ret < 0){
		return -1;
	}
	printf("recv AMF0 Command:%s\n",command_name);

	if(strcmp(command_name,"_result") == 0){
		if(rtmp_status == rtmp_status_wait_connect){
			return rtmp_process_cmd_0x14_connect_result();
		}else if(rtmp_status == rtmp_status_wait_create_stream){
			return rtmp_process_cmd_0x14_create_stream_result(messsge_data + ret,data_len - ret);
		}
	}

	return 0;
}

/***************************************************************
//0x14 connect 响应解析，并发送create stream
//
***************************************************************/
//不用关心connect返回信息,仅需要执行create stream
int rtmp_client::rtmp_process_cmd_0x14_connect_result(){
	char buf[1024] = {0};
	int  ret_len = 0;
	int  data_pos = 0;

	ret_len = rtmp_send_to_server_win_ack_size(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;
	ret_len = rtmp_send_to_server_create_stream(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;
	ret_len = rtmp_send_to_server_checkbw(buf + data_pos,sizeof(buf) - data_pos);
	if(ret_len == -1){
		return -1;
	}
	data_pos += ret_len;
	rtmp_socket.Send(buf,data_pos);
	rtmp_status = rtmp_status_wait_create_stream;
	return 0;
}

/***************************************************************
//0x14 create stream消息响应，并发送play命令
//
***************************************************************/
int rtmp_client::rtmp_process_cmd_0x14_create_stream_result(unsigned char *messsge_data,int data_len){
	int ret = 0;
	double trans_id = 0.0;
	ret = amf0_process.rtmp_amf0_pop_number(&trans_id, messsge_data, data_len );
    if( ret < 0 ){
        return -1;
	}
	messsge_data += ret;
	data_len -= ret;

	ret = amf0_process.rtmp_amf0_pop_null(messsge_data, data_len );
    if( ret < 0 ){
        return -1;
	}
	messsge_data += ret;
	data_len -= ret;

	ret = amf0_process.rtmp_amf0_pop_number(&rtmp_now_stream_id, messsge_data, data_len );
    if( ret < 0 ){
        return -1;
	}
	rtmp_send_to_server_get_stream_length();
	rtmp_send_to_server_play();
	rtmp_send_to_server_Set_buffer_length();

	rtmp_status = rtmp_status_wait_play;
	return 0;
}
//-------------------------------------------------------------------------------//

/***************************************************************
//chunk消息处理，合并message
//
***************************************************************/
int rtmp_client::rtmp_process_chunk(unsigned char *Bufdata,int datalen){
	rtmp_chunk_t chunk_info = {0};
	int fmt = 0;
	int chunk_id = 0;
	int chunk_header_len = 0;
	int is_new_pack = 0;
	int chunk_len = 0;

	if(datalen == 0){
		return 1;
	}

	fmt = (Bufdata[0] & 0xc0) >> 6;
	chunk_id = Bufdata[0] & 0x3f;

	if(chunk_id == 0){
		chunk_id = Bufdata[1] + 64;
		Bufdata ++;
	}else if(chunk_id == 1){
		chunk_id = Bufdata[2]*256 +  Bufdata[1] + 64;
		Bufdata += 2;
	}
	//一个用户端不用使用这么多路流
	if(chunk_id >= MAX_CHUNK_ID){
		printf("chunk_id:%d\n",chunk_id);
		//chunk_id = chunk_id%MAX_CHUNK_ID;
		return -1;
	}
	printf("fmt:%d\n",fmt);
	switch(fmt){
		case 0:
			chunk_header_len = 11;
			if(datalen < chunk_header_len + 1){
				return 1;
			}
			//timestamp 3个字节
			chunk_info.time_stamp = (((unsigned int)(Bufdata[1]))<<16) + (((unsigned int)(Bufdata[2]))<<8) + Bufdata[3];
			//message_size 3个字节
			chunk_info.message_size = (((unsigned int)(Bufdata[4]))<<16) + (((unsigned int)(Bufdata[5]))<<8) + Bufdata[6];
			//message type id 1个字节
			chunk_info.message_type = Bufdata[7];
			//steam id 4个字节，小端存储
			chunk_info.message_stream_id = (((unsigned int)(Bufdata[8]))<<24) + (((unsigned int)(Bufdata[9]))<<16) + 
											(((unsigned int)(Bufdata[10]))<<8) + Bufdata[11];
			memcpy(rtmp_chunk[chunk_id].rtmp_fmt0_buf,Bufdata + 1,chunk_header_len);

			if(chunk_info.time_stamp == 0xffffff){
				 if( datalen < chunk_header_len + 5 ){
                      return 1;
                 }

				chunk_info.time_stamp = (((unsigned int)(Bufdata[12]))<<24) + (((unsigned int)(Bufdata[13]))<<16)
					+ (((unsigned int)(Bufdata[14]))<<8) + Bufdata[15];
				printf("0x%x,0x%x,0x%x,0x%x\n",Bufdata[12],Bufdata[13],Bufdata[14],Bufdata[15]);
				chunk_header_len += 4;
			}
			if(chunk_info.message_type == 0x08){
				rtmp_last_audio_timestamp = chunk_info.time_stamp;
			}else if(chunk_info.message_type == 0x09){
				rtmp_last_video_timestamp = chunk_info.time_stamp;
			}
			rtmp_last_message_type = chunk_info.message_type;
			printf("--------------message:%d--time:%ld------\n",chunk_info.message_size,chunk_info.time_stamp);
			break;
		case 1:
			chunk_header_len = 7;
			if(datalen < chunk_header_len + 1){
				return 1;
			}
			//timestamp 3个字节 fmt 为1情况下，time为上一个chunk的相对时间
			chunk_info.time_stamp = (((unsigned int)(Bufdata[1]))<<16) + (((unsigned int)(Bufdata[2]))<<8) + Bufdata[3];
			//message_size 3个字节
			chunk_info.message_size = (((unsigned int)(Bufdata[4]))<<16) + (((unsigned int)(Bufdata[5]))<<8) + Bufdata[6];
			//message type id 1个字节
			chunk_info.message_type = Bufdata[7];
			memcpy(rtmp_chunk[chunk_id].rtmp_fmt0_buf,Bufdata + 1,chunk_header_len);
			//差值，一般不会出现
			if(chunk_info.time_stamp == 0xffffff){
				 if( datalen < chunk_header_len + 5 ){
                      return 1;
                 }

				chunk_info.time_stamp = (((unsigned int)(Bufdata[8]))<<24) + (((unsigned int)(Bufdata[9]))<<16)
					+ (((unsigned int)(Bufdata[10]))<<8) + Bufdata[11];
				chunk_header_len += 4;
			}

			if(chunk_info.message_type == 0x08){
				chunk_info.time_stamp += rtmp_last_audio_timestamp;
				rtmp_last_audio_timestamp = chunk_info.time_stamp;
			}else if(chunk_info.message_type == 0x09){
				chunk_info.time_stamp += rtmp_last_video_timestamp;
				rtmp_last_video_timestamp = chunk_info.time_stamp;
			}
			rtmp_last_message_type = chunk_info.message_type;
			printf("--------------message:%d--time:%ld------\n",chunk_info.message_size,chunk_info.time_stamp);
			break;
		case 2:
			chunk_header_len = 3;
			if(datalen < chunk_header_len + 1){
				return 1;
			}
			//timestamp 3个字节 时间差
			chunk_info.time_stamp = (((unsigned int)(Bufdata[1]))<<16) + (((unsigned int)(Bufdata[2]))<<8) + Bufdata[3];
			if(chunk_info.time_stamp == 0xffffff){
				 if( datalen < chunk_header_len + 5 ){
                      return 1;
                 }

				chunk_info.time_stamp = (((unsigned int)(Bufdata[4]))<<24) + (((unsigned int)(Bufdata[5]))<<16)
					+ (((unsigned int)(Bufdata[6]))<<8) + Bufdata[7];
				chunk_header_len += 4;
			}
			
			if(rtmp_last_message_type == 0x08){
				chunk_info.time_stamp += rtmp_last_audio_timestamp;
				rtmp_last_audio_timestamp = chunk_info.time_stamp;
			}else if(rtmp_last_message_type == 0x09){
				chunk_info.time_stamp += rtmp_last_video_timestamp;
				rtmp_last_video_timestamp = chunk_info.time_stamp;
			}
			printf("--------------message:%d--time:%ld------\n",chunk_info.message_size,chunk_info.time_stamp);
			break;
		case 3:
			if(datalen < 1){
				return 1;
			}
			break;
		default:
			return -1;
			break;
	}
	
	if(rtmp_chunk[chunk_id].last_message_size == 0){
		is_new_pack = 1;
		if(chunk_info.message_size >= rtmp_chunk_max_size){
			chunk_len = rtmp_chunk_max_size;
		}else{
			chunk_len = chunk_info.message_size;
		}
	}else{
		if(rtmp_chunk[chunk_id].last_message_size >= rtmp_chunk_max_size){
			chunk_len = rtmp_chunk_max_size;
		}else{
			chunk_len = rtmp_chunk[chunk_id].last_message_size;
		}
	}
	//数据不够一个chunk时，继续接收
	if(datalen < chunk_len + chunk_header_len + 1)
    {
        return 1;
    }
	//新包
	if(is_new_pack){
		rtmp_chunk[chunk_id].last_message_size = chunk_info.message_size;
		rtmp_chunk[chunk_id].time_stamp = chunk_info.time_stamp;
		rtmp_chunk[chunk_id].message_size = chunk_info.message_size;
		rtmp_chunk[chunk_id].message_type = chunk_info.message_type;
		rtmp_chunk[chunk_id].message_stream_id = chunk_info.message_stream_id;
		//一个chunk就为一个message
		if(chunk_len == chunk_info.message_size){
			rtmp_chunk[chunk_id].last_message_size -= chunk_len;
			m_iParsePos += chunk_len + chunk_header_len + 1;
			return rtmp_process_message(&rtmp_chunk[chunk_id],Bufdata + chunk_header_len + 1, chunk_len);
		}else{
			//其余情况创建buf进行缓存message
			delete rtmp_chunk[chunk_id].message_buf;
			rtmp_chunk[chunk_id].message_buf_pos = 0;
			rtmp_chunk[chunk_id].message_buf_len = (chunk_info.message_size + 64*1024)/(64*1024)*(64*1024);
			rtmp_chunk[chunk_id].message_buf = new unsigned char[rtmp_chunk[chunk_id].message_buf_len];
			if(rtmp_chunk[chunk_id].message_buf == NULL){
				return -1;
			}
		}
	}

	memcpy(rtmp_chunk[chunk_id].message_buf + rtmp_chunk[chunk_id].message_buf_pos,Bufdata + chunk_header_len + 1,chunk_len);
	rtmp_chunk[chunk_id].message_buf_pos += chunk_len;
	m_iParsePos += chunk_len + chunk_header_len + 1;
	rtmp_chunk[chunk_id].last_message_size -= chunk_len;
	
	if(rtmp_chunk[chunk_id].message_buf_pos < rtmp_chunk[chunk_id].message_size){
		return 0;
	}
	return rtmp_process_message(&rtmp_chunk[chunk_id],rtmp_chunk[chunk_id].message_buf,rtmp_chunk[chunk_id].message_size);
}
/***************************************************************
//message消息处理
//
***************************************************************/
int rtmp_client::rtmp_process_message(rtmp_chunk_t *chunk,unsigned char *messsge_data,int data_len){
	rtmp_metadata_t metadata = {0};
	printf("message_type :%d\n",chunk->message_type);
	switch(chunk->message_type){
		//Type ID: Set Chunk Size (0x01)
		case 0x01:
			rtmp_chunk_max_size = (((unsigned int)(messsge_data[0]))<<24) + (((unsigned int)(messsge_data[1]))<<16)
									+ (((unsigned int)(messsge_data[2]))<<8) + messsge_data[3];
			printf("recv Set Chunk Size:%u\n",rtmp_chunk_max_size);
			return 0;
		//Type ID: Window Acknowledgement Size (0x05)
		case 0x05:
			acknowledgement_size = (((unsigned int)(messsge_data[0]))<<24) + (((unsigned int)(messsge_data[1]))<<16)
									+ (((unsigned int)(messsge_data[2]))<<8) + messsge_data[3];
			printf("recv Window Acknowledgement Size:%u\n",acknowledgement_size);
			return 0;
		//Type ID: Set Peer Bandwidth (0x06)
		case 0x06:
			bandwidth =  (((unsigned int)(messsge_data[0]))<<24) + (((unsigned int)(messsge_data[1]))<<16)
							+ (((unsigned int)(messsge_data[2]))<<8) + messsge_data[3];
			printf("recv Set Peer Bandwidth :%u\n",bandwidth);
			return 0;
		//audio
		case 0x08:
#ifdef SAVE_FLV
		return rtmp_process_data_to_flv(chunk,messsge_data,data_len);
#else
		return rtmp_process_audio(chunk,messsge_data,data_len);
#endif
		//video
		case 0x09:
#ifdef SAVE_FLV
		return rtmp_process_data_to_flv(chunk,messsge_data,data_len);
#else
		return rtmp_process_video(chunk,messsge_data,data_len);
#endif
		case 0x12:
#ifdef SAVE_FLV
			int ret = 0;
			char command_name[256]={0};
			ret = amf0_process.rtmp_amf0_pop_string(command_name,sizeof(command_name),messsge_data,data_len);
			if(ret < 0){
				return -1;
			}
			printf("recv AMF0 Command:%s\n",command_name);
			if(strcmp(command_name,"onMetaData") == 0){
				rtmp_process_cmd_0x12_onmetadata_result(&metadata,messsge_data,data_len);
				rtmp_status = rtmp_status_wait_data;
				return rtmp_process_metadata_to_flv(chunk,metadata);
			}else{
				return 0;
			}
			break;
#else
			return 0;
#endif
		//Type ID: AMF0 Command (0x14)
		case 0x14:
			printf("recv AMF0 Command \n");
			return rtmp_process_cmd_0x14(messsge_data,data_len);
		default:
			return 0;
	}
	return 1;
}
#ifdef SAVE_FLV
/***************************************************************
//0x14 create stream消息响应，并发送play命令
//
***************************************************************/
int rtmp_client::rtmp_process_cmd_0x12_onmetadata_result(rtmp_metadata_t *data,unsigned char *messsge_data,int data_len){
	char tmp_buf[256]={0};
    char command_name[256]={0};
    int ret = 0;
    double trans_id = 0.0;
    ret = amf0_process.rtmp_amf0_pop_string(command_name, sizeof(command_name), messsge_data, data_len );
    if( ret < 0 )
        return -1;
    messsge_data += ret;
    data_len -= ret;

    //command object
    ret = amf0_process.rtmp_amf0_pop_object_begin( messsge_data, data_len );
    if( ret < 0 ){
        return -1;
    }
    messsge_data += ret;
    data_len -= ret;

    while( data_len > 0 )
    {
        ret = amf0_process.rtmp_amf0_pop_object_prop_name( tmp_buf, sizeof(tmp_buf), messsge_data, data_len );
        if( ret < 0 )
            return -1;
        else if( ret == 0 )
            break;
        messsge_data += ret;
        data_len -= ret;

		if(strcmp(tmp_buf,"Server") == 0){
			ret = amf0_process.rtmp_amf0_pop_string((char *)data->server,sizeof(data->server), messsge_data, data_len );
		}else if (strcmp(tmp_buf,"width") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->width,messsge_data, data_len);
		}else if (strcmp(tmp_buf,"height") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->height,messsge_data, data_len);
		}else if (strcmp(tmp_buf,"displayWidth") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->displayWidth,messsge_data, data_len);
		}else if (strcmp(tmp_buf,"displayHeight") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->displayHeight,messsge_data, data_len);
		}else if (strcmp(tmp_buf,"duration") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->duration,messsge_data, data_len);
		}else if (strcmp(tmp_buf,"framerate") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->framerate,messsge_data, data_len);
		}else if (strcmp(tmp_buf,"fps") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->fps,messsge_data, data_len);
		}else if (strcmp(tmp_buf,"videodatarate") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->videodatarate,messsge_data, data_len);
		}else if (strcmp(tmp_buf,"videocodecid") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->videocodecid,messsge_data, data_len);
		}else if (strcmp(tmp_buf,"audiodatarate") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->audiodatarate,messsge_data, data_len);
		}else if (strcmp(tmp_buf,"audiocodecid") == 0){
			ret = amf0_process.rtmp_amf0_pop_number(&data->audiocodecid,messsge_data, data_len);
		}else if(strcmp(tmp_buf,"profile") == 0){
			ret = amf0_process.rtmp_amf0_pop_string((char *)data->profile,sizeof(data->server), messsge_data, data_len );
		}else if(strcmp(tmp_buf,"level") == 0){
			ret = amf0_process.rtmp_amf0_pop_string((char *)data->level,sizeof(data->server), messsge_data, data_len );
		}

        if( ret < 0 )
            return -1;
        else if( ret == 0 )
            break;
        messsge_data += ret;
        data_len -= ret;

    }
    ret = amf0_process.rtmp_amf0_pop_object_end( messsge_data, data_len );
    if( ret < 0 )
        return -1;
    messsge_data += ret;
    data_len -= ret;

	return 0;
}
int rtmp_client::rtmp_process_metadata_to_flv(rtmp_chunk_t *chunk,rtmp_metadata_t data){
	static const char flvHeader[] = { 'F', 'L', 'V', 0x01,0x05,0x00, 0x00, 0x00, 0x09,0x00, 0x00, 0x00, 0x00};
	int message_len = 0;
	int tag_len = 0;
	unsigned int amf0_ret = 0;
	unsigned int tag_header = 11;
	unsigned char data_write[2048] = {0};
	unsigned char *data_ptr = data_write;
	unsigned char *data_end = data_write + sizeof(data_write);
	//写入flv头
	flv_fp = file_open("receive.flv", O_CREAT|O_RDWR, FILE_PERMS_ALL,NULL);
	if(flv_fp == INVALID_HANDLE_VALUE){
		return -1;
	}
	file_write(flv_fp,flvHeader,sizeof(flvHeader));

	//写tag header 11个字节固定
	*data_ptr = chunk->rtmp_fmt0_buf[6];
	data_ptr += 11;
	//写data
	//amf 1 type :0x02
	*data_ptr = 0x02;
	data_ptr ++;
	//object onMetaData 
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name((char *)data_ptr,data_end-data_ptr,"onMetaData");
	data_ptr += amf0_ret;
	//amf 2 type:0x08
	*data_ptr = 0x08;
	data_ptr ++;
	//一共多少个array 占定8个;
	int array_len = 7;
	array_len = SWAP_4BYTE(array_len);
	memcpy(data_ptr,(char *)&array_len,4);
	data_ptr += 4;
	//
	//duration
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name((char *)data_ptr,data_end-data_ptr,"duration");
	data_ptr += amf0_ret;
	//duration number
	amf0_ret = amf0_process.rtmp_amf0_push_number((char *)data_ptr,data_end-data_ptr,60);
	data_ptr += amf0_ret;
	//width
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name((char *)data_ptr,data_end-data_ptr,"width");
	data_ptr += amf0_ret;
	//width number
	amf0_ret = amf0_process.rtmp_amf0_push_number((char *)data_ptr,data_end-data_ptr,data.width);
	data_ptr += amf0_ret;
	//height
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name((char *)data_ptr,data_end-data_ptr,"height");
	data_ptr += amf0_ret;
	//height number
	amf0_ret = amf0_process.rtmp_amf0_push_number((char *)data_ptr,data_end-data_ptr,data.height);
	data_ptr += amf0_ret;
	//videodatarate
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name((char *)data_ptr,data_end-data_ptr,"videodatarate");
	data_ptr += amf0_ret;
	//videodatarate number
	amf0_ret = amf0_process.rtmp_amf0_push_number((char *)data_ptr,data_end-data_ptr,0);
	data_ptr += amf0_ret;
	//framerate
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name((char *)data_ptr,data_end-data_ptr,"framerate");
	data_ptr += amf0_ret;
	//framerate number
	amf0_ret = amf0_process.rtmp_amf0_push_number((char *)data_ptr,data_end-data_ptr,data.framerate);
	data_ptr += amf0_ret;
	//videocodecid
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name((char *)data_ptr,data_end-data_ptr,"videocodecid");
	data_ptr += amf0_ret;
	//videocodecid number
	amf0_ret = amf0_process.rtmp_amf0_push_number((char *)data_ptr,data_end-data_ptr,data.videocodecid);
	data_ptr += amf0_ret;
	//filesize
	amf0_ret = amf0_process.rtmp_amf0_push_object_prop_name((char *)data_ptr,data_end-data_ptr,"filesize");
	data_ptr += amf0_ret;
	//filesize number
	amf0_ret = amf0_process.rtmp_amf0_push_number((char *)data_ptr,data_end-data_ptr,1024*1024);
	data_ptr += amf0_ret;
	//object end
	amf0_ret = amf0_process.rtmp_amf0_push_object_ender((char *)data_ptr,data_end-data_ptr);
	data_ptr += amf0_ret;

	//message len
	message_len = data_ptr - data_write - tag_header;
	message_len = SWAP_4BYTE(message_len);
	memcpy( data_write + 1, ((char *)(&message_len)) + 1, 3 );
	printf("data:0x%x,0x%x,0x%x\n",data_write[0],data_write[1],data_write[2]);
	//写 previous tag size
	tag_len = data_ptr - data_write;
	tag_len = SWAP_4BYTE(tag_len);
	memcpy( data_ptr ,((char *)(&tag_len)), 4 );
	printf("data:0x%x,0x%x,0x%x,0x%x\n",data_ptr[0],data_ptr[1],data_ptr[2],data_ptr[3]);
	data_ptr += 4;
	//写文件
	file_write(flv_fp, data_write,data_ptr - data_write);
	return 0;
}

int rtmp_client::rtmp_process_data_to_flv(rtmp_chunk_t *chunk,unsigned char *messsge_data,int data_len){
	unsigned int message_len = 0;
	unsigned int time_stamp = 0;
	unsigned char *data_write = new unsigned char[data_len + 15];
	memset(data_write,0,data_len + 15);

	if(flv_fp == INVALID_HANDLE_VALUE){
		delete data_write;
		return -1;
	}
	//写tag header
	data_write[0] = chunk->rtmp_fmt0_buf[6];
	data_write[1] = chunk->rtmp_fmt0_buf[3];
	data_write[2] = chunk->rtmp_fmt0_buf[4];
	data_write[3] = chunk->rtmp_fmt0_buf[5];
	time_stamp = SWAP_4BYTE(chunk->time_stamp);
	memcpy( data_write + 4, ((char *)(&time_stamp)) + 1, 3 );
	//写data
	memcpy(data_write + 11,messsge_data,data_len);
	//写 previous tag size
	message_len = data_len + 11;
	message_len = SWAP_4BYTE(message_len);
	memcpy( data_write + 11 + data_len, ((char *)(&message_len)), 4 );
	//写文件
	file_write(flv_fp, data_write,data_len + 15);
	printf("data:0x%x,0x%x,0x%x,0x%x,0x%x\n",data_write[12],data_write[13],data_write[14],data_write[15],data_write[16]);
	delete data_write;
	return 0;
}
#else
int  rtmp_client::rtmp_process_video(rtmp_chunk_t *chunk,unsigned char *messsge_data,int data_len){
	pack_data_t pack;
	unsigned int offset_time = 0;
	flv_tag_videodata_h_t *tag_header = (flv_tag_videodata_h_t *)messsge_data;
	flv_tag_avcvideopacket_t *tag_avc = (flv_tag_avcvideopacket_t *)(messsge_data + sizeof(flv_tag_videodata_h_t));
	//video 第一个字节描述帧类型
	if(tag_header->codec_id != 7){
		return -1;
	}
	memset(&pack, 0, sizeof(pack));
	//后面4个字节 描述avc
	messsge_data += 5;
	data_len -= 5;
	//解析数据
	pack.type = data_type_h264;
	offset_time = (((unsigned int)(tag_avc->composition_time[0])) << 16 ) + 
				(((unsigned int)(tag_avc->composition_time[1])) << 8 )
				+ tag_avc->composition_time[2];
    if( offset_time > 1000 )
        offset_time = 0;

	pack.picture_ms = chunk->time_stamp + offset_time;
    pack.decode_ms = chunk->time_stamp;
	//i帧
	if(tag_header->frame_type == 1){
		//AVC的profie和 SPS PPS
		if(tag_avc->avc_packet_type == 0){
			unsigned short v_u16 = 0;
			video_head_data_len = 0;
			//开始保存SPS
			video_head_data[video_head_data_len++] = 0;
			video_head_data[video_head_data_len++] = 0;
			video_head_data[video_head_data_len++] = 0;
			video_head_data[video_head_data_len++] = 1;
			if(data_len < 8){
				return -1;
			}
			//2个字节 SPS的长度
			v_u16 = *((unsigned short*)(messsge_data + 6));
            v_u16 = SWAP_2BYTE(v_u16);
            messsge_data += 8;
            data_len -= 8;
			printf("SPS len:%d\n",v_u16);
			if( v_u16 > 256 ){
				return -1;
			}
			//PPS 以0x67开始
			memcpy(video_head_data + video_head_data_len, messsge_data, v_u16 );

            video_head_data_len += v_u16;
            messsge_data += v_u16;
            data_len -= v_u16;
			//开始保存PPS
			video_head_data[video_head_data_len++] = 0;
			video_head_data[video_head_data_len++] = 0;
			video_head_data[video_head_data_len++] = 0;
			video_head_data[video_head_data_len++] = 1;
			//2个字节 PPS的长度
			v_u16 = *((unsigned short*)(messsge_data + 1));
            v_u16 = SWAP_2BYTE(v_u16);
            messsge_data += 3;
            data_len -= 3;
			printf("PPS len:%d\n",v_u16);
			if( v_u16 > 256 ){
				return -1;
			}
			//PPS 以0x68开始
			memcpy(video_head_data + video_head_data_len, messsge_data, v_u16 );

            video_head_data_len += v_u16;
            messsge_data += v_u16;
            data_len -= v_u16;
			return 0;
		}
		pack.head_data = video_head_data;
        pack.head_data_len = video_head_data_len;
        pack.is_keyframe = 1;
	}
	pack.data = messsge_data;
    pack.data_len = data_len;

	while( data_len > 4 )
	{
		unsigned int len = *((unsigned int*)messsge_data);
		len = SWAP_4BYTE(len);
		if( len + 4 > (unsigned int)data_len )
			return -1;

		messsge_data[0] = 0;
		messsge_data[1] = 0;
		messsge_data[2] = 0;
		messsge_data[3] = 1;

		messsge_data += len + 4;
		data_len -= (len + 4); 
	}
	rtmp_savetots.rtmp_tots_packet_from_cache(&pack);
	return 0;
}

int rtmp_client::rtmp_process_audio(rtmp_chunk_t *chunk,unsigned char *messsge_data,int data_len){
	flv_tag_audiodata_h_t *tag_audio_header = (flv_tag_audiodata_h_t *)messsge_data;
	pack_data_t pack;
	memset(&pack,0,sizeof(pack_data_t));

	pack.data = messsge_data + 1;
	pack.data_len = data_len - 1;

	pack.decode_ms = chunk->time_stamp;
	pack.picture_ms = chunk->time_stamp;

	if(tag_audio_header->sound_format == 10){
		pack.type = data_type_aac;
		//判断audio header
		if(audio_head_data_len == 0 && messsge_data[1] != 0){
			return 0;
		}else if(messsge_data[1] == 0){
			//不带CRC时为7个字节，如果带 则9个字节
			// 同步字节0xfff 占12位
			//参考：https://wiki.multimedia.cx/index.php?title=ADTS
			audio_head_data[0] = 0xFF;    
			audio_head_data[1] = 0xF1;
			audio_head_data[2] = ( ( ((( messsge_data[2] >> 3 ) & 0x1F) - 1) & 0x03 ) << 6 );
			audio_head_data[2] |= ( ( (((messsge_data[2] & 0x07) << 1) | ((messsge_data[3]>>7) & 0x01) ) & 0x0F ) << 2 );
			audio_head_data[2] |= ( ( ((messsge_data[3]>>3) & 0x0F ) >> 2 ) & 0x01 );
			audio_head_data[3] = ( ( ((messsge_data[3]>>3) & 0x0F ) & 0x03 ) << 6 );
			audio_head_data[4] = 0;
			audio_head_data[5] = 0;
			audio_head_data[6] = 0;

			audio_head_data_len = 7;
			return 0;
		}
		pack.head_data = audio_head_data;
        pack.head_data_len = audio_head_data_len;
        pack.data++;
        pack.data_len--;
	}

	unsigned int aac_frame_length = pack.data_len + audio_head_data_len;
	unsigned char b3= audio_head_data[3];
    //ADTS帧长度包括ADTS长度和AAC声音数据长度的和 13位
	audio_head_data[3] |= ( ( aac_frame_length >> 11 ) &0x03);
	audio_head_data[4] =  ( aac_frame_length >> 3 )&0xFF;
	audio_head_data[5] =  ( ( aac_frame_length & 0x07 ) << 5 );                
            
	rtmp_savetots.rtmp_tots_packet_from_cache(&pack);

	audio_head_data[3] = b3;
	return 0;
}
#endif
/***************************************************************
//socket 数据接收
//
***************************************************************/
void rtmp_client::rtmp_recv_data(){
	int size = 0;
	int ret = 0;
	unsigned char *pos  = NULL;
	while(1){
		m_iParsePos = 0;
		ret = rtmp_socket.Receive(m_pBuffer + size,MAX_BUF_LEN - size);
		if(ret <= 0){
			printf("recv data error:%d\n",ret);
			break;
		}else{
			pos = m_pBuffer;
			size += ret;
		}
		printf("data recv size:%d\n",size);
		//s0s1一共1537个字节，不用关心数据
		if(rtmp_status == rtmp_status_wait_s0s1){
			if(size >= 1537){
				pos += 1537;
				size-= 1537;
				if(rtmp_send_to_server_c2() < 0){
					break;
				}
			}
		}
		//S2一共1536个字节，不用关心数据，一般服务器会同时发送s0s1s2
		if(rtmp_status == rtmp_status_wait_s2){
			if(size >= 1536){
				pos += 1536;
				size-= 1536;
				if(rtmp_send_to_server_connect() < 0){
					break;
				}
			}
		}
		//处理相关数据
		if(rtmp_status > rtmp_status_wait_chunk){
			while(1){
				printf("pos:%d size:%d\n",m_iParsePos,size);
				ret = rtmp_process_chunk(pos + m_iParsePos ,size - m_iParsePos);
				if(ret > 0){
					//一次没有收取完成
					size -= m_iParsePos;
					memcpy(m_pBuffer,pos + m_iParsePos, size);
					printf("data recv again\n");
					break;
				}else if(ret < 0){
					printf("data process chunk error!!\n");
					return ;
				}
			}
		}
	}
}