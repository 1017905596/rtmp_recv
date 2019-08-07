#include "rtmp_lib.h"

int main(){
	int ret = 0;
	rtmp_client *rtmp_c = new rtmp_client(); 
	char url[512] = "rtmp://192.168.174.129/live/live3.mp4";
	char url1[512] = "rtmp://202.69.69.180:443/webcast/bshdlive-pc";
	char url2[512] = "rtmp://58.200.131.2:1935/livetv/hunantv";
	
#ifdef WIN
	WORD version = MAKEWORD( 2, 2 );    
    WSADATA wsd_data;        
    WSAStartup( version, &wsd_data );
#endif
	rtmp_c->rtmp_setupurl(url,strlen(url));
	rtmp_c->rtmp_connect();
	rtmp_c->rtmp_start_play();
	while(1){
#ifdef WIN
		Sleep(2000);
#else
		sleep(2);
#endif
	}
}