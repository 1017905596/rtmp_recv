#include "rtmp_tots.h"

rtmp_tots::rtmp_tots(){
	memset(&rtmp_packet,0,sizeof(rtmp_packet));
	rtmp_packet.pes_buf = new unsigned char[PACKET_PES_BUF];
	rtmp_packet.ts_fp = file_open("rtmp_live1.ts", O_CREAT|O_RDWR, FILE_PERMS_ALL,NULL);
	rtmp_packet.last_psi_time = get_millisecond64();
}

rtmp_tots::~rtmp_tots(){
	delete rtmp_packet.pes_buf;
	file_close(rtmp_packet.ts_fp);
}
//用于给一视频帧的pes数据添加上pts和dts
void rtmp_tots::rtmp_tots_packet_pes_set_time(unsigned char *buf, uint64 pts, uint64 dts)
{
    buf[0] = ((pts >> 29) | 0x31) & 0x3f;
    buf[1] = (pts >> 22) & 0xff;
    buf[2] = ((pts >> 14) | 0x01) & 0xff;
    buf[3] = (pts >> 7) & 0xff;
    buf[4] = ((pts << 1) | 0x01) & 0xff;

    buf[5] = ((dts >> 29) | 0x11) & 0x1f;
    buf[6] = (dts >> 22) & 0xff;
    buf[7] = ((dts >> 14) | 0x01) & 0xff;
    buf[8] = (dts >> 7) & 0xff;
    buf[9] = ((dts << 1) | 0x01) & 0xff;
}

unsigned int rtmp_tots::rtmp_tots_packet_ts_crc32(unsigned char *data, int len)
{   
    int i = 0;   
    unsigned int crc = 0xFFFFFFFF;    

    if(NULL == data){
        len = 0;
    }

    for(i = 0; i < len; i++) {
        crc = (crc << 8) ^ crc32table[((crc >> 24) ^ *data++) & 0xFF];       
    }

    return crc;
}

int rtmp_tots::rtmp_tots_packet_ts_pat_table(){
	unsigned char pat_header[5] = {0x47, 0x40, 0x00, 0x10, 0x00}; /* 简单初始化pat表,pat表的PID为0x0000,最后一个字节是调整字节 */
    unsigned char pat_table[16] = {0};                            /* 大小为pat_table[2] + 3 */

    pat_table[0] = 0x00;    /* table  id */
    pat_table[1] = 0xb0;    /* section_syntax_indacator = 1,对于PAT,必须为1 */
    pat_table[1] = pat_table[1];   /* section_length,共12bits */
    pat_table[2] = 0x0d;    /* section_length,表示该字段后的pat_table长度 */
    pat_table[3] = 0x00;    /* transport_stream_id */
    pat_table[4] = 0x01;    /* transport_stream_id */
    pat_table[5] = 0xc1;    /* version_number = 00000, current_next_indicator = 1 */
    pat_table[6] = 0x00;    /* section_number */
    pat_table[7] = 0x00;    /* last_section_number */
    /* 下面是一个4个字节的循环,表示NIT,PMT包的PID */
    pat_table[8] = 0x00;    /* program_number */
    pat_table[9] = 0x01;    /* pargram_number */
    pat_table[10] = 0xe1;   /* PMT的PID */
    pat_table[11] = 0x00;   /* PMT的PID：0x100 */

    /* crc32循环校验码 */
    {
        unsigned int crc32 = rtmp_tots_packet_ts_crc32(pat_table, 12);

        pat_table[12] = crc32 >> 24;
        pat_table[13] = (crc32 >> 16) & 0xff;
        pat_table[14] = (crc32 >> 8) & 0xff;
        pat_table[15] = crc32 & 0xff;
    }

    /* 修正pat表的header */
    pat_header[3] = pat_header[3] | (rtmp_packet.pat_num % 0x10);    /* 计数pat表 */

    /* 保存pat表 */
    rtmp_packet.pat_packet[0] = '\0';
    memcpy(rtmp_packet.pat_packet, pat_header, 5);
    memcpy(rtmp_packet.pat_packet + 5, pat_table, 16);
    memset(rtmp_packet.pat_packet + 21, 0xff, TS_PACKET_SIZE - 21);

    return 0;
}

