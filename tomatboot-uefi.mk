# CC Should be clang
CC ?= clang-8

#########################
# Configuration
#########################

TOMATBOOT_UEFI_DIR ?= $(notdir $(shell pwd)/)
TOMATBOOT_UEFI_DIR_BIN ?= $(TOMATBOOT_UEFI_DIR)bin/
TOMATBOOT_UEFI_DIR_BUILD ?= $(TOMATBOOT_UEFI_DIR)build/

#########################
# All the source files
#########################

TOMATBOOT_UEFI_SRCS += $(shell find $(TOMATBOOT_UEFI_DIR)src/ -name '*.c')
TOMATBOOT_UEFI_SRCS += $(shell find $(TOMATBOOT_UEFI_DIR)lib/ -name '*.c')

# All the headers, we use them as dependency
TOMATBOOT_UEFI_HDRS += $(shell find $(TOMATBOOT_UEFI_DIR)lib/ -name '*.h')
TOMATBOOT_UEFI_HDRS += $(shell find $(TOMATBOOT_UEFI_DIR)src/ -name '*.h')

# Get the objects and their dirs
TOMATBOOT_UEFI_OBJS := $(TOMATBOOT_UEFI_SRCS:%.c=$(TOMATBOOT_UEFI_DIR_BUILD)/%.o)
TOMATBOOT_UEFI_OBJDIRS := $(dir $(TOMATBOOT_UEFI_OBJS))

#########################
# Include directories
#########################

TOMATBOOT_UEFI_INCLUDE_DIRS += $(TOMATBOOT_UEFI_DIR)lib/
TOMATBOOT_UEFI_INCLUDE_DIRS += $(TOMATBOOT_UEFI_DIR)lib/libc/
TOMATBOOT_UEFI_INCLUDE_DIRS += $(TOMATBOOT_UEFI_DIR)src/

#########################
# Shutdown module
#########################

TOMATBOOT_SHUTDOWN_DIR ?= $(TOMATBOOT_UEFI_DIR)modules/shutdown/

include $(TOMATBOOT_UEFI_DIR)modules/shutdown/tomatboot-shutdown.mk

#########################
# Flags
#########################

# The flags 
TOMATBOOT_UEFI_CFLAGS += \
	-target x86_64-unknown-windows \
	-ffreestanding \
	-fno-stack-check \
	-fno-stack-protector \
	-fshort-wchar \
	-g \
	-mno-red-zone \
	-Werror

# Add all the includes
TOMATBOOT_UEFI_CFLAGS += $(TOMATBOOT_UEFI_INCLUDE_DIRS:%=-I%)

# Set the linking flags
TOMATBOOT_UEFI_LDFLAGS += \
	-target x86_64-unknown-windows \
	-nostdlib \
	-Wl,-entry:EfiMain \
	-Wl,-subsystem:efi_application \
	-fuse-ld=lld-link

.PHONY: \
	tomatboot-uefi-clean

#########################
# Cleaning
#########################

# Clean
tomatboot-uefi-clean: tomatboot-shutdown-clean
	rm -rf $(TOMATBOOT_UEFI_DIR_BUILD) $(TOMATBOOT_UEFI_DIR_BIN)

#########################
# Actual build process
#########################

# Build the main efi file
$(TOMATBOOT_UEFI_DIR_BIN)/BOOTX64.EFI: $(TOMATBOOT_SHUTDOWN_DIR_BIN)/shutdown.elf $(TOMATBOOT_UEFI_OBJDIRS) $(TOMATBOOT_UEFI_OBJS)
	mkdir -p $(TOMATBOOT_UEFI_DIR_BIN)
	$(CC) $(TOMATBOOT_UEFI_LDFLAGS) -o $@ $(TOMATBOOT_UEFI_OBJS)

# Build each of the c files
$(TOMATBOOT_UEFI_DIR_BUILD)/%.o: %.c
	$(CC) $(TOMATBOOT_UEFI_CFLAGS) -c -o $@ $<

# Create each of the dirs
$(TOMATBOOT_UEFI_DIR_BUILD)/%:
	mkdir -p $@
