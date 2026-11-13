#include "comandos.h"
#include "../strutil.h"
#include "layout.h"
#include "version.h"
#include "gdt.h"
#include "include/heap.h"
#include "include/pmm.h"
#include "include/video.h"
#include "gfx.h"
#include "images/logo/logo.h"
#include "render.h"

// Escreve exatamente 8 digitos hexadecimais (sem depender de printf/libc).
static void escrever_hex(char *destino, uint32_t valor) {
  const char *digitos = "0123456789ABCDEF";
  for (int i = 0; i < 8; i++)
    destino[i] = digitos[(valor >> (28 - i * 4)) & 0xF];
}

static int escrever_uint(char *destino, uint32_t valor) {
  char temp[12];
  int i = 0;
  if (valor == 0) {
    destino[0] = '0';
    destino[1] = '\0';
    return 1;
  }
  while (valor > 0) {
    temp[i++] = '0' + (valor % 10);
    valor /= 10;
  }
  for (int j = 0; j < i; j++) {
    destino[j] = temp[i - 1 - j];
  }
  destino[i] = '\0';
  return i;
}

static void copiar_str(char *dest, const char *src, int *pos) {
  while (*src) {
    dest[(*pos)++] = *src++;
  }
}

static void escrever_info_uint(char *destino, const char *prefixo,
                               uint32_t valor, const char *sufixo) {
  int pos = 0;
  copiar_str(destino, prefixo, &pos);
  pos += escrever_uint(destino + pos, valor);
  copiar_str(destino, sufixo, &pos);
  destino[pos] = '\0';
}

static int preparar_info_logo(layout_bounds_t area, uint32_t linhas,
                              int *cursor_y) {
  int ultima_linha = area.y + area.height - LAYOUT_CELL_H;
  int inicio_maximo = ultima_linha - (int)(linhas - 1U) * LAYOUT_CELL_H;
  layout_next_line(cursor_y);
  int inicio = *cursor_y;

  if (inicio > inicio_maximo) {
    while (*cursor_y < ultima_linha)
      layout_next_line(cursor_y);
    uint32_t linhas_rolar = (uint32_t)(inicio - inicio_maximo) / LAYOUT_CELL_H;
    for (uint32_t i = 0; i < linhas_rolar; i++)
      layout_next_line(cursor_y);
    inicio = inicio_maximo;
  }
  return inicio;
}

static void mostrar_info(int *cursor_y) {
  char linhas[7][64];
  pmm_stats_t memoria;
  pmm_get_stats(&memoria);

  int pos = 0;
  copiar_str(linhas[0], OS_NAME, &pos);
  linhas[0][pos] = '\0';

  pos = 0;
  copiar_str(linhas[1], "Versao: ", &pos);
  copiar_str(linhas[1], OS_VERSION "-" OS_STAGE, &pos);
  linhas[1][pos] = '\0';

  pos = 0;
  copiar_str(linhas[2], "Kernel: x86 (32-bit)", &pos);
  linhas[2][pos] = '\0';

  pos = 0;
  copiar_str(linhas[3], "Tela: ", &pos);
  pos += escrever_uint(linhas[3] + pos, video_get_width());
  copiar_str(linhas[3], "x", &pos);
  pos += escrever_uint(linhas[3] + pos, video_get_height());
  copiar_str(linhas[3], " @ ", &pos);
  pos += escrever_uint(linhas[3] + pos, video_get_bpp());
  copiar_str(linhas[3], " bpp", &pos);
  linhas[3][pos] = '\0';

  pos = 0;
  copiar_str(linhas[4], "RAM: ", &pos);
  pos += escrever_uint(linhas[4] + pos, memoria.used_memory_kb / 1024U);
  copiar_str(linhas[4], " / ", &pos);
  pos += escrever_uint(linhas[4] + pos, memoria.total_memory_kb / 1024U);
  copiar_str(linhas[4], " MB", &pos);
  linhas[4][pos] = '\0';

  escrever_info_uint(linhas[5], "RAM livre: ",
                     memoria.free_memory_kb / 1024U, " MB");
  escrever_info_uint(linhas[6], "Heap livre: ",
                     (uint32_t)(heap_free_bytes() / 1024U), " KB");

  layout_bounds_t area = layout_get_terminal_bounds();
  uint32_t logo_linhas = (logo_height + LAYOUT_CELL_H - 1U) / LAYOUT_CELL_H;
  uint32_t max_info = 0;
  for (uint32_t i = 0; i < sizeof(linhas) / sizeof(linhas[0]); i++) {
    uint32_t tamanho = (uint32_t)minha_strlen(linhas[i]);
    if (tamanho > max_info)
      max_info = tamanho;
  }

  uint32_t linhas_info = sizeof(linhas) / sizeof(linhas[0]);
  uint32_t total_linhas = logo_linhas + linhas_info;
  uint32_t linhas_tela = (uint32_t)area.height / LAYOUT_CELL_H;
  if ((uint32_t)area.width < logo_width || linhas_tela < total_linhas) {
    for (uint32_t i = 0; i < linhas_info; i++)
      layout_write_line(linhas[i], LAYOUT_TEXT, cursor_y);
    return;
  }

  uint32_t largura_info = logo_width + 2U * LAYOUT_CELL_W + max_info * LAYOUT_CELL_W;
  int ao_lado = largura_info <= (uint32_t)area.width;
  uint32_t linhas_reservadas = ao_lado ? logo_linhas : total_linhas;
  int inicio = preparar_info_logo(area, linhas_reservadas, cursor_y);
  uint32_t altura_reservada = linhas_reservadas * LAYOUT_CELL_H;
  video_fill_rect((uint32_t)area.x, (uint32_t)inicio,
                  (uint32_t)area.width, altura_reservada, LAYOUT_BG);
  desenhar_logo(video_get_draw_buffer(), video_get_draw_pitch(),
                area.x, inicio);

  if (ao_lado) {
    uint32_t x_info = (uint32_t)(area.x + logo_width + 2U * LAYOUT_CELL_W);
    for (uint32_t i = 0; i < linhas_info; i++)
      gfx_draw_string(linhas[i], x_info,
                      (uint32_t)(inicio + i * LAYOUT_CELL_H), LAYOUT_TEXT);
    *cursor_y = inicio + (int)(logo_linhas - 1U) * LAYOUT_CELL_H;
    return;
  }

  *cursor_y = inicio + (int)(logo_linhas - 1U) * LAYOUT_CELL_H;
  for (uint32_t i = 0; i < linhas_info; i++)
    layout_write_line(linhas[i], LAYOUT_TEXT, cursor_y);
}

