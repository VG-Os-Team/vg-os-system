#ifndef VIDEO_H
#define VIDEO_H

#include <stdint.h>
#include "multiboot.h"

/* Bit 12 do campo flags do Multiboot 1 indica tabela de framebuffer valida */
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO (1U << 12)

/* Tipo de framebuffer linear RGB (Multiboot 1) */
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB  1U

/* Resolucao alvo padrao da Tarefa 15 */
#define VIDEO_TARGET_WIDTH  1024U
#define VIDEO_TARGET_HEIGHT 768U
#define VIDEO_TARGET_BPP    32U

/* Tamanho por pixel e tamanho de referencia do backbuffer na RAM (Tarefa 16: 1024 * 768 * 4 bytes) */
#define VIDEO_BYTES_PER_PIXEL 4U
#define VIDEO_BACKBUFFER_SIZE (VIDEO_TARGET_WIDTH * VIDEO_TARGET_HEIGHT * VIDEO_BYTES_PER_PIXEL)

typedef enum {
    VIDEO_OK = 0,
    VIDEO_ERR_NULL_MBD,
    VIDEO_ERR_NO_FRAMEBUFFER,
    VIDEO_ERR_UNSUPPORTED_FORMAT,
    VIDEO_ERR_UNSUPPORTED_RGB,
    VIDEO_ERR_INVALID_GEOMETRY,
    VIDEO_ERR_NOT_INITIALIZED,
    VIDEO_ERR_BACKBUFFER_ALLOC
} video_status_t;

typedef struct {
    uint32_t *vram;             /* Ponteiro base mapeado da VRAM (32bpp) */
    uint32_t *backbuffer;       /* Ponteiro do backbuffer na RAM alocado via kmalloc */
    uint32_t phys_addr;         /* Endereco fisico base do framebuffer */
    uint32_t width;             /* Largura em pixels (ex: 1024) */
    uint32_t height;            /* Altura em pixels (ex: 768) */
    uint32_t pitch;             /* Bytes por linha (scanline) da VRAM */
    uint32_t backbuffer_pitch;  /* Bytes por linha no backbuffer (width * 4) */
    uint8_t  bpp;               /* Bits por pixel (32) */
    uint32_t size_bytes;        /* Tamanho total da VRAM em bytes (pitch * height) */
    uint32_t backbuffer_size;   /* Tamanho total do backbuffer na RAM (width * height * 4) */
} video_info_t;

/*
 * Lê e valida os campos de video da estrutura multiboot_info_t
 * e guarda o ponteiro base da VRAM e geometria.
 */
video_status_t video_init(const multiboot_info_t *mbd);

/*
 * Aloca na RAM (via kmalloc) o backbuffer equivalente ao tamanho da tela
 * (1024 * 768 * 4 bytes em 1024x768 32bpp, ou width * height * 4 da resolucao
 * ativa) e direciona todas as operacoes de desenho para o backbuffer (Tarefa 16).
 */
video_status_t video_init_backbuffer(void);

/* Libera o backbuffer da RAM via kfree e restaura o desenho para a VRAM */
void video_free_backbuffer(void);

/* Retorna ponteiro somente-leitura para as informacoes atuais do video */
const video_info_t *video_get_info(void);

/* Getters rapidos para integracao com o restante do kernel e renderizador */
uint32_t *video_get_framebuffer(void);
uint32_t  video_get_pitch(void);
uint32_t *video_get_backbuffer(void);
uint32_t  video_get_backbuffer_pitch(void);
uint32_t  video_get_backbuffer_size(void);
uint32_t *video_get_draw_buffer(void);
uint32_t  video_get_draw_pitch(void);
int       video_is_double_buffered(void);
uint32_t  video_get_width(void);
uint32_t  video_get_height(void);
uint8_t   video_get_bpp(void);
uint32_t  video_get_size_bytes(void);

/* Escrita de pixel em modo 32bpp (0x00RRGGBB) no buffer ativo (backbuffer) */
void video_put_pixel(uint32_t x, uint32_t y, uint32_t color);

/* Preenche uma regiao retangular no buffer ativo (backbuffer) */
void video_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

/* Limpa toda a tela no buffer ativo (backbuffer) com a cor informada */
void video_clear(uint32_t color);

/* Retorna string descritiva de erro de inicializacao */
const char *video_status_str(video_status_t status);

#endif /* VIDEO_H */