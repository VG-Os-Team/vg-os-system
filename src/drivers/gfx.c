
#include "../../include/gfx.h"
#include "../../include/video.h"
#include "../../Render/font.h"
#include <stddef.h>

/* Inicializa o modulo grafico */
void gfx_init(void) {
    /* O driver de video deve estar inicializado */
    if (video_get_info() == NULL)
        return;
}

/* Desenha um pixel na tela */
void gfx_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    video_put_pixel(x, y, color);
}

/* Desenha um retangulo preenchido */
void gfx_draw_rect(uint32_t x, uint32_t y,
                   uint32_t width, uint32_t height,
                   uint32_t color) {
    video_fill_rect(x, y, width, height, color);
}

/* Desenha uma linha utilizando o algoritmo de Bresenham */
void gfx_draw_line(int x0, int y0, int x1, int y1,
                   uint32_t color) {

    int dx = x1 - x0;
    int dy = y1 - y0;

    int sx = (dx < 0) ? -1 : 1;
    int sy = (dy < 0) ? -1 : 1;

    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    int err = dx - dy;

    while (1) {
        if (x0 >= 0 && y0 >= 0) {
            gfx_put_pixel((uint32_t)x0, (uint32_t)y0, color);
        }

        if (x0 == x1 && y0 == y1)
            break;

        int e2 = 2 * err;

        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }

        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
}

/* Desenha um caractere usando a fonte bitmap 16x24 */
void gfx_draw_char(char c, uint32_t x, uint32_t y,
                   uint32_t color) {

    unsigned char index = (unsigned char)c;

    for (uint32_t row = 0; row < 24; row++) {

        uint16_t bits = font_ibm_plex[index][row];

        for (uint32_t col = 0; col < 16; col++) {

            if (bits & (1U << col)) {
                gfx_put_pixel(x + col, y + row, color);
            }
        }
    }
}

/* Desenha uma string utilizando a fonte bitmap */
void gfx_draw_string(const char *str, uint32_t x, uint32_t y,
                     uint32_t color) {

    if (str == NULL)
        return;

    uint32_t start_x = x;

    while (*str != '\0') {

        if (*str == '\n') {
            x = start_x;
            y += 24;
            str++;
            continue;
        }

        if (*str == '\r') {
            x = start_x;
            str++;
            continue;
        }

        gfx_draw_char(*str, x, y, color);

        x += 16;
        str++;
    }
}
