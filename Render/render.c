#include "render.h"
#include "font.h"
#include "logo.h"
#include <stdint.h>

void desenhar_logo(uint32_t *fb, uint32_t pitch, int start_x, int start_y) {
  int index = 0;
  for (uint32_t y = 0; y < logo_height; y++) {
    for (uint32_t x = 0; x < logo_width; x++) {
      uint32_t cor = logo_pixels[index++];
      if (cor != 0x00000000)
        fb[((start_y + y) * pitch / 4) + (start_x + x)] = cor;
    }
  }
}

// Desenha uma única letra (escala 2x)
void desenhar_char(char c, int x, int y, uint32_t cor, uint32_t *fb,
                   uint32_t pitch) {
  const uint8_t *bitmap = font8x8_basic[(uint8_t)c];

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      if (bitmap[row] & (1 << col)) {
        for (int dy = 0; dy < 2; dy++)
          for (int dx = 0; dx < 2; dx++)
            fb[((y + row * 2 + dy) * pitch / 4) + (x + col * 2 + dx)] = cor;
      } else {
        for (int dy = 0; dy < 2; dy++)
          for (int dx = 0; dx < 2; dx++)
            fb[((y + row * 2 + dy) * pitch / 4) + (x + col * 2 + dx)] =
                0x00000000;
      }
    }
  }
}

// Desenha uma frase inteira reaproveitando o desenhar_char
void desenhar_string(const char *str, int start_x, int start_y, uint32_t cor,
                     uint32_t *fb, uint32_t pitch) {
  int x = start_x;
  for (int i = 0; str[i] != '\0'; i++) {
    desenhar_char(str[i], x, start_y, cor, fb, pitch);
    x += 16;
  }
}