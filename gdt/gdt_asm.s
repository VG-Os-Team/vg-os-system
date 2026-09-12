.set SEL_CODIGO_KERNEL, 0x08
.set SEL_DADOS_KERNEL,  0x10

.section .text
.global gdt_flush
.type gdt_flush, @function

# void gdt_flush(uint32_t gdt_ptr_endereco)
# Recebe pela pilha (cdecl) o endereço da struct gdt_ptr_t montada em gdt.c
gdt_flush:
    mov 4(%esp), %eax
    lgdt (%eax)

    # Os seletores de dados podem ser recarregados direto
    mov $SEL_DADOS_KERNEL, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss

    # CS só troca via far jump: o salto carrega o novo seletor e
    # continua na instrução seguinte, já com a GDT nova valendo
    ljmp $SEL_CODIGO_KERNEL, $recarrega_cs
recarrega_cs:
    ret
.size gdt_flush, . - gdt_flush
