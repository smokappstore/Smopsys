; SOS - Smart Operative System 
; Versión: 0.3.2 "Quantum AI"
; Stage 0: MBR con Operador de Proyeccion Angular (Fix: Direction Flag)
; ------------------------------------------------------------------

[BITS 16]
[ORG 0x7C00]

start:
    cli                         ; Entropia Cero
    cld                         ; Asegurar direccion incrementales desde el inicio
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1                   ; Activar Protected Mode
    mov cr0, eax
    jmp 0x08:init_pm            ; Salto al segmento de codigo (Entry 1 en GDT)

; --- GDT (Global Descriptor Table) ---
gdt_start:
    dq 0x0                      ; Null Descriptor
gdt_code:
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00
gdt_data:
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ------------------------------------------------------------------
[BITS 32]
%define PHI_CONST 0x9E3779B9

init_pm:
    cld                         ; REFUERZO: Direccion de memoria incremental
    mov ax, 0x10                ; DATA_SEG (Entry 2 en GDT)
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov ebp, 0x90000            ; Pila en zona segura
    mov esp, ebp

    call clear_screen

    ; --- Trazas DIT ---
    mov esi, msg_header
    mov ebx, 0                  ; Fila 0
    call print_str

    mov esi, msg_init
    mov ebx, 2                  ; Fila 2
    call print_str
    
    mov dword [lfsr_state], 0xACE1

    mov ecx, 6                  ; 6 muestras de fase
    mov ebx, 4                  ; Iniciar en fila 4
.loop:
    push ecx
    push ebx
    
    ; --- Algoritmo de Proyeccion ---
    mov eax, [lfsr_state]
    mov edx, eax
    shr edx, 1
    and eax, 1
    neg eax
    and eax, 0xEDB88320         ; Polinomio de Galois
    xor eax, edx
    mov [lfsr_state], eax
    imul eax, PHI_CONST         ; Operador de Proyeccion Angular
    ror eax, 13

    mov esi, msg_rfsc
    call print_str
    call print_hex   

    pop ebx
    pop ecx
    inc ebx
    
    ; Delay para que el ojo humano perciba el flujo
    mov eax, 0x01FFFFFF
.d: dec eax
    jnz .d

    loop .loop

    mov esi, msg_ready
    mov ebx, 11
    call print_str

    ; Z-Pinch Lock (El aviso Cian en el centro de la pantalla Q)
    mov word [0xB8000 + 160*13 + 78], 0x0B5A ; 0B=Cian, 5A='Q'

    jmp $                       ; Halt infinito

; --- Funciones de Bajo Nivel ---

print_hex:
    pusha
    mov edi, 0xB8000
    imul ecx, ebx, 160
    add edi, ecx
    add edi, 50                 ; Columna de salida hex
    mov ecx, 8
.h: rol eax, 4
    mov edx, eax
    and edx, 0x0F
    mov dl, [hex_chars + edx]
    mov [edi], dl
    mov byte [edi+1], 0x0A      ; Verde brillante
    add edi, 2
    loop .h
    popa
    ret

print_str:
    pusha
    mov edi, 0xB8000
    imul eax, ebx, 160
    add edi, eax
.s: lodsb                       ; Carga AL desde DS:ESI e incrementa ESI (gracias a CLD)
    test al, al
    jz .done
    mov [edi], al
    mov byte [edi+1], 0x0F      ; Blanco intenso
    add edi, 2
    jmp .s
.done: 
    popa
    ret

clear_screen:
    pusha
    mov edi, 0xB8000
    mov ecx, 80*25
    mov ax, 0x0720              ; Espacio vacio (0x20) fondo negro (0x07)
    rep stosw                   ; Escribe AX en EDI de forma incremental (gracias a CLD)
    popa
    ret

; --- Seccion de Datos ---
lfsr_state  dd 0
hex_chars   db "0123456789ABCDEF"
msg_header  db " SOS - Q-SOS [Quantum Smart OS] ", 0
msg_init    db "Applying Quasi-Projection... ", 0
msg_rfsc    db "[RFSC] Angular Phase: ", 0
msg_ready   db "METRIPLECTIC EQUILIBRIUM REACHED. ", 0

; Relleno y Firma
times 510-($-$$) db 0
dw 0xAA55
