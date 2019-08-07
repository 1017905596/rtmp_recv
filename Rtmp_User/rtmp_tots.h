#ifndef _RTMP_TOTS_H_
#define _RTMP_TOTS_H_

#include <stdio.h>
#include "rtmp_type.h"

/* ����crc32ѭ������У�� */
static unsigned int crc32table[256] = {   
        0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,   
        0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,   
        0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,   
        0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,   
        0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,   
        0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,   
        0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,   
        0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,   
        0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,   
        0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,   
        0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,   
        0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,   
        0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,   
        0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,   
        0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,   
        0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,   
        0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,   
        0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,   
        0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,   
        0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,   
        0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,   
        0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,   
        0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,   
        0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,   
        0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,   
        0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,   
        0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,   
        0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,   
        0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,   
        0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,   
        0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,   
        0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,   
        0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,   
        0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,   
        0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,   
        0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,   
        0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,   
        0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,   
        0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,   
        0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,   
        0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,   
        0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,   
        0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};

/* pts��dts�Ľṹ�� */
typedef struct rtmp_tots_pts_dts_s
{
    unsigned char marker_third:1;       /* ��־λ4 */
    unsigned char pts_dts_6_0:7;        /* pts�ֶ�1 */
    unsigned char pts_dts_14_7;         /* pts�ֶ�1 */

    unsigned char marker_second:1;      /* ��־λ2 */
    unsigned char pts_dts_21_15:7;      /* pts�ֶ�2 */
    unsigned char pts_dts_29_22;        /* pts�ֶ�2 */

    unsigned char marker_first:1;       /* ��־λ1 */
    unsigned char pts_dts_32_30:3;      /* pts�ֶ�1 */
    unsigned char first_flag:4;         /* 4λ�Ŀ�ʼ��־λ */

}rtmp_tots_pts_dts_t;

//H264һ֡���ݵĽṹ��
typedef struct rtmp_tots_h264_s
{
    unsigned char forbidden_bit;           /* Should always be FALSE */
    unsigned char nal_reference_idc;        /* NALU_PRIORITY_xxxx */
    unsigned char nal_unit_type;           /* NALU_TYPE_xxxx */
    unsigned int  startcodeprefix_len;      /* ǰ׺�ֽ��� */ 
    unsigned int  len;                    /* ����nal ͷ��nal ���ȣ��ӵ�һ��00000001����һ��000000001�ĳ��� */
    unsigned int  max_size;                /* ����һ��nal �ĳ��� */
    unsigned char * buf;                  /* ����nal ͷ��nal ���� */
    unsigned char Frametype;               /* ֡���� */
    unsigned int  lost_packets;            /* Ԥ�� */
} rtmp_tots_h264_t;


//ADTS ͷ��������õ���Ϣ �����ʡ���������֡����
typedef struct rtmp_tots_aac_s
{
    unsigned int syncword;                /* 12 bslbf ͬ����The bit string ��1111 1111 1111����˵��һ��ADTS֡�Ŀ�ʼ */
    unsigned int id;                    /* 1 bslbf   MPEG ��ʾ��, ����Ϊ1 */
    unsigned int layer;                /* 2 uimsbf Indicates which layer is used. Set to ��00�� */
    unsigned int protection_absent;        /* 1 bslbf  ��ʾ�Ƿ�����У�� */
    unsigned int profile;             /* 2 uimsbf  ��ʾʹ���ĸ������AAC����01 Low Complexity(LC)--- AACLC */
    unsigned int sf_index;            /* 4 uimsbf  ��ʾʹ�õĲ������±� */
    unsigned int private_bit;         /* 1 bslbf */
    unsigned int channel_configuration;/* 3 uimsbf  ��ʾ������ */
    unsigned int original;               /* 1 bslbf */
    unsigned int home;               /*  1 bslbf */
    /*�����Ϊ�ı�Ĳ�����ÿһ֡����ͬ*/
    unsigned int copyright_identification_bit;   /* 1 bslbf */
    unsigned int copyright_identification_start; /* 1 bslbf */
    unsigned int aac_frame_length;             /* 13 bslbf  һ��ADTS֡�ĳ��Ȱ���ADTSͷ��raw data block */
    unsigned int adts_buffer_fullness;          /* 11 bslbf     0x7FF ˵�������ʿɱ������ */
    /*no_raw_data_blocks_in_frame ��ʾADTS֡����number_of_raw_data_blocks_in_frame + 1��AACԭʼ֡.
    ����˵number_of_raw_data_blocks_in_frame == 0 
    ��ʾ˵ADTS֡����һ��AAC���ݿ鲢����˵û�С�(һ��AACԭʼ֡����һ��ʱ����1024���������������)
    */
    unsigned int no_raw_data_blocks_in_frame;   /* 2 uimsfb */
} rtmp_tots_aac_t;

