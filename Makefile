# Make sure we use the correct compilers
CC = clang-8
LD = ld.lld-8

# Set the boot shutdown directories
BOOT_SHUTDOWN_DIR = modules/boot-shutdown/
BOOT_SHUTDOWN_DIR_BIN = bin/image

# Set the bootloader directories
KRETLIM_UEFI_BOOT_DIR_BIN = bin/image/EFI/BOOT

.PHONY: all clean image qemu

all: bin/image.img

# We wanna build the kretlim-uefi-boot of course
include kretlim-uefi-boot.mk
include modules/boot-shutdown/boot-shutdown.mk

# Test in qemu with the default image
qemu: tools/OVMF.fd bin/image.img
	qemu-system-x86_64 -drive if=pflash,format=raw,readonly,file=tools/OVMF.fd -net none -hda bin/image.img

# Shortcut
image: bin/image.img

# Build the image
bin/image.img: tools/image-builder.py tools/kretlim-boot-config.py $(KRETLIM_UEFI_BOOT_DIR_BIN)/BOOTX64.EFI $(BOOT_SHUTDOWN_DIR_BIN)/shutdown.elf
	./tools/kretlim-boot-config.py default.yaml bin/image/kbootcfg.bin
	cd bin && ../tools/image-builder.py ../image.yaml

# Clean everything
clean: kretlim-uefi-boot-clean boot-shutdown-clean

# delete all the tools
clean-tools:
	rm -rf tools

# Get the image builder tool
tools/image-builder.py:
	mkdir -p tools
	cd tools && wget https://raw.githubusercontent.com/kretlim/image-builder/master/image-builder.py

# Get the boot config creator tool
tools/kretlim-boot-config.py:
	mkdir -p tools
	cd tools && wget https://raw.githubusercontent.com/kretlim/kretlim-boot-config/master/kretlim-boot-config.py

# Get the bios
tools/OVMF.fd:
	mkdir -p tools
	cd tools && wget http://downloads.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip
	cd tools && unzip OVMF-X64-r15214.zip OVMF.fd
	rm tools/OVMF-X64-r15214.zip