int rtmp_tots::rtmp_tots_packet_ts_pmt_table(){
	unsigned char pmt_header[5] = {0x47, 0x41, 0x00, 0x10, 0x00}; /* 简单初始化pmt表,pmt表的PID为0x0100,最后一个字节是调整字节 */
    unsigned char pmt_table[26] = {0};

    pmt_table[0] = 0x02;    /* table_id : 0x02 */
    pmt_table[1] = 0xb0;    /* section_snytax_indicator:1; zero:1; */
    pmt_table[1] = pmt_table[1];    /* 后四位是section_length */
    pmt_table[2] = 0x17;    /* section_length后8位 */
    pmt_table[3] = 0x00;    /* program number */
    pmt_table[4] = 0x01;   
    pmt_table[5] = 0xc1;    /* reserved:2;version_number:5;current_next_indicator:1; 这个值同pat表一样 */
    pmt_table[6] = 0x00;    /* section_number */
    pmt_table[7] = 0x00;    /* last_section_number */
    pmt_table[8] = VIDEO_PID >> 8 | 0xe0;    /* reserved:3;PCR_PID:5,pcr所在的pid在视频包里,也可以单独的生成一个ta包,专门用于pcr */
    pmt_table[9] = 0x00;    /* PCR_PID:8 */
    pmt_table[10] = 0xf0;    /* reserved:4;program_info_legnth:4 */
    pmt_table[11] = 0x00;    /* program_info_length:8 */
    /* 视频pid */
    pmt_table[12] = 0x1b;    /* stream_type:8;h264类型视频  */
    pmt_table[13] = (VIDEO_PID >> 8) | 0xe0 ;    /* reserved:3;elementary_pid:5 */
    pmt_table[14] = 0x00;
    pmt_table[15] = 0xf0;    /* reserved:4;es_info_length:4 */
    pmt_table[16] = 0x00;    /* es_info_length:4 */
    /* 音频pid */
    pmt_table[17] = 0x0f;    /* stream_type:8;aac类型音频  */
    pmt_table[18] = (AUDIO_PID >> 8) | 0xe0 ;    /* reserved:3;elementary_pid:5 */
    pmt_table[19] = AUDIO_PID & 0xff;    /* elementary_pid:8 */
    pmt_table[20] = 0xf0;    /* reserved:4;es_info_length:4 */
    pmt_table[21] = 0x00;    /* es_info_length:4 */

    /* crc32循环校验码 */
    {
        unsigned int crc32 = rtmp_tots_packet_ts_crc32(pmt_table, 22);

        pmt_table[22] = crc32 >> 24;
        pmt_table[23] = (crc32 >> 16) & 0xff;
        pmt_table[24] = (crc32 >> 8) & 0xff;
        pmt_table[25] = crc32 & 0xff;
    }

    /* 修正pmt表的header,并增加计数,pmt_num同pat_nume一样 */
    pmt_header[3] = pmt_header[3] | (rtmp_packet.pat_num) % 0x10;
    rtmp_packet.pat_num++;    

    /* 保存pmt表 */
    rtmp_packet.pmt_packet[0] = '\0';
    memcpy(rtmp_packet.pmt_packet, pmt_header, 5);
    memcpy(rtmp_packet.pmt_packet + 5, pmt_table, 26);
    memset(rtmp_packet.pmt_packet + 31, 0xff, TS_PACKET_SIZE - 31);

    return 0;
}

