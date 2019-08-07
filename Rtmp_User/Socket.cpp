#include "Socket.h"
Socket::Socket(){
	m_nSocket = -1;
	m_IsV6 = 0;
	printf("Socket Create !!!\n");
}

Socket::~Socket(){

}

void Socket::SetSocketTimeOut(int nTimeOut)
{
#ifdef WIN
	setsockopt(m_nSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&nTimeOut, sizeof(nTimeOut));
	setsockopt(m_nSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&nTimeOut, sizeof(nTimeOut));
#else
	struct timeval tv;
	
	tv.tv_usec = (nTimeOut % 1000) * 1000;
	tv.tv_sec = nTimeOut / 1000;
	setsockopt(m_nSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&tv, sizeof(tv));
	setsockopt(m_nSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(tv));
#endif
}

int Socket::Create(const char *host,int sock_type){
	struct addrinfo hint;
	struct addrinfo *addrRet = NULL;
	m_IsV6 = 0;

	memset(&hint,0,sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host, NULL, &hint, &addrRet) != 0){
		printf("Create:%s,%s\n", host, gai_strerror(errno));
		return m_nSocket;
	}
	if (addrRet->ai_family == AF_INET6){
		memcpy(&m_addrV6, (struct sockaddr_in6*)addrRet->ai_addr, sizeof(m_addrV6));
		m_nSocket = socket(AF_INET6, sock_type, 0);
		m_IsV6 = 1;
	}else{
		memcpy(&m_addrV4, (struct sockaddr_in*)addrRet->ai_addr, sizeof(m_addrV4));
		m_nSocket = socket(AF_INET, SOCK_STREAM, 0);
	}
	freeaddrinfo(addrRet);
	return m_nSocket;
}

int Socket::Connect(unsigned int server_port, int timeout){
	int ret = -1;
	printf("Connect:%s:%d \n",inet_ntoa(m_addrV4.sin_addr) , server_port);
	if(m_IsV6){
		m_addrV6.sin6_port = htons(server_port);
		ret = connect(m_nSocket, (struct sockaddr *)&m_addrV6, sizeof(m_addrV6));
	}else{
		m_addrV4.sin_port = htons(server_port);
		ret = connect(m_nSocket,(struct sockaddr *) &m_addrV4,sizeof(m_addrV4));
	}
	SetSocketTimeOut(timeout);
	return ret;
}

int Socket::Receive(unsigned char* pBuffer, int nLen){
	return recv(m_nSocket, (char *)pBuffer, nLen, 0);
}

int Socket::Send(const void * pBuffer, int nLen){
	return send(m_nSocket, (char *)pBuffer, nLen, 0);
}

int Socket::Bind(unsigned int client_port){
	if(m_IsV6){
		struct sockaddr_in6 RecvAddr;
		memset(&RecvAddr, 0, sizeof(sockaddr_in6));
		RecvAddr.sin6_family = AF_INET6;
		RecvAddr.sin6_port = htons(client_port);
		return bind(m_nSocket, (struct sockaddr *)&RecvAddr, sizeof(RecvAddr));
	}else{
		struct sockaddr_in Recvaddr;
		memset(&Recvaddr, 0, sizeof(sockaddr_in));
		Recvaddr.sin_family = AF_INET;
		Recvaddr.sin_port = htons(client_port);
		Recvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		return bind(m_nSocket,(sockaddr *)&Recvaddr,sizeof(Recvaddr));
	}
}

void Socket::Colse(){
	if (m_nSocket != -1)
	{
#ifdef WIN
		closesocket(m_nSocket);
#else
		close(m_nSocket);
#endif
		m_nSocket = -1;
	}
}