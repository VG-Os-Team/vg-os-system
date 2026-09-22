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

// Desenha uma única letra em alta resolução
void desenhar_char(char c, int x, int y, uint32_t cor, uint32_t *fb,
                   uint32_t pitch) {
  const uint16_t *bitmap = font_ibm_plex[(uint8_t)c];

  for (int row = 0; row < 24; row++) {
    uint16_t bits = bitmap[row];
    for (int col = 0; col < 16; col++) {
      if (bits & (1 << col)) {
        fb[((y + row) * pitch / 4) + (x + col)] = cor;
      } else {
        fb[((y + row) * pitch / 4) + (x + col)] = 0x00000000;
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