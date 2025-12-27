; ============================================================
; Kernel Entry Point - Smopsys Q-CORE
; 
; Trampolín de Assembly a C para el kernel principal.
; Este código se ejecuta después del bootloader.
; 
; Responsabilidades:
; - Configurar pila en modo protegido
; - Llamar a kernel_main()
; - Manejar retorno (halt infinito)
; ============================================================

[BITS 32]
[GLOBAL _start]
[EXTERN kernel_main]

section .text

_start:
    ; -----------------------------------------------------
    ; Inicialización del entorno de ejecución C
    ; -----------------------------------------------------
    
    ; Asegurar dirección de memoria incremental
    cld
    
    ; -----------------------------------------------------
    ; Habilitar FPU (Floating Point Unit)
    ; -----------------------------------------------------
    mov eax, cr0
    and ax, 0xFFFB      ; Clear EM (bit 2)
    or ax, 0x0022       ; Set MP (bit 1) and NE (bit 5)
    mov cr0, eax
    finit               ; Inicializar FPU
    
    ; Configurar segmentos de datos
    mov ax, 0x10            ; Selector de datos (GDT entry 2)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Configurar pila usando el espacio reservado en BSS
    mov esp, stack_top
    mov ebp, esp
    
    ; -----------------------------------------------------
    ; Limpiar sección BSS (poner a cero)
    ; -----------------------------------------------------
    [EXTERN __bss_start]
    [EXTERN __bss_end]
    
    mov edi, __bss_start    ; Inicio de BSS
    mov ecx, __bss_end      ; Fin de BSS
    sub ecx, edi            ; Tamaño en bytes
    xor eax, eax            ; Valor a escribir (0)
    rep stosb               ; Escribir ceros byte a byte
    
    ; -----------------------------------------------------
    ; Llamar al kernel principal en C
    ; -----------------------------------------------------
    call kernel_main
    
    ; -----------------------------------------------------
    ; Si kernel_main retorna, entrar en halt infinito
    ; (Esto no debería ocurrir en operación normal)
    ; -----------------------------------------------------
.halt:
    cli                     ; Deshabilitar interrupciones
    hlt                     ; Detener CPU
    jmp .halt               ; Loop por si HLT no persiste

; ============================================================
; Sección de datos BSS (no inicializada)
; ============================================================
section .bss
    resb 32768              ; Reservar 32KB para pila
stack_top:
