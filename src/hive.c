#include "hive.h"
#include "stb_ds.h"

int hive_init(struct hive *hive)
{
    static const enum hive_type defaultInventory[HIVE_INVENTORY_SIZE] = {
        HIVE_QUEEN,
        HIVE_BEETLE,
        HIVE_BEETLE,
        HIVE_GRASSHOPPER,
        HIVE_GRASSHOPPER,
        HIVE_GRASSHOPPER,
        HIVE_SPIDER,
        HIVE_SPIDER,
        HIVE_ANT,
        HIVE_ANT,
        HIVE_ANT,
        HIVE_LADYBUG,
        HIVE_MOSQUITO,
        HIVE_PILLBUG,
        HIVE_QUEEN,
        HIVE_BEETLE,
        HIVE_BEETLE,
        HIVE_GRASSHOPPER,
        HIVE_GRASSHOPPER,
        HIVE_GRASSHOPPER,
        HIVE_SPIDER,
        HIVE_SPIDER,
        HIVE_ANT,
        HIVE_ANT,
        HIVE_ANT,
        HIVE_LADYBUG,
        HIVE_MOSQUITO,
        HIVE_PILLBUG
    };

    memset(hive, 0, sizeof(*hive));
    if ((hive->win = newpad(GRID_ROWS * 2, GRID_COLUMNS * 4)) == NULL)
        return -1;
    hive->turn = HIVE_BLACK;
    memcpy(&hive->blackInventory,
        defaultInventory, sizeof(defaultInventory));
    memcpy(&hive->whiteInventory,
        defaultInventory, sizeof(defaultInventory));

    hive->stackSz = 1;

    /* test setup */
    struct vec3 pos = { 6, 6, 0 };
    for (int i = 0; i < 20; i++) {
        const enum hive_type t = 1 + rand() % (HIVE_TYPES - 1);
        const enum hive_side s = (1 + rand() % 2) << 4;
        hive->grid[pos.x + pos.y * GRID_COLUMNS] = t | s;
        pos = vec_move(&pos, rand() % 6);
    }
    return 0;
}
/*  struct stack {
 *      struct vec3 pos;
 *      piece_t above;
 *  };
 */
/* (stack.above) The piece above pos.
*/

stack_t hive_getabovestack(struct hive *hive, struct vec3 *pos)
{
    for (int i = 0; i < hive->stackSz; i++)
        if (memcmp(&hive->stacks[i].pos, pos, sizeof *pos) == 0)
            return i;
    return 0;
}

piece_t hive_getabove(struct hive *hive, struct vec3 *pos)
{
    for (int i = 0; i < hive->stackSz; i++)
        if (memcmp(&hive->stacks[i].pos, pos, sizeof *pos) == 0)
            return hive->stacks[i].above;
    return 0;
}

piece_t hive_getexposedpiece(struct hive *hive, struct vec3 *pos)
{
    piece_t piece =
        hive->grid[pos->x + pos->y * GRID_COLUMNS];

    while (piece & HIVE_ABOVE) {
        piece = hive_getabove(hive, pos);
        pos->z++;
    }
    return piece;
}

void hive_stack(struct hive *hive, struct vec3 *pos, piece_t piece)
{
    assert(hive->stackSz < HIVE_STACK_SIZE);
    hive->stacks[hive->stackSz].pos = *pos;
    hive->stacks[hive->stackSz].above = piece;
    hive->stackSz++;
}

void hive_unstack(struct hive *hive, stack_t stack)
{
    assert(hive->stackSz > 1);
    hive->stackSz--;
    memmove(&hive->stacks[stack],
        &hive->stacks[hive->stackSz], sizeof *hive->stacks);
}

void hive_putpiece(struct hive *hive, struct vec3 *pos, piece_t piece)
{
    stack_t stack = 0;
    piece_t below =
        hive->grid[pos->x + pos->y * GRID_COLUMNS];

    if (below & HIVE_ABOVE) {
        while (below & HIVE_ABOVE) {
            stack = hive_getabovestack(hive, pos);
            below = hive->stacks[stack].above;
            pos->z++;
        }
        hive_stack(hive, pos, piece);
        hive->stacks[stack].above |= HIVE_ABOVE;
    }
    else if (below) {
        hive_stack(hive, pos, piece);
        hive->grid[pos->x + pos->y * GRID_COLUMNS] |= HIVE_ABOVE;
    }
    else {
        hive->grid[pos->x + pos->y * GRID_COLUMNS] = piece;
    }
}

