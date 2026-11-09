#include "heap.h"
#include "paging.h"
#include "pmm.h"

#define ALIGNMENT       16
#define ALIGN_UP(x)     (((x) + (ALIGNMENT - 1)) & ~(size_t)(ALIGNMENT - 1))
#define ALIGN_DOWN(x)   ((x) & ~(size_t)(ALIGNMENT - 1))

#define MAGIC_USED      0xA110C8EDu
#define MAGIC_FREE      0xF4EEB10Cu

/* Cabecalho que precede cada bloco. A lista e ordenada por endereco e
 * os blocos sao contiguos, entao 'next' e sempre o vizinho fisico. */
typedef struct block {
    size_t        size;     /* tamanho do payload (sem o cabecalho) */
    uint32_t      magic;    /* MAGIC_USED ou MAGIC_FREE */
    struct block *next;
    struct block *prev;
} block_t;

#define HEADER_SIZE ALIGN_UP(sizeof(block_t))
/* So divide um bloco se sobrar espaco para um cabecalho + payload minimo. */
#define MIN_SPLIT   (HEADER_SIZE + ALIGNMENT)

static block_t *heap_head  = NULL;
static uintptr_t heap_lo   = 0;
static uintptr_t heap_hi   = 0;

void heap_init(void *start, size_t size)
{
    uintptr_t lo = ALIGN_UP((uintptr_t)start);
    uintptr_t hi = ALIGN_DOWN((uintptr_t)start + size);

    if (hi <= lo || (hi - lo) < MIN_SPLIT) {
        heap_head = NULL;
        return;
    }

    heap_lo = lo;
    heap_hi = hi;

    heap_head        = (block_t *)lo;
    heap_head->size  = (hi - lo) - HEADER_SIZE;
    heap_head->magic = MAGIC_FREE;
    heap_head->next  = NULL;
    heap_head->prev  = NULL;
}

void *kmalloc(size_t size)
{
    if (size == 0 || heap_head == NULL)
        return NULL;

    size_t need = ALIGN_UP(size);
    if (need < size)            /* overflow no alinhamento */
        return NULL;

    for (block_t *b = heap_head; b != NULL; b = b->next) {
        if (b->magic != MAGIC_FREE || b->size < need)
            continue;

        /* Divide o bloco se sobrar espaco util. */
        if (b->size >= need + MIN_SPLIT) {
            block_t *rest = (block_t *)((uint8_t *)b + HEADER_SIZE + need);
            rest->size  = b->size - need - HEADER_SIZE;
            rest->magic = MAGIC_FREE;
            rest->next  = b->next;
            rest->prev  = b;
            if (b->next)
                b->next->prev = rest;
            b->next = rest;
            b->size = need;
        }

        b->magic = MAGIC_USED;
        return (uint8_t *)b + HEADER_SIZE;
    }

    return NULL;
}

void kfree(void *ptr)
{
    if (ptr == NULL)
        return;

    uintptr_t addr = (uintptr_t)ptr;
    if (addr < heap_lo + HEADER_SIZE || addr >= heap_hi)
        return;                 /* ponteiro fora do heap */

    block_t *b = (block_t *)((uint8_t *)ptr - HEADER_SIZE);
    if (b->magic != MAGIC_USED)
        return;                 /* double free ou ponteiro invalido */

    b->magic = MAGIC_FREE;

    /* Junta com o proximo bloco, se estiver livre. */
    if (b->next && b->next->magic == MAGIC_FREE) {
        block_t *n = b->next;
        b->size += HEADER_SIZE + n->size;
        b->next  = n->next;
        if (n->next)
            n->next->prev = b;
        n->magic = 0;
    }

    /* Junta com o bloco anterior, se estiver livre. */
    if (b->prev && b->prev->magic == MAGIC_FREE) {
        block_t *p = b->prev;
        p->size += HEADER_SIZE + b->size;
        p->next  = b->next;
        if (b->next)
            b->next->prev = p;
        b->magic = 0;
    }
}

size_t heap_free_bytes(void)
{
    size_t total = 0;
    for (block_t *b = heap_head; b != NULL; b = b->next)
        if (b->magic == MAGIC_FREE)
            total += b->size;
    return total;
}

int heap_init_from_pmm(size_t size)
{
    if (size == 0)
        return -1;

    size_t pages = (size + PMM_BLOCK_SIZE - 1U) / PMM_BLOCK_SIZE;
    size_t bytes = pages * PMM_BLOCK_SIZE;

    void *mem = pmm_alloc_blocks(pages);
    if (mem == NULL)
        return -1;

    /* A regiao pode estar acima da faixa mapeada no boot (16 MB). */
    if (paging_identity_map_range((uint32_t)(uintptr_t)mem, (uint32_t)bytes,
                                  PAGING_FLAG_WRITABLE) != PAGING_OK) {
        pmm_free_blocks(mem, pages);
        return -1;
    }

    heap_init(mem, bytes);
    return heap_head != NULL ? 0 : -1;
}
