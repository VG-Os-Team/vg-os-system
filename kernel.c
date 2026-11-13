#include "comandos.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "keyboard.h"
#include "layout.h"
#include "gfx.h"
#include "pic/pic.h"
#include "render.h"
#include "strutil.h"
#include "version.h"
#include "include/panic.h"
#include <stdint.h>
#include <stddef.h>

#include "include/multiboot.h"
#include "include/pmm.h"
#include "include/paging.h"
#include "include/heap.h"
#include "include/video.h"

#define LINHA_MAX 1024

static inline void outw(uint16_t port, uint16_t val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

// Copia provisoria do backbuffer (RAM) para a VRAM fisica ate a criacao
// de render_flush() na Tarefa 19, mantendo o terminal visivel e sem flicker.
static void sincronizar_vram_provisorio(void) {
  const uint32_t *src = video_get_backbuffer();
  uint32_t *dst = video_get_framebuffer();
  if (!src || !dst || src == dst)
    return;

  uint32_t width = video_get_width();
  uint32_t height = video_get_height();
  uint32_t src_pitch = video_get_backbuffer_pitch();
  uint32_t dst_pitch = video_get_pitch();

  if (src_pitch == dst_pitch && src_pitch == width * 4U) {
    uint32_t total_pixels = width * height;
    asm volatile("cld; rep movsl"
                 : "+S"(src), "+D"(dst), "+c"(total_pixels)
                 :
                 : "memory", "cc");
    return;
  }

  for (uint32_t y = 0; y < height; y++) {
    const uint32_t *linha_src =
        (const uint32_t *)((const uint8_t *)src + y * src_pitch);
    uint32_t *linha_dst = (uint32_t *)((uint8_t *)dst + y * dst_pitch);
    uint32_t cols = width;
    asm volatile("cld; rep movsl"
                 : "+S"(linha_src), "+D"(linha_dst), "+c"(cols)
                 :
                 : "memory", "cc");
  }
}

static const char prompt[] = "root@vgos:~# ";

typedef struct {
    layout_bounds_t area;
    int x, y, len;
    char linha[LINHA_MAX];
} entrada_t;

static void iniciar_prompt(entrada_t *e) {
    gfx_draw_string(prompt, e->area.x, e->y, LAYOUT_PROMPT);
    e->x = e->area.x + (sizeof(prompt) - 1) * LAYOUT_CELL_W;
    e->len = 0;
}

static void limpar_terminal(entrada_t *e) {
    video_fill_rect(e->area.x, e->area.y, e->area.width, e->area.height, LAYOUT_BG);
    e->y = e->area.y;
    iniciar_prompt(e);
}

static void marcar_cursor(const entrada_t *e, uint32_t cor) {
    video_fill_rect(e->x, e->y + LAYOUT_CELL_H - 2, LAYOUT_CELL_W, 2, cor);
}

static int inserir_char(entrada_t *e, char c) {
    /* Mantem toda a entrada editavel visivel, inclusive a celula do cursor. */
    int cells = (e->area.width / LAYOUT_CELL_W) * (e->area.height / LAYOUT_CELL_H);
    if (e->len >= LINHA_MAX - 1 || e->len + (int)sizeof(prompt) >= cells)
        return 0;
    desenhar_char_com_fundo(c, e->x, e->y, LAYOUT_TEXT, LAYOUT_BG,
                           video_get_draw_buffer(), video_get_draw_pitch());
    e->linha[e->len++] = c;
    e->x += LAYOUT_CELL_W;
    if (e->x == e->area.x + e->area.width) {
        e->x = e->area.x;
        layout_next_line(&e->y);
    }
    return 1;
}

static int apagar_char(entrada_t *e) {
    if (!e->len)
        return 0;
    marcar_cursor(e, LAYOUT_BG);
    if (e->x == e->area.x) {
        e->x += e->area.width;
        e->y -= LAYOUT_CELL_H;
    }
    e->x -= LAYOUT_CELL_W;
    e->len--;
    video_fill_rect(e->x, e->y, LAYOUT_CELL_W, LAYOUT_CELL_H, LAYOUT_BG);
    return 1;
}

static void executar_terminal(uint32_t width) {
    layout_draw();
    entrada_t entrada = { .area = layout_get_terminal_bounds() };
    if (!entrada.area.width)
        kernel_panic("Resolucao insuficiente para o terminal.", 0);
    entrada.y = entrada.area.y;
    iniciar_prompt(&entrada);
    marcar_cursor(&entrada, LAYOUT_ACCENT);
    sincronizar_vram_provisorio();

    char clipboard[LINHA_MAX];
    int clipboard_len = 0;
    while (1) {
        int tecla = teclado_ler();
        if (tecla == TECLA_NENHUMA) {
            asm volatile("pause");
            continue;
        }

        switch (tecla) {
        case TECLA_CTRL_L:
            limpar_terminal(&entrada);
            break;
        case TECLA_CTRL_C:
            for (int i = 0; i < entrada.len; i++)
                clipboard[i] = entrada.linha[i];
            clipboard_len = entrada.len;
            continue;
        case TECLA_CTRL_V: {
            int i = 0;
            while (i < clipboard_len && inserir_char(&entrada, clipboard[i]))
                i++;
            if (!i)
                continue;
            break;
        }
        case '\b':
            if (!apagar_char(&entrada))
                continue;
            break;
        case '\n': {
            marcar_cursor(&entrada, LAYOUT_BG);
            /* O cursor ja quebrou para a proxima linha ao preencher a anterior. */
            if (entrada.x == entrada.area.x)
                entrada.y -= LAYOUT_CELL_H;
            comando_resultado_t r = comandos_executar(
                entrada.linha, entrada.len, video_get_draw_buffer(),
                video_get_draw_pitch(), width, &entrada.y);
            if (r == COMANDO_LIMPAR) {
                limpar_terminal(&entrada);
            } else if (r == COMANDO_EXIT) {
                layout_write_line("Desligando o VG-OS...", LAYOUT_TEXT, &entrada.y);
                sincronizar_vram_provisorio();
                outw(0x604, 0x2000);
                outw(0xB004, 0x2000);
                outw(0x4004, 0x3400);
                layout_write_line("Sistema finalizado. Pode desligar o computador com seguranca.",
                                  LAYOUT_MUTED, &entrada.y);
                sincronizar_vram_provisorio();
                asm volatile("cli");
                while (1)
                    asm volatile("hlt");
            } else {
                layout_next_line(&entrada.y);
                iniciar_prompt(&entrada);
            }
            break;
        }
        default:
            if (tecla < 0 || !inserir_char(&entrada, (char)tecla))
                continue;
        }
        marcar_cursor(&entrada, LAYOUT_ACCENT);
        sincronizar_vram_provisorio();
    }
}

void kernel_main(multiboot_info_t *mbd) {
  // Mantém interrupções mascaráveis desabilitadas durante a inicialização
  asm volatile("cli" ::: "memory");

  // 1. Inicializa o driver de Framebuffer VESA (1024x768 32bpp)
  video_status_t vstatus = video_init(mbd);
  if (vstatus != VIDEO_OK)
    kernel_panic(video_status_str(vstatus), 0);

  uint32_t *fb    = video_get_framebuffer();
  uint32_t pitch  = video_get_pitch();
  uint32_t width  = video_get_width();
  uint32_t height = video_get_height();

  // O framebuffer só fica habilitado no panic se toda a geometria for válida.
  if (!panic_configurar_video(fb, pitch, width, height))
    kernel_panic("Geometria de framebuffer invalida.", 0);

  // 2. Inicializa o parser do mapa de memória Multiboot e PMM
  multiboot_mem_status_t mem_status = multiboot_mem_init(mbd);
  if (mem_status != MULTIBOOT_MEM_OK)
    kernel_panic("Falha ao ler o mapa de memoria Multiboot.", 0);

  pmm_init(mbd);

  // 3. Inicializa a GDT, IDT e PIC
  gdt_instalar();
  idt_install();
  isr_install();
  irq_install();
  pic_init();

  // 4. Ativa a paginação x86 (identity map do kernel e da VRAM do framebuffer)
  if (paging_init((uint32_t)(uintptr_t)fb, video_get_size_bytes()) != PAGING_OK)
    kernel_panic("Falha ao ativar a paginacao.", 0);

  // 5. Heap dinâmico do kernel (kmalloc/kfree), sobre páginas pedidas ao PMM
  if (heap_init_from_pmm(HEAP_INITIAL_SIZE) != 0)
    kernel_panic("Falha ao inicializar o heap do kernel.", 0);

  // 6. Aloca o backbuffer na RAM para Double Buffering (Tarefa 16)
  vstatus = video_init_backbuffer();
  if (vstatus != VIDEO_OK)
    kernel_panic(video_status_str(vstatus), 0);

  executar_terminal(width);
}