void hive_delpiece(struct hive *hive, struct vec3 *pos)
{
    stack_t stack = 0;
    stack_t previousStack = -1;
    piece_t below =
        hive->grid[pos->x + pos->y * GRID_COLUMNS];

    while (below & HIVE_ABOVE) {
        previousStack = stack;
        stack = hive_getabovestack(hive, pos);
        below = hive->stacks[stack].above;
        pos->z++;
    }

    if (previousStack == -1) {
        hive->grid[pos->x + pos->y * GRID_COLUMNS] = 0;
    }
    else if (previousStack == 0) {
        hive_unstack(hive, stack);
        hive->grid[pos->x + pos->y * GRID_COLUMNS] &= ~HIVE_ABOVE;
    }
    else {
        hive_unstack(hive, stack);
        hive->stacks[previousStack].above &= ~HIVE_ABOVE;
    }
}

static void hive_handlemousepress(struct hive *hive, const struct vec3 *mp)
{
    struct vec3 pos;

    pos = *mp;
    pos.x = pos.x / 4;
    pos.y = (pos.y - (pos.x % 2)) / 2;
    pos.z = 0;

    if (hive->pendingMove.startPos.x != pos.x ||
            hive->pendingMove.startPos.y != pos.y) {
        if (hive->selectedPiece) {
            hive->pendingMove.endPos = pos;
        }
        else {
            hive->pendingMove.startPos = pos;
            hive->selectedPiece = hive_getexposedpiece(hive, &pos);
        }
    }
}

int hive_handle(struct hive *hive, int c)
{
    MEVENT event;

    switch(c) {
    case KEY_MOUSE:
        getmouse(&event);
        if (event.bstate & BUTTON1_CLICKED ||
                event.bstate & BUTTON1_PRESSED) {
            struct vec3 pos;

            pos.x = event.x;
            pos.y = event.y;
            wmouse_trafo(hive->win, &pos.y, &pos.x, FALSE);

            pos.x = pos.x + hive->winPos.x;
            pos.y = pos.y + hive->winPos.y;
            hive_handlemousepress(hive, &pos);
        
            pos.x = pos.x / 4;
            pos.y = (pos.y - (pos.x % 2)) / 2;
            pos.z = 0;

            const enum hive_type t = 1 + rand() % (HIVE_TYPES - 1);
            const enum hive_side s = (1 + rand() % 2) << 4;
            hive_putpiece(hive, &pos, t | s);
        }
        else if (event.bstate & BUTTON3_CLICKED ||
                event.bstate & BUTTON3_PRESSED) {
            struct vec3 pos;
            pos.x = event.x;
            pos.y = event.y;
            wmouse_trafo(hive->win, &pos.y, &pos.x, FALSE);

            pos.x = pos.x + hive->winPos.x;
            pos.y = pos.y + hive->winPos.y;
            hive_handlemousepress(hive, &pos);
        
            pos.x = pos.x / 4;
            pos.y = (pos.y - (pos.x % 2)) / 2;
            pos.z = 0;

            const enum hive_type t = 1 + rand() % (HIVE_TYPES - 1);
            const enum hive_side s = (1 + rand() % 2) << 4;
            hive_delpiece(hive, &pos);
        }
        break;
    case 'd':
        hive->winPos.x = hive->winPos.x < getmaxx(hive->win) ?
            hive->winPos.x + 1 : hive->winPos.x;
        break;
    case 'a':
        hive->winPos.x = hive->winPos.x > 0 ?
            hive->winPos.x - 1 : hive->winPos.x;
        break;
    case 'w':
        hive->winPos.y = hive->winPos.y > 0 ?
            hive->winPos.y - 1 : hive->winPos.y;
        break;
    case 's':
        hive->winPos.y = hive->winPos.y < getmaxy(hive->win) ?
            hive->winPos.y + 1 : hive->winPos.y;
        break;
    }
    return 0;
}

void hive_render(struct hive *hive)
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
    WINDOW *const win = hive->win;
    pos.z = 0;

    werase(win);
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
    prefresh(win, hive->winPos.y, hive->winPos.x, 0, 0, LINES - 1, COLS - 1);
}
