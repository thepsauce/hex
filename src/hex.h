#ifndef INCLUDED_HIVE_H
#define INCLUDED_HIVE_H

#include <assert.h>
#include <ctype.h>
#include <curses.h>
#include <limits.h>
#include <locale.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MIN(a, b) ({ \
	__auto_type _a = (a); \
	__auto_type _b = (b); \
	_a < _b ? _a : _b; \
})

#define MAX(a, b) ({ \
	__auto_type _a = (a); \
	__auto_type _b = (b); \
	_a > _b ? _a : _b; \
})

#define ARRLEN(a) (sizeof(a)/sizeof*(a))
#define ASSERT(p, msg) do { \
	if (!(p)) { \
		endwin(); \
		fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, (msg)); \
		exit(-1); \
	} \
} while (0)

void curses_init(void);

typedef struct point {
	int x, y;
} Point;

#define point_add(p, a) ({ \
	Point *const _p = (p); \
	const Point _a = (a); \
	_p->x += _a.x; \
	_p->y += _a.y; \
})

#define point_subtract(p, a) ({ \
	Point *const _p = (p); \
	const Point _a = (a); \
	_p->x -= _a.x; \
	_p->y -= _a.y; \
})

#define point_isequal(a, b) ({ \
	const Point _a = (a); \
	const Point _b = (b); \
	_a.x == _b.x && _a.y == _b.y; \
})

typedef struct point_list {
	Point *points;
	size_t count;
} PointList;

bool point_list_push(PointList *list, Point p);
bool point_list_contains(const PointList *list, Point p);
void point_list_clear(PointList *list);

enum {
	/* 0 is the reserved default attribute */
	/* 1 to 256 are the normal color pairs */
	PAIR_NORMAL = 257,
	PAIR_ERROR,
	PAIR_INFO,
	PAIR_COMMAND,
	PAIR_ARGUMENT,
};

#define COLOR(fg, bg) (1 + (fg) + (bg) * 8)

#include "hive.h"
#include "net.h"
#include "hc.h"

#endif
