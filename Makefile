CC = gcc
CFLAGS = -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-asynchronous-unwind-tables -Wall -Wextra -I. -IRender -Ikeyboard -Icomandos -Iimages/logo -Igdt -Iidt
AS = as
ASFLAGS = --32
LD = ld
LDFLAGS = -m elf_i386 -T linker.ld

BUILD_DIR = build
BIN = $(BUILD_DIR)/meuos.bin
ISO = vgos.iso

OBJS = $(BUILD_DIR)/boot.o \
       $(BUILD_DIR)/kernel.o \
       $(BUILD_DIR)/render.o \
       $(BUILD_DIR)/keyboard.o \
       $(BUILD_DIR)/strutil.o \
       $(BUILD_DIR)/logo.o \
       $(BUILD_DIR)/comandos.o \
       $(BUILD_DIR)/gdt.o \
       $(BUILD_DIR)/gdt_asm.o \
       $(BUILD_DIR)/idt.o \
       $(BUILD_DIR)/idt_asm.o

all: $(BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/boot.o: boot/boot.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kernel.o: kernel.c | $(BUILD_DIR)
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

$(BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

iso: $(BIN)
	mkdir -p iso_root/boot/grub
	cp $(BIN) iso_root/boot/meuos.bin
	cp boot/grub.cfg iso_root/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) iso_root

clean:
	rm -rf $(BUILD_DIR) $(ISO) *.o meuos.bin Render/*.o keyboard/*.o comandos/*.o images/logo/*.o gdt/*.o idt/*.o