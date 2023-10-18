#include "test.h"

HiveChat hive_chat;

int main(void)
{
	Hive *const hive = &hive_chat.hive;

	setlocale(LC_ALL, "");

	initscr();
	keypad(stdscr, true);
	cbreak();
	noecho();
	curs_set(0);
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

	const struct {
		int key;
	} keys[] = {
		{ 'f' },
		{ 's' },
		{ 'w' },
		{ 'p' },
		{ 'r' },
		{ 't' },
	};

	HivePiece mover = {
		.side = HIVE_WHITE,
		.type = HIVE_ANT,
		.position = { 8, 8 },
	};
	while (1) {
		hive_render(hive);
		hive_piece_render(&mover, stdscr, (Point) { 0, 0 });
		mvprintw(10, 10, "%d, %d", mover.position.x, mover.position.y);
		const int c = getch();
		switch (c) {
		default:
			for (int d = 0; d < (int) ARRLEN(keys); d++) {
				if (keys[d].key != c)
					continue;
				hive_movepoint(&mover.position, d);
				break;
			}
			break;
		case KEY_LEFT:
			mover.position.x--;
			break;
		case KEY_UP:
			mover.position.y--;
			break;
		case KEY_RIGHT:
			mover.position.x++;
			break;
		case KEY_DOWN:
			mover.position.y++;
			break;
		}
		hive_handle(hive, c);
	}
	
	endwin();
	return 0;
}
