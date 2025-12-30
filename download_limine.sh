#!/bin/bash
# Download Limine binary release
# We need the 'limine' executable for host, and 'limine.sys' etc for the image.

LIMINE_VERSION="v5.20240125.0"
LIMINE_URL="https://github.com/limine-bootloader/limine/raw/v7.x-binary"

mkdir -p limine

# Download limine host tool
if [ ! -f limine/limine ]; then
    echo "Downloading Limine host tool..."
    # Warning: We are downloading a binary. In a real scenario, should build from source.
    # For this environment, we'll try to find a static binary or build it?
    # Actually, the repo has a 'limine' C source we can compile quickly.
    
    # Let's clone the branch intended for binaries which often has the executable or simple source
    git clone https://github.com/limine-bootloader/limine.git --branch v7.x-binary --depth 1 limine_repo
    
    # Build the 'limine' utility
    cd limine_repo
    make limine
    cp limine ../limine/
    cp limine-bios.sys ../limine/
    cp limine-bios-cd.bin ../limine/
    cp limine-uefi-cd.bin ../limine/
    cp BOOTX64.EFI ../limine/
    cp BOOTIA32.EFI ../limine/
    cd ..
    rm -rf limine_repo
    echo "Limine tools ready in ./limine/"
else
    echo "Limine already present."
fi
