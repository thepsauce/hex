#include "test.h"

HiveChat hive_chat;

int main(void)
{
	Hive *const hive = &hive_chat.hive;

	setlocale(LC_ALL, "");

	initscr();
	keypad(stdscr, true);
	cbreak();
	curs_set(0);
	noecho();
	mousemask(ALL_MOUSE_EVENTS, NULL);
	mouseinterval(0);
	start_color();
	timeout(10);
	init_pair(HIVE_PAIR_BLACK, COLOR_RED, COLOR_BLACK);
	init_pair(HIVE_PAIR_WHITE, COLOR_BLUE, COLOR_BLACK);
	init_pair(HIVE_PAIR_BLACK_WHITE, COLOR_RED, COLOR_BLUE);
	init_pair(HIVE_PAIR_WHITE_BLACK, COLOR_BLUE, COLOR_RED);
	init_pair(HIVE_PAIR_BLACK_BLACK, COLOR_RED, COLOR_RED);
	init_pair(HIVE_PAIR_WHITE_WHITE, COLOR_BLUE, COLOR_BLUE);
	init_pair(HIVE_PAIR_WHITE_WHITE, COLOR_BLUE, COLOR_BLUE);
	init_pair(HIVE_PAIR_SELECTED, COLOR_YELLOW, COLOR_BLACK);
	init_pair(HIVE_PAIR_CHOICE, COLOR_GREEN, COLOR_BLACK);
	refresh();

	hc_init(&hive_chat);
	while (1) {
		MEVENT ev;

		hive_render(hive);
		doupdate();
		const int c = getch();
		switch (c) {
		case KEY_MOUSE:
			getmouse(&ev);
			if ((ev.bstate & BUTTON1_CLICKED) ||
					(ev.bstate & BUTTON1_PRESSED))
				hive_handlemousepress(hive,
						(Point) { ev.x, ev.y });
			break;
		default:
			hive_handle(hive, c);
		}
	}

	endwin();
	return 0;
}
