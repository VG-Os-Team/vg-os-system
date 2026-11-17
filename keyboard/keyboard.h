#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdint.h>

#define TECLA_NENHUMA 0
#define TECLA_CTRL_L -1 // limpar terminal
#define TECLA_CTRL_C -2 // copiar linha atual
#define TECLA_CTRL_V -3 // colar

#define TECLA_SETA_CIMA -10
#define TECLA_SETA_BAIXO -11
#define TECLA_SETA_ESQUERDA -12
#define TECLA_SETA_DIREITA -13
#define TECLA_DELETE -14
#define TECLA_HOME -15
#define TECLA_END -16

// Inicializa o driver de teclado PS/2 via IRQ1 (registra handler e desmascara no PIC).
void teclado_init(void);

// Retorna o caractere correspondente ao proximo evento na fila do teclado, ja
// considerando shift/capslock. Retorna 0 se a fila estiver vazia ou se a tecla
// nao gerar caractere.
// Retorna: 0 = nada, >0 = código ASCII do caractere, <0 = comando especial
int teclado_ler(void);

// Coloca a CPU em repouso (hlt) de forma segura ate a proxima interrupcao caso
// a fila do teclado esteja vazia.
void teclado_aguardar(void);

#endif