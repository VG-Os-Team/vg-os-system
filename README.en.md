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
* **Custom Kernel (32-bit Protected Mode):** Written in C and x86 Assembly with manual memory and hardware management.
* **GDT and TSS:** GDT with 6 descriptors, flat kernel/user segments, and a 32-bit TSS loaded via `LTR`. Reserved kernel stack for future privilege transitions. (This stage does not implement processes, scheduling, or paging memory isolation yet).
* **Kernel Panic:** Fatal error screen displaying the message, CPU registers saved at the exception, and halting via `CLI`/`HLT`. Integrated into the 32 exception handlers. The `panic` command triggers `UD2` for demonstration and requires restarting the virtual machine.
* **Multiboot Bootloader (GRUB):** Native video mode configuration via Multiboot header, entering graphics mode directly at boot.
* **Linear Framebuffer Graphics Output (VESA):**
  - Direct pixel-by-pixel rendering without relying on real-mode BIOS interrupts.
  - High-resolution custom image/logo display.
  - Text renderer with a scaled (2x) bitmap font.
* **Complete PS/2 Keyboard Driver:**
  - Support for the **Brazilian ABNT2** layout (including `ç` / `Ç` and `/ ?`).
  - Extended scancode support (`0xE0`), such as arrow keys, Delete, Home, End, and Right Ctrl.
  - Keyboard shortcut support: `Ctrl+L` (clear screen), `Ctrl+C` (copy line), and `Ctrl+V` (paste).
  - Key repeat handling and Caps Lock.
* **Physical Memory Manager (PMM - 4KB Bitmap):**
  - 4KB physical page allocator based on a bitmap array (`pmm_alloc_block`, `pmm_free_block`).
  - Routines to mark pages as free/used (`pmm_set_block`, `pmm_unset_block`, `pmm_test_block`).
  - Automatic initialization from the physical memory map provided by Multiboot (`multiboot_mmap`).
  - Protection of critical regions: low memory (first 1MB), kernel code/data/BSS, VESA framebuffer, and Multiboot structures.
* **Integrated Interactive Terminal:**
  - Built-in commands: `help`, `version`, `clear`, `exit`, `gdt` (GDT/TSS diagnostics), `pmm` (physical memory manager diagnostics), `panic` (fatal exception test).
  - Automatic line wrapping and multiline Backspace handling.
* **Modular Architecture:**
  - Centralized system versioning (`version.h`).
  - Isolated compilation output in the `build/` directory.
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
├── include/        # Centralized system and diagnostic headers (panic.h)
├── iso_root/       # Directory structure used by grub-mkrescue
├── isr/            # CPU interrupt and exception handlers (isr.c, isr.h, interrupts.s)
├── keyboard/       # PS/2 keyboard controller driver and ABNT2 scancodes (keyboard.c, keyboard.h)
├── pic/            # 8259 PIC remap and controller (pic.c, pic.h)
├── Render/         # Linear Framebuffer video subsystem and bitmap font (render.c, render.h, font.h)
├── src/            # Architecture-organized modules (src/arch/x86/panic.c)
├── build/          # Binaries (.bin) and object files (.o) generated during compilation
├── kernel.c        # Kernel entry point (kernel_main) and main loop
├── linker.ld       # Linker script defining the physical memory layout
├── strutil.c / .h  # Helper string manipulation functions for freestanding environment
├── version.h       # Centralized VG OS version and build metadata
└── Makefile        # Build automation, assembly, and ISO generation
```
## 📚 References & Acknowledgments
* **[OSDev Wiki](https://wiki.osdev.org/)** - For providing invaluable tutorials and documentation, as well as the foundational "Bare Bones" guide used to build this operating system.
