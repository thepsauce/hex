enum {
	HIVE_PAIR_BLACK = 1,
	HIVE_PAIR_BLACK_BLACK,
	HIVE_PAIR_BLACK_WHITE,
	HIVE_PAIR_WHITE,
	HIVE_PAIR_WHITE_BLACK,
	HIVE_PAIR_WHITE_WHITE,
	HIVE_PAIR_SELECTED,
};

enum hive_side {
	HIVE_BLACK,
	HIVE_WHITE,
};

enum hive_type {
	HIVE_ANT,
	HIVE_BEETLE,
	HIVE_GRASSHOPPER,
	HIVE_QUEEN,
	HIVE_SPIDER,
};

#define HIVE_VISITED (1 << 0)
#define HIVE_SELECTED (1 << 1)

typedef struct point {
	int x, y;
} Point;

#define point_add(p, a) ({ \
	Point *const _p = (p); \
	const Point _a = (a); \
	_p->x += _a.x; \
	_p->y += _a.y; \
})

#define point_subtract(p, a) ({ \
	Point *const _p = (p); \
	const Point _a = (a); \
	_p->x -= _a.x; \
	_p->y -= _a.y; \
})

#define point_isequal(a, b) ({ \
	const Point _a = (a); \
	const Point _b = (b); \
	_a.x == _b.x && _a.y == _b.y; \
})

void hive_movepoint(Point *point, int dir);
void hive_pointtoworld(Point *point, Point translation);
void hive_pointtogrid(Point *point, Point translation);

enum {
	HIVE_NORTH, HIVE_SOUTH,
	HIVE_NORTH_WEST, HIVE_NORTH_EAST,
	HIVE_SOUTH_WEST, HIVE_SOUTH_EAST,
	HIVE_ABOVE, HIVE_BELOW
};

#define hive_oppositedirection(d) ({ \
	int o; \
	switch (d) { \
	case HIVE_NORTH: o = HIVE_SOUTH; break; \
	case HIVE_SOUTH: o = HIVE_NORTH; break; \
	case HIVE_NORTH_WEST: o = HIVE_SOUTH_EAST; break; \
	case HIVE_NORTH_EAST: o = HIVE_SOUTH_WEST; break; \
	case HIVE_SOUTH_WEST: o = HIVE_NORTH_EAST; break; \
	case HIVE_SOUTH_EAST: o = HIVE_NORTH_WEST; break; \
	default: o = -1; \
	} \
	o; \
})

typedef struct hive_piece {
	uint64_t flags;
	enum hive_side side;
	enum hive_type type;
	Point position;
	/* this is basically chaching for higher efficiency and gets
	 * rid of an endless amount of hive_region_getsurrounding() calls
	 */
	union {
		struct {
			struct hive_piece *north, *south;
			struct hive_piece *northWest, *northEast;
			struct hive_piece *southWest, *southEast;
			struct hive_piece *above, *below;
		};
		struct hive_piece *neighbors[8];
	};
} HivePiece;

void hive_piece_render(HivePiece *piece, WINDOW *win, Point t);

typedef struct hive_region {
	WINDOW *win;
	Point translation;
	HivePiece *pieces[22];
	size_t numPieces;
} HiveRegion;

int hive_region_init(HiveRegion *region, int x, int y, int w, int h);
int hive_region_addpiece(HiveRegion *region, HivePiece *piece);
int hive_region_removepiece(HiveRegion *region, HivePiece *piece);
size_t hive_region_getsurrounding(HiveRegion *region, Point at,
		HivePiece *pieces[6]);
HivePiece *hive_region_pieceat(HiveRegion *region, Point p);
void hive_region_setposition(HiveRegion *region, int x, int y, int w, int h);
void hive_region_clearflags(HiveRegion *region, uint64_t flags);
uint32_t hive_region_count(HiveRegion *region, HivePiece *origin);
void hive_region_render(HiveRegion *region);

typedef struct hive_move {
	bool fromInventory;
	Point from;
	Point to;
} HiveMove;

typedef struct hive_move_list {
	HiveMove *moves;
	size_t count;
} HiveMoveList;

void hive_move_list_push(HiveMoveList *list, const HiveMove *move);
bool hive_move_list_contains(HiveMoveList *list, Point from, Point to);
void hive_move_list_clear(HiveMoveList *list);

typedef struct hive {
	union {
		struct {
			HiveRegion blackInventory;
			HiveRegion whiteInventory;
			HiveRegion board;
		};
		HiveRegion regions[3];
	};
	HivePiece allPieces[22];
	HivePiece *selectedPiece;
	HiveRegion *selectedRegion;
	enum hive_side turn;
	HiveMoveList moves;
	HiveMoveList history;
} Hive;

int hive_init(Hive *hive, int x, int y, int w, int h);
void hive_render(Hive *hive);
void hive_computemoves(Hive *hive);
int hive_handle(Hive *hive, int c);

