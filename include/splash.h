#ifndef SPLASH_H
#define SPLASH_H

#include <stdint.h>

void splash_show(uint32_t width, uint32_t height);
void splash_progress(uint32_t width, uint32_t height, uint32_t pct,
                     const char *mensagem);
void splash_finish(void);

#endif
