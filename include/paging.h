#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>

/*
 * Paginacao x86 (32 bits, paginas de 4 KB) do VG OS.
 *
 * Estrutura em dois niveis:
 *   CR3 -> Page Directory (1024 entradas, cada uma cobre 4 MB)
 *            -> Page Table (1024 entradas, cada uma cobre 4 KB)
 *
 * Neste estagio todo o mapeamento e "identity map": endereco virtual ==
 * endereco fisico. Assim, o codigo que ja existe (kernel em 1 MB, pilha,
 * GDT/IDT, framebuffer) continua funcionando no instante em que a paginacao
 * e ligada.
 */

/* Constantes de geometria da paginacao */
#define PAGING_PAGE_SIZE 4096U
#define PAGING_ENTRIES 1024U
#define PAGING_ADDR_MASK 0xFFFFF000U

/* Quantos MB do inicio da memoria fisica sao sempre mapeados (kernel,
 * estruturas do Multiboot, VGA legado). Se o kernel for maior que isso,
 * o mapeamento cresce automaticamente ate o fim do kernel. */
#define PAGING_IDENTITY_MB 16U

/* Quantidade maxima de Page Tables disponiveis (cada uma cobre 4 MB).
 * Sao alocadas de um pool estatico, pois a paginacao precisa existir antes
 * de qualquer alocador dinamico ser util. 16 tabelas = 64 MB de cobertura
 * possivel, mais que suficiente para kernel + framebuffer. */
#define PAGING_MAX_TABLES 16U

/* Flags de entrada (valem para Page Directory e Page Table) */
#define PAGING_FLAG_PRESENT 0x001U  /* pagina valida/mapeada */
#define PAGING_FLAG_WRITABLE 0x002U /* escrita permitida */
#define PAGING_FLAG_USER 0x004U     /* acessivel no anel 3 (nao usado ainda) */

/* Codigos de retorno das funcoes publicas */
typedef enum {
    PAGING_OK = 0,                /* operacao concluida */
    PAGING_ERR_UNALIGNED = -1,    /* endereco nao alinhado a 4 KB */
    PAGING_ERR_NO_TABLES = -2,    /* pool de Page Tables esgotado */
    PAGING_ERR_INVALID_RANGE = -3, /* faixa vazia ou acima de 4 GB */
    PAGING_ERR_ALREADY_ON = -4    /* paginacao ja foi ativada */
} paging_status_t;

/*
 * Monta o Page Directory, faz identity map de:
 *   - 0 ate max(PAGING_IDENTITY_MB, fim do kernel)
 *   - framebuffer: [fb_addr, fb_addr + fb_size)  (ignorado se fb_size == 0)
 * carrega o diretorio em CR3 e liga o bit PG em CR0.
 *
 * Deve ser chamada uma unica vez, depois de idt/isr instalados (assim um
 * erro de pagina vira kernel panic em vez de reinicio por Triple Fault).
 */
paging_status_t paging_init(uint32_t fb_addr, uint32_t fb_size);

/* Mapeia uma unica pagina de 4 KB: virt -> phys. Ambos alinhados a 4 KB. */
paging_status_t paging_map_page(uint32_t virt, uint32_t phys, uint32_t flags);

/* Identity map de [start, start + size). Nao exige alinhamento: a faixa e
 * expandida para cobrir paginas inteiras. */
paging_status_t paging_identity_map_range(uint32_t start, uint32_t size,
                                          uint32_t flags);

/* Retorna 1 se o bit PG de CR0 esta ligado, 0 caso contrario. */
int paging_is_enabled(void);

/* Endereco fisico do Page Directory (valor carregado em CR3). */
uint32_t paging_get_directory_address(void);

#endif /* PAGING_H */
