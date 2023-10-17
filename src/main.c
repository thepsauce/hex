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

HiveChat hive_chat;

int main(int argc, char *argv[])
{
	Hive *const hive = &hive_chat.hive;
	NetChat *const chat = &hive_chat.chat;
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
	init_pair(HIVE_PAIR_CHOICE, COLOR_GREEN, COLOR_BLACK);

	refresh();

	hc_init(&hive_chat);
	while (1) {
		MEVENT ev;

		net_chat_render(chat);
		hive_render(hive);
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
				inChat = net_chat_handlemousepress(chat,
						(Point) { ev.x, ev.y });
				curs_set(inChat);
				hive_handlemousepress(hive,
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
				net_chat_handle(chat, c);
			else
				hive_handle(hive, c);
		}
		}
	}

	endwin();
	return 0;
}

