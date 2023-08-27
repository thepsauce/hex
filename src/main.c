#include "hive.h"

struct hive hive_game;

enum {
	STATE_BOARD,
};

const struct {
	int (*state)(void *param, int c);
	void *param;
} all_states[] = {
	[STATE_BOARD] = {
		(int (*)(void*, int)) hive_handle,
		(void*) &hive_game
	},
};
int cur_state;

int main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	setlocale(LC_ALL, "");

	initscr();
	curs_set(false);
	keypad(stdscr, true);
	cbreak();
	noecho();
	mousemask(ALL_MOUSE_EVENTS, NULL);
	mouseinterval(0);
	start_color();
	//nodelay(stdscr, true);
	init_pair(HIVE_PAIR_BLACK, COLOR_RED, COLOR_BLACK);
	init_pair(HIVE_PAIR_WHITE, COLOR_BLUE, COLOR_BLACK);
	init_pair(HIVE_PAIR_BLACK_WHITE, COLOR_RED, COLOR_BLUE);
	init_pair(HIVE_PAIR_WHITE_BLACK, COLOR_BLUE, COLOR_RED);
	init_pair(HIVE_PAIR_BLACK_BLACK, COLOR_RED, COLOR_RED);
	init_pair(HIVE_PAIR_WHITE_WHITE, COLOR_BLUE, COLOR_BLUE);
	refresh();

	hive_init(&hive_game);

	while (1) {
		int c;
		MEVENT event;

		c = getch();
		if (c == 'q')
			break;
		if(all_states[cur_state].
				state(all_states[cur_state].param, c) == 1) {
			switch (c) {
			case KEY_MOUSE:
				/* need to consume this for states
				 * that don't handle mouse events
				 */
				getmouse(&event);
				break;
			}
		}

		hive_render(&hive, pad);
		prefresh(pad, pad_pos.y, pad_pos.x, 0, 0, LINES - 1, COLS - 1);
	}

	endwin();
	return 0;
}
