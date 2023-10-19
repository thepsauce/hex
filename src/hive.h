enum hive_side {
	HIVE_BLACK,
	HIVE_WHITE,
};

enum hive_type {
	HIVE_ANT,
	HIVE_BEETLE,
	HIVE_GRASSHOPPER,
	HIVE_LADYBUG,
	HIVE_MOSQUITO,
	HIVE_PILLBUG,
	HIVE_QUEEN,
	HIVE_SPIDER,

	/* special type for a special set of moves for pillbug that
	 * has selected a pieces */
	HIVE_PILLBUG_CARRYING,
};

#define HIVE_VISITED (1 << 0)
#define HIVE_SELECTED (1 << 1)
#define HIVE_ISACTOR (1 << 2)

#define HIVE_PIECE_COUNT 28

void hive_movepoint(Point *point, int dir);
void hive_pointtoworld(Point *point, Point translation);
void hive_pointtogrid(Point *point, Point translation);

enum {
	HIVE_NORTH, HIVE_SOUTH,
	HIVE_NORTH_EAST, HIVE_NORTH_WEST,
	HIVE_SOUTH_EAST, HIVE_SOUTH_WEST,
	HIVE_ABOVE, HIVE_BELOW
};

#define hive_oppositedirection(d) ({ \
	int o; \
	switch (d) { \
	case HIVE_NORTH: o = HIVE_SOUTH; break; \
	case HIVE_SOUTH: o = HIVE_NORTH; break; \
	case HIVE_NORTH_EAST: o = HIVE_SOUTH_WEST; break; \
	case HIVE_NORTH_WEST: o = HIVE_SOUTH_EAST; break; \
	case HIVE_SOUTH_EAST: o = HIVE_NORTH_WEST; break; \
	case HIVE_SOUTH_WEST: o = HIVE_NORTH_EAST; break; \
	default: o = -1; \
	} \
	o; \
})

typedef struct hive_piece {
	uint64_t flags;
	enum hive_side side;
	enum hive_type type;
	Point position;
} HivePiece;

typedef struct hive_region {
	WINDOW *win;
	Point translation;
	HivePiece *pieces[HIVE_PIECE_COUNT];
	size_t numPieces;
} HiveRegion;

int hive_region_init(HiveRegion *region, int x, int y, int w, int h);
int hive_region_addpiece(HiveRegion *region, HivePiece *piece);
int hive_region_removepiece(HiveRegion *region, HivePiece *piece);

/* the little 'r' stands for "reverse" */
HivePiece *hive_region_pieceat(HiveRegion *region, HivePiece *from, Point at);
HivePiece *hive_region_pieceatr(HiveRegion *region, HivePiece *from, Point at);
size_t hive_region_getsurrounding(HiveRegion *region, Point at,
		HivePiece *pieces[6]);
size_t hive_region_getsurroundingr(HiveRegion *region, Point at,
		HivePiece *pieces[6]);
size_t hive_region_countat(HiveRegion *region, Point at);
#define hive_region_getabove(region, piece) ({ \
	HivePiece *const _piece = (piece); \
	HivePiece *const _p = hive_region_pieceat(region, _piece, _piece->position); \
	_p; \
})
#define hive_region_getbelow(region, piece) ({ \
	HivePiece *const _piece = (piece); \
	HivePiece *const _p = hive_region_pieceatr(region, _piece, _piece->position); \
	_p; \
})

void hive_region_setposition(HiveRegion *region, int x, int y, int w, int h);
void hive_region_clearflags(HiveRegion *region, uint64_t flags);
uint32_t hive_region_count(HiveRegion *region, HivePiece *origin);
void hive_region_renderpieceat(HiveRegion *region, HivePiece *piece,
		size_t cnt, Point at);
void hive_region_renderpiece(HiveRegion *region, HivePiece *piece);
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
bool hive_move_list_contains(const HiveMoveList *list, Point from, Point to);
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
	HivePiece allPieces[HIVE_PIECE_COUNT];
	HivePiece *actor;
	HivePiece *selectedPiece;
	HiveRegion *selectedRegion;
	enum hive_side turn;
	PointList moves;
	PointList choices;
	HiveMoveList history;
} Hive;

int hive_init(Hive *hive, int x, int y, int w, int h);
void hive_domove(Hive *hive, const HiveMove *move, bool doNotify);
void hive_render(Hive *hive);
void hive_computemoves(Hive *hive, enum hive_type type);
bool hive_handlemousepress(Hive *hive, int button, Point mouse);
int hive_handle(Hive *hive, int c);

