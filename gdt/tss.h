#ifndef TSS_H
#define TSS_H
#include <stdint.h>

// Layout do TSS x86 de 32 bits (104 bytes). Campos reservados ficam em zero.
typedef struct {
  uint16_t anterior, reservado0;
  uint32_t esp0;
  uint16_t ss0, reservado1;
  uint32_t esp1;
  uint16_t ss1, reservado2;
  uint32_t esp2;
  uint16_t ss2, reservado3;
  uint32_t cr3, eip, eflags;
  uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
  uint16_t es, reservado4;
  uint16_t cs, reservado5;
  uint16_t ss, reservado6;
  uint16_t ds, reservado7;
  uint16_t fs, reservado8;
  uint16_t gs, reservado9;
  uint16_t ldt, reservado10;
  uint16_t trap, iomap_base;
} __attribute__((packed)) tss_t;

const tss_t *tss_preparar(void);
const tss_t *tss_obter(void);

// Futuro escalonador: informar o topo de uma pilha de kernel valida antes
// de retornar ao usuario, com interrupcoes desativadas (versao uniprocessador).
void tss_definir_pilha_kernel(uint32_t topo);
#endif
