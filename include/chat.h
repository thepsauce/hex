/* This file should not be included anywhere, use
 * #include "hive.h"
 * instead.
 */

#define CHAT_IN_CHANGED		0x01
#define CHAT_OUT_CHANGED	0x02
#define CHAT_CURSOR_CHANGED	0x04
#define CHAT_CHANGED_MASK	0x07

#define CHAT_NEWLINE		0x08

struct chat {
	int flags;
	int socket;
	/* pad region for offscreen rendering and measuring */
	WINDOW *win;
	int x, y, w, h;
	char out[32768];
	size_t nOut;
	char in[1024];
	size_t nIn, iIn;
};

int chat_init(struct chat *chat, int x, int y, int w, int h);
/* executes the command stored in `in` */
int chat_exec(struct chat *chat);
int chat_setposition(struct chat *chat, int x, int y, int w, int h);
size_t chat_writein(struct chat *chat, const char *buf, size_t szBuf);
size_t chat_writeout(struct chat *chat, const char *buf, size_t szBuf);
int chat_handle(struct chat *chat, int c);
int chat_update(struct chat *chat);
