#include "hex.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define HIVE_DIRECTION_BIT(dir) (1 << (dir))

#define HIVE_SOUTH_EAST_BIT	HIVE_DIRECTION_BIT(HIVE_SOUTH_EAST)
#define HIVE_NORTH_EAST_BIT	HIVE_DIRECTION_BIT(HIVE_NORTH_EAST)
#define HIVE_NORTH_BIT		HIVE_DIRECTION_BIT(HIVE_NORTH)
#define HIVE_NORTH_WEST_BIT	HIVE_DIRECTION_BIT(HIVE_NORTH_WEST)
#define HIVE_SOUTH_WEST_BIT	HIVE_DIRECTION_BIT(HIVE_SOUTH_WEST)
#define HIVE_SOUTH_BIT		HIVE_DIRECTION_BIT(HIVE_SOUTH)
#define HIVE_ALL_DIRECTIONS 0x3f

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

static unsigned char neigh_bitset(Hive *hive,
		const Vec3 *pos, hive_piece_t side)
{
	unsigned char neighBitset = 0;

	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		Vec3 neighPos;
		
		neighPos = vec_move(pos, i);
		const hive_piece_t neighPiece =
			hive_getexposedpiece(hive, &neighPos);
		if (neighPiece & side)
			neighBitset |= HIVE_DIRECTION_BIT(i);
	}
	return neighBitset;
}

bool hive_canplace(Hive *hive, const Vec3 *pos, hive_piece_t piece)
{
	return neigh_bitset(hive, pos, HIVE_GETSIDE(piece));
}

void hive_addneighbor(Hive *hive,
	Vec3 **posQueue, const Vec3 *startPos)
{
	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		const Vec3 neighPos = vec_move(startPos, i);
		const hive_piece_t neighPiece =
			hive->grid[neighPos.x + neighPos.y * HIVE_GRID_COLUMNS];
		if (neighPiece) {
			arrput(*posQueue, neighPos);
			break;
		}
	}
}

void hive_addneighbors(Hive *hive,
	Vec3 **posQueue, const Vec3 *startPos)
{
	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		const Vec3 neighPos = vec_move(startPos, i);
		const hive_piece_t neighPiece =
			hive->grid[neighPos.x + neighPos.y * HIVE_GRID_COLUMNS];
		if (neighPiece)
			arrput(*posQueue, neighPos);
	}
}

bool hive_onehive(Hive *hive, const Vec3 *startPos)
{
	int piecesVisited = 0;
	Vec3 *posQueue = NULL;

	Vec3 pos = *startPos;
	(void) hive_getexposedpiece(hive, &pos);
	if (pos.z > 0)
		return true;

	for (int x = 0; x < HIVE_GRID_COLUMNS; x++)
		for (int y = 0; y < HIVE_GRID_ROWS; y++) {
			Vec3 pos = { x, y, 0 };
			hive->grid[pos.x + pos.y * HIVE_GRID_COLUMNS] &= ~HIVE_VISIT;
		}

	hive->grid[pos.x + pos.y * HIVE_GRID_COLUMNS] |= HIVE_VISIT;
	piecesVisited++;

	hive_addneighbor(hive, &posQueue, startPos);

	while (arrlen(posQueue)) {
		Vec3 neighPos = arrpop(posQueue);
		/* FIXME: unused variable */
		const hive_piece_t neighPiece =
			hive_getexposedpiece(hive, &neighPos);
		if (hive->grid[neighPos.x + neighPos.y * HIVE_GRID_COLUMNS] & HIVE_VISIT)
			continue;
		hive->grid[neighPos.x + neighPos.y * HIVE_GRID_COLUMNS] |= HIVE_VISIT;
		piecesVisited += neighPos.z + 1;
		hive_addneighbors(hive, &posQueue, &neighPos);
	}

	arrfree(posQueue);

	return piecesVisited == hive->piecesPlayed;
}

static bool find_pos(const Vec3 *posQueue, const Vec3 *pos)
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

/* Ant can move any number of spaces along the hive.
 */
