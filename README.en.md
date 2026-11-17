![C Language](https://img.shields.io/badge/Language-C-%2300599C?style=for-the-badge&logo=c&logoColor=white)
![Assembly](https://img.shields.io/badge/Arch-x86_Assembly-%23E34F26?style=for-the-badge&logo=assemblyscript&logoColor=white)
![Platform](https://img.shields.io/badge/Platform-Bare_Metal_%2F_QEMU-%2341B883?style=for-the-badge&logo=linux&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge)

<p align="center">
  <a href="README.md">🇧🇷 Português</a> • <a href="README.en.md"><b>🇺🇸 English</b></a>
</p>

# VG OS 

A 32-bit x86 operating system developed from scratch for academic and low-level learning purposes. The system features Multiboot/GRUB initialization, a custom kernel running in Protected Mode, and an interactive graphical terminal with direct linear Framebuffer rendering.
---
## 📋 Main Features
* **Custom Kernel (32-bit Protected Mode):** Written in C and x86 Assembly with manual memory, interrupt, and hardware management.
* **GDT, IDT, PIC, and TSS:** GDT with 6 descriptors, 32-bit TSS loaded via `LTR`, IDT with 32 CPU exceptions and 16 IRQ lines via remapped 8259A PIC (including spurious IRQ handling and per-line masking).
* **Kernel Panic:** Fatal error screen displaying the message, CPU registers saved at the exception, and halting via `CLI`/`HLT`, integrated into all 32 exception handlers. The `panic` command triggers `UD2` for demonstration.
* **Multiboot Bootloader (GRUB):** Boot menu resolution selection (`1920×1080`, `1440×1080`, `1024×768`) initializing VESA graphics mode directly at boot.
* **Boot Splash Screen:** Displays the official VG OS logo, version banner (`OS_BANNER`), and a real-time progress bar tracking each kernel initialization stage.
* **Linear Framebuffer Graphics (VESA), Double Buffering & Primitives:**
  - RAM backbuffer allocation (`video_init_backbuffer`) and fast 32-bit copy (`rep movsl`) to VRAM via `render_flush()`, preserving line pitch padding.
  - Drawing primitives (`gfx.c`): pixels, rectangles, lines, and strings using a scaled (2x) bitmap font.
  - Modular graphical layout (`src/ui/layout.c`) with header, scrollable terminal window, and responsive footer showing backbuffer status and shortcuts.
* **Interrupt-Driven PS/2 Keyboard Driver (IRQ1):**
  - **IRQ1** handler reading Set 1 scancodes from I/O port `0x60` into a circular buffer, with low-power idle waiting (`sti; hlt`).
  - Support for the **Brazilian ABNT2** layout (including `ç` / `Ç`, `/ ?`, and dead-key accents `´`, `` ` ``, `~`, `^`).
  - Extended scancode support (`0xE0`: arrow keys, Delete, Home, End, Right Ctrl) and shortcuts (`Ctrl+L`, `Ctrl+C`, `Ctrl+V`).
  - Shift, Caps Lock, Enter, Backspace, and key repeat handling.
* **Memory Management (PMM, x86 Paging & Heap):**
  - **PMM (4KB Bitmap):** Physical page allocator initialized from the Multiboot memory map, protecting low memory, kernel sections, VESA framebuffer, and Multiboot structures.
  - **x86 Paging:** Identity mapping of the kernel and framebuffer VRAM.
  - **Dynamic Kernel Heap:** `kmalloc` / `kfree` allocator backed by PMM pages.
* **Integrated Interactive Terminal:**
  - Built-in commands: `help`, `info` (renders logo with live system, video, RAM, and Heap stats), `version`, `clear`, `exit`, `gdt`, `pmm`, `panic`.
  - Automatic line wrapping, scrolling, and multiline Backspace handling.
* **Modular Architecture & Testing:**
  - Centralized system versioning (`version.h`).
  - Isolated native video driver tests (`make test-video` in `tests/render_flush_test.c`).
  - Isolated build output in the `build/` directory.
---
## 🛠️ Technologies and Tools
* **Languages:** C (freestanding C99 standard) and x86 Assembly (GNU Assembler / NASM)
* **Compiler / Linker:** GCC (`-m32`, `-ffreestanding`) and GNU LD
* **Bootloader:** GNU GRUB 2
* **Emulation / Testing:** QEMU (`qemu-system-i386`)
* **Utilities:** `xorriso`, `grub-mkrescue`
---
## 💻 How to Run
### Option 1: Downloading the ISO:
1. Download the `.iso` file from the latest release.
2. Run the command below in your terminal using QEMU:
```bash
qemu-system-x86_64 -cdrom vgos.iso
```
### Option 2: From Source Code:
1. Clone the repository:
```bash
git clone https://github.com/VG-Os-Team/vg-os-system
```
2. Build the ISO:
```bash
make clean && make iso
```
3. Run the virtual machine with QEMU:
```bash
qemu-system-x86_64 -cdrom vgos.iso
```
or simply:
```bash
make run
```
---
## 📁 Repository Structure
```text
vg-os-system/
├── boot/           # Assembly bootloader entry (boot.s) and GRUB configuration (grub.cfg)
├── comandos/       # Terminal command interpreter and routines (comandos.c, comandos.h)
├── gdt/            # Global Descriptor Table and TSS (gdt.c, gdt.h, gdt_asm.s, tss.c, tss.h)
├── idt/            # Interrupt Descriptor Table (idt.c, idt.h, idt_asm.s)
├── images/         # Graphical assets and bitmap logo (images/logo/)
├── include/        # Centralized headers (video.h, gfx.h, layout.h, splash.h, pmm.h, paging.h, heap.h, panic.h, idt.h, multiboot.h)
├── iso_root/       # Directory structure used by grub-mkrescue
├── isr/            # CPU interrupt and exception handlers (isr.c, isr.h, interrupts.s)
├── keyboard/       # IRQ1-driven PS/2 keyboard driver and ABNT2 scancodes (keyboard.c, keyboard.h)
├── pic/            # 8259 PIC remap, EOI, and IRQ masking (pic.c, pic.h)
├── Render/         # Bitmap font text rendering and logo drawing (render.c, render.h, font.h)
├── src/            # Organized subsystems: arch/x86/ (idt, panic), drivers/ (video, gfx), mm/ (pmm, paging, heap), and ui/ (layout, splash)
├── tests/          # Isolated native tests (render_flush_test.c via make test-video)
├── build/          # Binaries (.bin) and object files (.o) generated during compilation
├── kernel.c        # Kernel entry point (kernel_main) and main loop
├── linker.ld       # Linker script defining the physical memory layout
├── strutil.c / .h  # Helper string manipulation functions for freestanding environment
├── version.h       # Centralized VG OS version and build metadata
└── Makefile        # Build automation, assembly, and ISO generation
```
## 📚 References & Acknowledgments
* **[OSDev Wiki](https://wiki.osdev.org/)** - For providing invaluable tutorials and documentation, as well as the foundational "Bare Bones" guide used to build this operating system.
