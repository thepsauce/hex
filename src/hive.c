#include "hive.h"
#include "stb_ds.h"

int hive_init(struct hive *hive)
{
	static const enum hive_type defaultInventory[HIVE_INVENTORY_SIZE] = {
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
	if ((hive->win = newpad(GRID_ROWS * 2, GRID_COLUMNS * 4)) == NULL)
		return -1;
	hive->turn = HIVE_BLACK;
	memcpy(&hive->blackInventory,
		defaultInventory, sizeof(defaultInventory));
	memcpy(&hive->whiteInventory,
		defaultInventory, sizeof(defaultInventory));

	/* test setup */
	struct vec3 pos = { 6, 6, 0 };
	for (int i = 0; i < 20; i++) {
		const enum hive_type t = 1 + rand() % (HIVE_TYPES - 1);
		const enum hive_side s = (1 + rand() % 2) << 4;
		hive->grid[pos.x + pos.y * GRID_COLUMNS] = t | s;
		pos = vec_move(&pos, rand() % 6);
	}
	hive_render(hive);
	return 0;
}

piece_t hive_getabove(struct hive *hive, const struct vec3 *pos)
{
	for (int i = 0; i < HIVE_STACK_SIZE; i++)
		if (memcmp(&hive->stacks[i].pos, pos, sizeof(*pos)) == 0)
			return hive->stacks[i].above;
	return 0;
}

piece_t hive_getexposedpiece(struct hive *hive, struct vec3 *pos)
{
	piece_t piece;

	piece = hive->grid[pos->x + pos->y * GRID_COLUMNS];
	while (piece & HIVE_ABOVE) {
		piece = hive_getabove(hive, pos);
		pos->z++;
	}
	return piece;
}

static void hive_playmove(struct hive *hive, struct move *move)
{
	hive->grid[move->endPos.x +
		       move->endPos.y * GRID_COLUMNS] = hive->selectedPiece;
	hive->selectedPiece = 0;
	hive->grid[move->startPos.x +
		       move->startPos.y * GRID_COLUMNS] = 0;
	/* startPos and endPos must differ.
	 * If they are the same it is considered an accident and no move will be made.
	 */
	memset(&move->startPos, 0, sizeof(move->startPos));
	memset(&move->endPos, 1, sizeof(move->endPos));
}

static void hive_handlemousepress(struct hive *hive, const struct vec3 *mp)
{
	struct vec3 pos;
	struct move *pendingMove;

	pos = *mp;
	pos.x = pos.x / 4;
	pos.y = (pos.y - (pos.x % 2)) / 2;
	pos.z = 0;
	pendingMove = &hive->pendingMove;
	if (pos.x != pendingMove->endPos.x || pos.y != pendingMove->endPos.y) {
		if (hive->selectedPiece) {
			pendingMove->endPos = pos;
		} else {
			pendingMove->startPos = pos;
			hive->selectedPiece = hive_getexposedpiece(hive, &pos);
		}
	}
}

int hive_handle(struct hive *hive, int c)
{
	MEVENT event;

	switch(c) {
	case KEY_MOUSE:
		getmouse(&event);
		if (event.bstate & BUTTON1_CLICKED ||
				event.bstate & BUTTON1_PRESSED) {
			struct vec3 pos;

			pos.x = event.x;
			pos.y = event.y;
			wmouse_trafo(hive->win, &pos.y, &pos.x, FALSE);

			pos.x = pos.x + hive->winPos.x;
			pos.y = pos.y + hive->winPos.y;
			hive_handlemousepress(hive, &pos);

			hive_generatemoves(hive);

			for (int i = 0; i < arrlen(hive->validMoves); i++) {
				struct move m = hive->validMoves[i];
				if (memcmp(&m, &hive->pendingMove, sizeof(m)) == 0) {
					hive_playmove(hive, &hive->pendingMove);
					break;
				}
			}
		}
		break;
	case 'd':
		hive->winPos.x = hive->winPos.x < getmaxx(hive->win) ?
			hive->winPos.x + 1 : hive->winPos.x;
		break;
	case 'a':
		hive->winPos.x = hive->winPos.x > 0 ?
			hive->winPos.x - 1 : hive->winPos.x;
		break;
	case 'w':
		hive->winPos.y = hive->winPos.y > 0 ?
			hive->winPos.y - 1 : hive->winPos.y;
		break;
	case 's':
		hive->winPos.y = hive->winPos.y < getmaxy(hive->win) ?
			hive->winPos.y + 1 : hive->winPos.y;
		break;
	}
	return 0;
}

void hive_render(struct hive *hive)
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
	static const enum hive_direction toDirection[] = {
		HIVE_NORTH_WEST,
		HIVE_NORTH_EAST,
		HIVE_SOUTH_EAST,
		HIVE_SOUTH_WEST
	};
	static const struct vec3 cellOffsets[] = {
		{ 0, 0, 0 },
		{ 4, 0, 0 },
		{ 4, 1, 0 },
		{ 0, 1, 0 }
	};
	static const int pairs[3][3] = {
		/* -1 because these values are unused */
		{ -1, -1, -1},
		{ HIVE_PAIR_BLACK,
			HIVE_PAIR_BLACK_BLACK,
			HIVE_PAIR_BLACK_WHITE },
		{ HIVE_PAIR_WHITE,
			HIVE_PAIR_WHITE_BLACK,
			HIVE_PAIR_WHITE_WHITE },
	};
	struct vec3 pos, world;
	WINDOW *const win = hive->win;

	werase(win);
	for (pos.y = 0; pos.y < GRID_ROWS; pos.y++)
		for (pos.x = 0; pos.x < GRID_COLUMNS; pos.x++) {
			piece_t piece;
			enum hive_type type;

			world.x = pos.x * 4;
			world.y = pos.y * 2 + pos.x % 2;

			piece = hive_getexposedpiece(hive, &pos);
			type = HIVE_GETTYPE(piece);
			if (type == HIVE_EMPTY)
				continue;

			const enum hive_side side = HIVE_GETSIDE(piece);
			wattr_set(win, A_REVERSE, side == HIVE_WHITE ?
				HIVE_PAIR_WHITE : HIVE_PAIR_BLACK, NULL);

			mvwprintw(win, world.y, world.x + 1,
				" %s ", pieceNames[type]);
			mvwprintw(win, world.y + 1, world.x + 1,
				"%s", "   ");

			for (int n = 0; n < 4; n++) {
				struct vec3 neighPos;

				neighPos = vec_move(&pos, toDirection[n]);
				const piece_t neighPiece =
					hive_getexposedpiece(hive, &neighPos);
				const piece_t p =
					pairs[HIVE_GETNSIDE(piece)]
						[HIVE_GETNSIDE(neighPiece)];
				wattr_set(win, 0, p, NULL);

				const struct vec3 off = cellOffsets[n];
				mvwprintw(win, world.y + off.y,
					world.x + off.x,
					"%s", triangles[n]);
			}
		}
	prefresh(win, hive->winPos.y, hive->winPos.x, 0, 0, LINES - 1, COLS - 1);
}
