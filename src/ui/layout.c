#include "layout.h"
#include "gfx.h"
#include "render.h"
#include "version.h"
#include "video.h"

#define PANEL  0x00071D1AU
#define BORDER 0x0005614BU
#define MARGIN 24
#define HEADER 60
#define FOOTER 40
#define TITLE  36
#define INSET  16

static layout_bounds_t terminal;

layout_bounds_t layout_get_terminal_bounds(void) {
    return terminal;
}

/* Em modos estreitos, omite o rotulo direito antes de sobrepor o esquerdo. */
static void draw_bar(int y, int height, int width, const char *left,
                     const char *right, int right_chars) {
    int x = MARGIN;
    int text_y = y + (height - LAYOUT_CELL_H) / 2;
    gfx_draw_rect(0, y, width, height, PANEL);
    while (*left && x + LAYOUT_CELL_W <= width - MARGIN) {
        gfx_draw_char(*left++, x, text_y, LAYOUT_TEXT);
        x += LAYOUT_CELL_W;
    }
    int right_x = width - MARGIN - right_chars * LAYOUT_CELL_W;
    if (right_x >= x + LAYOUT_CELL_W)
        gfx_draw_string(right, right_x, text_y, LAYOUT_MUTED);
}

static void draw_footer(int w, int h) {
    static const char keys[] = "Ctrl+L limpar  Ctrl+C copiar  Ctrl+V colar";
    int buffered = video_is_double_buffered();
    static const char active[] = "Shell | " OS_VERSION "-" OS_STAGE " | Backbuffer ativo";
    static const char inactive[] = "Shell | " OS_VERSION "-" OS_STAGE " | Backbuffer inativo";
    const char *status = buffered ? active : inactive;
    int columns = (w - 2 * MARGIN) / LAYOUT_CELL_W;
    if (columns < (int)sizeof(inactive) - 1)
        status = buffered ? OS_VERSION "-" OS_STAGE " | BB on" : OS_VERSION "-" OS_STAGE " | BB off";
    draw_bar(h - FOOTER, FOOTER, w, status, keys, sizeof(keys) - 1);
    gfx_draw_rect(0, h - FOOTER, w, 1, BORDER);
}

void layout_draw(void) {
    int w = video_get_width(), h = video_get_height();
    int frame_w = w - 2 * MARGIN;
    int frame_y = HEADER + MARGIN;
    int bottom = h - FOOTER - MARGIN;
    int frame_h = bottom - frame_y;
    static const char version[] = OS_VERSION "-" OS_STAGE;

    terminal = (layout_bounds_t){0};
    if (!video_is_double_buffered() || frame_w < 2 * INSET + 16 * LAYOUT_CELL_W ||
        frame_h < TITLE + 2 * INSET + 2 * LAYOUT_CELL_H)
        return;

    terminal.x = MARGIN + INSET;
    terminal.y = frame_y + TITLE + INSET;
    terminal.width = (frame_w - 2 * INSET) / LAYOUT_CELL_W * LAYOUT_CELL_W;
    terminal.height = (frame_h - TITLE - 2 * INSET) / LAYOUT_CELL_H * LAYOUT_CELL_H;

    video_clear(LAYOUT_BG);
    draw_bar(0, HEADER, w, "VGOS", version, sizeof(version) - 1);
    gfx_draw_rect(0, HEADER - 1, w, 1, BORDER);
    gfx_draw_rect(MARGIN, frame_y, frame_w, 1, BORDER);
    gfx_draw_rect(MARGIN, frame_y + 1, 1, frame_h - 2, BORDER);
    gfx_draw_rect(w - MARGIN - 1, frame_y + 1, 1, frame_h - 2, BORDER);
    gfx_draw_rect(MARGIN, bottom - 1, frame_w, 1, BORDER);
    gfx_draw_rect(MARGIN + 1, frame_y + 1, frame_w - 2, TITLE - 2, PANEL);
    gfx_draw_string("Terminal", terminal.x, frame_y + (TITLE - LAYOUT_CELL_H) / 2,
                    LAYOUT_TEXT);
    gfx_draw_rect(MARGIN, frame_y + TITLE - 1, frame_w, 1, BORDER);
    draw_footer(w, h);
}

void layout_next_line(int *y) {
    if (!y || !terminal.height)
        return;
    int last = terminal.y + terminal.height - LAYOUT_CELL_H;
    /* A API publica recebe pixels; normalize antes de acessar a grade. */
    if (*y < terminal.y) {
        *y = terminal.y;
        return;
    }
    if (*y < last)
        *y -= (*y - terminal.y) % LAYOUT_CELL_H;
    if (*y < last) {
        *y += LAYOUT_CELL_H;
        return;
    }

    /* Copia de cima para baixo: a origem sobreposta sempre fica adiante. */
    unsigned stride = video_get_draw_pitch() / sizeof(uint32_t);
    uint32_t *dst = video_get_draw_buffer() + terminal.y * stride + terminal.x;
    const uint32_t *src = dst + LAYOUT_CELL_H * stride;
    for (int row = terminal.y; row < last; row++, dst += stride, src += stride) {
        for (int x = 0; x < terminal.width; x++)
            dst[x] = src[x];
    }
    gfx_draw_rect(terminal.x, last, terminal.width, LAYOUT_CELL_H, LAYOUT_BG);
    *y = last;
}

void layout_write_line(const char *text, uint32_t color, int *y) {
    if (!text || !y || !terminal.width)
        return;
    layout_next_line(y);
    int x = terminal.x;
    uint32_t *buffer = video_get_draw_buffer();
    uint32_t pitch = video_get_draw_pitch();
    while (*text) {
        if (x == terminal.x + terminal.width) {
            layout_next_line(y);
            x = terminal.x;
        }
        desenhar_char_com_fundo(*text++, x, *y, color, LAYOUT_BG, buffer, pitch);
        x += LAYOUT_CELL_W;
    }
}
