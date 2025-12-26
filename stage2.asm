; ============================================================
; Stage 2 Bootloader - Smopsys Q-CORE
; 
; Responsabilidades:
; 1. Detectar memoria disponible (E820)
; 2. Habilitar línea A20
; 3. Cargar kernel desde disco
; 4. Configurar GDT y pasar a modo protegido
; 5. Saltar al kernel
; 
; Se carga en 0x7E00 por Stage 1
; ============================================================

[BITS 16]
[ORG 0x7E00]

; Constantes
KERNEL_LOAD_ADDR    equ 0x10000     ; 64KB
KERNEL_SECTORS      equ 64          ; 32KB máximo
KERNEL_START_SECTOR equ 6           ; Después de Stage1 + Stage2

stage2_start:
    ; Mostrar mensaje
    mov si, msg_stage2
    call print_string
    
    ; --------------------------------------------------------
    ; PASO 1: DETECTAR MEMORIA (E820)
    ; --------------------------------------------------------
    
    mov si, msg_memory
    call print_string
    call detect_memory
    
    ; --------------------------------------------------------
    ; PASO 2: HABILITAR LÍNEA A20
    ; --------------------------------------------------------
    
    mov si, msg_a20
    call print_string
    call enable_a20
    
    mov si, msg_ok
    call print_string
    
    ; --------------------------------------------------------
    ; PASO 3: CARGAR KERNEL
    ; --------------------------------------------------------
    
    mov si, msg_kernel
    call print_string
    call load_kernel
    
    mov si, msg_ok
    call print_string
    
    ; --------------------------------------------------------
    ; PASO 4: MODO PROTEGIDO
    ; --------------------------------------------------------
    
    mov si, msg_pm
    call print_string
    
    ; Desactivar interrupciones
    cli
    
    ; Cargar GDT
    lgdt [gdt_descriptor]
    
    ; Activar bit PE en CR0
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    ; Salto lejano a código de 32 bits
    jmp 0x08:protected_mode_entry

; ============================================================
; MODO PROTEGIDO (32 bits)
; ============================================================

[BITS 32]

protected_mode_entry:
    ; Configurar segmentos de datos
    mov ax, 0x10            ; Selector de datos
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Configurar pila
    mov ebp, 0x90000
    mov esp, ebp
    
    ; Limpiar pantalla
    call clear_screen_32
    
    ; Mostrar mensaje de entrada al kernel
    mov esi, msg_entering_kernel
    mov edi, 0xB8000
    call print_string_32
    
    ; Saltar al kernel en 0x10000
    jmp KERNEL_LOAD_ADDR

; ============================================================
; FUNCIONES 32-BIT
; ============================================================

clear_screen_32:
    pusha
    mov edi, 0xB8000
    mov ecx, 80 * 25
    mov ax, 0x0720          ; Espacio, fondo negro
    rep stosw
    popa
    ret

print_string_32:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0F            ; Blanco sobre negro
    stosw
    jmp .loop
.done:
    popa
    ret

; ============================================================
; FUNCIONES 16-BIT
; ============================================================

[BITS 16]

print_string:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .loop
.done:
    popa
    ret

; Detectar memoria usando INT 15h, E820
detect_memory:
    pusha
    mov di, memory_map
    xor ebx, ebx
    mov edx, 0x534D4150     ; 'SMAP'
    
.loop:
    mov eax, 0xE820
    mov ecx, 24
    int 0x15
    
    jc .done                ; Error o fin
    cmp eax, 0x534D4150
    jne .done
    
    add di, 24
    inc word [memory_entries]
    
    test ebx, ebx
    jnz .loop
    
.done:
    popa
    ret

; Habilitar línea A20 (método rápido + teclado)
enable_a20:
    pusha
    
    ; Método 1: Fast A20 (puerto 0x92)
    in al, 0x92
    test al, 2
    jnz .done
    or al, 2
    and al, 0xFE
    out 0x92, al
    
    ; Método 2: Controlador de teclado (por si Fast falla)
    call .wait_input
    mov al, 0xAD
    out 0x64, al
    
    call .wait_input
    mov al, 0xD0
    out 0x64, al
    
    call .wait_output
    in al, 0x60
    push ax
    
    call .wait_input
    mov al, 0xD1
    out 0x64, al
    
    call .wait_input
    pop ax
    or al, 2
    out 0x60, al
    
    call .wait_input
    mov al, 0xAE
    out 0x64, al
    
    call .wait_input
    
.done:
    popa
    ret

.wait_input:
    in al, 0x64
    test al, 2
    jnz .wait_input
    ret

.wait_output:
    in al, 0x64
    test al, 1
    jz .wait_output
    ret

; Cargar kernel desde disco
load_kernel:
    pusha
    
    ; Cargar kernel en segmentos de 64 sectores
    mov ax, KERNEL_LOAD_ADDR >> 4   ; Segmento
    mov es, ax
    xor bx, bx                      ; Offset 0
    
    mov ah, 0x02
    mov al, KERNEL_SECTORS
    mov ch, 0
    mov cl, KERNEL_START_SECTOR
    mov dh, 0
    mov dl, [boot_drive]
    
    int 0x13
    jc .error
    
    popa
    ret

.error:
    mov si, msg_error
    call print_string
    cli
    hlt

; ============================================================
; GDT (Global Descriptor Table)
; ============================================================

gdt_start:
    ; Descriptor nulo
    dq 0
    
gdt_code:
    ; Segmento de código: base=0, limit=4GB, 32-bit, ejecutable
    dw 0xFFFF       ; Limit bits 0-15
    dw 0x0000       ; Base bits 0-15
    db 0x00         ; Base bits 16-23
    db 10011010b    ; Access: Present, Ring 0, Code, Readable
    db 11001111b    ; Flags: 4KB granularity, 32-bit + Limit bits 16-19
    db 0x00         ; Base bits 24-31

gdt_data:
    ; Segmento de datos: base=0, limit=4GB, 32-bit, writable
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b    ; Access: Present, Ring 0, Data, Writable
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1  ; Tamaño
    dd gdt_start                ; Dirección

; ============================================================
; DATOS
; ============================================================

boot_drive      db 0
memory_entries  dw 0
memory_map      times 24*20 db 0    ; Espacio para 20 entradas E820

msg_stage2          db 'SMOPSYS Stage 2 Loader', 13, 10, 0
msg_memory          db 'Detecting memory (E820)...', 0
msg_a20             db 'Enabling A20 line...', 0
msg_kernel          db 'Loading kernel...', 0
msg_pm              db 'Entering protected mode...', 13, 10, 0
msg_ok              db 'OK', 13, 10, 0
msg_error           db 'ERROR!', 13, 10, 0
msg_entering_kernel db 'SMOPSYS Q-CORE Kernel Entry', 0

; ============================================================
; PADDING para 2KB (4 sectores)
; ============================================================

times 2048 - ($ - $$) db 0
