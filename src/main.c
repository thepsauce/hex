#include "hex.h"

/*  ___     ___
 * /   \   /   \
 * \___/   \___/
 * /   \___/   \
 * \___/   \___/
 * /   \___/   \
 * \___/   \___/
 * /   \   /   \
 * \___/   \___/
 */

Hive hive_game;

NetChat chat_term;

int main(int argc, char *argv[])
{
	bool inChat = true;
	bool prefixed = false;

	(void) argc;
	(void) argv;

	setlocale(LC_ALL, "");

	initscr();
	keypad(stdscr, true);
	cbreak();
	noecho();
	mousemask(ALL_MOUSE_EVENTS, NULL);
	mouseinterval(0);
	timeout(10);

	start_color();
	init_pair(PAIR_NORMAL, COLOR_WHITE, COLOR_BLACK);
	init_pair(PAIR_ERROR, COLOR_RED, COLOR_BLACK);
	init_pair(PAIR_INFO, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(PAIR_COMMAND, COLOR_BLUE, COLOR_BLACK);
	init_pair(PAIR_ARGUMENT, COLOR_GREEN, COLOR_BLACK);

	init_pair(HIVE_PAIR_BLACK, COLOR_RED, COLOR_BLACK);
	init_pair(HIVE_PAIR_WHITE, COLOR_BLUE, COLOR_BLACK);
	init_pair(HIVE_PAIR_BLACK_WHITE, COLOR_RED, COLOR_BLUE);
	init_pair(HIVE_PAIR_WHITE_BLACK, COLOR_BLUE, COLOR_RED);
	init_pair(HIVE_PAIR_BLACK_BLACK, COLOR_RED, COLOR_RED);
	init_pair(HIVE_PAIR_WHITE_WHITE, COLOR_BLUE, COLOR_BLUE);
	init_pair(HIVE_PAIR_SELECTED, COLOR_YELLOW, COLOR_BLACK);

	refresh();

	if (hive_init(&hive_game, 0, 0, COLS / 2 - 1, LINES) < 0) {
		endwin();
		perror("hive_init");
		return -1;
	}

	if (net_chat_init(&chat_term, COLS / 2, 0, COLS - COLS / 2, LINES, 10000) < 0) {
		endwin();
		perror("chat_init");
		return -1;
	}
	while (1) {
		MEVENT ev;

		net_chat_render(&chat_term);
		hive_render(&hive_game);
		doupdate();
		/* separator line */
		for (int y = 0; y < LINES; y++)
			mvaddch(y, COLS / 2 - 1, '|');
		const int c = getch();
		switch (c) {
		case 'W' - 'A' + 1:
			prefixed = true;
			break;
		case KEY_MOUSE:
			getmouse(&ev);
			if ((ev.bstate & BUTTON1_CLICKED) ||
					(ev.bstate & BUTTON1_PRESSED)) {
				inChat = net_chat_handlemousepress(&chat_term,
						(Point) { ev.x, ev.y });
				curs_set(inChat);
				hive_handlemousepress(&hive_game,
						(Point) { ev.x, ev.y });
			}
			break;
		default: {
			const bool p = prefixed;
			prefixed = false;
			if (p) {
				switch (c) {
				case 'w': case 'W':
					inChat = !inChat;
					break;
				}
				break;
			}
			if (inChat)
				net_chat_handle(&chat_term, c);
			else
				hive_handle(&hive_game, c);
		}
		}
	}

	endwin();
	return 0;
}

