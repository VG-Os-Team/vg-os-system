#ifndef GDT_H
#define GDT_H
#include <stdint.h>

#define SELETOR_CODIGO_KERNEL 0x08
#define SELETOR_DADOS_KERNEL 0x10
#define SELETOR_CODIGO_USER 0x1B // entrada 3 (0x18) + RPL 3
#define SELETOR_DADOS_USER 0x23  // entrada 4 (0x20) + RPL 3
#define SELETOR_TSS 0x28
#define GDT_NUM_ENTRADAS 6

// Um descritor de segmento da GDT (8 bytes, layout fixo pela arquitetura).
// Base e limite ficam picotados em campos não contíguos por herança do 286.
typedef struct {
  uint16_t limit_low;   // limite, bits 0-15
  uint16_t base_low;    // base, bits 0-15
  uint8_t base_middle;  // base, bits 16-23
  uint8_t access;       // presente / anel / tipo do segmento
  uint8_t granularity;  // flags (G, D/B) nos bits 4-7 + limite bits 16-19
  uint8_t base_high;    // base, bits 24-31
} __attribute__((packed)) gdt_entry_t;

// Operando do LGDT: tamanho da tabela - 1, seguido do endereço linear dela
typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

// Monta os 6 descritores e carrega GDT e TSS. Chamar no boot com IRQs desativadas.
void gdt_instalar(void);

typedef struct {
  gdt_ptr_t gdtr;
  uint16_t cs, ds, ss, tr;
  uint16_t ss0, iomap_base;
  uint32_t esp0;
  uint8_t tss_access;
  int valido;
} gdt_estado_t;

// Le os registradores reais da CPU para o comando de demonstracao.
void gdt_obter_estado(gdt_estado_t *estado);

// Implementada em gdt_asm.s: executa o LGDT e recarrega todos os
// registradores de segmento (incluindo CS, via far jump)
extern void gdt_flush(uint32_t gdt_ptr_endereco);
extern void tss_flush(uint32_t seletor);

#endif
