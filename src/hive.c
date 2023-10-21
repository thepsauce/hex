#include "hex.h"

void hive_movepoint(Point *point, int dir)
{
	static const Point directions[2][6] = {
		{
			[HIVE_NORTH] = {  0, -1 },
			[HIVE_SOUTH] = {  0,  1 },
			[HIVE_NORTH_EAST] = { -1, -1 },
			[HIVE_NORTH_WEST] = {  1, -1 },
			[HIVE_SOUTH_EAST] = { -1,  0 },
			[HIVE_SOUTH_WEST] = {  1,  0 },
		},
		{
			[HIVE_NORTH] = {  0, -1 },
			[HIVE_SOUTH] = {  0,  1 },
			[HIVE_NORTH_EAST] = { -1,  0 },
			[HIVE_NORTH_WEST] = {  1,  0 },
			[HIVE_SOUTH_EAST] = { -1,  1 },
			[HIVE_SOUTH_WEST] = {  1,  1 },
		},
	};
	const Point delta = directions[(point->x & 1)][dir];
	point_add(point, delta);
}

void hive_pointtoworld(Point *point, Point translation)
{
	point->y = point->y * 2 + (point->x & 1);
	point->x *= 4;
	point_add(point, translation);
}

void hive_pointtogrid(Point *point, Point translation)
{
	point_subtract(point, translation);
	point->x = point->x < 0 ? point->x / 4 - 1 : point->x / 4;
	point->y = (point->y - (point->x & 1)) / 2;
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

static const HivePiece default_black_pieces[] = {
	{ .side = HIVE_BLACK, .type = HIVE_ANT, .position = { 5, 1 } },
	{ .side = HIVE_BLACK, .type = HIVE_ANT, .position = { 6, 1 } },
	{ .side = HIVE_BLACK, .type = HIVE_ANT, .position = { 7, 1 } },
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

static const HivePiece default_white_pieces[] = {
	{ .side = HIVE_WHITE, .type = HIVE_ANT, .position = { 5, 1 } },
	{ .side = HIVE_WHITE, .type = HIVE_ANT, .position = { 6, 1 } },
	{ .side = HIVE_WHITE, .type = HIVE_ANT, .position = { 7, 1 } },
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

int hive_init(Hive *hive, int x, int y, int w, int h)
{
	Point cur;

	memset(hive, 0, sizeof(*hive));
	memcpy(&hive->allPieces[ARRLEN(default_white_pieces)],
			default_black_pieces, sizeof(default_black_pieces));
	memcpy(hive->allPieces,
			default_white_pieces, sizeof(default_white_pieces));

	hive_region_init(&hive->blackInventory,
			x, y + h - h / 5, w, h / 5);
	for (size_t i = 0; i < ARRLEN(default_black_pieces); i++)
		hive_region_addpiece(&hive->blackInventory,
			&hive->allPieces[ARRLEN(default_black_pieces) + i]);

	hive_region_init(&hive->whiteInventory,
			x, y, w, h / 5);
	for (size_t i = 0; i < ARRLEN(default_white_pieces); i++)
		hive_region_addpiece(&hive->whiteInventory,
				&hive->allPieces[i]);

	hive_region_init(&hive->board, x, y + h / 5, w, h - 2 * (h / 5));
	cur = (Point) { w / 2, h / 4 };
	hive_pointtogrid(&cur, hive->board.translation);
	hive->hexCursor = cur;
	return 0;
}

void hive_reset(Hive *hive)
{
	for (size_t i = 0; i < ARRLEN(hive->regions); i++)
		hive->regions[i].numPieces = 0;

	for (size_t i = 0; i < ARRLEN(default_black_pieces); i++)
		hive_region_addpiece(&hive->blackInventory,
				&hive->allPieces[ARRLEN(default_black_pieces) + i]);
	for (size_t i = 0; i < ARRLEN(default_white_pieces); i++)
		hive_region_addpiece(&hive->whiteInventory,
				&hive->allPieces[i]);
	hive->selectedPiece = NULL;
	hive->turn = HIVE_BLACK;
	hive->moves.count = 0;
	hive->choices.count = 0;
	hive->history.count = 0;
}

struct keeper {
	Point start;
	HivePiece *piece;
	bool addAll;
	bool needsPivot;
	PointList visited;
	uint32_t distance;
	int fromDirection;
};

/* note: pos is the position that is already moved towards dir */
static bool hive_canmoveto(Hive *hive, Point pos, int dir, bool needsPivot)
{
	HivePiece *at;
	HivePiece *pieces[6];
	HivePiece *front[2];

	/* make sure to not pass through pieces */
	at = hive_region_pieceat(&hive->board, NULL, pos);
	if (at != NULL)
		return false;

	/* check if moving there would isolate the piece */
	if (hive_region_getsurrounding(&hive->board, pos, pieces) == 1)
		return false;

	switch (dir) {
	case HIVE_NORTH:
		front[0] = pieces[HIVE_SOUTH_EAST];
		front[1] = pieces[HIVE_SOUTH_WEST];
		break;
	case HIVE_SOUTH:
		front[0] = pieces[HIVE_NORTH_EAST];
		front[1] = pieces[HIVE_NORTH_WEST];
		break;
	case HIVE_NORTH_EAST:
		front[0] = pieces[HIVE_SOUTH];
		front[1] = pieces[HIVE_NORTH_WEST];
		break;
	case HIVE_NORTH_WEST:
		front[0] = pieces[HIVE_SOUTH];
		front[1] = pieces[HIVE_NORTH_EAST];
		break;
	case HIVE_SOUTH_EAST:
		front[0] = pieces[HIVE_NORTH];
		front[1] = pieces[HIVE_SOUTH_WEST];
		break;
	case HIVE_SOUTH_WEST:
		front[0] = pieces[HIVE_NORTH];
		front[1] = pieces[HIVE_SOUTH_EAST];
		break;
	}
	/* check if the piece can pass through */
	if (front[0] != NULL && front[1] != NULL)
		return false;
	/* check if there is a pivot to move around
	 * (not every piece like the queen needs a pivot to move)
	 */
	if (needsPivot && front[0] == NULL && front[1] == NULL)
		return false;
	return true;
}

static void hive_moveexhaustive_recursive(Hive *hive, struct keeper *k)
{
	Point pos;

	const int od = hive_oppositedirection(k->fromDirection);
	for (int d = 0; d < 6; d++) {
		if (d == od)
			continue;
		pos = k->piece->position;
		hive_movepoint(&pos, d);
		if (point_list_contains(&k->visited, pos))
			continue;
		if (!hive_canmoveto(hive, pos, d, k->needsPivot))
			continue;
		point_list_push(&k->visited, pos);
		if (k->distance == 1 || k->addAll)
			point_list_push(&hive->moves, pos);
		if (k->distance > 1) {
			const Point origPos = k->piece->position;
			const int origFromDir = k->fromDirection;
			k->piece->position = pos;
			k->distance--;
			k->fromDirection = d;
			hive_moveexhaustive_recursive(hive, k);
			k->fromDirection = origFromDir;
			k->distance++;
			k->piece->position = origPos;
		}
	}
}

void hive_moveexhaustive(Hive *hive, HivePiece *piece, bool addAll,
		bool needsPivot, uint32_t dist)
{
	struct keeper k;

	memset(&k, 0, sizeof(k));
	point_list_push(&k.visited, piece->position);
	k.start = piece->position;
	k.piece = piece;
	k.addAll = addAll;
	k.needsPivot = needsPivot;
	k.distance = dist;
	k.fromDirection = -1;
	hive_moveexhaustive_recursive(hive, &k);
	free(k.visited.points);
}

static void hive_computemovesant(Hive *hive)
{
	hive_moveexhaustive(hive, hive->selectedPiece, true, false, UINT32_MAX);
}

/* note: pos is the position that is already moved towards dir */
static bool hive_canmoveontop(Hive *hive, Point pos, int dir)
{
	HivePiece *pieces[6];
	Point prevPos;
	HivePiece *front[3];
	int cnts[3];
	int cnt;

	prevPos = pos;
	hive_movepoint(&prevPos, hive_oppositedirection(dir));
	if (hive_region_getsurrounding(&hive->board, pos, pieces) == 1 &&
			hive_region_countat(&hive->board, prevPos) == 1)
		return false;

	front[1] = hive_region_pieceat(&hive->board, NULL, pos);
	switch (dir) {
	case HIVE_NORTH:
		front[0] = pieces[HIVE_SOUTH_EAST];
		front[2] = pieces[HIVE_SOUTH_WEST];
		break;
	case HIVE_SOUTH:
		front[0] = pieces[HIVE_NORTH_EAST];
		front[2] = pieces[HIVE_NORTH_WEST];
		break;
	case HIVE_NORTH_EAST:
		front[0] = pieces[HIVE_SOUTH];
		front[2] = pieces[HIVE_NORTH_WEST];
		break;
	case HIVE_NORTH_WEST:
		front[0] = pieces[HIVE_SOUTH];
		front[2] = pieces[HIVE_NORTH_EAST];
		break;
	case HIVE_SOUTH_EAST:
		front[0] = pieces[HIVE_NORTH];
		front[2] = pieces[HIVE_SOUTH_WEST];
		break;
	case HIVE_SOUTH_WEST:
		front[0] = pieces[HIVE_NORTH];
		front[2] = pieces[HIVE_SOUTH_EAST];
		break;
	}

	for (uint32_t i = 0; i < ARRLEN(front); i++)
		cnts[i] = front[i] == NULL ? 0 :
			hive_region_countat(&hive->board, front[i]->position);
	hive_movepoint(&pos, hive_oppositedirection(dir));
	cnt = hive_region_countat(&hive->board, pos);
	return MAX(cnts[1], cnt) >= MIN(cnts[0], cnts[2]);
}

static void hive_computemovesbeetle(Hive *hive)
{
	Point pos;

	for (int d = 0; d < 6; d++) {
		pos = hive->selectedPiece->position;
		hive_movepoint(&pos, d);
		if (!hive_canmoveontop(hive, pos, d))
			continue;
		point_list_push(&hive->moves, pos);
	}
}

static void hive_computemovesgrasshopper(Hive *hive)
{
	Point pos;
	uint32_t cnt;

	for (int d = 0; d < 6; d++) {
		pos = hive->selectedPiece->position;
		cnt = 0;
		do {
			cnt++;
			hive_movepoint(&pos, d);
		} while (hive_region_pieceat(&hive->board, NULL, pos) != NULL);
		if (cnt == 1)
			continue;
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
			if (!hive_canmoveontop(hive, pos, d))
				continue;
			break;
		/* drop down */
		case 1:
			if (piece != NULL)
				continue;
			if (!hive_canmoveontop(hive, pos, d))
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
	HivePiece *pieces[6];

	if (hive_region_getbelow(&hive->board, hive->selectedPiece) != NULL) {
		hive_computemovesbeetle(hive);
		return;
	}
	hive_region_getsurrounding(&hive->board, hive->selectedPiece->position,
			pieces);
	for (int d = 0; d < 6; d++) {
		HivePiece *const piece = pieces[d];
		if (piece == NULL || piece->type == HIVE_MOSQUITO)
			continue;
		point_list_push(&hive->choices, piece->position);
	}
}

static void hive_computemovespillbug(Hive *hive)
{
	HivePiece *pieces[6];

	const Point last = hive->history.count == 0 ?
		(Point) { INT_MIN, INT_MIN } :
		hive->history.moves[hive->history.count - 1].to;
	hive_moveexhaustive(hive, hive->selectedPiece, false, false, 1);
	/* can't carry a piece if the pillbug was recently moved */
	if (point_isequal(last, hive->selectedPiece->position))
		return;
	hive_region_getsurrounding(&hive->board, hive->selectedPiece->position,
			pieces);
	for (int d = 0; d < 6; d++) {
		HivePiece *const piece = pieces[d];
		if (piece == NULL)
			continue;
		/* can't carry a recently moved piece */
		if (point_isequal(last, piece->position))
			return;
		if (hive_region_getbelow(&hive->board, piece) == NULL ||
				!hive_canmoveontop(hive,
					hive->selectedPiece->position,
					hive_oppositedirection(d)))
			continue;
		point_list_push(&hive->choices, piece->position);
	}
}

static void hive_computemovespillbug_carrying(Hive *hive)
{
	HivePiece *pieces[6];
	Point pos;

	hive_region_getsurrounding(&hive->board, hive->selectedPiece->position,
			pieces);
	for (int d = 0; d < 6; d++) {
		HivePiece *const piece = pieces[d];
		if (piece != NULL)
			continue;
		pos = hive->actor->position;
		hive_movepoint(&pos, d);
		point_list_push(&hive->moves, pos);
	}
}

static void hive_computemovesqueen(Hive *hive)
{
	hive_moveexhaustive(hive, hive->selectedPiece, false, false, 1);
}

static void hive_computemovesspider(Hive *hive)
{
	hive_moveexhaustive(hive, hive->selectedPiece, false, true, 3);
}

static bool hive_canmoveaway(Hive *hive)
{
	size_t i;
	HivePiece *piece;
	HivePiece *pieces[6];
	int dir;

	/* check if the queen was placed already */
	for (i = 0; i < hive->board.numPieces; i++) {
		piece = hive->board.pieces[i];
		if (piece->type == HIVE_QUEEN && piece->side == hive->turn)
			break;
	}
	if (i == hive->board.numPieces)
		return false;
	piece = hive->selectedPiece;
	if (hive_region_getbelow(&hive->board, piece) != NULL)
		return true;
	/* check if this would break the hive */
	hive_region_clearflags(&hive->board, HIVE_VISITED);
	piece->flags |= HIVE_VISITED;
	/* pick any direction to go in */
	dir = -1;
	hive_region_getsurrounding(&hive->board, piece->position, pieces);
	for (int d = 0; d < 6; d++)
		if (pieces[d] != NULL) {
			dir = d;
			break;
		}
	if (dir == -1)
		/* there are no pieces to move */
		return false;
	piece = pieces[dir];
	return hive_region_count(&hive->board, piece) ==
		hive->board.numPieces - 1;
}

bool hive_isqueensurrounded(Hive *hive)
{
	HivePiece *pieces[6];

	for (size_t i = 0; i < hive->board.numPieces; i++) {
		HivePiece *const piece = hive->board.pieces[i];
		if (piece->side == hive->turn && piece->type == HIVE_QUEEN)
			return hive_region_getsurrounding(&hive->board,
					piece->position, pieces) == 6;
	}
	return false;
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
	bool wouldBreak;

	point_list_clear(&hive->moves);
	point_list_clear(&hive->choices);
	if (hive_isqueensurrounded(hive))
		return;
	/* we have to check for the pillbug because it has
	 * the option to not move itself
	 */
	wouldBreak = !hive_canmoveaway(hive);
	if (!wouldBreak || type == HIVE_PILLBUG)
		computes[type](hive);
	if (wouldBreak)
		hive->moves.count = 0;
}

static bool hive_canplace(Hive *hive, HivePiece *piece, Point pos)
{
	HivePiece *pieces[6];
	bool affirm;

	if (hive->board.numPieces == 0)
		return true;
	if (hive_region_pieceat(&hive->board, NULL, pos) != NULL)
		return false;
	if (hive_region_getsurroundingr(&hive->board, pos, pieces) == 1 &&
			hive->board.numPieces == 1)
		return true;
	affirm = false;
	for (int d = 0; d < 6; d++) {
		HivePiece *const piece = pieces[d];
		if (piece == NULL)
			continue;
		if (piece->side != hive->turn)
			return false;
		if (piece->side == hive->turn)
			affirm = true;
	}
	return affirm;
}

static void hive_computeplaces(Hive *hive)
{
	Point pos;

	point_list_clear(&hive->moves);
	point_list_clear(&hive->choices);
	for (size_t i = 0; i < hive->board.numPieces; i++) {
		HivePiece *const piece = hive->board.pieces[i];
		if (piece->side != hive->turn && hive->board.numPieces > 1)
			continue;
		for (int d = 0; d < 6; d++) {
			pos = piece->position;
			hive_movepoint(&pos, d);
			if (!hive_canplace(hive, piece, pos))
				continue;
			point_list_push(&hive->moves, pos);
		}
	}
}

static bool hive_hasanymoves(Hive *hive)
{
	if (hive->board.numPieces <= 3)
		return true;
	if (hive_getinventory(hive)->numPieces > 0) {
		hive_computeplaces(hive);
		if (hive->moves.count > 0)
			return true;
	}
	for (size_t i = 0; i < hive->board.numPieces; i++) {
		HivePiece *const piece = hive->board.pieces[i];
		if (piece->side != hive->turn)
			continue;
		hive->selectedPiece = piece;
		hive_computemoves(hive, piece->type);
		if (hive->moves.count > 0) {
			hive->selectedPiece = NULL;
			return true;
		}
		if (hive->choices.count == 0 || piece->type != HIVE_PILLBUG)
			continue;
		const size_t cnt = hive->choices.count;
		for (size_t c = 0; c < cnt; c++) {
			hive->selectedPiece =
				hive_region_pieceatr(&hive->board, NULL,
						hive->choices.points[c]);
			hive_computemoves(hive, HIVE_PILLBUG_CARRYING);
			if (hive->moves.count > 0) {
				hive->selectedPiece = NULL;
				return true;
			}
		}
	}
	hive->selectedPiece = NULL;
	return false;
}

static void hive_selectpiece(Hive *hive, HiveRegion *region, HivePiece *piece)
{
	if (hive->actor != NULL) {
		hive->actor->flags &= ~HIVE_ISACTOR;
		hive->actor = NULL;
	}
	if (hive->selectedPiece != NULL) {
		hive->selectedPiece->flags &= ~HIVE_SELECTED;
		if (hive->selectedPiece == piece) {
			hive->selectedPiece = NULL;
			return;
		}
	}
	if (piece == NULL || piece->side != hive->turn) {
		hive->selectedPiece = NULL;
		return;
	}
	hive->selectedRegion = region;
	hive->selectedPiece = piece;
	piece->flags |= HIVE_SELECTED;
	if (region == &hive->board) {
		hive_computemoves(hive, piece->type);
	} else {
		if (piece->type != HIVE_QUEEN) {
			size_t cnt;
			bool hasQueen;

			/* count the number of pieces of this side to
			 * check if the queen needs to be placed */
			cnt = 0;
			hasQueen = false;
			for (size_t i = 0; i < hive->board.numPieces; i++) {
				HivePiece *const piece = hive->board.pieces[i];
				if (piece->side != hive->turn)
					continue;
				if (piece->type == HIVE_QUEEN) {
					hasQueen = true;
					break;
				}
				cnt++;
			}
			/* cnt == 3 means that three turns were played already,
			 * meaning the queen has to be placed now
			 */
			if (!hasQueen && cnt == 3)
				return;
		}
		hive_computeplaces(hive);
	}
}

void hive_domove(Hive *hive, const HiveMove *move, bool doNotify)
{
	hive->selectedRegion = move->fromInventory ? hive_getinventory(hive) :
		&hive->board;
	hive->selectedPiece = hive_region_pieceatr(hive->selectedRegion,
			NULL, move->from);
	hive_region_removepiece(hive->selectedRegion,
			hive->selectedPiece);
	hive->selectedPiece->position = move->to;
	hive_region_addpiece(&hive->board, hive->selectedPiece);
	hive_selectpiece(hive, NULL, NULL);
	hive_move_list_push(&hive->history, move);
	hive->turn = hive->turn == HIVE_WHITE ? HIVE_BLACK : HIVE_WHITE;
	if (!hive_hasanymoves(hive))
		hive->turn = hive->turn == HIVE_WHITE ? HIVE_BLACK : HIVE_WHITE;
	if (doNotify)
		hc_notifymove(hive, move);
}

static bool hive_transferpiece(Hive *hive, HiveRegion *region, Point pos)
{
	HiveRegion *inventory;
	HiveMove move;

	if (hive->selectedPiece == NULL || region != &hive->board)
		return false;
	inventory = hive_getinventory(hive);
	move.fromInventory = hive->selectedRegion == inventory;
	move.from = hive->selectedPiece->position;
	move.to = pos;
	hive->selectedPiece->flags &= ~HIVE_SELECTED;
	if (point_list_contains(&hive->moves, pos) ||
			hive->board.numPieces == 0) {
		hive_domove(hive, &move, true);
		return true;
	}
	if (point_list_contains(&hive->choices, pos)) {
		HivePiece *const piece =
			hive_region_pieceatr(&hive->board, NULL, pos);
		if (hive->selectedPiece->type == HIVE_MOSQUITO &&
				/* if the actor is not null, the
				 * mosquito is trying to mimic a pillbug
				 */
				hive->actor == NULL) {
			hive->actor = hive->selectedPiece;
			hive->actor->flags |= HIVE_ISACTOR;
			hive_computemoves(hive, piece->type);
		} else {
			hive->actor = hive->selectedPiece;
			hive->actor->flags |= HIVE_ISACTOR;
			hive->selectedPiece->flags &= ~HIVE_SELECTED;
			hive->selectedPiece = piece;
			piece->flags |= HIVE_SELECTED;
			hive_computemoves(hive, HIVE_PILLBUG_CARRYING);
		}
		return true;
	}
	return false;
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

static void hive_pressposition(Hive *hive, HiveRegion *region, Point pos)
{
	HivePiece *piece;

	if (!hive_transferpiece(hive, region, pos)) {
		piece = hive_region_pieceatr(region, NULL, pos);
		hive_selectpiece(hive, region, piece);
	}
}

bool hive_handlemousepress(Hive *hive, int button, Point mouse)
{
	HiveRegion *region;
	Point pos;

	region = hive_getregionat(hive, mouse);
	if (region == NULL) {
		hive_selectpiece(hive, NULL, NULL);
		return false;
	}
	pos = mouse;
	wmouse_trafo(region->win, &pos.y, &pos.x, false);
	hive_pointtogrid(&pos, region->translation);
	switch (button) {
	case 0:
		hive_pressposition(hive, region, pos);
		break;
	}
	return true;
}

static void hive_selectnexttype(Hive *hive, HiveRegion *region,
		enum hive_type type)
{
	HivePiece *cur;
	size_t i;

	cur = hive->selectedPiece;
	if (region != hive->selectedRegion)
		cur = NULL;
	if (cur != NULL) {
		for (i = 0; i < region->numPieces; i++)
			if (region->pieces[i] == cur)
				break;
		for (i++; i < region->numPieces; i++) {
			HivePiece *const piece = region->pieces[i];
			if (hive_region_getabove(region, piece) != NULL)
				continue;
			if (piece->side != hive->turn || piece->type != type)
				continue;
			hive_selectpiece(hive, region, piece);
			return;
		}
	}

	for (i = 0; i < region->numPieces; i++) {
		HivePiece *const piece = region->pieces[i];
		if (piece == cur)
			break;
		if (hive_region_getabove(region, piece) != NULL)
			continue;
		if (piece->side != hive->turn || piece->type != type)
			continue;
		hive_selectpiece(hive, region, piece);
		break;
	}
}

int hive_handle(Hive *hive, int c)
{
	static const struct {
		char key;
		enum hive_type type;
	} keyTypeMap[] = {
		{ 'a', HIVE_ANT },
		{ 'b', HIVE_BEETLE },
		{ 'g', HIVE_GRASSHOPPER },
		{ 'l', HIVE_LADYBUG },
		{ 'm', HIVE_MOSQUITO },
		{ 'p', HIVE_PILLBUG },
		{ 'q', HIVE_QUEEN },
		{ 's', HIVE_SPIDER },
	};
	HiveRegion *region;

	if (hive->selectedRegion == NULL)
		hive->selectedRegion = &hive->board;
	region = hive->selectedRegion;
	switch (c) {
	case '0':
		if (hive->selectedPiece != NULL) {
			hive->hexCursor.x = 0;
			hive->hexCursor.y = 0;
		} else {
			region->translation.x = 0;
			region->translation.y = 0;
		}
		break;
	case KEY_LEFT:
		if (hive->selectedPiece != NULL)
			hive->hexCursor.x--;
		else
			region->translation.x--;
		break;
	case KEY_RIGHT:
		if (hive->selectedPiece != NULL)
			hive->hexCursor.x++;
		else
			region->translation.x++;
		break;
	case KEY_UP:
		if (hive->selectedPiece != NULL)
			hive->hexCursor.y--;
		else
			region->translation.y--;
		break;
	case KEY_DOWN:
		if (hive->selectedPiece != NULL)
			hive->hexCursor.y++;
		else
			region->translation.y++;
		break;

	case '\n':
	case '\r':
		if (hive->selectedPiece != NULL)
			hive_pressposition(hive, &hive->board, hive->hexCursor);
		break;

	case 'a' ... 'z':
	case 'A' ... 'Z':
		for (size_t i = 0; i < ARRLEN(keyTypeMap); i++)
			if (keyTypeMap[i].key == tolower(c)) {
				hive_selectnexttype(hive,
						islower(c) ? &hive->board :
							hive_getinventory(hive),
						keyTypeMap[i].type);
				break;
			}
		break;

	case 0x1b:
		hive_selectpiece(hive, NULL, NULL);
		break;
	}
	return 0;
}

void hive_render(Hive *hive)
{
	Point p;
	size_t cnt;

	for (size_t i = 0; i < ARRLEN(hive->regions); i++)
		hive_region_render(&hive->regions[i]);
	if (hive->selectedPiece == NULL)
		return;
	hive_region_renderhexat(&hive->board, COLOR_MAGENTA, hive->hexCursor);
	wattr_set(hive->board.win, 0, COLOR(COLOR_BLACK, COLOR_YELLOW), NULL);
	for (size_t i = 0; i < hive->moves.count; i++) {
		p = hive->moves.points[i];
		hive_pointtoworld(&p, hive->board.translation);
		mvwaddstr(hive->board.win, p.y, p.x + 1, "   ");
		mvwaddstr(hive->board.win, p.y + 1, p.x + 1, "   ");
	}
	wattr_set(hive->board.win, 0, COLOR(COLOR_BLACK, COLOR_GREEN), NULL);
	for (size_t i = 0; i < hive->choices.count; i++) {
		p = hive->choices.points[i];
		hive_pointtoworld(&p, hive->board.translation);
		mvwaddstr(hive->board.win, p.y, p.x + 1, "   ");
		mvwaddstr(hive->board.win, p.y + 1, p.x + 1, "   ");
	}
	if (hive->selectedRegion != &hive->board) {
		wnoutrefresh(hive->board.win);
		return;
	}
	/* render the piece stack */
	getmaxyx(hive->board.win, p.y, p.x);
	p.y = 0;
	p.x -= 7;
	cnt = hive_region_countat(&hive->board, hive->selectedPiece->position);
	for (HivePiece *piece = hive->selectedPiece; piece != NULL;
			piece = hive_region_getbelow(&hive->board, piece)) {
		hive_region_renderpieceat(&hive->board, piece, cnt, p);
		cnt--;
		p.y += 3;
	}
	wnoutrefresh(hive->board.win);
}
