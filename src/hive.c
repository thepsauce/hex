#include "hex.h"
#include "stb_ds.h"

int hive_init(Hive *hive)
{
	static const hive_piece_t defaultInventory[HIVE_INVENTORY_SIZE] = {
		HIVE_QUEEN,
		HIVE_BEETLE,
		HIVE_BEETLE,
		HIVE_GRASSHOPPER,
		HIVE_GRASSHOPPER,
		HIVE_GRASSHOPPER,
		HIVE_SPIDER,
		HIVE_SPIDER,
		HIVE_ANT,
		HIVE_ANT,
		HIVE_ANT,
		HIVE_LADYBUG,
		HIVE_MOSQUITO,
		HIVE_PILLBUG,
		HIVE_QUEEN,
		HIVE_BEETLE,
		HIVE_BEETLE,
		HIVE_GRASSHOPPER,
		HIVE_GRASSHOPPER,
		HIVE_GRASSHOPPER,
		HIVE_SPIDER,
		HIVE_SPIDER,
		HIVE_ANT,
		HIVE_ANT,
		HIVE_ANT,
		HIVE_LADYBUG,
		HIVE_MOSQUITO,
		HIVE_PILLBUG
	};

	memset(hive, 0, sizeof(*hive));
	if ((hive->win = newpad(HIVE_GRID_ROWS * 2, HIVE_GRID_COLUMNS * 4)) == NULL)
		return -1;
	hive->turn = HIVE_BLACK;
	memcpy(&hive->blackInventory,
		defaultInventory, sizeof(defaultInventory));
	memcpy(&hive->whiteInventory,
		defaultInventory, sizeof(defaultInventory));
	hive->winPos.x = (HIVE_GRID_COLUMNS / 2) * 4 - COLS / 4 + 1;
	hive->winPos.y = (HIVE_GRID_ROWS / 2) * 2 - LINES / 2 + 1;
	hive->winSize.x = (COLS - 1) / 2;
	hive->winSize.y = LINES - 1;


	srand(time(NULL));

	/* test setup */
	Vec3 pos = { HIVE_GRID_COLUMNS / 2, HIVE_GRID_ROWS / 2, 0 };
	hive_playmove(hive, &(struct move) {
		.startPos = (Vec3) { -1, -1, -1 },
		.endPos = pos,
		.piece = HIVE_WHITE | HIVE_QUEEN
	});
	hive_render(hive);

	return 0;
}

int hive_getabovestack(Hive *hive, const Vec3 *pos)
{
	for (int i = 0; i < hive->stackSz; i++)
		if (memcmp(&hive->stacks[i].pos, pos, sizeof *pos) == 0)
			return i;
	return 0;
}

hive_piece_t hive_getabove(Hive *hive, Vec3 *pos)
{
	for (int i = 0; i < hive->stackSz; i++)
		if (memcmp(&hive->stacks[i].pos, pos, sizeof *pos) == 0)
			return hive->stacks[i].piece;
	return 0;
}

hive_piece_t hive_getexposedpiece(Hive *hive, Vec3 *pos)
{
	hive_piece_t piece;
	
	piece = hive->grid[pos->x + pos->y * HIVE_GRID_COLUMNS];
	while (piece & HIVE_ABOVE) {
		piece = hive_getabove(hive, pos);
		pos->z++;
	}
	return piece;
}

void hive_stack(Hive *hive, const Vec3 *pos, hive_piece_t piece)
{
	assert(hive->stackSz < HIVE_STACK_SIZE);
	hive->stacks[hive->stackSz].pos = *pos;
	hive->stacks[hive->stackSz].piece = piece;
	hive->stackSz++;
}

void hive_unstack(Hive *hive, const Vec3 *pos)
{
	int stack;

	assert(hive->stackSz > 0);

	stack = hive_getabovestack(hive, pos);
	hive->stackSz--;
	memmove(&hive->stacks[stack],
		&hive->stacks[hive->stackSz], sizeof *hive->stacks);
}

