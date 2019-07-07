# CC Should be clang
CC ?= clang-8

# this is where the final result is put
BIN_FOLDER ?= bin

# this is where the intermidiate files are put
BUILD_FOLDER ?= build

# Get all the files to build
KRETLIM_UEFI_BOOT_SRCS += $(shell find src -name '*.c')
KRETLIM_UEFI_BOOT_SRCS += $(shell find lib -name '*.c')

# All the headers, we use them as dependency
KRETLIM_UEFI_BOOT_HDRS += $(shell find lib -name '*.h')
KRETLIM_UEFI_BOOT_HDRS += $(shell find src -name '*.h')

# Get the objects and their dirs
KRETLIM_UEFI_BOOT_OBJS := $(KRETLIM_UEFI_BOOT_SRCS:%.c=$(BUILD_FOLDER)/%.o)
KRETLIM_UEFI_BOOT_OBJDIRS := $(dir $(KRETLIM_UEFI_BOOT_OBJS))

# Include dirs
KRETLIM_UEFI_BOOT_INCLUDE_DIRS += lib/
KRETLIM_UEFI_BOOT_INCLUDE_DIRS += lib/libc
KRETLIM_UEFI_BOOT_INCLUDE_DIRS += src/

# The flags 
KRETLIM_UEFI_BOOT_CFLAGS += \
	-target x86_64-unknown-windows \
	-ffreestanding \
	-fno-stack-check \
	-fno-stack-protector \
	-fshort-wchar \
	-c \
	-mno-red-zone

# Add all the includes
KRETLIM_UEFI_BOOT_CFLAGS += $(KRETLIM_UEFI_BOOT_INCLUDE_DIRS:%=-I%)

# Set the linking flags
KRETLIM_UEFI_BOOT_LDFLAGS += \
	-target x86_64-unknown-windows \
	-nostdlib \
	-Wl,-entry:EfiMain \
	-Wl,-subsystem:efi_application \
	-fuse-ld=lld-link

.PHONY: \
	kretlim-uefi-boot \
	kretlim-uefi-boot-clean

# The "entry", will build everything you need
kretlim-uefi-boot: $(BIN_FOLDER)/BOOTX64.EFI

# Clean
kretlim-uefi-boot-clean:
	rm -rf $(BUILD_FOLDER) $(BIN_FOLDER)

# Build the main efi file
$(BIN_FOLDER)/BOOTX64.EFI: $(KRETLIM_UEFI_BOOT_OBJDIRS) $(KRETLIM_UEFI_BOOT_OBJS)
	mkdir -p bin
	$(CC) $(KRETLIM_UEFI_BOOT_LDFLAGS) -o $@ $(KRETLIM_UEFI_BOOT_OBJS)

# Build each of the c files
$(BUILD_FOLDER)/%.o: %.c
	$(CC) $(KRETLIM_UEFI_BOOT_CFLAGS) -o $@ $<

# Create each of the dirs
$(BUILD_FOLDER)/%:
	mkdir -p $@
