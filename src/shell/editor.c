#include "gfx.h"
#include "include/video.h"
#include "keyboard.h"
#include "layout.h"
#include "render.h"
#include "simplefs.h"
#include <stdint.h>

static void editor_draw(const layout_bounds_t *area, const char *filename,
                        const char *buf, uint32_t len, const char *status_msg) {
  video_fill_rect(area->x, area->y, area->width, area->height, LAYOUT_BG);

  /* Barra superior do editor */
  int y = area->y;
  gfx_draw_string("[EDIT] ", area->x, y, LAYOUT_ACCENT);
  gfx_draw_string(filename, area->x + 7 * LAYOUT_CELL_W, y, LAYOUT_PROMPT);

  int info_x = area->x + 28 * LAYOUT_CELL_W;
  if (info_x < area->x + area->width) {
    gfx_draw_string("Ctrl+C: Salvar | ESC/Ctrl+L: Sair", info_x, y,
                    LAYOUT_MUTED);
  }

  y += LAYOUT_CELL_H;
  video_fill_rect(area->x, y + 2, area->width, 1, LAYOUT_MUTED);
  y += LAYOUT_CELL_H;

  /* Area de texto */
  int text_max_y = area->y + area->height - (2 * LAYOUT_CELL_H);
  int x = area->x;

  for (uint32_t i = 0; i < len; i++) {
    char c = buf[i];
    if (c == '\n') {
      x = area->x;
      y += LAYOUT_CELL_H;
      if (y >= text_max_y) {
        break;
      }
      continue;
    }

    desenhar_char_com_fundo(c, x, y, LAYOUT_TEXT, LAYOUT_BG,
                            video_get_draw_buffer(), video_get_draw_pitch());
    x += LAYOUT_CELL_W;
    if (x + LAYOUT_CELL_W > area->x + area->width) {
      x = area->x;
      y += LAYOUT_CELL_H;
      if (y >= text_max_y) {
        break;
      }
    }
  }

  /* Cursor */
  if (y < text_max_y) {
    video_fill_rect(x, y + LAYOUT_CELL_H - 2, LAYOUT_CELL_W, 2, LAYOUT_ACCENT);
  }

  /* Barra de status inferior */
  int status_y = area->y + area->height - LAYOUT_CELL_H;
  if (status_msg != 0 && status_msg[0] != '\0') {
    gfx_draw_string(status_msg, area->x, status_y, LAYOUT_ACCENT);
  }

  render_flush();
}

void editor_open(const char *filename) {
  layout_bounds_t area = layout_get_terminal_bounds();
  if (!area.width) {
    return;
  }

  static char buf[SFS_FILE_MAX_SIZE];
  uint32_t len = 0;
  const char *status = "Novo arquivo (nao salvo).";

  if (sfs_read_file(filename, buf, SFS_FILE_MAX_SIZE, &len) == 0) {
    status = "Arquivo carregado do disco.";
  } else {
    len = 0;
    buf[0] = '\0';
  }

  editor_draw(&area, filename, buf, len, status);

  while (1) {
    int tecla = teclado_ler();
    if (tecla == TECLA_NENHUMA) {
      teclado_aguardar();
      continue;
    }

    /* Sair com ESC (27) ou Ctrl+L */
    if (tecla == 27) {
      break;
    }
    if (tecla == TECLA_CTRL_L) {
      break;
    }

    /* Salvar com Ctrl+C */
    if (tecla == TECLA_CTRL_C) {
      if (sfs_write_file(filename, buf, len) == 0) {
        status = "Salvo com sucesso no SimpleFS!";
      } else {
        status = "Erro ao salvar no disco ATA.";
      }
      editor_draw(&area, filename, buf, len, status);
      continue;
    }

    /* Apagar caractere */
    if (tecla == '\b') {
      if (len > 0) {
        len--;
        buf[len] = '\0';
        status = "Modificado (Ctrl+C para salvar).";
        editor_draw(&area, filename, buf, len, status);
      }
      continue;
    }

    /* Inserir caractere ou quebra de linha */
    if (tecla == '\n' && len < SFS_FILE_MAX_SIZE - 1) {
      buf[len++] = '\n';
      buf[len] = '\0';
      status = "Modificado (Ctrl+C para salvar).";
      editor_draw(&area, filename, buf, len, status);
      continue;
    }

    if (tecla >= 32 && tecla <= 255 && len < SFS_FILE_MAX_SIZE - 1) {
      buf[len++] = (char)tecla;
      buf[len] = '\0';
      status = "Modificado (Ctrl+C para salvar).";
      editor_draw(&area, filename, buf, len, status);
    }
  }
}