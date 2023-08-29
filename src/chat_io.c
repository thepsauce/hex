#include "hive.h"

void chat_sendmessage(struct chat *chat)
{
	char *const buf = malloc(1 + sizeof(chat->name) + 2 + chat->nIn + 2);
	sprintf(buf, "<%s> %.*s\n", chat->name, (int) chat->nIn, chat->in);
	if (chat->sender >= 0)
		net_send(chat->sender, buf, strlen(buf));
	chat_writeout(chat, buf, strlen(buf));
	free(buf);
	chat->iIn = 0;
	chat->nIn = 0;
	chat->flags |= CHAT_CURSOR_CHANGED;
	chat->flags |= CHAT_IN_CHANGED;
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

int chat_printf(struct chat *chat, const char *fmt, ...)
{
	char *buf;
	int sz;
	va_list l;

	va_start(l, fmt);
	sz = vsnprintf(NULL, 0, fmt, l);
	if (sz < 0)
		return sz;
	va_end(l);

	buf = malloc(sz + 1);
	va_start(l, fmt);
	vsprintf(buf, fmt, l);
	va_end(l);

	chat_writeout(chat, buf, sz);
	free(buf);
	return sz % sizeof(chat->out);
}
