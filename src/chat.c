#include "hive.h"

#define chat_lockout(c) pthread_mutex_lock(&((c)->outLock))
#define chat_unlockout(c) pthread_mutex_unlock(&((c)->outLock))

#define chat_locksend(c) pthread_mutex_lock(&((c)->sendLock))
#define chat_unlocksend(c) pthread_mutex_unlock(&((c)->sendLock))

static bool chat_iscommand(struct chat *chat)
{
	for (size_t i = 0; i < chat->nIn; i++)
		if(!isspace(chat->in[i]))
			return chat->in[i] == '/';
	return false;
}

static void chat_render(struct chat *chat)
{
	WINDOW *const win = chat->win;
	int x, y;
	int cx, cy;
	int boxHeight, inputHeight;

	if (chat->w == 0 || chat->h == 0)
		return;

	if (chat_iscommand(chat))
		wcolor_set(win, HIVE_PAIR_WHITE, NULL);
	if (chat->flags & CHAT_NEWLINE)
		wcolor_set(win, HIVE_PAIR_BLACK, NULL);
	wmove(win, 0, 0);
	for (size_t i = 0;; i++) {
		getyx(win, y, x);
		if (y == chat->h / 2) {
			if(chat->iIn < i)
				break;
			scroll(win);
			y--;
			wmove(win, y, x);
			cy--;
		}
		if (i == chat->iIn) {
			cx = x;
			cy = y;
		}
		if (i == chat->nIn)
			break;
		waddch(win, chat->in[i]);
	}
	wclrtobot(win);
	getyx(win, y, x);
	inputHeight = y;
	boxHeight = chat->h - inputHeight;
	pnoutrefresh(win, y - inputHeight, 0,
		chat->y + boxHeight - 1, chat->x,
		chat->y + chat->h - 1, chat->x + chat->w - 1);

	wcolor_set(win, 0, NULL);

	chat_lockout(chat);
	mvwaddnstr(win, 0, 0, chat->out, chat->nOut);
	chat_unlockout(chat);

	getyx(win, y, x);
	wclrtobot(win);
	pnoutrefresh(win, y - boxHeight, 0,
			chat->y, chat->x,
			chat->y + boxHeight - 2, chat->x + chat->w - 1);

	move(chat->y + boxHeight - 1 + cy, chat->x + cx);
}

int chat_init(struct chat *chat, int x, int y, int w, int h)
{
	memset(chat, 0, sizeof(*chat));
	chat->x = x;
	chat->y = y;
	chat->w = w;
	chat->h = h;
	if ((chat->win = newpad(h, w)) == NULL)
		return -1;
	if ((pthread_mutex_init(&chat->outLock, NULL)) < 0) {
		delwin(chat->win);
		return -1;
	}
	for (size_t j = 0; j < ARRLEN(chat->jobs); j++)
		chat->jobs[j].chat = chat;
	chat->syncJob.chat = chat;
	scrollok(chat->win, true);
	chat_render(chat);
	return 0;
}

int chat_setposition(struct chat *chat, int x, int y, int w, int h)
{
	if(w != chat->w || h != chat->h) {
		chat->w = w;
		chat->h = h;
		delwin(chat->win);
		if ((chat->win = newpad(h, w)) == NULL)
			return -1;
		scrollok(chat->win, true);
	} else if(x == chat->x && y == chat->y) {
		return 1;
	}
	chat->x = x;
	chat->y = y;
	chat_render(chat);
	return 0;
}

size_t chat_writein(struct chat *chat, const char *buf, size_t szBuf)
{
	const size_t capacity = sizeof(chat->in) - chat->nIn;
	const size_t actual = capacity < szBuf ? capacity : szBuf;

	buf += szBuf - actual;
	if (chat->flags & CHAT_NEWLINE) {
		chat->flags ^= CHAT_NEWLINE;
		if (chat->nIn + 1 + actual >= sizeof(chat->in))
			return 0;
		memmove(chat->in + chat->iIn + 1,
			chat->in + chat->iIn,
			chat->nIn - chat->iIn);
		chat->in[chat->iIn++] = '\n';
		chat->nIn++;
	}
	memmove(chat->in + chat->iIn + actual,
		chat->in + chat->iIn,
		chat->nIn - chat->iIn);
	memcpy(chat->in + chat->iIn, buf, actual);
	chat->iIn += actual;
	chat->nIn += actual;
	chat->flags |= CHAT_IN_CHANGED;
	return actual;
}

