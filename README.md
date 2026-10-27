![C Language](https://img.shields.io/badge/Language-C-%2300599C?style=for-the-badge&logo=c&logoColor=white)
![Assembly](https://img.shields.io/badge/Arch-x86_Assembly-%23E34F26?style=for-the-badge&logo=assemblyscript&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Bare_Metal_%2F_QEMU-%2341B883?style=for-the-badge&logo=linux&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)
# VG OS 

Um sistema operacional x86 de 32 bits desenvolvido do zero para fins acadêmicos e de aprendizado em baixo nível. O sistema conta com inicialização via Multiboot/GRUB, kernel próprio em Modo Protegido e um terminal gráfico interativo com renderização direta em Framebuffer linear.
---
## 📋 Funcionalidades Principais
* **Kernel Próprio (Modo Protegido 32-bit):** Escrito em C e Assembly x86 com gerenciamento manual de memória e periféricos.
* **GDT e TSS:** GDT com 6 descritores, segmentos flat de kernel/usuario e TSS de 32 bits carregado por `LTR`. Pilha de kernel reservada para futuras transicoes de privilegio. Essa etapa nao implementa processos, escalonamento ou isolamento de memoria por paginacao.
* **Kernel panic:** Tela de erro fatal com mensagem, registradores salvos na excecao e parada em `CLI`/`HLT`. Integrada aos 32 tratadores de excecao. O comando `panic` provoca `UD2` para demonstracao e exige reiniciar a maquina virtual.
* **Bootloader Multiboot (GRUB):** Configuração nativa de vídeo via cabeçalho Multiboot, inicializando o modo gráfico diretamente no boot.
* **Saída Gráfica em Framebuffer Linear (VESA):**
  - Renderização direta pixel a pixel sem depender de interrupções da BIOS em modo real.
  - Exibição de imagem/logotipo customizado em alta resolução.
  - Renderizador de texto com fonte bitmap escalonada (2x).
* **Driver de Teclado PS/2 Completo:**
  - Suporte ao padrão **ABNT2 brasileiro** (incluindo `ç` / `Ç` e `/ ?`).
  - Suporte a scancodes estendidos (`0xE0`), como setas direcionais, Delete, Home, End e Ctrl Direito.
  - Suporte a atalhos de teclado: `Ctrl+L` (limpar tela), `Ctrl+C` (copiar linha) e `Ctrl+V` (colar).
  - Tratamento de repetição de teclas (*key repeat*) e Caps Lock.
* **Gerenciador de Memória Física (PMM - 4KB Bitmap):**
  - Alocador de páginas físicas de 4KB baseado em array de bitmap (`pmm_alloc_block`, `pmm_free_block`).
  - Rotinas para marcar páginas como livres/ocupadas (`pmm_set_block`, `pmm_unset_block`, `pmm_test_block`).
  - Inicialização automática via mapa de memória física obtido do Multiboot (`multiboot_mmap`).
  - Proteção de regiões críticas: baixa memória (primeiro 1MB), código/dados/BSS do kernel, framebuffer VESA e estruturas do Multiboot.
* **Terminal Interativo Integrado:**
  - Comandos embutidos: `help`, `version`, `clear`, `exit`, `gdt` (diagnostico de GDT/TSS), `pmm` (diagnostico do alocador de memoria fisica), `panic` (teste fatal).
  - Tratamento de quebra automática de linha (*line wrap*) e Backspace multilinha.
* **Arquitetura Modular:**
  - Centralização de versão do sistema (`version.h`).
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
├── include/        # Cabeçalhos centralizados de sistema e diagnóstico (panic.h)
├── iso_root/       # Estrutura base de diretórios utilizada pelo grub-mkrescue
├── isr/            # Tratadores de interrupções e exceções da CPU (isr.c, isr.h, interrupts.s)
├── keyboard/       # Driver do controlador de teclado PS/2 e scancodes ABNT2 (keyboard.c, keyboard.h)
├── pic/            # Remapeamento e controle do chip 8259 PIC (pic.c, pic.h)
├── Render/         # Subsistema de vídeo em Framebuffer linear e fonte bitmap (render.c, render.h, font.h)
├── src/            # Módulos organizados por arquitetura (src/arch/x86/panic.c)
├── build/          # Binários (.bin) e arquivos objeto (.o) gerados na compilação
├── kernel.c        # Ponto de entrada do kernel (kernel_main) e ciclo principal
├── linker.ld       # Script do Linker definindo o layout de memória física
├── strutil.c / .h  # Funções auxiliares para manipulação de strings em modo freestanding
├── version.h       # Informações centralizadas de versão e build do VG OS
└── Makefile        # Automação de compilação, montagem e geração da ISO
```
## 📚 Referências & Agradecimentos
* **[OSDev Wiki](https://wiki.osdev.org/)** - Por fornecer tutoriais e documentação inestimáveis, bem como o guia fundamental "Bare Bones" utilizado para construir este sistema operacional.
