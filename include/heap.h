#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include <stdint.h>

/*
 * Heap dinamico do kernel (kmalloc / kfree).
 *
 * Alocador de lista encadeada com first-fit, divisao de blocos
 * na alocacao e coalescencia de vizinhos livres na liberacao.
 * A regiao usada pelo heap precisa estar mapeada (identity map).
 */

/* Tamanho inicial do heap do kernel (pedido ao PMM).
 * Dimensionado em 16 MB para comportar o backbuffer de video na RAM
 * (3 MB em 1024x768x32bpp e ate ~8 MB em 1920x1080x32bpp) com folga
 * para as demais alocacoes dinamicas do kernel. */
#define HEAP_INITIAL_SIZE (16U * 1024U * 1024U)

/* Inicializa o heap sobre a regiao [start, start + size). */
void heap_init(void *start, size_t size);

/*
 * Reserva paginas fisicas contiguas no PMM, garante o identity map delas
 * no paging e inicializa o heap sobre essa regiao.
 * Deve ser chamada depois de pmm_init() e paging_init().
 * Retorna 0 em caso de sucesso e -1 se faltar memoria ou o mapeamento falhar.
 */
int heap_init_from_pmm(size_t size);

/* Aloca 'size' bytes (alinhados em 16). Retorna NULL se nao houver memoria. */
void *kmalloc(size_t size);

/* Libera um ponteiro retornado por kmalloc. kfree(NULL) e permitido. */
void kfree(void *ptr);

/* Total de bytes livres (somente payload), util para depuracao. */
size_t heap_free_bytes(void);

#endif /* HEAP_H */