void hive_toggle(Hive *hive, const Vec3 *pos, hive_piece_t bits)
{
	for (int i = 0; i < hive->stackSz; i++)
		if (memcmp(&hive->stacks[i].pos, pos, sizeof *pos) == 0) {
			hive->stacks[i].piece ^= bits;
			return;
		}
	/* else */
	hive->grid[pos->x + pos->y * HIVE_GRID_COLUMNS] ^= bits;
}

void hive_putpiece(Hive *hive, Vec3 *pos, hive_piece_t piece)
{
	hive_piece_t below = hive->grid[pos->x + pos->y * HIVE_GRID_COLUMNS];

	if(below) {
		int z = -1;
		while (below & HIVE_ABOVE) {
			below = hive_getabove(hive, pos);
			z = pos->z++;
		}

		hive_toggle(hive, &(Vec3){
			pos->x, pos->y, z
		}, HIVE_ABOVE);

		hive_stack(hive, pos, piece);
	} else
		hive->grid[pos->x + pos->y * HIVE_GRID_COLUMNS] = piece;
}

void hive_delpiece(Hive *hive, Vec3 *pos)
{
	hive_piece_t below = hive->grid[pos->x + pos->y * HIVE_GRID_COLUMNS];

	if (below) {
		int z = -1;
		while (below & HIVE_ABOVE) {
			below = hive_getabove(hive, pos);
			z = pos->z++;
		}

		if (z == -1) {
			hive->grid[pos->x + pos->y * HIVE_GRID_COLUMNS] = 0;
			return;
		}

		hive_toggle(hive, &(Vec3){
			pos->x, pos->y, z - 1
		}, HIVE_ABOVE);

		pos->z = z;

		hive_unstack(hive, pos);
	}
}

static void hive_handlemousepress(Hive *hive, const Vec3 *mp)
{
	Vec3 pos;

	pos = *mp;
	pos.x = pos.x / 4;
	pos.y = (pos.y - (pos.x % 2)) / 2;
	pos.z = 0;

	if (hive->selectedPiece) {
		hive->pendingMove.endPos = pos;
		hive->pendingMove.piece = hive->selectedPiece;
		hive->playMove = true;
	}
	else {
		hive->pendingMove.startPos = pos;
		hive->selectedPiece = hive_getexposedpiece(hive, &pos);
	}
}

bool hive_haspiece(Hive *hive, hive_piece_t type, hive_piece_t side)
{
	hive_piece_t *inventory = NULL;
	if (side & HIVE_WHITE)
		inventory = hive->whiteInventory;
	else
		inventory = hive->blackInventory;

	for (int i = 0; i < HIVE_INVENTORY_SIZE; i++)
		if (inventory[i] == type)
			return true;
	/* else */
	return false;
}

bool hive_playmove(Hive *hive, struct move *move)
{
	const Vec3 nullVec = { -1, -1, -1 };

	hive->playMove = false;

	if (!hive->piecesPlayed)
		goto makemove;

	for (int i = 0; i < arrlen(hive->validMoves); i++)
		if (memcmp(&hive->validMoves[i], move, sizeof *move) == 0)
			goto makemove;
	/* else */
	return false;

makemove:
	if (memcmp(&move->startPos, &nullVec, sizeof(nullVec)) == 0) {
		/* This may not be needed.  I will probably generate all placements and
		 * store them along with the valid moves.
		 */
		if (!hive_haspiece(hive, HIVE_GETTYPE(move->piece),
				hive->turn % 2 ? HIVE_WHITE : HIVE_BLACK))
			return false;

		hive_putpiece(hive, &move->endPos, move->piece);
		hive->piecesPlayed++;
	} else {
		hive_delpiece(hive, &move->startPos);
		hive_putpiece(hive, &move->endPos, move->piece);
	}
	return true;
}

