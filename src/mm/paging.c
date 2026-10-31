#include "paging.h"

#include <stddef.h>
#include <stdint.h>

/* Simbolos definidos em linker.ld: limites do kernel na memoria fisica. */
extern char _kernel_start[];
extern char _kernel_end[];

#define CR0_PG_BIT 0x80000000U

/*
 * Page Directory e pool de Page Tables ficam no .bss do kernel.
 * O alinhamento de 4 KB e obrigatorio: CR3 e as entradas do diretorio
 * guardam apenas os 20 bits altos do endereco. Como o kernel e linkado em
 * 1 MB sem higher-half, o endereco destas variaveis ja e o endereco fisico.
 * Por estarem dentro de [_kernel_start, _kernel_end), o PMM ja as trata
 * como memoria ocupada.
 */
static uint32_t page_directory[PAGING_ENTRIES]
    __attribute__((aligned(PAGING_PAGE_SIZE)));
static uint32_t page_tables[PAGING_MAX_TABLES][PAGING_ENTRIES]
    __attribute__((aligned(PAGING_PAGE_SIZE)));

static uint32_t tables_used;
static int paging_active;

static uint32_t align_down(uint32_t value) {
    return value & PAGING_ADDR_MASK;
}

/* Retorna a Page Table do indice de diretorio informado, criando-a se ainda
 * nao existir. Retorna NULL se o pool acabou. */
static uint32_t *paging_get_table(uint32_t dir_index) {
    uint32_t entry = page_directory[dir_index];

    if (entry & PAGING_FLAG_PRESENT)
        return (uint32_t *)(uintptr_t)(entry & PAGING_ADDR_MASK);

    if (tables_used >= PAGING_MAX_TABLES)
        return NULL;

    uint32_t *table = page_tables[tables_used++];

    /* Zera explicitamente: uma entrada com lixo seria interpretada como
     * pagina valida pela CPU. */
    for (uint32_t i = 0; i < PAGING_ENTRIES; i++)
        table[i] = 0;

    /* No diretorio, WRITABLE fica ligado e o controle fino fica por pagina:
     * a CPU exige as duas permissoes (diretorio E tabela) para permitir. */
    page_directory[dir_index] = (uint32_t)(uintptr_t)table |
                                PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE;
    return table;
}

paging_status_t paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags) {
    if ((virt & ~PAGING_ADDR_MASK) || (phys & ~PAGING_ADDR_MASK))
        return PAGING_ERR_UNALIGNED;

    /* Endereco virtual de 32 bits = 10 bits (diretorio) | 10 bits (tabela)
     * | 12 bits (offset dentro da pagina). */
    uint32_t dir_index = virt >> 22;
    uint32_t table_index = (virt >> 12) & 0x3FF;

    uint32_t *table = paging_get_table(dir_index);
    if (table == NULL)
        return PAGING_ERR_NO_TABLES;

    table[table_index] = phys | (flags & 0xFFFU) | PAGING_FLAG_PRESENT;

    /* Com a paginacao ligada, a CPU pode ter a traducao antiga no cache
     * (TLB); INVLPG descarta so a entrada desta pagina. */
    if (paging_active)
        __asm__ volatile("invlpg (%0)" : : "r"(virt) : "memory");

    return PAGING_OK;
}

paging_status_t paging_identity_map_range(uint32_t start, uint32_t size,
                                          uint32_t flags) {
    if (size == 0)
        return PAGING_ERR_INVALID_RANGE;

    /* uint64_t evita estouro de 32 bits ao somar start + size. */
    uint64_t first = align_down(start);
    uint64_t end = (uint64_t)start + size;
    end = (end + PAGING_PAGE_SIZE - 1) & PAGING_ADDR_MASK;

    if (end > 0x100000000ULL)
        return PAGING_ERR_INVALID_RANGE;

    for (uint64_t addr = first; addr < end; addr += PAGING_PAGE_SIZE) {
        paging_status_t st =
            paging_map_page((uint32_t)addr, (uint32_t)addr, flags);
        if (st != PAGING_OK)
            return st;
    }
    return PAGING_OK;
}

int paging_is_enabled(void) {
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    return (cr0 & CR0_PG_BIT) ? 1 : 0;
}

uint32_t paging_get_directory_address(void) {
    return (uint32_t)(uintptr_t)page_directory;
}

paging_status_t paging_init(uint32_t fb_addr, uint32_t fb_size) {
    if (paging_active || paging_is_enabled())
        return PAGING_ERR_ALREADY_ON;

    for (uint32_t i = 0; i < PAGING_ENTRIES; i++)
        page_directory[i] = 0;
    tables_used = 0;

    const uint32_t rw = PAGING_FLAG_WRITABLE;

    /* 1) Identity map da memoria baixa: cobre kernel (1 MB em diante), pilha,
     *    GDT/IDT, tabelas de paginacao e estruturas entregues pelo GRUB.
     *    Se o kernel crescer alem do minimo, estende ate o fim dele. */
    uint32_t low_end = PAGING_IDENTITY_MB * 1024U * 1024U;
    uint32_t kernel_end = (uint32_t)(uintptr_t)_kernel_end;
    if (kernel_end > low_end)
        low_end = kernel_end;

    paging_status_t st = paging_identity_map_range(0, low_end, rw);
    if (st != PAGING_OK)
        return st;

    /* 2) Framebuffer linear. Em modos VESA ele fica em endereco alto (perto
     *    de 3 GB), fora da faixa acima; se ficasse sem mapa, a primeira
     *    escrita de pixel apos ligar a paginacao causaria Page Fault. */
    if (fb_size != 0) {
        st = paging_identity_map_range(fb_addr, fb_size, rw);
        if (st != PAGING_OK)
            return st;
    }

    /* 3) Carrega o diretorio em CR3. Ainda sem efeito: a paginacao esta off. */
    __asm__ volatile("mov %0, %%cr3"
                     :
                     : "r"((uint32_t)(uintptr_t)page_directory)
                     : "memory");

    /* 4) Liga o bit PG (bit 31) de CR0. Dai em diante todo acesso a memoria
     *    passa pelas tabelas. Como virt == phys, a proxima instrucao e a
     *    pilha continuam validas. */
    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= CR0_PG_BIT;
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");

    paging_active = 1;
    return PAGING_OK;
}
