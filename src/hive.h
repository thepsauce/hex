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

#define HIVE_PAIR_BLACK 1
#define HIVE_PAIR_WHITE 2
#define HIVE_PAIR_BLACK_WHITE 3
#define HIVE_PAIR_WHITE_BLACK 4
#define HIVE_PAIR_BLACK_BLACK 5
#define HIVE_PAIR_WHITE_WHITE 6

typedef struct vec3 {
	int x, y, z;
} Vec3;

typedef enum hive_direction {
	HIVE_SOUTH_EAST,
	HIVE_NORTH_EAST,
	HIVE_NORTH,
	HIVE_NORTH_WEST,
	HIVE_SOUTH_WEST,
	HIVE_SOUTH,
	HIVE_DIRECTION_COUNT
} hive_direction_t;

typedef char hive_piece_t;

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

typedef struct move {
	Vec3 startPos;
	Vec3 endPos;
	hive_piece_t piece;
} Move;

typedef struct hive {
	WINDOW *win;
	Vec3 winPos;
	Vec3 winSize;
	hive_piece_t turn;
	hive_piece_t whiteInventory[HIVE_INVENTORY_SIZE];
	hive_piece_t blackInventory[HIVE_INVENTORY_SIZE];
	hive_piece_t selectedPiece;
	Move pendingMove;
	int piecesPlayed;
	Move *validMoves;
	hive_piece_t grid[HIVE_GRID_COLUMNS * HIVE_GRID_ROWS];
	bool playMove;
	struct {
		Vec3 pos;
		hive_piece_t piece;
	} stacks[HIVE_STACK_SIZE];
	int stackSz;
} Hive;

Vec3 vec_move(const Vec3 *vec, hive_direction_t dir);
hive_direction_t vec_getdir(const Vec3 *a, const Vec3 *b);

int hive_init(Hive *hive);
void hive_render(Hive *hive);
int hive_handle(Hive *hive, int c);

hive_piece_t hive_getabove(Hive *hive, Vec3 *pos);
hive_piece_t hive_getexposedpiece(Hive *hive, Vec3 *pos);
bool hive_onehive(Hive *hive, const Vec3 *startPos);

bool hive_playmove(Hive *hive, Move *move);
void hive_movesforant(Hive *hive, const Vec3 *startPos);
void hive_movesforbeetle(Hive *hive, const Vec3 *startPos);
void hive_movesforspider(Hive *hive, const Vec3 *startPos);
void hive_movesforgrasshopper(Hive *hive, const Vec3 *startPos);
void hive_movesforqueen(Hive *hive, const Vec3 *startPos);
void hive_generatemoves(Hive *hive);

bool hive_canplace(Hive *hive, const Vec3 *pos, hive_piece_t piece);

