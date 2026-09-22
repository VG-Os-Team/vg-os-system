#include "isr.h"

// Descricoes textuais das 32 excecoes padrao da CPU x86 (Intel SDM vol. 3,
// cap. 6). Usadas futuramente por uma rotina de kernel panic.
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

// Handler generico: todos os 32 stubs de excecao (interrupts.s) caem aqui.
//
// Por enquanto so trava a maquina, pois este modulo nao tem acesso ao
// framebuffer (isso pertence a render.h / kernel.c). O ponto de extensao
// natural e chamar uma futura rotina de "kernel panic" passando
// excecoes[regs.int_no], regs.err_code e regs.eip para desenhar a tela de
// erro antes de travar.
void isr_handler(registers_t regs) {
    const char *nome = (regs.int_no < 32) ? excecoes[regs.int_no] : "Excecao desconhecida";
    (void)nome; // evita warning de "unused" ate a integracao com render.h

    // TODO: chamar kernel_panic(nome, regs.err_code, regs.eip) quando essa
    // rotina existir.

    asm volatile("cli");
    while (1) {
        asm volatile("hlt");
    }
}

// ATENCAO: presume-se que o modulo idt/ exponha uma funcao com esta
// assinatura, seguindo a mesma convencao de gdt_set_entry:
//
//   void idt_set_entry(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
//
// Se o nome ou a assinatura real em idt.h/idt.c forem diferentes, ajuste
// esta declaracao e a chamada dentro de isr_install().
extern void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

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
        idt_set_gate((uint8_t)i, (uint32_t)isr_stubs[i], 0x08, 0x8E);
    }
}
