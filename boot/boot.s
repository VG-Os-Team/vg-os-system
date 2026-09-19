.set ALIGN,      1<<0
.set MEMINFO,    1<<1
.set VIDEOMODE,  1<<2          
.set FLAGS,      ALIGN | MEMINFO | VIDEOMODE
.set MAGIC,      0x1BADB002
.set CHECKSUM,   -(MAGIC + FLAGS)

.ifndef SCR_WIDTH
.set SCR_WIDTH, 1920
.endif

.ifndef SCR_HEIGHT
.set SCR_HEIGHT, 1080
.endif

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
.long 0, 0, 0, 0, 0 

.long 0         
.long SCR_WIDTH      
.long SCR_HEIGHT      
.long 32        

.section .bss
.align 16
stack_bottom:
.skip 16384
stack_top:

.section .text
.global _start
.type _start, @function
_start:
    mov $stack_top, %esp
    push %ebx 
    call kernel_main
    cli
hang:
    hlt
    jmp hang
.size _start, . - _start
