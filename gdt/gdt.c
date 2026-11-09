#include "gdt.h"
#include "tss.h"
#include <stdint.h>

_Static_assert(sizeof(gdt_entry_t) == 8, "Descritor deve ter 8 bytes");
_Static_assert(sizeof(gdt_ptr_t) == 6, "GDTR deve ter 6 bytes");

static gdt_entry_t gdt[GDT_NUM_ENTRADAS];
static gdt_ptr_t gdt_ptr;

// Espalha base e limite pelos campos picotados do descritor
static void gdt_definir_entrada(int num, uint32_t base, uint32_t limite,
                                uint8_t access, uint8_t granularity) {
  gdt[num].base_low = (uint16_t)(base & 0xFFFF);
  gdt[num].base_middle = (uint8_t)((base >> 16) & 0xFF);
  gdt[num].base_high = (uint8_t)((base >> 24) & 0xFF);

  gdt[num].limit_low = (uint16_t)(limite & 0xFFFF);
  // Os 4 bits altos do limite moram na parte baixa do byte de granularidade
  gdt[num].granularity = (uint8_t)(((limite >> 16) & 0x0F) | (granularity & 0xF0));

  gdt[num].access = access;
}

void gdt_instalar(void) {
  gdt_ptr.limit = (uint16_t)(sizeof(gdt) - 1);
  gdt_ptr.base = (uint32_t)(uintptr_t)&gdt;

  // Entrada zero nao e usada pela CPU; mantemos seus bytes zerados.
  // Seletor nulo em DS/ES/FS/GS e permitido, mas nao para acessar memoria.
  // SS e CS nao podem usar seletor nulo.
  gdt_definir_entrada(0, 0, 0, 0, 0);

  // Granularidade 0xCF em todos os segmentos abaixo:
  //   bit 7 (G)   = 1 -> limite contado em páginas de 4KB
  //   bit 6 (D/B) = 1 -> segmento de 32 bits
  //   bit 5 (L)   = 0 -> não é código 64 bits
  //   bit 4 (AVL) = 0 -> livre para o SO, não usamos
  //   bits 3-0        -> limite bits 16-19 (0xF)
  // Limite efetivo = (0xFFFFF << 12) | 0xFFF = 0xFFFFFFFF: 4 GiB.

  // Código Kernel (seletor 0x08) — access 0x9A = 1001 1010
  //   bit 7 (P)   = 1  -> segmento presente na memória
  //   bits 6-5 (DPL) = 00 -> ring 0, privilégio de kernel
  //   bit 4 (S)   = 1  -> descritor de código/dados (não é de sistema)
  //   bit 3 (Ex)  = 1  -> executável, é um segmento de código
  //   bit 2 (DC)  = 0  -> não-conforming: só ring 0 pode executar
  //   bit 1 (RW)  = 1  -> leitura permitida (código nunca é gravável)
  //   bit 0 (A)   = 0  -> accessed, quem liga é a CPU
  gdt_definir_entrada(1, 0, 0xFFFFF, 0x9A, 0xCF);

  // Dados Kernel (seletor 0x10) — access 0x92 = 1001 0010
  //   bit 7 (P)   = 1  -> segmento presente
  //   bits 6-5 (DPL) = 00 -> ring 0
  //   bit 4 (S)   = 1  -> código/dados
  //   bit 3 (Ex)  = 0  -> não executável, é segmento de dados
  //   bit 2 (DC)  = 0  -> expand-up, cresce para endereços maiores
  //   bit 1 (RW)  = 1  -> escrita permitida
  //   bit 0 (A)   = 0  -> accessed
  gdt_definir_entrada(2, 0, 0xFFFFF, 0x92, 0xCF);

  // Codigo User (offset 0x18; seletor com RPL 3 = 0x1B).
  //   Igual ao código kernel, mudando só o DPL:
  //   bits 6-5 (DPL) = 11 -> ring 3, código de usuário
  gdt_definir_entrada(3, 0, 0xFFFFF, 0xFA, 0xCF);

  // Dados User (offset 0x20; seletor com RPL 3 = 0x23).
  //   Igual aos dados kernel, mudando só o DPL:
  //   bits 6-5 (DPL) = 11 -> ring 3
  gdt_definir_entrada(4, 0, 0xFFFFF, 0xF2, 0xCF);

  const tss_t *tss = tss_preparar();
  // 0x89: presente, DPL 0, segmento de sistema, TSS 32 bits disponivel.
  // G=0: limite em bytes; D/B e L devem ficar em zero para este descritor.
  gdt_definir_entrada(5, (uint32_t)(uintptr_t)tss, sizeof(*tss) - 1, 0x89, 0);

  gdt_flush((uint32_t)(uintptr_t)&gdt_ptr);
  tss_flush(SELETOR_TSS); // LTR marca o descritor como busy (0x8B).
}

void gdt_obter_estado(gdt_estado_t *estado) {
  const tss_t *tss = tss_obter();
  asm volatile("sgdt %0" : "=m"(estado->gdtr));
  asm volatile("str %0" : "=r"(estado->tr));
  asm volatile("mov %%cs, %0" : "=r"(estado->cs));
  asm volatile("mov %%ds, %0" : "=r"(estado->ds));
  asm volatile("mov %%ss, %0" : "=r"(estado->ss));
  estado->ss0 = tss->ss0;
  estado->esp0 = tss->esp0;
  estado->iomap_base = tss->iomap_base;
  estado->tss_access = gdt[5].access;
  uint32_t base_tss = gdt[5].base_low | ((uint32_t)gdt[5].base_middle << 16)
                      | ((uint32_t)gdt[5].base_high << 24);
  estado->valido = estado->gdtr.base == (uint32_t)(uintptr_t)gdt
      && estado->gdtr.limit == sizeof(gdt) - 1
      && estado->cs == SELETOR_CODIGO_KERNEL
      && estado->ds == SELETOR_DADOS_KERNEL
      && estado->ss == SELETOR_DADOS_KERNEL
      && estado->tr == SELETOR_TSS && estado->tss_access == 0x8B
      && base_tss == (uint32_t)(uintptr_t)tss
      && gdt[5].limit_low == sizeof(*tss) - 1 && gdt[5].granularity == 0
      && estado->ss0 == SELETOR_DADOS_KERNEL && estado->esp0 != 0
      && estado->iomap_base == sizeof(*tss);
}
