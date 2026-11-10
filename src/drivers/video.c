#include "../../include/video.h"
#include "../../include/heap.h"
#include <stdint.h>
#include <stddef.h>

static video_info_t g_video_info;
static int g_video_initialized = 0;
static int g_double_buffering_enabled = 0;

video_status_t video_init(const multiboot_info_t *mbd) {
    if (mbd == NULL)
        return VIDEO_ERR_NULL_MBD;

    /* Verifica se o bootloader preencheu a tabela de framebuffer (bit 12) */
    if (!(mbd->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO))
        return VIDEO_ERR_NO_FRAMEBUFFER;

    /* Valida endereco fisico em 32 bits, 32bpp e modo linear RGB (tipo 1) */
    if (mbd->framebuffer_addr == 0 ||
        mbd->framebuffer_addr > UINT32_MAX ||
        mbd->framebuffer_bpp != VIDEO_TARGET_BPP ||
        mbd->framebuffer_type != MULTIBOOT_FRAMEBUFFER_TYPE_RGB) {
        return VIDEO_ERR_UNSUPPORTED_FORMAT;
    }

    /*
     * Valida layout RGB 0x00RRGGBB:
     * Red:   pos 16, size 8
     * Green: pos 8,  size 8
     * Blue:  pos 0,  size 8
     */
    if (mbd->color_info[0] != 16 || mbd->color_info[1] != 8 ||
        mbd->color_info[2] != 8  || mbd->color_info[3] != 8 ||
        mbd->color_info[4] != 0  || mbd->color_info[5] != 8) {
        return VIDEO_ERR_UNSUPPORTED_RGB;
    }

    /* Valida geometria e alinhamento de 4 bytes do pitch */
    if (mbd->framebuffer_width == 0 ||
        mbd->framebuffer_height == 0 ||
        mbd->framebuffer_pitch < (mbd->framebuffer_width * 4U) ||
        (mbd->framebuffer_pitch % 4U) != 0) {
        return VIDEO_ERR_INVALID_GEOMETRY;
    }

    /* Verifica se pitch * height nao estoura 32 bits */
    uint64_t total_bytes = (uint64_t)mbd->framebuffer_pitch *
                           (uint64_t)mbd->framebuffer_height;
    if (total_bytes == 0 || total_bytes > UINT32_MAX)
        return VIDEO_ERR_INVALID_GEOMETRY;

    g_video_info.phys_addr        = (uint32_t)mbd->framebuffer_addr;
    g_video_info.vram             = (uint32_t *)(uintptr_t)g_video_info.phys_addr;
    g_video_info.backbuffer       = NULL;
    g_video_info.width            = mbd->framebuffer_width;
    g_video_info.height           = mbd->framebuffer_height;
    g_video_info.pitch            = mbd->framebuffer_pitch;
    g_video_info.backbuffer_pitch = 0;
    g_video_info.bpp              = mbd->framebuffer_bpp;
    g_video_info.size_bytes       = (uint32_t)total_bytes;
    g_video_info.backbuffer_size  = 0;

    g_video_initialized = 1;
    g_double_buffering_enabled = 0;
    return VIDEO_OK;
}

video_status_t video_init_backbuffer(void) {
    if (!g_video_initialized)
        return VIDEO_ERR_NOT_INITIALIZED;

    /*
     * Calcula o tamanho em bytes equivalente a tela (1024 * 768 * 4 bytes
     * no modo padrao 1024x768 32bpp, ou width * height * 4 da resolucao ativa).
     */
    uint64_t bb_bytes = (uint64_t)g_video_info.width *
                        (uint64_t)g_video_info.height *
                        (uint64_t)VIDEO_BYTES_PER_PIXEL;
    if (bb_bytes == 0 || bb_bytes > UINT32_MAX)
        return VIDEO_ERR_INVALID_GEOMETRY;

    uint32_t alloc_size = (uint32_t)bb_bytes;

    if (g_video_info.backbuffer != NULL) {
        if (g_video_info.backbuffer_size == alloc_size) {
            g_double_buffering_enabled = 1;
            video_clear(0x00000000);
            return VIDEO_OK;
        }
        video_free_backbuffer();
    }

    uint32_t *buf = (uint32_t *)kmalloc((size_t)alloc_size);
    if (buf == NULL)
        return VIDEO_ERR_BACKBUFFER_ALLOC;

    g_video_info.backbuffer       = buf;
    g_video_info.backbuffer_pitch = g_video_info.width * VIDEO_BYTES_PER_PIXEL;
    g_video_info.backbuffer_size  = alloc_size;
    g_double_buffering_enabled    = 1;

    /* Zera todo o backbuffer recem-alocado na RAM */
    video_clear(0x00000000);
    return VIDEO_OK;
}

