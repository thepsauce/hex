#include "hive.h"

int net_socket(void)
{
	return socket(AF_INET, SOCK_STREAM, 0);
}

int net_host(int socket, int port)
{
	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);
	if(bind(socket, (struct sockaddr*) &address, sizeof(address)) < 0)
		return -1;
	return 0;
}

int net_connect(int socket, const char *ip, int port)
{
	struct sockaddr_in address;

	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	if(inet_pton(AF_INET, ip, &address.sin_addr) <= 0)
		return -1;
	if(connect(socket, (struct sockaddr*) &address, sizeof(address)) < 0)
		return -1;
	return 0;
}

ssize_t net_receive(int socket, char *buf, size_t szBuf)
{
	return recv(socket, buf, szBuf, 0);
}

ssize_t net_send(int socket, const char *buf, size_t szBuf)
{
	return send(socket, buf, szBuf, 0);
}
