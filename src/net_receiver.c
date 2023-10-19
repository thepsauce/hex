#include "hex.h"

int net_receiver_init(NetReceiver *rcv, int sock, bool isServer)
{
	struct pollfd *fd;
	struct net_entry *entry;

	fd = malloc(sizeof(*fd));
	if (fd == NULL)
		return -1;
	entry = malloc(sizeof(*entry));
	if (entry == NULL) {
		free(fd);
		return -1;
	}
	memset(rcv, 0, sizeof(*rcv));
	fd->events = POLLIN;
	fd->fd = sock;
	rcv->pollfds = fd;
	rcv->numPollfds = 1;
	memset(entry, 0, sizeof(*entry));
	entry->socket = sock;
	strcpy(entry->name, "<this>");
	rcv->entries = entry;
	rcv->socket = sock;
	rcv->isServer = isServer;
	return 0;
}

void net_receiver_uninit(NetReceiver *rcv)
{
	for (nfds_t i = 0; i < rcv->numPollfds; i++)
		close(rcv->pollfds[i].fd);
	free(rcv->pollfds);
	rcv->numPollfds = 0;
	rcv->socket = 0;
}

ssize_t net_receiver_send(NetReceiver *rcv, NetRequest *req)
{
	const char *const msg = net_request_serialize(req);
	const size_t lenMsg = strlen(msg);
	ssize_t total = 0;

	if (!rcv->isServer)
		return send(rcv->pollfds[0].fd, msg, lenMsg, 0);

	for (nfds_t i = 1; i < rcv->numPollfds; i++) {
		const int sock = rcv->pollfds[i].fd;
		const ssize_t n = send(sock, msg, lenMsg, 0);
		if (n > 0)
			total += n;
	}
	return total;
}

ssize_t net_receiver_sendformatted(NetReceiver *rcv, int socket,
		net_request_type_t type, const char *fmt, ...)
{
	NetRequest req;
	va_list l;

	gettimeofday(&req.time, NULL);
	req.type = type;
	va_start(l, fmt);
	const int n = vsnprintf(req.extra, sizeof(req.extra), fmt, l);
	va_end(l);
	if ((size_t) n >= sizeof(req.extra))
		return -1;
	if (socket == 0)
		return net_receiver_send(rcv, &req);
	const char *const msg = net_request_serialize(&req);
	const size_t lenMsg = strlen(msg);
	return send(socket, msg, lenMsg, 0);
}

ssize_t net_receiver_sendany(NetReceiver *rcv, int socket,
		net_request_type_t type, ...)
{
	va_list l;
	NetRequest req;

	va_start(l, type);
	const int r = net_request_vinit(&req, type, l);
	va_end(l);
	if (r < 0)
		return -1;
	if (socket == 0)
		return net_receiver_send(rcv, &req);
	const char *const msg = net_request_serialize(&req);
	const size_t lenMsg = strlen(msg);
	return send(socket, msg, lenMsg, 0);
}

bool net_receiver_nextrequest(NetReceiver *rcv, struct net_entry **pEntry, NetRequest *req)
{
	int sock;
	const int pc = poll(rcv->pollfds, rcv->numPollfds, -1);
	if (pc < 0) {
		net_receiver_uninit(rcv);
		return false;
	}

	for (nfds_t i = 0; i < rcv->numPollfds; i++) {
		struct pollfd *const pfd = rcv->pollfds + i;
		if (!(pfd->revents & POLLIN))
			continue;
		struct net_entry *const entry = &rcv->entries[i];
		sock = pfd->fd;
		*pEntry = entry;
		if (sock == rcv->socket && rcv->isServer) {
			struct sockaddr_in addr;
			socklen_t addrlen;

			addrlen = sizeof(addr);
			const int newSock = accept(rcv->socket,
				(struct sockaddr*) &addr, &addrlen);
			if (newSock < 0) {
				net_request_init(req, NET_REQUEST_NONE);
				return true;
			}
			if (net_receiver_put(rcv, newSock) < 0) {
				close(newSock);
				net_request_init(req, NET_REQUEST_NONE);
				return true;
			}
			net_request_init(req, NET_REQUEST_JIN);
		} else {
			char *chr;
			const ssize_t nBuf = recv(sock,
				entry->data + entry->ptr,
				sizeof(entry->data) - entry->ptr, 0);
			if (nBuf <= 0) {
				net_request_init(req, NET_REQUEST_LVE,
						entry->name);
				goto disconnect;
			}
			entry->ptr += nBuf;
			if ((chr = memchr(entry->data, '\r',
						entry->ptr)) != NULL) {
				chr++;
				const size_t n = chr - entry->data;
				/* got a full package */
				if (net_request_deserialize(req,
							entry->data) < 0) {
					/* don't deal with sockets
					** that send invalid data */
					net_request_init(req, NET_REQUEST_KCK,
							entry->name);
					goto disconnect;
				}
				memmove(entry->data, chr, n);
				entry->ptr -= n;
			}
		}
	}
	return true;
disconnect:
	if (rcv->isServer) {
		close(sock);
		net_receiver_remove(rcv, sock);
		return true;
	}
	net_receiver_uninit(rcv);
	return false;
}

nfds_t net_receiver_indexof(NetReceiver *rcv, int sock)
{
	for (nfds_t i = 0; i < rcv->numPollfds; i++) {
		if (rcv->pollfds[i].fd != sock)
			continue;
		return i;
	}
	return (nfds_t) -1;
}

int net_receiver_put(NetReceiver *rcv, int sock)
{
	struct net_entry *entry;
	struct pollfd *fd;

	entry = realloc(rcv->entries, sizeof(*rcv->entries) *
			(rcv->numPollfds + 1));
	if (entry == NULL)
		return -1;
	rcv->entries = entry;
	entry += rcv->numPollfds;
	memset(entry, 0, sizeof(*entry));
	strcpy(entry->name, "Anon");
	entry->socket = sock;

	fd = realloc(rcv->pollfds, sizeof(*rcv->pollfds) *
			(rcv->numPollfds + 1));
	if (fd == NULL)
		return -1;
	rcv->pollfds = fd;
	fd += rcv->numPollfds;
	fd->fd = sock;
	fd->events = POLLIN;
	rcv->numPollfds++;
	return 0;
}

int net_receiver_remove(NetReceiver *rcv, int sock)
{
	nfds_t i;

	if ((i = net_receiver_indexof(rcv, sock)) == (nfds_t) -1)
		return -1;
	rcv->numPollfds--;
	rcv->pollfds[i] = rcv->pollfds[rcv->numPollfds];
	rcv->entries[i] = rcv->entries[rcv->numPollfds];
	return 0;
}
