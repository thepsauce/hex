#include <hive.h>
#define STB_DS_IMPLEMENTATION
#include <stb_ds.h>

#define HIVE_DIRECTION_BIT(dir) (1 << (dir))

const unsigned char HIVE_SOUTH_EAST_BIT = HIVE_DIRECTION_BIT(HIVE_SOUTH_EAST);
const unsigned char HIVE_NORTH_EAST_BIT = HIVE_DIRECTION_BIT(HIVE_NORTH_EAST);
const unsigned char HIVE_NORTH_BIT = HIVE_DIRECTION_BIT(HIVE_NORTH);
const unsigned char HIVE_NORTH_WEST_BIT = HIVE_DIRECTION_BIT(HIVE_NORTH_WEST);
const unsigned char HIVE_SOUTH_WEST_BIT = HIVE_DIRECTION_BIT(HIVE_SOUTH_WEST);
const unsigned char HIVE_SOUTH_BIT = HIVE_DIRECTION_BIT(HIVE_SOUTH);

static unsigned char slide_bitset(unsigned char neigh)
{
    if ((neigh & (HIVE_NORTH_BIT | HIVE_SOUTH_EAST_BIT | HIVE_SOUTH_WEST_BIT)) ==
                 (HIVE_NORTH_BIT | HIVE_SOUTH_EAST_BIT | HIVE_SOUTH_WEST_BIT))
        return 0;
    if ((neigh & (HIVE_SOUTH_BIT | HIVE_NORTH_EAST_BIT | HIVE_NORTH_WEST_BIT)) ==
                 (HIVE_SOUTH_BIT | HIVE_NORTH_EAST_BIT | HIVE_NORTH_WEST_BIT))
        return 0;

    unsigned char avail = 0;

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

    unsigned char block = 0;

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

static unsigned char neigh_bitset(struct hive *hive, struct vec3 pos, enum hive_side side)
{
    unsigned char neighBitset = 0;
    for(int i = 0; i < 6; ++i) {
    	struct vec3 neighPos = vec_move(&pos, i);
        piece_t neighPiece = hive_getexposedpiece(hive, &neighPos);
        if (neighPiece & side)
        	neighBitset |= HIVE_DIRECTION_BIT(i);
    }
    return neighBitset;
}

bool hive_can_place(struct hive *hive, struct vec3 pos, piece_t piece)
{
	return neigh_bitset(hive, pos, HIVE_GETSIDE(piece));
}


bool hive_one_hive(struct hive *hive, struct vec3 pos)
{
	int count = 0;
	piece_t piece = hive_getexposedpiece(hive, &pos);
	if (piece & HIVE_BELOW && pos.z > 0) {
		return true;
	}

	char *visited = malloc(HIVE_INVENTORY_SIZE);
	visited[HIVE_GETNPIECE(piece)] = true;
	count++;

	struct vec3 *posQueue;

	struct vec3 adjPos;
	piece_t adjPiece;

	for (int i = 0; i < 6; i++) {
		adjPos = vec_move(&pos, i);
		adjPiece = hive_getabove(hive, &adjPos);
		if (adjPiece) {
			arrput(posQueue, adjPos);
			break;
		}
	}

	while (arrlen(posQueue)) {
		piece_t piece = hive->grid[adjPos.x + adjPos.y * GRID_COLUMNS];
		if (visited[HIVE_GETNPIECE(piece)] == false) {
			visited[HIVE_GETNPIECE(piece)] == true;
			count++;
		}

		for (int i = 0; i < 6; i++) {
			adjPos = vec_move(&pos, i);
			adjPiece = hive_getabove(hive, &adjPos);
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
   	for (int i = 0; i < arrlen(posQueue); i++) {
   		if (memcmp(&posQueue[i], pos, sizeof pos) == 0) {
   			return true;
   		}
   	}
   	return false;
}

void hive_moves_for_ant(struct hive *hive, struct vec3 pos)
{
    struct vec3 *posQueue = NULL;
    arrput(posQueue, pos);
    while (arrlen(posQueue)) {
        struct vec3 currentPos = arrpop(posQueue);
        unsigned char neigh = neigh_bitset(hive, currentPos, HIVE_WHITE & HIVE_BLACK);
        unsigned char slide = slide_bitset(neigh);
        for (int i = 0; i < 6; i++) {
            if(slide & HIVE_DIRECTION_BIT(i)) {
                struct vec3 adjPos = vec_move(&currentPos, i);
                if (find_pos(posQueue, &adjPos) == false) {
                	arrput(posQueue, adjPos);
                }
            }
        }
    }
    arrfree(posQueue);
}
