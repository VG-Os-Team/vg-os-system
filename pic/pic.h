#ifndef VGOS_PIC_H
#define VGOS_PIC_H

#include <stdint.h>

/*
 * Inicializa os PICs no boot: IRQ0-IRQ15 passam a usar os vetores 0x20-0x2F.
 * Deve ser chamada com interrupções mascaráveis desabilitadas (IF = 0).
 * Ao retornar, todas as IRQs continuam mascaradas. Esta função não executa STI.
 */
void pic_init(void);

/*
 * Encerra o atendimento de uma IRQ real. O argumento é a linha (0-15),
 * não o vetor da IDT (0x20-0x2F). Valores fora desse intervalo são ignorados.
 * O handler deve tratar IRQs espurias 7/15 antes de usar este EOI comum.
 * Não chamar para exceções da CPU nem para interrupções de software.
 */
void pic_send_eoi(uint8_t irq);

#endif
