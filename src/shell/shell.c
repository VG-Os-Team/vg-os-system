#include "shell.h"

#include "comandos.h"
#include "gfx.h"
#include "include/panic.h"
#include "include/video.h"
#include "keyboard.h"
#include "layout.h"
#include "render.h"
#include "simplefs.h"
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

static int str_starts_with(const char *s, const char *prefix) {
  while (*prefix) {
    if (*s != *prefix)
      return 0;
    s++;
    prefix++;
  }
  return 1;
}

static int str_equals(const char *a, const char *b) {
  while (*a && (*a == *b)) {
    a++;
    b++;
  }
  return (*a == '\0' && *b == '\0');
}

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
  int cells = (e->area.width / LAYOUT_CELL_W) * (e->area.height / LAYOUT_CELL_H);
  if (e->len >= LINHA_MAX - 1)
    return 0;
  if (e->len + (int)sizeof(SHELL_PROMPT) >= cells)
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

static int executar_comando_fs(entrada_t *e) {
  /* Comando: edit <filename> */
  if (str_starts_with(e->linha, "edit ")) {
    const char *nome = e->linha + 5;
    while (*nome == ' ')
      nome++;
    if (*nome == '\0') {
      layout_next_line(&e->y);
      layout_write_line("Uso: edit <nome_do_arquivo>", LAYOUT_MUTED, &e->y);
      layout_next_line(&e->y);
      iniciar_prompt(e);
      return 1;
    }
    if (!sfs_is_ready()) {
      layout_next_line(&e->y);
      layout_write_line("Erro: Disco ATA/SimpleFS nao detectado.", LAYOUT_MUTED,
                        &e->y);
      layout_next_line(&e->y);
      iniciar_prompt(e);
      return 1;
    }
    editor_open(nome);
    limpar_terminal(e);
    return 1;
  }

  /* Comando: ls */
  if (str_equals(e->linha, "ls")) {
    layout_next_line(&e->y);
    if (!sfs_is_ready()) {
      layout_write_line("Erro: SimpleFS nao montado.", LAYOUT_MUTED, &e->y);
    } else {
      int achou = 0;
      sfs_entry_t entry;
      for (int i = 0; i < SFS_MAX_FILES; i++) {
        if (sfs_get_entry(i, &entry)) {
          layout_write_line(entry.name, LAYOUT_TEXT, &e->y);
          achou = 1;
        }
      }
      if (!achou) {
        layout_write_line("(nenhum arquivo salvo)", LAYOUT_MUTED, &e->y);
      }
    }
    layout_next_line(&e->y);
    iniciar_prompt(e);
    return 1;
  }

  /* Comando: rm <filename> */
  if (str_starts_with(e->linha, "rm ")) {
    const char *nome = e->linha + 3;
    while (*nome == ' ')
      nome++;
    layout_next_line(&e->y);
    if (sfs_delete_file(nome) == 0) {
      layout_write_line("Arquivo removido.", LAYOUT_TEXT, &e->y);
    } else {
      layout_write_line("Arquivo nao encontrado.", LAYOUT_MUTED, &e->y);
    }
    layout_next_line(&e->y);
    iniciar_prompt(e);
    return 1;
  }

  return 0;
}

void shell_run(void) {
  uint32_t width = video_get_width();

  /* Inicializa o driver ATA e monta/formata o SimpleFS */
  sfs_init();

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
      if (entrada.x == entrada.area.x)
        entrada.y -= LAYOUT_CELL_H;
      entrada.linha[entrada.len] = '\0';

      if (executar_comando_fs(&entrada)) {
        break;
      }

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
      if (tecla < 0)
        continue;
      if (!inserir_char(&entrada, (char)tecla))
        continue;
    }

    marcar_cursor(&entrada, LAYOUT_ACCENT);
    render_flush();
  }
}