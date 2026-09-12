#include "comandos.h"
#include "keyboard.h"
#include "logo.h"
#include "render.h"
#include "strutil.h"
#include "version.h"
#include <stdint.h>

typedef struct {
  uint32_t flags;
  uint32_t mem_lower;
  uint32_t mem_upper;
  uint32_t boot_device;
  uint32_t cmdline;
  uint32_t mods_count;
  uint32_t mods_addr;
  uint32_t syms[4];
  uint32_t mmap_length;
  uint32_t mmap_addr;
  uint32_t drives_length;
  uint32_t drives_addr;
  uint32_t config_table;
  uint32_t boot_loader_name;
  uint32_t apm_table;
  uint32_t vbe_control_info;
  uint32_t vbe_mode_info;
  uint16_t vbe_mode;
  uint16_t vbe_interface_seg;
  uint16_t vbe_interface_off;
  uint16_t vbe_interface_len;
  uint64_t framebuffer_addr;
  uint32_t framebuffer_pitch;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint8_t framebuffer_bpp;
  uint8_t framebuffer_type;
  uint8_t color_info[6];
} __attribute__((packed)) multiboot_info_t;

#define LINHA_MAX 1024

static void limpar_terminal(uint32_t *fb, uint32_t pitch, uint32_t width,
                            uint32_t height, int terminal_y_inicial,
                            int *cursor_x, int *cursor_y, int *cursor_base_x,
                            int *linha_len) {
  for (uint32_t y = terminal_y_inicial; y < height; y++)
    for (uint32_t x = 0; x < width; x++)
      fb[(y * pitch / 4) + x] = 0x00000000;

  *cursor_y = terminal_y_inicial;
  desenhar_string("root@vgos:~# ", 50, *cursor_y, 0x0000FF00, fb, pitch);
  *cursor_base_x = 50 + 208;
  *cursor_x = *cursor_base_x;
  *linha_len = 0;
}

// Insere um caractere no terminal: desenha, avança cursor, cuida de quebra de
// linha, e registra no buffer da linha atual (usado por digitação normal e por
// Ctrl+V)
static void inserir_char(char c, uint32_t *fb, uint32_t pitch, uint32_t width,
                         int *cursor_x, int *cursor_y, char *linha,
                         int *linha_len) {
  if (*linha_len >= LINHA_MAX - 1)
    return;

  if (*cursor_x + 16 >= (int)width - 50) {
    *cursor_y += 24;
    *cursor_x = 50;
  }
  desenhar_char(c, *cursor_x, *cursor_y, 0x00FFFFFF, fb, pitch);
  *cursor_x += 16;
  linha[(*linha_len)++] = c;
}

void kernel_main(multiboot_info_t *mbd) {
  if (!(mbd->flags & (1 << 12)))
    return;

  uint32_t *fb = (uint32_t *)(uintptr_t)mbd->framebuffer_addr;
  uint32_t pitch = mbd->framebuffer_pitch;
  uint32_t width = mbd->framebuffer_width;
  uint32_t height = mbd->framebuffer_height;

  // Fundo preto
  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++)
      fb[(y * pitch / 4) + x] = 0x00000000;
  }

  int logo_x = (width - logo_width) / 2;
  int logo_y = 5;
  desenhar_logo(fb, pitch, logo_x, logo_y);

  int text_y = logo_y + logo_height + 40;

  const char *msg = OS_BANNER;
  int msg_largura = minha_strlen(msg) * 16;
  int msg_x = (width - msg_largura) / 2;

  desenhar_string(msg, msg_x, text_y, 0x00FFFFFF, fb, pitch);

  text_y += 30; // Pula linha
  desenhar_string("root@vgos:~# ", 50, text_y, 0x0000FF00, fb, pitch);

  // Configura o cursor para iniciar exatamente depois da palavra "root@vgos:~#
  // " "root@vgos:~# " tem 13 caracteres. 13 * 16 pixels de largura = 208
  int terminal_y_inicial = text_y;

  char linha_atual[LINHA_MAX];
  int linha_len = 0;

  char clipboard[LINHA_MAX];
  int clipboard_len = 0;

  int cursor_base_x = 50 + 208;
  int cursor_x = cursor_base_x;
  int cursor_y = text_y;

  // Loop do Teclado
  // O Loop do Terminal
  while (1) {
    int tecla = teclado_ler();
    if (tecla == TECLA_NENHUMA) {
      asm volatile("pause");
      continue;
    }

    switch (tecla) {
    case TECLA_CTRL_L:
      limpar_terminal(fb, pitch, width, height, terminal_y_inicial, &cursor_x,
                      &cursor_y, &cursor_base_x, &linha_len);
      break;

    case TECLA_CTRL_C:
      for (int i = 0; i < linha_len; i++)
        clipboard[i] = linha_atual[i];
      clipboard_len = linha_len;
      break;

    case TECLA_CTRL_V:
      for (int i = 0; i < clipboard_len; i++)
        inserir_char(clipboard[i], fb, pitch, width, &cursor_x, &cursor_y,
                     linha_atual, &linha_len);
      break;

    default: {
      if (tecla < 0) {
        // Teclas estendidas que não geram ASCII (como setas, Delete, etc.)
        // ignoradas por enquanto
        break;
      }
      char c = (char)tecla;
      if (c == '\b') {
        if (linha_len > 0) {
          int min_x = (cursor_y == terminal_y_inicial) ? cursor_base_x : 50;
          if (cursor_x > min_x) {
            cursor_x -= 16;
            desenhar_char(' ', cursor_x, cursor_y, 0x00FFFFFF, fb, pitch);
            linha_len--;
          } else if (cursor_y > terminal_y_inicial) {
            cursor_y -= 24;
            int chars_por_linha = (width - 100) / 16;
            cursor_x = 50 + (chars_por_linha - 1) * 16;
            desenhar_char(' ', cursor_x, cursor_y, 0x00FFFFFF, fb, pitch);
            linha_len--;
          }
        }
      } else if (c == '\n') {
        comando_resultado_t r = comandos_executar(linha_atual, linha_len, fb,
                                                  pitch, width, &cursor_y);

        if (r == COMANDO_LIMPAR) {
          limpar_terminal(fb, pitch, width, height, terminal_y_inicial,
                          &cursor_x, &cursor_y, &cursor_base_x, &linha_len);
        } else {
          cursor_y += 24;
          desenhar_string("root@vgos:~# ", 50, cursor_y, 0x0000FF00, fb, pitch);
          cursor_base_x = 50 + 208;
          cursor_x = cursor_base_x;
          linha_len = 0;
        }
      } else {
        inserir_char(c, fb, pitch, width, &cursor_x, &cursor_y, linha_atual,
                     &linha_len);
      }
      break;
    }
    }
  }
}