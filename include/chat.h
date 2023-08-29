/* This file should not be included anywhere, use
 * #include "hive.h"
 * instead.
 */

#include <pthread.h>

#define CHAT_IN_CHANGED		0x01
#define CHAT_OUT_CHANGED	0x02
#define CHAT_CURSOR_CHANGED	0x04
#define CHAT_CHANGED_MASK	0x07

#define CHAT_NEWLINE		0x08

#define CHAT_CONNECTED		0x10

struct chat_job {
	struct chat *chat;
	pthread_t tid;
	char *args;
	int nargs;
};

struct chat {
	/* name of the user */
	char name[256];
	int flags;
	/* net socket (this socket is active while the flag
	 * CHAT_CONNECTED is toggled)
	 */
	int socket;
	/* running jobs (background commands) */
	struct chat_job jobs[10];
	struct chat_job syncJob;
	/* pad region for offscreen rendering and measuring */
	WINDOW *win;
	int x, y, w, h;
	/* mutex for the output stream */
	pthread_mutex_t outLock;
	/* user viewable output buffer */
	char out[32768];
	size_t nOut;
	/* active user input buffer */
	char in[1024];
	size_t nIn, iIn;
};

int chat_init(struct chat *chat, int x, int y, int w, int h);
/* executes the command stored in `in` */
int chat_exec(struct chat *chat);
int chat_setposition(struct chat *chat, int x, int y, int w, int h);
size_t chat_writein(struct chat *chat, const char *buf, size_t szBuf);
size_t chat_writeout(struct chat *chat, const char *buf, size_t szBuf);
size_t chat_syncwriteout(struct chat *chat, const char *buf, size_t szBuf);
int chat_handle(struct chat *chat, int c);
int chat_update(struct chat *chat);
