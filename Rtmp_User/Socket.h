#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "rtmp_type.h"

#ifndef WIN
#include <netdb.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

class Socket{
public:
	Socket();
	virtual ~Socket();

public:
	int Create(const char *host,int sock_type);
	int Bind(unsigned int client_port);
	int Connect(unsigned int server_port, int timeout);
	int Receive(unsigned char* pBuffer, int nLen);
	int Send(const void * pBuffer, int nLen);
	void Colse();
protected:
	void SetSocketTimeOut(int nTimeOut);
protected:
	int m_nSocket;
	int m_IsV6;
	struct sockaddr_in	m_addrV4;
	struct sockaddr_in6 m_addrV6;
};

#endif