int rtmp_tots::rtmp_tots_packet_from_cache(pack_data_t *pack){
	uint64 pts = 0;
	uint64 dts = 0;
	unsigned int len = 0;
	unsigned char *p = NULL;
	
	rtmp_packet.pes_buf[0] = '\0';
	rtmp_packet.pes_len = 0;

	if(pack->head_data != NULL){
		p = pack->head_data;
		len = pack->head_data_len;
	}else{
		p = pack->data;
		len = pack->data_len;
	}
	rtmp_packet.unit_type = es_unit_start_type_init;
	if((p[0] == 0xff) && ((p[1] & 0xf0) == 0xf0)){
		rtmp_packet.unit_type = es_unit_start_type_aac;
    }
	if(p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x01){
        rtmp_packet.unit_type = es_unit_start_type_startcode3;
    }
	if(p[0] == 0x00 && p[1] == 0x00 && p[2] == 0x00 && p[3] == 0x01){
        rtmp_packet.unit_type = es_unit_start_type_startcode4;
	}
	rtmp_packet.pack_data = pack;
	
	if(pack->type == data_type_aac){
		///* 如果音视频的pts时间相差很大时音频采用视频的pts,不然会没声音 */
		if((rtmp_packet.last_video_picture_ms == 0) || 
			((pack->picture_ms - 100 < rtmp_packet.last_video_picture_ms) 
			&& (rtmp_packet.last_video_picture_ms < pack->picture_ms + 100))){
            pts = pack->picture_ms * 90;
        }else{
            pts = rtmp_packet.last_video_picture_ms * 90;
        }
		printf("aac pts %lld\n",pts);
        return rtmp_tots_packet_pes_audio_header(pts);
	}else if(pack->type == data_type_h264){
		dts = pack->decode_ms * 90;
        pts = pack->picture_ms * 90;
        rtmp_packet.last_video_picture_ms = pack->picture_ms;
		printf("h264 pts/dts %lld/%lld\n",pts,dts);
        return rtmp_tots_packet_pes_video_header(dts, pts);
	}
	return 0;
}
//视频
int rtmp_tots::rtmp_tots_packet_pes_video_header(uint64 dts,uint64 pts){
	unsigned char * p = NULL;
    unsigned char header[9] = {0};
    char time_buf[10] = {0};
    unsigned char unit_start[6] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10}; /* 数据帧之间的分割码 */
	pack_data_t *data = rtmp_packet.pack_data;
    unsigned int  es_len = data->head_data_len + data->data_len;

    /* 第0~5位字节 */
    header[0] = 0x00;
    header[1] = 0x00;
    header[2] = 0x01;
    header[3] = 0xe0;    /* 视频 */
    header[4] = 0x00; /* 设置成0x00,这样即使一个pes的长度大于65535,也能够封装 */ 
    header[5] = 0x00;
    header[6] = 0x80;/* 第6位字节,fix_bit = 0x10 */
    header[7] = 0xc0;/* 第7位字节,pts_dts_flag = 0x11 */
    header[8] = 0x0a;/* 第8位字节,PES_header_data_length = 0x0a */

    rtmp_tots_packet_pes_set_time((unsigned char *)(&time_buf), pts, dts);

    memcpy(rtmp_packet.pes_buf, header, 9);
    memcpy(rtmp_packet.pes_buf + 9, time_buf, 10); 

    if(es_len > ( PACKET_PES_BUF - 25)){
        return -1;
    }

    /* 如果head_data不为NULL则指向head_data,否则指向data */
    p = (data->head_data != NULL && data->head_data_len > 0) ? data->head_data : data->data;

    /* 无分割码需要添加分割码,不然有些播放器会失败 */
    if((es_unit_start_type_startcode3 == rtmp_packet.unit_type && p[3] != 0x09) ||
        (es_unit_start_type_startcode4 == rtmp_packet.unit_type && p[4] != 0x09))
    {
        memcpy(rtmp_packet.pes_buf+19, unit_start, 6);
        if(data->head_data != NULL){
            memcpy(rtmp_packet.pes_buf + 25, data->head_data, data->head_data_len);
        }
		if(data->data != NULL){
            memcpy(rtmp_packet.pes_buf + 25 + data->head_data_len, data->data, data->data_len);
        }
        rtmp_packet.pes_len = es_len + 25;
    }
    else
    {
        if(data->head_data != NULL){
            memcpy(rtmp_packet.pes_buf + 19, data->head_data, data->head_data_len);
        }
		if(data->data != NULL){
            memcpy(rtmp_packet.pes_buf + 19 + data->head_data_len, data->data, data->data_len);
        }
        rtmp_packet.pes_len = es_len + 19;
    }

    /* 打上ts的头包,在输出到文件 */
    return rtmp_tots_packet_ts_header(pts); 
}