size_t chat_writeout(struct chat *chat, const char *buf, size_t szBuf)
{
	const size_t actual = szBuf % sizeof(chat->out);
	const size_t capacity = sizeof(chat->out) - chat->nOut;

	buf += szBuf - actual;
	if (actual > capacity)
		memmove(chat->out,
			chat->out + actual - capacity,
			chat->nOut - (actual - capacity));
	memcpy(chat->out + chat->nOut, buf, szBuf);
	chat->nOut += actual;
	chat->flags |= CHAT_OUT_CHANGED;
	return actual;
}

size_t chat_syncwriteout(struct chat *chat, const char *buf, size_t szBuf)
{
	pthread_mutex_lock(&chat->outLock);
	const size_t r = chat_writeout(chat, buf, szBuf);
	pthread_mutex_unlock(&chat->outLock);
	return r;
}

void chat_sendmessage(struct chat *chat)
{
	if (chat->flags & CHAT_CONNECTED) {
		char *const buf = malloc(300 + chat->nIn);
		sprintf(buf, "<%s> %.*s\n", chat->name,
				(int) chat->nIn, chat->in);
		net_send(chat->socket, buf, strlen(buf));
		free(buf);
	}

	chat_lockout(chat);
	chat_writeout(chat, "<", 1);
	chat_writeout(chat, chat->name, strlen(chat->name));
	chat_writeout(chat, "> ", 2);
	chat_writeout(chat, chat->in, chat->nIn);
	chat_writeout(chat, "\n", 1);
	chat_unlockout(chat);
	chat->iIn = 0;
	chat->nIn = 0;
	chat->flags |= CHAT_CURSOR_CHANGED;
	chat->flags |= CHAT_IN_CHANGED;
}

int chat_handle(struct chat *chat, int c)
{
	switch(c) {
	case KEY_HOME:
		if (chat->iIn == 0)
			break;
		chat->iIn = 0;
		chat->flags |= CHAT_CURSOR_CHANGED;
		break;
	case KEY_END:
		if (chat->iIn == chat->nIn)
			break;
		chat->iIn = chat->nIn;
		chat->flags |= CHAT_CURSOR_CHANGED;
		break;
	case KEY_LEFT:
		if (chat->iIn == 0)
			break;
		chat->iIn--;
		chat->flags |= CHAT_CURSOR_CHANGED;
		break;
	case KEY_RIGHT:
		if (chat->iIn == chat->nIn)
			break;
		chat->iIn++;
		chat->flags |= CHAT_CURSOR_CHANGED;
		break;
	case KEY_BACKSPACE:
	case 0x7f:
		if (chat->iIn == 0)
			break;
		chat->iIn--;
		chat->nIn--;
		memmove(chat->in + chat->iIn,
			chat->in + chat->iIn + 1,
			chat->nIn - chat->iIn);
		chat->flags |= CHAT_IN_CHANGED;
		break;
	case '\n':
		if (chat->nIn == 0)
			break;
		if (chat_iscommand(chat)) {
			chat_exec(chat);
			break;
		}
		if (chat->flags & CHAT_NEWLINE)
			chat_sendmessage(chat);
		chat->flags ^= CHAT_NEWLINE;
		chat->flags |= CHAT_IN_CHANGED;
		break;
	default:
		if (!isspace(c) && (c < 32 || c > 0xff))
			break;
		chat_writein(chat, &(char) { c }, 1);
	}
	if (c != '\n')
		chat->flags &= ~CHAT_NEWLINE;
	return 0;
}

int chat_update(struct chat *chat)
{
	if (chat->flags & CHAT_CHANGED_MASK) {
		chat_render(chat);
		chat->flags &= ~CHAT_CHANGED_MASK;
	}
	return 0;
}
