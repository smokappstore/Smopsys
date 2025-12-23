; ============================================================
; Stage 1 Bootloader - Smopsys Q-CORE
; 
; MBR (512 bytes) que carga Stage 2 desde disco.
; Compatible con BIOS Legacy.
; 
; Mapa de memoria:
;   0x7C00       : Stage 1 (este archivo)
;   0x7E00       : Stage 2 (cargado por Stage 1)
;   0x10000      : Kernel (cargado por Stage 2)
; ============================================================

[BITS 16]
[ORG 0x7C00]

; Constantes
STAGE2_LOAD_ADDR    equ 0x7E00
STAGE2_SECTORS      equ 4           ; 2KB para Stage 2
STAGE2_START_SECTOR equ 2           ; Sector 2 (después del MBR)

start:
    ; Desactivar interrupciones durante setup
    cli
    cld
    
    ; Configurar segmentos
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    ; Guardar unidad de boot
    mov [boot_drive], dl
    
    ; Activar interrupciones
    sti
    
    ; Mostrar mensaje de inicio
    mov si, msg_boot
    call print_string
    
    ; --------------------------------------------------------
    ; CARGAR STAGE 2 DESDE DISCO
    ; --------------------------------------------------------
    
    mov si, msg_loading
    call print_string
    
    ; Configurar lectura de disco (INT 13h, AH=02h)
    mov ah, 0x02                ; Función: leer sectores
    mov al, STAGE2_SECTORS      ; Número de sectores
    mov ch, 0                   ; Cilindro 0
    mov cl, STAGE2_START_SECTOR ; Sector inicial
    mov dh, 0                   ; Cabeza 0
    mov dl, [boot_drive]        ; Unidad de boot
    mov bx, STAGE2_LOAD_ADDR    ; Destino ES:BX
    
    int 0x13
    jc disk_error               ; Error si CF está activo
    
    ; Verificar sectores leídos
    cmp al, STAGE2_SECTORS
    jne disk_error
    
    ; Stage 2 cargado exitosamente
    mov si, msg_ok
    call print_string
    
    ; Saltar a Stage 2
    jmp STAGE2_LOAD_ADDR

; --------------------------------------------------------
; MANEJO DE ERRORES
; --------------------------------------------------------

disk_error:
    mov si, msg_disk_error
    call print_string
    jmp halt

halt:
    cli
    hlt
    jmp halt

; --------------------------------------------------------
; FUNCIONES DE UTILIDAD
; --------------------------------------------------------

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

; --------------------------------------------------------
; DATOS
; --------------------------------------------------------

boot_drive      db 0
msg_boot        db 'SMOPSYS Q-CORE Stage 1', 13, 10, 0
msg_loading     db 'Loading Stage 2...', 0
msg_ok          db 'OK', 13, 10, 0
msg_disk_error  db 'DISK ERROR!', 13, 10, 0

; --------------------------------------------------------
; PADDING Y FIRMA MBR
; --------------------------------------------------------

times 510 - ($ - $$) db 0
dw 0xAA55
