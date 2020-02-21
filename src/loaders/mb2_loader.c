
#include <config.h>
#include <multiboot2.h>
#include <util/file_utils.h>
#include <Uefi.h>
#include <Library/DebugLib.h>

static UINT8* image;
static UINTN image_length;

#define ASSERT_IF_NOT_OPTIONAL(tag, name) \
    do { \
        if (tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL) { \
            DebugPrint(0, name " ignored\n"); \
        } else { \
            ASSERT(!(name " are not supported!\n")); \
        } \
    } while(0)

#define CHECK_IN_IMAGE(ptr) ASSERT(ptr > image && ptr < image + image_length)

/**
 * Will search for the headers according to the spec
 *
 * ```
 * The Multiboot2 header must be contained completely
 * within the first 32768 bytes of the OS image, and
 * must be 64-bit aligned. In general
 * ```
 */
static struct multiboot_header* search_for_mb2_header() {
    for (int i = 0; i < (MULTIBOOT_SEARCH - sizeof(struct multiboot_header)); i += MULTIBOOT_HEADER_ALIGN) {
        struct multiboot_header* header = (struct multiboot_header*)&image[i];

        // maybe found, verify checksum
        if (header->magic == MULTIBOOT2_HEADER_MAGIC) {
            // check the checksum
            if (header->magic + header->architecture + header->header_length + header->checksum != 0) {
                DebugPrint(0, "Found MB2 header with invalid checksum, ignoring\n");
                continue;
            }

            // check the architecture
            if (header->architecture != MULTIBOOT_ARCHITECTURE_I386) {
                DebugPrint(0, "Found MB2 header with invalid architecture, ignoring\n");
                continue;
            }

            // this should be a valid module :)
            return header;
        }
    }

    ASSERT(!"The MultiBoot2 header was not found!\n");
    return NULL;
}

void load_mb2_binary(boot_entry_t* entry) {
    // start by simply loading it
    load_file(entry->path, EfiLoaderData, (void*)&image, &image_length);

    // get the header
    struct multiboot_header* header = search_for_mb2_header();
    DebugPrint(0, "Header found at %p\n", header);

    // if these are set will override the elf loading/elf entries
    struct multiboot_header_tag_address* address_tag = NULL;
    struct multiboot_header_tag_entry_address* entry_tag = NULL;

    // iterate the tags until the end
    for (struct multiboot_header_tag* tag = (struct multiboot_header_tag*)(header + 1);
         tag < (struct multiboot_header_tag*)((UINTN)header + header->header_length) || tag->type == MULTIBOOT_HEADER_TAG_END;
         tag = (struct multiboot_header_tag*)((UINTN)tag + ALIGN_VALUE(tag->size, 8))) {
        switch (tag->type) {
            /*
             * We are gonna provide everything we can by default, regardless of the request
             * but we still check that we have whatever they want and is not optional by them
             */
            case MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST:

                break;

            /*
             * We support non-elf loading
             */
            case MULTIBOOT_HEADER_TAG_ADDRESS:
                address_tag = (void*)tag;
                break;

            /*
             * we support entry point override
             */
            case MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS:
                entry_tag = (void*)tag;
                break;

            /*
             * We do not support console flags
             * if not optional assert
             */
            case MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS:
                ASSERT_IF_NOT_OPTIONAL(tag, "Console flags");
                break;

            /*
             * We support the framebuffer but we are gonna ignore
             * this and set whatever is setup from the bootloader
             * config
             */
            case MULTIBOOT_HEADER_TAG_FRAMEBUFFER:
                break;

            /*
             * We always load modules at an aligned address so we can
             * ignore this
             */
            case MULTIBOOT_HEADER_TAG_MODULE_ALIGN:
                break;


            // TODO: we can easily add support for these
            case MULTIBOOT_HEADER_TAG_EFI_BS:
                ASSERT_IF_NOT_OPTIONAL(tag, "EFI BootServices");
                break;
            case MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI32:
                ASSERT_IF_NOT_OPTIONAL(tag, "EFI Entry 32");
                break;
            case MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI64:
                ASSERT_IF_NOT_OPTIONAL(tag, "EFI Entry 64");
                break;

            /*
             * TODO: This should be easy enough to add support for
             */
            case MULTIBOOT_HEADER_TAG_RELOCATABLE:
                ASSERT_IF_NOT_OPTIONAL(tag, "Relocatable Image");
                break;
        }
    }

}
