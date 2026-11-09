#include "tss.h"
#include "gdt.h"
#include <stddef.h>

_Static_assert(sizeof(tss_t) == 104, "TSS deve ter 104 bytes");
_Static_assert(offsetof(tss_t, esp0) == 4, "Offset incorreto de ESP0");
_Static_assert(offsetof(tss_t, ss0) == 8, "Offset incorreto de SS0");
_Static_assert(offsetof(tss_t, iomap_base) == 102, "Offset incorreto de I/O map");

static tss_t tss __attribute__((aligned(16)));
// Pilha separada da pilha de boot; cresce dos enderecos altos para os baixos.
static uint8_t pilha_kernel[16384] __attribute__((aligned(16)));

const tss_t *tss_preparar(void) {
  uint8_t *bytes = (uint8_t *)&tss;
  for (unsigned int i = 0; i < sizeof(tss); i++)
    bytes[i] = 0;

  tss.ss0 = SELETOR_DADOS_KERNEL;
  tss_definir_pilha_kernel((uint32_t)(uintptr_t)(pilha_kernel + sizeof(pilha_kernel)));
  // Fora do limite (103): nao existe bitmap. Com CPL > IOPL, I/O gera #GP.
  tss.iomap_base = sizeof(tss);
  return &tss;
}

const tss_t *tss_obter(void) {
  return &tss;
}

void tss_definir_pilha_kernel(uint32_t topo) {
  tss.esp0 = topo;
}
