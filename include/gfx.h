
#ifndef GFX_H
#define GFX_H

#include <stdint.h>

/* Inicializacao do modulo grafico */
void gfx_init(void);

/* Desenha um pixel */
void gfx_put_pixel(uint32_t x, uint32_t y, uint32_t color);

/* Desenha um retangulo preenchido */
void gfx_draw_rect(uint32_t x, uint32_t y,
                   uint32_t width, uint32_t height,
                   uint32_t color);

/* Desenha uma linha */
void gfx_draw_line(int x0, int y0, int x1, int y1,
                   uint32_t color);

/* Desenha um caractere */
void gfx_draw_char(char c, uint32_t x, uint32_t y,
                   uint32_t color);

/* Desenha uma string */
void gfx_draw_string(const char *str, uint32_t x, uint32_t y,
                     uint32_t color);

#endif /* GFX_H */
