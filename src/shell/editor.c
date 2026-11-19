#include "gfx.h"
#include "include/video.h"
#include "keyboard.h"
#include "layout.h"
#include "render.h"
#include "simplefs.h"
#include <stdint.h>

static int editor_append_str(char *dst, int pos, int max, const char *src) {
  while (*src && pos < max - 1)
    dst[pos++] = *src++;
  dst[pos] = '\0';
  return pos;
}

static int editor_append_uint(char *dst, int pos, int max, uint32_t val) {
  char tmp[12];
  int i = 0;
  if (val == 0) {
    if (pos < max - 1)
      dst[pos++] = '0';
    dst[pos] = '\0';
    return pos;
  }
  while (val > 0) {
    tmp[i++] = (char)('0' + (val % 10U));
    val /= 10U;
  }
  while (i > 0 && pos < max - 1)
    dst[pos++] = tmp[--i];
  dst[pos] = '\0';
  return pos;
}

static void editor_format_kb(char *dst, int max, uint32_t bytes) {
  uint32_t kb_int = bytes / 1024U;
  uint32_t kb_dec = ((bytes % 1024U) * 100U) / 1024U;
  if (bytes > 0U && kb_int == 0U && kb_dec == 0U)
    kb_dec = 1U;

  int pos = editor_append_str(dst, 0, max, "[");
  pos = editor_append_uint(dst, pos, max, kb_int);
  pos = editor_append_str(dst, pos, max, ".");
  if (kb_dec < 10U && pos < max - 1)
    dst[pos++] = '0';
  pos = editor_append_uint(dst, pos, max, kb_dec);
  pos = editor_append_str(dst, pos, max, " / 4.00 KB]");
}

static void editor_draw(const layout_bounds_t *area, const char *filename,
                        const char *buf, uint32_t len, const char *status_msg) {
  video_fill_rect(area->x, area->y, area->width, area->height, LAYOUT_BG);

  /* Barra superior do editor */
  int y = area->y;
  gfx_draw_string("[EDIT] ", area->x, y, LAYOUT_ACCENT);
  gfx_draw_string(filename, area->x + 7 * LAYOUT_CELL_W, y, LAYOUT_PROMPT);

  int info_x = area->x + 28 * LAYOUT_CELL_W;
  if (info_x < area->x + area->width) {
    gfx_draw_string("Ctrl+O: Salvar | Ctrl+X: Sair", info_x, y,
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

  char kb_info[32];
  editor_format_kb(kb_info, sizeof(kb_info), len);
  int kb_len = 0;
  while (kb_info[kb_len] != '\0')
    kb_len++;
  int kb_x = area->x + area->width - kb_len * LAYOUT_CELL_W;
  if (kb_x > area->x) {
    gfx_draw_string(kb_info, kb_x, status_y, LAYOUT_MUTED);
  }

  render_flush();
}

void editor_open(const char *filename) {
  layout_bounds_t area = layout_get_terminal_bounds();
  if (!area.width) {
    return;
  }

  static char buf[SFS_FILE_MAX_SIZE];
  static char editor_clipboard[SFS_FILE_MAX_SIZE];
  static uint32_t editor_clip_len = 0;
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

    /* Sair com Ctrl+X ou ESC (27) */
    if (tecla == TECLA_CTRL_X || tecla == 27) {
      break;
    }

    /* Salvar com Ctrl+O */
    if (tecla == TECLA_CTRL_O) {
      if (sfs_write_file(filename, buf, len) == 0) {
        status = "Salvo com sucesso no SimpleFS!";
      } else {
        status = "Erro ao salvar no disco ATA.";
      }
      editor_draw(&area, filename, buf, len, status);
      continue;
    }

    /* Copiar linha atual com Ctrl+C */
    if (tecla == TECLA_CTRL_C) {
      uint32_t line_start = len;
      while (line_start > 0 && buf[line_start - 1] != '\n') {
        line_start--;
      }
      editor_clip_len = 0;
      for (uint32_t i = line_start; i < len && editor_clip_len < SFS_FILE_MAX_SIZE - 1; i++) {
        editor_clipboard[editor_clip_len++] = buf[i];
      }
      editor_clipboard[editor_clip_len] = '\0';
      status = "Linha copiada (Ctrl+V para colar).";
      editor_draw(&area, filename, buf, len, status);
      continue;
    }

    /* Colar com Ctrl+V */
    if (tecla == TECLA_CTRL_V) {
      if (editor_clip_len > 0) {
        uint32_t i = 0;
        while (i < editor_clip_len && len < SFS_FILE_MAX_SIZE - 1) {
          buf[len++] = editor_clipboard[i++];
        }
        buf[len] = '\0';
        status = "Texto colado (Ctrl+O para salvar).";
        editor_draw(&area, filename, buf, len, status);
      }
      continue;
    }

    /* Apagar caractere */
    if (tecla == '\b') {
      if (len > 0) {
        len--;
        buf[len] = '\0';
        status = "Modificado (Ctrl+O para salvar).";
        editor_draw(&area, filename, buf, len, status);
      }
      continue;
    }

    /* Inserir caractere ou quebra de linha */
    if (tecla == '\n' && len < SFS_FILE_MAX_SIZE - 1) {
      buf[len++] = '\n';
      buf[len] = '\0';
      status = "Modificado (Ctrl+O para salvar).";
      editor_draw(&area, filename, buf, len, status);
      continue;
    }

    if (tecla >= 32 && tecla <= 255 && len < SFS_FILE_MAX_SIZE - 1) {
      buf[len++] = (char)tecla;
      buf[len] = '\0';
      status = "Modificado (Ctrl+O para salvar).";
      editor_draw(&area, filename, buf, len, status);
    }
  }
}