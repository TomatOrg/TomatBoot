#include "menu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uefi/uefi.h>

#define WSTRING_LEN(wstr) (sizeof(wstr) / sizeof(wchar_t))
#define HW (text_width / 2)
#define HH (text_height / 2)
#define CENTER_STR(str) (HW - (WSTRING_LEN(str) / 2))

#define KRETLIM_BOOT L"-Kretlim Boot-"
#define PLEASE_SELECT L"Please select an image."
#define ADDITIONAL_COMMANDS L"Press E to write additional commands."

size_t text_width = 80;
size_t text_height = 25;

static int selected_index = 0;

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
            printf(L"%a", entry->name);
        }else {
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_LIGHTGRAY));
            gST->ConOut->SetCursorPosition(gST->ConOut, HW - lw / 2, HH - lh / 2 + i);
            printf(L"%a", entry->name);
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
            printf(L"%a", entry->command_line);
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

static boot_entry_t* edit_command(boot_config_t* config) {
    boot_entry_t* entry = config->entries;
    for(int i = 0; i < config->entry_count; i++) {
        if(selected_index == i) {
            size_t len = strlen(entry->command_line) - 1;
            gST->ConOut->EnableCursor(gST->ConOut, 1);
            while(1) {
                gST->ConOut->SetCursorPosition(gST->ConOut, (HW - (text_width - 4) / 2) + len, text_height - 1);
                gBS->Stall(10);
                EFI_INPUT_KEY key;
                if(gST->ConIn->ReadKeyStroke(gST->ConIn, &key) != EFI_SUCCESS) continue;
                if(key.ScanCode == EFI_SCANCODE_ESCAPE) {
                    return NULL;
                }else if(key.UnicodeChar == 8) {
                    if(len > 0) {
                        entry->command_line[--len] = 0;
                        draw_command(config);
                    }
                }else if(key.UnicodeChar >= L' ' && key.UnicodeChar <= L'~') {
                    if(len < text_width - 4) {
                        entry->command_line[len] = (char)key.UnicodeChar;
                        len++;
                        entry->command_line[len] = 0;
                        draw_command(config);
                    }
                }else if(key.UnicodeChar == 13) {
                    return entry;
                }
            }
            gST->ConOut->EnableCursor(gST->ConOut, 0);

            return NULL;
        }
        entry = (boot_entry_t*)(((size_t)entry) + entry->size);
    }

    return NULL;
}

boot_entry_t* start_menu(boot_config_t* config) {
    selected_index = config->default_option;

    // render it!
    draw_menu(config);

    // boot menu
    while(1) {
        gBS->Stall(10);
        EFI_INPUT_KEY key;
        if(gST->ConIn->ReadKeyStroke(gST->ConIn, &key) != EFI_SUCCESS) continue;

        if(key.UnicodeChar == L'E' || key.UnicodeChar == L'e') {
            boot_entry_t* entry = edit_command(config);
            if(entry != 0) {
                return entry;
            }
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
                    return entry;
                }
                entry = (boot_entry_t*)(((size_t)entry) + entry->size);
            }
        }
    }
}
