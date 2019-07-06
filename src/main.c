#include <uefi/uefi.h>
#include <kboot/kboot.h>

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <kboot/map.h>
#include <kboot/buf.h>

typedef struct boot_module_entry {
    const wchar_t path[256];
} __attribute__((packed)) boot_module_entry_t;

typedef struct boot_entry {
    size_t size;
    wchar_t name[256];
    wchar_t filename[256];
    wchar_t command_line[256];
    size_t modules_count;
    boot_module_entry_t boot_modules[0];
} __attribute__((packed)) boot_entry_t;

typedef struct boot_config {
    size_t default_option;
    size_t entry_count;
    boot_entry_t entries[0];
} __attribute__((packed)) boot_config_t;

static boot_config_t* get_boot_config() {
    // TODO load from the file

    boot_config_t* config = calloc(sizeof(boot_config_t) + sizeof(boot_entry_t) * 2, 1);
    config->default_option = 0;
    config->entry_count = 2;
    config->entries[0].size = sizeof(boot_entry_t);
    memcpy(config->entries[0].name, L"Kretlim", sizeof(L"Kretlim"));
    memcpy(config->entries[0].filename, L"kretlim.elf", sizeof(L"kretlim.elf"));
    memcpy(config->entries[0].command_line, L"", sizeof(L""));

    config->entries[1].size = sizeof(boot_entry_t);
    memcpy(config->entries[1].name, L"Kretlim Debug", sizeof(L"Kretlim Debug"));
    memcpy(config->entries[1].filename, L"kretlim.elf", sizeof(L"kretlim.elf"));
    memcpy(config->entries[1].command_line, L"logger=debug", sizeof(L"logger=debug"));
    return config;
}

static void do_boot(boot_entry_t* entry);

static size_t text_width = 80, text_height = 25;

static void clear_menu() {
    gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0);
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
    gST->ConOut->ClearScreen(gST->ConOut);
}

static void clear_box(int sx, int sy, int width, int height) {
    for(int y = sy; y < sy + height; y++) {
        for(int x = sx; x < sx + width; x++) {
            //printf(L"%d, %d\n\r", x, text_height - y);
            gST->ConOut->SetCursorPosition(gST->ConOut, x, y);
            gST->ConOut->OutputString(gST->ConOut, L" ");
        }
    }
}

#define WSTRING_LEN(wstr) (sizeof(wstr) / sizeof(wchar_t))
#define HW (text_width / 2)
#define HH (text_height / 2)
#define CENTER_STR(str) (HW - (WSTRING_LEN(str) / 2))

#define KRETLIM_BOOT L"-Kretlim Boot-"
#define PLEASE_SELECT L"Please select an image."
#define ADDITIONAL_COMMANDS L"Press E to write additional commands."

static int selected_index = 0;

static void draw_list(boot_config_t* config) {
    int lw = WSTRING_LEN(PLEASE_SELECT), lh = text_height - (4 + 4);
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_LIGHTGRAY));
    clear_box(HW - lw / 2, HH - lh / 2, lw, lh);

    boot_entry_t* entry = config->entries;
    for(int i = 0; i < config->entry_count; i++) {
        if(selected_index == i) {
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLUE));
            clear_box(HW - lw / 2, HH - lh / 2 + i, lw, 1);
            gST->ConOut->SetCursorPosition(gST->ConOut, HW - lw / 2, HH - lh / 2 + i);
            gST->ConOut->OutputString(gST->ConOut, entry->name);
        }else {
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_LIGHTGRAY));
            gST->ConOut->SetCursorPosition(gST->ConOut, HW - lw / 2, HH - lh / 2 + i);
            gST->ConOut->OutputString(gST->ConOut, entry->name);
        }

        entry = (boot_entry_t*)(((size_t)entry) + entry->size);
    }
}

