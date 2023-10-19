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
	curses_init();

	hc_init(&hive_chat);
	while (1) {
		MEVENT ev;

		net_chat_render(chat);
		hive_render(hive);
		doupdate();
		/* separator line */
		attr_set(0, 0, NULL);
		for (int y = 0; y < LINES - 1; y++)
			mvaddch(y, COLS / 2 - 1, '|');
		if (inChat) {
			int xBeg, yBeg;
			int x, y;

			getbegyx(chat->win, yBeg, xBeg);
			getyx(chat->win, y, x);
			move(y + yBeg, x + xBeg);
		}
		const int c = getch();
		switch (c) {
		case 'W' - 'A' + 1:
			prefixed = true;
			break;
		case KEY_MOUSE:
			getmouse(&ev);
			if (ev.bstate & BUTTON1_PRESSED) {
				inChat = net_chat_handlemousepress(chat,
						(Point) { ev.x, ev.y });
				curs_set(inChat);
				hive_handlemousepress(hive, 0,
						(Point) { ev.x, ev.y });
			} else if (ev.bstate & BUTTON2_PRESSED) {
				hive_handlemousepress(hive, 1,
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

