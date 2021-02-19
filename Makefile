########################################################################################################################
# TomatBoot!
########################################################################################################################

# Declare the default target
.PHONY: default
default: all

########################################################################################################################
# Constants
########################################################################################################################

PROJECT_NAME	:= TomatBoot

#
# Set the c compiler and the fuse linker
#
CC 				:= x86_64-w64-mingw32-gcc

#
# Output folders
#
OUT_DIR			:= out
BIN_DIR			:= $(OUT_DIR)/bin
BUILD_DIR		:= $(OUT_DIR)/build

#
# The default cflags
#
CFLAGS			:= -Wall -Werror -Os -flto -ffat-lto-objects -g
CFLAGS			+= -nostdlib -nostdinc -std=c11
CFLAGS			+= -fshort-wchar -ffreestanding
CFLAGS			+= -Isrc
CFLAGS 			+= -Wl,-eEfiMain -Wl,-subsystem,10
CFLAGS			+= -Wno-unused-label

#
# Add the git revision
#
CFLAGS			+= -D__GIT_REVISION__=\"$(shell git rev-parse HEAD)\"

#
# Assembler flags
#
NASMFLAGS		:= -g -f win64

########################################################################################################################
# Sources
########################################################################################################################

#
# Add the main project sources
#
SRCS 			:= $(shell find src/ -name '*.c')
SRCS 			+= $(shell find src/ -name '*.nasm')

#
# Add the auto-generated guid c file
#
SRCS			+= $(BUILD_DIR)/guids.c

#
# The UEFI headers
#
UEFI_HDRS		:= $(shell find edk2/Include -name '*.h')


########################################################################################################################
# EDK2 stuff
########################################################################################################################

#
# EDK2 includes
#
CFLAGS			+= -Iedk2/Include
CFLAGS			+= -Iedk2/Include/X64

#
# Caller base name
#
CFLAGS			+= -DgEfiCallerBaseName=\"$(PROJECT_NAME)\"

#
# EDK2 sources
#
SRCS 			+= $(shell find edk2/Library/ -name '*.c')
SRCS 			+= $(shell find edk2/Library/ -name '*.nasm')

#
# EDK2 PCDs
#

# boolean pcds
EDK2_PCD_BOOL 	:= PcdVerifyNodeInList=FALSE

# uint32 pcds
EDK2_PCD_UINT32	:= PcdMaximumUnicodeStringLength=0
EDK2_PCD_UINT32 += PcdMaximumAsciiStringLength=0
EDK2_PCD_UINT32 += PcdMaximumDevicePathNodeCount=0
EDK2_PCD_UINT32 += PcdMaximumLinkedListLength=0
EDK2_PCD_UINT32 += PcdFixedDebugPrintErrorLevel=0xFFFFFFFF
EDK2_PCD_UINT32 += PcdDebugPrintErrorLevel=0xFFFFFFFF

# uint16 pcds
EDK2_PCD_UINT16	:= PcdUefiFileHandleLibPrintBufferSize=1536

# uint8 pcds
EDK2_PCD_UINT8 	:= PcdDebugPropertyMask=DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED
EDK2_PCD_UINT8 	+= PcdDebugClearMemoryValue=0xAF

# setup the actual full token
CFLAGS 			+= $(EDK2_PCD_UINT32:%=-D_PCD_GET_MODE_32_%)
CFLAGS 			+= $(EDK2_PCD_UINT16:%=-D_PCD_GET_MODE_16_%)
CFLAGS 			+= $(EDK2_PCD_UINT8:%=-D_PCD_GET_MODE_8_%)
CFLAGS 			+= $(EDK2_PCD_BOOL:%=-D_PCD_GET_MODE_BOOL_%)

########################################################################################################################
# Targets
########################################################################################################################

# Get the objects and their dirs
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

# Include the deps if any
DEPS := $(OBJS:%.o=%.d)
-include $(DEPS)

#
# The default target
#
.PHONY: all
all: $(BIN_DIR)/$(PROJECT_NAME).efi

#
# Link it to a final binary
#
$(BIN_DIR)/$(PROJECT_NAME).efi: $(OBJS) | Makefile
	@echo LD $@
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -o $@ $^

#
# specifically for the uefi lib
# we need to generate that file, have it depend on the includes of UEFI
#
$(BUILD_DIR)/guids.c: $(UEFI_HDRS)
	@echo Generating EFI guids
	@mkdir -p $(@D)
	@./scripts/gen_guids.py $@

#
# Build each of the c files
#
$(BUILD_DIR)/%.c.o: %.c
	@echo CC $@
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -MMD -c -o $@ $<

#
# Build each of the c files
#
$(BUILD_DIR)/%.nasm.o: %.nasm
	@echo NASM $@
	@mkdir -p $(@D)
	@nasm $(NASMFLAGS) -o $@ $<

#
# just delete the output folder
#
.PHONY: clean
clean:
	rm -rf $(OUT_DIR)


########################################################################################################################
# Test targets
########################################################################################################################

QEMU_ARGS += -m 4G -smp 4
QEMU_ARGS += -machine q35
QEMU_ARGS += -debugcon stdio
QEMU_ARGS += -monitor telnet:localhost:4321,server,nowait
QEMU_ARGS += --no-shutdown
QEMU_ARGS += --no-reboot

ifeq ($(shell uname -r | sed -n 's/.*\( *Microsoft *\).*/\1/p'), Microsoft)
    QEMU := qemu-system-x86_64.exe
else
    QEMU := qemu-system-x86_64
endif

#
# Target to create the image
#
$(BIN_DIR)/image.img: \
		$(BIN_DIR)/$(PROJECT_NAME).efi \
		./tests/artifacts/example.cfg \
		./tests/artifacts/test.elf \
		./tests/artifacts/image.yaml
	mkdir -p ./image/fat/EFI/BOOT/
	cp $(BIN_DIR)/$(PROJECT_NAME).efi ./image/fat/EFI/BOOT/BOOTX64.EFI
	mkdir -p ./image/ext4/boot
	cp ./tests/artifacts/example.cfg ./image/ext4/boot/tomatboot.cfg
	cp ./tests/artifacts/test.elf ./image/ext4/boot/test.elf
	./tools/image-builder/image-builder.py ./tests/artifacts/image.yaml $(BIN_DIR)/image.img

#
# Target to make an image as a phony target
#
.PHONY: image
image: $(BIN_DIR)/image.img

#
# Target to start in qemu
#
.PHONY: qemu
qemu: $(BIN_DIR)/image.img
	$(QEMU) $(QEMU_ARGS) -bios OVMF.fd -hda $(BIN_DIR)/image.img
