#include "shell.h"

#include "comandos.h"
#include "gfx.h"
#include "include/panic.h"
#include "include/video.h"
#include "keyboard.h"
#include "layout.h"
#include "render.h"
#include <stdint.h>

#define LINHA_MAX 1024
#define SHELL_PROMPT "root@vgos:~# "

static inline void outw(uint16_t port, uint16_t val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

typedef struct {
  layout_bounds_t area;
  int x, y, len;
  char linha[LINHA_MAX];
} entrada_t;

static void iniciar_prompt(entrada_t *e) {
  gfx_draw_string(SHELL_PROMPT, e->area.x, e->y, LAYOUT_PROMPT);
  e->x = e->area.x + (int)(sizeof(SHELL_PROMPT) - 1) * LAYOUT_CELL_W;
  e->len = 0;
  e->linha[0] = '\0';
}

static void limpar_terminal(entrada_t *e) {
  video_fill_rect(e->area.x, e->area.y, e->area.width, e->area.height,
                  LAYOUT_BG);
  e->y = e->area.y;
  iniciar_prompt(e);
}

static void marcar_cursor(const entrada_t *e, uint32_t cor) {
  video_fill_rect(e->x, e->y + LAYOUT_CELL_H - 2, LAYOUT_CELL_W, 2, cor);
}

static int inserir_char(entrada_t *e, char c) {
  /* Mantem toda a entrada editavel visivel, inclusive a celula do cursor. */
  int cells = (e->area.width / LAYOUT_CELL_W) * (e->area.height / LAYOUT_CELL_H);
  if (e->len >= LINHA_MAX - 1 || e->len + (int)sizeof(SHELL_PROMPT) >= cells)
    return 0;

  desenhar_char_com_fundo(c, e->x, e->y, LAYOUT_TEXT, LAYOUT_BG,
                          video_get_draw_buffer(), video_get_draw_pitch());
  e->linha[e->len++] = c;
  e->linha[e->len] = '\0';
  e->x += LAYOUT_CELL_W;
  if (e->x == e->area.x + e->area.width) {
    e->x = e->area.x;
    layout_next_line(&e->y);
  }
  return 1;
}

static int apagar_char(entrada_t *e) {
  if (!e->len)
    return 0;

  marcar_cursor(e, LAYOUT_BG);
  if (e->x == e->area.x) {
    e->x += e->area.width;
    e->y -= LAYOUT_CELL_H;
  }
  e->x -= LAYOUT_CELL_W;
  e->len--;
  e->linha[e->len] = '\0';
  video_fill_rect(e->x, e->y, LAYOUT_CELL_W, LAYOUT_CELL_H, LAYOUT_BG);
  return 1;
}

void shell_run(void) {
  uint32_t width = video_get_width();

  layout_draw();
  entrada_t entrada = {.area = layout_get_terminal_bounds()};
  if (!entrada.area.width)
    kernel_panic("Resolucao insuficiente para o terminal.", 0);

  entrada.y = entrada.area.y;
  iniciar_prompt(&entrada);
  marcar_cursor(&entrada, LAYOUT_ACCENT);
  render_flush();

  char clipboard[LINHA_MAX];
  int clipboard_len = 0;

  while (1) {
    int tecla = teclado_ler();
    if (tecla == TECLA_NENHUMA) {
      teclado_aguardar();
      continue;
    }

    switch (tecla) {
    case TECLA_CTRL_L:
      limpar_terminal(&entrada);
      break;
    case TECLA_CTRL_C:
      for (int i = 0; i < entrada.len; i++)
        clipboard[i] = entrada.linha[i];
      clipboard_len = entrada.len;
      continue;
    case TECLA_CTRL_V: {
      int i = 0;
      while (i < clipboard_len && inserir_char(&entrada, clipboard[i]))
        i++;
      if (!i)
        continue;
      break;
    }
    case '\b':
      if (!apagar_char(&entrada))
        continue;
      break;
    case '\n': {
      marcar_cursor(&entrada, LAYOUT_BG);
      /* O cursor ja quebrou para a proxima linha ao preencher a anterior. */
      if (entrada.x == entrada.area.x)
        entrada.y -= LAYOUT_CELL_H;
      entrada.linha[entrada.len] = '\0';
      comando_resultado_t r = comandos_executar(
          entrada.linha, entrada.len, video_get_draw_buffer(),
          video_get_draw_pitch(), width, &entrada.y);
      if (r == COMANDO_LIMPAR) {
        limpar_terminal(&entrada);
      } else if (r == COMANDO_EXIT) {
        layout_write_line("Desligando o VG-OS...", LAYOUT_TEXT, &entrada.y);
        render_flush();
        outw(0x604, 0x2000);
        outw(0xB004, 0x2000);
        outw(0x4004, 0x3400);
        layout_write_line(
            "Sistema finalizado. Pode desligar o computador com seguranca.",
            LAYOUT_MUTED, &entrada.y);
        render_flush();
        asm volatile("cli");
        while (1)
          asm volatile("hlt");
      } else {
        layout_next_line(&entrada.y);
        iniciar_prompt(&entrada);
      }
      break;
    }
    default:
      if (tecla < 0 || !inserir_char(&entrada, (char)tecla))
        continue;
    }

    marcar_cursor(&entrada, LAYOUT_ACCENT);
    render_flush();
  }
}
