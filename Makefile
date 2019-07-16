# Make sure we use the correct compilers
CC = clang-8
LD = ld.lld-8

#############################
# We want to build an image
#############################

.PHONY: default all clean image qemu
default: all

#############################
# Setup the configs
#############################

TOMATBOOT_UEFI_DIR_BIN := bin/image/EFI/BOOT/
TOMATBOOT_SHUTDOWN_DIR_BIN := bin/image/

include tomatboot-uefi.mk

#############################
# Default config
#############################

all: bin/image.img

#############################
# Start in qemu
# Will also download the
# OVMF bios
#############################

# Test in qemu with the default image
qemu: tools/OVMF.fd bin/image.img
	qemu-system-x86_64 -drive if=pflash,format=raw,readonly,file=tools/OVMF.fd -net none -hda bin/image.img

#############################
# Build an image
#############################

# Shortcut
image: bin/image.img

# Build the image
bin/image.img: tools/image-builder.py tools/tomatboot-config.py $(TOMATBOOT_UEFI_DIR_BIN)/BOOTX64.EFI $(TOMATBOOT_SHUTDOWN_DIR_BIN)/shutdown.elf
	./tools/tomatboot-config.py config/test-boot-config.yaml bin/image/kbootcfg.bin
	cd bin && ../tools/image-builder.py ../config/test-image.yaml

#############################
# Clean
#############################

# Clean everything
clean: tomatboot-uefi-clean
	rm -rf bin build

# delete all the tools
clean-tools:
	rm -rf tools

#############################
# Tools
#############################

# Maybe download some specific release instead of the latest?

# Get the image builder tool
# TODO: use https://github.com/vineyard-os/image-builder instead
tools/image-builder.py:
	mkdir -p tools
	cd tools && wget https://raw.githubusercontent.com/TomatOrg/image-builder/master/image-builder.py
	chmod +x tools/image-builder.py

# Get the boot config creator tool
tools/tomatboot-config.py:
	mkdir -p tools
	cd tools && wget https://raw.githubusercontent.com/TomatOrg/tomatboot-config/master/tomatboot-config.py
	chmod +x tools/tomatboot-config.py

# Get the bios
tools/OVMF.fd:
	mkdir -p tools
	cd tools && wget http://downloads.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip
	cd tools && unzip OVMF-X64-r15214.zip OVMF.fd
	rm tools/OVMF-X64-r15214.zip

