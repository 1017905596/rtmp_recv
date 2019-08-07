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
//���ڸ�һ��Ƶ֡��pes���������pts��dts
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
	unsigned char pat_header[5] = {0x47, 0x40, 0x00, 0x10, 0x00}; /* �򵥳�ʼ��pat��,pat���PIDΪ0x0000,���һ���ֽ��ǵ����ֽ� */
    unsigned char pat_table[16] = {0};                            /* ��СΪpat_table[2] + 3 */

    pat_table[0] = 0x00;    /* table  id */
    pat_table[1] = 0xb0;    /* section_syntax_indacator = 1,����PAT,����Ϊ1 */
    pat_table[1] = pat_table[1];   /* section_length,��12bits */
    pat_table[2] = 0x0d;    /* section_length,��ʾ���ֶκ��pat_table���� */
    pat_table[3] = 0x00;    /* transport_stream_id */
    pat_table[4] = 0x01;    /* transport_stream_id */
    pat_table[5] = 0xc1;    /* version_number = 00000, current_next_indicator = 1 */
    pat_table[6] = 0x00;    /* section_number */
    pat_table[7] = 0x00;    /* last_section_number */
    /* ������һ��4���ֽڵ�ѭ��,��ʾNIT,PMT����PID */
    pat_table[8] = 0x00;    /* program_number */
    pat_table[9] = 0x01;    /* pargram_number */
    pat_table[10] = 0xe1;   /* PMT��PID */
    pat_table[11] = 0x00;   /* PMT��PID��0x100 */

    /* crc32ѭ��У���� */
    {
        unsigned int crc32 = rtmp_tots_packet_ts_crc32(pat_table, 12);

        pat_table[12] = crc32 >> 24;
        pat_table[13] = (crc32 >> 16) & 0xff;
        pat_table[14] = (crc32 >> 8) & 0xff;
        pat_table[15] = crc32 & 0xff;
    }

    /* ����pat���header */
    pat_header[3] = pat_header[3] | (rtmp_packet.pat_num % 0x10);    /* ����pat�� */

    /* ����pat�� */
    rtmp_packet.pat_packet[0] = '\0';
    memcpy(rtmp_packet.pat_packet, pat_header, 5);
    memcpy(rtmp_packet.pat_packet + 5, pat_table, 16);
    memset(rtmp_packet.pat_packet + 21, 0xff, TS_PACKET_SIZE - 21);

    return 0;
}

