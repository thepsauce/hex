#include "hive.h"

void hive_render(struct hive *hive, WINDOW *win)
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
}
