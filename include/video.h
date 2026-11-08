#ifndef VIDEO_H
#define VIDEO_H

#include <stddef.h>
#include <stdint.h>

/*
 * Subsistema de Video VESA e Double Buffering do VG OS.
 *
 * Arquitetura em duas camadas de memoria:
 *   1) Frontbuffer (VRAM fisica): framebuffer linear VESA entregue pelo
 *      Multiboot/GRUB (configurado pela Tarefa 15 / kernel_main).
 *   2) Backbuffer (RAM): buffer secundario do tamanho da tela
 *      (1024 * 768 * 4 bytes no modo padrao 1024x768 32bpp), alocado
 *      dinamicamente via kmalloc() (Tarefa 16).
 *
 * Todas as operacoes de desenho sao direcionadas ao backbuffer na RAM
 * atraves de video_get_draw_buffer() e video_get_draw_pitch(), evitando
 * cintilacao (flicker) causada por escritas parciais diretas na VRAM.
 * A copia final do backbuffer para a VRAM cabe a rotina render_flush()
 * (Tarefa 19).
 */

/* Geometria de referencia do modo VESA 1024x768 32bpp */
#define VIDEO_DEFAULT_WIDTH    1024U
#define VIDEO_DEFAULT_HEIGHT   768U
#define VIDEO_BPP              32U
#define VIDEO_BYTES_PER_PIXEL  4U

/* Tamanho em bytes do backbuffer no modo padrao 1024x768x32bpp (3 MB) */
#define VIDEO_BACKBUFFER_SIZE  \
    (VIDEO_DEFAULT_WIDTH * VIDEO_DEFAULT_HEIGHT * VIDEO_BYTES_PER_PIXEL)

/* Codigos de status retornados pelo driver de video */
typedef enum {
    VIDEO_OK              =  0,
    VIDEO_ERR_INVALID_ARG = -1,
    VIDEO_ERR_OVERFLOW    = -2,
    VIDEO_ERR_NO_MEMORY   = -3,
    VIDEO_ERR_NOT_READY   = -4
} video_status_t;

/*
 * Registra o ponteiro base da VRAM fisica e a geometria ativa da tela.
 * Deixa o ponto de integracao pronto para a inicializacao VESA (Tarefa 15).
 * Se ja existir um backbuffer com tamanho diferente da nova geometria,
 * ele e realocado automaticamente para manter a consistencia.
 */
video_status_t video_set_vram(uint32_t *vram, uint32_t pitch,
                              uint32_t width, uint32_t height, uint8_t bpp);

/*
 * Aloca o backbuffer na RAM via kmalloc() com tamanho equivalente a tela
 * ativa (width * height * 4 bytes; ou 1024 * 768 * 4 bytes por padrao caso
 * nenhuma geometria tenha sido registrada antes), zera toda a regiao e
 * direciona as operacoes de desenho para o backbuffer.
 *
 * Deve ser chamada apos heap_init_from_pmm().
 */
video_status_t video_backbuffer_init(void);

/* Libera o backbuffer da RAM via kfree() e restaura o desenho para a VRAM. */
void video_backbuffer_free(void);

/* Preenche todo o buffer de desenho ativo (backbuffer) com a cor 0x00RRGGBB. */
void video_backbuffer_clear(uint32_t color);

/* Escreve um pixel 0x00RRGGBB em (x, y) no buffer de desenho ativo (backbuffer). */
void video_put_pixel(uint32_t x, uint32_t y, uint32_t color);

/*
 * Alvo ativo de renderizacao: devolve o ponteiro do backbuffer na RAM quando
 * o Double Buffering esta ativo (ou a VRAM fisica como fallback seguro).
 */
uint32_t *video_get_draw_buffer(void);
uint32_t  video_get_draw_pitch(void);

/* Consultas diretas aos buffers (usadas pelas primitivas e pelo render_flush) */
uint32_t *video_get_backbuffer(void);
uint32_t  video_get_backbuffer_pitch(void);
size_t    video_get_backbuffer_size(void);

uint32_t *video_get_frontbuffer(void);
uint32_t  video_get_vram_pitch(void);

/* Consultas de geometria e estado */
uint32_t  video_get_width(void);
uint32_t  video_get_height(void);
uint8_t   video_get_bpp(void);
int       video_is_double_buffered(void);

#endif /* VIDEO_H */
