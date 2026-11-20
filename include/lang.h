#ifndef LANG_H
#define LANG_H

#include "include/multiboot.h"

typedef enum {
  LANG_PT = 0,
  LANG_EN = 1
} lang_id_t;

void lang_init_from_multiboot(const multiboot_info_t *mbd);
void lang_set(lang_id_t id);
lang_id_t lang_get(void);
const char *lang_pick(const char *pt, const char *en);

#define L(pt, en) lang_pick((pt), (en))

#endif
