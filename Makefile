CC = gcc
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -Wall -Wextra -I. -IRender -Ikeyboard -Icomandos -Iimages/logo -Igdt -Iidt -Iisr
AS = as
ASFLAGS = --32
NASM = nasm
NASMFLAGS = -f elf32
LD = ld
LDFLAGS = -m elf_i386 -T linker.ld

BUILD_DIR = build
BIN = $(BUILD_DIR)/meuos.bin
BIN_1080 = $(BUILD_DIR)/meuos-1080.bin
BIN_1440 = $(BUILD_DIR)/meuos-1440.bin
BIN_1024 = $(BUILD_DIR)/meuos-1024.bin
ISO = vgos.iso

COMMON_OBJS = $(BUILD_DIR)/kernel.o \
              $(BUILD_DIR)/render.o \
              $(BUILD_DIR)/keyboard.o \
              $(BUILD_DIR)/strutil.o \
              $(BUILD_DIR)/logo.o \
              $(BUILD_DIR)/comandos.o \
              $(BUILD_DIR)/gdt.o \
              $(BUILD_DIR)/gdt_asm.o \
              $(BUILD_DIR)/idt.o \
              $(BUILD_DIR)/idt_asm.o \
              $(BUILD_DIR)/isr.o \
              $(BUILD_DIR)/interrupts.o \
              $(BUILD_DIR)/pic.o

all: $(BIN) $(BIN_1080) $(BIN_1440) $(BIN_1024)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot_1080.o: boot/boot.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) --defsym SCR_WIDTH=1920 --defsym SCR_HEIGHT=1080 $< -o $@

$(BUILD_DIR)/boot_1440.o: boot/boot.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) --defsym SCR_WIDTH=1440 --defsym SCR_HEIGHT=1080 $< -o $@

$(BUILD_DIR)/boot_1024.o: boot/boot.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) --defsym SCR_WIDTH=1024 --defsym SCR_HEIGHT=768 $< -o $@

$(BUILD_DIR)/kernel.o: kernel.c pic/pic.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/pic.o: pic/pic.c pic/pic.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/strutil.o: strutil.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/logo.o: images/logo/logo.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/render.o: Render/render.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/keyboard.o: keyboard/keyboard.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/comandos.o: comandos/comandos.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gdt.o: gdt/gdt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gdt_asm.o: gdt/gdt_asm.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/idt.o: idt/idt.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/idt_asm.o: idt/idt_asm.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/isr.o: isr/isr.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# interrupts.s usa sintaxe NASM (nao GAS), por isso monta com nasm e nao com $(AS).
$(BUILD_DIR)/interrupts.o: isr/interrupts.s | $(BUILD_DIR)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BIN_1080): $(BUILD_DIR)/boot_1080.o $(COMMON_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(BIN): $(BIN_1080)
	cp $< $@

$(BIN_1440): $(BUILD_DIR)/boot_1440.o $(COMMON_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

$(BIN_1024): $(BUILD_DIR)/boot_1024.o $(COMMON_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

iso: $(BIN) $(BIN_1080) $(BIN_1440) $(BIN_1024)
	mkdir -p iso_root/boot/grub
	cp $(BIN) iso_root/boot/meuos.bin
	cp $(BIN_1080) iso_root/boot/meuos-1080.bin
	cp $(BIN_1440) iso_root/boot/meuos-1440.bin
	cp $(BIN_1024) iso_root/boot/meuos-1024.bin
	cp boot/grub.cfg iso_root/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso_root

run: iso
	qemu-system-i386 -cdrom $(ISO) -vga std

clean:
	rm -rf $(BUILD_DIR) $(ISO) *.o meuos.bin Render/*.o keyboard/*.o comandos/*.o images/logo/*.o gdt/*.o idt/*.o isr/*.o iso_root/boot/*.bin