/* pesͷ�ṹ�壬����ṹ˵���ο�,ISOICE 13818-1.pdf page:56 */
typedef struct rtmp_tots_pes_s
{
    unsigned char    packet_start_code_prefix[3];        /* pes��ʼ��־λ,Ϊ0x000001 */
    unsigned char    stream_id;                          /* pes��id,һ��Ϊ��0xbd;0xbe;0xbf;0xc0-0xdf;0xe0-0xef */
    unsigned char    PES_packet_length[2];               /* pes�ĳ���,����pesͷ���и��ֶ�֮��ĳ���,pes�Ĵ�СΪpes_packet_length+6 */

    /* ��7,8,9�ֽ���pes����չͷ���ֶ�,�����������Ļ�����Ϣ */
    unsigned char    original_or_copy:1;                 /* 0x01��ʾoriginal packet;0x00��ʾcopy packet */      
    unsigned char    copyright:1;                        /* 0x01��ʾpes is protected by copyright */
    unsigned char    data_alignment_indicator:1;         /* �ֽڶ���ָʾ�� */
    unsigned char    PES_priority:1;                     /* ����ָʾ��pes�����ȼ� */
    unsigned char    PES_scrambling_control:2;           
    unsigned char    fix_bit:2;                          /* 0x10 */

    unsigned char    PES_extension_flag:1;
    unsigned char    PES_CRC_flag:1;                     
    unsigned char    additional_copy_info_flag:1;
    unsigned char    DSM_trick_mode_flag:1;
    unsigned char    ES_rate_flag:1;                     /* ���Ϊ1,����ͷ������3�ֽڵ�es rate */
    unsigned char    ESCR_flag:1;                        /* ���Ϊ1,����ͷ������4�ֽڵ�escr */
    unsigned char    PTS_DTS_flags:2;                    /* pts��dts��ָʾλ,00��ʾ��pts��dts,01��ֹʹ��,10��ʾpesͷ���ֶλḽ��pts�ṹ,11��ʾ�ḽ��pts��dts�Ľṹ */
                                                        /* pts��dtsʹ�õ���90hzʱ��ʱ��,��1pts��ʾ1/90000�� */
    unsigned char    PES_header_data_length;             /* pesͷ���ֶθ��ֶ�֮��ĳ��� */

}rtmp_tots_pes_t;

#define TS_PACKET_SIZE 188
#define VIDEO_PID    0x1100        /* ts������Ƶ��PID */
#define AUDIO_PID    0x1101        /* ts������Ƶ��PID */
#define PACKET_PES_BUF (1024*1024*4+1024)

typedef enum data_type_s
{
    data_type_mp3=1,
    data_type_aac,
    data_type_h264,
}data_type_t;

typedef enum es_unit_start_type_s
{
    es_unit_start_type_init = 0,
    es_unit_start_type_startcode3, /* û�зָ���,ȫ����Ƶ���� */
    es_unit_start_type_startcode4,
    es_unit_start_type_aac
}es_unit_start_type_t;

typedef struct pack_data_s
{
	data_type_t type;
    uint64 decode_ms;
    uint64 picture_ms;
    unsigned char *head_data;
    unsigned int head_data_len;
    unsigned char *data;//����
    unsigned int data_len;//���ݳ���
    int is_keyframe;//1:�ؼ�֡
}pack_data_t;


typedef struct rtmp_packet_s{
	unsigned char pat_packet[TS_PACKET_SIZE];/* pat�� */
    unsigned char pmt_packet[TS_PACKET_SIZE];/* pmt�� */
    unsigned int pat_num;        /* �����pat������� */
    unsigned int frame_num;      /* ��ȡ������֡������ */
    unsigned int video_count;     /* video_count,video��ts�� */
    unsigned int audio_count;    /* audio��ts�� */

	unsigned char ts_cache[7*TS_PACKET_SIZE]; /* ���ڽ���װ�õ����ݷ��ػ�ȥ */
    unsigned int ts_cache_len;    /* ts_cache�����ݵĴ�С */

    unsigned char * pes_buf;    /* ����װ�ط�װ��pes������ */
    unsigned int pes_len;        /* pes�����ݳ��� */
	pack_data_t* pack_data;/* ��rtmp��������Ƶ���� */
	es_unit_start_type_t unit_type;
	uint64 last_video_picture_ms;    /* ��Ƶ��һ��pts */
    uint64 last_psi_time;      /*ÿ300s��һ��psi��Ϣ */

	HANDLE         ts_fp;

}rtmp_packet_t;

class rtmp_tots{
public:
	rtmp_tots();
	virtual ~rtmp_tots();
public:
	int rtmp_tots_packet_from_cache(pack_data_t *pack);
protected:
	unsigned int rtmp_tots_packet_ts_crc32(unsigned char *data, int len);
	void rtmp_tots_packet_pes_set_time(unsigned char *buf, uint64 pts, uint64 dts);
	int rtmp_tots_packet_pes_video_header(uint64 dts,uint64 pts);
	int rtmp_tots_packet_pes_audio_header(uint64 pts);
	int rtmp_tots_packet_ts_header(uint64 pts);
	int rtmp_tots_packet_ts_pat_table();
	int rtmp_tots_packet_ts_pmt_table();
	int rtmp_tots_write_flie();
protected:
	rtmp_packet_t rtmp_packet;
};


#endif