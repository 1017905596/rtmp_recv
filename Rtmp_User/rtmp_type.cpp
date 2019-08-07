#include "rtmp_type.h"

HANDLE file_open(const char *filename, int flag, mode_t perms, LPSECURITY_ATTRIBUTES sa )
{
    #ifdef WIN
      DWORD access = GENERIC_READ;
      DWORD creation = OPEN_ALWAYS;
      DWORD win_flag = 0;
      DWORD shared_mode = perms;

      if (YY_P_BIT_ENABLED(flag, O_WRONLY))
        access = GENERIC_WRITE;
      else if(YY_P_BIT_ENABLED(flag, O_RDWR))
        access = GENERIC_READ | GENERIC_WRITE;
      else
        access = GENERIC_READ;

      if ((flag & (_O_CREAT | _O_EXCL)) == (_O_CREAT | _O_EXCL))
        creation = CREATE_NEW;
      else if ((flag & (_O_CREAT | _O_TRUNC)) == (_O_CREAT | _O_TRUNC))
        creation = CREATE_ALWAYS;
      else if (YY_P_BIT_ENABLED(flag, _O_CREAT))
        creation = OPEN_ALWAYS;
      else if (YY_P_BIT_ENABLED(flag, _O_TRUNC))
        creation = TRUNCATE_EXISTING;
      else
        creation = OPEN_EXISTING;

  

      if( YY_P_BIT_ENABLED(flag, _O_TEMPORARY) )
        win_flag |= FILE_FLAG_DELETE_ON_CLOSE | FILE_ATTRIBUTE_TEMPORARY;

      if (YY_P_BIT_ENABLED(flag, FILE_FLAG_WRITE_THROUGH))
        win_flag |= FILE_FLAG_WRITE_THROUGH;
      if (YY_P_BIT_ENABLED(flag, FILE_FLAG_OVERLAPPED))
        win_flag |= FILE_FLAG_OVERLAPPED;
      if (YY_P_BIT_ENABLED(flag, FILE_FLAG_NO_BUFFERING))
        win_flag |= FILE_FLAG_NO_BUFFERING;
      if (YY_P_BIT_ENABLED(flag, FILE_FLAG_RANDOM_ACCESS))
        win_flag |= FILE_FLAG_RANDOM_ACCESS;
      if (YY_P_BIT_ENABLED(flag, FILE_FLAG_SEQUENTIAL_SCAN))
        win_flag |= FILE_FLAG_SEQUENTIAL_SCAN;
      if (YY_P_BIT_ENABLED(flag, FILE_FLAG_DELETE_ON_CLOSE))
        win_flag |= FILE_FLAG_DELETE_ON_CLOSE;
      if (YY_P_BIT_ENABLED(flag, FILE_FLAG_BACKUP_SEMANTICS))
        win_flag |= FILE_FLAG_BACKUP_SEMANTICS;
      if (YY_P_BIT_ENABLED(flag, FILE_FLAG_POSIX_SEMANTICS))
        win_flag |= FILE_FLAG_POSIX_SEMANTICS;
  
      /** CreateFile�ĺ����Ǵ���File����ں˶��󣬶����Ǵ�����������ϵġ��ļ�����
      *   ��Win32 API����һϵ�в����ں˶���ĺ����������ں˶���ĺ����������ΪCreateXxxx�͡�
      */
      return CreateFileA (filename, access, shared_mode, NULL, creation, win_flag, 0 );
    #else
        return open( filename, flag, perms );
    #endif
}

int file_close( HANDLE fd )
{
    #ifdef WIN
        if( CloseHandle( fd ) )
            return 0;
        else
            return -1;
    #else
        return close( fd );
    #endif
}

DWORD file_read( HANDLE fd,  void *buf, DWORD len )
{
    #ifdef WIN    
        DWORD ok_len = 0;
        assert( len < LONG_MAX );
        if( ReadFile( fd, buf, (DWORD)len, &ok_len, NULL ) ) /* ���һ������һ��ΪNULL */
            return (DWORD)ok_len;
        else
            return -1;
    #else
        return read( fd, buf, len );
    #endif
}

DWORD file_write( HANDLE fd,  const void *buf, DWORD len )
{
    #ifdef WIN    
        DWORD ok_len = 0;
        assert( len < LONG_MAX );
        if( WriteFile( fd, buf, (DWORD)len, &ok_len, NULL ) )
            return (DWORD)ok_len;
        else
            return -1;
    #else
        return write( fd, buf, len );
    #endif
}

int gettimeofday( timeval_t * t )
{
    if( t != NULL )
    {
#ifdef WIN
        FILETIME   file_time;       /* FILETIME�ṹ���е�64λ�޷��ŵ��ļ������ں�ʱ��ֵ */
        ULARGE_INTEGER _100ns;       /* һ��64λ�޷������� */
    
        GetSystemTimeAsFileTime (&file_time);  /* �õ���ǰ��ϵͳʱ�䣬100nsΪ��λ */

        _100ns.LowPart = file_time.dwLowDateTime;
        _100ns.HighPart = file_time.dwHighDateTime;
        _100ns.QuadPart -= 0x19db1ded53e8000;
 
        t->sec = (long) (_100ns.QuadPart / (10000 * 1000));             // Convert 100ns units to seconds; 
        t->usec = (long) ((_100ns.QuadPart % (10000 * 1000)) / 10);    // Convert remainder to microseconds;
    
        return 0;
#else
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            t->sec = ts.tv_sec;
            t->usec = ts.tv_nsec/1000;
        return 0;
#endif
    }
        return -1;
}
uint64 get_millisecond64()
{
    uint64 ms = 0;
    timeval_t tv;
    gettimeofday( &tv );
    ms = tv.sec;
    ms *= 1000;
    ms += tv.usec/1000;
    return ms;
}