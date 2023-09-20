#include <arpa/inet.h>

/* Creates a new socket */
int net_socket(void);

/* Makes a socket a server host socket */
int net_host(int socket, int port);

/* Makes a socket a client socket */
int net_connect(int socket, const char *ip, int port);

/* Reads message from a socket */
ssize_t net_receive(int socket, char *buf, size_t szBuf);

/* Writes message to a socket */
ssize_t net_send(int socket, const char *buf, size_t szBuf);

