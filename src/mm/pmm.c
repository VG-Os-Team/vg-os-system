#include "pmm.h"
#include "strutil.h"
#include <stdint.h>
#include <stddef.h>

/*
 * Simbolos exportados pelo script de linkagem (linker.ld)
 * marcando o inicio e o fim da imagem binaria do kernel.
 */
extern const uint8_t _kernel_start[];
extern const uint8_t _kernel_end[];

/*
 * Bitmap do gerenciador de memoria fisica (PMM):
 * Cada bit representa uma moldura de pagina (frame) de 4KB.
 *
 * Convencao:
 *   1 = Bloco ocupado / reservado
 *   0 = Bloco livre
 */
static uint32_t pmm_bitmap[PMM_BITMAP_SIZE];
static uint32_t pmm_max_blocks = 0;
static uint32_t pmm_used_blocks = 0;

static inline void pmm_bitmap_set(uint32_t bit) {
    pmm_bitmap[bit / PMM_BLOCKS_PER_WORD] |= (1U << (bit % PMM_BLOCKS_PER_WORD));
}

static inline void pmm_bitmap_unset(uint32_t bit) {
    pmm_bitmap[bit / PMM_BLOCKS_PER_WORD] &= ~(1U << (bit % PMM_BLOCKS_PER_WORD));
}

static inline int pmm_bitmap_test(uint32_t bit) {
    return (pmm_bitmap[bit / PMM_BLOCKS_PER_WORD] & (1U << (bit % PMM_BLOCKS_PER_WORD))) != 0;
}

void pmm_set_block(uint32_t bit) {
    if (bit >= pmm_max_blocks) {
        return;
    }
    if (!pmm_bitmap_test(bit)) {
        pmm_bitmap_set(bit);
        pmm_used_blocks++;
    }
}

void pmm_unset_block(uint32_t bit) {
    if (bit >= pmm_max_blocks) {
        return;
    }
    if (pmm_bitmap_test(bit)) {
        pmm_bitmap_unset(bit);
        if (pmm_used_blocks > 0) {
            pmm_used_blocks--;
        }
    }
}

int pmm_test_block(uint32_t bit) {
    if (bit >= pmm_max_blocks) {
        return 1; /* Fora do limite gerenciado e tratado como ocupado */
    }
    return pmm_bitmap_test(bit);
}

int pmm_first_free_block(void) {
    uint32_t max_words = (pmm_max_blocks + PMM_BLOCKS_PER_WORD - 1U) / PMM_BLOCKS_PER_WORD;

    for (uint32_t i = 0; i < max_words; i++) {
        /* Se a palavra tiver todos os 32 bits em 1, todos estao ocupados */
        if (pmm_bitmap[i] != 0xFFFFFFFFU) {
            for (int b = 0; b < 32; b++) {
                if (!(pmm_bitmap[i] & (1U << b))) {
                    uint32_t bit = i * PMM_BLOCKS_PER_WORD + (uint32_t)b;
                    if (bit < pmm_max_blocks) {
                        return (int)bit;
                    }
                }
            }
        }
    }

    return -1; /* Memoria fisica esgotada */
}

int pmm_first_free_blocks(size_t count) {
    if (count == 0) {
        return -1;
    }
    if (count == 1) {
        return pmm_first_free_block();
    }

    uint32_t streak = 0;
    uint32_t start_bit = 0;

    for (uint32_t bit = 0; bit < pmm_max_blocks; bit++) {
        if (!pmm_bitmap_test(bit)) {
            if (streak == 0) {
                start_bit = bit;
            }
            streak++;
            if (streak == count) {
                return (int)start_bit;
            }
        } else {
            streak = 0;
        }
    }

    return -1;
}

void *pmm_alloc_block(void) {
    int bit = pmm_first_free_block();
    if (bit < 0) {
        return NULL;
    }

    pmm_set_block((uint32_t)bit);
    return PMM_BLOCK_TO_ADDR(bit);
}

void pmm_free_block(void *ptr) {
    if (!ptr) {
        return;
    }

    uint32_t addr = (uint32_t)(uintptr_t)ptr;
    uint32_t bit = PMM_ADDR_TO_BLOCK(addr);

    pmm_unset_block(bit);
}

void *pmm_alloc_blocks(size_t count) {
    if (count == 0) {
        return NULL;
    }

    int start_bit = pmm_first_free_blocks(count);
    if (start_bit < 0) {
        return NULL;
    }

    for (size_t i = 0; i < count; i++) {
        pmm_set_block((uint32_t)start_bit + (uint32_t)i);
    }

    return PMM_BLOCK_TO_ADDR(start_bit);
}

