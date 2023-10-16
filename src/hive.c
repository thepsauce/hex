#include "hex.h"

void hive_movepoint(Point *point, int dir)
{
	static const Point directions[2][6] = {
		{
			{  0, -1 }, /* north */
			{  0,  1 }, /* south */

			{ -1, -1 }, /* north east */
			{  1, -1 }, /* north west */

			{ -1,  0 }, /* south east */
			{  1,  0 }, /* south west */
		},
		{
			{  0, -1 }, /* north */
			{  0,  1 }, /* south */

			{ -1,  0 }, /* north east */
			{  1,  0 }, /* north west */

			{ -1,  1 }, /* south east */
			{  1,  1 }, /* south west */
		},
	};
	const Point delta = directions[point->x % 2][dir];
	point_add(point, delta);
}

void hive_pointtoworld(Point *point, Point translation)
{
	point->y = point->y * 2 + point->x % 2;
	point->x *= 4;
	point_add(point, translation);
}

void hive_pointtogrid(Point *point, Point translation)
{
	point_subtract(point, translation);
	point->x /= 4;
	point->y = (point->y - point->x % 2) / 2;
}

void hive_move_list_push(HiveMoveList *list, const HiveMove *move)
{
	HiveMove *newMoves;

	newMoves = realloc(list->moves, sizeof(*list->moves) *
			(list->count + 1));
	if (newMoves == NULL)
		return;
	list->moves = newMoves;
	list->moves[list->count++] = *move;
}

bool hive_move_list_contains(HiveMoveList *list, Point from, Point to)
{
	for (size_t i = 0; i < list->count; i++) {
		const HiveMove m = list->moves[i];
		if (point_isequal(from, m.from) && point_isequal(to, m.to))
			return true;
	}
	return false;
}

void hive_move_list_clear(HiveMoveList *list)
{
	list->count = 0;
}

int hive_init(Hive *hive, int x, int y, int w, int h)
{
	static const HivePiece defaultBlackPieces[] = {
		{ .side = HIVE_BLACK, .type = HIVE_ANT, .position = { 8, 0 } },
		{ .side = HIVE_BLACK, .type = HIVE_ANT, .position = { 9, 0 } },
		{ .side = HIVE_BLACK, .type = HIVE_ANT, .position = { 10, 0 } },
		{ .side = HIVE_BLACK, .type = HIVE_BEETLE, .position = { 1, 0 } },
		{ .side = HIVE_BLACK, .type = HIVE_BEETLE, .position = { 2, 0 } },
		{ .side = HIVE_BLACK, .type = HIVE_GRASSHOPPER, .position = { 3, 0 } },
		{ .side = HIVE_BLACK, .type = HIVE_GRASSHOPPER, .position = { 4, 0 } },
		{ .side = HIVE_BLACK, .type = HIVE_GRASSHOPPER, .position = { 5, 0 } },
		{ .side = HIVE_BLACK, .type = HIVE_QUEEN, .position = { 0, 0 } },
		{ .side = HIVE_BLACK, .type = HIVE_SPIDER, .position = { 6, 0 } },
		{ .side = HIVE_BLACK, .type = HIVE_SPIDER, .position = { 7, 0 } },
	};

	static const HivePiece defaultWhitePieces[] = {
		{ .side = HIVE_WHITE, .type = HIVE_ANT, .position = { 8, 0 } },
		{ .side = HIVE_WHITE, .type = HIVE_ANT, .position = { 9, 0 } },
		{ .side = HIVE_WHITE, .type = HIVE_ANT, .position = { 10, 0 } },
		{ .side = HIVE_WHITE, .type = HIVE_BEETLE, .position = { 1, 0 } },
		{ .side = HIVE_WHITE, .type = HIVE_BEETLE, .position = { 2, 0 } },
		{ .side = HIVE_WHITE, .type = HIVE_GRASSHOPPER, .position = { 3, 0 } },
		{ .side = HIVE_WHITE, .type = HIVE_GRASSHOPPER, .position = { 4, 0 } },
		{ .side = HIVE_WHITE, .type = HIVE_GRASSHOPPER, .position = { 5, 0 } },
		{ .side = HIVE_WHITE, .type = HIVE_QUEEN, .position = { 0, 0 } },
		{ .side = HIVE_WHITE, .type = HIVE_SPIDER, .position = { 6, 0 } },
		{ .side = HIVE_WHITE, .type = HIVE_SPIDER, .position = { 7, 0 } },
	};
	memset(hive, 0, sizeof(*hive));
	memcpy(&hive->allPieces[ARRLEN(defaultWhitePieces)],
			defaultBlackPieces, sizeof(defaultBlackPieces));
	memcpy(hive->allPieces,
			defaultWhitePieces, sizeof(defaultWhitePieces));

	hive_region_init(&hive->blackInventory,
			x, y + h - h / 5, w, h / 5);
	for (int i = 0; i < 11; i++)
		hive_region_addpiece(&hive->blackInventory,
				&hive->allPieces[11 + i]);

	hive_region_init(&hive->whiteInventory,
			x, y, w, h / 5);
	for (int i = 0; i < 11; i++)
		hive_region_addpiece(&hive->whiteInventory,
				&hive->allPieces[i]);

	hive_region_init(&hive->board, x, y + h / 5, w, h - 2 * (h / 5));
	return 0;
}

