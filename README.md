# TomatBoot for UEFI

TomatBoot is a simple kernel loader for 64bit UEFI based systems.

The idea of TomatBoot is to provide a simple interface for kernel developers to load their kernel into a 64bit environment 
(unlike most bootloaders which load you to 32bit). The reason for using a loader and not implementing UEFI directly is so
you won't have to mix UEFI and your kernel code.

![Main Menu](screenshots/mainmenu.png)

![Setup Menu](screenshots/setup.png)

![Boot Menu](screenshots/bootmenu.png)

## Features

* Boot menu
	* change width and height
	* change default entry and delay 
	* modify the command line at boot
* Support for static ELF64 kernels
	* the kernel entry must be sysv abi
* Passing boot information to the kernel
	* Command line
	* Framebuffer
	* ACPI table 
	* Memory Map (to be changed)
	* Boot modules (additional files to load) (not implemented yet)
	* Boot Device Path (?) (not implemented yet)

## UEFI Library

The uefi library consists mainly of headers and source files taken directly from [EDK2](https://github.com/tianocore/edk2). The reason for that is 
to cut on development time and use existing headers, but not using EDK2 build system.

The reason that I am not just using a submodle of edk2 is for two reasons:
* EDK2 is HUGE and I don't want the builder to have to clone that all
* I make some changes to make it actually work nicely without EDK2 build system

The license of EDK2 can be found [here](lib/uefi/License.txt) and is redistributed under that license. Note that the rest of the code is
distributed as the main license file shows.
