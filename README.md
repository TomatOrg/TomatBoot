# kretlim-uefi-boot

This bootloader is aimed at simplifying the process of loading a 64bit kernel, it will be done by loading the kernel using the correct virtual addresses specified, and give capabilities like creating a direct mapping at the specified address, giving the efi mmap directly to the kernel and more.

## Features (soon:tm:)
* load directly into long mode, either using identity mapping or kernel + direct mapping
* get the full uefi memory map
* get the rsdp
* get and set the framebuffer
* load elf64 binaries
* boot modules support
* figure the boot drive (still need to figure out how)

## Boot protocol
Everything here is also defined in `kboot.h`

### Enviroment 

#### CPU Mode
* long mode
* nx bit enabled if possible

#### GDT
The GDT will have the following entries:
* NULL entry
* Kernel Code (64bit)
* Kernel Data (64bit)
* User data (64bit)
* User code (64bit)

#### TSS
Same as UEFI state after ExitBootServices

#### IDT
Same as UEFI state after ExitBootServices

#### Paging
Described in the boot headers

### Header
You will need to have a section in your binary named `.kretlim-boot` and it is defined as follows
```c
struct kboot_header {
  struct {
      uint8_t type;
      uint64_t direct_mapping_base;
  } mapping;
  
  struct {
    uint8_t bpp;
    uint32_t width;
    uint32_t height;
  } framebuffer;
};
```
#### Mapping
The mapping allows you to specify how the kernel should be mapped

The possible mapping types are
* `KBOOT_MAPPING_IDENTITY` (0) - we will identity map the whole memory, this means the entry needs to be as a physical address. 
* `KBOOT_MAPPING_VIRTUAL` (1) - we will map the kernel to the higher half and direct map the memory as the `direct_mapping_base` specifies

#### Framebuffer
The framebuffer allows you to configure the prefered bpp, width and height. if any of them is specified as zero, the bootloader will choose the highest value it can.

### Information structure
The bootloader will treat entry function as the following format:
```c
void (*kboot_entry_function)(uint32_t magic, kboot_info_t* info);
```

The magic is defined as `0xCAFEBABE`

The boot info structure is defined as
```c
struct kboot_mmap_entry {
  uint64_t base;
  uint64_t length;
  uint8_t type;
}

struct kboot_info {
  struct {
    uint64_t counts;
    kboot_mmap_entry* entries;
  } mmap;
  struct {
    uint64_t framebuffer_addr;
    uint32_t width;
    uint32_t height;
    uint32_t bpp;
  } framebuffer;
  struct {
    uint32_t length;
    char* cmdline;
  } cmdline;
  uint64_t rsdp;
}
```

#### Memory Map
The memory map consists of entries, it is made directly from the efi mmap

The possible entry types are:
* `KBOOT_MMAP_TYPE_RESERVED` (0) - this area is reserved and should not be used by the kernel
* `KBOOT_MMAP_TYPE_BAD_MEMORY` (1) - this is bad memory which should not be used
* `KBOOT_MMAP_TYPE_ACPI_RECLAIM` (2) - this is area which can be used by the kernel after reading the acpi tables
* `KBOOT_MMAP_TYPE_USABLE` (3) - the kernel can use this area
* `KBOOT_MMAP_TYPE_ACPI_NVS` (4) - this is an area which should be saved when hibernating (ACPI related)
* `KBOOT_MMAP_TYPE_KBOOT_RECLAIMABLE` (5) - this is where kboot stored stuff like the kboot info, the paging, the gdt and other stuff, if you plan to switch them all, then you can simply treat it as useable 

#### Framebuffer
The framebuffer will give the address of the vram (physical) and the dimensions of it

#### Command line
The command line is a string that can be defined at boot to change the kernel behaviour dynamically 

#### RSDP
The pointer to the RSDP table
