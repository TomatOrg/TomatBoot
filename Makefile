# Sources, all c files basically
SRCS += $(shell find . -name '*.c')
OBJS := $(SRCS:%.c=build/%.o)
OBJDIRS := $(dir $(OBJS))

# include dirs
INCLUDE_DIRS += lib/
INCLUDE_DIRS += lib/libc
INCLUDE_DIRS += src/

# Set the flags
CFLAGS += \
	-target x86_64-unknown-windows \
	-ffreestanding \
	-fno-stack-check \
	-fno-stack-protector \
	-fshort-wchar \
	-mno-red-zone

# Set the include dirs
CFLAGS += $(INCLUDE_DIRS:%=-I%)

# Set the linking flags
LDFLAGS += \
	-target x86_64-unknown-windows \
	-nostdlib \
	-Wl,-entry:EfiMain \
	-Wl,-subsystem:efi_application \
	-fuse-ld=lld-link

.PHONY: all clean BOOTX64.efi BOOTX64.dll

#########################
# Compiling
#########################
all: BOOTX64.EFI

# Shortcuts
BOOTX64.EFI: bin/BOOTX64.EFI 

# Turn the dll into an efi app
bin/BOOTX64.EFI: $(OBJDIRS) $(OBJS)
	mkdir -p bin
	clang $(LDFLAGS) -o bin/BOOTX64.EFI $(OBJS)

build/%.o: %.c
	clang $(CFLAGS) -c -o $@ $<

build/%:
	mkdir -p $@

#########################
# QEMU SHIT REEEEE
#########################
qemu: BOOTX64.EFI OVMF.fd image/EFI/BOOT/BOOTX64.EFI
	qemu-system-x86_64 -bios OVMF.fd -net none -hda fat:rw:image

image/EFI/BOOT/BOOTX64.EFI: bin/BOOTX64.EFI
	rm -rf image
	mkdir -p image/EFI/BOOT
	cp bin/BOOTX64.EFI image/EFI/BOOT/

OVMF.fd:
	wget http://downloads.sourceforge.net/project/edk2/OVMF/OVMF-X64-r15214.zip
	unzip OVMF-X64-r15214.zip OVMF.fd
	rm OVMF-X64-r15214.zip

# Clean everything
clean:
	rm -rf build bin image
