# Make sure we use the correct compilers
CLANG ?= clang
FUSE_LD ?= lld-link

default: all

.PHONY: all clean

########################################################################################################################
# All the source files
# TODO: support for Ia32
########################################################################################################################

# Sources for the core
SRCS += $(shell find src/ -name '*.c')
SRCS += $(shell find src/ -name '*.nasm')

# Runtime support
SRCS += $(shell find lib/runtime -name '*.c')

# UEFI Lib
SRCS += $(shell find lib/uefi/Library -name '*.c')
SRCS += $(shell find lib/uefi/Library -name '*.nasm')

# Make sure we build the guids c file if it does not exists
SRCS += lib/uefi/Library/guids.c

# Get the objects and their dirs
OBJS := $(SRCS:%=./build/%.o)
DEPS := $(OBJS:%.o=%.d)

# The uefi headers, as dependency for the guids generation
UEFI_HDRS := $(shell find lib/uefi/Include -name '*.h')

########################################################################################################################
# Include directories
# TODO: support for Ia32
########################################################################################################################

INCLUDE_DIRS += lib/uefi/Include
INCLUDE_DIRS += lib/uefi/Include/X64
INCLUDE_DIRS += src/

########################################################################################################################
# EDK2 flags
########################################################################################################################

# These are taken from the default configurations
# of the MdePkg of edk2

# boolean options
EDK2_OPTS_BOOL := PcdVerifyNodeInList=FALSE
EDK2_OPTS_BOOL += PcdComponentNameDisable=FALSE
EDK2_OPTS_BOOL += PcdDriverDiagnostics2Disable=FALSE
EDK2_OPTS_BOOL += PcdComponentName2Disable=FALSE
EDK2_OPTS_BOOL += PcdDriverDiagnosticsDisable=FALSE
EDK2_OPTS_BOOL += PcdUgaConsumeSupport=TRUE

# uint32 options
EDK2_OPTS_UINT32 := PcdMaximumLinkedListLength=1000000
EDK2_OPTS_UINT32 += PcdMaximumUnicodeStringLength=1000000
EDK2_OPTS_UINT32 += PcdMaximumAsciiStringLength=1000000
EDK2_OPTS_UINT32 += PcdSpinLockTimeout=10000000
EDK2_OPTS_UINT32 += PcdFixedDebugPrintErrorLevel=0xFFFFFFFF
EDK2_OPTS_UINT32 += PcdUefiLibMaxPrintBufferSize=320
EDK2_OPTS_UINT32 += PcdMaximumDevicePathNodeCount=0
EDK2_OPTS_UINT32 += PcdCpuLocalApicBaseAddress=0xfee00000
EDK2_OPTS_UINT32 += PcdCpuInitIpiDelayInMicroSeconds=10000

EDK2_OPTS_UINT16 := PcdUefiFileHandleLibPrintBufferSize=1536

EDK2_OPTS_UINT8 := PcdSpeculationBarrierType=0x1
EDK2_OPTS_UINT8 += PcdDebugPropertyMask=DEBUG_PROPERTY_ASSERT_BREAKPOINT_ENABLED
EDK2_OPTS_UINT8 += PcdDebugClearMemoryValue=0xAF

# setup the actual full token
EDK2_FLAGS := $(EDK2_OPTS_UINT32:%=-D_PCD_GET_MODE_32_%)
EDK2_FLAGS += $(EDK2_OPTS_UINT16:%=-D_PCD_GET_MODE_16_%)
EDK2_FLAGS += $(EDK2_OPTS_UINT8:%=-D_PCD_GET_MODE_8_%)
EDK2_FLAGS += $(EDK2_OPTS_BOOL:%=-D_PCD_GET_MODE_BOOL_%)

########################################################################################################################
# Flags
########################################################################################################################

# The flags
CFLAGS := \
	-target x86_64-unknown-windows \
	-ffreestanding \
	-fshort-wchar \
	-nostdinc \
	-nostdlib \
	-std=c11 \
	-Wall \
	-Werror \
	-Os \
	-flto \
	-g

