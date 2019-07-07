# CC Should be clang
CC ?= clang-8

# Config on where to build everything
KRETLIM_UEFI_BOOT_DIR ?= $(notdir $(shell pwd)/)
KRETLIM_UEFI_BOOT_DIR_BIN ?= $(KRETLIM_UEFI_BOOT_DIR)bin
KRETLIM_UEFI_BOOT_DIR_BUILD ?= $(KRETLIM_UEFI_BOOT_DIR)build

# Get all the files to build
KRETLIM_UEFI_BOOT_SRCS += $(shell find $(KRETLIM_UEFI_BOOT_DIR)src -name '*.c')
KRETLIM_UEFI_BOOT_SRCS += $(shell find $(KRETLIM_UEFI_BOOT_DIR)lib -name '*.c')

# All the headers, we use them as dependency
KRETLIM_UEFI_BOOT_HDRS += $(shell find $(KRETLIM_UEFI_BOOT_DIR)lib -name '*.h')
KRETLIM_UEFI_BOOT_HDRS += $(shell find $(KRETLIM_UEFI_BOOT_DIR)src -name '*.h')

# Get the objects and their dirs
KRETLIM_UEFI_BOOT_OBJS := $(KRETLIM_UEFI_BOOT_SRCS:%.c=$(KRETLIM_UEFI_BOOT_DIR_BUILD)/%.o)
KRETLIM_UEFI_BOOT_OBJDIRS := $(dir $(KRETLIM_UEFI_BOOT_OBJS))

# Include dirs
KRETLIM_UEFI_BOOT_INCLUDE_DIRS += $(KRETLIM_UEFI_BOOT_DIR)lib/
KRETLIM_UEFI_BOOT_INCLUDE_DIRS += $(KRETLIM_UEFI_BOOT_DIR)lib/libc
KRETLIM_UEFI_BOOT_INCLUDE_DIRS += $(KRETLIM_UEFI_BOOT_DIR)src/

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
kretlim-uefi-boot: $(KRETLIM_UEFI_BOOT_DIR_BIN)/BOOTX64.EFI

# Clean
kretlim-uefi-boot-clean:
	rm -rf $(KRETLIM_UEFI_BOOT_DIR_BUILD) $(KRETLIM_UEFI_BOOT_DIR_BIN)

# Build the main efi file
$(KRETLIM_UEFI_BOOT_DIR_BIN)/BOOTX64.EFI: $(KRETLIM_UEFI_BOOT_OBJDIRS) $(KRETLIM_UEFI_BOOT_OBJS)
	mkdir -p $(KRETLIM_UEFI_BOOT_DIR_BIN)
	$(CC) $(KRETLIM_UEFI_BOOT_LDFLAGS) -o $@ $(KRETLIM_UEFI_BOOT_OBJS)

# Build each of the c files
$(KRETLIM_UEFI_BOOT_DIR_BUILD)/%.o: %.c
	$(CC) $(KRETLIM_UEFI_BOOT_CFLAGS) -o $@ $<

# Create each of the dirs
$(KRETLIM_UEFI_BOOT_DIR_BUILD)/%:
	mkdir -p $@