static bool hive_canmoveto(Hive *hive, Point pos, int direction)
{
	HivePiece *at;
	HivePiece *pieces[6];

	/* make sure to not pass through pieces */
	at = hive_region_pieceat(&hive->board, pos);
	if (at != NULL)
		return false;
	/* check if moving there would isolate the piece */
	if (hive_region_getsurrounding(&hive->board, pos, pieces) == 1)
		return false;
	/* check if the piece can pass through */
	switch (direction) {
	case HIVE_NORTH:
		if (pieces[HIVE_SOUTH_EAST] != NULL &&
				pieces[HIVE_SOUTH_WEST] != NULL)
			return false;
		break;
	case HIVE_SOUTH:
		if (pieces[HIVE_NORTH_EAST] != NULL &&
				pieces[HIVE_NORTH_WEST] != NULL)
			return false;
		break;
	case HIVE_NORTH_EAST:
		if (pieces[HIVE_SOUTH] != NULL &&
				pieces[HIVE_NORTH_EAST] != NULL)
			return false;
		break;
	case HIVE_NORTH_WEST:
		if (pieces[HIVE_SOUTH] != NULL &&
				pieces[HIVE_NORTH_EAST] != NULL)
			return false;
		break;
	case HIVE_SOUTH_EAST:
		if (pieces[HIVE_NORTH] != NULL &&
				pieces[HIVE_SOUTH_WEST] != NULL)
			return false;
		break;
	case HIVE_SOUTH_WEST:
		if (pieces[HIVE_NORTH] != NULL &&
				pieces[HIVE_SOUTH_EAST] != NULL)
			return false;
		break;
	}
	return true;
}

static void hive_moveexhaustive(Hive *hive, Point start,
		HivePiece *piece, int fromDir, uint32_t left)
{
	Point pos;

	const int od = hive_oppositedirection(fromDir);
	for (int d = 0; d < 6; d++) {
		if (d == od)
			continue;
		pos = piece->position;
		hive_movepoint(&pos, d);
		if (!hive_canmoveto(hive, pos, d))
		       continue;	
		if (left == 1) {
			hive_move_list_push(&hive->moves, &(HiveMove) {
				false, start, pos
			});
		} else {
			const Point orig = piece->position;
			piece->position = pos;
			hive_moveexhaustive(hive, start, piece, d, left - 1);
			piece->position = orig;
		}
	}
}

struct hive_ant_keeper {
	Point start;
	HivePiece *piece;
	Point *positions;
	size_t numPositions;
};

static bool hive_findmovesant(Hive *hive, struct hive_ant_keeper *ak)
{
	Point *newPositions;
	Point pos;

	for (int d = 0; d < 6; d++) {
		bool hasVisited = false;

		pos = ak->piece->position;
		hive_movepoint(&pos, d);
		if (point_isequal(pos, ak->start))
			continue;
		for (size_t i = 0; i < ak->numPositions; i++)
			if (point_isequal(ak->positions[i], pos)) {
				hasVisited = true;
				break;
			}
		if (hasVisited || !hive_canmoveto(hive, pos, d))
		       continue;

		hive_move_list_push(&hive->moves, &(HiveMove) {
			false, ak->start, pos
		});
		newPositions = realloc(ak->positions, sizeof(*ak->positions) *
				(ak->numPositions + 1));
		if (newPositions == NULL)
			return false;
		ak->positions = newPositions;
		ak->positions[ak->numPositions++] = pos;

		const Point orig = ak->piece->position;
		ak->piece->position = pos;
		if (!hive_findmovesant(hive, ak)) {
			ak->piece->position = orig;
			return false;
		}
		ak->piece->position = orig;
	}
	return true;
}

