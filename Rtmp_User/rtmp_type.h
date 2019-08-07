#ifndef _RTMP_TYPE_H_
#define _RTMP_TYPE_H_
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>


#define WIN//win下使用
//#define SAVE_FLV //保存为flv格式文件

typedef unsigned long long int uint64;
#define YY_P_BIT_ENABLED(WORD, BIT) (((WORD) & (BIT)) != 0)
#define MIN( a, b ) ((a)>=(b)?(b):(a))
#ifdef WIN
	#include <process.h>
	#include <Ws2tcpip.h> 
	typedef DWORD mode_t;
	#define FILE_PERMS_ALL (FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE)
	#pragma comment( lib, "ws2_32.lib" )
#else
	#include <unistd.h>
	typedef unsigned long DWORD,*LPDWORD;
	typedef int HANDLE;
    #define INVALID_HANDLE_VALUE -1
	#define FILE_PERMS_ALL  0777
	typedef void * LPSECURITY_ATTRIBUTES;
#endif

typedef struct timeval_s{
    long sec;   // 秒数
    long usec;  //1sec = 1000000usec
}timeval_t;

HANDLE file_open(const char *filename, int flag, mode_t perms, LPSECURITY_ATTRIBUTES sa );
int file_close( HANDLE fd );
DWORD file_read( HANDLE fd,  void *buf, DWORD len );
DWORD file_write( HANDLE fd,  const void *buf, DWORD len );
uint64 get_millisecond64();

#endif