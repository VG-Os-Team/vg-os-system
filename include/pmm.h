#ifndef PMM_H
#define PMM_H

#include "multiboot.h"
#include <stddef.h>
#include <stdint.h>

/* Tamanho de cada bloco (pagina fisica) gerenciado: 4KB */
#define PMM_BLOCK_SIZE          4096U
#define PMM_BLOCKS_PER_BYTE     8U
#define PMM_BLOCKS_PER_WORD     32U

/*
 * Suporte a ate 4GB de memoria fisica no espaco de enderecamento de 32 bits:
 * 4GB = 4 * 1024 * 1024 * 1024 bytes = 4.294.967.296 bytes.
 * 4.294.967.296 / 4096 = 1.048.576 blocos de 4KB.
 * Em um array de bitmap onde cada bit representa 1 bloco:
 * 1.048.576 bits = 131.072 bytes (128 KB) = 32.768 palavras de 32 bits (uint32_t).
 */
#define PMM_MAX_BLOCKS          1048576U
#define PMM_BITMAP_SIZE         (PMM_MAX_BLOCKS / PMM_BLOCKS_PER_WORD)

/* Macros de conversao e alinhamento */
#define PMM_ADDR_TO_BLOCK(addr)     ((uint32_t)(uintptr_t)(addr) / PMM_BLOCK_SIZE)
#define PMM_BLOCK_TO_ADDR(block)    ((void *)(uintptr_t)((uint32_t)(block) * PMM_BLOCK_SIZE))
#define PMM_ALIGN_DOWN(addr)        ((addr) & ~(PMM_BLOCK_SIZE - 1U))
#define PMM_ALIGN_UP(addr)          (((addr) + PMM_BLOCK_SIZE - 1U) & ~(PMM_BLOCK_SIZE - 1U))

/* Estrutura para relatorio de estatisticas */
typedef struct {
    uint32_t total_blocks;
    uint32_t used_blocks;
    uint32_t free_blocks;
    uint32_t total_memory_kb;
    uint32_t used_memory_kb;
    uint32_t free_memory_kb;
} pmm_stats_t;

/* Inicializacao do PMM com base nas regioes de memoria do Multiboot */
void pmm_init(const multiboot_info_t *mbd);

/* Rotinas de manipulacao do bitmap */
void pmm_set_block(uint32_t bit);
void pmm_unset_block(uint32_t bit);
int pmm_test_block(uint32_t bit);

/* Busca de blocos livres no bitmap */
int pmm_first_free_block(void);
int pmm_first_free_blocks(size_t count);

/* Alocacao e liberacao de pagina fisica de 4KB */
void *pmm_alloc_block(void);
void pmm_free_block(void *ptr);

/* Alocacao e liberacao de paginas continuas */
void *pmm_alloc_blocks(size_t count);
void pmm_free_blocks(void *ptr, size_t count);

/* Configuracao de regioes continuas (marcar livres/ocupadas) */
void pmm_init_region(uint32_t base, size_t size);
void pmm_deinit_region(uint32_t base, size_t size);

/* Diagnostico e consulta de estado */
uint32_t pmm_get_max_blocks(void);
uint32_t pmm_get_used_blocks(void);
uint32_t pmm_get_free_blocks(void);
uint32_t pmm_get_total_memory_kb(void);
uint32_t pmm_get_used_memory_kb(void);
uint32_t pmm_get_free_memory_kb(void);
void pmm_get_stats(pmm_stats_t *stats);

#endif /* PMM_H */