static void hive_computemovesant(Hive *hive)
{
	struct hive_ant_keeper ak;

	memset(&ak, 0, sizeof(ak));
	ak.start = hive->selectedPiece->position;
	ak.piece = hive->selectedPiece;
	hive_findmovesant(hive, &ak);
	free(ak.positions);
}

static void hive_computemovesbeetle(Hive *hive)
{
	HivePiece *piece;
	Point pos;
	HivePiece *pieces[6];

	piece = hive->selectedPiece;
	for (int d = 0; d < 6; d++) {
		pos = piece->position;
		hive_movepoint(&pos, d);
		if (piece->neighbors[d] == NULL &&
				hive_region_getsurrounding(&hive->board, pos,
					pieces) == 1)
			continue;
		hive_move_list_push(&hive->moves, &(HiveMove) {
			false, piece->position, pos
		});
	}
}

static void hive_computemovesgrasshopper(Hive *hive)
{
	HivePiece *piece;
	Point pos;

	piece = hive->selectedPiece;
	for (int d = 0; d < 6; d++) {
		if (piece->neighbors[d] == NULL)
			continue;
		pos = piece->position;
		for (HivePiece *p = piece; p != NULL; p = p->neighbors[d])
			hive_movepoint(&pos, d);
		hive_move_list_push(&hive->moves, &(HiveMove) {
			false, piece->position, pos
		});
	}
}

static void hive_computemovesqueen(Hive *hive)
{
	hive_moveexhaustive(hive, hive->selectedPiece->position,
			hive->selectedPiece, -1, 1);
}

static void hive_computemovesspider(Hive *hive)
{
	hive_moveexhaustive(hive, hive->selectedPiece->position,
			hive->selectedPiece, -1, 3);
}

static bool hive_canmoveaway(Hive *hive)
{
	HivePiece *piece;
	int dir;

	piece = hive->selectedPiece;
	if (piece->above != NULL)
		/* should in theory never happen because
		 * a piece below can't be selected
		 */
		return false;
	if (piece->below != NULL)
		/* the piece is on top and can freely move */
		return true;
	/* check if enclosed (grasshopper and beetle don't care)
	 * this is solely an optimization!
	 */
	if (piece->type != HIVE_BEETLE && piece->type != HIVE_GRASSHOPPER) {
		if (piece->north != NULL &&
				piece->southWest != NULL &&
				piece->southEast != NULL)
			return false;
		if (piece->south != NULL &&
				piece->northWest != NULL &&
				piece->northEast != NULL)
			return false;
	}
	/* check if this would break the hive */
	hive_region_clearflags(&hive->board, HIVE_VISITED);
	piece->flags |= HIVE_VISITED;
	/* pick any direction to go in */
	dir = -1;
	for (int d = 0; d < 6; d++)
		if (piece->neighbors[d] != NULL) {
			dir = d;
			break;
		}
	if (dir == -1)
		/* there are no pieces to move */
		return false;
	piece = piece->neighbors[dir];
	return hive_region_count(&hive->board, piece) ==
		hive->board.numPieces - 1;
}

void hive_computemoves(Hive *hive)
{
	static const void (*computes[])(Hive *hive) = {
		[HIVE_ANT] = hive_computemovesant,
		[HIVE_BEETLE] = hive_computemovesbeetle,
		[HIVE_GRASSHOPPER] = hive_computemovesgrasshopper,
		[HIVE_QUEEN] = hive_computemovesqueen,
		[HIVE_SPIDER] = hive_computemovesspider,
	};
	hive_move_list_clear(&hive->moves);
	if (hive_canmoveaway(hive))
		computes[hive->selectedPiece->type](hive);
}

static bool hive_canplace(Hive *hive, Point pos)
{
	HivePiece *surrounding[6];
	bool affirm;

	if (hive->board.numPieces == 0)
		return true;
	if (hive_region_pieceat(&hive->board, pos) != NULL)
		return false;
	if (hive_region_getsurrounding(&hive->board, pos, surrounding) == 1 &&
			hive->board.numPieces == 1)
		return true;
	affirm = false;
	for (size_t i = 0; i < 6; i++) {
		HivePiece *const p = surrounding[i];
		if (p == NULL)
			continue;
		if (p->side != hive->turn)
			return false;
		if (p->side == hive->turn)
			affirm = true;
	}
	return affirm;
}

