#include "hex.h"

int hive_region_init(HiveRegion *region, int x, int y, int w, int h)
{
	WINDOW *win;

	if ((win = newwin(h, w, y, x)) == NULL)
		return -1;
	memset(region, 0, sizeof(*region));
	region->win = win;
	return 0;
}

int hive_region_addpiece(HiveRegion *region, HivePiece *piece)
{
	HivePiece *at;
	HivePiece *pieces[6];

	/* should in theory never happen */
	if (region->numPieces == ARRLEN(region->pieces))
		return -1;
	at = hive_region_pieceat(region, piece->position);
	region->pieces[region->numPieces++] = piece;
	/* link the piece */
	if (at != NULL) {
		for (; at->above != NULL; at = at->above);
		at->above = piece;
		piece->below = at;
		return 0;
	}
	hive_region_getsurrounding(region, piece->position, pieces);
	for (size_t i = 0; i < 6; i++) {
		HivePiece *const p = pieces[i];
		if (p == NULL)
			continue;
		switch (i) {
		case HIVE_NORTH:
			piece->north = p;
			p->south = piece;
			break;
		case HIVE_SOUTH:
			piece->south = p;
			p->north = piece;
			break;
		case HIVE_NORTH_WEST:
			piece->northWest = p;
			p->southEast = piece;
			break;
		case HIVE_NORTH_EAST:
			piece->northEast = p;
			p->southWest = piece;
			break;
		case HIVE_SOUTH_WEST:
			piece->southWest = p;
			p->northEast = piece;
			break;
		case HIVE_SOUTH_EAST:
			piece->southEast = p;
			p->northWest = piece;
			break;
		}
	}
	return 0;
}

int hive_region_removepiece(HiveRegion *region, HivePiece *piece)
{
	for (size_t i = 0; i < region->numPieces; i++) {
		if (region->pieces[i] != piece)
			continue;
		/* unlink the piece */
		if (piece->north != NULL)
			piece->north->south = NULL;
		if (piece->south != NULL)
			piece->south->north = NULL;
		if (piece->northWest != NULL)
			piece->northWest->southEast = NULL;
		if (piece->northEast != NULL)
			piece->northEast->southWest = NULL;
		if (piece->southWest != NULL)
			piece->southWest->northEast = NULL;
		if (piece->southEast != NULL)
			piece->southEast->northWest = NULL;
		if (piece->above != NULL)
			piece->above->below = piece->below;
		if (piece->below != NULL)
			piece->below->above = piece->above;
		memset(piece->neighbors, 0, sizeof(piece->neighbors));
		region->numPieces--;
		memmove(&region->pieces[i],
			&region->pieces[i + 1],
			sizeof(*region->pieces) *
				(region->numPieces - i));
		return 0;
	}
	/* should in theory never happen */
	return -1;
}

void hive_region_clearflags(HiveRegion *region, uint64_t flags)
{
	for (size_t i = 0; i < region->numPieces; i++)
		region->pieces[i]->flags &= ~flags;
}

HivePiece *hive_region_pieceat(HiveRegion *region, Point p)
{
	for (size_t i = 0; i < region->numPieces; i++) {
		HivePiece *piece;

		piece = region->pieces[i];
		if (point_isequal(piece->position, p)) {
			while (piece->below != NULL)
				piece = piece->below;
			return piece;
		}
	}
	return NULL;
}

size_t hive_region_getsurrounding(HiveRegion *region, Point at,
		HivePiece *pieces[6])
{
	size_t num = 0;
	for (int d = 0; d < 6; d++) {
		Point p;

		p = at;
		hive_movepoint(&p, d);
		if ((pieces[d] = hive_region_pieceat(region, p)) != NULL)
			num++;
	}
	return num;
}

uint32_t hive_region_count(HiveRegion *region, HivePiece *origin)
{
	/* 1 for itself */
	uint32_t cnt = 1;

	for (HivePiece *p = origin; (p = p->above) != NULL; )
		cnt++;
	origin->flags |= HIVE_VISITED;
	for (int d = 0; d < 6; d++) {
		HivePiece *const p = origin->neighbors[d];
		if (p != NULL && !(p->flags & HIVE_VISITED))
			cnt += hive_region_count(region, p);
	}
	return cnt;
}

void hive_piece_render(HivePiece *piece, WINDOW *win, Point translation)
{
	static const char *pieceNames[] = {
		[HIVE_ANT] = "A",
		[HIVE_BEETLE] = "B",
		[HIVE_GRASSHOPPER] = "G",
		[HIVE_LADYBUG] = "L",
		[HIVE_MOSQUITO] = "M",
		[HIVE_PILLBUG] = "P",
		[HIVE_QUEEN] = "Q",
		[HIVE_SPIDER] = "S",
	};
	static const char *triangles[] = {
		"\u25e2",
		"\u25e3",
		"\u25e4",
		"\u25e5"
	};
	static const Point cellOffsets[] = {
		{ 0, 0 },
		{ 4, 0 },
		{ 4, 1 },
		{ 0, 1 }
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
	Point world;
	HivePiece *bottom;

	world = piece->position;
	hive_pointtoworld(&world, translation);

	wattr_set(win, A_REVERSE, (piece->flags & HIVE_SELECTED) ?
			HIVE_PAIR_SELECTED : (piece->side == HIVE_WHITE ?
		HIVE_PAIR_WHITE : HIVE_PAIR_BLACK), NULL);
	mvwprintw(win, world.y, world.x + 1, " %s ",
			pieceNames[(int) piece->type]);
	mvwaddstr(win, world.y + 1, world.x + 1, "   ");

	for (bottom = piece; bottom->below != NULL; bottom = bottom->below);
	for (int n = 0; n < 4; n++) {
		HivePiece *neighbor = n == 0 ?
			bottom->northWest : n == 1 ?
			bottom->northEast : n == 2 ?
			bottom->southEast : bottom->southWest;

		/* (Vaxeral) Get the exposed, neighboring piece. */
		if (neighbor != NULL)
			while (neighbor->above != NULL)
				neighbor = neighbor->above;
		const int side = neighbor == NULL ? 0 :
			neighbor->side + 1;
		const short pair = pairs[(int) piece->side + 1][side];
		wattr_set(win, 0, pair, NULL);

		const Point off = cellOffsets[n];
		mvwaddstr(win, world.y + off.y, world.x + off.x, triangles[n]);
	}
}

void hive_region_render(HiveRegion *region)
{
	WINDOW *const win = region->win;

	werase(win);
	for (size_t i = 0; i < region->numPieces; i++) {
		HivePiece *const piece = region->pieces[i];
		if (piece->above == NULL)
			hive_piece_render(piece, win, region->translation);
	}
	wnoutrefresh(win);
}
