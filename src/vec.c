#include "hive.h"

static const struct vec3 dir_offsets[2][6] = {
	{
		{  1,  0 }, {  1, -1 }, { 0, -1 },
		{ -1, -1 }, { -1,  0 }, { 0,  1 }
	},
	{
		{  1,  1 }, {  1,  0 }, { 0, -1 },
		{ -1,  0 }, { -1,  1 }, { 0,  1 }
	},
};

void vec_move(const struct vec3 *vec3, enum hive_direction dir,
		struct vec3 *dest)
{
	const struct vec3 offset = dir_offsets[vec3->x % 2][dir];
	dest->x = vec3->x + offset.x;
	dest->y = vec3->y + offset.y;
}

enum hive_direction vec_getdir(const struct vec3 *a, const struct vec3 *b)
{
	const int dx = a->x - b->x;
	const int dy = a->y - b->y;
	const struct vec3 *offsets;

	offsets = dir_offsets[a->y % 2];
	for (enum hive_direction d = 0; d < 6; d++)
		if (offsets[0].x == dx && offsets[1].y == dy)
			return d;
	return -1;
}
