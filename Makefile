# Make sure we use the correct compilers
CC = clang-8
LD = ld.lld-8

.PHONY: all clean image qemu

all: image

# We wanna build the kretlim-uefi-boot of course
include kretlim-uefi-boot.mk

# Test in qemu with the default image
qemu: tools/OVMF.fd image
	qemu-system-x86_64 -drive if=pflash,format=raw,readonly,file=tools/OVMF.fd -net none -hda bin/image.img

# Shortcut
image: bin/image.img

# Build the image
bin/image.img: tools/image-builder.py kretlim-uefi-boot
	mkdir -p bin/image/EFI/BOOT/
	cp bin/BOOTX64.EFI bin/image/EFI/BOOT/BOOTX64.EFI
	cd bin && ../tools/image-builder.py ../image.yaml

# Clean everything
clean: kretlim-uefi-boot-clean

clean-tools:
	rm -rf tools

# Get the image builder tool
tools/image-builder.py:
	mkdir -p tools
	cd tools && wget https://raw.githubusercontent.com/kretlim/image-builder/master/image-builder.py

# Get the bios
tools/OVMF.fd:
	mkdir -p tools
	cd tools && wget http://downloads.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip
	cd tools && unzip OVMF-X64-r15214.zip OVMF.fd
	rm tools/OVMF-X64-r15214.zip

