# kretlim-uefi-boot

This bootloader is aimed at simplifying the process of loading a 64bit kernel, it will be done by loading the kernel using the correct virtual addresses specified, and give capabilities like creating a direct mapping at the specified address, giving the efi mmap directly to the kernel and more.

You can find the [boot protocol specs here](boot-protocol.md)

![Boot menu](screenshot.png)

## Current Features
* cool looking boot menu
  * you can choose and edit the command line options of each entry

## Soon:tm:
* actually loading the kernel
* loading the boot list from a file
  * and a tool to create such file