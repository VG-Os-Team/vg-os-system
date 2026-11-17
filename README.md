![C Language](https://img.shields.io/badge/Language-C-%2300599C?style=for-the-badge&logo=c&logoColor=white)
![Assembly](https://img.shields.io/badge/Arch-x86_Assembly-%23E34F26?style=for-the-badge&logo=assemblyscript&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Bare_Metal_%2F_QEMU-%2341B883?style=for-the-badge&logo=linux&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)

<p align="center">
  <a href="README.md"><b>🇧🇷 Português</b></a> • <a href="README.en.md">🇺🇸 English</a>
</p>

# VG OS 

Um sistema operacional x86 de 32 bits desenvolvido do zero para fins acadêmicos e de aprendizado em baixo nível. O sistema conta com inicialização via Multiboot/GRUB, kernel próprio em Modo Protegido e um terminal gráfico interativo com renderização direta em Framebuffer linear.
---
## 📋 Funcionalidades Principais
* **Kernel Próprio (Modo Protegido 32-bit):** Escrito em C e Assembly x86 com gerenciamento manual de memória, interrupções e periféricos.
* **GDT, IDT, PIC e TSS:** GDT com 6 descritores, TSS de 32 bits carregado por `LTR`, IDT com 32 exceções da CPU e 16 linhas de IRQ via PIC 8259A remapeado (com detecção de IRQs espúrias e controle individual de máscara).
* **Kernel Panic:** Tela de erro fatal com mensagem, registradores salvos na exceção e parada em `CLI`/`HLT`, integrada aos 32 tratadores de exceção. O comando `panic` provoca `UD2` para demonstração.
* **Bootloader Multiboot (GRUB):** Seleção de resolução no menu de boot (`1920×1080`, `1440×1080`, `1024×768`) inicializando o modo gráfico VESA diretamente no boot.
* **Tela de Carregamento (Splash Screen):** Exibição da logo oficial do VG OS, versão (`OS_BANNER`) e barra de progresso acompanhando as etapas reais de inicialização do kernel.
* **Saída Gráfica em Framebuffer Linear (VESA), Double Buffering & Primitivas:**
  - Alocação de *backbuffer* na RAM (`video_init_backbuffer`) e cópia otimizada em 32 bits (`rep movsl`) para a VRAM via `render_flush()`, preservando *padding* de *pitch*.
  - Primitivas gráficas de desenho (`gfx.c`): pixels, retângulos, linhas e strings com fonte bitmap escalonada (2x).
  - Layout gráfico modular (`src/ui/layout.c`) com cabeçalho, janela de terminal com rolagem e rodapé responsivo exibindo status do backbuffer e atalhos.
* **Driver de Teclado PS/2 por Interrupção (IRQ1):**
  - Handler registrado na **IRQ1** lendo scancodes (Set 1) da porta `0x60` e enfileirando em buffer circular, com espera de baixo consumo (`sti; hlt`).
  - Suporte ao padrão **ABNT2 brasileiro** (incluindo `ç` / `Ç`, `/ ?` e *dead keys* de acentuação `´`, `` ` ``, `~`, `^`).
  - Suporte a scancodes estendidos (`0xE0`: setas direcionais, Delete, Home, End, Ctrl Direito) e atalhos (`Ctrl+L`, `Ctrl+C`, `Ctrl+V`).
  - Tratamento de Shift, Caps Lock, Enter, Backspace e repetição de teclas.
* **Gerenciamento de Memória (PMM, Paginação x86 e Heap):**
  - **PMM (4KB Bitmap):** Alocador de páginas físicas inicializado pelo mapa de memória Multiboot, protegendo baixa memória, kernel, framebuffer e estruturas do Multiboot.
  - **Paginação x86:** *Identity map* do kernel e da VRAM do framebuffer.
  - **Heap Dinâmico:** Alocador `kmalloc` / `kfree` construído sobre páginas fornecidas pelo PMM.
* **Terminal Interativo Integrado:**
  - Comandos embutidos: `help`, `info` (exibe logo e métricas de sistema, vídeo, RAM e Heap), `version`, `clear`, `exit`, `gdt`, `pmm`, `panic`.
  - Quebra automática de linha (*line wrap*), rolagem e Backspace multilinha.
* **Arquitetura Modular & Testes:**
  - Centralização de versão do sistema (`version.h`).
  - Testes nativos isolados para o driver de vídeo (`make test-video` em `tests/render_flush_test.c`).
  - Saída de compilação isolada no diretório `build/`.
---
## 🛠️ Tecnologias e Ferramentas
* **Linguagens:** C (padrão C99 freestanding) e Assembly x86 (GNU Assembler/ NASM)
* **Compilador / Linker:** GCC (`-m32`, `-ffreestanding`) e GNU LD
* **Bootloader:** GNU GRUB 2
* **Emulação / Testes:** QEMU (`qemu-system-i386`)
* **Utilitários:** `xorriso`, `grub-mkrescue`
---
## 💻 Como usar:
### Opção 1: Baixando a Iso:
1. Faça o Download do .iso presente no último release:
2. Execute no terminal o comando abaixo usando o QEMU:
```bash
qemu-system-x86_64 -cdrom vgos.iso
```
### Opção 2: Baixando código fonte:
1. Baixe o codigo clonando o repositório no seu editor de código-fonte:
```bash
git clone https://github.com/VG-Os-Team/vg-os-system
```
2. Compile o codigo fonte no terminal do seu editor de código-fonte:
```bash
make clean && make iso
```
3. Execute um dos comandos abaixo usando o QEMU:
```bash
qemu-system-x86_64 -cdrom vgos.iso
```
ou esse comando (caso prefira):
```bash
make run
```
---
## 📁 Estrutura do Repositório
```text
vg-os-system/
├── boot/           # Inicialização em Assembly (boot.s) e configuração do GRUB (grub.cfg)
├── comandos/       # Interpretador e rotinas dos comandos do terminal (comandos.c, comandos.h)
├── gdt/            # Global Descriptor Table e TSS (gdt.c, gdt.h, gdt_asm.s, tss.c, tss.h)
├── idt/            # Interrupt Descriptor Table (idt.c, idt.h, idt_asm.s)
├── images/         # Recursos gráficos e logotipo em matriz de pixels (images/logo/)
├── include/        # Cabeçalhos centralizados (video.h, gfx.h, layout.h, splash.h, pmm.h, paging.h, heap.h, panic.h, idt.h, multiboot.h)
├── iso_root/       # Estrutura base de diretórios utilizada pelo grub-mkrescue
├── isr/            # Tratadores de interrupções e exceções da CPU (isr.c, isr.h, interrupts.s)
├── keyboard/       # Driver do teclado PS/2 via IRQ1 e scancodes ABNT2 (keyboard.c, keyboard.h)
├── pic/            # Remapeamento, EOI e máscara do controlador 8259 PIC (pic.c, pic.h)
├── Render/         # Renderização de texto em fonte bitmap e logotipo (render.c, render.h, font.h)
├── src/            # Subsistemas organizados: arch/x86/ (idt, panic), drivers/ (video, gfx), mm/ (pmm, paging, heap) e ui/ (layout, splash)
├── tests/          # Testes nativos isolados (render_flush_test.c via make test-video)
├── build/          # Binários (.bin) e arquivos objeto (.o) gerados na compilação
├── kernel.c        # Ponto de entrada do kernel (kernel_main) e ciclo principal
├── linker.ld       # Script do Linker definindo o layout de memória física
├── strutil.c / .h  # Funções auxiliares para manipulação de strings em modo freestanding
├── version.h       # Informações centralizadas de versão e build do VG OS
└── Makefile        # Automação de compilação, montagem e geração da ISO
```
## 📚 Referências & Agradecimentos
* **[OSDev Wiki](https://wiki.osdev.org/)** - Por fornecer tutoriais e documentação inestimáveis, bem como o guia fundamental "Bare Bones" utilizado para construir este sistema operacional.
