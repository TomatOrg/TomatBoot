# Make sure we use the correct compilers
CLANG ?= clang-8

#########################
# By default just build
# the EFI
#########################

.PHONY: default all clean
default: all

#########################
# Configuration
#########################

TOMATBOOT_UEFI_DIR ?= $(notdir $(shell pwd)/)
TOMATBOOT_UEFI_DIR_BIN ?= $(TOMATBOOT_UEFI_DIR)bin/
TOMATBOOT_UEFI_DIR_BUILD ?= $(TOMATBOOT_UEFI_DIR)build/

#########################
# All the source files
#########################

# Actual sources
SRCS += $(shell find $(TOMATBOOT_UEFI_DIR)src/ -name '*.c')

# All the headers, we use them as dependency
HDRS += $(shell find $(TOMATBOOT_UEFI_DIR)src/ -name '*.h')

# UEFI Lib
SRCS += $(shell find $(TOMATBOOT_UEFI_DIR)lib/uefi/Library -name '*.c')
SRCS += $(shell find $(TOMATBOOT_UEFI_DIR)lib/uefi/Library -name '*.nasm')
HDRS += $(shell find $(TOMATBOOT_UEFI_DIR)lib/uefi/Include -name '*.h')

# Make sure we build the guids c file if it does not exists
SRCS += $(TOMATBOOT_UEFI_DIR)lib/uefi/Library/guids.c

# Get the objects and their dirs
OBJS := $(SRCS:%=$(TOMATBOOT_UEFI_DIR_BUILD)/%.o)
OBJDIRS := $(dir $(OBJS))

#########################
# Include directories
#########################

INCLUDE_DIRS += $(TOMATBOOT_UEFI_DIR)lib/uefi/Include
INCLUDE_DIRS += $(TOMATBOOT_UEFI_DIR)lib/uefi/Include/X64
INCLUDE_DIRS += $(TOMATBOOT_UEFI_DIR)src/

#########################
# Flags
#########################

# The flags
CFLAGS := \
	-target x86_64-unknown-windows \
	-ffreestanding \
	-fno-stack-check \
	-fno-stack-protector \
	-fshort-wchar \
	-g \
	-flto \
	-Ofast \
	-mno-red-zone \
	-std=c99 \
	-Werror

# Add all the includes
CFLAGS += $(INCLUDE_DIRS:%=-I%)

# Set the linking flags
LDFLAGS := \
	-target x86_64-unknown-windows \
	-nostdlib \
	-Wl,-entry:EfiMain \
	-Wl,-subsystem:efi_application \
	-fuse-ld=lld-link
	
NASMFLAGS := \
	-g \
	-f win64

NASMFLAGS += $(INCLUDE_DIRS:%=-i %/)

#########################
# Make an image for testing
#########################

test: tools/OVMF.fd bin/image.img
	qemu-system-x86_64 -drive if=pflash,format=raw,readonly,file=tools/OVMF.fd -net none -hda bin/image.img

# Build the image
bin/image.img: tools/image-builder.py \
               $(TOMATBOOT_UEFI_DIR_BIN)/BOOTX64.EFI
	mkdir -p bin/image/EFI/BOOT/
	cp $(TOMATBOOT_UEFI_DIR_BIN)/BOOTX64.EFI bin/image/EFI/BOOT/
	cd bin && ../tools/image-builder.py ../config/test-image.yaml

# Download the tool
# TODO: Checksum
tools/image-builder.py:
	mkdir -p tools
	cd tools && wget https://raw.githubusercontent.com/TomatOrg/image-builder/master/image-builder.py
	chmod +x tools/image-builder.py

# Download the bios
# TODO: Checksum
tools/OVMF.fd:
	mkdir -p tools
	cd tools && wget http://downloads.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip
	cd tools && unzip OVMF-X64-r15214.zip OVMF.fd
	rm tools/OVMF-X64-r15214.zip

#########################
# Cleaning
#########################

# Clean
clean:
	rm -rf $(TOMATBOOT_UEFI_DIR_BUILD) $(TOMATBOOT_UEFI_DIR_BIN)

#########################
# Actual build process
#########################

all: $(TOMATBOOT_UEFI_DIR_BIN)/BOOTX64.EFI

# specifically for the uefi lib
# we need to generate that file, have it depend on the includes of UEFI
$(TOMATBOOT_UEFI_DIR)lib/uefi/Library/guids.c: $(shell find $(TOMATBOOT_UEFI_DIR)lib/uefi/Include -name '*.h')
	cd $(TOMATBOOT_UEFI_DIR)lib/uefi/Library && python gen_guids.py


# Build the main efi file
$(TOMATBOOT_UEFI_DIR_BIN)/BOOTX64.EFI: $(OBJDIRS) $(OBJS)
	mkdir -p $(TOMATBOOT_UEFI_DIR_BIN)
	$(CLANG) $(LDFLAGS) -o $@ $(OBJS)

# Build each of the c files
$(TOMATBOOT_UEFI_DIR_BUILD)/%.c.o: %.c
	$(CLANG) $(CFLAGS) -D __FILENAME__="\"$<\"" -c -o $@ $<

# Build each of the c files
$(TOMATBOOT_UEFI_DIR_BUILD)/%.nasm.o: %.nasm
	nasm $(NASMFLAGS) -o $@ $<

# Create each of the dirs
$(TOMATBOOT_UEFI_DIR_BUILD)/%:
	mkdir -p $@
