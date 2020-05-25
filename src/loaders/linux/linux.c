
#include <util/Except.h>
#include <loaders/Loaders.h>
#include <config/BootEntries.h>

#include <Library/LoadLinuxLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>

/**
 * Implementation References
 * - https://github.com/qemu/qemu/blob/master/hw/i386/x86.c#L333
 * - https://github.com/tianocore/edk2/blob/master/OvmfPkg/Library/PlatformBootManagerLib/QemuKernel.c
 *
 */
EFI_STATUS LoadLinuxKernel(BOOT_ENTRY* Entry) {
    EFI_STATUS Status = EFI_SUCCESS;
    UINTN KernelSize = 0;
    UINT8* KernelImage = NULL;

    Print(L"Loading kernel image\n");
    BOOT_MODULE Module = {
        .Path = Entry->Path,
        .Fs = Entry->Fs,
    };
    CHECK_AND_RETHROW(LoadBootModule(&Module, (UINTN*)&KernelImage, &KernelSize));

    // get the setup size
    UINTN SetupSize = KernelImage[0x1f1];
    if(SetupSize == 0) {
        SetupSize = 4;
    }
    SetupSize  = (SetupSize + 1) * 512;
    CHECK(SetupSize < KernelSize);
    KernelSize -= SetupSize;
    Print(L"Setup Size: 0x%x\n", SetupSize);

    // load the setup
    UINT8* SetupBuf = LoadLinuxAllocateKernelSetupPages(EFI_SIZE_TO_PAGES(SetupSize));
    CHECK(SetupBuf != NULL);
    CopyMem(SetupBuf, KernelImage, SetupSize);
    EFI_CHECK(LoadLinuxCheckKernelSetup(SetupBuf, SetupSize));
    EFI_CHECK(LoadLinuxInitializeKernelSetup(SetupBuf));

    // we don't have a type :(
    SetupBuf[0x210] = 0xF;
    SetupBuf[0x211] = 0xF;

    // load the kernel
    UINT64 KernelInitialSize  = LoadLinuxGetKernelSize(SetupBuf, KernelSize);
    CHECK(KernelInitialSize  != 0);
    Print(L"Kernel size: 0x%x\n", KernelSize);
    UINT8* KernelBuf = LoadLinuxAllocateKernelPages(SetupBuf, EFI_SIZE_TO_PAGES(KernelInitialSize));
    CHECK(KernelBuf != NULL);
    CopyMem(KernelBuf, KernelImage + SetupSize, KernelSize);

    // we can free the kernel image now
    FreePages(KernelImage, EFI_SIZE_TO_PAGES(KernelSize));
    KernelImage = NULL;

    // load the command line arguments, if any
    CHAR8* CommandLineBuf = NULL;
    if(Entry->Cmdline) {
        Print(L"Command line: `%s`\n", Entry->Cmdline);
        UINTN CommandLineSize = StrLen(Entry->Cmdline) + 1;
        CommandLineBuf = LoadLinuxAllocateCommandLinePages(EFI_SIZE_TO_PAGES(CommandLineSize));
        CHECK(CommandLineBuf != NULL);
        UnicodeStrToAsciiStr(Entry->Cmdline, CommandLineBuf);
    }
    EFI_CHECK(LoadLinuxSetCommandLine(SetupBuf, CommandLineBuf));

    // TODO: don't assume the first module is the initrd
    // load the initrd, if any
    UINTN InitrdSize = 0;
    UINT8* InitrdBuf = NULL;
    if(!IsListEmpty(&Entry->BootModules)) {
        BOOT_MODULE* InitrdModule = BASE_CR(Entry->BootModules.ForwardLink, BOOT_MODULE, Link);

        UINT8* InitrdBase;
        LoadBootModule(InitrdModule, (UINTN*)&InitrdBase, &InitrdSize);
        Print(L"Initrd size: 0x%x\n", InitrdSize);

        InitrdBuf = LoadLinuxAllocateInitrdPages(SetupBuf, EFI_SIZE_TO_PAGES(InitrdSize));
        CHECK(InitrdBuf != NULL);
        Print(L"Initrd Buf: 0x%p\n", InitrdBuf);
        CopyMem(InitrdBuf, InitrdBase, InitrdSize);

        // can free the initrd now
        FreePages(InitrdBase, EFI_SIZE_TO_PAGES(InitrdSize));
        InitrdBase = NULL;
    }

    Print(L"Loading Initrd...");
    EFI_CHECK(LoadLinuxSetInitrd(SetupBuf, InitrdBuf, InitrdSize));
    Print(L" Dones\n");

    // call the kernel
    Print(L"Calling linux");
    EFI_CHECK(LoadLinux(KernelBuf, SetupBuf));

cleanup:
    if (KernelImage != NULL) {
        FreePages(KernelImage, KernelSize);
    }
    return Status;
}