int rtmp_tots::rtmp_tots_packet_ts_pmt_table(){
	unsigned char pmt_header[5] = {0x47, 0x41, 0x00, 0x10, 0x00}; /* �򵥳�ʼ��pmt��,pmt���PIDΪ0x0100,���һ���ֽ��ǵ����ֽ� */
    unsigned char pmt_table[26] = {0};

    pmt_table[0] = 0x02;    /* table_id : 0x02 */
    pmt_table[1] = 0xb0;    /* section_snytax_indicator:1; zero:1; */
    pmt_table[1] = pmt_table[1];    /* ����λ��section_length */
    pmt_table[2] = 0x17;    /* section_length��8λ */
    pmt_table[3] = 0x00;    /* program number */
    pmt_table[4] = 0x01;   
    pmt_table[5] = 0xc1;    /* reserved:2;version_number:5;current_next_indicator:1; ���ֵͬpat��һ�� */
    pmt_table[6] = 0x00;    /* section_number */
    pmt_table[7] = 0x00;    /* last_section_number */
    pmt_table[8] = VIDEO_PID >> 8 | 0xe0;    /* reserved:3;PCR_PID:5,pcr���ڵ�pid����Ƶ����,Ҳ���Ե���������һ��ta��,ר������pcr */
    pmt_table[9] = 0x00;    /* PCR_PID:8 */
    pmt_table[10] = 0xf0;    /* reserved:4;program_info_legnth:4 */
    pmt_table[11] = 0x00;    /* program_info_length:8 */
    /* ��Ƶpid */
    pmt_table[12] = 0x1b;    /* stream_type:8;h264������Ƶ  */
    pmt_table[13] = (VIDEO_PID >> 8) | 0xe0 ;    /* reserved:3;elementary_pid:5 */
    pmt_table[14] = 0x00;
    pmt_table[15] = 0xf0;    /* reserved:4;es_info_length:4 */
    pmt_table[16] = 0x00;    /* es_info_length:4 */
    /* ��Ƶpid */
    pmt_table[17] = 0x0f;    /* stream_type:8;aac������Ƶ  */
    pmt_table[18] = (AUDIO_PID >> 8) | 0xe0 ;    /* reserved:3;elementary_pid:5 */
    pmt_table[19] = AUDIO_PID & 0xff;    /* elementary_pid:8 */
    pmt_table[20] = 0xf0;    /* reserved:4;es_info_length:4 */
    pmt_table[21] = 0x00;    /* es_info_length:4 */

    /* crc32ѭ��У���� */
    {
        unsigned int crc32 = rtmp_tots_packet_ts_crc32(pmt_table, 22);

        pmt_table[22] = crc32 >> 24;
        pmt_table[23] = (crc32 >> 16) & 0xff;
        pmt_table[24] = (crc32 >> 8) & 0xff;
        pmt_table[25] = crc32 & 0xff;
    }

    /* ����pmt���header,�����Ӽ���,pmt_numͬpat_numeһ�� */
    pmt_header[3] = pmt_header[3] | (rtmp_packet.pat_num) % 0x10;
    rtmp_packet.pat_num++;    

    /* ����pmt�� */
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
		///* �������Ƶ��ptsʱ�����ܴ�ʱ��Ƶ������Ƶ��pts,��Ȼ��û���� */
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
//��Ƶ
int rtmp_tots::rtmp_tots_packet_pes_video_header(uint64 dts,uint64 pts){
	unsigned char * p = NULL;
    unsigned char header[9] = {0};
    char time_buf[10] = {0};
    unsigned char unit_start[6] = {0x00, 0x00, 0x00, 0x01, 0x09, 0x10}; /* ����֮֡��ķָ��� */
	pack_data_t *data = rtmp_packet.pack_data;
    unsigned int  es_len = data->head_data_len + data->data_len;

    /* ��0~5λ�ֽ� */
    header[0] = 0x00;
    header[1] = 0x00;
    header[2] = 0x01;
    header[3] = 0xe0;    /* ��Ƶ */
    header[4] = 0x00; /* ���ó�0x00,������ʹһ��pes�ĳ��ȴ���65535,Ҳ�ܹ���װ */ 
    header[5] = 0x00;
    header[6] = 0x80;/* ��6λ�ֽ�,fix_bit = 0x10 */
    header[7] = 0xc0;/* ��7λ�ֽ�,pts_dts_flag = 0x11 */
    header[8] = 0x0a;/* ��8λ�ֽ�,PES_header_data_length = 0x0a */

    rtmp_tots_packet_pes_set_time((unsigned char *)(&time_buf), pts, dts);

    memcpy(rtmp_packet.pes_buf, header, 9);
    memcpy(rtmp_packet.pes_buf + 9, time_buf, 10); 

    if(es_len > ( PACKET_PES_BUF - 25)){
        return -1;
    }

    /* ���head_data��ΪNULL��ָ��head_data,����ָ��data */
    p = (data->head_data != NULL && data->head_data_len > 0) ? data->head_data : data->data;

    /* �޷ָ�����Ҫ��ӷָ���,��Ȼ��Щ��������ʧ�� */
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

    /* ����ts��ͷ��,��������ļ� */
    return rtmp_tots_packet_ts_header(pts); 
}

int rtmp_tots::rtmp_tots_packet_pes_audio_header(uint64 pts){
	unsigned char header[14] = {0};
    pack_data_t *data = rtmp_packet.pack_data;
    unsigned int es_len = data->head_data_len + data->data_len;

    /* ��0~5λ�ֽ�,0~2��pes����ʼ�� */
    header[0] = 0x00;
    header[1] = 0x00;
    header[2] = 0x01;
    header[3] = 0xc0;    /* ��Ƶpes_id */
    header[4] = (unsigned char)((es_len + 8) >> 8);//����PES_packet_length[2],2���ֽ�
    header[5] = (es_len + 8) & 0xff;  /* PES_packet_length��8 */
    header[6] = 0x80;/* ��6λ�ֽ�,fix_bit = 0x10 */
    header[7] = 0x80;/* ��7λ�ֽ�,pts_dts_flag = 0x10,��ƵֻҪpts,��Ҫdts */
    header[8] = 0x05;/* ��8λ�ֽ�,PES_header_data_length = 0x05 */
    header[9] = ((pts >> 29) | 0x21) & 0x2f;    /* '0010xxx1' */
    header[10] = (pts >> 22) & 0xff;
    header[11] = ((pts >> 14) | 0x01) & 0xff;
    header[12] = (pts >> 7) & 0xff;
    header[13] = ((pts << 1) | 0x01) & 0xff;

    memcpy(rtmp_packet.pes_buf, header, 14);

    /* ��pesͷ��esԭʼ���������һ��,����pes������ */
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
    /* ����ts��ͷ��,��������ļ� */
    return rtmp_tots_packet_ts_header(pts); 
}

int rtmp_tots::rtmp_tots_packet_ts_header(uint64 pts){
	unsigned char * packet = NULL;      /* һ��188�ֽڵ�ts�� */
    int new_pes = 1;                    /* pes�ĵ�һ���� */
    unsigned int pid = 0;                /* PID��video_pid,audio_pid */
    unsigned int *count = NULL;         /* conutity_counter */
    int i_payload = 0;                  /* ts���ĸ������,=0,��ʾ184�ֽڵĴ�������;=1,��ʾ��ts������pcr */
    int i_size = 0;                     /* ʣ���pes���Ĵ�С */
    int i_copy = 0;                     /* */
    int b_adaptation_field = 0;         /*����ts���Ƿ��е����ֶ�,�����������1��ts������pcr;2�������184�ֽ� */
    unsigned char * p = NULL;           /* ��ȡ���ݵ�ָ�� */
    pack_data_t *pack_data = rtmp_packet.pack_data;/* ���esԭʼ���� */
    uint64 now_psi_time = get_millisecond64();/* ��ǰpsiʱ�� */

    if(NULL == rtmp_packet.pes_buf || NULL == pack_data || rtmp_packet.ts_fp == INVALID_HANDLE_VALUE){
        return -1;
    }

    p = rtmp_packet.pes_buf;
    i_size = rtmp_packet.pes_len;

    /* ��һ֡���в����Ĳ���7*188�ֽڵ����� */
    packet = rtmp_packet.ts_cache + rtmp_packet.ts_cache_len;

    /* ��ȡts����pcr, ֻ����Ƶ֡����pcr,���ø���Ƶ֡�� */
    if(pack_data->type == data_type_h264){
        pid = VIDEO_PID;
        count = &(rtmp_packet.video_count);
    }else if(pack_data->type == data_type_aac){
        pid = AUDIO_PID;
        count = &(rtmp_packet.audio_count);
    }
    
    //�ж��Ƿ�д��pat��pmt��,
	//ÿ4֡���ݾͼ���һ��pat��pmt��
	// �����ǹؼ�֡ǰ�����pat��pmt��, ��Ϊ�˷�ֹ����ȱʧ�ܹ����κεط���ʼ���� 
    if((pack_data->is_keyframe) || (rtmp_packet.frame_num == 0) || (now_psi_time - 300 > rtmp_packet.last_psi_time))
    {
        rtmp_packet.last_psi_time = now_psi_time;
        rtmp_tots_packet_ts_pat_table();   /* ��ȡpat�� */
        memcpy(packet, rtmp_packet.pat_packet, TS_PACKET_SIZE);
        packet += TS_PACKET_SIZE;
        rtmp_packet.ts_cache_len += TS_PACKET_SIZE;
        /* ������7������ʱ����ûص� */
        if(packet - rtmp_packet.ts_cache == 7*TS_PACKET_SIZE)
        {
            assert(rtmp_packet.ts_cache_len == 7*TS_PACKET_SIZE);
			file_write(rtmp_packet.ts_fp,rtmp_packet.ts_cache, 7*TS_PACKET_SIZE);
            packet = rtmp_packet.ts_cache;
            rtmp_packet.ts_cache_len = 0;
        }
        rtmp_tots_packet_ts_pmt_table();    /* ��ȡpmt�� */
        memcpy(packet, rtmp_packet.pmt_packet, TS_PACKET_SIZE);
        packet += TS_PACKET_SIZE;
        rtmp_packet.ts_cache_len += TS_PACKET_SIZE;
        /* ������7������ʱ����ûص� */
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
        i_payload = 184;                /* ֻ����Ƶ�ĵ�һ��ts����pcr */
        i_copy = MIN(i_size, i_payload);/* һ����Ҫ���������ݴ�С */
        b_adaptation_field = (new_pes||i_size<i_payload) ? 1 : 0; /* �Ƿ��е����ֶ�,1:��Ƶpes��ʼ��һ����;2 ʣ���ֽڲ���188 */

        packet[0] = 0x47;                                             /* ts����ʼλ */
        packet[1] = (new_pes ? 0x40 : 0x00) | ((pid&0x1f00)>>8);        /* PID | payload_unit_start_indicator = 1,��ʾts_header�����һ�������ֽ�,�ĵ����ֶ��Ǻ�������ֶεĳ��� */
        packet[2] = pid&0xff;                                         /* PID */
        packet[3] = (b_adaptation_field ? 0x30 : 0x10) | (*count&0x0f);/* transport_scarmbling_control = 00(δ����) | adaptation_filed_control = 10(����184��Ч����);=11,������0~182�ĵ����ֶ� | continutity_counter */
        *count = (*count + 1) % 0x10;                 /* ����count */

        if(b_adaptation_field)
        {
            /* ֻ����Ƶ�ĵĵ�һ��ts������pcr,��Ƶ���ô�,Ҳ���Ե�����һ��TS�������pcr,����pat��pmt���,ֻҪ��pmt����ָ�� */
            if(new_pes && (pid == VIDEO_PID))
            {
                int i_stuffing = 0;    /* �����ֶ� */
                uint64 pcr = pts * 300;
                uint64 pcr_base = pcr / 300;
                uint64 pcr_ext = pcr % 300;

                /* ��Ƶ֡����pcr,��Ҫ���¼�������Ӧ���� */
                i_payload = MIN(176, i_payload);
                i_copy = MIN(i_size, i_payload);
                i_stuffing = i_payload - i_copy;
                
				packet[4] = (unsigned char)(7 + i_stuffing);     /* ��������ֶεĴ�С */
                packet[5] = 0x10;               /* PCR_flag = 1,�����ֶ�ȫΪ0 */
                packet[6] = (pcr_base >> 25)&0xff;   /* program_clock_reference_base(33λ) | reserved(6λ) | program_clock_reference_extension(9λ) */
                packet[7] = (pcr_base >> 17)&0xff;
                packet[8] = (pcr_base >> 9)&0xff;
                packet[9] = (pcr_base >> 1)&0xff;
                packet[10]= ((pcr_base << 7)&0x80) | 0x7E | ((pcr_ext>>8) & 0x01);
                packet[11] = pcr_ext & 0xFF;

                memset(packet+12, 0xff, i_stuffing);   /* ��������ֽ� */
            }
            else
            {
                int i_stuffing = i_payload - i_copy;
                if(i_stuffing > 1)
                {
                    packet[4] = (unsigned char)(i_stuffing - 1);     /* ����1<= i_stuffing <= 183 */
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

        /* ������7������ʱ����ûص� */
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