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

bool hive_move_list_contains(const HiveMoveList *list, Point from, Point to)
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
		{ .side = HIVE_BLACK, .type = HIVE_LADYBUG, .position = { 0, 1 } },
		{ .side = HIVE_BLACK, .type = HIVE_MOSQUITO, .position = { 1, 1 } },
		{ .side = HIVE_BLACK, .type = HIVE_PILLBUG, .position = { 2, 1 } },
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
		{ .side = HIVE_WHITE, .type = HIVE_LADYBUG, .position = { 0, 1 } },
		{ .side = HIVE_WHITE, .type = HIVE_MOSQUITO, .position = { 1, 1 } },
		{ .side = HIVE_WHITE, .type = HIVE_PILLBUG, .position = { 2, 1 } },
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
	for (size_t i = 0; i < ARRLEN(defaultBlackPieces); i++)
		hive_region_addpiece(&hive->blackInventory,
				&hive->allPieces[ARRLEN(defaultBlackPieces) + i]);

	hive_region_init(&hive->whiteInventory,
			x, y, w, h / 5);
	for (size_t i = 0; i < ARRLEN(defaultWhitePieces); i++)
		hive_region_addpiece(&hive->whiteInventory,
				&hive->allPieces[i]);

	hive_region_init(&hive->board, x, y + h / 5, w, h - 2 * (h / 5));
	return 0;
}

static bool hive_canmoveontop(Hive *hive, Point pos, int direction)
{
	/* atempt at slide rule on top of hive.
	 * moving on top of the hive requires different logic
	 * than moving up onto the hive.
	 * TODO: complete this. */
	HivePiece *at;
	HivePiece *pieces[6];
	HivePiece *piece;

	/* make sure we are actually moving on top. */
	at = hive_region_pieceat(&hive->board, pos);
	if (at == NULL)
		return false;

	(void) hive_region_getsurrounding(&hive->board, pos, pieces);

	hive_movepoint(&pos, hive_oppositedirection(direction));
	piece = hive_region_pieceat(&hive->board, pos);

	/* get the neighbors on the same level as the piece. */
	while (piece->above) {
		piece = piece->above;

		for (int d = 0; d < 6; d++)
			if (pieces[d] != NULL && pieces[d]->above != NULL)
				pieces[d] = pieces[d]->above;
	}

	/* check if the piece can pass through */
	switch (direction) {
	case HIVE_NORTH:
		if (pieces[HIVE_SOUTH_EAST] != NULL && 
				pieces[HIVE_SOUTH_EAST]->above != NULL &&
					pieces[HIVE_SOUTH_WEST] != NULL &&
						pieces[HIVE_SOUTH_WEST]->above != NULL)
			return false;
		break;
	case HIVE_SOUTH:
		if (pieces[HIVE_NORTH_EAST] != NULL && 
				pieces[HIVE_NORTH_EAST]->above != NULL &&
					pieces[HIVE_NORTH_WEST] != NULL &&
						pieces[HIVE_NORTH_WEST]->above != NULL)
			return false;
		break;
	case HIVE_NORTH_EAST:
		if (pieces[HIVE_SOUTH] != NULL &&
				pieces[HIVE_SOUTH]->above != NULL &&
					pieces[HIVE_NORTH_EAST] != NULL &&
						pieces[HIVE_NORTH_EAST]->above != NULL)
			return false;
		break;
	case HIVE_NORTH_WEST:
		if (pieces[HIVE_SOUTH] != NULL &&
				pieces[HIVE_SOUTH]->above != NULL &&
					pieces[HIVE_NORTH_EAST] != NULL &&
						pieces[HIVE_NORTH_EAST]->above != NULL)
			return false;
		break;
	case HIVE_SOUTH_EAST:
		if (pieces[HIVE_NORTH] != NULL &&
				pieces[HIVE_NORTH]->above != NULL &&
					pieces[HIVE_SOUTH_WEST] != NULL &&
						pieces[HIVE_SOUTH_WEST]->above != NULL)
			return false;
		break;
	case HIVE_SOUTH_WEST:
		if (pieces[HIVE_NORTH] != NULL &&
				pieces[HIVE_NORTH]->above != NULL &&
					pieces[HIVE_SOUTH_EAST] != NULL &&
						pieces[HIVE_SOUTH_EAST]->above != NULL)
			return false;
		break;
	}
	return true;
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

static void hive_moveexhaustive(Hive *hive,
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
			point_list_push(&hive->moves, pos);
		} else {
			const Point orig = piece->position;
			piece->position = pos;
			hive_moveexhaustive(hive, piece, d, left - 1);
			piece->position = orig;
		}
	}
}

