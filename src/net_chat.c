#include "hex.h"

int net_chat_init(NetChat *chat, int x, int y, int w, int h, int outArea)
{
	memset(chat, 0, sizeof(*chat));
	if ((chat->win = newwin(h, w, y, x)) == NULL)
		return -1;
	if ((chat->output.win = newpad(outArea / w + 1, w)) == NULL) {
		delwin(chat->win);
		return -1;
	}
	if ((pthread_mutex_init(&chat->output.lock, NULL)) < 0) {
		delwin(chat->win);
		delwin(chat->output.win);
		return -1;
	}
	strcpy(chat->name, "Anon");
	for (size_t i = 0; i < ARRLEN(chat->jobs); i++)
		chat->jobs[i].chat = chat;
	chat->syncJob.chat = chat;
	scrollok(chat->output.win, true);
	/* print initial text */
	WINDOW *const win = chat->output.win;
	wattr_set(win, 0, PAIR_INFO, NULL);
	waddstr(win, "Welcome to hex, a simple chatting network that can also"
			"be used to play the board game Hive!\n");
	waddstr(win,
		"  ___     ___    ___ ___    ___     ___   \n"
		" /   \\   /   \\  /   \\   \\  /   \\___/   \\ \n"
		" \\___/   \\___/  \\___/___/  \\___/   \\___/  \n"
		" /   \\___/   \\  /   \\___       \\___/      \n"
		" \\___/   \\___/  \\___/   \\      /   \\     \n"
		" /   \\___/   \\  /   \\___/      \\___/      \n"
		" \\___/   \\___/  \\___/___    ___/   \\___  \n"
		" /   \\   /   \\  /   \\   \\  /   \\___/   \\ \n"
		" \\___/   \\___/  \\___/___/  \\___/   \\___/  \n\n");
	waddstr(win, "You can either ignore this and play offline or host/join a server\n");
	waddstr(win, "Type '/help' for help\nGL&HF!\n\n");
	return 0;
}

int net_chat_setposition(NetChat *chat, int x, int y, int w, int h)
{
	int xOld, yOld, wOld, hOld;

	getbegyx(chat->win, yOld, xOld);
	getmaxyx(chat->win, hOld, wOld);
	hOld -= xOld - 1;
	wOld -= yOld - 1;
	if(w != wOld || h != hOld) {
		WINDOW *next, *old;
		int xMax, yMax;

		old = chat->win;
		if ((next = newwin(h, w, y, x)) == NULL)
			return -1;
		chat->win = next;
		delwin(old);

		old = chat->output.win;
		if ((next = newpad(chat->output.area / w + 1, w)) == NULL)
			return -1;
		scrollok(next, true);

		pthread_mutex_lock(&chat->output.lock);
		getmaxyx(old, yMax, xMax);
		for (int y = 0; y < yMax; y++)
			for (int x = 0; x < xMax; x++)
				waddch(next, mvwinch(old, y, x));
		chat->output.win = next;
		pthread_mutex_unlock(&chat->output.lock);

		delwin(old);
	} else if(x == xOld && y == yOld) {
		return 1;
	}
	return 0;
}

static bool net_chat_iscommand(NetChat *chat)
{
	for (size_t i = 0; i < chat->input.length; i++)
		if (!isspace(chat->input.buffer[i]))
			return chat->input.buffer[i] == '/';
	return false;
}

size_t net_chat_writein(NetChat *chat,
		const char *buf, size_t szBuf)
{
	const size_t capacity = sizeof(chat->input.buffer) - chat->input.length;
	const size_t actual = capacity < szBuf ? capacity : szBuf;

	buf += szBuf - actual;
	memmove(chat->input.buffer + chat->input.index + actual,
		chat->input.buffer + chat->input.index,
		chat->input.length - chat->input.index);
	memcpy(chat->input.buffer + chat->input.index, buf, actual);
	chat->input.index += actual;
	chat->input.length += actual;
	return actual;
}

