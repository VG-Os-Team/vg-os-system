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

typedef enum {
    VIDEO_OK = 0,
    VIDEO_ERR_NULL_MBD,
    VIDEO_ERR_NO_FRAMEBUFFER,
    VIDEO_ERR_UNSUPPORTED_FORMAT,
    VIDEO_ERR_UNSUPPORTED_RGB,
    VIDEO_ERR_INVALID_GEOMETRY
} video_status_t;

typedef struct {
    uint32_t *vram;        /* Ponteiro base mapeado da VRAM (32bpp) */
    uint32_t phys_addr;    /* Endereco fisico base do framebuffer */
    uint32_t width;        /* Largura em pixels (ex: 1024) */
    uint32_t height;       /* Altura em pixels (ex: 768) */
    uint32_t pitch;        /* Bytes por linha (scanline) */
    uint8_t  bpp;          /* Bits por pixel (32) */
    uint32_t size_bytes;   /* Tamanho total da VRAM em bytes (pitch * height) */
} video_info_t;

/*
 * Lê e valida os campos de video da estrutura multiboot_info_t
 * e guarda o ponteiro base da VRAM e geometria.
 */
video_status_t video_init(const multiboot_info_t *mbd);

/* Retorna ponteiro somente-leitura para as informacoes atuais do video */
const video_info_t *video_get_info(void);

/* Getters rapidos para integracao com o restante do kernel e renderizador */
uint32_t *video_get_framebuffer(void);
uint32_t  video_get_pitch(void);
uint32_t  video_get_width(void);
uint32_t  video_get_height(void);
uint8_t   video_get_bpp(void);
uint32_t  video_get_size_bytes(void);

/* Escrita direta de pixel em modo 32bpp (0x00RRGGBB) com checagem de limites */
void video_put_pixel(uint32_t x, uint32_t y, uint32_t color);

/* Preenche uma regiao retangular na VRAM */
void video_fill_rect(uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color);

/* Limpa toda a tela com a cor informada */
void video_clear(uint32_t color);

/* Retorna string descritiva de erro de inicializacao */
const char *video_status_str(video_status_t status);

#endif /* VIDEO_H */