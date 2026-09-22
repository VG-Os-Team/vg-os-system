#ifndef GDT_H
#define GDT_H
#include <stdint.h>

#define SELETOR_CODIGO_KERNEL 0x08
#define SELETOR_DADOS_KERNEL 0x10
#define SELETOR_CODIGO_USER 0x18
#define SELETOR_DADOS_USER 0x20

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

// Monta os 5 descritores e carrega a nova GDT no processador
void gdt_instalar(void);

// Implementada em gdt_asm.s: executa o LGDT e recarrega todos os
// registradores de segmento (incluindo CS, via far jump)
extern void gdt_flush(uint32_t gdt_ptr_endereco);

#endif
