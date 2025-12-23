/*
 * Linker Script - Smopsys Q-CORE
 * 
 * Define el layout de memoria para el kernel.
 * 
 * Mapa de memoria:
 * 0x00000 - 0x07BFF : Zona de BIOS/interrupciones
 * 0x07C00 - 0x07DFF : Bootloader (512 bytes)
 * 0x10000 - 0x8FFFF : Kernel y datos (512KB)
 * 0x90000 - 0x9FFFF : Pila (64KB)
 * 0xB8000           : Video VGA
 */

OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    /* Kernel cargado en 0x10000 (64KB) */
    . = 0x10000;
    
    /* Sección de código ejecutable */
    .text : ALIGN(4096)
    {
        *(.text.boot)       /* Código de entrada primero */
        *(.text)            /* Resto del código */
        *(.text.*)
    }
    
    /* Datos de solo lectura */
    .rodata : ALIGN(4096)
    {
        *(.rodata)
        *(.rodata.*)
    }
    
    /* Datos inicializados */
    .data : ALIGN(4096)
    {
        *(.data)
        *(.data.*)
    }
    
    /* Datos no inicializados (BSS) */
    .bss : ALIGN(4096)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }
    
    /* Símbolo de fin del kernel */
    __kernel_end = .;
    
    /* Descartar secciones innecesarias */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame*)
    }
}
