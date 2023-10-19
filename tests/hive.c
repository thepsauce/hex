#include "test.h"

HiveChat hive_chat;

bool act_on_piece(Hive *hive, int c)
{
	static const struct {
		int key;
	} keys[] = {
		{ 'f' },
		{ 's' },
		{ 'w' },
		{ 'p' },
		{ 'r' },
		{ 't' },
	};

	HivePiece *const piece = hive->selectedPiece;
	if (piece == NULL)
		return false;
	switch (c) {
	default: {
		int d;
		for (d = 0; d < (int) ARRLEN(keys); d++)
			if (keys[d].key == c)
				break;
		if (d == (int) ARRLEN(keys))
			return false;
		hive_movepoint(&piece->position, d);
		break;
	}
	case KEY_LEFT:
		piece->position.x--;
		break;
	case KEY_UP:
		piece->position.y--;
		break;
	case KEY_RIGHT:
		piece->position.x++;
		break;
	case KEY_DOWN:
		piece->position.y++;
		break;
	case '+':
		if (piece->type == 7)
			piece->type = 0;
		else
			piece->type++;
		break;
	case '-':
		if (piece->type == 0)
			piece->type = 7;
		else
			piece->type--;
		break;
	case '0':
		piece->type = 0;
		break;
	case '/':
		piece->side = piece->side == HIVE_BLACK ? HIVE_WHITE :
			HIVE_BLACK;
		break;
	}
	hive_computemoves(hive, piece->type);
	return true;
}

void show_surround(Hive *hive)
{
	int xMax, yMax;

	getmaxyx(hive->board.win, yMax, xMax);
	wattrset(hive->board.win, 0);
	for (int x = 0; x <= xMax / 4; x++)
		for (int y = 0; y <= yMax / 2; y++) {
			Point p = { x, y };
			HivePiece *pieces[6];
			const size_t c = hive_region_getsurrounding(
					&hive->board, p, pieces);
			hive_pointtoworld(&p, hive->board.translation);
			mvwprintw(hive->board.win, p.y + 1, p.x + 2, "%zu", c);
		}
	wnoutrefresh(hive->board.win);
}

int main(void)
{
	curses_init();
	curs_set(0);
	hc_init(&hive_chat);

	Hive *const hive = &hive_chat.hive;
	enum hive_type type = HIVE_QUEEN;
	bool showSurround = false;
	const char *names[] = {
		"Ant", "Beetle", "Grasshopper",
		"Ladybug", "Mosquito", "Pillbug",
		"Queen", "Spider"
	};
	while (1) {
		MEVENT ev;
		static const char *infoText =
			"Middle mouse - Place piece\n"
			"Right mouse - Remove piece\n"
			"1 to 8 - Select piece type\n"
			"While a piece is selected:\n"
			"LEFT, UP, RIGHT, DOWN, f, s, w, p, r, t - Move the piece\n"
			"\t+ - Increment piece type\n"
			"\t- - Decrement piece type\n"
			"\t0 - Set piece type to ant\n"
			"\t/ - Swap side\n"
			". - Swap turn\n"
			"c - Toggle surround view\n";
		/* abusing the chat window to display info text */
		WINDOW *const info = hive_chat.chat.win;

		hive_render(hive);
		werase(info);
		mvwaddstr(info, 0, 0, infoText);
		wnoutrefresh(info);
		hive_render(hive);
		if (showSurround)
			show_surround(hive);
		doupdate();
		attr_set(0, 0, NULL);
		mvprintw(LINES - 1, 0, "Placing: %s", names[type]);
		clrtoeol();
		const int c = getch();
		if (hive->selectedPiece != NULL &&
				act_on_piece(hive, c))
			continue;
		switch (c) {
		default:
			hive_handle(hive, c);
			break;
		case '1' ... '8':
			type = c - '1';
			break;
		case '.':
			hive->turn = hive->turn == HIVE_WHITE ? HIVE_BLACK :
				HIVE_WHITE;
			break;
		case 'c':
			showSurround = !showSurround;
			break;
		case KEY_MOUSE: {
			Point p;

			getmouse(&ev);
			p = (Point) { ev.x, ev.y };
			if (ev.bstate & BUTTON1_PRESSED) {
				hive_handlemousepress(hive, 0, p);
			} else {
				if (wmouse_trafo(hive->board.win, &p.y, &p.x, false)) {
					hive_pointtogrid(&p, hive->board.translation);
					if (ev.bstate & BUTTON2_PRESSED) {
						HivePiece *const piece = malloc(sizeof(*piece));
						if (piece == NULL)
							break;
						piece->flags = 0;
						piece->side = hive->turn;
						piece->type = type;
						piece->position = p;
						hive_region_addpiece(&hive->board, piece);
					} else if (ev.bstate & BUTTON3_PRESSED) {
						hive_region_removepiece(&hive->board,
							hive_region_pieceatr(&hive->board, NULL, p));
					}
				}
			}
			break;
		}
		}
	}

	endwin();
	return 0;
}
