CC = gcc
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables \
         -Wall -Wextra -MMD -MP \
         -I. -Iinclude -IRender -Ikeyboard -Icomandos -Iimages/logo -Igdt -Iidt -Iisr

AS = as
ASFLAGS = --32

NASM = nasm
NASMFLAGS = -f elf32

LD = ld
LDFLAGS = -m elf_i386 -T linker.ld

BUILD_DIR = build
ISO = vgos.iso

BIN_1080 = $(BUILD_DIR)/meuos-1080.bin
BIN_1440 = $(BUILD_DIR)/meuos-1440.bin
BIN_1024 = $(BUILD_DIR)/meuos-1024.bin
BIN      = $(BUILD_DIR)/meuos.bin

# Fontes C do kernel e subsistemas
C_SRCS = \
	kernel.c \
	strutil.c \
	comandos/comandos.c \
	gdt/gdt.c \
	gdt/tss.c \
	src/arch/x86/idt.c \
	images/logo/logo.c \
	isr/isr.c \
	keyboard/keyboard.c \
	pic/pic.c \
	Render/render.c \
	src/arch/x86/panic.c \
	src/mm/multiboot_mem.c \
	src/mm/pmm.c \
	src/mm/paging.c


# Fontes Assembly com sintaxe GNU Assembler (GAS)
GAS_SRCS = \
	gdt/gdt_asm.s \
	idt/idt_asm.s

# Fontes Assembly com sintaxe NASM
NASM_SRCS = \
	isr/interrupts.s

# Mapeia fontes para objetos dentro de $(BUILD_DIR) preservando subdiretorios
COMMON_OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(C_SRCS)) \
              $(patsubst %.s, $(BUILD_DIR)/%.o, $(GAS_SRCS)) \
              $(patsubst %.s, $(BUILD_DIR)/%.o, $(NASM_SRCS))

# Dependencias automaticas (.d) geradas pelo GCC (-MMD -MP)
DEPS = $(COMMON_OBJS:.o=.d)

all: $(BIN) $(BIN_1080) $(BIN_1440) $(BIN_1024)

# Regra generica para compilacao de arquivos C com rastreamento automatico de headers
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Regra generica para montagem de Assembly GNU (GAS)
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# Montagem de Assembly NASM (excecao para interrupts.s)
$(BUILD_DIR)/isr/interrupts.o: isr/interrupts.s
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

# Bootloader parametrizado por resolucao
$(BUILD_DIR)/boot/boot_1080.o: boot/boot.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) --defsym SCR_WIDTH=1920 --defsym SCR_HEIGHT=1080 $< -o $@

$(BUILD_DIR)/boot/boot_1440.o: boot/boot.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) --defsym SCR_WIDTH=1440 --defsym SCR_HEIGHT=1080 $< -o $@

$(BUILD_DIR)/boot/boot_1024.o: boot/boot.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) --defsym SCR_WIDTH=1024 --defsym SCR_HEIGHT=768 $< -o $@

# Linkagem dos binarios do kernel
$(BIN_1080): $(BUILD_DIR)/boot/boot_1080.o $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^

$(BIN): $(BIN_1080)
	cp $< $@

$(BIN_1440): $(BUILD_DIR)/boot/boot_1440.o $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^

$(BIN_1024): $(BUILD_DIR)/boot/boot_1024.o $(COMMON_OBJS)
	@mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $^

# Geracao da ISO inicializavel
iso: $(BIN) $(BIN_1080) $(BIN_1440) $(BIN_1024)
	@mkdir -p iso_root/boot/grub
	@cp $(BIN) iso_root/boot/meuos.bin
	@cp $(BIN_1080) iso_root/boot/meuos-1080.bin
	@cp $(BIN_1440) iso_root/boot/meuos-1440.bin
	@cp $(BIN_1024) iso_root/boot/meuos-1024.bin
	@cp boot/grub.cfg iso_root/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso_root

# Execucao no QEMU
run: iso
	qemu-system-i386 -cdrom $(ISO) -vga std

# Limpeza completa dos artefatos de build
clean:
	rm -rf $(BUILD_DIR) $(ISO) iso_root/boot/*.bin

# Inclui os arquivos de dependencias gerados dinamicamente
-include $(DEPS)

.PHONY: all iso run clean
