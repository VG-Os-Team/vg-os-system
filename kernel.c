#include "comandos.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
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

static inline void outw(uint16_t port, uint16_t val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

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
                         uint32_t height, int *cursor_x, int *cursor_y,
                         char *linha, int *linha_len) {
  if (*linha_len >= LINHA_MAX - 1)
    return;

  if (*cursor_x + 16 >= (int)width - 50) {
    if (*cursor_y + 24 < (int)height - 24) {
      *cursor_y += 24;
      *cursor_x = 50;
    } else {
      return;
    }
  }
  desenhar_char(c, *cursor_x, *cursor_y, 0x00FFFFFF, fb, pitch);
  *cursor_x += 16;
  linha[(*linha_len)++] = c;
}

void kernel_main(multiboot_info_t *mbd) {
  // Troca a GDT provisória do GRUB pela nossa, antes de qualquer outra coisa
  gdt_instalar();

  // Instala a Tabela de Interrupções (IDT)
  idt_install();

  // Registra os 32 stubs de exceção (interrupts.s) na IDT
  isr_install();


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

  // Letras VGOS em blocos grandes
  static const uint16_t letra_V[] = {
    0xC03, 0xC03, 0xC03, 0x606, 0x606, 0x30C,
    0x30C, 0x198, 0x198, 0x0F0, 0x060, 0x060
  };
  static const uint16_t letra_G[] = {
    0x3FC, 0x7FE, 0xE07, 0x003, 0x003, 0xFE3,
    0xC03, 0xC03, 0xE07, 0x7FE, 0x3FC, 0x000
  };
  static const uint16_t letra_O[] = {
    0x3FC, 0x7FE, 0xE07, 0xC03, 0xC03, 0xC03,
    0xC03, 0xC03, 0xE07, 0x7FE, 0x3FC, 0x000
  };
  static const uint16_t letra_S[] = {
    0x3FC, 0x7FE, 0xE07, 0x003, 0x01E, 0x0FC,
    0x3C0, 0x780, 0xE07, 0x7FE, 0x3FC, 0x000
  };

  int blk_w = 6, blk_h = 8;
  int lcols = 12, lrows = 12;
  int let_w = lcols * blk_w;   // 72px por letra
  int let_h = lrows * blk_h;   // 96px de altura
  int let_gap = 12;            // espaço entre letras

  // Centralizar logo + VGOS juntos na tela
  int art_total_w = let_w * 4 + let_gap * 3;  // 324px de texto
  int header_gap = 50;                         // espaço entre logo e texto
  int total_header_w = (int)logo_width + header_gap + art_total_w;
  int start_x = ((int)width - total_header_w) / 2;
  if (start_x < 20)
    start_x = 20;

  int logo_x = start_x;
  int logo_y = 20;
  desenhar_logo(fb, pitch, logo_x, logo_y);

  int art_x = logo_x + (int)logo_width + header_gap;
  int art_y = logo_y + ((int)logo_height - let_h) / 2; // centralizar verticalmente

  // Renderizar as 4 letras em verde escuro
  uint32_t cor_vgos = 0x00228B22; // Verde escuro (Forest Green)
  const uint16_t *letras[] = {letra_V, letra_G, letra_O, letra_S};
  for (int li = 0; li < 4; li++) {
    int lx = art_x + li * (let_w + let_gap);
    for (int r = 0; r < lrows; r++) {
      uint16_t bits = letras[li][r];
      for (int c = 0; c < lcols; c++) {
        if (bits & (1 << c)) {
          int bx = lx + c * blk_w;
          int by = art_y + r * blk_h;
          for (int py = 0; py < blk_h; py++)
            for (int px = 0; px < blk_w; px++)
              fb[((by + py) * pitch / 4) + (bx + px)] = cor_vgos;
        }
      }
    }
  }

  // Linha separadora sutil
  int margin = 50;
  int sep_y = logo_y + (int)logo_height + 15;
  for (uint32_t sx = (uint32_t)margin; sx < width - (uint32_t)margin; sx++)
    fb[(sep_y * pitch / 4) + sx] = 0x00333333;

  // Terminal abaixo da linha separadora
  int text_y = sep_y + 15;
  desenhar_string("root@vgos:~# ", 50, text_y, 0x0000FF00, fb, pitch);

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
        inserir_char(clipboard[i], fb, pitch, width, height, &cursor_x,
                     &cursor_y, linha_atual, &linha_len);
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
        } else if (r == COMANDO_EXIT) {
          if (cursor_y + 24 < (int)height - 24)
            cursor_y += 24;
          desenhar_string("Desligando o VG-OS...", 50, cursor_y, 0x00FFFF00, fb, pitch);

          // Tentar desligamento via portas de E/S (QEMU / Bochs / VirtualBox)
          outw(0x604, 0x2000);   // QEMU ACPI shutdown
          outw(0xB004, 0x2000);  // Bochs / QEMU legado
          outw(0x4004, 0x3400);  // VirtualBox

          // Fallback caso a máquina não desligue por porta (ou hardware real sem ACPI)
          if (cursor_y + 24 < (int)height - 24)
            cursor_y += 24;
          desenhar_string("Sistema finalizado. Voce pode desligar o computador com seguranca.",
                          50, cursor_y, 0x00AAAAAA, fb, pitch);
          asm volatile("cli");
          while (1) {
            asm volatile("hlt");
          }
        } else {
          if (cursor_y + 24 < (int)height - 24) {
            cursor_y += 24;
          }
          desenhar_string("root@vgos:~# ", 50, cursor_y, 0x0000FF00, fb, pitch);
          cursor_base_x = 50 + 208;
          cursor_x = cursor_base_x;
          linha_len = 0;
        }
      } else {
        inserir_char(c, fb, pitch, width, height, &cursor_x, &cursor_y,
                     linha_atual, &linha_len);
      }
      break;
    }
    }
  }
}