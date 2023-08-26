#ifndef INCLUDED_HIVE_H
#define INCLUDED_HIVE_H

#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#define GRID_COLUMNS 127
#define GRID_ROWS 127
#define GRID_LAYERS 127
#define GRID_SPACES (GRID_COLUMNS*GRID_ROWS)

#define HIVE_INVENTORY_SIZE 28

#define HIVE_SPECIES_MASK	0x0f
#define HIVE_SIDE_MASK		0x30

enum hive_color_pair {
	HIVE_PAIR_BLACK = 1,
	HIVE_PAIR_WHITE = 2,
	HIVE_PAIR_BLACK_WHITE = 3,
	HIVE_PAIR_WHITE_BLACK = 4,
	HIVE_PAIR_BLACK_BLACK = 5,
	HIVE_PAIR_WHITE_WHITE = 6,
};

struct vec3 {
	int x, y, z;
};

enum hive_direction {
	HIVE_SOUTH_EAST,
	HIVE_NORTH_EAST,
	HIVE_NORTH,
	HIVE_NORTH_WEST,
	HIVE_SOUTH_WEST,
	HIVE_SOUTH,
};

enum hive_type {
	HIVE_EMPTY		= 0x00,
	HIVE_QUEEN		= 0x01,
	HIVE_BEETLE		= 0x02,
	HIVE_GRASSHOPPER	= 0x03,
	HIVE_SPIDER		= 0x04,
	HIVE_ANT		= 0x05,
	HIVE_LADYBUG		= 0x06,
	HIVE_MOSQUITO		= 0x07,
	HIVE_PILLBUG		= 0x08,
	HIVE_SPECIES
};

enum hive_side {
	HIVE_BLACK = 0x10,
	HIVE_WHITE = 0x20,
};

struct hive {
	enum hive_side turn;
	enum hive_type whiteInventory[HIVE_INVENTORY_SIZE];
	enum hive_type blackInventory[HIVE_INVENTORY_SIZE];
	int grid[GRID_COLUMNS * GRID_ROWS * GRID_LAYERS];
};

void vec_move(const struct vec3 *vec, enum hive_direction dir,
		struct vec3 *dest);
enum hive_direction vec_getdir(const struct vec3 *a, const struct vec3 *b);

void hive_render(struct hive *hive, WINDOW *win);

#endif
