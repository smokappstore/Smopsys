[bits 32]

extern isr_handler

%macro isr_stub 1
isr_stub_%1:
    push byte 0 ; Dummy error code
    push %1     ; Interrupt number
    jmp common_isr_handler
%endmacro

%macro isr_stub_err 1
isr_stub_%1:
    push %1     ; Interrupt number (error code already on stack)
    jmp common_isr_handler
%endmacro

; Definir los 256 stubs
%assign i 0
%rep 256
    ; Algunos interrupts (8, 10-14, 17, 30) pushean error code
    %if i == 8 || (i >= 10 && i <= 14) || i == 17 || i == 30
        isr_stub_err i
    %else
        isr_stub i
    %endif
%assign i i+1
%endrep

common_isr_handler:
    pushad      ; Pushes edi, esi, ebp, esp, ebx, edx, ecx, eax
    
    mov ax, ds  ; Save data segment
    push eax
    
    mov ax, 0x10 ; Use kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp    ; Pass pointer to registers (optional, but good practice)
    call isr_handler
    add esp, 4
    
    pop eax     ; Restore data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    popad       ; Restore registers
    add esp, 8  ; Cleans up pushed error code and pushed ISR number
    iret        ; Return from interrupt

; Tabla de punteros a los stubs
global isr_stub_table
isr_stub_table:
%assign i 0
%rep 256
    dd isr_stub_ %+ i
%assign i i+1
%endrep
