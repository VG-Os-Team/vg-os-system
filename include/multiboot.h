
#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stddef.h>
#include <stdint.h>

/* Flags da estrutura Multiboot 1 */
#define MULTIBOOT_INFO_MEMORY (1U << 0)
#define MULTIBOOT_INFO_MMAP   (1U << 6)

/* Tipos de regioes do mapa de memoria */
#define MULTIBOOT_MEMORY_AVAILABLE          1U
#define MULTIBOOT_MEMORY_RESERVED           2U
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE   3U
#define MULTIBOOT_MEMORY_NVS                4U
#define MULTIBOOT_MEMORY_BADRAM             5U

/*
 * Estrutura recebida do GRUB.
 * Preserva os offsets do framebuffer usados pelo VGOS atual.
 */
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;
    uint8_t framebuffer_padding[2];
    uint8_t color_info[6];
} __attribute__((packed)) multiboot_info_t;

/* Verifica os offsets utilizados pelo kernel */
_Static_assert(offsetof(multiboot_info_t, mmap_length) == 44,
               "Offset de mmap_length incorreto");
_Static_assert(offsetof(multiboot_info_t, mmap_addr) == 48,
               "Offset de mmap_addr incorreto");
_Static_assert(offsetof(multiboot_info_t, framebuffer_addr) == 88,
               "Offset de framebuffer_addr incorreto");
_Static_assert(offsetof(multiboot_info_t, framebuffer_pitch) == 96,
               "Offset de framebuffer_pitch incorreto");
_Static_assert(offsetof(multiboot_info_t, framebuffer_width) == 100,
               "Offset de framebuffer_width incorreto");
_Static_assert(offsetof(multiboot_info_t, framebuffer_height) == 104,
               "Offset de framebuffer_height incorreto");
_Static_assert(offsetof(multiboot_info_t, framebuffer_bpp) == 108,
               "Offset de framebuffer_bpp incorreto");
_Static_assert(offsetof(multiboot_info_t, framebuffer_type) == 109,
               "Offset de framebuffer_type incorreto");
_Static_assert(offsetof(multiboot_info_t, framebuffer_padding) == 110,
               "Offset de framebuffer_padding incorreto");
_Static_assert(offsetof(multiboot_info_t, color_info) == 112,
               "Offset de color_info incorreto");

               /* Entrada individual do mapa de memoria */
typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry_t;

#define MULTIBOOT_MMAP_ENTRY_MIN_SIZE 20U

_Static_assert(sizeof(multiboot_mmap_entry_t) - sizeof(uint32_t) ==
                   MULTIBOOT_MMAP_ENTRY_MIN_SIZE,
               "Tamanho minimo da entrada mmap incorreto");

/* Resultado da analise do mapa, em bytes */
typedef struct {
    uint64_t available_bytes;
    uint64_t reserved_bytes;
    uint32_t region_count;
} multiboot_mem_info_t;

/* Resultados possiveis do parser */
typedef enum {
    MULTIBOOT_MEM_OK = 0,
    MULTIBOOT_MEM_ERR_NULL_POINTER,
    MULTIBOOT_MEM_ERR_NO_MMAP,
    MULTIBOOT_MEM_ERR_INVALID_MAP,
    MULTIBOOT_MEM_ERR_MALFORMED,
    MULTIBOOT_MEM_ERR_OVERFLOW
} multiboot_mem_status_t;

/* Analisa o mapa sem guardar estado global */
multiboot_mem_status_t multiboot_mem_parse(
    const multiboot_info_t *mbd,
    multiboot_mem_info_t *out
);

/* Inicializa e guarda o resultado para outros modulos */
multiboot_mem_status_t multiboot_mem_init(
    const multiboot_info_t *mbd
);

/* Consulta o resultado da inicializacao */
const multiboot_mem_info_t *multiboot_mem_get_info(void);

/* Retorna uma descricao do resultado */
const char *multiboot_mem_status_str(
    multiboot_mem_status_t status
);

#endif
