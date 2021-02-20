# TomatBoot

TomatBoot is a simple bootlaoder for 64bit UEFI based systems.

The goal of this bootloader is to serve as an example on how to create UEFI applications, with the use of edk2 headers/sources
but without their bootloader for added simplicity.

## Main Features
* Tomato Logo
* Setup Menu
    * Allows to edit to configuration on the fly
* Support for variety of boot protocols
    * Linux boot (32bit and UEFI handover)
    * Multiboot2 
    * Stivale/Stivale2
* EXT2/3/4 filesystem support

## Boot protocols
### Linux boot
With linux boot you can give TomatBoot a `vmlinuz` and `initrd` images and it will load it according to the linux boot
protocol.

It supports both 32bit and a EFI Handover boot.

### Multiboot 2 

With MB2 boot you can load any multiboot2 compatible kernel image.

* Command line
* Boot Modules
* E820 + Efi Memory Map
* ELF32/ELF64 Images + Elf Sections
* Framebuffer
* New/Old ACPI tables

### Stivale/Stivale2

[Stivale](https://github.com/stivale/stivale/blob/master/STIVALE.md) is a simple boot protocols aimed to provide everything a modern 64bit kernel needs:
* Command line
* Boot modules
* Memory map
* Framebuffer
* ACPI Tables
* KASLR
  
[Stivale2](https://github.com/stivale/stivale/blob/master/STIVALE2.md) adds additional features such as:
* SMP Boot
* More dynamic features (using a linked list of tags)
* Direct higher half support (with 5 level paging support)

## How to
### Getting the UEFI module

The latest module can is avaiable to download from [Github Actions as an artifact]().

Alternatively you can build it from source:
```
git clone git@github.com:TomatOrg/TomatBoot.git
cd TomatBoot
make -j8
```

### Creating an image
To create a bootable image you will need to have a GPT formatted image with one EFI FAT partition. You will need to 
place the UEFI module under `EFI/BOOT/BOOTX64.EFI`

Other than the binary, you will also need to provide a configuration file. For an example you can see the [example config](). 
The config file needs to be placed at the root of the efi partition with the name `tomatboot.cfg`.

Example file structure inside the UEFI partition:
```
.
├── EFI
│   └── BOOT
│       └── BOOTX64.EFI
├── tomatboot.cfg
└── kernel.elf
```

## Config format

Check [CONFIG.md](CONFIG.md)