static void mostrar_pmm(int *cursor_y) {
  pmm_stats_t stats;
  pmm_get_stats(&stats);

  layout_write_line("--- PMM: Alocador de Memoria Fisica (4KB Bitmap) ---", LAYOUT_TEXT, cursor_y);

  char buf[96];
  int p = 0;
  copiar_str(buf, "Total: ", &p);
  p += escrever_uint(buf + p, stats.total_memory_kb / 1024);
  copiar_str(buf, " MB (", &p);
  p += escrever_uint(buf + p, stats.total_blocks);
  copiar_str(buf, " blocos de 4KB)", &p);
  buf[p] = '\0';
  layout_write_line(buf, LAYOUT_TEXT, cursor_y);

  p = 0;
  copiar_str(buf, "Usados: ", &p);
  p += escrever_uint(buf + p, stats.used_blocks);
  copiar_str(buf, " | Livres: ", &p);
  p += escrever_uint(buf + p, stats.free_blocks);
  copiar_str(buf, " (", &p);
  p += escrever_uint(buf + p, stats.free_memory_kb / 1024);
  copiar_str(buf, " MB livres)", &p);
  buf[p] = '\0';
  layout_write_line(buf, LAYOUT_MUTED, cursor_y);

  // Teste dinamico de alocacao e liberacao de bloco fisico
  void *bloco = pmm_alloc_block();
  p = 0;
  if (bloco) {
    copiar_str(buf, "Teste alocacao: bloco alocado em 0x", &p);
    escrever_hex(buf + p, (uint32_t)(uintptr_t)bloco);
    p += 8;
    copiar_str(buf, " [OK]", &p);
    buf[p] = '\0';
    layout_write_line(buf, LAYOUT_ACCENT, cursor_y);

    pmm_free_block(bloco);
    layout_write_line("Teste liberacao: pmm_free_block executado com sucesso [OK]", LAYOUT_ACCENT, cursor_y);
  } else {
    layout_write_line("Teste alocacao: FALHA - memoria fisica esgotada", 0x00FF0000, cursor_y);
  }
}

