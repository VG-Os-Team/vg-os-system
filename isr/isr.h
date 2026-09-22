#ifndef ISR_H
#define ISR_H

#include <stdint.h>

// Estado dos registradores da CPU no momento da excecao.
//
// A ordem dos campos aqui e FIXADA pelo stub em Assembly (interrupts.s).
// Se alterar a ordem dos "push" la, atualize esta struct tambem (e
// vice-versa), ou o handler em C vai ler os valores errados.
//
// Obs: como o VGOS ainda roda inteiramente em ring 0 (sem user mode), a CPU
// nao empilha ESP/SS extras nas excecoes, entao esses campos nao aparecem
// aqui. Quando houver suporte a ring 3, sera preciso revisar isso.
typedef struct {
    uint32_t ds;                                     // segmento de dados salvo
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax; // pushados por pusha
    uint32_t int_no, err_code;                        // numero da excecao e codigo de erro
    uint32_t eip, cs, eflags;                          // empilhados automaticamente pela CPU
} __attribute__((packed)) registers_t;

// Handler generico em C, chamado por todos os stubs de excecao (0 a 31).
// Implementado em isr.c.
void isr_handler(registers_t regs);

// Registra as 32 excecoes da CPU na IDT. Deve ser chamada apos idt_install(),
// antes de habilitar interrupcoes (sti).
void isr_install(void);

// Stubs em Assembly, um por excecao da CPU (definidos em interrupts.s).
extern void isr0(void);
extern void isr1(void);
extern void isr2(void);
extern void isr3(void);
extern void isr4(void);
extern void isr5(void);
extern void isr6(void);
extern void isr7(void);
extern void isr8(void);
extern void isr9(void);
extern void isr10(void);
extern void isr11(void);
extern void isr12(void);
extern void isr13(void);
extern void isr14(void);
extern void isr15(void);
extern void isr16(void);
extern void isr17(void);
extern void isr18(void);
extern void isr19(void);
extern void isr20(void);
extern void isr21(void);
extern void isr22(void);
extern void isr23(void);
extern void isr24(void);
extern void isr25(void);
extern void isr26(void);
extern void isr27(void);
extern void isr28(void);
extern void isr29(void);
extern void isr30(void);
extern void isr31(void);

#endif
