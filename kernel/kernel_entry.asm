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
    
    ; Configurar segmentos de datos (ya deberían estar desde bootloader)
    mov ax, 0x10            ; Selector de datos (GDT entry 2)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Configurar pila
    mov ebp, 0x90000        ; Base de pila en zona segura
    mov esp, ebp
    
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
    resb 16384              ; Reservar 16KB para pila
stack_top:
