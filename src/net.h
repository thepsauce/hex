#include <pthread.h>
#include <errno.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <poll.h>

#define NET_USER_DATA_SIZE 1024
#define NET_EXTRA_SIZE 512
/* inclusive: the name can be of length NET_MIN_NAME or higher */
#define NET_MIN_NAME 3
/* exclusive: the name length can only be less than NET_MAX_NAME */
#define NET_MAX_NAME 32

bool net_isvalidname(const char *name);
int net_porthash(const char *name);

/* Request format:
 * [second].[nano seconds] [type]:[data]
 */

typedef enum net_request_type {
	NET_REQUEST_NONE,
	NET_REQUEST_MSG,
	NET_REQUEST_SRV,
	NET_REQUEST_SUN,
	NET_REQUEST_JIN,
	NET_REQUEST_LVE,
	NET_REQUEST_KCK,

	NET_REQUEST_HIVE_CHALLENGE,
	NET_REQUEST_HIVE_MOVE,
} net_request_type_t;

typedef struct net_request {
	struct timeval time;
	net_request_type_t type;
	char name[NET_MAX_NAME];
	char extra[NET_EXTRA_SIZE];
} NetRequest;

const char *net_request_serialize(const NetRequest *req);
int net_request_vinit(NetRequest *req, net_request_type_t type, va_list l);
int net_request_init(NetRequest *req, net_request_type_t type, ...);
int net_request_deserialize(NetRequest *req, const char *data);

struct net_chat;

typedef struct net_chat_job {
	struct net_chat *chat;
	pthread_t threadId;
	char *args;
	int numArgs;
} NetChatJob;

typedef struct net_receiver {
	struct pollfd *pollfds;
	nfds_t numPollfds;
	struct net_entry {
		int socket;
		char name[NET_MAX_NAME];
		char data[NET_USER_DATA_SIZE];
		size_t ptr;
	} *entries;
	int socket;
	bool isServer;
} NetReceiver;

int net_receiver_init(NetReceiver *rec, int sock, bool isServer);
void net_receiver_uninit(NetReceiver *rec);
ssize_t net_receiver_send(NetReceiver *rcv, NetRequest *req);
ssize_t net_receiver_sendformatted(NetReceiver *rcv, int socket,
		net_request_type_t type, const char *fmt, ...);
ssize_t net_receiver_sendany(NetReceiver *rcv, int socket,
		net_request_type_t type, ...);
bool net_receiver_nextrequest(NetReceiver *rec, struct net_entry **pEntry,
		NetRequest *req);
nfds_t net_receiver_indexof(NetReceiver *rcv, int sock);
int net_receiver_put(NetReceiver *rec, int sock);
int net_receiver_remove(NetReceiver *rec, int sock);

typedef struct net_chat {
	WINDOW *win;
	char name[NET_MAX_NAME];
	NetReceiver net;
	/* sockets of the two players */
	/* there are three options:
	 * 1. Both are zero:
	 * 	Nobody is challenging or playing.
	 * 2. blackPlayer is not zero:
	 * 	One player has issued a challenge.
	 * 3. Both are not zero:
	 * 	Two players are playing
	 */
	int blackPlayer, whitePlayer;
	/* running jobs (background commands) */
	NetChatJob jobs[10];
	NetChatJob syncJob;
	struct {
		/* area of the output pad */
		size_t area;
		/* mutex for the output pad */
		pthread_mutex_t lock;
		/* user viewable output pad */
		WINDOW *win;
		int scroll;
	} output;
	/* active user input buffer */
	struct {
		char buffer[NET_EXTRA_SIZE];
		size_t length, index;
	} input;
} NetChat;

int net_chat_init(NetChat *chat, int x, int y, int w, int h, int outArea);
bool net_chat_handlemousepress(NetChat *chat, Point mouse);
int net_chat_handle(NetChat *chat, int c);
void net_chat_render(NetChat *chat);
/* Executes the command that is currently in the input field */
int net_chat_exec(NetChat *chat);
/* Sends what is currently in the input field to the server as MSG request */
int net_chat_post(NetChat *chat);

