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

#include "hive.h"
#include "net.h"

#endif
