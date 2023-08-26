#include <curses.h>
#include <locale.h>
#include <assert.h>
#include <string.h>

enum species {
	QUEEN,
	BEETLE,
	GRASSHOPPER,
	SPIDER,
	ANT,
	LADYBUG,
	MOSQUITO,
	PILLBUG,
	SPECIES_COUNT
};

enum direction {
	SOUTH_EAST,
	NORTH_EAST,
	NORTH,
	NORTH_WEST,
	SOUTH_WEST,
	SOUTH,
	DIRECTION_COUNT
};

enum piece {
    WQ1,
    WB1,
    WB2,
    WG1,
    WG2,
    WG3,
    WS1,
    WS2,
    WA1,
    WA2,
    WA3,
    WL1,
    WM1,
    WP1,
    BQ1,
    BB1,
    BB2,
    BG1,
    BG2,
    BG3,
    BS1,
    BS2,
    BA1,
    BA2,
    BA3,
    BL1,
    BM1,
    BP1,
    PIECE_COUNT
};

enum color {
	WHITE,
	BLACK
};

struct hex {
	int x;
	int y;
	int z;
};

#define GRID_COLUMNS 127
#define GRID_ROWS 127
#define GRID_LAYERS 127
#define GRID_SPACES (GRID_COLUMNS * GRID_ROWS)

int grid[GRID_COLUMNS * GRID_ROWS * GRID_LAYERS];
struct hex positions[PIECE_COUNT];

struct hex offsets[2][DIRECTION_COUNT] = {
	{
    	{+1,  0}, {+1, -1}, { 0, -1}, 
    	{-1, -1}, {-1,  0}, { 0, +1}
   	},
    {
    	{+1, +1}, {+1,  0}, { 0, -1}, 
    	{-1,  0}, {-1, +1}, { 0, +1}
    }
};

char letter[] = {
	'Q',
	'B',
	'G',
	'S',
	'A',
	'L',
	'M',
	'P'
};

int piece_species[] = {
    QUEEN,
    BEETLE,
    BEETLE,
    GRASSHOPPER,
    GRASSHOPPER,
    GRASSHOPPER,
    SPIDER,
    SPIDER,
    ANT,
    ANT,
    ANT,
    LADYBUG,
    MOSQUITO,
    PILLBUG,
    QUEEN,
    BEETLE,
    BEETLE,
    GRASSHOPPER,
    GRASSHOPPER,
    GRASSHOPPER,
    SPIDER,
    SPIDER,
    ANT,
    ANT,
    ANT,
    LADYBUG,
    MOSQUITO,
    PILLBUG
};

int triangles[] = {
	L'\x25E2',
	L'\x25E3',
	L'\x25E4',
	L'\x25E5'
};

int adjacent[8][8];

struct cell {
	int x;
	int y;
};

#define PARITY(n) (n % 2)

void print(void) {
	int to_direction[] = {
		NORTH_WEST,
		NORTH_EAST,
		SOUTH_EAST,
		SOUTH_WEST
	};
	struct cell cell_offsets[] = {
		{0, 0},
		{4, 0},
		{4, 1},
		{0, 1}
	};
	for (int j = 0; j < GRID_ROWS; ++j) {
		for (int i = 0; i < GRID_COLUMNS; ++i) {
			int parity;
			int color;
			struct hex position;
			struct cell start_cell;
			int piece;
			int opponent;
			position.x = i;
			position.y = j;
			parity = PARITY(i);
			start_cell.x = position.x * 4;
			start_cell.y = position.y * 2 + parity;

			piece = grid[i + j * GRID_COLUMNS];

			if (piece > PIECE_COUNT - 1) {
				continue;
			}

			color = piece < BQ1 ? COLOR_WHITE : COLOR_BLACK;
			opponent = piece < BQ1 ? COLOR_BLACK : COLOR_WHITE;

			attron(COLOR_PAIR(adjacent[opponent][color]));
			mvprintw(start_cell.y, start_cell.x + 1, " %c ", letter[piece_species[piece]]);
			mvprintw(start_cell.y + 1, start_cell.x + 1, "%s", "   ");

			for (int n = 0; n < 4; ++n) {
				int direction = to_direction[n];
				int piece;
				struct hex offset = offsets[parity][direction];
				struct hex neighbor;
				int neighbor_color;
				struct cell cell_offset;
				struct cell cell;
				neighbor.x = position.x + offset.x;
				neighbor.y = position.y + offset.y;
				piece = grid[neighbor.x + neighbor.y * GRID_COLUMNS];
				if (piece < BQ1) {
					neighbor_color = COLOR_WHITE;
				}
				else if (piece < PIECE_COUNT) {
					neighbor_color = COLOR_BLACK;
				}
				else {
					neighbor_color = COLOR_BLUE;
				}

				attron(COLOR_PAIR(adjacent[color][neighbor_color]));
				cell_offset = cell_offsets[n];
				cell.x = start_cell.x + cell_offset.x;
				cell.y = start_cell.y + cell_offset.y;
				mvprintw(cell.y, cell.x, "%lc", triangles[n]);
			}
		}
	}
}

int main(int argc, char *argv[]) {
	int parity;
	struct hex position;
	struct hex offset;
	int direction;
	struct hex neighbor;
	setlocale(LC_ALL, "");
	initscr();
	curs_set(FALSE);
	keypad(stdscr, TRUE);
	// nodelay(stdscr, TRUE);
	for (int i = 0; i < GRID_SPACES; ++i) {
		grid[i] = 255;
	}
	position.x = 2;
	position.y = 2;

	direction = SOUTH_WEST;

	parity = PARITY(position.x);

	offset = offsets[parity][direction];

	neighbor.x = position.x + offset.x;
	neighbor.y = position.y + offset.y;

	grid[position.x + position.y * GRID_COLUMNS] = BQ1;
	grid[neighbor.x + neighbor.y * GRID_COLUMNS] = WQ1;

	cbreak();
    noecho();
	mousemask(ALL_MOUSE_EVENTS, NULL);
	start_color();
	for (int i = 0; i < 8; ++i) {
		for (int j = 0; j < 8; ++j) {
			adjacent[i][j] = j + i * 8 + 1;
			init_pair(adjacent[i][j], i, j);
		}
	}
	bkgd(COLOR_PAIR(adjacent[4][4]));
	print();
	refresh();
	getch();
	endwin();
	return 0;
}
