/* idt/idt.h */
#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* Estrutura de uma entrada da IDT (Gate) */
struct idt_entry_struct {
    uint16_t base_low;    /* 16 bits inferiores do endereço da rotina */
    uint16_t sel;         /* Seletor de segmento de código */
    uint8_t  always0;     /* Este byte deve ser sempre zero */
    uint8_t  flags;       /* Flags (Presente, Privilégio, Tipo) */
    uint16_t base_high;   /* 16 bits superiores do endereço da rotina */
} __attribute__((packed));

typedef struct idt_entry_struct idt_entry_t;

/* Estrutura do ponteiro da IDT passado para a instrução lidt */
struct idt_ptr_struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

typedef struct idt_ptr_struct idt_ptr_t;

/* Protótipos das funções em C */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void idt_install();

/* Protótipo da rotina em Assembly */
extern void idt_flush(uint32_t idt_ptr_addr);

#endif