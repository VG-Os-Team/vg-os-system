#include "gdt.h"
#include <stdint.h>

#define GDT_NUM_ENTRADAS 5

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

  // Descritor nulo (seletor 0x00). A arquitetura exige que o primeiro
  // descritor seja todo zero; carregar o seletor 0 gera #GP de propósito.
  gdt_definir_entrada(0, 0, 0, 0, 0);

  // Granularidade 0xCF em todos os segmentos abaixo:
  //   bit 7 (G)   = 1 -> limite contado em páginas de 4KB
  //   bit 6 (D/B) = 1 -> segmento de 32 bits
  //   bit 5 (L)   = 0 -> não é código 64 bits
  //   bit 4 (AVL) = 0 -> livre para o SO, não usamos
  //   bits 3-0        -> limite bits 16-19 (0xF)
  // Com G=1 e limite 0xFFFFF, o segmento cobre 0xFFFFF * 4KB = 4GB.

  // Código Kernel (seletor 0x08) — access 0x9A = 1001 1010
  //   bit 7 (P)   = 1  -> segmento presente na memória
  //   bits 6-5 (DPL) = 00 -> ring 0, privilégio de kernel
  //   bit 4 (S)   = 1  -> descritor de código/dados (não é de sistema)
  //   bit 3 (Ex)  = 1  -> executável, é um segmento de código
  //   bit 2 (DC)  = 0  -> não-conforming: só ring 0 pode executar
  //   bit 1 (RW)  = 1  -> leitura permitida (código nunca é gravável)
  //   bit 0 (A)   = 0  -> accessed, quem liga é a CPU
  gdt_definir_entrada(1, 0, 0xFFFFFFFF, 0x9A, 0xCF);

  // Dados Kernel (seletor 0x10) — access 0x92 = 1001 0010
  //   bit 7 (P)   = 1  -> segmento presente
  //   bits 6-5 (DPL) = 00 -> ring 0
  //   bit 4 (S)   = 1  -> código/dados
  //   bit 3 (Ex)  = 0  -> não executável, é segmento de dados
  //   bit 2 (DC)  = 0  -> expand-up, cresce para endereços maiores
  //   bit 1 (RW)  = 1  -> escrita permitida
  //   bit 0 (A)   = 0  -> accessed
  gdt_definir_entrada(2, 0, 0xFFFFFFFF, 0x92, 0xCF);

  // Código User (seletor 0x18) — access 0xFA = 1111 1010
  //   Igual ao código kernel, mudando só o DPL:
  //   bits 6-5 (DPL) = 11 -> ring 3, código de usuário
  gdt_definir_entrada(3, 0, 0xFFFFFFFF, 0xFA, 0xCF);

  // Dados User (seletor 0x20) — access 0xF2 = 1111 0010
  //   Igual aos dados kernel, mudando só o DPL:
  //   bits 6-5 (DPL) = 11 -> ring 3
  gdt_definir_entrada(4, 0, 0xFFFFFFFF, 0xF2, 0xCF);

  gdt_flush((uint32_t)(uintptr_t)&gdt_ptr);
}
