#!/bin/bash
# ============================================================
# Smopsys Q-CORE - USB Writer Script
# 
# Escribe la imagen del OS a una memoria USB para arrancar
# en hardware real.
# 
# USO:
#   ./write_usb.sh /dev/sdX
# 
# ADVERTENCIA: Este script BORRA TODO el contenido del
# dispositivo destino. Asegúrate de seleccionar el
# dispositivo correcto.
# ============================================================

set -e

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Imagen del OS
OS_IMAGE="smopsys.bin"

# ============================================================
# FUNCIONES
# ============================================================

print_banner() {
    echo -e "${CYAN}"
    echo "============================================================"
    echo "  SMOPSYS Q-CORE USB Writer"
    echo "  Metriplectic Operating System"
    echo "============================================================"
    echo -e "${NC}"
}

print_error() {
    echo -e "${RED}[ERROR] $1${NC}"
}

print_success() {
    echo -e "${GREEN}[OK] $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}[WARNING] $1${NC}"
}

print_info() {
    echo -e "${CYAN}[INFO] $1${NC}"
}

check_root() {
    if [ "$EUID" -ne 0 ]; then
        print_error "Este script debe ejecutarse como root"
        echo "Uso: sudo $0 /dev/sdX"
        exit 1
    fi
}

check_image() {
    if [ ! -f "$OS_IMAGE" ]; then
        print_error "Imagen del OS no encontrada: $OS_IMAGE"
        echo "Ejecuta 'make' primero para compilar el sistema"
        exit 1
    fi
    print_success "Imagen encontrada: $OS_IMAGE ($(stat -c %s $OS_IMAGE) bytes)"
}

validate_device() {
    local device=$1
    
    # Verificar que el dispositivo existe
    if [ ! -b "$device" ]; then
        print_error "Dispositivo no encontrado: $device"
        exit 1
    fi
    
    # Verificar que no es el disco del sistema
    root_device=$(df / | tail -1 | awk '{print $1}' | sed 's/[0-9]*$//')
    if [ "$device" == "$root_device" ]; then
        print_error "¡No puedes escribir al disco del sistema!"
        exit 1
    fi
    
    # Mostrar información del dispositivo
    print_info "Dispositivo seleccionado: $device"
    echo ""
    lsblk "$device" 2>/dev/null || true
    echo ""
}

confirm_write() {
    local device=$1
    
    print_warning "¡ADVERTENCIA! Esto BORRARÁ todo el contenido de $device"
    echo ""
    read -p "¿Estás seguro de que quieres continuar? (escribe 'SI' para confirmar): " confirm
    
    if [ "$confirm" != "SI" ]; then
        print_info "Operación cancelada"
        exit 0
    fi
}

unmount_device() {
    local device=$1
    
    print_info "Desmontando particiones de $device..."
    
    # Desmontar todas las particiones
    for part in ${device}*; do
        if mountpoint -q "$part" 2>/dev/null; then
            umount "$part" 2>/dev/null || true
        fi
    done
    
    # Esperar un momento
    sleep 1
    sync
}

write_image() {
    local device=$1
    
    print_info "Escribiendo imagen a $device..."
    echo ""
    
    dd if="$OS_IMAGE" of="$device" bs=512 status=progress conv=fsync
    
    # Sincronizar
    sync
    
    print_success "Imagen escrita exitosamente"
}

verify_write() {
    local device=$1
    local image_size=$(stat -c %s "$OS_IMAGE")
    
    print_info "Verificando escritura..."
    
    # Leer de vuelta y comparar
    if dd if="$device" bs=512 count=$((image_size / 512 + 1)) 2>/dev/null | \
       head -c "$image_size" | diff - "$OS_IMAGE" > /dev/null 2>&1; then
        print_success "Verificación exitosa"
    else
        print_warning "La verificación falló. La imagen puede haberse escrito correctamente de todos modos."
    fi
}

print_instructions() {
    echo ""
    echo -e "${GREEN}============================================================${NC}"
    echo -e "${GREEN}  ¡Escritura completada!${NC}"
    echo -e "${GREEN}============================================================${NC}"
    echo ""
    echo "Para arrancar Smopsys Q-CORE:"
    echo ""
    echo "  1. Inserta la USB en la computadora destino"
    echo "  2. Reinicia y entra al BIOS/UEFI (F2, F12, DEL)"
    echo "  3. Configura:"
    echo "     - Boot Mode: Legacy/CSM (no UEFI)"
    echo "     - Boot Order: USB primero"
    echo "  4. Guarda y reinicia"
    echo ""
    echo "El sistema mostrará:"
    echo "  - Banner de Q-CORE"
    echo "  - Evolución del operador áureo O_n"
    echo "  - Salida serial en COM1 (38400 baud)"
    echo ""
}

# ============================================================
# MAIN
# ============================================================

print_banner

# Verificar argumentos
if [ $# -ne 1 ]; then
    echo "Uso: sudo $0 /dev/sdX"
    echo ""
    echo "Dispositivos disponibles:"
    lsblk -d -o NAME,SIZE,TYPE,MOUNTPOINT | grep disk
    exit 1
fi

DEVICE=$1

check_root
check_image
validate_device "$DEVICE"
confirm_write "$DEVICE"
unmount_device "$DEVICE"
write_image "$DEVICE"
verify_write "$DEVICE"
print_instructions

exit 0