static void hive_transferpiece(Hive *hive, HiveRegion *region, Point pos)
{
	HiveRegion *inventory;
	Point from;

	if (hive->selectedPiece == NULL || region != &hive->board)
		return;
	from = hive->selectedPiece->position;
	inventory = hive->turn == HIVE_WHITE ? &hive->whiteInventory :
		&hive->blackInventory;
	hive->selectedPiece->flags &= ~HIVE_SELECTED;
	/* transfer from inventory to board */
	if (hive->selectedRegion == inventory && hive_canplace(hive, pos))
		goto do_move;
	/* move on the board */
	if (hive->selectedRegion == &hive->board &&
			hive_move_list_contains(&hive->moves, from, pos))
		goto do_move;
	return;
do_move:
	hive_region_removepiece(hive->selectedRegion,
			hive->selectedPiece);
	hive->selectedPiece->position = pos;
	hive_region_addpiece(region, hive->selectedPiece);
	hive->selectedPiece = NULL;
	hive->turn = hive->turn == HIVE_WHITE ? HIVE_BLACK : HIVE_WHITE;
	hive_move_list_push(&hive->history, &(HiveMove) {
		hive->selectedRegion == inventory, from, pos
	});
}

static void hive_selectpiece(Hive *hive, HiveRegion *region, HivePiece *piece)
{
	if (hive->selectedPiece != NULL) {
		hive->selectedPiece->flags &= ~HIVE_SELECTED;
		if (hive->selectedPiece == piece) {
			hive->selectedPiece = NULL;
			return;
		}
	}
	if (piece->side != hive->turn)
		return;
	hive->selectedRegion = region;
	hive->selectedPiece = piece;
	piece->flags |= HIVE_SELECTED;
	if (region == &hive->board)
		hive_computemoves(hive);
}

static HiveRegion *hive_getregionat(Hive *hive, Point at)
{
	for (size_t i = 0; i < ARRLEN(hive->regions); i++) {
		Point pos;

		HiveRegion *const region = &hive->regions[i];
		pos = at;
		if (wmouse_trafo(region->win, &pos.y, &pos.x, false))
			return region;
	}
	return NULL;
}

static void hive_handlemousepress(Hive *hive, Point mouse)
{
	HivePiece *piece;
	HiveRegion *region;
	Point pos;

	region = hive_getregionat(hive, mouse);
	if (region == NULL) {
		hive->selectedPiece = NULL;
		return;
	}
	pos = mouse;
	wmouse_trafo(region->win, &pos.y, &pos.x, false);
	hive_pointtogrid(&pos, region->translation);
	/* need to check for a special case where the piece
	 * moves on top of another piece
	 */
	if (hive->selectedPiece != NULL &&
			hive->selectedRegion == &hive->board &&
			region == &hive->board &&
			hive_move_list_contains(&hive->moves,
				hive->selectedPiece->position, pos)) {
		hive_transferpiece(hive, region, pos);
		return;
	}

	piece = hive_region_pieceat(region, pos);
	if (piece != NULL) {
		while (piece->above != NULL)
			piece = piece->above;
		hive_selectpiece(hive, region, piece);
	} else {
		hive_transferpiece(hive, region, pos);
	}
}

int hive_handle(Hive *hive, int c)
{
	MEVENT ev;

	switch(c) {
	case KEY_MOUSE:
		getmouse(&ev);
		if ((ev.bstate & BUTTON1_CLICKED) ||
				(ev.bstate & BUTTON1_PRESSED))
			hive_handlemousepress(hive, (Point) { ev.x, ev.y });
		break;
	}
	return 0;
}

void hive_render(Hive *hive)
{
	for (size_t i = 0; i < ARRLEN(hive->regions); i++)
		hive_region_render(&hive->regions[i]);
	/* render all possible moves */
	if (hive->selectedPiece == NULL ||
			hive->selectedRegion != &hive->board)
		return;
	for (size_t i = 0; i < hive->moves.count; i++) {
		Point p;

		p = hive->moves.moves[i].to;
		hive_pointtoworld(&p, hive->board.translation);
		wattr_set(hive->board.win, A_REVERSE, HIVE_PAIR_SELECTED, NULL);
		mvwaddstr(hive->board.win, p.y, p.x + 1, "   ");
		mvwaddstr(hive->board.win, p.y + 1, p.x + 1, "   ");
	}
}