int rtmp_tots::rtmp_tots_packet_pes_audio_header(uint64 pts){
	unsigned char header[14] = {0};
    pack_data_t *data = rtmp_packet.pack_data;
    unsigned int es_len = data->head_data_len + data->data_len;

    /* 第0~5位字节,0~2是pes的起始码 */
    header[0] = 0x00;
    header[1] = 0x00;
    header[2] = 0x01;
    header[3] = 0xc0;    /* 音频pes_id */
    header[4] = (unsigned char)((es_len + 8) >> 8);//定义PES_packet_length[2],2个字节
    header[5] = (es_len + 8) & 0xff;  /* PES_packet_length：8 */
    header[6] = 0x80;/* 第6位字节,fix_bit = 0x10 */
    header[7] = 0x80;/* 第7位字节,pts_dts_flag = 0x10,音频只要pts,不要dts */
    header[8] = 0x05;/* 第8位字节,PES_header_data_length = 0x05 */
    header[9] = ((pts >> 29) | 0x21) & 0x2f;    /* '0010xxx1' */
    header[10] = (pts >> 22) & 0xff;
    header[11] = ((pts >> 14) | 0x01) & 0xff;
    header[12] = (pts >> 7) & 0xff;
    header[13] = ((pts << 1) | 0x01) & 0xff;

    memcpy(rtmp_packet.pes_buf, header, 14);

    /* 将pes头和es原始数据组合在一起,放在pes缓存中 */
    if(es_len > (PACKET_PES_BUF - 14)){
        return -1;
    }
	if(data->head_data != NULL){
        memcpy(rtmp_packet.pes_buf + 14, data->head_data, data->head_data_len);
    }
	if(data->data != NULL){
        memcpy(rtmp_packet.pes_buf + 14 + data->head_data_len, data->data, data->data_len);
    }
    rtmp_packet.pes_len = es_len + 14;
    /* 打上ts的头包,在输出到文件 */
    return rtmp_tots_packet_ts_header(pts); 
}

