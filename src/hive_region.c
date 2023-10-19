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
	/* should in theory never happen */
	if (region->numPieces == ARRLEN(region->pieces))
		return -1;
	region->pieces[region->numPieces++] = piece;
	return 0;
}

int hive_region_removepiece(HiveRegion *region, HivePiece *piece)
{
	for (size_t i = 0; i < region->numPieces; i++) {
		if (region->pieces[i] != piece)
			continue;
		region->numPieces--;
		memmove(&region->pieces[i], &region->pieces[i + 1],
			sizeof(*region->pieces) * (region->numPieces - i));
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

HivePiece *hive_region_pieceatr(HiveRegion *region, HivePiece *from, Point at)
{
	bool doReturn;

	doReturn = from == NULL;
	for (size_t i = region->numPieces; i-- != 0; ) {
		HivePiece *piece;

		piece = region->pieces[i];
		if (piece == from) {
			doReturn = true;
			continue;
		}
		if (doReturn && point_isequal(piece->position, at))
			return piece;
	}
	return NULL;
}

HivePiece *hive_region_pieceat(HiveRegion *region, HivePiece *from, Point at)
{
	bool doReturn;

	doReturn = from == NULL;
	for (size_t i = 0; i < region->numPieces; i++) {
		HivePiece *piece;

		piece = region->pieces[i];
		if (piece == from) {
			doReturn = true;
			continue;
		}
		if (doReturn && point_isequal(piece->position, at))
			return piece;
	}
	return NULL;
}

size_t hive_region_countat(HiveRegion *region, Point at)
{
	size_t cnt = 0;

	for (size_t i = 0; i < region->numPieces; i++)
		if (point_isequal(region->pieces[i]->position, at))
			cnt++;
	return cnt;
}

size_t hive_region_getsurrounding(HiveRegion *region, Point at,
		HivePiece *pieces[6])
{
	size_t num = 0;
	for (int d = 0; d < 6; d++) {
		Point p;

		p = at;
		hive_movepoint(&p, d);
		if ((pieces[d] = hive_region_pieceat(region, NULL, p)) != NULL)
			num++;
	}
	return num;
}

size_t hive_region_getsurroundingr(HiveRegion *region, Point at,
		HivePiece *pieces[6])
{
	size_t num = 0;
	for (int d = 0; d < 6; d++) {
		Point p;

		p = at;
		hive_movepoint(&p, d);
		if ((pieces[d] = hive_region_pieceatr(region, NULL, p)) != NULL)
			num++;
	}
	return num;
}

uint32_t hive_region_count(HiveRegion *region, HivePiece *origin)
{
	/* 1 for itself */
	uint32_t cnt = 0;
	HivePiece *pieces[6];

	cnt = hive_region_countat(region, origin->position);
	origin->flags |= HIVE_VISITED;
	hive_region_getsurrounding(region, origin->position, pieces);
	for (int d = 0; d < 6; d++) {
		HivePiece *const piece = pieces[d];
		if (piece == NULL || (piece->flags & HIVE_VISITED))
			continue;
		cnt += hive_region_count(region, piece);
	}
	return cnt;
}

static const char *piece_names[] = {
	[HIVE_ANT] = "A",
	[HIVE_BEETLE] = "B",
	[HIVE_GRASSHOPPER] = "G",
	[HIVE_LADYBUG] = "L",
	[HIVE_MOSQUITO] = "M",
	[HIVE_PILLBUG] = "P",
	[HIVE_QUEEN] = "Q",
	[HIVE_SPIDER] = "S",
};

static const char *side_triangles[] = {
	"\u25e2",
	"\u25e3",
	"\u25e4",
	"\u25e5"
};

static const Point cell_offsets[] = {
	{ 0, 0 },
	{ 4, 0 },
	{ 4, 1 },
	{ 0, 1 }
};

void hive_region_renderpieceat(HiveRegion *region, HivePiece *piece,
		size_t cnt, Point at)
{
	short fg;

	fg = piece->side == HIVE_WHITE ?  COLOR_BLUE : COLOR_RED;
	wattr_set(region->win, 0, COLOR(COLOR_BLACK, fg), NULL);
	mvwprintw(region->win, at.y, at.x + 1, " %s ",
			piece_names[(int) piece->type]);
	mvwaddstr(region->win, at.y + 1, at.x + 1, "   ");
	if (cnt > 1)
		mvwprintw(region->win, at.y + 1, at.x + 1, " %zu ", cnt);
	else
		mvwaddstr(region->win, at.y + 1, at.x + 1, "   ");

	for (int n = 0; n < 4; n++) {
		wcolor_set(region->win, COLOR(fg, COLOR_YELLOW), NULL);
		const Point off = cell_offsets[n];
		mvwaddstr(region->win, at.y + off.y, at.x + off.x,
				side_triangles[n]);
	}
}

void hive_region_renderpiece(HiveRegion *region, HivePiece *piece)
{
	Point world;
	HivePiece *pieces[6];
	short fg;

	world = piece->position;
	hive_pointtoworld(&world, region->translation);

	fg = (piece->flags & HIVE_SELECTED) ? COLOR_YELLOW :
		(piece->flags & HIVE_ISACTOR) ? COLOR_GREEN :
		piece->side == HIVE_WHITE ?  COLOR_BLUE : COLOR_RED;
	wattr_set(region->win, 0, COLOR(COLOR_BLACK, fg), NULL);
	mvwprintw(region->win, world.y, world.x + 1, " %s ",
			piece_names[(int) piece->type]);
	const size_t cnt = hive_region_countat(region, piece->position);
	if (cnt > 1)
		mvwprintw(region->win, world.y + 1, world.x + 1, " %zu ", cnt);
	else
		mvwaddstr(region->win, world.y + 1, world.x + 1, "   ");

	hive_region_getsurroundingr(region, piece->position, pieces);
	for (int n = 0; n < 4; n++) {
		short bg;
		HivePiece *neighbor;

		neighbor = n == 0 ? pieces[HIVE_NORTH_EAST] :
			n == 1 ? pieces[HIVE_NORTH_WEST] :
			n == 2 ? pieces[HIVE_SOUTH_WEST] :
			pieces[HIVE_SOUTH_EAST];
		bg = neighbor == NULL ? COLOR_BLACK :
			(neighbor->flags & HIVE_SELECTED) ? COLOR_YELLOW :
			(neighbor->flags & HIVE_ISACTOR) ? COLOR_GREEN :
			neighbor->side == HIVE_WHITE ?  COLOR_BLUE : COLOR_RED;
		wcolor_set(region->win, COLOR(fg, bg), NULL);
		const Point off = cell_offsets[n];
		mvwaddstr(region->win, world.y + off.y, world.x + off.x,
				side_triangles[n]);
	}
}

void hive_region_render(HiveRegion *region)
{
	werase(region->win);
	for (size_t i = 0; i < region->numPieces; i++) {
		HivePiece *const piece = region->pieces[i];
		if (hive_region_getabove(region, piece) == NULL)
			hive_region_renderpiece(region, piece);
	}
	wnoutrefresh(region->win);
}
