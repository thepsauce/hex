#ifndef INCLUDED_HIVE_H
#define INCLUDED_HIVE_H

#include <assert.h>
#include <curses.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#define GRID_COLUMNS 127
#define GRID_ROWS 127
#define GRID_SPACES (GRID_COLUMNS * GRID_ROWS)

#define HIVE_INVENTORY_SIZE 28

#define HIVE_STACK_SIZE 6

#define HIVE_TYPE_MASK	0x0f
#define HIVE_SIDE_MASK	0x30
#define HIVE_STACK_MASK	0xc0

/* These macros give normalized results, do not compare
 * HIVE_GETNSIDE with HIVE_WHITE or HIVE_BLACK.
 * Instead use HIVE_GETSIDE to get the real value.
 */
#define HIVE_GETNTYPE(p) ((p) & HIVE_TYPE_MASK)
#define HIVE_GETNSIDE(p) (((p) & HIVE_SIDE_MASK) >> 4)

#define HIVE_GETTYPE(p) ((p) & HIVE_TYPE_MASK)
#define HIVE_GETSIDE(p) ((p) & HIVE_SIDE_MASK)

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

typedef char piece_t;

enum hive_type {
	HIVE_EMPTY,
	HIVE_QUEEN,
	HIVE_BEETLE,
	HIVE_GRASSHOPPER,
	HIVE_SPIDER,
	HIVE_ANT,
	HIVE_LADYBUG,
	HIVE_MOSQUITO,
	HIVE_PILLBUG,
	HIVE_TYPES,
};

enum hive_side {
	HIVE_BLACK = 0x10,
	HIVE_WHITE = 0x20,
};

enum hive_stack {
	HIVE_ABOVE = 0x40,
	HIVE_BELOW = 0x80,
};

struct hive {
	enum hive_side turn;
	enum hive_type whiteInventory[HIVE_INVENTORY_SIZE];
	enum hive_type blackInventory[HIVE_INVENTORY_SIZE];
	piece_t selectedPiece;
	struct vec3 selectedPos;
	piece_t grid[GRID_COLUMNS * GRID_ROWS];
	struct {
		struct vec3 pos;
		piece_t above;
	} stacks[HIVE_STACK_SIZE];
};

struct vec3 vec_move(const struct vec3 *vec, enum hive_direction dir);
enum hive_direction vec_getdir(const struct vec3 *a, const struct vec3 *b);

piece_t hive_getexposedpiece(struct hive *hive, struct vec3 *pos);
piece_t hive_getabove(struct hive *hive, const struct vec3 *pos);
void hive_handlemousepress(struct hive *hive, const struct vec3 *mp);
void hive_render(struct hive *hive, WINDOW *win);

#endif
