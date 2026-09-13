#ifndef COMANDOS_H
#define COMANDOS_H
#include <stdint.h>

typedef enum { COMANDO_OK, COMANDO_LIMPAR, COMANDO_EXIT } comando_resultado_t;

comando_resultado_t comandos_executar(const char *linha, int len, uint32_t *fb,
                                      uint32_t pitch, uint32_t width,
                                      int *cursor_y);

#endif