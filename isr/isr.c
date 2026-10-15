#include "isr.h"
#include "panic.h"
#include "idt.h"
#include "gdt.h"
#include "pic/pic.h"
#include <stddef.h>

_Static_assert(sizeof(registers_t) == 56, "Quadro ISR deve ter 56 bytes");
_Static_assert(offsetof(registers_t, esp_dummy) == 16, "Offset de PUSHA incorreto");
_Static_assert(offsetof(registers_t, eip) == 44, "Offset de EIP incorreto");

// Descricoes textuais das 32 excecoes padrao da CPU x86 (Intel SDM vol. 3,
// cap. 6). Exibidas pela rotina de kernel panic.
static const char *excecoes[32] = {
    "Divisao por Zero",
    "Depuracao (Debug)",
    "Interrupcao Nao Mascaravel (NMI)",
    "Breakpoint",
    "Overflow",
    "Limite de Intervalo Excedido (BOUND)",
    "Opcode Invalido",
    "Dispositivo Nao Disponivel (No Math Coprocessor)",
    "Dupla Falha (Double Fault)",
    "Overrun de Coprocessador (legado)",
    "TSS Invalida",
    "Segmento Nao Presente",
    "Falha de Pilha (Stack-Segment Fault)",
    "Falha de Protecao Geral (General Protection Fault)",
    "Falha de Pagina (Page Fault)",
    "Reservada",
    "Excecao de Ponto Flutuante (x87 FPU)",
    "Verificacao de Alinhamento",
    "Falha de Maquina (Machine Check)",
    "Excecao SIMD de Ponto Flutuante",
    "Excecao de Virtualizacao",
    "Excecao de Protecao de Controle",
    "Reservada",
    "Reservada",
    "Reservada",
    "Reservada",
    "Reservada",
    "Reservada",
    "Injecao de Hypervisor",
    "Excecao de Comunicacao com VMM",
    "Excecao de Seguranca",
    "Reservada"
};

// Excecoes do tipo "abort" (Intel SDM vol. 3, cap. 6.5): o estado da CPU nao
// permite retomar a execucao, entao um handler nunca pode "consertar" e voltar.
#define EXC_DOUBLE_FAULT  8
#define EXC_MACHINE_CHECK 18

// Tabela de despacho, indexada pelo numero do vetor (regs.int_no):
//   [0..31]  excecoes da CPU      -> isr_register_handler()
//   [32..47] IRQs 0-15 do PIC     -> irq_register_handler()
// NULL = sem handler. Fica em .bss, entao comeca toda zerada. Cada entrada e
// um unico ponteiro de 32 bits alinhado, gravado com uma unica instrucao: nao
// existe leitura "pela metade" caso uma interrupcao ocorra durante o registro.
static interrupt_handler_t handlers[INT_NUM_VETORES];

// `vetor` ja deve ter sido validado pelos chamadores (< INT_NUM_VETORES).
static int vetor_registrar(uint32_t vetor, interrupt_handler_t handler) {
    if (handler == NULL || handlers[vetor] != NULL)
        return -1;
    handlers[vetor] = handler;
    return 0;
}

int isr_register_handler(uint8_t vetor, interrupt_handler_t handler) {
    if (vetor >= ISR_NUM_EXCECOES)
        return -1;
    return vetor_registrar(vetor, handler);
}

int isr_unregister_handler(uint8_t vetor) {
    if (vetor >= ISR_NUM_EXCECOES)
        return -1;
    handlers[vetor] = NULL;
    return 0;
}

int irq_register_handler(uint8_t irq, interrupt_handler_t handler) {
    if (irq >= IRQ_NUM_LINHAS)
        return -1;
    return vetor_registrar(IRQ_VETOR_BASE + irq, handler);
}

int irq_unregister_handler(uint8_t irq) {
    if (irq >= IRQ_NUM_LINHAS)
        return -1;
    handlers[IRQ_VETOR_BASE + irq] = NULL;
    return 0;
}