int hive_handle(Hive *hive, int c)
{
	MEVENT event;

	switch(c) {
	case KEY_MOUSE:
		getmouse(&event);
		if (event.bstate & BUTTON1_CLICKED || event.bstate & BUTTON1_PRESSED) {
			Vec3 pos;

			pos.x = event.x;
			pos.y = event.y;
			wmouse_trafo(hive->win, &pos.y, &pos.x, FALSE);

			pos.x = pos.x + hive->winPos.x;
			pos.y = pos.y + hive->winPos.y;
			hive_handlemousepress(hive, &pos);

			if (hive->playMove)
				if (hive_playmove(hive, &hive->pendingMove)) {
					hive_generatemoves(hive);
					hive_render(hive);
					hive->selectedPiece = 0;
				}
		}
		if (event.bstate & BUTTON3_CLICKED || event.bstate & BUTTON3_PRESSED)
			hive->selectedPiece = 0;
		break;
	case 'a':
		hive->winPos.x = hive->winPos.x < getmaxx(hive->win) ?
			hive->winPos.x + 1 : hive->winPos.x;
		break;
	case 'd':
		hive->winPos.x = hive->winPos.x > 0 ?
			hive->winPos.x - 1 : hive->winPos.x;
		break;
	case 's':
		hive->winPos.y = hive->winPos.y > 0 ?
			hive->winPos.y - 1 : hive->winPos.y;
		break;
	case 'w':
		hive->winPos.y = hive->winPos.y < getmaxy(hive->win) ?
			hive->winPos.y + 1 : hive->winPos.y;
		break;
	}
	return 0;
}

void hive_render(Hive *hive)
{
	static const char *pieceNames[] = {
		" ",
		"Q",
		"B",
		"G",
		"S",
		"A",
		"L",
		"M",
		"P"
	};
	static const char *triangles[] = {
		"\u25e2",
		"\u25e3",
		"\u25e4",
		"\u25e5"
	};
	static const hive_direction_t toDirection[] = {
		HIVE_NORTH_WEST,
		HIVE_NORTH_EAST,
		HIVE_SOUTH_EAST,
		HIVE_SOUTH_WEST
	};
	static const Vec3 cellOffsets[] = {
		{ 0, 0, 0 },
		{ 4, 0, 0 },
		{ 4, 1, 0 },
		{ 0, 1, 0 }
	};
	static const short pairs[3][3] = {
		/* -1 because these values are unused */
		{ -1, -1, -1},
		{ HIVE_PAIR_BLACK,
			HIVE_PAIR_BLACK_BLACK,
			HIVE_PAIR_BLACK_WHITE },
		{ HIVE_PAIR_WHITE,
			HIVE_PAIR_WHITE_BLACK,
			HIVE_PAIR_WHITE_WHITE },
	};
	Vec3 pos, world;
	WINDOW *const win = hive->win;

	werase(win);
	for (pos.y = 0; pos.y < HIVE_GRID_ROWS; pos.y++)
		for (pos.x = 0; pos.x < HIVE_GRID_COLUMNS; pos.x++) {
			hive_piece_t piece;
			hive_piece_t type;

			world.x = pos.x * 4;
			world.y = pos.y * 2 + pos.x % 2;

			pos.z = 0;
			piece = hive_getexposedpiece(hive, &pos);
			type = HIVE_GETTYPE(piece);
			if (type == HIVE_EMPTY)
				continue;

			const hive_piece_t side = HIVE_GETSIDE(piece);
			wattr_set(win, A_REVERSE, side == HIVE_WHITE ?
				HIVE_PAIR_WHITE : HIVE_PAIR_BLACK, NULL);

			mvwprintw(win, world.y, world.x + 1,
				" %s ", pieceNames[(int) type]);
			mvwprintw(win, world.y + 1, world.x + 1,
				"%s", "   ");

			for (int n = 0; n < 4; n++) {
				Vec3 neighPos;

				neighPos = vec_move(&pos, toDirection[n]);
				const hive_piece_t neighPiece =
					hive_getexposedpiece(hive, &neighPos);
				const hive_piece_t p =
					pairs[HIVE_GETNSIDE(piece)]
						[HIVE_GETNSIDE(neighPiece)];
				wattr_set(win, 0, p, NULL);

				const Vec3 off = cellOffsets[n];
				mvwprintw(win, world.y + off.y,
					world.x + off.x,
					"%s", triangles[n]);
			}
		}
	prefresh(win,
		hive->winPos.y, hive->winPos.x, 0, 0,
		hive->winSize.y, hive->winSize.x);
}
