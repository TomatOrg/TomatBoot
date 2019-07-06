
# Boot protocol
Everything here is also defined in `kboot.h`

## Enviroment 

One note worth mentioning is that all of the boot info structure, the efi memory map and any other information the bootloader passes to the user, is allocated as EfiRuntimeServicesData, which is technically reserved, but the kernel may choose to ignore that and still use that area (will be easy to know since we do not provide any runtime services).

### CPU Mode
* long mode
* nx bit enabled if possible

### GDT, TSS and IDT
Same as UEFI state after ExitBootServices

### Paging
Described in the boot headers

## Header
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
### Mapping
The mapping allows you to specify how the kernel should be mapped

The possible mapping types are
* `KBOOT_MAPPING_IDENTITY` (0) - we will identity map the whole memory, this means the entry needs to be as a physical address. 
* `KBOOT_MAPPING_VIRTUAL` (1) - we will map the kernel to the higher half and direct map the memory as the `direct_mapping_base` specifies

### Framebuffer
The framebuffer allows you to configure the prefered bpp, width and height. if any of them is specified as zero, the bootloader will choose the highest value it can.

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
    uint64_t counts;
    uint64_t descriptor_size;
    EFI_MEMORY_DESCRIPTOR* entries;
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

### Memory Map
The memory map is the exact copy of the UEFI one.

### Framebuffer
The framebuffer will give the address of the vram (physical) and the dimensions of it

### Command line
The command line is a string that can be defined at boot to change the kernel behaviour dynamically 

### RSDP
The pointer to the RSDP table
