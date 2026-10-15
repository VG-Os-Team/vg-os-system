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

/*
 * Diz se a IRQ recebida é espúria. O argumento é a linha (0-15), não o vetor.
 * Somente as linhas 7 (mestre) e 15 (escravo) podem ser espúrias; para as
 * demais devolve sempre 0, sem tocar no hardware.
 *
 * Retorna 1 se for espúria: o chamador NÃO deve despachar handler nem chamar
 * pic_send_eoi(). Para a IRQ 15 espúria, esta função já envia o EOI ao PIC
 * mestre, que é o único que precisa dele nesse caso.
 * Retorna 0 se a IRQ for real: o chamador segue o fluxo normal (EOI + handler).
 */
int pic_irq_espuria(uint8_t irq);

#endif
