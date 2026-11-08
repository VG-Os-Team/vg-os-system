#include "video.h"
#include "heap.h"

/*
 * Estado interno do driver de video.
 *
 * Mantem separados o frontbuffer fisico (VRAM mapeada) e o backbuffer em RAM
 * (alocado via kmalloc), deixando a estrutura pronta tanto para a Tarefa 15
 * (inicializacao VESA a partir do Multiboot) quanto para as Tarefas 17-19
 * (primitivas graficas, layout de UI e render_flush).
 */
typedef struct {
    uint32_t *vram;             /* Ponteiro base da VRAM fisica (frontbuffer) */
    uint32_t  vram_pitch;       /* Pitch da VRAM fisica em bytes */
    uint32_t *backbuffer;       /* Buffer secundario na RAM (via kmalloc) */
    uint32_t  backbuffer_pitch; /* Pitch do backbuffer em bytes (width * 4) */
    size_t    backbuffer_size;  /* Tamanho alocado do backbuffer em bytes */
    uint32_t  width;            /* Largura da tela em pixels */
    uint32_t  height;           /* Altura da tela em pixels */
    uint8_t   bpp;              /* Bits por pixel (32) */
    int       double_buffered;  /* 1 se o desenho esta direcionado ao backbuffer */
} video_state_t;

static video_state_t g_video;

static void preencher_u32(uint32_t *dst, size_t count, uint32_t value) {
    if (!dst || count == 0)
        return;
#if defined(__i386__)
    __asm__ volatile("cld; rep stosl"
                     : "+D"(dst), "+c"(count)
                     : "a"(value)
                     : "memory", "cc");
#else
    for (size_t i = 0; i < count; i++)
        dst[i] = value;
#endif
}

static video_status_t calcular_bytes_tela(uint32_t width, uint32_t height,
                                          size_t *out_row_bytes,
                                          size_t *out_total_bytes) {
    if (width == 0 || height == 0)
        return VIDEO_ERR_INVALID_ARG;

    size_t w = width;
    size_t h = height;

    if (w > SIZE_MAX / VIDEO_BYTES_PER_PIXEL)
        return VIDEO_ERR_OVERFLOW;

    size_t row_bytes = w * VIDEO_BYTES_PER_PIXEL;
    if (h > SIZE_MAX / row_bytes)
        return VIDEO_ERR_OVERFLOW;

    *out_row_bytes   = row_bytes;
    *out_total_bytes = row_bytes * h;
    return VIDEO_OK;
}

video_status_t video_set_vram(uint32_t *vram, uint32_t pitch,
                              uint32_t width, uint32_t height, uint8_t bpp) {
    if (!vram || ((uintptr_t)vram % VIDEO_BYTES_PER_PIXEL) != 0)
        return VIDEO_ERR_INVALID_ARG;

    if (width == 0 || height == 0 || bpp != VIDEO_BPP)
        return VIDEO_ERR_INVALID_ARG;

    if (pitch == 0 || (pitch % VIDEO_BYTES_PER_PIXEL) != 0 ||
        (pitch / VIDEO_BYTES_PER_PIXEL) < width)
        return VIDEO_ERR_INVALID_ARG;

    if (height > UINT32_MAX / pitch)
        return VIDEO_ERR_OVERFLOW;

    uint32_t vram_bytes = pitch * height;
    if ((uintptr_t)vram > UINTPTR_MAX - vram_bytes)
        return VIDEO_ERR_OVERFLOW;

    size_t row_bytes = 0;
    size_t expected_backbuffer_bytes = 0;
    video_status_t st = calcular_bytes_tela(width, height,
                                            &row_bytes,
                                            &expected_backbuffer_bytes);
    if (st != VIDEO_OK)
        return st;

    g_video.vram       = vram;
    g_video.vram_pitch = pitch;
    g_video.width      = width;
    g_video.height     = height;
    g_video.bpp        = bpp;

    /* Se o backbuffer ja havia sido alocado antes (ex.: com 1024x768 padrao)
     * e a geometria real mudou, realoca para casar exatamente com a tela. */
    if (g_video.backbuffer != NULL) {
        if (g_video.backbuffer_size != expected_backbuffer_bytes) {
            video_backbuffer_free();
            return video_backbuffer_init();
        }
        g_video.backbuffer_pitch = (uint32_t)row_bytes;
    }

    return VIDEO_OK;
}

