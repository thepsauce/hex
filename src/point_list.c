#include "hex.h"

bool point_list_push(PointList *list, Point p)
{
	Point *newPoints;

	newPoints = realloc(list->points, sizeof(*list->points) *
			(list->count + 1));
	if (newPoints == NULL)
		return false;
	list->points = newPoints;
	list->points[list->count++] = p;
	return true;
}

bool point_list_contains(const PointList *list, Point p)
{
	for (size_t i = 0; i < list->count; i++)
		if (point_isequal(list->points[i], p))
			return true;
	return false;
}

void point_list_clear(PointList *list)
{
	list->count = 0;
}

