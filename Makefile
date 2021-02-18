########################################################################################################################
# TomatBoot!
########################################################################################################################

# Declare the default target
default: all

########################################################################################################################
# Constants
########################################################################################################################

PROJECT_NAME	:= TomatBoot

#
# Set the c compiler and the fuse linker
#
CC 				:= clang
LD				:= lld-link

#
# Output folders
#
OUT_DIR			:= out
BIN_DIR			:= $(OUT_DIR)/bin
BUILD_DIR		:= $(OUT_DIR)/build

#
# The default cflags
#
CFLAGS			:= -target x86_64-unknown-windows
CFLAGS			+= -Wall -Werror -Os -flto -ggdb
CFLAGS			+= -nostdlib -nostdinc -std=c11
CFLAGS			+= -fshort-wchar -ffreestanding
CFLAGS			+= -Isrc
CFLAGS			+= -Wno-unused-label

#
# Add the git revision
#
CFLAGS			+= -D__GIT_REVISION__=\"$(shell git rev-parse HEAD)\"

#
# The default link flags
#
LDFLAGS			:= -target x86_64-unknown-windows
LDFLAGS			+= -nostdlib -fuse-ld=$(LD)
LDFLAGS			+= -Wl,-entry:EfiMain
LDFLAGS			+= -Wl,-subsystem:efi_application

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
all: $(BIN_DIR)/$(PROJECT_NAME).efi

#
# Link it to a final binary
#
$(BIN_DIR)/$(PROJECT_NAME).efi: $(OBJS)
	@echo LD $@
	@mkdir -p $(@D)
	@$(CC) $(LDFLAGS) -o $@ $^

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
./build/%.nasm.o: %.nasm
	@echo NASM $@
	@mkdir -p $(@D)
	@nasm $(NASMFLAGS) -o $@ $<

#
# just delete the output folder
#
clean:
	rm -rf $(OUT_DIR)
