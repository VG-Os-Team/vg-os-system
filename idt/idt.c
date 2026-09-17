/* idt/idt.c */
#include "idt.h"

/* Tabela com as 256 entradas de interrupção */
idt_entry_t idt[256];

/* Ponteiro que será carregado na CPU */
idt_ptr_t idtp;

/* Função para registrar seletores/atributos de portas de interrupção */
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

/* Instala a IDT */
void idt_install() {
    /* Configura o limite e a base do ponteiro da IDT */
    idtp.limit = (sizeof(idt_entry_t) * 256) - 1;
    idtp.base = (uint32_t)&idt;

    /* Chama a instrução Assembly que avisa o processador onde está a IDT */
    idt_flush((uint32_t)&idtp);
}