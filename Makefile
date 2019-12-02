# Make sure we use the correct compilers
CLANG ?= clang
FUSE_LD ?= lld-link

#########################
# By default just build
# the EFI
#########################

.PHONY: default all clean
default: all

#########################
# All the source files
#########################

# Actual sources
SRCS += $(shell find ./src/ -name '*.c')

# All the headers, we use them as dependency
HDRS += $(shell find ./src/ -name '*.h')

# UEFI Lib
SRCS += $(shell find ./lib/uefi/Library -name '*.c')
SRCS += $(shell find ./lib/uefi/Library -name '*.nasm')
HDRS += $(shell find ./lib/uefi/Include -name '*.h')

# Make sure we build the guids c file if it does not exists
SRCS += ./lib/uefi/Library/guids.c

# Get the objects and their dirs
OBJS := $(SRCS:%=./build/%.o)
OBJDIRS := $(dir $(OBJS))

#########################
# Include directories
#########################

INCLUDE_DIRS += ./lib/uefi/Include
INCLUDE_DIRS += ./lib/uefi/Include/X64
INCLUDE_DIRS += ./lib/
INCLUDE_DIRS += ./src/

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
	-mno-red-zone \
	-std=c11 \
	-Werror \
	-Ofast \
	-flto

# Add all the includes
CFLAGS += $(INCLUDE_DIRS:%=-I%)

# Set the linking flags
LDFLAGS := \
	-target x86_64-unknown-windows \
	-nostdlib \
	-Wl,-entry:EfiMain \
	-Wl,-subsystem:efi_application \
	-fuse-ld=$(FUSE_LD)
	
NASMFLAGS := \
	-g \
	-f win64

NASMFLAGS += $(INCLUDE_DIRS:%=-i %/)

#########################
# Make a corepure64 image
#########################

# Shortcut
linux: bin/image.img

# Create the boot image
bin/image.img: tools/linux \
                tools/image-builder.py \
                bin/BOOTX64.EFI
	mkdir -p ./bin/image/EFI/BOOT/
	cp ./bin/BOOTX64.EFI ./bin/image/EFI/BOOT/
	cp ./config/linux.cfg ./bin/image/tomatboot.cfg
	cp ./tools/linux/* ./bin/image
	cd bin && ../tools/image-builder.py ../config/linux-image.yaml

# Download the corepure64 files
tools/linux:
	echo "Please put a vmlinuz and initrd files in the ./tools/linux folder to create a linux bootable image"
	false

# Download the tool
# TODO: Checksum
# TODO: have this as submodule instead
tools/image-builder.py:
	mkdir -p tools
	cd tools && wget https://raw.githubusercontent.com/TomatOrg/image-builder/master/image-builder.py
	chmod +x tools/image-builder.py

#########################
# Cleaning
#########################

# Clean
clean:
	rm -rf ./bin ./build

#########################
# Actual build process
#########################

all: ./bin/BOOTX64.EFI

# specifically for the uefi lib
# we need to generate that file, have it depend on the includes of UEFI
./lib/uefi/Library/guids.c:
	cd ./lib/uefi/Library && python gen_guids.py

# Build the main efi file
./bin/BOOTX64.EFI: $(OBJDIRS) $(OBJS)
	mkdir -p bin
	$(CLANG) $(LDFLAGS) -o $@ $(OBJS)

# Build each of the c files
./build/%.c.o: %.c
	$(CLANG) $(CFLAGS) -c -o $@ $<

# Build each of the c files
./build/%.nasm.o: %.nasm
	nasm $(NASMFLAGS) -o $@ $<

# Create each of the dirs
./build/%:
	mkdir -p $@
