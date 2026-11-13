#ifndef LAYOUT_H
#define LAYOUT_H

#include <stdint.h>
#include "render.h"

#define LAYOUT_CELL_W RENDER_CHAR_WIDTH
#define LAYOUT_CELL_H RENDER_CHAR_HEIGHT
#define LAYOUT_BG     0x00020E0EU
#define LAYOUT_TEXT   0x00D8F1E7U
#define LAYOUT_MUTED  0x0090B8AAU
#define LAYOUT_ACCENT 0x0001DE82U

/* Pixels; largura e altura sao multiplos da celula bitmap. */
typedef struct {
    int x, y, width, height;
} layout_bounds_t;

void layout_draw(void);
layout_bounds_t layout_get_terminal_bounds(void);

/* Avanca uma linha, rolando somente os pixels da grade quando necessario. */
void layout_next_line(int *y);
void layout_write_line(const char *text, uint32_t color, int *y);

#endif
