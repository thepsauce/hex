#include <hive.h>
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#define HIVE_DIRECTION_BIT(dir) (1 << (dir))

#define HIVE_SOUTH_EAST_BIT	HIVE_DIRECTION_BIT(HIVE_SOUTH_EAST)
#define HIVE_NORTH_EAST_BIT	HIVE_DIRECTION_BIT(HIVE_NORTH_EAST)
#define HIVE_NORTH_BIT		HIVE_DIRECTION_BIT(HIVE_NORTH)
#define HIVE_NORTH_WEST_BIT	HIVE_DIRECTION_BIT(HIVE_NORTH_WEST)
#define HIVE_SOUTH_WEST_BIT	HIVE_DIRECTION_BIT(HIVE_SOUTH_WEST)
#define HIVE_SOUTH_BIT		HIVE_DIRECTION_BIT(HIVE_SOUTH)
#define HIVE_ALL_DIRECTIONS 0x3F

#define HIVE_BLOCK_BITS1 \
	(HIVE_NORTH_BIT | HIVE_SOUTH_EAST_BIT | HIVE_SOUTH_WEST_BIT)

#define HIVE_BLOCK_BITS2 \
	(HIVE_SOUTH_BIT | HIVE_NORTH_EAST_BIT | HIVE_NORTH_WEST_BIT)

static unsigned char slide_bitset(unsigned char neigh)
{
	unsigned char avail = 0, block = 0;

	if ((neigh & HIVE_BLOCK_BITS1) == HIVE_BLOCK_BITS1)
		return 0;
	if ((neigh & HIVE_BLOCK_BITS2) == HIVE_BLOCK_BITS2)
		return 0;

	if (neigh & HIVE_SOUTH_EAST_BIT)
		avail |= ~neigh & (HIVE_SOUTH_BIT | HIVE_NORTH_EAST_BIT);
	if (neigh & HIVE_NORTH_EAST_BIT)
		avail |= ~neigh & (HIVE_NORTH_BIT | HIVE_SOUTH_EAST_BIT);
	if (neigh & HIVE_NORTH_BIT)
		avail |= ~neigh & (HIVE_NORTH_EAST_BIT | HIVE_NORTH_WEST_BIT);
	if (neigh & HIVE_NORTH_WEST_BIT)
		avail |= ~neigh & (HIVE_NORTH_BIT | HIVE_SOUTH_WEST_BIT);
	if (neigh & HIVE_SOUTH_WEST_BIT)
		avail |= ~neigh & (HIVE_NORTH_WEST_BIT | HIVE_SOUTH_BIT);
	if (neigh & HIVE_SOUTH_BIT)
		avail |= ~neigh & (HIVE_SOUTH_EAST_BIT | HIVE_SOUTH_WEST_BIT);

	if ((neigh & (HIVE_SOUTH_BIT | HIVE_NORTH_EAST_BIT)) ==
			(HIVE_SOUTH_BIT | HIVE_NORTH_EAST_BIT))
		block |= HIVE_SOUTH_EAST_BIT;
	if ((neigh & (HIVE_NORTH_BIT | HIVE_SOUTH_EAST_BIT)) ==
			(HIVE_NORTH_BIT | HIVE_SOUTH_EAST_BIT))
		block |= HIVE_NORTH_EAST_BIT;
	if ((neigh & (HIVE_NORTH_EAST_BIT | HIVE_NORTH_WEST_BIT)) ==
			(HIVE_NORTH_EAST_BIT | HIVE_NORTH_WEST_BIT))
		block |= HIVE_NORTH_BIT;
	if ((neigh & (HIVE_NORTH_BIT | HIVE_SOUTH_WEST_BIT)) ==
			(HIVE_NORTH_BIT | HIVE_SOUTH_WEST_BIT))
		block |= HIVE_NORTH_WEST_BIT;
	if ((neigh & (HIVE_NORTH_WEST_BIT | HIVE_SOUTH_BIT)) ==
			(HIVE_NORTH_WEST_BIT | HIVE_SOUTH_BIT))
		block |= HIVE_SOUTH_WEST_BIT;
	if ((neigh & (HIVE_SOUTH_EAST_BIT | HIVE_SOUTH_WEST_BIT)) ==
			(HIVE_SOUTH_EAST_BIT | HIVE_SOUTH_WEST_BIT))
		block |= HIVE_SOUTH_BIT;

	return avail & ~block;
}

static unsigned char neigh_bitset(struct hive *hive,
		const struct vec3 *pos, enum hive_side side)
{
	unsigned char neighBitset = 0;

	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		struct vec3 neighPos;
		
		neighPos = vec_move(pos, i);
		const piece_t neighPiece = hive_getexposedpiece(hive, &neighPos);
		if (neighPiece & side)
			neighBitset |= HIVE_DIRECTION_BIT(i);
	}
	return neighBitset;
}

