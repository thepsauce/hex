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

	for(int i = 0; i < 6; ++i) {
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
	visited = malloc(HIVE_INVENTORY_SIZE);
	visited[HIVE_GETNPIECE(piece)] = true;
	count++;

	for (int i = 0; i < 6; i++) {
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

		for (int i = 0; i < 6; i++) {
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

void hive_moves_for_ant(struct hive *hive, struct vec3 *pos)
{
	struct vec3 *posQueue = NULL;
	struct vec3 *movePos = NULL;
	struct vec3 currentPos;

	arrput(posQueue, *pos);
	while (arrlen(posQueue)) {
		currentPos = arrpop(posQueue);

		unsigned char
			neigh = neigh_bitset(hive,
					&currentPos, HIVE_WHITE | HIVE_BLACK),
			slide = slide_bitset(neigh);
		for (int i = 0; i < 6; i++) {
			if (slide & HIVE_DIRECTION_BIT(i)) {
				const struct vec3 adjPos = vec_move(&currentPos, i);
				if (find_pos(posQueue, &adjPos) == false) {
					arrput(posQueue, adjPos);
					arrput(movePos, adjPos);
				}
			}
		}
	}
	arrfree(posQueue);
	arrfree(movePos);
}

void hive_moves_for_spider(struct hive *hive, struct vec3 *pos)
{
	/* FIXME: unused variables */
	const unsigned char
		neigh = neigh_bitset(hive, pos, HIVE_WHITE | HIVE_BLACK),
		slide = slide_bitset(neigh);
	struct node {
		struct vec3 pos;
		struct vec3 previousPos;
		int depth;
	} *nodeQueue = NULL;
	struct vec3 *movePos = NULL;

	arrput(nodeQueue,
		((struct node) { *pos, (struct vec3) { 0, 0, 0 }, 0 }));

	while (arrlen(nodeQueue)) {
		const struct node node = arrpop(nodeQueue);
		if (node.depth == 2)
			arrput(movePos, node.pos);

		const unsigned char
			neigh = neigh_bitset(hive,
					&node.pos, HIVE_WHITE | HIVE_BLACK),
			slide = slide_bitset(neigh);
		for (int i = 0; i < 6; i++) {
			if (slide & HIVE_DIRECTION_BIT(i)) {
				const struct vec3 adjPos = vec_move(&node.pos, i);
				if (memcmp(&adjPos, &node.previousPos,
						sizeof(node.previousPos)) != 0) {
					arrput(nodeQueue, ((struct node)
					{ adjPos, node.pos, node.depth + 1 }));
				}
			}
		}
	}
	arrfree(nodeQueue);
	arrfree(movePos);
}

void hive_moves_for_grasshopper(struct hive *hive, struct vec3 *pos)
{
	struct vec3 *movePos = NULL;
	struct vec3 adjPos;
	piece_t adjPiece;

	for (int i = 0; i < 6; i++) {
		adjPos = vec_move(pos, i);
		adjPiece = hive->grid[adjPos.x + adjPos.y * GRID_COLUMNS];
		if (adjPiece) {
			while (adjPiece) {
				adjPos = vec_move(&adjPos, i);
				adjPiece = hive->grid[adjPos.x +
					adjPos.y * GRID_COLUMNS];
			}
			arrput(movePos, adjPos);
		}
	}
	arrfree(movePos);
}

void hive_moves_for_queen(struct hive *hive, struct vec3 *pos)
{
	const unsigned char
		neigh = neigh_bitset(hive, pos, HIVE_WHITE | HIVE_BLACK),
		slide = slide_bitset(neigh);
	struct vec3 *movePos = NULL;

	for (int i = 0; i < 6; ++i) {
		if (slide & HIVE_DIRECTION_BIT(i)) {
			struct vec3 adjPos = vec_move(pos, i);
			arrput(movePos, adjPos);
		}
	}
	arrfree(movePos);
}

void hive_moves_for_beetle(struct hive *hive, struct vec3 *pos)
{
	const unsigned char
		neigh = neigh_bitset(hive, pos, HIVE_WHITE | HIVE_BLACK);
	unsigned char slide; /* the beetle is special because it can be on top */
	struct vec3 *movePos = NULL;
	const piece_t piece = hive_getexposedpiece(hive, pos);

	if (pos->z > 0)
		slide = 0xFF;
	else
		slide = slide_bitset(neigh) & neigh;
	for (int i = 0; i < 6; i++) {
		if (slide & HIVE_DIRECTION_BIT(i)) {
			const struct vec3 adjPos = vec_move(pos, i);
			arrput(movePos, adjPos);
		}
	}
	arrfree(movePos);
}
