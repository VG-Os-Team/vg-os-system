#include "pic.h"

#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA    0x21
#define PIC_SLAVE_COMMAND  0xA0
#define PIC_SLAVE_DATA     0xA1

#define PIC_MASTER_OFFSET 0x20
#define PIC_SLAVE_OFFSET  0x28

#define PIC_ICW1_INIT    0x10
#define PIC_ICW1_ICW4    0x01
#define PIC_ICW4_8086    0x01
#define PIC_MASTER_IRQ2  0x04
#define PIC_SLAVE_ID     0x02
#define PIC_MASK_ALL     0xFF
#define PIC_EOI          0x20
#define PIC_OCW3_READ_ISR 0x0B  // próxima leitura da porta de comando devolve o ISR

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static inline void io_wait(void) {
    // Porta de diagnóstico POST: espera de E/S tradicional em PCs x86.
    outb(0x80, 0);
}

void pic_init(void) {
    // ICW1: modo em cascata, disparo por borda e presença de ICW4.
    outb(PIC_MASTER_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();
    outb(PIC_SLAVE_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    io_wait();

    // ICW2: mestre em 0x20-0x27; escravo em 0x28-0x2F.
    outb(PIC_MASTER_DATA, PIC_MASTER_OFFSET);
    io_wait();
    outb(PIC_SLAVE_DATA, PIC_SLAVE_OFFSET);
    io_wait();

    // ICW3: o escravo esta ligado a linha IRQ2 do mestre.
    outb(PIC_MASTER_DATA, PIC_MASTER_IRQ2);
    io_wait();
    outb(PIC_SLAVE_DATA, PIC_SLAVE_ID);
    io_wait();

    // ICW4: modo 8086, com EOI enviado pelo software (AEOI desativado).
    outb(PIC_MASTER_DATA, PIC_ICW4_8086);
    io_wait();
    outb(PIC_SLAVE_DATA, PIC_ICW4_8086);
    io_wait();

    /*
     * A inicialização do 8259A limpa suas mascaras. Bloqueamos todas as
     * linhas até que existam handlers de IRQ instalados na IDT.
     * Cada bit em 1 mascara uma linha; não restauramos máscaras do firmware.
    */
    outb(PIC_MASTER_DATA, PIC_MASK_ALL);
    io_wait();
    outb(PIC_SLAVE_DATA, PIC_MASK_ALL);
    io_wait();
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 16)
        return;

    // IRQ8-IRQ15 passam pelo escravo e pela cascata IRQ2 do mestre.
    if (irq >= 8)
        outb(PIC_SLAVE_COMMAND, PIC_EOI);

    outb(PIC_MASTER_COMMAND, PIC_EOI);
}

// Lê o In-Service Register (ISR) do PIC cuja porta de comando é informada.
// Bit n em 1 = a IRQ n desse chip está em atendimento (recebeu INTA, ainda sem EOI).
static uint8_t pic_read_isr(uint16_t command_port) {
    outb(command_port, PIC_OCW3_READ_ISR);
    return inb(command_port);
}

int pic_irq_espuria(uint8_t irq) {
    /*
     * Uma IRQ espúria acontece quando a linha cai antes do INTA: o PIC entrega
     * o vetor da sua última linha (7 no mestre, 15 no escravo), mas o bit
     * correspondente no ISR NÃO fica ligado. Só essas duas linhas podem ser
     * espúrias; para as demais não há o que verificar.
     */
    if (irq == 7)
        // Espúria do mestre: nenhum EOI, pois o mestre não marcou nada em serviço.
        return !(pic_read_isr(PIC_MASTER_COMMAND) & (1u << 7));

    if (irq == 15) {
        if (pic_read_isr(PIC_SLAVE_COMMAND) & (1u << 7))
            return 0;

        /*
         * Espúria do escravo: ele não recebe EOI, mas o mestre já tratou a
         * cascata (IRQ2) como real e está com o bit 2 ligado no ISR dele.
         * Sem este EOI, o mestre ficaria bloqueado para IRQs de menor prioridade.
         */
        outb(PIC_MASTER_COMMAND, PIC_EOI);
        return 1;
    }

    return 0;
}
