#include "hive.h"

static const struct vec3 dir_offsets[2][6] = {
	{
		{  1,  0, 0 }, {  1, -1, 0 }, { 0, -1, 0 },
		{ -1, -1, 0 }, { -1,  0, 0 }, { 0,  1, 0 }
	},
	{
		{  1,  1, 0 }, {  1,  0, 0 }, { 0, -1, 0 },
		{ -1,  0, 0 }, { -1,  1, 0 }, { 0,  1, 0 }
	},
};

struct vec3 vec_move(const struct vec3 *vec3, enum hive_direction dir)
{
	const struct vec3 offset = dir_offsets[vec3->x % 2][dir];
	return (struct vec3) {
		vec3->x + offset.x,
		vec3->y + offset.y,
		0
	};
}

enum hive_direction vec_getdir(const struct vec3 *a, const struct vec3 *b)
{
	const int dx = a->x - b->x;
	const int dy = a->y - b->y;
	const struct vec3 *const offsets = dir_offsets[a->x % 2];
	for (enum hive_direction d = 0; d < 6; d++)
		if (offsets[d].x == dx && offsets[d].y == dy)
			return d;
	return -1;
}
