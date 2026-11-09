#include "idt.h"

#define IDT_ENTRIES 256

static idt_entry_t idt[IDT_ENTRIES];
static idt_ptr_t idtp;

void idt_set_gate(
    unsigned char num,
    unsigned int base,
    unsigned short sel,
    unsigned char flags
) {
    idt[num].base_low = (unsigned short)(base & 0xFFFFu);
    idt[num].base_high = (unsigned short)((base >> 16) & 0xFFFFu);
    idt[num].sel = sel;
    idt[num].always0 = 0;
    idt[num].flags = flags;
}

void idt_install(void) {
    idtp.limit = (unsigned short)(sizeof(idt_entry_t) * IDT_ENTRIES - 1u);
    idtp.base = (unsigned int)&idt;

    idt_flush((unsigned int)&idtp);
}
