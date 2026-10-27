#include "comandos.h"
#include "../strutil.h"
#include "render.h"
#include "version.h"
#include "gdt.h"
#include "include/pmm.h"

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

static void mostrar_pmm(uint32_t *fb, uint32_t pitch, int *cursor_y) {
  pmm_stats_t stats;
  pmm_get_stats(&stats);

  *cursor_y += 24;
  desenhar_string("--- PMM: Alocador de Memoria Fisica (4KB Bitmap) ---", 50, *cursor_y,
                  0x00FFD166, fb, pitch);

  char buf[96];
  int p = 0;
  copiar_str(buf, "Total: ", &p);
  p += escrever_uint(buf + p, stats.total_memory_kb / 1024);
  copiar_str(buf, " MB (", &p);
  p += escrever_uint(buf + p, stats.total_blocks);
  copiar_str(buf, " blocos de 4KB)", &p);
  buf[p] = '\0';
  *cursor_y += 24;
  desenhar_string(buf, 50, *cursor_y, 0x00FFFFFF, fb, pitch);

  p = 0;
  copiar_str(buf, "Usados: ", &p);
  p += escrever_uint(buf + p, stats.used_blocks);
  copiar_str(buf, " | Livres: ", &p);
  p += escrever_uint(buf + p, stats.free_blocks);
  copiar_str(buf, " (", &p);
  p += escrever_uint(buf + p, stats.free_memory_kb / 1024);
  copiar_str(buf, " MB livres)", &p);
  buf[p] = '\0';
  *cursor_y += 24;
  desenhar_string(buf, 50, *cursor_y, 0x00AAAAAA, fb, pitch);

  // Teste dinamico de alocacao e liberacao de bloco fisico
  void *bloco = pmm_alloc_block();
  p = 0;
  if (bloco) {
    copiar_str(buf, "Teste alocacao: bloco alocado em 0x", &p);
    escrever_hex(buf + p, (uint32_t)(uintptr_t)bloco);
    p += 8;
    copiar_str(buf, " [OK]", &p);
    buf[p] = '\0';
    *cursor_y += 24;
    desenhar_string(buf, 50, *cursor_y, 0x0000FF00, fb, pitch);

    pmm_free_block(bloco);
    *cursor_y += 24;
    desenhar_string("Teste liberacao: pmm_free_block executado com sucesso [OK]", 50,
                    *cursor_y, 0x0000FF00, fb, pitch);
  } else {
    *cursor_y += 24;
    desenhar_string("Teste alocacao: FALHA - memoria fisica esgotada", 50,
                    *cursor_y, 0x00FF0000, fb, pitch);
  }
}

static void mostrar_gdt(uint32_t *fb, uint32_t pitch, int *cursor_y) {
  gdt_estado_t estado;
  gdt_obter_estado(&estado);
  *cursor_y += 24;
  desenhar_string(estado.valido ? "GDT/TSS OK - 6 entradas, limite 47" :
                                  "GDT/TSS FALHA - conferir registradores",
                  50, *cursor_y, estado.valido ? 0x0000FF00 : 0x00FF0000, fb, pitch);
  char registradores[] = "CS=00000000 DS=00000000 TR=00000000";
  escrever_hex(registradores + 3, estado.cs);
  escrever_hex(registradores + 15, estado.ds);
  escrever_hex(registradores + 27, estado.tr);
  *cursor_y += 24;
  desenhar_string(registradores, 50, *cursor_y, 0x00FFFFFF, fb, pitch);
  char pilha[] = "SS0=00000000 ESP0=00000000";
  escrever_hex(pilha + 4, estado.ss0);
  escrever_hex(pilha + 18, estado.esp0);
  *cursor_y += 24;
  desenhar_string(pilha, 50, *cursor_y, 0x00FFFFFF, fb, pitch);
  char acesso[] = "TSS access=00000000 IOMAP=00000000";
  escrever_hex(acesso + 11, estado.tss_access);
  escrever_hex(acesso + 26, estado.iomap_base);
  *cursor_y += 24;
  desenhar_string(acesso, 50, *cursor_y, 0x00FFFFFF, fb, pitch);
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
    desenhar_string("gdt     - verifica a GDT e o TSS", 50, *cursor_y,
                    0x00AAAAAA, fb, pitch);
    *cursor_y += 24;
    desenhar_string("pmm     - diagnostico do alocador de memoria fisica (PMM)", 50, *cursor_y,
                    0x00AAAAAA, fb, pitch);
    *cursor_y += 24;
    desenhar_string("panic   - teste fatal; exige reinicio", 50, *cursor_y,
                    0x00AAAAAA, fb, pitch);
    *cursor_y += 24;
    desenhar_string("clear   - limpa o terminal (tambem Ctrl+L)", 50, *cursor_y,
                    0x00AAAAAA, fb, pitch);
    *cursor_y += 24;
    desenhar_string("exit    - sai do sistema / desliga", 50, *cursor_y,
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

  if (comando_igual(linha, len, "gdt")) {
    mostrar_gdt(fb, pitch, cursor_y);
    return COMANDO_OK;
  }

  if (comando_igual(linha, len, "pmm") || comando_igual(linha, len, "mem")) {
    mostrar_pmm(fb, pitch, cursor_y);
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
