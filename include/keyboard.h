#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

/* Tamanho da fila circular FIFO exigido na Tarefa 21 (256 posicoes) */
#define KBD_BUFFER_SIZE 256U
#define KBD_FIFO_MASK   (KBD_BUFFER_SIZE - 1U)

#define TECLA_NENHUMA       0
#define TECLA_CTRL_L       -1  // limpar terminal
#define TECLA_CTRL_C       -2  // copiar linha atual
#define TECLA_CTRL_V       -3  // colar
#define TECLA_CTRL_O       -4  // salvar arquivo no editor
#define TECLA_CTRL_X       -5  // sair do editor

#define TECLA_SETA_CIMA    -10
#define TECLA_SETA_BAIXO   -11
#define TECLA_SETA_ESQUERDA -12
#define TECLA_SETA_DIREITA -13
#define TECLA_DELETE       -14
#define TECLA_HOME         -15
#define TECLA_END          -16

/* Estrutura formal do Buffer Circular FIFO de 256 bytes/eventos */
typedef struct {
  uint8_t           data[KBD_BUFFER_SIZE]; /* Armazena os bytes de eventos/caracteres (256 bytes) */
  volatile uint32_t head;                  /* Indice de escrita (produtor: IRQ1) */
  volatile uint32_t tail;                  /* Indice de leitura (consumidor: Shell/UI) */
  volatile uint32_t count;                 /* Total de bytes armazenados no momento */
  volatile uint32_t dropped;               /* Contador de eventos descartados por buffer cheio */
} kbd_fifo_t;

// Inicializa o driver de teclado PS/2 via IRQ1 e zera a fila FIFO.
void teclado_init(void);

// Operacoes diretas da fila circular FIFO de 256 bytes
void     kbd_fifo_init(void);
void     kbd_fifo_clear(void);
int      kbd_fifo_is_empty(void);
int      kbd_fifo_is_full(void);
uint32_t kbd_fifo_count(void);
uint32_t kbd_fifo_dropped(void);
int      kbd_fifo_push(uint8_t byte_evento);
int      kbd_fifo_pop(uint8_t *out_byte);

// Retorna o proximo caractere/comando da fila FIFO (0 se vazia, >0 ASCII, <0 comando).
int teclado_ler(void);

// Coloca a CPU em repouso (hlt) de forma segura ate a proxima interrupcao caso a fila esteja vazia.
void teclado_aguardar(void);

#endif /* KEYBOARD_H */