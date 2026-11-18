#ifndef SHELL_H
#define SHELL_H

/*
 * Rotina principal do shell interativo do VG OS.
 * Desenha o layout do terminal, renderiza o prompt, aguarda entradas da fila
 * de teclado (IRQ1), atualiza o cursor na tela, acumula a linha de texto e
 * despacha comandos ao pressionar Enter.
 */
void shell_run(void);

#endif
