#include "comandos.h"
#include "../strutil.h"
#include "layout.h"
#include "version.h"
#include "gdt.h"
#include "include/pmm.h"
#include "gfx.h"

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
