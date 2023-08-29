#include "hive.h"

struct hive hive_game;

struct chat chat_term;

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

	if (hive_init(&hive_game) < 0) {
		endwin();
		perror("hive_init");
		return -1;
	}

	if (chat_init(&chat_term, 0, 0, 30, LINES) < 0) {
		endwin();
		perror("chat_init");
		return -1;
	}
	while (1) {
		int c;
		MEVENT event;

		c = getch();
		if (c == 'q')
			break;
		if (c == -1) {
			chat_update(&chat_term);
			continue;
		}
		chat_handle(&chat_term, c);
		continue;
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
	}

	endwin();
	return 0;
}