static void mostrar_gdt(int *cursor_y) {
  gdt_estado_t estado;
  gdt_obter_estado(&estado);
  layout_write_line(estado.valido ? "GDT/TSS OK - 6 entradas, limite 47" :
                                  "GDT/TSS FALHA - conferir registradores", estado.valido ? LAYOUT_ACCENT : 0x00FF0000, cursor_y);
  char registradores[] = "CS=00000000 DS=00000000 TR=00000000";
  escrever_hex(registradores + 3, estado.cs);
  escrever_hex(registradores + 15, estado.ds);
  escrever_hex(registradores + 27, estado.tr);
  layout_write_line(registradores, LAYOUT_TEXT, cursor_y);
  char pilha[] = "SS0=00000000 ESP0=00000000";
  escrever_hex(pilha + 4, estado.ss0);
  escrever_hex(pilha + 18, estado.esp0);
  layout_write_line(pilha, LAYOUT_TEXT, cursor_y);
  char acesso[] = "TSS access=00000000 IOMAP=00000000";
  escrever_hex(acesso + 11, estado.tss_access);
  escrever_hex(acesso + 26, estado.iomap_base);
  layout_write_line(acesso, LAYOUT_TEXT, cursor_y);
}

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
  (void)fb;
  (void)pitch;
  (void)width;
  if (len == 0)
    return COMANDO_OK; // Enter sem digitar nada

  if (comando_igual(linha, len, "help") ||
      comando_igual(linha, len, "helpet")) {
    layout_write_line("------Comandos disponiveis:------", LAYOUT_TEXT, cursor_y);
    layout_write_line("help    - mostra esta lista", LAYOUT_MUTED, cursor_y);
    layout_write_line("info    - mostra logo e informacoes do sistema", LAYOUT_MUTED, cursor_y);
    layout_write_line("version - mostra a versao do sistema", LAYOUT_MUTED, cursor_y);
    layout_write_line("gdt     - verifica a GDT e o TSS", LAYOUT_MUTED, cursor_y);
    layout_write_line("pmm     - diagnostico do alocador de memoria fisica (PMM)", LAYOUT_MUTED, cursor_y);
    layout_write_line("panic   - teste fatal; exige reinicio", LAYOUT_MUTED, cursor_y);
    layout_write_line("clear   - limpa o terminal (tambem Ctrl+L)", LAYOUT_MUTED, cursor_y);
    layout_write_line("exit    - sai do sistema / desliga", LAYOUT_MUTED, cursor_y);
    layout_write_line("Ctrl+C  - copia a linha atual", LAYOUT_MUTED, cursor_y);
    layout_write_line("Ctrl+V  - cola o que foi copiado", LAYOUT_MUTED, cursor_y);
    return COMANDO_OK;
  }

  /* Teste das primitivas graficas - Tarefa 17 */
  if (comando_igual(linha, len, "gfx")) {
    layout_bounds_t area = layout_get_terminal_bounds();
    layout_next_line(cursor_y);
    gfx_draw_rect(area.x, *cursor_y, 3 * LAYOUT_CELL_W, LAYOUT_CELL_H, LAYOUT_MUTED);
    gfx_draw_line(area.x, *cursor_y, area.x + 47, *cursor_y + 23, LAYOUT_ACCENT);
    gfx_put_pixel(area.x + 47, *cursor_y, LAYOUT_TEXT);
    gfx_draw_char('V', area.x + LAYOUT_CELL_W, *cursor_y, LAYOUT_TEXT);
    layout_write_line("VGOS GRAPHICS OK", LAYOUT_TEXT, cursor_y);
    return COMANDO_OK;
  }
  if (comando_igual(linha, len, "version")) {
    layout_write_line(OS_BANNER, LAYOUT_TEXT, cursor_y);
    return COMANDO_OK;
  }

  if (comando_igual(linha, len, "info")) {
    mostrar_info(cursor_y);
    return COMANDO_OK;
  }

  if (comando_igual(linha, len, "gdt")) {
    mostrar_gdt(cursor_y);
    return COMANDO_OK;
  }

  if (comando_igual(linha, len, "pmm") || comando_igual(linha, len, "mem")) {
    mostrar_pmm(cursor_y);
    return COMANDO_OK;
  }

  if (comando_igual(linha, len, "panic")) {
    // UD2 gera a excecao real #UD (vetor 6), passando por IDT e ISR.
    asm volatile("ud2" ::: "memory");
    return COMANDO_OK; // O handler fatal nunca retorna a este ponto.
  }

  if (comando_igual(linha, len, "clear")) {
    return COMANDO_LIMPAR;
  }

  if (comando_igual(linha, len, "exit") || comando_igual(linha, len, "sair")) {
    return COMANDO_EXIT;
  }

  // Entrada nao reconhecida: nao mostra erro, kernel avanca o prompt
  // normalmente
  return COMANDO_OK;
}
