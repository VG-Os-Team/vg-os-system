#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "keyboard.h"
#include "pic/pic.h"
#include "shell.h"
#include "splash.h"
#include "include/panic.h"
#include <stdint.h>
#include <stddef.h>

#include "include/multiboot.h"
#include "include/pmm.h"
#include "include/paging.h"
#include "include/heap.h"
#include "include/video.h"

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

  splash_show(width, height);
  splash_progress(width, height, 15U, "Inicializando video VESA...");

  // 2. Inicializa o parser do mapa de memória Multiboot e PMM
  multiboot_mem_status_t mem_status = multiboot_mem_init(mbd);
  if (mem_status != MULTIBOOT_MEM_OK)
    kernel_panic("Falha ao ler o mapa de memoria Multiboot.", 0);

  pmm_init(mbd);
  splash_progress(width, height, 35U, "Configurando memoria fisica (PMM)...");

  // 3. Inicializa a GDT, IDT e PIC
  gdt_instalar();
  idt_install();
  isr_install();
  irq_install();
  pic_init();
  splash_progress(width, height, 55U, "Configurando GDT, IDT e PIC...");

  // 4. Ativa a paginação x86 (identity map do kernel e da VRAM do framebuffer)
  if (paging_init((uint32_t)(uintptr_t)fb, video_get_size_bytes()) != PAGING_OK)
    kernel_panic("Falha ao ativar a paginacao.", 0);
  splash_progress(width, height, 75U, "Ativando paginacao x86...");

  // 5. Heap dinâmico do kernel (kmalloc/kfree), sobre páginas pedidas ao PMM
  if (heap_init_from_pmm(HEAP_INITIAL_SIZE) != 0)
    kernel_panic("Falha ao inicializar o heap do kernel.", 0);
  splash_progress(width, height, 90U, "Inicializando heap do kernel...");

  // 6. Aloca o backbuffer na RAM para Double Buffering (Tarefa 16)
  vstatus = video_init_backbuffer();
  if (vstatus != VIDEO_OK)
    kernel_panic(video_status_str(vstatus), 0);

  splash_show(width, height);
  splash_progress(width, height, 100U, "Sistema pronto!");
  splash_finish();

  teclado_init();
  asm volatile("sti" ::: "memory");

  shell_run();
}