static bool hive_findmovesant(Hive *hive, HivePiece *piece, PointList *visited)
{
	Point pos;

	for (int d = 0; d < 6; d++) {
		pos = piece->position;
		hive_movepoint(&pos, d);
		if (point_list_contains(visited, pos) ||
				!hive_canmoveto(hive, pos, d))
			continue;
		point_list_push(&hive->moves, pos);
		point_list_push(visited, pos);

		const Point orig = piece->position;
		piece->position = pos;
		if (!hive_findmovesant(hive, piece, visited)) {
			piece->position = orig;
			return false;
		}
		piece->position = orig;
	}
	return true;
}

static void hive_computemovesant(Hive *hive)
{
	PointList visited;

	memset(&visited, 0, sizeof(visited));
	point_list_push(&visited, hive->selectedPiece->position);
	hive_findmovesant(hive, hive->selectedPiece, &visited);
	free(visited.points);
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
		/* this condition only applies to beetles on the ground. */
		if (piece->below == NULL && 
				piece->neighbors[d] == NULL &&
					hive_region_getsurrounding(&hive->board, pos,
						pieces) == 1)
			continue;
		/* if moving on top make sure it can pass through. */
		if (piece->neighbors[d] != NULL && !hive_canmoveontop(hive, pos, d))
			continue;
		/* otherwise push neighbors. */
		point_list_push(&hive->moves, pos);
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
		point_list_push(&hive->moves, pos);
	}
}

struct hive_ladybug_keeper {
	Point start;
	HivePiece *piece;
	uint32_t numRemaining;
};

static void hive_findmovesladybug(Hive *hive, struct hive_ladybug_keeper *lk)
{
	Point pos;
	HivePiece *pieces[6];
	HivePiece *piece;

	hive_region_getsurrounding(&hive->board, lk->piece->position, pieces);
	for (int d = 0; d < 6; d++) {
		pos = lk->piece->position;
		hive_movepoint(&pos, d);
		if (point_isequal(lk->start, pos))
			continue;

		piece = pieces[d];
		switch (lk->numRemaining) {
		/* move over two non null pieces */
		case 3:
		case 2:
			if (piece == NULL)
				continue;
			break;
		/* drop down */
		case 1:
			if (piece != NULL)
				continue;
			point_list_push(&hive->moves, pos);
			break;
		}

		if (lk->numRemaining > 0) {
			const Point orig = lk->piece->position;
			lk->piece->position = pos;
			lk->numRemaining--;
			hive_findmovesladybug(hive, lk);
			lk->numRemaining++;
			lk->piece->position = orig;
		}
	}
}

static void hive_computemovesladybug(Hive *hive)
{
	struct hive_ladybug_keeper lk;

	memset(&lk, 0, sizeof(lk));
	lk.start = hive->selectedPiece->position;
	lk.piece = hive->selectedPiece;
	lk.numRemaining = 3;
	hive_findmovesladybug(hive, &lk);
}

static void hive_computemovesmosquito(Hive *hive)
{
	Point pos;
	
	for (int d = 0; d < 6; d++) {
		pos = hive->selectedPiece->position;
		hive_movepoint(&pos, d);
		HivePiece *const piece = hive_region_pieceat(&hive->board, pos);
		if (piece != NULL && piece->type != HIVE_MOSQUITO)
			point_list_push(&hive->choices, pos);
	}
}

static void hive_computemovespillbug(Hive *hive)
{
	Point pos;

	for (int d = 0; d < 6; d++) {
		pos = hive->selectedPiece->position;
		hive_movepoint(&pos, d);
		HivePiece *const piece = hive_region_pieceat(&hive->board, pos);
		if (piece != NULL && piece->below == NULL)
			point_list_push(&hive->choices, pos);
	}
}

static void hive_computemovespillbug_carrying(Hive *hive)
{
	HivePiece *selectedPiece;
	Point selectedPos;
	Point pos;

	selectedPiece = hive->selectedPiece;
	selectedPos = hive->selectedPiece->position;

	/* get pillbug pos. */
	for (int d = 0; d < 6; d++) {
		pos = selectedPos;
		hive_movepoint(&pos, d);
		HivePiece *const piece = hive_region_pieceat(&hive->board, pos);
		if (piece != NULL && piece->type == HIVE_PILLBUG)
			break;
	}
	const Point orig = pos;
	for (int d = 0; d < 6; d++) {
		pos = orig;
		hive_movepoint(&pos, d);
		HivePiece *const piece = hive_region_pieceat(&hive->board, pos);
		if (piece == NULL)
			point_list_push(&hive->moves, pos);
	}
}

static void hive_computemovesqueen(Hive *hive)
{
	hive_moveexhaustive(hive, hive->selectedPiece, -1, 1);
}

static void hive_computemovesspider(Hive *hive)
{
	hive_moveexhaustive(hive, hive->selectedPiece, -1, 3);
}