void video_free_backbuffer(void) {
    if (g_video_info.backbuffer != NULL) {
        kfree(g_video_info.backbuffer);
        g_video_info.backbuffer = NULL;
    }
    g_video_info.backbuffer_pitch = 0;
    g_video_info.backbuffer_size  = 0;
    g_double_buffering_enabled    = 0;
}

const video_info_t *video_get_info(void) {
    return g_video_initialized ? &g_video_info : NULL;
}

uint32_t *video_get_framebuffer(void) {
    return g_video_info.vram;
}

uint32_t video_get_pitch(void) {
    return g_video_info.pitch;
}

uint32_t *video_get_backbuffer(void) {
    return g_video_info.backbuffer;
}

uint32_t video_get_backbuffer_pitch(void) {
    return g_video_info.backbuffer_pitch;
}

uint32_t video_get_backbuffer_size(void) {
    return g_video_info.backbuffer_size;
}

uint32_t *video_get_draw_buffer(void) {
    if (g_double_buffering_enabled && g_video_info.backbuffer != NULL)
        return g_video_info.backbuffer;
    return g_video_info.vram;
}

uint32_t video_get_draw_pitch(void) {
    if (g_double_buffering_enabled && g_video_info.backbuffer != NULL)
        return g_video_info.backbuffer_pitch;
    return g_video_info.pitch;
}

int video_is_double_buffered(void) {
    return g_double_buffering_enabled && (g_video_info.backbuffer != NULL);
}

uint32_t video_get_width(void) {
    return g_video_info.width;
}

uint32_t video_get_height(void) {
    return g_video_info.height;
}

uint8_t video_get_bpp(void) {
    return g_video_info.bpp;
}

uint32_t video_get_size_bytes(void) {
    return g_video_info.size_bytes;
}

void video_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (!g_video_initialized)
        return;
    if (x >= g_video_info.width || y >= g_video_info.height)
        return;

    uint32_t *target = video_get_draw_buffer();
    uint32_t stride = video_get_draw_pitch() >> 2; /* pitch / 4 */
    target[y * stride + x] = color;
}

void video_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    if (!g_video_initialized || w == 0 || h == 0)
        return;
    if (x >= g_video_info.width || y >= g_video_info.height)
        return;

    uint32_t end_x = x + w;
    uint32_t end_y = y + h;
    if (end_x > g_video_info.width)
        end_x = g_video_info.width;
    if (end_y > g_video_info.height)
        end_y = g_video_info.height;

    uint32_t *target = video_get_draw_buffer();
    uint32_t stride = video_get_draw_pitch() >> 2;
    for (uint32_t py = y; py < end_y; py++) {
        uint32_t *row = &target[py * stride];
        for (uint32_t px = x; px < end_x; px++) {
            row[px] = color;
        }
    }
}

void video_clear(uint32_t color) {
    if (!g_video_initialized)
        return;
    video_fill_rect(0, 0, g_video_info.width, g_video_info.height, color);
}

const char *video_status_str(video_status_t status) {
    switch (status) {
    case VIDEO_OK:
        return "OK";
    case VIDEO_ERR_NULL_MBD:
        return "Ponteiro Multiboot nulo.";
    case VIDEO_ERR_NO_FRAMEBUFFER:
        return "Boot sem framebuffer disponivel.";
    case VIDEO_ERR_UNSUPPORTED_FORMAT:
        return "Formato de framebuffer nao suportado.";
    case VIDEO_ERR_UNSUPPORTED_RGB:
        return "Layout RGB de framebuffer nao suportado.";
    case VIDEO_ERR_INVALID_GEOMETRY:
        return "Geometria de framebuffer invalida.";
    case VIDEO_ERR_NOT_INITIALIZED:
        return "Driver de video nao inicializado.";
    case VIDEO_ERR_BACKBUFFER_ALLOC:
        return "Falha ao alocar o backbuffer na RAM.";
    default:
        return "Erro desconhecido de video.";
    }
}