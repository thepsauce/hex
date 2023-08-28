#include "hive.h"

int chat_init(struct chat *chat)
{
	memset(chat, 0, sizeof(*chat));
	return 0;
}

static void chat_render(struct chat *chat)
{
	WINDOW *win;
	int x, y;
	int cx, cy; 
	int boxHeight, inputHeight;

	win = newpad(chat->h, chat->w);
	scrollok(win, true);

	if(chat->newlineMode)
		wcolor_set(win, HIVE_PAIR_BLACK, NULL);
	for (size_t i = 0;; i++) {
		getyx(win, y, x);
		if (y == chat->h / 2) {
			if(chat->iWrite < i)
				break;
			scroll(win);
			y--;
			wmove(win, y, x);
			cy--;
		}
		if (i == chat->iWrite) {
			cx = x;
			cy = y;
		}
		if (i == chat->nWrite)
			break;
		waddch(win, chat->wBuf[i]);
	}

	getyx(win, y, x);
	inputHeight = y;
	boxHeight = chat->h - inputHeight;
	pnoutrefresh(win, y - inputHeight, 0,
		chat->y + boxHeight - 1, chat->x,
		chat->y + chat->h - 1, chat->x + chat->w - 1);

	wcolor_set(win, 0, NULL);
	mvwaddnstr(win, 0, 0, chat->rBuf, chat->nRead);
	getyx(win, y, x);
	wclrtobot(win);
	pnoutrefresh(win, y - boxHeight, 0,
			chat->y, chat->x,
			chat->y + boxHeight - 2, chat->x + chat->w - 1);

	delwin(win);

	move(chat->y + boxHeight - 1 + cy, chat->x + cx);
}

int chat_handle(struct chat *chat, int c)
{
	switch(c) {
	case KEY_HOME:
		if (chat->iWrite == 0)
			return 0;
		chat->iWrite = 0;
		break;
	case KEY_END:
		if (chat->iWrite == chat->nWrite)
			return 0;
		chat->iWrite = chat->nWrite;
		break;
	case KEY_LEFT:
		if (chat->iWrite == 0)
			return 0;
		chat->iWrite--;
		break;
	case KEY_RIGHT:
		if (chat->iWrite == chat->nWrite)
			return 0;
		chat->iWrite++;
		break;
	case KEY_BACKSPACE:
	case 0x7f:
		if (chat->iWrite == 0)
			return 0;
		chat->iWrite--;
		chat->nWrite--;
		memmove(chat->wBuf + chat->iWrite,
			chat->wBuf + chat->iWrite + 1,
			chat->nWrite - chat->iWrite);
		break;
	case '\n':
		if (chat->nWrite == 0)
			return 0;
		if (chat->newlineMode) {
			memcpy(chat->rBuf + chat->nRead,
				"<Reik> ", sizeof("<Reik>"));
			chat->nRead += sizeof("<Reik>");
			memcpy(chat->rBuf + chat->nRead,
				chat->wBuf, chat->nWrite);
			chat->nRead += chat->nWrite;
			chat->rBuf[chat->nRead++] = '\n';
			chat->iWrite = 0;
			chat->nWrite = 0;
			chat->newlineMode = false;
		} else {
			chat->newlineMode = true;
		}
		break;
	default:
		if (!isspace(c) && (c < 32 || c > 0xff))
			return 0;
		if (chat->newlineMode) {
			chat->newlineMode = false;
			if (chat->nWrite + 1 >= sizeof(chat->wBuf))
				return 0;
			memmove(chat->wBuf + chat->iWrite + 1,
				chat->wBuf + chat->iWrite,
				chat->nWrite - chat->iWrite);
			chat->wBuf[chat->iWrite++] = '\n';
			chat->nWrite++;
		}
		if (chat->nWrite == sizeof(chat->wBuf))
			return 0;
		memmove(chat->wBuf + chat->iWrite + 1,
			chat->wBuf + chat->iWrite,
			chat->nWrite - chat->iWrite);
		chat->wBuf[chat->iWrite++] = c;
		chat->nWrite++;
	}
	if (chat->w * chat->h == 0)
		return 0;

	if (c != '\n')
		chat->newlineMode = false;
	chat_render(chat);
	return 0;
}

int chat_write(struct chat *chat, const char *msg, size_t nMsg)
{
	return 0;
}
