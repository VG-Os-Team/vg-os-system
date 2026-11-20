#include "splash.h"

#include "gfx.h"
#include "images/logo/logo.h"
#include "include/video.h"
#include "lang.h"
#include "layout.h"
#include "render.h"
#include "strutil.h"
#include "version.h"

#define SPLASH_BAR_WIDTH  320U
#define SPLASH_BAR_HEIGHT 14U
#define SPLASH_BAR_BORDER 0x00114842U
#define SPLASH_BAR_BG     0x00051F1DU
#define SPLASH_STEP_DELAY 2500000U
#define SPLASH_END_DELAY  8000000U

static void splash_delay(uint32_t ciclos) {
  for (volatile uint32_t i = 0; i < ciclos; i++)
    asm volatile("pause");
}

static void calcular_pos_barra(uint32_t width, uint32_t height,
                               uint32_t *bar_x, uint32_t *bar_y) {
  int logo_y = ((int)height - (int)logo_height) / 2 - 48;
  if (logo_y < 0)
    logo_y = 0;

  int banner_y = logo_y + (int)logo_height + 20;
  *bar_x = (width > SPLASH_BAR_WIDTH) ? (width - SPLASH_BAR_WIDTH) / 2U : 0U;
  *bar_y = (uint32_t)(banner_y + RENDER_CHAR_HEIGHT + 18);
}

void splash_show(uint32_t width, uint32_t height) {
  video_clear(LAYOUT_BG);

  int logo_x = ((int)width - (int)logo_width) / 2;
  int logo_y = ((int)height - (int)logo_height) / 2 - 48;
  if (logo_x < 0)
    logo_x = 0;
  if (logo_y < 0)
    logo_y = 0;

  if (width >= logo_width && height >= logo_height) {
    desenhar_logo(video_get_draw_buffer(), video_get_draw_pitch(),
                  logo_x, logo_y);
  }

  const char *banner =
      L(OS_BANNER, OS_NAME " Version " OS_VERSION "-" OS_STAGE);
  int banner_len = minha_strlen(banner);
  int banner_x = ((int)width - banner_len * RENDER_CHAR_WIDTH) / 2;
  int banner_y = logo_y + (int)logo_height + 20;
  if (banner_x < 0)
    banner_x = 0;
  gfx_draw_string(banner, (uint32_t)banner_x, (uint32_t)banner_y, LAYOUT_TEXT);

  uint32_t bar_x, bar_y;
  calcular_pos_barra(width, height, &bar_x, &bar_y);
  gfx_draw_rect(bar_x, bar_y, SPLASH_BAR_WIDTH, SPLASH_BAR_HEIGHT, SPLASH_BAR_BORDER);
  gfx_draw_rect(bar_x + 2U, bar_y + 2U,
                SPLASH_BAR_WIDTH - 4U, SPLASH_BAR_HEIGHT - 4U, SPLASH_BAR_BG);
}

void splash_progress(uint32_t width, uint32_t height, uint32_t pct,
                     const char *mensagem) {
  if (pct > 100U)
    pct = 100U;

  uint32_t bar_x, bar_y;
  calcular_pos_barra(width, height, &bar_x, &bar_y);

  uint32_t inner_w = SPLASH_BAR_WIDTH - 4U;
  uint32_t fill_w = (inner_w * pct) / 100U;
  if (fill_w > 0U) {
    gfx_draw_rect(bar_x + 2U, bar_y + 2U,
                  fill_w, SPLASH_BAR_HEIGHT - 4U, LAYOUT_ACCENT);
  }

  uint32_t msg_y = bar_y + SPLASH_BAR_HEIGHT + 14U;
  video_fill_rect(0, msg_y, width, RENDER_CHAR_HEIGHT, LAYOUT_BG);

  int msg_len = minha_strlen(mensagem);
  int msg_x = ((int)width - msg_len * RENDER_CHAR_WIDTH) / 2;
  if (msg_x < 0)
    msg_x = 0;
  gfx_draw_string(mensagem, (uint32_t)msg_x, msg_y, LAYOUT_MUTED);

  if (pct < 100U)
    splash_delay(SPLASH_STEP_DELAY);
}

void splash_finish(void) {
  render_flush();
  splash_delay(SPLASH_END_DELAY);
}
