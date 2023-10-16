#ifndef INCLUDED_HIVE_H
#define INCLUDED_HIVE_H

#include <assert.h>
#include <ctype.h>
#include <curses.h>
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

enum {
	PAIR_NORMAL = 1,
	PAIR_ERROR,
	PAIR_INFO,
	PAIR_COMMAND,
	PAIR_ARGUMENT,

	HIVE_PAIR_BLACK,
	HIVE_PAIR_BLACK_BLACK,
	HIVE_PAIR_BLACK_WHITE,
	HIVE_PAIR_WHITE,
	HIVE_PAIR_WHITE_BLACK,
	HIVE_PAIR_WHITE_WHITE,
	HIVE_PAIR_SELECTED,
};

#define ATTR_NORMAL COLOR_PAIR(PAIR_NORMAL)
#define ATTR_ERROR COLOR_PAIR(PAIR_ERROR)
#define ATTR_INFO COLOR_PAIR(PAIR_INFO)
#define ATTR_COMMAND COLOR_PAIR(PAIR_COMMAND)
#define ATTR_ARGUMENT COLOR_PAIR(PAIR_ARGUMENT)

#include "hive.h"
#include "net.h"

#endif