static bool hive_canmoveaway(Hive *hive, enum hive_type type)
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
	if (type != HIVE_BEETLE && type != HIVE_GRASSHOPPER &&
			type != HIVE_LADYBUG && type != HIVE_PILLBUG_CARRYING) {
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

void hive_computemoves(Hive *hive, enum hive_type type)
{
	static void (*const computes[])(Hive *hive) = {
		[HIVE_ANT] = hive_computemovesant,
		[HIVE_BEETLE] = hive_computemovesbeetle,
		[HIVE_GRASSHOPPER] = hive_computemovesgrasshopper,
		[HIVE_LADYBUG] = hive_computemovesladybug,
		[HIVE_MOSQUITO] = hive_computemovesmosquito,
		[HIVE_PILLBUG] = hive_computemovespillbug,
		[HIVE_PILLBUG_CARRYING] = hive_computemovespillbug_carrying,
		[HIVE_QUEEN] = hive_computemovesqueen,
		[HIVE_SPIDER] = hive_computemovesspider,
	};
	point_list_clear(&hive->moves);
	point_list_clear(&hive->choices);
	if (hive_canmoveaway(hive, type))
		computes[type](hive);
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

void hive_domove(Hive *hive, const HiveMove *move, bool doNotify)
{
	hive_region_removepiece(hive->selectedRegion,
			hive->selectedPiece);
	hive->selectedPiece->position = move->to;
	hive_region_addpiece(&hive->board, hive->selectedPiece);
	hive->selectedPiece = NULL;
	hive_move_list_push(&hive->history, move);
	hive->turn = hive->turn == HIVE_WHITE ? HIVE_BLACK : HIVE_WHITE;
	if (doNotify)
		hc_notifymove(hive, move);
}

static void hive_transferpiece(Hive *hive, HiveRegion *region, Point pos)
{
	HiveRegion *inventory;
	HiveMove move;

	if (hive->selectedPiece == NULL || region != &hive->board)
		return;
	inventory = hive->turn == HIVE_WHITE ? &hive->whiteInventory :
		&hive->blackInventory;
	move.fromInventory = hive->selectedRegion == inventory;
	move.from = hive->selectedPiece->position;
	move.to = pos;
	hive->selectedPiece->flags &= ~HIVE_SELECTED;
	/* transfer from inventory to board */
	if (hive->selectedRegion == inventory && hive_canplace(hive, pos)) {
		hive_domove(hive, &move, true);
	/* move on the board */
	} else if (hive->selectedRegion == &hive->board) {
		if (point_list_contains(&hive->moves, pos))
			hive_domove(hive, &move, true);
		else if (point_list_contains(&hive->choices, pos)) {
			HivePiece *const piece =
				hive_region_pieceat(&hive->board, pos);
			if (hive->selectedPiece->type == HIVE_MOSQUITO) {
				hive_computemoves(hive, piece->type);
			} else {
				hive->selectedPiece = piece;
				hive_computemoves(hive, HIVE_PILLBUG_CARRYING);
			}
		}
	}
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
		hive_computemoves(hive, piece->type);
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

bool hive_handlemousepress(Hive *hive, Point mouse)
{
	HivePiece *piece;
	HiveRegion *region;
	Point pos;

	region = hive_getregionat(hive, mouse);
	if (region == NULL) {
		hive->selectedPiece = NULL;
		return false;
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
			(point_list_contains(&hive->moves, pos) ||
			point_list_contains(&hive->choices, pos))) {
		hive_transferpiece(hive, region, pos);
		return true;
	}

	piece = hive_region_pieceat(region, pos);
	if (piece != NULL) {
		while (piece->above != NULL)
			piece = piece->above;
		hive_selectpiece(hive, region, piece);
	} else {
		hive_transferpiece(hive, region, pos);
	}
	return true;
}

int hive_handle(Hive *hive, int c)
{
	switch(c) {
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

		p = hive->moves.points[i];
		hive_pointtoworld(&p, hive->board.translation);
		wattr_set(hive->board.win, A_REVERSE, HIVE_PAIR_SELECTED, NULL);
		mvwaddstr(hive->board.win, p.y, p.x + 1, "   ");
		mvwaddstr(hive->board.win, p.y + 1, p.x + 1, "   ");
	}
	for (size_t i = 0; i < hive->choices.count; i++) {
		Point p;

		p = hive->choices.points[i];
		hive_pointtoworld(&p, hive->board.translation);
		wattr_set(hive->board.win, A_REVERSE, HIVE_PAIR_CHOICE, NULL);
		mvwaddstr(hive->board.win, p.y, p.x + 1, "   ");
		mvwaddstr(hive->board.win, p.y + 1, p.x + 1, "   ");
	}
	wnoutrefresh(hive->board.win);
}
