# kretlim-uefi-boot

This bootloader is aimed at simplifying the process of loading a 64bit kernel, it will be done by loading the kernel using the correct virtual addresses specified, and give capabilities like creating a direct mapping at the specified address, giving the efi mmap directly to the kernel and more.

You can find the [boot protocol specs here](boot-protocol.md)

![Boot menu](screenshot.png)

## Current Features
* cool looking boot menu
    * allows to select between different boot options
    * allows to modify the command line for each option
* kernel loading
    * elf64 binaries
* boot info that contains:
    * full EFI Memory Map
    * the framebuffer address and size
    * command line options
    * RSDP

## Soon:tm:
* loading the boot list from a file
  * and a tool to create such file

## Building

You will need
* clang
* qemu (for running)

All output binaries will be located in `bin`, all intermidiate files will be located in `build`

To build the main bootloader simply run `make` and you should get the build EFI file in the `bin` folder

To run in qemu you can use `make qemu`, this will create an `image` folder where it will put everything it needs and run qemu. This will also build the shutdown module.