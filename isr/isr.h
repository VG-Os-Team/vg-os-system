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

// Faixas de vetores da IDT atendidas por este modulo.
#define ISR_NUM_EXCECOES  32                                // vetores 0-31: excecoes da CPU
#define IRQ_VETOR_BASE    32                                // IRQ 0 -> vetor 32 (remapeamento do PIC)
#define IRQ_NUM_LINHAS    16                                // IRQ 0-15: PIC mestre + escravo
#define INT_NUM_VETORES   (IRQ_VETOR_BASE + IRQ_NUM_LINHAS) // tamanho da tabela de handlers

// Assinatura dos handlers registrados na tabela de despacho.
//
// `regs` aponta para o quadro salvo pelo stub em Assembly, o mesmo que o
// popa/iret do stub restaura no fim. Como isr_handler/irq_handler recebem o
// registers_t por valor (contrato do stub), esse endereco e o do proprio quadro
// empilhado, entao alteracoes em *regs (ex.: eax como valor de retorno, eip
// para pular uma instrucao) valem na retomada. O ponteiro so e valido durante
// a chamada: nao guarde.
//
// O handler roda com interrupcoes mascaraveis desabilitadas (gate de
// interrupcao, flags 0x8E, e "cli" no stub). Mantenha-o curto e nao bloqueante;
// habilitar IF aqui permite reentrada, e isso fica por conta de quem o fizer.
typedef void (*interrupt_handler_t)(registers_t *regs);

// Registra `handler` para a excecao da CPU `vetor` (0-31).
// Retorna 0 em sucesso, ou -1 se `vetor` for invalido, `handler` for NULL ou
// ja existir um handler nesse vetor (use isr_unregister_handler antes para trocar).
int isr_register_handler(uint8_t vetor, interrupt_handler_t handler);

// Remove o handler da excecao `vetor`; volta ao comportamento padrao (kernel
// panic). Retorna 0 (inclusive se nao havia handler) ou -1 se `vetor` for invalido.
int isr_unregister_handler(uint8_t vetor);

// Registra `handler` para a linha de IRQ `irq` (0-15, nao o vetor). O vetor
// usado na tabela e IRQ_VETOR_BASE + irq. Mesmos codigos de retorno de
// isr_register_handler. Registrar NAO desmascara a linha no PIC: isso continua
// sendo responsabilidade do driver.
int irq_register_handler(uint8_t irq, interrupt_handler_t handler);

// Remove o handler da linha `irq` (0-15). A IRQ passa a ser apenas confirmada
// no PIC e ignorada. Retorna 0 (inclusive se nao havia handler) ou -1 se `irq`
// for invalida.
int irq_unregister_handler(uint8_t irq);

// Despachante das excecoes da CPU (vetores 0-31), chamado por isr_common_stub.
// Procura o handler em regs.int_no e o invoca com o quadro salvo.
//  - Sem handler registrado: kernel panic (comportamento padrao).
//  - Handler que retorna: a execucao e retomada em regs.eip via iret. Para
//    escalar a falha, o proprio handler pode chamar kernel_panic().
//  - Abortos (#DF = 8, #MC = 18) nao sao retomaveis: o handler roda (util para
//    diagnostico), mas ao retornar o kernel entra em panic de qualquer forma.
void isr_handler(registers_t regs);

// Despachante das IRQs 0-15 (vetores 32-47), chamado por irq_common_stub.
// Ordem das operacoes:
//  1. IRQ espuria (7/15 sem bit no ISR): descarta sem handler e sem EOI.
//  2. EOI no PIC, ANTES do handler. As interrupcoes seguem mascaradas na CPU,
//     entao nao ha reentrada, e o PIC nao fica travado se o handler nunca
//     retornar por aqui (ex.: troca de contexto no timer).
//  3. Invoca o handler registrado. Sem handler, a IRQ so e confirmada e ignorada.
void irq_handler(registers_t regs);

// Registra as 16 IRQs (vetores 32-47) na IDT. Chamar apos idt_install() e
// pic_init(); as IRQs continuam mascaradas no PIC ate cada driver liberar a sua.
void irq_install(void);

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


// Stubs em Assembly, um por IRQ de hardware (definidos em interrupts.s).
extern void irq0(void);
extern void irq1(void);
extern void irq2(void);
extern void irq3(void);
extern void irq4(void);
extern void irq5(void);
extern void irq6(void);
extern void irq7(void);
extern void irq8(void);
extern void irq9(void);
extern void irq10(void);
extern void irq11(void);
extern void irq12(void);
extern void irq13(void);
extern void irq14(void);
extern void irq15(void);

#endif
