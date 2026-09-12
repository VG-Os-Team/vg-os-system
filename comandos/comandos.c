#include "comandos.h"
#include "../strutil.h"
#include "render.h"
#include "version.h"

static int comando_igual(const char *linha, int len, const char *cmd) {
  int cmd_len = minha_strlen(cmd);
  if (len != cmd_len)
    return 0;
  for (int i = 0; i < len; i++)
    if (linha[i] != cmd[i])
      return 0;
  return 1;
}

comando_resultado_t comandos_executar(const char *linha, int len, uint32_t *fb,
                                      uint32_t pitch, uint32_t width,
                                      int *cursor_y) {
  (void)width;
  if (len == 0)
    return COMANDO_OK; // Enter sem digitar nada

  if (comando_igual(linha, len, "help") ||
      comando_igual(linha, len, "helpet")) {
    *cursor_y += 24;
    desenhar_string("------Comandos disponiveis:------", 50, *cursor_y, 0x00FFFFFF, fb,
                    pitch);
    *cursor_y += 24;
    desenhar_string("help    - mostra esta lista", 50, *cursor_y, 0x00AAAAAA,
                    fb, pitch);
    *cursor_y += 24;
    desenhar_string("version - mostra a versao do sistema", 50, *cursor_y,
                    0x00AAAAAA, fb, pitch);
    *cursor_y += 24;
    desenhar_string("clear   - limpa o terminal (tambem Ctrl+L)", 50, *cursor_y,
                    0x00AAAAAA, fb, pitch);
    *cursor_y += 24;
    desenhar_string("Ctrl+C  - copia a linha atual", 50, *cursor_y, 0x00AAAAAA,
                    fb, pitch);
    *cursor_y += 24;
    desenhar_string("Ctrl+V  - cola o que foi copiado", 50, *cursor_y,
                    0x00AAAAAA, fb, pitch);
    return COMANDO_OK;
  }

  if (comando_igual(linha, len, "version")) {
    *cursor_y += 24;
    desenhar_string(OS_BANNER, 50, *cursor_y, 0x00FFFFFF, fb, pitch);
    return COMANDO_OK;
  }

  if (comando_igual(linha, len, "clear")) {
    return COMANDO_LIMPAR;
  }

  // Entrada nao reconhecida: nao mostra erro, kernel avanca o prompt
  // normalmente
  return COMANDO_OK;
}