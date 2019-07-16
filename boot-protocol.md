
# Boot protocol
Everything here is also defined in `kboot.h`

## Enviroment 

### CPU Mode
* long mode
* interrupts are off

### IDT
will be nullified

### GDT, TSS
Same as UEFI state after ExitBootServices

### Paging
Identity mapping of all the memory

## Information structure
The bootloader will treat entry function as the following format:
```c
void (*kboot_entry_function)(uint32_t magic, kboot_info_t* info);
```

The magic is defined as `0xCAFEBABE`

The boot info structure is defined as
```c
struct kboot_info {
  struct {
    uint64_t count;
    kboot_mmap_entry_t* entries;
  } mmap;

  struct {
    uint64_t addr;
    uint32_t width;
    uint32_t height;
  } framebuffer;

  struct {
    uint32_t length;
    char* cmdline;
  } cmdline;
  
  uint64_t rsdp;
}
```

the mmap entry is defined as
```c
struct kboot_mmap_entry {
  uint32_t type;
  uint64_t addr;
  uint64_t len;
}
```

With the types being
* `KBOOT_MEMORY_TYPE_RESERVED` (0) - same as e820 type
* `KBOOT_MEMORY_TYPE_BAD_MEMORY` (1) - same as e820 type
* `KBOOT_MEMORY_TYPE_ACPI_RECLAIM` (2) - same as e820 type
* `KBOOT_MEMORY_TYPE_USABLE` (3) - same as e820 type
* `KBOOT_MEMORY_TYPE_ACPI_NVS` (4) - same as e820 type

### Memory Map
The memory map is created from the UEFI memory map using this table

It is worth noting that the kernel code and data, as well as the boot info structure is marked as reserved.

### Framebuffer

Gives the OS info about a framebuffer it can print to

The framebuffer must be in BGR 32bpp format `0x__RRGGBB` (`_` has no meaning)

#### Address
The address to the start of the framebuffer

#### width/height
The width and height of the framebuffer in pixels

### Command line
The command line is a string that can be defined at boot to change the kernel behaviour dynamically 

### RSDP
The pointer to the RSDP table