CFLAGS += -D__GIT_REVISION__=\"$(shell git rev-parse HEAD)\"

# Add all the includes
CFLAGS += $(INCLUDE_DIRS:%=-I%)

# Set the linking flags
LDFLAGS := \
	-target x86_64-unknown-windows \
	-nostdlib \
	-Wl,-entry:EfiMain \
	-Wl,-subsystem:efi_application \
	-fuse-ld=$(FUSE_LD)

# Assembler flags
NASMFLAGS := \
	-g \
	-f win64

# Add includes
NASMFLAGS += $(INCLUDE_DIRS:%=-i %/)

########################################################################################################################
# Cleaning
########################################################################################################################

# Clean
clean:
	rm -rf ./build ./bin

clean-all: clean
	rm -rf ./image

########################################################################################################################
# Actual build process
########################################################################################################################

# Include all deps
-include $(DEPS)

all: ./bin/BOOTX64.EFI

# Link the main efi file
./bin/BOOTX64.EFI: $(OBJS)
	@echo LD $@
	@mkdir -p $(@D)
	@$(CLANG) $(LDFLAGS) -o $@ $(OBJS)

# specifically for the uefi lib
# we need to generate that file, have it depend on the includes of UEFI
./lib/uefi/Library/guids.c: $(UEFI_HDRS)
	@echo Generating EFI guids
	@mkdir -p $(@D)
	@cd ./lib/uefi/Library && python gen_guids.py

# specifically for the uefi lib as well
# add the edk2 pcd flags
./build/lib/uefi/%.c.o: lib/uefi/%.c
	@echo CC $@
	@mkdir -p $(@D)
	@$(CLANG) $(CFLAGS) $(EDK2_FLAGS) -c -o $@ $<

# Build each of the c files
./build/%.c.o: %.c
	@echo CC $@
	@mkdir -p $(@D)
	@$(CLANG) $(CFLAGS) -D__FILENAME__="\"$<\"" -D__MODULE__="\"$(notdir $(basename $<))\"" -MMD -c -o $@ $<

# Build each of the c files
./build/%.nasm.o: %.nasm
	@echo NASM $@
	@mkdir -p $(@D)
	@nasm $(NASMFLAGS) -o $@ $<

########################################################################################################################
# Test with qemu (manually)
########################################################################################################################

QEMU_ARGS += -m 4G -smp 4
QEMU_ARGS += -machine q35
QEMU_ARGS += -debugcon stdio
QEMU_ARGS += -monitor telnet:localhost:4321,server,nowait
QEMU_ARGS += --no-shutdown
QEMU_ARGS += --no-reboot
QEMU_ARGS += -cpu host

ifeq ($(shell uname -r | sed -n 's/.*\( *Microsoft *\).*/\1/p'), Microsoft)
    QEMU := qemu-system-x86_64.exe
#    ifneq ($(DEBUGGER), 1)
#        QEMU_ARGS += --accel whpx
#    endif
else
    QEMU := qemu-system-x86_64
	QEMU_ARGS += --enable-kvm
endif

tools/OVMF.fd:
	rm -f OVMF-X64.zip
	mkdir -p ./tools
	wget -P ./tools https://efi.akeo.ie/OVMF/OVMF-X64.zip
	cd tools && unzip OVMF-X64.zip
	rm -f BUILD_INFO OVMF-X64.zip README

image: ./bin/BOOTX64.EFI ./config/example.cfg
	mkdir -p ./image/EFI/BOOT/
	cp ./bin/BOOTX64.EFI ./image/EFI/BOOT/BOOTX64.EFI
	cp ./config/example.cfg ./image/tomatboot.cfg
	cp ./tests/binaries/test.elf ./image/test.elf


qemu: image tools/OVMF.fd
	$(QEMU) $(QEMU_ARGS) -L tools -bios OVMF.fd -hda fat:rw:image

########################################################################################################################
# Pytest tests
########################################################################################################################