// Despachante: todos os 32 stubs de excecao (interrupts.s) caem aqui.
//
// Mantem o argumento por valor, conforme o contrato do stub Assembly existente.
// Seu endereco aponta para o quadro salvo, nao para registradores lidos depois.
void isr_handler(registers_t regs) {
    // O stub atual nao limpa DF; garanta a direcao esperada pelas rotinas C.
    // O valor original de EFLAGS permanece salvo em regs.eflags.
    asm volatile("cld" ::: "cc");

    uint32_t vetor = regs.int_no;

    // O stub so empilha 0-31, mas int_no vem da pilha: nao indexe a tabela
    // sem validar. Um valor fora da faixa indica quadro corrompido.
    if (vetor >= ISR_NUM_EXCECOES)
        kernel_panic("Excecao desconhecida", &regs);

    interrupt_handler_t handler = handlers[vetor];
    if (handler == NULL)
        kernel_panic(excecoes[vetor], &regs);

    handler(&regs);

    // Aborts nao sao retomaveis: se o handler voltou, ainda assim e fatal.
    if (vetor == EXC_DOUBLE_FAULT || vetor == EXC_MACHINE_CHECK)
        kernel_panic(excecoes[vetor], &regs);
}

// Registra as 32 excecoes da CPU na IDT.
void isr_install(void) {
    void (*isr_stubs[32])(void) = {
        isr0,  isr1,  isr2,  isr3,  isr4,  isr5,  isr6,  isr7,
        isr8,  isr9,  isr10, isr11, isr12, isr13, isr14, isr15,
        isr16, isr17, isr18, isr19, isr20, isr21, isr22, isr23,
        isr24, isr25, isr26, isr27, isr28, isr29, isr30, isr31
    };

    for (int i = 0; i < 32; i++) {
        // Seletor 0x08: segmento de codigo do kernel (definido na GDT).
        // Flags 0x8E: presente, DPL = 0, gate de interrupcao de 32 bits.
        idt_set_gate((uint8_t)i, (uint32_t)isr_stubs[i], SELETOR_CODIGO_KERNEL, 0x8E);
    }
}

// Despachante das IRQs de hardware: todos os 16 stubs irqN (interrupts.s) caem aqui.
void irq_handler(registers_t regs) {
    asm volatile("cld" ::: "cc");

    // Aritmetica sem sinal: int_no < IRQ_VETOR_BASE "da a volta" e tambem
    // falha no teste abaixo. Fora da faixa nao ha linha de PIC para confirmar.
    uint32_t irq = regs.int_no - IRQ_VETOR_BASE;
    if (irq >= IRQ_NUM_LINHAS)
        return;

    // IRQ 7 e 15 podem ser espurias (pic.h). Nesse caso nao ha handler nem
    // EOI a enviar por aqui; pic_irq_espuria() cuida do que o hardware exigir.
    if (pic_irq_espuria((uint8_t)irq))
        return;

    // EOI antes do handler: as interrupcoes continuam mascaradas na CPU ate o
    // iret, entao a mesma linha nao reentra, e o PIC nao fica travado se o
    // handler nao voltar por aqui (ex.: troca de contexto no timer).
    pic_send_eoi((uint8_t)irq);

    interrupt_handler_t handler = handlers[regs.int_no];
    if (handler != NULL)
        handler(&regs);
}

// Registra as 16 IRQs na IDT, nos vetores 32-47 (mesmo remapeamento do PIC).
void irq_install(void) {
    void (*irq_stubs[16])(void) = {
        irq0,  irq1,  irq2,  irq3,  irq4,  irq5,  irq6,  irq7,
        irq8,  irq9,  irq10, irq11, irq12, irq13, irq14, irq15
    };

    for (int i = 0; i < 16; i++)
        idt_set_gate((uint8_t)(32 + i), (uint32_t)irq_stubs[i], SELETOR_CODIGO_KERNEL, 0x8E);
}
