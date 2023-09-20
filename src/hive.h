#define HIVE_GRID_COLUMNS 127
#define HIVE_GRID_ROWS 127
#define HIVE_GRID_SPACES (HIVE_GRID_COLUMNS * HIVE_GRID_ROWS)

#define HIVE_INVENTORY_SIZE 28

#define HIVE_STACK_SIZE 6

#define HIVE_TYPE_MASK	0x0f
#define HIVE_SIDE_MASK	0x30
#define HIVE_STACK_MASK	0xc0
#define HIVE_PIECE_MASK 0x3f

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
	HIVE_DIRECTION_COUNT
};

typedef char piece_t;

#define HIVE_EMPTY 0x00
#define HIVE_QUEEN 0x01
#define HIVE_BEETLE 0x02
#define HIVE_GRASSHOPPER 0x03
#define HIVE_SPIDER 0x04
#define HIVE_ANT 0x05
#define HIVE_TYPES 0x06
#define HIVE_LADYBUG 0x07
#define HIVE_MOSQUITO 0x08
#define HIVE_PILLBUG 0x09

#define HIVE_BLACK 0x10
#define HIVE_WHITE 0x20

#define HIVE_ABOVE 0x40
/* Temporary flag */
#define HIVE_VISIT 0x80

struct state {
	int (*state)(void *param, int c);
	void *param;
};

struct window {
	WINDOW *win;
	struct state *state;
};

struct move {
	struct vec3 startPos;
	struct vec3 endPos;
	piece_t piece;
};

struct hive {
	WINDOW *win;
	struct vec3 winPos;
	struct vec3 winSize;
	piece_t turn;
	piece_t whiteInventory[HIVE_INVENTORY_SIZE];
	piece_t blackInventory[HIVE_INVENTORY_SIZE];
	piece_t selectedPiece;
	struct move pendingMove;
	int piecesPlayed;
	struct move *validMoves;
	piece_t grid[HIVE_GRID_COLUMNS * HIVE_GRID_ROWS];
	bool playMove;
	struct {
		struct vec3 pos;
		piece_t piece;
	} stacks[HIVE_STACK_SIZE];
	int stackSz;
};

struct vec3 vec_move(const struct vec3 *vec, enum hive_direction dir);
enum hive_direction vec_getdir(const struct vec3 *a, const struct vec3 *b);

int hive_init(struct hive *hive);
void hive_render(struct hive *hive);
int hive_handle(struct hive *hive, int c);

piece_t hive_getabove(struct hive *hive, struct vec3 *pos);
piece_t hive_getexposedpiece(struct hive *hive, struct vec3 *pos);
bool hive_onehive(struct hive *hive, const struct vec3 *startPos);

bool hive_playmove(struct hive *hive, struct move *move);
void hive_movesforant(struct hive *hive, const struct vec3 *startPos);
void hive_movesforbeetle(struct hive *hive, const struct vec3 *startPos);
void hive_movesforspider(struct hive *hive, const struct vec3 *startPos);
void hive_movesforgrasshopper(struct hive *hive, const struct vec3 *startPos);
void hive_movesforqueen(struct hive *hive, const struct vec3 *startPos);
void hive_generatemoves(struct hive *hive);
bool hive_canplace(struct hive *hive, const struct vec3 *pos, piece_t piece);

