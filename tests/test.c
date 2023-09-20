#include "test.h"

struct chat chat_term;

int main(void)
{
	setlocale(LC_ALL, "");

	initscr();
	keypad(stdscr, true);
	cbreak();
	noecho();
	mousemask(ALL_MOUSE_EVENTS, NULL);
	mouseinterval(0);
	start_color();
	timeout(10);
	//nodelay(stdscr, true);
	init_pair(HIVE_PAIR_BLACK, COLOR_RED, COLOR_BLACK);
	init_pair(HIVE_PAIR_WHITE, COLOR_BLUE, COLOR_BLACK);
	init_pair(HIVE_PAIR_BLACK_WHITE, COLOR_RED, COLOR_BLUE);
	init_pair(HIVE_PAIR_WHITE_BLACK, COLOR_BLUE, COLOR_RED);
	init_pair(HIVE_PAIR_BLACK_BLACK, COLOR_RED, COLOR_RED);
	init_pair(HIVE_PAIR_WHITE_WHITE, COLOR_BLUE, COLOR_BLUE);
	refresh();

	chat_init(&chat_term, 0, 0, 30, LINES);

	const char *str = "hello there";
	strcpy(chat_term.out, str);
	chat_term.nOut = strlen(str);
	strcpy(chat_term.in, str);
	chat_term.nIn = strlen(str);
	while(1) {
		const int c = getch();
		chat_handle(&chat_term, c);
		chat_update(&chat_term);
	}

	return 0;
}
