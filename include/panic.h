#ifndef PANIC_H
#define PANIC_H

#include <stdint.h>
#include "isr.h"

// Registra um framebuffer de 32 bits com pixels 0x00RRGGBB.
// O chamador valida a presenca dos dados Multiboot, o endereco completo antes
// da conversao para ponteiro, bpp, tipo RGB e posicoes/tamanhos dos canais.
// Aqui sao validados alinhamento, geometria e limites aritmeticos do endereco;
// isso nao comprova que a memoria esteja mapeada e acessivel.
// Retorna 0 e desabilita o video se a validacao falhar; panic apenas para.
// Registre durante a inicializacao, antes de usar o framebuffer.
int panic_configurar_video(uint32_t *fb, uint32_t pitch,
                          uint32_t largura, uint32_t altura);

// Nao retorna. regs pode ser NULL quando nao ha um quadro de excecao.
void kernel_panic(const char *message, registers_t *regs)
    __attribute__((noreturn));

#endif
