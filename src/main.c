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
	//nodelay(stdscr, TRUE);

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

	hive_render(&hive, stdscr);

	while (1) {
		int c;

		c = getch();
		if (c == 'q')
			break;
	}

	endwin();
	return 0;
}
