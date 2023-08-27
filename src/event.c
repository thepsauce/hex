#include "hive.h"

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
	while(piece & HIVE_ABOVE) {
		piece = hive_getabove(hive, pos);
		pos->z++;
	}
	return piece;
}

void hive_handlemousepress(struct hive *hive, const struct vec3 *mp)
{
	struct vec3 pos;

	pos = *mp;
	pos.x = pos.x / 4;
	pos.y = (pos.y - (pos.x % 2)) / 2;
	pos.z = 0;
	if (pos.x != hive->selectedPos.x || pos.y != hive->selectedPos.y) {
		if (hive->selectedPiece) {
			hive->grid[pos.x + pos.y * GRID_COLUMNS] =
				hive->selectedPiece;

			hive->selectedPiece = 0;
			hive->grid[hive->selectedPos.x +
				hive->selectedPos.y * GRID_COLUMNS] = 0;
		} else {
			hive->selectedPos = pos;
			hive->selectedPiece = hive_getexposedpiece(hive, &pos);
		}
	}
}