bool hive_can_place(struct hive *hive, struct vec3 *pos, piece_t piece)
{
	return neigh_bitset(hive, pos, HIVE_GETSIDE(piece));
}

bool hive_one_hive(struct hive *hive, struct vec3 *pos)
{
	int count = 0;
	piece_t piece;
	char *visited;
	struct vec3 *posQueue = NULL;
	struct vec3 adjPos;
	piece_t adjPiece;

	piece = hive_getexposedpiece(hive, pos);
	if (piece & HIVE_BELOW && pos->z > 0) {
		return true;
	}

	/* TODO: add explanation for this?? */
    /*
        (Vaxeral)
        The flood fill algorithm works by visiting adjacent
        pieces and marking them so that they do not get put
        back into the queue.  As it is a piece does not have
        enough room for a flag to mark it as visited so i instead
        mark each unique piece as visited. A unqiue piece is
        the first 6 bits of the piece_t.  However you need an array
        bigger than the inventory size becuase not every bit representation
        of a piece_t can fit.
    */
	visited = malloc(HIVE_INVENTORY_SIZE);
	visited[HIVE_GETNPIECE(piece)] = true;
	count++;

	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		adjPos = vec_move(pos, i);
		adjPiece = hive_getabove(hive, &adjPos);
		if (adjPiece) {
			arrput(posQueue, adjPos);
			break;
		}
	}

	while (arrlen(posQueue)) {
		struct vec3 pos;

		pos = arrpop(posQueue);
		piece = hive->grid[pos.x + pos.y * GRID_COLUMNS];
		if (!visited[HIVE_GETNPIECE(piece)]) {
			visited[HIVE_GETNPIECE(piece)] = true;
			count++;
		}

		for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
			adjPos = vec_move(&pos, i);
			adjPiece = hive->grid[adjPos.x + adjPos.y * GRID_COLUMNS];
			if (adjPiece) {
				arrput(posQueue, adjPos);
				break;
			}
		}
	}

	free(visited);
	arrfree(posQueue);
	return count == hive->piecesPlayed;
}

static bool find_pos(const struct vec3 *posQueue, const struct vec3 *pos)
{
	for (int i = 0; i < arrlen(posQueue); i++)
		if (memcmp(&posQueue[i], pos, sizeof(*pos)) == 0)
			return true;
	return false;
}

/* These functions are just prototypes, they are not finished yet.
 * Ignore any warnings.
 *
 * TODO: addd comments describing the movement of each piece above
 * their respective function for getting the moves.
 */

/*
 * Ant can move any number of spaces along the hive.
 */

void hive_movesforant(struct hive *hive, const struct vec3 *startPos)
{
	struct vec3 *posQueue = NULL;
	struct vec3 *posVisited = NULL;
	arrput(posQueue, *startPos);
	arrput(posVisited, *startPos);

	while (arrlen(posQueue)) {
		struct vec3 currentPos = arrpop(posQueue);
		unsigned char
			neigh = neigh_bitset(hive, &currentPos, HIVE_WHITE | HIVE_BLACK);
		unsigned char
			slide = slide_bitset(neigh);

		for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
			if (slide & HIVE_DIRECTION_BIT(i)) {
				const struct vec3 neighPos = vec_move(&currentPos, i);
				if (find_pos(posVisited, &neighPos) == false) {
					arrput(posVisited, neighPos);
					arrput(posQueue, neighPos);
					arrput(hive->validMoves, ((struct move){
						.startPos = *startPos,
						.endPos = neighPos
					}));
				}
			}
		}
	}
	arrfree(posVisited);
	arrfree(posQueue);
}

/*
 * Spider can move three spaces along the hive.  It must not backtrack.
 * 
 * (Note) The way i interpret backtracking
          is if the piece midway through its move
          slides to its immediate previous position.
 */

struct node {
	struct vec3 pos;
	struct vec3 previousPos;
	int depth;
};

bool vec_isequal(const struct vec3 *a, const struct vec3 *b)
{
	return (a->x == b->x) && (a->y == b->y) && (a->z == b->z);
}

static void add_neighbors(struct hive *hive, struct node **nodeQueue, const struct node *node)
{
	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		const unsigned char
			neigh = neigh_bitset(
				hive, &node->pos, HIVE_WHITE | HIVE_BLACK);
		const unsigned char
			slide = slide_bitset(neigh);
		if (slide & HIVE_DIRECTION_BIT(i)) {
			struct vec3 neighPos = vec_move(&node->pos, i);
			arrput(*nodeQueue, ((struct node){
				.pos = neighPos,
				.previousPos = node->pos,
				.depth = 0
			}));
		}
	}
}

