#include "lang.h"
#include <stdint.h>

#define MULTIBOOT_INFO_CMDLINE (1U << 2)

static lang_id_t g_lang = LANG_PT;

static int cmdline_has_lang_en(const char *s) {
  if (!s)
    return 0;

  for (int i = 0; i < 512 && s[i] != '\0'; i++) {
    if (s[i] == 'l' && s[i + 1] == 'a' && s[i + 2] == 'n' &&
        s[i + 3] == 'g' && s[i + 4] == '=') {
      char c0 = s[i + 5];
      char c1 = s[i + 6];
      if ((c0 == 'e' || c0 == 'E') && (c1 == 'n' || c1 == 'N'))
        return 1;
    }
  }
  return 0;
}

void lang_init_from_multiboot(const multiboot_info_t *mbd) {
  g_lang = LANG_PT;
  if (!mbd)
    return;

  if ((mbd->flags & MULTIBOOT_INFO_CMDLINE) && mbd->cmdline != 0U) {
    const char *cmdline = (const char *)(uintptr_t)mbd->cmdline;
    if (cmdline_has_lang_en(cmdline))
      g_lang = LANG_EN;
  }
}

void lang_set(lang_id_t id) {
  g_lang = (id == LANG_EN) ? LANG_EN : LANG_PT;
}

lang_id_t lang_get(void) {
  return g_lang;
}

const char *lang_pick(const char *pt, const char *en) {
  return (g_lang == LANG_EN) ? en : pt;
}
