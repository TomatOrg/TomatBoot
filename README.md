# TomatBoot for UEFI

TomatBoot is a simple kernel loader for 64bit UEFI based systems.

The gold of this bootloader is to serve as an example of how to create UEFI applications, 
we use the edk2 headers/libraries without the edk2 buildsystem for simplicity.

![Main Menu](screenshots/mainmenu.png)

## Features

* Boot menu
	* change the framebuffer settings
	* change default entry and delay
* Support for linux boot
* Support for MB2
* Support for Stivale/Stivale2

### Future plans
* allow to edit the configuration file on the fly

## Boot Protocols
### Linux Boot (`linux`)
With linux boot you can give TomatBoot a `vmlinuz` and `initrd` images and it will load it according to the linux 
boot protocol.

It supports both 32bit and a EFI Handover boot.

### Multiboot 2 (`mb2`)
With MB2 boot you can load any mb2 compatible kernel image.

We support:
* Command line
* Boot Modules
* E820 + Efi Memory Map
* ELF32/ELF64 Images + Elf Sections
* Framebuffer
* New/Old ACPI tables

### Stivale (`stivale`)
[Stivale](https://github.com/limine-bootloader/limine/blob/master/STIVALE.md) is a simple boot protocol aimed to provide 
everything a modern x86_64 kernel needs:
* Direct higher half support (with 5 Level paging support)
* Command line
* Boot Modules
* Memory Map
* Framebuffer
* ACPI tables

### Stivale2 (`stivale2`)
[Stivale2](https://github.com/limine-bootloader/limine/blob/master/STIVALE2.md) is a simple boot protocol aimed to provide 
everything an advanced modern x86_64 kernel needs, it includes all provided by `stivale` along side:
* More dynamic features (using a linked list of tags)
* SMP Boot (WIP)

## How to
### Getting the EFI module
First of all the latest EFI module is available to download from [Github Actions as an Artifact](https://github.com/TomatOrg/TomatBoot-UEFI/actions?query=workflow%3ATomatBoot-UEFI).

If you want to build from source then simply run:
```shell script
git clone git@github.com:TomatOrg/TomatBoot-UEFI.git
cd TomatBoot-UEFI
make
```

It will create the module and place it under `bin/BOOTX64.EFI`

### Creating an image
To create a bootable image you will need to have a GPT formatted image with one EFI FAT partition. You will 
need to place the UEFI module under `EFI/BOOT/BOOTX64.EFI` 

Other than the binary, you will also need to provide a configuration file. For an example you can see the 
[example config](config/example.cfg). The config file needs to be placed at the root of the efi partition 
with the name `tomatboot.cfg`


Example file structure inside the UEFI partition:
```
.
├── EFI
│   └── BOOT
│       └── BOOTX64.EFI
├── tomatboot.cfg
└── kernel.elf
```

### Config format
Check [CONFIG.md](CONFIG.md).

## UEFI Library

The uefi library consists mainly of headers and source files taken directly from [EDK2](https://github.com/tianocore/edk2). 
The reason for that is to cut on development time and use existing headers, but not using EDK2 build system.
