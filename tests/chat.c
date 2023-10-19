#include "test.h"

HiveChat hive_chat;

int main(void)
{
	NetChat *const chat = &hive_chat.chat;

	curses_init();
	hc_init(&hive_chat);
	while(1) {
		int x, y;
		int xBeg, yBeg;

		net_chat_render(chat);
		getyx(chat->win, y, x);
		getbegyx(chat->win, yBeg, xBeg);
		move(yBeg + y, xBeg + x);
		const int c = getch();
		net_chat_handle(chat, c);
	}

	return 0;
}