static void draw_command(boot_config_t* config) {
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_LIGHTGRAY));
    clear_box(HW - (text_width - 4) / 2, text_height - 1, text_width - 4, 1);

    boot_entry_t* entry = config->entries;
    for(int i = 0; i < config->entry_count; i++) {
        if(selected_index == i) {
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGREEN, EFI_LIGHTGRAY));
            gST->ConOut->SetCursorPosition(gST->ConOut, HW - (text_width - 4) / 2, text_height - 1);
            gST->ConOut->OutputString(gST->ConOut, entry->command_line);
            break;
        }
        entry = (boot_entry_t*)(((size_t)entry) + entry->size);
    }
}

static void draw_menu(boot_config_t* config) {
    clear_menu();
    
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
    gST->ConOut->SetCursorPosition(gST->ConOut, CENTER_STR(KRETLIM_BOOT), 0);
    gST->ConOut->OutputString(gST->ConOut, KRETLIM_BOOT);
    gST->ConOut->SetCursorPosition(gST->ConOut, CENTER_STR(PLEASE_SELECT), 1);
    gST->ConOut->OutputString(gST->ConOut, PLEASE_SELECT);
    gST->ConOut->SetCursorPosition(gST->ConOut, CENTER_STR(ADDITIONAL_COMMANDS), text_height - 2);
    gST->ConOut->OutputString(gST->ConOut, ADDITIONAL_COMMANDS);

    draw_list(config);
    draw_command(config);
}

static void edit_command(boot_config_t* config) {
    boot_entry_t* entry = config->entries;
    for(int i = 0; i < config->entry_count; i++) {
        if(selected_index == i) {
            size_t len = wcslen(entry->command_line) - 1;
            gST->ConOut->EnableCursor(gST->ConOut, 1);
            while(1) {
                gST->ConOut->SetCursorPosition(gST->ConOut, (HW - (text_width - 4) / 2) + len, text_height - 1);
                gBS->Stall(10);
                EFI_INPUT_KEY key;
                if(gST->ConIn->ReadKeyStroke(gST->ConIn, &key) != EFI_SUCCESS) continue;
                if(key.ScanCode == EFI_SCANCODE_ESCAPE) {
                    break;
                }else if(key.UnicodeChar == 8) {
                    if(len > 0) {
                        entry->command_line[--len] = 0;
                        draw_command(config);
                    }
                }else if(key.UnicodeChar >= L' ' && key.UnicodeChar <= L'~') {
                    if(len < text_width - 4) {
                        entry->command_line[len] = key.UnicodeChar;
                        len++;
                        entry->command_line[len] = 0;
                        draw_command(config);
                    }
                }else if(key.UnicodeChar == 13) {
                    do_boot(entry);
                }
            }
            gST->ConOut->EnableCursor(gST->ConOut, 0);

            break;
        }
        entry = (boot_entry_t*)(((size_t)entry) + entry->size);
    }
}

static void do_boot(boot_entry_t* entry) {
    clear_menu();
    printf(L"Booting into `%s` from file `%s`\n\r", entry->name, entry->filename);
    printf(L"Command line `%s`\n\r", entry->command_line);
    while(1);
}

/**
 * Most of the implementation is here
 */
EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    gImageHandle = ImageHandle;
    clear_menu();

    // read the boot config
    boot_config_t* config = get_boot_config();

    draw_menu(config);

    // boot menu
    while(1) {
        gBS->Stall(10);
        EFI_INPUT_KEY key;
        if(gST->ConIn->ReadKeyStroke(gST->ConIn, &key) != EFI_SUCCESS) continue;

        if(key.UnicodeChar == L'E' || key.UnicodeChar == L'e') {
            edit_command(config);
        }else if(key.ScanCode == EFI_SCANCODE_DOWN) {
            if(selected_index < config->entry_count - 1) {
                selected_index++;
                draw_list(config);
                draw_command(config);
            }
        }else if(key.ScanCode == EFI_SCANCODE_UP) {
            if(selected_index > 0) {
                selected_index--;
                draw_list(config);
                draw_command(config);
            }
        }else if(key.UnicodeChar == 13) {
            boot_entry_t* entry = config->entries;
            for(int i = 0; i < config->entry_count; i++) {
                if(selected_index == i) {
                    do_boot(entry);
                }
                entry = (boot_entry_t*)(((size_t)entry) + entry->size);
            }
        }
    }
    return EFI_SUCCESS;
}