void pmm_free_blocks(void *ptr, size_t count) {
    if (!ptr || count == 0) {
        return;
    }

    uint32_t addr = (uint32_t)(uintptr_t)ptr;
    uint32_t start_bit = PMM_ADDR_TO_BLOCK(addr);

    for (size_t i = 0; i < count; i++) {
        pmm_unset_block(start_bit + (uint32_t)i);
    }
}

void pmm_init_region(uint32_t base, size_t size) {
    if (size == 0) {
        return;
    }

    /* Arredonda o endereco base para cima (garante que apenas paginas inteiras livres sao liberadas) */
    uint32_t start_block = (base + PMM_BLOCK_SIZE - 1U) / PMM_BLOCK_SIZE;

    uint64_t end = (uint64_t)base + size;
    /* Arredonda o endereco final para baixo */
    uint32_t end_block = (uint32_t)(end / PMM_BLOCK_SIZE);

    for (uint32_t b = start_block; b < end_block && b < pmm_max_blocks; b++) {
        pmm_unset_block(b);
    }
}

void pmm_deinit_region(uint32_t base, size_t size) {
    if (size == 0) {
        return;
    }

    /* Arredonda o endereco base para baixo para cobrir qualquer pagina tocada */
    uint32_t start_block = base / PMM_BLOCK_SIZE;

    uint64_t end = (uint64_t)base + size;
    /* Arredonda o endereco final para cima */
    uint32_t end_block = (uint32_t)((end + PMM_BLOCK_SIZE - 1U) / PMM_BLOCK_SIZE);

    for (uint32_t b = start_block; b < end_block && b < pmm_max_blocks; b++) {
        pmm_set_block(b);
    }
}

/* Contexto para descobrir o maior endereco fisico reportado */
struct pmm_mmap_scan_ctx {
    uint64_t max_addr;
};

static void pmm_descobrir_teto_cb(uint64_t addr, uint64_t len, uint32_t type, void *ctx) {
    struct pmm_mmap_scan_ctx *scan = (struct pmm_mmap_scan_ctx *)ctx;
    if (type == MULTIBOOT_MEMORY_AVAILABLE) {
        uint64_t fim = addr + len;
        if (fim > scan->max_addr) {
            scan->max_addr = fim;
        }
    }
}

static void pmm_liberar_disponiveis_cb(uint64_t addr, uint64_t len, uint32_t type, void *ctx) {
    (void)ctx;
    if (type == MULTIBOOT_MEMORY_AVAILABLE && addr < 0x100000000ULL) {
        uint32_t base = (uint32_t)addr;
        uint64_t tam = len;
        if ((uint64_t)base + tam > 0x100000000ULL) {
            tam = 0x100000000ULL - base;
        }
        pmm_init_region(base, (size_t)tam);
    }
}