int rtmp_tots::rtmp_tots_packet_ts_header(uint64 pts){
	unsigned char * packet = NULL;      /* 一个188字节的ts包 */
    int new_pes = 1;                    /* pes的第一个包 */
    unsigned int pid = 0;                /* PID：video_pid,audio_pid */
    unsigned int *count = NULL;         /* conutity_counter */
    int i_payload = 0;                  /* ts包的负载情况,=0,表示184字节的纯净数据;=1,表示该ts包带有pcr */
    int i_size = 0;                     /* 剩余的pes包的大小 */
    int i_copy = 0;                     /* */
    int b_adaptation_field = 0;         /*表明ts包是否有调整字段,有两种情况：1、ts包带有pcr;2、最后不足184字节 */
    unsigned char * p = NULL;           /* 读取数据的指针 */
    pack_data_t *pack_data = rtmp_packet.pack_data;/* 存放es原始数据 */
    uint64 now_psi_time = get_millisecond64();/* 当前psi时间 */

    if(NULL == rtmp_packet.pes_buf || NULL == pack_data || rtmp_packet.ts_fp == INVALID_HANDLE_VALUE){
        return -1;
    }

    p = rtmp_packet.pes_buf;
    i_size = rtmp_packet.pes_len;

    /* 上一帧还有残留的不足7*188字节的数据 */
    packet = rtmp_packet.ts_cache + rtmp_packet.ts_cache_len;

    /* 获取ts包的pcr, 只给视频帧打上pcr,不用给音频帧打 */
    if(pack_data->type == data_type_h264){
        pid = VIDEO_PID;
        count = &(rtmp_packet.video_count);
    }else if(pack_data->type == data_type_aac){
        pid = AUDIO_PID;
        count = &(rtmp_packet.audio_count);
    }
    
    //判断是否写入pat、pmt包,
	//每4帧数据就加入一次pat、pmt包
	// 现在是关键帧前面插入pat、pmt包, 是为了防止数据缺失能够从任何地方开始播放 
    if((pack_data->is_keyframe) || (rtmp_packet.frame_num == 0) || (now_psi_time - 300 > rtmp_packet.last_psi_time))
    {
        rtmp_packet.last_psi_time = now_psi_time;
        rtmp_tots_packet_ts_pat_table();   /* 获取pat表 */
        memcpy(packet, rtmp_packet.pat_packet, TS_PACKET_SIZE);
        packet += TS_PACKET_SIZE;
        rtmp_packet.ts_cache_len += TS_PACKET_SIZE;
        /* 当满足7个包的时候调用回调 */
        if(packet - rtmp_packet.ts_cache == 7*TS_PACKET_SIZE)
        {
            assert(rtmp_packet.ts_cache_len == 7*TS_PACKET_SIZE);
			file_write(rtmp_packet.ts_fp,rtmp_packet.ts_cache, 7*TS_PACKET_SIZE);
            packet = rtmp_packet.ts_cache;
            rtmp_packet.ts_cache_len = 0;
        }
        rtmp_tots_packet_ts_pmt_table();    /* 获取pmt表 */
        memcpy(packet, rtmp_packet.pmt_packet, TS_PACKET_SIZE);
        packet += TS_PACKET_SIZE;
        rtmp_packet.ts_cache_len += TS_PACKET_SIZE;
        /* 当满足7个包的时候调用回调 */
        if(packet - rtmp_packet.ts_cache == 7*TS_PACKET_SIZE)
        {
            assert(rtmp_packet.ts_cache_len == 7*TS_PACKET_SIZE);
            file_write(rtmp_packet.ts_fp,rtmp_packet.ts_cache, 7*TS_PACKET_SIZE);
            packet = rtmp_packet.ts_cache;
            rtmp_packet.ts_cache_len = 0;
        }
    }
    rtmp_packet.frame_num++;

    while(i_size > 0)
    {
        i_payload = 184;                /* 只有视频的第一个ts包有pcr */
        i_copy = MIN(i_size, i_payload);/* 一次需要拷贝的数据大小 */
        b_adaptation_field = (new_pes||i_size<i_payload) ? 1 : 0; /* 是否有调整字段,1:视频pes开始第一个包;2 剩余字节不足188 */

        packet[0] = 0x47;                                             /* ts的起始位 */
        packet[1] = (new_pes ? 0x40 : 0x00) | ((pid&0x1f00)>>8);        /* PID | payload_unit_start_indicator = 1,表示ts_header后会有一个调整字节,改调整字段是后面填充字段的长度 */
        packet[2] = pid&0xff;                                         /* PID */
        packet[3] = (b_adaptation_field ? 0x30 : 0x10) | (*count&0x0f);/* transport_scarmbling_control = 00(未加扰) | adaptation_filed_control = 10(后面184有效净荷);=11,后面有0~182的调整字段 | continutity_counter */
        *count = (*count + 1) % 0x10;                 /* 计算count */

        if(b_adaptation_field)
        {
            /* 只给视频的的第一个ts包打上pcr,音频不用打,也可以单独用一个TS包来存放pcr,放在pat、pmt表后,只要在pmt表中指出 */
            if(new_pes && (pid == VIDEO_PID))
            {
                int i_stuffing = 0;    /* 调整字段 */
                uint64 pcr = pts * 300;
                uint64 pcr_base = pcr / 300;
                uint64 pcr_ext = pcr % 300;

                /* 视频帧插入pcr,需要重新计算自适应长度 */
                i_payload = MIN(176, i_payload);
                i_copy = MIN(i_size, i_payload);
                i_stuffing = i_payload - i_copy;
                
				packet[4] = (unsigned char)(7 + i_stuffing);     /* 后面调整字段的大小 */
                packet[5] = 0x10;               /* PCR_flag = 1,其他字段全为0 */
                packet[6] = (pcr_base >> 25)&0xff;   /* program_clock_reference_base(33位) | reserved(6位) | program_clock_reference_extension(9位) */
                packet[7] = (pcr_base >> 17)&0xff;
                packet[8] = (pcr_base >> 9)&0xff;
                packet[9] = (pcr_base >> 1)&0xff;
                packet[10]= ((pcr_base << 7)&0x80) | 0x7E | ((pcr_ext>>8) & 0x01);
                packet[11] = pcr_ext & 0xFF;

                memset(packet+12, 0xff, i_stuffing);   /* 加入填充字节 */
            }
            else
            {
                int i_stuffing = i_payload - i_copy;
                if(i_stuffing > 1)
                {
                    packet[4] = (unsigned char)(i_stuffing - 1);     /* 这里1<= i_stuffing <= 183 */
                    packet[5] = 0x00;
                    memset(packet + 6, 0xff, i_stuffing - 2);
                }
                else 
                {
                    packet[4] = 0x01;            
                    packet[5] = 0x00;
                    i_copy = 182;    
                }
            }
        }

        memcpy(packet + (TS_PACKET_SIZE - i_copy), p, i_copy);
        p += i_copy;
        i_size -= i_copy;
        packet += TS_PACKET_SIZE;
        new_pes = 0;
        rtmp_packet.ts_cache_len += TS_PACKET_SIZE;

        /* 当满足7个包的时候调用回调 */
        if(packet - rtmp_packet.ts_cache == 7*TS_PACKET_SIZE)
        {
            assert(rtmp_packet.ts_cache_len == 7*TS_PACKET_SIZE);
            file_write(rtmp_packet.ts_fp,rtmp_packet.ts_cache, 7*TS_PACKET_SIZE);
            packet = rtmp_packet.ts_cache;
            rtmp_packet.ts_cache_len = 0;
        }
    }

    return 0;

}