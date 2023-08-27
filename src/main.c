#include "hive.h"

static const enum hive_type default_inventory[HIVE_INVENTORY_SIZE] = {
	HIVE_QUEEN,
	HIVE_BEETLE,
	HIVE_BEETLE,
	HIVE_GRASSHOPPER,
	HIVE_GRASSHOPPER,
	HIVE_GRASSHOPPER,
	HIVE_SPIDER,
	HIVE_SPIDER,
	HIVE_ANT,
	HIVE_ANT,
	HIVE_ANT,
	HIVE_LADYBUG,
	HIVE_MOSQUITO,
	HIVE_PILLBUG,
	HIVE_QUEEN,
	HIVE_BEETLE,
	HIVE_BEETLE,
	HIVE_GRASSHOPPER,
	HIVE_GRASSHOPPER,
	HIVE_GRASSHOPPER,
	HIVE_SPIDER,
	HIVE_SPIDER,
	HIVE_ANT,
	HIVE_ANT,
	HIVE_ANT,
	HIVE_LADYBUG,
	HIVE_MOSQUITO,
	HIVE_PILLBUG
};

int main(int argc, char *argv[])
{
	struct hive hive;

	setlocale(LC_ALL, "");

	initscr();
	curs_set(FALSE);
	keypad(stdscr, TRUE);
	cbreak();
	noecho();
	mousemask(ALL_MOUSE_EVENTS, NULL);
	start_color();
	// nodelay(stdscr, TRUE);
	wtimeout(stdscr, 10);

	memset(&hive, 0, sizeof(hive));
	hive.turn = HIVE_BLACK;
	memcpy(&hive.blackInventory,
		default_inventory, sizeof(default_inventory));
	memcpy(&hive.whiteInventory,
		default_inventory, sizeof(default_inventory));

	/* test setup */
	struct vec3 pos = { 6, 6 };
	for (int i = 0; i < 20; i++) {
		const enum hive_type t = 1 + rand() % (HIVE_SPECIES - 1);
		const enum hive_side s = (1 + rand() % 2) << 4;
		hive.grid[pos.x + pos.y * GRID_COLUMNS] = t | s;
		vec_move(&pos, rand() % 6, &pos);
	}

	init_pair(HIVE_PAIR_BLACK, COLOR_BLACK, COLOR_RED);
	init_pair(HIVE_PAIR_WHITE, COLOR_BLACK, COLOR_BLUE);
	init_pair(HIVE_PAIR_BLACK_WHITE, COLOR_BLUE, COLOR_RED);
	init_pair(HIVE_PAIR_WHITE_BLACK, COLOR_RED, COLOR_BLUE);
	init_pair(HIVE_PAIR_BLACK_BLACK, COLOR_RED, COLOR_RED);
	init_pair(HIVE_PAIR_WHITE_WHITE, COLOR_BLUE, COLOR_BLUE);

	struct vec3 pad_max;
	struct vec3 pad_pos;
	struct vec3 hex_pos;
	int piece = 0;

	pad_pos.x = 0;
	pad_pos.y = 0;

	pad_max.x = GRID_COLUMNS * 4;
	pad_max.y = GRID_ROWS * 2;
	WINDOW *pad = newpad(pad_max.y, pad_max.x);

	while (1) {
		int c;
		MEVENT event;

		werase(pad);

		c = getch();
		if (c == 'q')
			break;
		if (c == KEY_MOUSE) {
			getmouse(&event);
			if (event.bstate & BUTTON1_CLICKED || event.bstate & BUTTON1_PRESSED) {
				struct vec3 pos;
				pos.y = event.y;
				pos.x = event.x;
				wmouse_trafo(pad, &pos.y, &pos.x, FALSE);
				pos.y = pos.y + pad_pos.y;
				pos.x = pos.x + pad_pos.x;
				pos.x = pos.x / 4;
				pos.y = (pos.y - (pos.x % 2)) / 2;
				if (pos.x != hex_pos.x || pos.y != hex_pos.y) {
					if (piece) {
						hive.grid[pos.x + pos.y * GRID_COLUMNS] = piece;
						piece = 0;
						hive.grid[hex_pos.x + hex_pos.y * GRID_COLUMNS] = 0;
					}
					else {
						hex_pos = pos;
						piece = hive.grid[pos.x + pos.y * GRID_COLUMNS];
					}
				}
			}
		}

		if (c == 'd') {
			if (pad_pos.x < pad_max.x)
				pad_pos.x = pad_pos.x + 1;
		}
		if (c == 'a') {
			if (pad_pos.x > 0)
				pad_pos.x = pad_pos.x - 1;
		}
		if (c == 'w') {
			if (pad_pos.y > 0)
				pad_pos.y = pad_pos.y - 1;
		}
		if (c == 's') {
			if (pad_pos.y < pad_max.y)
				pad_pos.y = pad_pos.y + 1;
		}

		hive_render(&hive, pad);
		prefresh(pad, pad_pos.y, pad_pos.x, 0, 0, LINES - 1, COLS - 1);
	}

	endwin();
	return 0;
}
