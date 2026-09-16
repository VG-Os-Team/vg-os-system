![C Language](https://img.shields.io/badge/Language-C-%2300599C?style=for-the-badge&logo=c&logoColor=white)
![Assembly](https://img.shields.io/badge/Arch-x86_Assembly-%23E34F26?style=for-the-badge&logo=assemblyscript&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Bare_Metal_%2F_QEMU-%2341B883?style=for-the-badge&logo=linux&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)
# VG OS 

Um sistema operacional x86 de 32 bits desenvolvido do zero para fins acadêmicos e de aprendizado em baixo nível. O sistema conta com inicialização via Multiboot/GRUB, kernel próprio em Modo Protegido e um terminal gráfico interativo com renderização direta em Framebuffer linear.
---
## 📋 Funcionalidades Principais
* **Kernel Próprio (Modo Protegido 32-bit):** Escrito em C e Assembly x86 com gerenciamento manual de memória e periféricos.
* **Gerenciamento e Proteção de Memória:** Configuração da Global Descriptor Table (GDT) com 5 descritores, estruturando o espaço de endereçamento de 32 bits e garantindo a proteção do sistema através de        rotinas de alinhamento de registradores de segmento.
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
* **Terminal Interativo Integrado:**
  - Comandos embutidos: `help`, `version`, `clear`.
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
3. Execute o comando abaixo usando o QEMU:
```bash
qemu-system-x86_64 -cdrom vgos.iso
```
---
## 📁 Estrutura do Repositório
```text
vg-os-system/
├── boot/           # Inicialização em Assembly (boot.s) e configuração do GRUB (grub.cfg)
├── gdt/            # Implementação da Global Descriptor Table (GDT) e rotinas de alinhamento (gdt.c, gdt.h, gdt_asm.s)
├── images/         # Recursos gráficos e logotipo em matriz de pixels (logo/)
├── keyboard/       # Driver do controlador de teclado PS/2 e mapeamento de scancodes
├── Render/         # Subsistema gráfico em Framebuffer e tabela de fontes (font.h)
├── comandos/       # Interpretador e implementação de comandos do terminal
├── build/          # Arquivos objeto (.o) e executável final gerados pelo Makefile
├── kernel.c        # Ponto de entrada do kernel (kernel_main) e loop do terminal
├── version.h       # Informações centralizadas de versão do VG OS
├── linker.ld       # Script do Linker definindo o layout de memória
└── Makefile        # Script automatizado de compilação e criação da ISO
```
## 📚 Referências & Agradecimentos
* **[OSDev Wiki](https://wiki.osdev.org/)** - Por fornecer tutoriais e documentação inestimáveis, bem como o guia fundamental "Bare Bones" utilizado para construir este sistema operacional.