static void branch(struct hive *hive, struct node **nodeQueue, const struct node *node)
{
	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		const unsigned char
			neigh = neigh_bitset(
				hive, &node->pos, HIVE_WHITE | HIVE_BLACK);
		const unsigned char
			slide = slide_bitset(neigh);
		if (slide & HIVE_DIRECTION_BIT(i)) {
			struct vec3 neighPos = vec_move(&node->pos, i);
			if (memcmp(&neighPos, &node->previousPos, sizeof(neighPos)) != 0) {
				arrput(*nodeQueue, ((struct node){
					.pos = neighPos,
					.previousPos = node->pos,
					.depth = node->depth + 1
				}));
			}
		}
	}
}

void hive_movesforspider(struct hive *hive, const struct vec3 *startPos)
{
	struct node *nodeQueue = NULL;

	piece_t piece = hive->grid[startPos->x + startPos->y * GRID_COLUMNS];
	hive->grid[startPos->x + startPos->y * GRID_COLUMNS] = 0;

	add_neighbors(hive, &nodeQueue, &(struct node){
		.pos = *startPos
	});

	while (arrlen(nodeQueue)) {
		const struct node
			node = arrpop(nodeQueue);
		if (node.depth == 2) {
		    arrput(hive->validMoves, ((struct move){
		        .startPos = *startPos,
		        .endPos = node.pos
		    }));
			continue;
		}
		branch(hive, &nodeQueue, &node);
	}
	arrfree(nodeQueue);
	hive->grid[startPos->x + startPos->y * GRID_COLUMNS] = piece;
}

/*
 * Grasshopper can hop in a straight line over pieces directly in front of it.
 * It does not matter if it is surrouned by other pieces.
 */

void hive_movesforgrasshopper(struct hive *hive, const struct vec3 *startPos)
{
	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		struct vec3 neighPos = vec_move(startPos, i);
		piece_t neighPiece = hive->grid[neighPos.x + neighPos.y * GRID_COLUMNS];
		if (neighPiece) {
			while (neighPiece) {
				neighPos = vec_move(&neighPos, i);
				neighPiece = hive->grid[neighPos.x +
					neighPos.y * GRID_COLUMNS];
			}
			arrput(hive->validMoves, ((struct move){
				.startPos = *startPos,
				.endPos = neighPos
			}));
		}
	}
}

/*
 *  Queen can move one space along the hive.
 */

void hive_movesforqueen(struct hive *hive, const struct vec3 *startPos)
{
	const unsigned char
		neigh = neigh_bitset(hive, startPos, HIVE_WHITE | HIVE_BLACK),
		slide = slide_bitset(neigh);

	for (int i = 0; i < 6; ++i) {
		if (slide & HIVE_DIRECTION_BIT(i)) {
			struct vec3 neighPos = vec_move(startPos, i);
			arrput(hive->validMoves, ((struct move){
				.startPos = *startPos,
				.endPos = neighPos
			}));
		}
	}
}

/*
 * Beetle is like a queen except it may move on top of the hive.
 * You may stack as many beetles as you want.  You need not be
 * on the same level to move on top of one.
 */

void hive_movesforbeetle(struct hive *hive, const struct vec3 *startPos)
{
	struct vec3 stackPos = *startPos;
	const unsigned char
		neigh = neigh_bitset(hive, &stackPos, HIVE_WHITE | HIVE_BLACK);
	unsigned char slide; /* the beetle is special because it can be on top */
	const piece_t
		piece = hive_getexposedpiece(hive, &stackPos);

	if (startPos->z > 0)
		slide = HIVE_ALL_DIRECTIONS;
	else
		slide = slide_bitset(neigh) | neigh;
	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		if (slide & HIVE_DIRECTION_BIT(i)) {
			const struct vec3 neighPos = vec_move(startPos, i);
			arrput(hive->validMoves, ((struct move){
				.startPos = *startPos,
				.endPos = neighPos
			}));
		}
	}
}

void hive_generatemoves(struct hive *hive)
{
	arrsetlen(hive->validMoves, 0);
	for (int x = 0; x < GRID_COLUMNS; x++) {
		for (int y = 0; y < GRID_ROWS; y++) {
			piece_t piece =
				hive->grid[x + y * GRID_COLUMNS];
			const struct vec3 pos = {
				x, y, 0
			};
			switch (HIVE_GETNTYPE(piece)) {
			case HIVE_QUEEN:
				hive_movesforqueen(hive, &pos);
				break;
			case HIVE_BEETLE:
				hive_movesforbeetle(hive, &pos);
				break;
			case HIVE_GRASSHOPPER:
				hive_movesforgrasshopper(hive, &pos);
				break;
			case HIVE_SPIDER:
				hive_movesforspider(hive, &pos);
				break;
			case HIVE_ANT:
				hive_movesforant(hive, &pos);
				break;
			}
		}
	}
}
