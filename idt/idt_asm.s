/* idt/idt_asm.s (Sintaxe AT&T para GNU as) */
.global idt_flush

/* void idt_flush(uint32_t idt_ptr_addr) */
idt_flush:
    movl 4(%esp), %eax  /* Pega o argumento passado em C (o endereço do idtp) */
    lidt (%eax)         /* Carrega a IDT */
    ret                 /* Retorna */