void pmm_init(const multiboot_info_t *mbd) {
    /*
     * 1. Inicializa todo o array de bitmap como 1 (todos os blocos ocupados/reservados).
     * Dessa forma, qualquer espaco nao reportado ou memoria reservada da placa-mae
     * nunca sera acidentalmente alocado.
     */
    for (uint32_t i = 0; i < PMM_BITMAP_SIZE; i++) {
        pmm_bitmap[i] = 0xFFFFFFFFU;
    }

    pmm_max_blocks = PMM_MAX_BLOCKS;
    pmm_used_blocks = pmm_max_blocks;

    /*
     * 2. Consome o mapa de memoria atraves do parser oficial do multiboot_mem,
     * respeitando a integridade do modulo e evitando duplicacao de parsing bruto.
     */
    struct pmm_mmap_scan_ctx scan = { .max_addr = 0 };
    multiboot_mem_status_t status = multiboot_mem_iterar_regioes(pmm_descobrir_teto_cb, &scan);

    if (status != MULTIBOOT_MEM_OK && mbd) {
        status = multiboot_mem_iterar(mbd, pmm_descobrir_teto_cb, &scan);
    }

    if (status == MULTIBOOT_MEM_OK && scan.max_addr > 0) {
        uint64_t max_addr = scan.max_addr;
        if (max_addr > 0x100000000ULL) {
            max_addr = 0x100000000ULL;
        }

        pmm_max_blocks = (uint32_t)(max_addr / PMM_BLOCK_SIZE);
        if (pmm_max_blocks > PMM_MAX_BLOCKS) {
            pmm_max_blocks = PMM_MAX_BLOCKS;
        }
        pmm_used_blocks = pmm_max_blocks;

        /* Marca regioes do tipo AVAILABLE como livres no bitmap */
        multiboot_mem_iterar_regioes(pmm_liberar_disponiveis_cb, NULL);
    } else {
        /* Fallback via campos legados mem_lower / mem_upper caso o mmap nao esteja disponivel */
        if (mbd && (mbd->flags & MULTIBOOT_INFO_MEMORY)) {
            uint32_t total_kb = 1024U + mbd->mem_upper;
            uint32_t total_bytes = total_kb * 1024U;
            pmm_max_blocks = total_bytes / PMM_BLOCK_SIZE;
            if (pmm_max_blocks > PMM_MAX_BLOCKS) {
                pmm_max_blocks = PMM_MAX_BLOCKS;
            }
            pmm_used_blocks = pmm_max_blocks;
            pmm_init_region(0x00100000U, (mbd->mem_upper) * 1024U);
        } else {
            /* Fallback seguro padrao de 32MB */
            pmm_max_blocks = (32U * 1024U * 1024U) / PMM_BLOCK_SIZE;
            pmm_used_blocks = pmm_max_blocks;
            pmm_init_region(0x00100000U, (31U * 1024U * 1024U));
        }
    }

    /*
     * 4. PROTECAO DE REGIOES CRITICAS DO SISTEMA
     *
     * (a) Baixa memoria (primeiro 1MB: 0x00000000 ate 0x00100000):
     *     Contem IVT, BDA, EBDA, buffer de video VGA e ROM da BIOS.
     */
    pmm_deinit_region(0x00000000U, 0x00100000U);

    /*
     * (b) Imagem do Kernel: codigo, dados constantes, variaveis e .bss (incluindo o proprio pmm_bitmap)
     */
    uint32_t kstart = (uint32_t)(uintptr_t)_kernel_start;
    uint32_t kend = (uint32_t)(uintptr_t)_kernel_end;
    if (kend > kstart) {
        pmm_deinit_region(kstart, kend - kstart);
    }

    /*
     * (c) Estrutura do Multiboot e tabela mmap
     */
    if (mbd) {
        pmm_deinit_region((uint32_t)(uintptr_t)mbd, sizeof(multiboot_info_t));
        if (mbd->mmap_addr && mbd->mmap_length) {
            pmm_deinit_region(mbd->mmap_addr, mbd->mmap_length);
        }
        if ((mbd->flags & (1U << 3)) && mbd->mods_count > 0 && mbd->mods_addr) {
            pmm_deinit_region(mbd->mods_addr, mbd->mods_count * 16U);
        }
        /*
         * (d) Framebuffer de video (se localizado dentro da memoria RAM gerenciada)
         */
        if ((mbd->flags & (1U << 12)) && mbd->framebuffer_addr < 0x100000000ULL) {
            uint32_t fb_size = mbd->framebuffer_pitch * mbd->framebuffer_height;
            if (fb_size > 0) {
                pmm_deinit_region((uint32_t)mbd->framebuffer_addr, fb_size);
            }
        }
    }
}

uint32_t pmm_get_max_blocks(void) {
    return pmm_max_blocks;
}

uint32_t pmm_get_used_blocks(void) {
    return pmm_used_blocks;
}

uint32_t pmm_get_free_blocks(void) {
    if (pmm_used_blocks >= pmm_max_blocks) {
        return 0;
    }
    return pmm_max_blocks - pmm_used_blocks;
}

uint32_t pmm_get_total_memory_kb(void) {
    return pmm_max_blocks * (PMM_BLOCK_SIZE / 1024U);
}

uint32_t pmm_get_used_memory_kb(void) {
    return pmm_used_blocks * (PMM_BLOCK_SIZE / 1024U);
}

uint32_t pmm_get_free_memory_kb(void) {
    return pmm_get_free_blocks() * (PMM_BLOCK_SIZE / 1024U);
}

void pmm_get_stats(pmm_stats_t *stats) {
    if (!stats) {
        return;
    }
    stats->total_blocks = pmm_max_blocks;
    stats->used_blocks = pmm_used_blocks;
    stats->free_blocks = pmm_get_free_blocks();
    stats->total_memory_kb = pmm_get_total_memory_kb();
    stats->used_memory_kb = pmm_get_used_memory_kb();
    stats->free_memory_kb = pmm_get_free_memory_kb();
}
