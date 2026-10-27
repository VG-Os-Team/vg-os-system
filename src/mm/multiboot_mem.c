
#include "multiboot.h"

#include <stdint.h>

static multiboot_mem_info_t mem_info;
static int mem_info_initialized = 0;

static int add_u64(uint64_t *total, uint64_t value)
{
    if (UINT64_MAX - *total < value) {
        return 0;
    }

    *total += value;
    return 1;
}

multiboot_mem_status_t multiboot_mem_parse(
    const multiboot_info_t *mbd,
    multiboot_mem_info_t *out)
{
    if (mbd == 0 || out == 0) {
        return MULTIBOOT_MEM_ERR_NULL_POINTER;
    }

    out->available_bytes = 0;
    out->reserved_bytes = 0;
    out->region_count = 0;

    if ((mbd->flags & MULTIBOOT_INFO_MMAP) == 0) {
        return MULTIBOOT_MEM_ERR_NO_MMAP;
    }

    if (mbd->mmap_length == 0 || mbd->mmap_addr == 0) {
        return MULTIBOOT_MEM_ERR_INVALID_MAP;
    }

    const uint8_t *map = (const uint8_t *)(uintptr_t)mbd->mmap_addr;
    uint32_t offset = 0;

    while (offset < mbd->mmap_length) {
        uint32_t remaining = mbd->mmap_length - offset;

        if (remaining < sizeof(uint32_t)) {
            return MULTIBOOT_MEM_ERR_MALFORMED;
        }

        const multiboot_mmap_entry_t *entry =
            (const multiboot_mmap_entry_t *)(map + offset);

        uint32_t entry_size = entry->size;

        if (entry_size < MULTIBOOT_MMAP_ENTRY_MIN_SIZE) {
            return MULTIBOOT_MEM_ERR_MALFORMED;
        }

        if (entry_size > remaining - sizeof(uint32_t)) {
            return MULTIBOOT_MEM_ERR_MALFORMED;
        }

        if (out->region_count == UINT32_MAX) {
            return MULTIBOOT_MEM_ERR_OVERFLOW;
        }

        if (entry->type == MULTIBOOT_MEMORY_AVAILABLE) {
            if (!add_u64(&out->available_bytes, entry->len)) {
                return MULTIBOOT_MEM_ERR_OVERFLOW;
            }
        } else {
            if (!add_u64(&out->reserved_bytes, entry->len)) {
                return MULTIBOOT_MEM_ERR_OVERFLOW;
            }
        }

        out->region_count++;

        offset += entry_size + sizeof(uint32_t);
    }

    return MULTIBOOT_MEM_OK;
}

multiboot_mem_status_t multiboot_mem_init(
    const multiboot_info_t *mbd)
{
    multiboot_mem_status_t status =
        multiboot_mem_parse(mbd, &mem_info);

    mem_info_initialized = (status == MULTIBOOT_MEM_OK);

    return status;
}

const multiboot_mem_info_t *multiboot_mem_get_info(void)
{
    if (!mem_info_initialized) {
        return 0;
    }

    return &mem_info;
}

const char *multiboot_mem_status_str(multiboot_mem_status_t status)
{
    switch (status) {
        case MULTIBOOT_MEM_OK:
            return "Sucesso";

        case MULTIBOOT_MEM_ERR_NULL_POINTER:
            return "Ponteiro nulo";

        case MULTIBOOT_MEM_ERR_NO_MMAP:
            return "Mapa de memoria indisponivel";

        case MULTIBOOT_MEM_ERR_INVALID_MAP:
            return "Mapa de memoria invalido";

        case MULTIBOOT_MEM_ERR_MALFORMED:
            return "Entrada do mapa malformada";

        case MULTIBOOT_MEM_ERR_OVERFLOW:
            return "Overflow ao calcular memoria";

        default:
            return "Erro desconhecido";
    }
}
