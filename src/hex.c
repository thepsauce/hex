#include "hex.h"

void curses_init(void)
{
	setlocale(LC_ALL, "");

	initscr();
	keypad(stdscr, true);
	cbreak();
	noecho();
	mousemask(ALL_MOUSE_EVENTS, NULL);
	mouseinterval(0);
	timeout(10);

	start_color();
	for (int fg = 0; fg < 8; fg++)
		for (int bg = 0; bg < 8; bg++)
			init_pair(1 + fg + bg * 8, fg, bg);
	init_pair(PAIR_NORMAL, COLOR_WHITE, COLOR_BLACK);
	init_pair(PAIR_ERROR, COLOR_RED, COLOR_BLACK);
	init_pair(PAIR_INFO, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(PAIR_COMMAND, COLOR_BLUE, COLOR_BLACK);
	init_pair(PAIR_ARGUMENT, COLOR_GREEN, COLOR_BLACK);

	refresh();
}