void hive_movesforant(Hive *hive, const Vec3 *startPos)
{
	Vec3 *posQueue = NULL;
	Vec3 *posVisited = NULL;
	arrput(posQueue, *startPos);
	arrput(posVisited, *startPos);

	hive_piece_t piece = hive->grid[startPos->x + startPos->y * HIVE_GRID_COLUMNS];
	hive->grid[startPos->x + startPos->y * HIVE_GRID_COLUMNS] = 0;

	while (arrlen(posQueue)) {
		Vec3 currentPos = arrpop(posQueue);
		unsigned char
			neigh = neigh_bitset(hive, &currentPos, HIVE_WHITE | HIVE_BLACK);
		unsigned char
			slide = slide_bitset(neigh);

		for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
			if (slide & HIVE_DIRECTION_BIT(i)) {
				const Vec3 
					neighPos = vec_move(&currentPos, i);
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
	hive->grid[startPos->x + startPos->y * HIVE_GRID_COLUMNS] = piece;
	arrfree(posVisited);
	arrfree(posQueue);
}

/* Spider can move three spaces along the hive.  It must not backtrack.
 */

struct node {
	Vec3 pos;
	Vec3 previousPos;
	int depth;
};

static void spider_slide(Hive *hive,
	struct node **nodeQueue, const struct node *node)
{
	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		const unsigned char
			neigh = neigh_bitset(
				hive, &node->pos, HIVE_WHITE | HIVE_BLACK);
		const unsigned char
			slide = slide_bitset(neigh);
		if (slide & HIVE_DIRECTION_BIT(i)) {
			Vec3 neighPos = vec_move(&node->pos, i);
			arrput(*nodeQueue, ((struct node){
				.pos = neighPos,
				.previousPos = node->pos,
				.depth = 0
			}));
		}
	}
}

static void spider_branch(Hive *hive,
	struct node **nodeQueue, const struct node *node)
{
	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		const unsigned char
			neigh = neigh_bitset(
				hive, &node->pos, HIVE_WHITE | HIVE_BLACK);
		const unsigned char
			slide = slide_bitset(neigh);
		if (slide & HIVE_DIRECTION_BIT(i)) {
			Vec3 neighPos = vec_move(&node->pos, i);
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

void hive_movesforspider(Hive *hive, const Vec3 *startPos)
{
	struct node *nodeQueue = NULL;

	hive_piece_t piece = hive->grid[startPos->x +
		startPos->y * HIVE_GRID_COLUMNS];
	hive->grid[startPos->x + startPos->y * HIVE_GRID_COLUMNS] = 0;

	spider_slide(hive, &nodeQueue, &(struct node){
		.pos = *startPos
	});

	const int maxDepth = 2;

	while (arrlen(nodeQueue)) {
		const struct node
			node = arrpop(nodeQueue);
		if (node.depth == maxDepth) {
			arrput(hive->validMoves, ((struct move){
				.startPos = *startPos,
				.endPos = node.pos
			}));
			continue;
		}
		spider_branch(hive, &nodeQueue, &node);
	}
	hive->grid[startPos->x +
		startPos->y * HIVE_GRID_COLUMNS] = piece;
	arrfree(nodeQueue);
}

/*
 * Grasshopper can hop in a straight line over pieces directly in front of it.
 * It does not matter if it is surrouned by other pieces.
 */

void hive_movesforgrasshopper(Hive *hive, const Vec3 *startPos)
{
	Vec3 neighPos;

	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++) {
		neighPos = vec_move(startPos, i);
		hive_piece_t neighPiece =
			hive->grid[neighPos.x + neighPos.y * HIVE_GRID_COLUMNS];
		if (neighPiece) {
			while (neighPiece) {
				neighPos = vec_move(&neighPos, i);
				neighPiece = hive->grid[neighPos.x +
					neighPos.y * HIVE_GRID_COLUMNS];
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

void hive_movesforqueen(Hive *hive, const Vec3 *startPos)
{
	const unsigned char
		neigh = neigh_bitset(hive, startPos, HIVE_WHITE | HIVE_BLACK);
	const unsigned char
		slide = slide_bitset(neigh);

	for (int i = 0; i < 6; ++i)
		if (slide & HIVE_DIRECTION_BIT(i)) {
			Vec3 neighPos = vec_move(startPos, i);
			arrput(hive->validMoves, ((struct move){
				.startPos = *startPos,
				.endPos = neighPos
			}));
		}
}

/*
 * Beetle is like a queen except it may move on top of the hive.
 * You may stack as many beetles as you want.  You need not be
 * on the same level to move on top of one.
 */

void hive_movesforbeetle(Hive *hive, const Vec3 *startPos)
{
	Vec3 pos = *startPos;
	const unsigned char
		neigh = neigh_bitset(hive, &pos, HIVE_WHITE | HIVE_BLACK);
	unsigned char slide; /* the beetle is special because it can be on top */
	hive_piece_t below =
		hive->grid[pos.x + pos.y * HIVE_GRID_COLUMNS];

	if (below & HIVE_ABOVE)
		slide = HIVE_ALL_DIRECTIONS;
	else
		slide = slide_bitset(neigh) | neigh;

	for (int i = 0; i < HIVE_DIRECTION_COUNT; i++)
		if (slide & HIVE_DIRECTION_BIT(i)) {
			const Vec3 neighPos = vec_move(startPos, i);
			arrput(hive->validMoves, ((struct move){
				.startPos = *startPos,
				.endPos = neighPos
			}));
		}
}

static void hive_movesfor(Hive *hive, Vec3 *pos, hive_piece_t piece)
{
	if (!hive_onehive(hive, pos))
		return;

	switch (HIVE_GETNTYPE(piece)) {
	case HIVE_QUEEN:
		hive_movesforqueen(hive, pos);
		break;
	case HIVE_BEETLE:
		hive_movesforbeetle(hive, pos);
		break;
	case HIVE_GRASSHOPPER:
		hive_movesforgrasshopper(hive, pos);
		break;
	case HIVE_SPIDER:
		hive_movesforspider(hive, pos);
		break;
	case HIVE_ANT:
		hive_movesforant(hive, pos);
		break;
	}
}

void hive_generatemoves(Hive *hive)
{
	arrsetlen(hive->validMoves, 0);

	for (int x = 0; x < HIVE_GRID_COLUMNS; x++)
		for (int y = 0; y < HIVE_GRID_ROWS; y++) {
			Vec3 pos = { x, y, 0 };
			hive_piece_t piece = hive_getexposedpiece(hive, &pos);
			if (piece)
				hive_movesfor(hive, &(Vec3){ x, y, 0 }, piece);
		}
}
