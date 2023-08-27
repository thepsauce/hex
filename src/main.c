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

	memset(&hive, 0, sizeof(hive));
	hive.turn = HIVE_BLACK;
	memcpy(&hive.blackInventory,
		default_inventory, sizeof(default_inventory));
	memcpy(&hive.whiteInventory,
		default_inventory, sizeof(default_inventory));

	/* test setup */
	struct vec3 pos = { 6, 6, 0 };
	for (int i = 0; i < 20; i++) {
		const enum hive_type t = 1 + rand() % (HIVE_TYPES - 1);
		const enum hive_side s = (1 + rand() % 2) << 4;
		hive.grid[pos.x + pos.y * GRID_COLUMNS] = t | s;
		pos = vec_move(&pos, rand() % 6);
	}

	init_pair(HIVE_PAIR_BLACK, COLOR_RED, COLOR_BLACK);
	init_pair(HIVE_PAIR_WHITE, COLOR_BLUE, COLOR_BLACK);
	init_pair(HIVE_PAIR_BLACK_WHITE, COLOR_RED, COLOR_BLUE);
	init_pair(HIVE_PAIR_WHITE_BLACK, COLOR_BLUE, COLOR_RED);
	init_pair(HIVE_PAIR_BLACK_BLACK, COLOR_RED, COLOR_RED);
	init_pair(HIVE_PAIR_WHITE_WHITE, COLOR_BLUE, COLOR_BLUE);

	/* TODO: clean this up */
	struct vec3 pad_max;
	struct vec3 pad_pos;

	pad_pos.x = 0;
	pad_pos.y = 0;

	pad_max.x = GRID_COLUMNS * 4;
	pad_max.y = GRID_ROWS * 2;
	WINDOW *pad = newpad(pad_max.y, pad_max.x);

	while (1) {
		int c;
		MEVENT event;

		/* TODO: figure out why this does not draw the first time */
		werase(pad);
		hive_render(&hive, pad);
		prefresh(pad, pad_pos.y, pad_pos.x, 0, 0, LINES - 1, COLS - 1);

		c = getch();
		if (c == 'q')
			break;
		switch(c) {
		case KEY_MOUSE:
			getmouse(&event);
			if (event.bstate & BUTTON1_CLICKED ||
					event.bstate & BUTTON1_PRESSED) {
				struct vec3 pos;

				pos.x = event.x;
				pos.y = event.y;
				wmouse_trafo(pad, &pos.y, &pos.x, FALSE);

				pos.x = pos.x + pad_pos.x;
				pos.y = pos.y + pad_pos.y;
				hive_handlemousepress(&hive, &pos);
			}
			break;
		case 'd':
			pad_pos.x = pad_pos.x < pad_max.x ?
				pad_pos.x + 1 : pad_pos.x;
			break;
		case 'a':
			pad_pos.x = pad_pos.x > 0 ?
				pad_pos.x - 1 : pad_pos.x;
			break;
		case 'w':
			pad_pos.y = pad_pos.y > 0 ?
				pad_pos.y - 1 : pad_pos.y;
			break;
		case 's':
			pad_pos.y = pad_pos.y < pad_max.y ?
				pad_pos.y + 1 : pad_pos.y;
			break;
		}
	}

	endwin();
	return 0;
}