void net_chat_render(NetChat *chat)
{
	WINDOW *const win = chat->win;
	int xBeg, yBeg, xMax, yMax;
	int x, y;

	getbegyx(win, yBeg, xBeg);
	getmaxyx(win, yMax, xMax);
	/* deduct input height */
	yMax--;

	pthread_mutex_lock(&chat->output.lock);
	getyx(chat->output.win, y, x);
	chat->output.scroll = MIN(MAX(y - yMax + yBeg, 0), chat->output.scroll);
	prefresh(chat->output.win, MAX(y - yMax + yBeg - chat->output.scroll, 0), 0,
			yBeg, xBeg, yBeg + yMax - 1, xBeg + xMax);
	pthread_mutex_unlock(&chat->output.lock);

	wattr_set(win, 0, net_chat_iscommand(chat) ? PAIR_COMMAND :
			PAIR_NORMAL, NULL);
	wmove(win, yMax, 0);
	wclrtoeol(win);
	wmove(win, yMax, 0);
	waddnstr(win, chat->input.buffer, chat->input.index);
	getyx(win, y, x);
	waddnstr(win, chat->input.buffer + chat->input.index,
			chat->input.length - chat->input.index);
	wmove(win, y, x);
	wnoutrefresh(win);
}

bool net_chat_handlemousepress(NetChat *chat, Point mouse)
{
	if (!wmouse_trafo(chat->win, &mouse.y, &mouse.x, false))
		return false;
	return true;
}

int net_chat_handle(NetChat *chat, int c)
{
	switch(c) {
	case KEY_HOME:
		if (chat->input.index == 0)
			break;
		chat->input.index = 0;
		break;
	case KEY_END:
		if (chat->input.index == chat->input.length)
			break;
		chat->input.index = chat->input.length;
		break;
	case KEY_LEFT:
		if (chat->input.index == 0)
			break;
		chat->input.index--;
		break;
	case KEY_RIGHT:
		if (chat->input.index == chat->input.length)
			break;
		chat->input.index++;
		break;
	case KEY_UP:
		chat->output.scroll++;
		break;
	case KEY_DOWN:
		if (chat->output.scroll == 0)
			break;
		chat->output.scroll--;
		break;
	case KEY_BACKSPACE:
	case 0x7f:
		if (chat->input.index == 0)
			break;
		chat->input.index--;
		chat->input.length--;
		memmove(chat->input.buffer + chat->input.index,
			chat->input.buffer + chat->input.index + 1,
			chat->input.length - chat->input.index);
		break;
	case '\n': {
		NetRequest req;

		if (chat->input.length == 0)
			break;
		if (net_chat_iscommand(chat)) {
			net_chat_exec(chat);
			break;
		}
		if (chat->net.socket > 0) {
			chat->input.index = chat->input.length;
			net_chat_writein(chat, "", 1);
			if (chat->net.isServer) {
				pthread_mutex_lock(&chat->output.lock);
				wattr_set(chat->output.win, 0, PAIR_INFO, NULL);
				wprintw(chat->output.win, "Server> %s\n",
						chat->input.buffer);
				pthread_mutex_unlock(&chat->output.lock);
				net_request_init(&req, NET_REQUEST_SRV,
					chat->input.buffer);
			} else {
				net_request_init(&req, NET_REQUEST_MSG,
					chat->name, chat->input.buffer);
			}
			net_receiver_send(&chat->net, &req);
		} else {
			pthread_mutex_lock(&chat->output.lock);
			wattr_set(chat->output.win, 0, PAIR_ERROR, NULL);
			wprintw(chat->output.win,
				"You are not connected to any network.\n");
			pthread_mutex_unlock(&chat->output.lock);
		}
		chat->input.index = 0;
		chat->input.length = 0;
		break;
	}
	default:
		if (!isspace(c) && (c < 32 || c > 0xff))
			break;
		net_chat_writein(chat, &(char) { c }, 1);
	}
	return 0;
}
