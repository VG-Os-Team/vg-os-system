; interrupts.s
;
; Stubs de baixo nivel para as 32 excecoes padrao da CPU x86 (vetores 0-31).
; Cada stub empilha o numero da excecao (e um codigo de erro, real ou "0"
; falso) e cai em isr_common_stub, que monta o registers_t e chama a funcao
; generica isr_handler() em C.
;
; Monta com: nasm -f elf32 interrupts.s -o interrupts.o

[BITS 32]

section .text

extern isr_handler

; ---------------------------------------------------------------------------
; Macros de geracao dos stubs
; ---------------------------------------------------------------------------

; Excecoes que NAO empilham codigo de erro automaticamente: empilhamos um
; "0" falso, para que todo stub deixe a pilha no mesmo formato.
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push dword 0
    push dword %1
    jmp isr_common_stub
%endmacro

; Excecoes em que a propria CPU empilha um codigo de erro real antes do EIP.
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push dword %1
    jmp isr_common_stub
%endmacro

; ---------------------------------------------------------------------------
; As 32 excecoes da CPU (Intel SDM vol. 3, cap. 6)
; ---------------------------------------------------------------------------
ISR_NOERRCODE 0   ; Divisao por zero
ISR_NOERRCODE 1   ; Debug
ISR_NOERRCODE 2   ; NMI
ISR_NOERRCODE 3   ; Breakpoint
ISR_NOERRCODE 4   ; Overflow
ISR_NOERRCODE 5   ; BOUND Range Exceeded
ISR_NOERRCODE 6   ; Opcode invalido
ISR_NOERRCODE 7   ; Dispositivo nao disponivel
ISR_ERRCODE   8   ; Double Fault (codigo de erro sempre 0)
ISR_NOERRCODE 9   ; Coprocessor Segment Overrun (legado)
ISR_ERRCODE   10  ; TSS invalida
ISR_ERRCODE   11  ; Segmento nao presente
ISR_ERRCODE   12  ; Stack-Segment Fault
ISR_ERRCODE   13  ; General Protection Fault
ISR_ERRCODE   14  ; Page Fault
ISR_NOERRCODE 15  ; Reservada
ISR_NOERRCODE 16  ; x87 FPU Exception
ISR_ERRCODE   17  ; Alignment Check
ISR_NOERRCODE 18  ; Machine Check
ISR_NOERRCODE 19  ; SIMD Floating-Point Exception
ISR_NOERRCODE 20  ; Virtualization Exception
ISR_ERRCODE   21  ; Control Protection Exception
ISR_NOERRCODE 22  ; Reservada
ISR_NOERRCODE 23  ; Reservada
ISR_NOERRCODE 24  ; Reservada
ISR_NOERRCODE 25  ; Reservada
ISR_NOERRCODE 26  ; Reservada
ISR_NOERRCODE 27  ; Reservada
ISR_NOERRCODE 28  ; Hypervisor Injection Exception
ISR_ERRCODE   29  ; VMM Communication Exception
ISR_ERRCODE   30  ; Security Exception
ISR_NOERRCODE 31  ; Reservada

; ---------------------------------------------------------------------------
; Stub comum
;
; IMPORTANTE: a ordem dos "push" abaixo define o layout de memoria lido pela
; struct registers_t (ver isr.h). Nao altere um sem atualizar o outro.
;
; isr_handler() recebe registers_t POR VALOR (nao por ponteiro), como pedido.
; Isso so e seguro aqui porque:
;   1) o VGOS ainda roda tudo em ring 0 -> ring 0 (sem troca de privilegio),
;      entao a CPU nao empilha ESP/SS extras na excecao;
;   2) registers_t tem exatamente os mesmos campos, na mesma ordem, que
;      estao sendo empilhados abaixo, entao o "argumento de pilha" que o
;      cdecl do GCC espera encontrar coincide byte a byte com o que ja esta
;      na pilha quando chamamos "call isr_handler".
; Se no futuro houver suporte a user mode (ring 3), troque para passar um
; ponteiro (push esp / call isr_handler / add esp,4) e mude a assinatura de
; isr_handler para registers_t* -- e mais seguro e nao depende desse
; alinhamento implicito.
; ---------------------------------------------------------------------------
isr_common_stub:
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10        ; seletor de dados do kernel (GDT)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8           ; descarta int_no e err_code empilhados pelo stub
    sti
    iret
