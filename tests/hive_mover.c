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
		Point d1, d2;
	} keys[] = {
		{ 'f', {  0, -1 }, {  0, -1 } },
		{ 's', {  0,  1 }, {  0,  1 } },
		{ 'w', { -1, -1 }, { -1,  0 } },
		{ 'p', {  1, -1 }, {  1,  0 } },
		{ 'r', { -1,  0 }, { -1,  1 } },
		{ 't', {  1,  0 }, {  1,  1 } },
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
			for (size_t i = 0; i < ARRLEN(keys); i++) {
				if (keys[i].key != c)
					continue;
				const Point d = mover.position.x % 2 == 0 ?
					keys[i].d1 : keys[i].d2;
				mover.position.x += d.x;
				mover.position.y += d.y;
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
