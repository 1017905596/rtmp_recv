#ifndef __RTMP_AMF0_H_
#define __RTMP_AMF0_H_

#include <stdio.h>
#include <string.h>

#define SWAP_2BYTE(L) ((((L) & 0x00FF) << 8) | (((L) & 0xFF00) >> 8))
#define SWAP_4BYTE(L) ((SWAP_2BYTE ((L) & 0xFFFF) << 16) | SWAP_2BYTE(((L) >> 16) & 0xFFFF))
#define SWAP_8BYTE(L) ((SWAP_4BYTE (((unsigned long long int)(L)) & 0xFFFFFFFF) << 32) | SWAP_4BYTE((((unsigned long long int)(L)) >> 32) & 0xFFFFFFFF))

class rtmp_amf0{
public:
	rtmp_amf0();
	virtual ~rtmp_amf0();
public:
	//数据切chunk
	int rtmp_amf0_splite_pack(char ct, const int header_len, char *pag_buf, const int pag_len);
	//构造数据
	int rtmp_amf0_push_string(char *dst, int size, const char* str_value);
	int rtmp_amf0_push_number(char *dst, int size, const double value);
	int rtmp_amf0_push_object_header(char *dst, int size);
	int rtmp_amf0_push_object_ender(char *dst, int size);
	int rtmp_amf0_push_object_prop_name(char *dst, int size, const char* name);
	int rtmp_amf0_push_bool(char *dst, int size,const int vlaue);
	int rtmp_amf0_push_null(char *dst, int size);
	//解析数据
	int rtmp_amf0_pop_string(char *dst, int dst_size, unsigned char *src, int src_len );
	int rtmp_amf0_pop_number(double *dst, unsigned char *src, int src_len);
	int rtmp_amf0_pop_null( unsigned char *src, int src_len );
	int rtmp_amf0_pop_object_begin( unsigned char *src, int src_len );
	int rtmp_amf0_pop_object_end( unsigned char *src, int src_len );
	int rtmp_amf0_pop_object_prop_name( char *dst, int dst_size, unsigned char *src, int src_len );
};

#endif
