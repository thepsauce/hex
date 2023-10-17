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
	init_pair(HIVE_PAIR_SELECTED, COLOR_YELLOW, COLOR_BLACK);
	init_pair(HIVE_PAIR_BLACK_WHITE, COLOR_RED, COLOR_BLUE);
	init_pair(HIVE_PAIR_WHITE_BLACK, COLOR_BLUE, COLOR_RED);
	init_pair(HIVE_PAIR_BLACK_BLACK, COLOR_RED, COLOR_RED);
	init_pair(HIVE_PAIR_WHITE_WHITE, COLOR_BLUE, COLOR_BLUE);
	init_pair(HIVE_PAIR_WHITE_WHITE, COLOR_BLUE, COLOR_BLUE);
	refresh();

	hc_init(&hive_chat);
	while (1) {
		int xMax, yMax;

		hive_render(&hive);
		getmaxyx(hive.board.win, yMax, xMax);
		wattrset(hive.board.win, 0);
		for (int x = 0; x <= xMax / 4; x++)
			for (int y = 0; y <= yMax / 2; y++) {
				Point p = { x, y };
				HivePiece *pieces[6];
				const size_t c = hive_region_getsurrounding(
						&hive.board, p, pieces);
				hive_pointtoworld(&p, hive.board.translation);
				mvwprintw(hive.board.win, p.y + 1, p.x + 2, "%zu", c);
			}
		wnoutrefresh(hive.board.win);
		doupdate();
		const int c = getch();
		if (hive.selectedPiece) {
			switch (c) {
			case KEY_LEFT:
				hive.selectedPiece->position.x--;
				break;
			case KEY_UP:
				hive.selectedPiece->position.y--;
				break;
			case KEY_RIGHT:
				hive.selectedPiece->position.x++;
				break;
			case KEY_DOWN:
				hive.selectedPiece->position.y++;
				break;
			}
			hive_computemoves(&hive);
		}
		hive_handle(&hive, c);
	}

	endwin();
	return 0;
}
