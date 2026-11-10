#include "comandos.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "keyboard.h"
#include "logo.h"
#include "pic/pic.h"
#include "render.h"
#include "strutil.h"
#include "version.h"
#include "include/panic.h"
#include <stdint.h>
#include <stddef.h>

#include "include/multiboot.h"
#include "include/pmm.h"
#include "include/paging.h"
#include "include/heap.h"
#include "include/video.h"

#define LINHA_MAX 1024

static inline void outw(uint16_t port, uint16_t val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

// Copia provisoria do backbuffer (RAM) para a VRAM fisica ate a criacao
// de render_flush() na Tarefa 19, mantendo o terminal visivel e sem flicker.
static void sincronizar_vram_provisorio(void) {
  const uint32_t *src = video_get_backbuffer();
  uint32_t *dst = video_get_framebuffer();
  if (!src || !dst || src == dst)
    return;

  uint32_t width = video_get_width();
  uint32_t height = video_get_height();
  uint32_t src_pitch = video_get_backbuffer_pitch();
  uint32_t dst_pitch = video_get_pitch();

  if (src_pitch == dst_pitch && src_pitch == width * 4U) {
    uint32_t total_pixels = width * height;
    asm volatile("cld; rep movsl"
                 : "+S"(src), "+D"(dst), "+c"(total_pixels)
                 :
                 : "memory", "cc");
    return;
  }

  for (uint32_t y = 0; y < height; y++) {
    const uint32_t *linha_src =
        (const uint32_t *)((const uint8_t *)src + y * src_pitch);
    uint32_t *linha_dst = (uint32_t *)((uint8_t *)dst + y * dst_pitch);
    uint32_t cols = width;
    asm volatile("cld; rep movsl"
                 : "+S"(linha_src), "+D"(linha_dst), "+c"(cols)
                 :
                 : "memory", "cc");
  }
}

static void limpar_terminal(uint32_t *fb, uint32_t pitch, uint32_t width,
                            uint32_t height, int terminal_y_inicial,
                            int *cursor_x, int *cursor_y, int *cursor_base_x,
                            int *linha_len) {
  video_fill_rect(0, (uint32_t)terminal_y_inicial, width,
                  height - (uint32_t)terminal_y_inicial, 0x00000000);

  *cursor_y = terminal_y_inicial;
  desenhar_string("root@vgos:~# ", 50, *cursor_y, 0x0000FF00, fb, pitch);
  *cursor_base_x = 50 + 208;
  *cursor_x = *cursor_base_x;
  *linha_len = 0;
}

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
  // Mantém interrupções mascaráveis desabilitadas durante a inicialização
  asm volatile("cli" ::: "memory");

  // 1. Inicializa o driver de Framebuffer VESA (1024x768 32bpp)
  video_status_t vstatus = video_init(mbd);
  if (vstatus != VIDEO_OK)
    kernel_panic(video_status_str(vstatus), 0);

  uint32_t *fb    = video_get_framebuffer();
  uint32_t pitch  = video_get_pitch();
  uint32_t width  = video_get_width();
  uint32_t height = video_get_height();

  // O framebuffer só fica habilitado no panic se toda a geometria for válida.
  if (!panic_configurar_video(fb, pitch, width, height))
    kernel_panic("Geometria de framebuffer invalida.", 0);

  // 2. Inicializa o parser do mapa de memória Multiboot e PMM
  multiboot_mem_status_t mem_status = multiboot_mem_init(mbd);
  if (mem_status != MULTIBOOT_MEM_OK)
    kernel_panic("Falha ao ler o mapa de memoria Multiboot.", 0);

  pmm_init(mbd);

  // 3. Inicializa a GDT, IDT e PIC
  gdt_instalar();
  idt_install();
  isr_install();
  irq_install();
  pic_init();

  // 4. Ativa a paginação x86 (identity map do kernel e da VRAM do framebuffer)
  if (paging_init((uint32_t)(uintptr_t)fb, video_get_size_bytes()) != PAGING_OK)
    kernel_panic("Falha ao ativar a paginacao.", 0);

  // 5. Heap dinâmico do kernel (kmalloc/kfree), sobre páginas pedidas ao PMM
  if (heap_init_from_pmm(HEAP_INITIAL_SIZE) != 0)
    kernel_panic("Falha ao inicializar o heap do kernel.", 0);

  // 6. Aloca o backbuffer na RAM para Double Buffering (Tarefa 16)
  vstatus = video_init_backbuffer();
  if (vstatus != VIDEO_OK)
    kernel_panic(video_status_str(vstatus), 0);

  // Direciona todas as operacoes de desenho para o backbuffer secundario na RAM
  fb    = video_get_draw_buffer();
  pitch = video_get_draw_pitch();

  // Limpa o backbuffer inteiro com fundo preto via driver de vídeo
  video_clear(0x00000000);

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
  int header_gap = 50;                        // espaço entre logo e texto
  int total_header_w = (int)logo_width + header_gap + art_total_w;
  int start_x = ((int)width - total_header_w) / 2;
  if (start_x < 20)
    start_x = 20;

  int logo_x = start_x;
  int logo_y = 20;
  desenhar_logo(fb, pitch, logo_x, logo_y);

  int art_x = logo_x + (int)logo_width + header_gap;
  int art_y = logo_y + ((int)logo_height - let_h) / 2;

  // Renderizar as 4 letras em verde escuro usando o driver de vídeo
  uint32_t cor_vgos = 0x00228B22;
  const uint16_t *letras[] = {letra_V, letra_G, letra_O, letra_S};
  for (int li = 0; li < 4; li++) {
    int lx = art_x + li * (let_w + let_gap);
    for (int r = 0; r < lrows; r++) {
      uint16_t bits = letras[li][r];
      for (int c = 0; c < lcols; c++) {
        if (bits & (1 << c)) {
          int bx = lx + c * blk_w;
          int by = art_y + r * blk_h;
          video_fill_rect((uint32_t)bx, (uint32_t)by, (uint32_t)blk_w,
                          (uint32_t)blk_h, cor_vgos);
        }
      }
    }
  }

  // Linha separadora subtil
  int margin = 50;
  int sep_y = logo_y + (int)logo_height + 15;
  for (uint32_t sx = (uint32_t)margin; sx < width - (uint32_t)margin; sx++)
    video_put_pixel(sx, (uint32_t)sep_y, 0x00333333);

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

  // Apresenta o quadro inicial completo na VRAM de uma unica vez (sem flicker)
  sincronizar_vram_provisorio();

  // O Loop do Terminal
  while (1) {
    int tecla = teclado_ler();
    if (tecla == TECLA_NENHUMA) {
      asm volatile("pause");
      continue;
    }

    int atualizar_tela = 0;

    switch (tecla) {
    case TECLA_CTRL_L:
      limpar_terminal(fb, pitch, width, height, terminal_y_inicial, &cursor_x,
                      &cursor_y, &cursor_base_x, &linha_len);
      atualizar_tela = 1;
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
      if (clipboard_len > 0)
        atualizar_tela = 1;
      break;

    default: {
      if (tecla < 0) {
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
            atualizar_tela = 1;
          } else if (cursor_y > terminal_y_inicial) {
            cursor_y -= 24;
            int chars_por_linha = (width - 100) / 16;
            cursor_x = 50 + (chars_por_linha - 1) * 16;
            desenhar_char(' ', cursor_x, cursor_y, 0x00FFFFFF, fb, pitch);
            linha_len--;
            atualizar_tela = 1;
          }
        }
      } else if (c == '\n') {
        comando_resultado_t r = comandos_executar(linha_atual, linha_len, fb,
                                                  pitch, width, &cursor_y);

        if (r == COMANDO_LIMPAR) {
          limpar_terminal(fb, pitch, width, height, terminal_y_inicial,
                          &cursor_x, &cursor_y, &cursor_base_x, &linha_len);
          atualizar_tela = 1;
        } else if (r == COMANDO_EXIT) {
          if (cursor_y + 24 < (int)height - 24)
            cursor_y += 24;
          desenhar_string("Desligando o VG-OS...", 50, cursor_y, 0x00FFFF00, fb, pitch);
          sincronizar_vram_provisorio();

          outw(0x604, 0x2000);   // QEMU ACPI shutdown
          outw(0xB004, 0x2000);  // Bochs / QEMU legado
          outw(0x4004, 0x3400);  // VirtualBox

          if (cursor_y + 24 < (int)height - 24)
            cursor_y += 24;
          desenhar_string("Sistema finalizado. Pode desligar o computador com seguranca.",
                          50, cursor_y, 0x00AAAAAA, fb, pitch);
          sincronizar_vram_provisorio();
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
          atualizar_tela = 1;
        }
      } else {
        inserir_char(c, fb, pitch, width, height, &cursor_x, &cursor_y,
                     linha_atual, &linha_len);
        atualizar_tela = 1;
      }
      break;
    }
    }

    if (atualizar_tela)
      sincronizar_vram_provisorio();
  }
}