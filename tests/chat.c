#include "test.h"

HiveChat hive_chat;

int main(void)
{
	NetChat *const chat = &hive_chat.chat;

	setlocale(LC_ALL, "");

	initscr();
	keypad(stdscr, true);
	cbreak();
	noecho();
	mousemask(ALL_MOUSE_EVENTS, NULL);
	mouseinterval(0);
	timeout(300);
	start_color();
	//nodelay(stdscr, true);
	init_pair(PAIR_NORMAL, COLOR_WHITE, COLOR_BLACK);
	init_pair(PAIR_ERROR, COLOR_RED, COLOR_BLACK);
	init_pair(PAIR_INFO, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(PAIR_COMMAND, COLOR_BLUE, COLOR_BLACK);
	init_pair(PAIR_ARGUMENT, COLOR_GREEN, COLOR_BLACK);
	refresh();

	hc_init(&hive_chat);
	while(1) {
		int x, y;

		net_chat_render(chat);
		getyx(chat.win, y, x);
		move(y, x);
		const int c = getch();
		net_chat_handle(chat, c);
	}

	return 0;
}
