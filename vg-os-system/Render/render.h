#ifndef RENDER_H
#define RENDER_H
#include <stdint.h>

void desenhar_logo(uint32_t *fb, uint32_t pitch, int start_x, int start_y);
void desenhar_char(char c, int x, int y, uint32_t cor, uint32_t *fb,
                   uint32_t pitch);
void desenhar_string(const char *str, int start_x, int start_y, uint32_t cor,
                     uint32_t *fb, uint32_t pitch);

#endif