video_status_t video_backbuffer_init(void) {
    uint32_t width  = (g_video.width  != 0) ? g_video.width  : VIDEO_DEFAULT_WIDTH;
    uint32_t height = (g_video.height != 0) ? g_video.height : VIDEO_DEFAULT_HEIGHT;

    size_t row_bytes = 0;
    size_t total_bytes = 0;
    video_status_t st = calcular_bytes_tela(width, height, &row_bytes, &total_bytes);
    if (st != VIDEO_OK)
        return st;

    /* Reaproveita o buffer caso ja esteja alocado na mesma resolucao */
    if (g_video.backbuffer != NULL) {
        if (g_video.backbuffer_size == total_bytes) {
            g_video.width            = width;
            g_video.height           = height;
            if (g_video.bpp == 0)
                g_video.bpp          = VIDEO_BPP;
            g_video.backbuffer_pitch = (uint32_t)row_bytes;
            g_video.double_buffered  = 1;
            preencher_u32(g_video.backbuffer, (size_t)width * (size_t)height, 0x00000000U);
            return VIDEO_OK;
        }
        video_backbuffer_free();
    }

    uint32_t *buf = (uint32_t *)kmalloc(total_bytes);
    if (buf == NULL)
        return VIDEO_ERR_NO_MEMORY;

    g_video.width            = width;
    g_video.height           = height;
    if (g_video.bpp == 0)
        g_video.bpp          = VIDEO_BPP;
    g_video.backbuffer       = buf;
    g_video.backbuffer_pitch = (uint32_t)row_bytes;
    g_video.backbuffer_size  = total_bytes;
    g_video.double_buffered  = 1;

    preencher_u32(g_video.backbuffer, (size_t)width * (size_t)height, 0x00000000U);
    return VIDEO_OK;
}

void video_backbuffer_free(void) {
    if (g_video.backbuffer != NULL) {
        kfree(g_video.backbuffer);
        g_video.backbuffer = NULL;
    }
    g_video.backbuffer_pitch = 0;
    g_video.backbuffer_size  = 0;
    g_video.double_buffered  = 0;
}

void video_backbuffer_clear(uint32_t color) {
    if (g_video.double_buffered && g_video.backbuffer != NULL) {
        size_t total_pixels = (size_t)g_video.width * (size_t)g_video.height;
        preencher_u32(g_video.backbuffer, total_pixels, color);
        return;
    }

    if (g_video.vram != NULL) {
        uint32_t stride = g_video.vram_pitch / VIDEO_BYTES_PER_PIXEL;
        for (uint32_t y = 0; y < g_video.height; y++)
            preencher_u32(g_video.vram + (size_t)y * stride, g_video.width, color);
    }
}

void video_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
    if (x >= g_video.width || y >= g_video.height)
        return;

    uint32_t *target = video_get_draw_buffer();
    uint32_t  pitch  = video_get_draw_pitch();
    if (!target || pitch == 0)
        return;

    target[(y * (pitch / VIDEO_BYTES_PER_PIXEL)) + x] = color;
}

uint32_t *video_get_draw_buffer(void) {
    if (g_video.double_buffered && g_video.backbuffer != NULL)
        return g_video.backbuffer;
    return g_video.vram;
}

uint32_t video_get_draw_pitch(void) {
    if (g_video.double_buffered && g_video.backbuffer != NULL)
        return g_video.backbuffer_pitch;
    return g_video.vram_pitch;
}

uint32_t *video_get_backbuffer(void) {
    return g_video.backbuffer;
}

uint32_t video_get_backbuffer_pitch(void) {
    return g_video.backbuffer_pitch;
}

size_t video_get_backbuffer_size(void) {
    return g_video.backbuffer_size;
}

uint32_t *video_get_frontbuffer(void) {
    return g_video.vram;
}

uint32_t video_get_vram_pitch(void) {
    return g_video.vram_pitch;
}

uint32_t video_get_width(void) {
    return g_video.width;
}

uint32_t video_get_height(void) {
    return g_video.height;
}

uint8_t video_get_bpp(void) {
    return g_video.bpp;
}

int video_is_double_buffered(void) {
    return g_video.double_buffered && (g_video.backbuffer != NULL);
}
