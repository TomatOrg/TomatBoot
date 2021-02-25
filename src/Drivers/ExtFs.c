
#include <Uefi.h>

#include <Protocol/DiskIo.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/SimpleFileSystem.h>

#include <Guid/FileInfo.h>

#include <Library/BaseLib.h>
#include <Library/TimeBaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Util/Except.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Ext structures
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma pack(1)

typedef struct _EXT_DIRECTORY_ENTRY {
    UINT32 INode;
    UINT16 EntryLength;
    UINT8 NameLength;
    UINT8 FileType;
    CHAR8 Name[0];
} EXT_DIRECTORY_ENTRY;

typedef struct _EXT_SUPER_BLOCK {
    UINT32 TotalInodes;
    UINT32 TotalBlocks;
    UINT32 ReservedBlocks;
    UINT32 FreeBlocks;
    UINT32 FreeInodes;
    UINT32 FirstBlock;
    UINT32 BlockSize;
    UINT32 FragSize;
    UINT32 BlocksPerGroup;
    UINT32 FragsPerGroup;
    UINT32 INodesPerGroup;
    UINT32 LastMountTime;
    UINT32 LastWriteTime;
    UINT16 MountCount;
    UINT16 MaxMountCount;
    UINT16 Magic;
    UINT16 StatusFlag;
    UINT16 ErrorDo;
    UINT16 MinorRev;
    UINT32 LastCheckTime;
    UINT32 CheckInterval;
    UINT32 CreatorOS;
    UINT32 Revision;
    UINT16 DefaultUID;
    UINT16 DefaultGID;
    UINT32 FirstINode;
    UINT16 INodeStructSize;
    UINT16 BlockGroupNo;
    UINT32 CompFeatMap;
    UINT32 IncompFeatMap;
    UINT32 ROCompatFeat;
    EFI_GUID UUID;
    CHAR8 VolumeName[16];
    CHAR8 LastPath[64];
    UINT32 AlgoBitmap;
    UINT8 PreAllocBlocks;
    UINT8 PreallocBlocksDir;
    UINT8 Padding[512 - 0xCE];
} EXT_SUPER_BLOCK;
STATIC_ASSERT(sizeof(EXT_SUPER_BLOCK) == 512, "Invalid super block size");

typedef struct _EXT_BGDT {
    UINT32 BlockBitmapBlk;
    UINT32 INodeBitmapBlk;
    UINT32 INodeTableBlk;
    UINT16 FreeBlocks;
    UINT16 FreeINodes;
    UINT16 UsedDirs;
    UINT16 Padding;
    UINT8 Reserved[12];
} EXT_BGDT;

typedef struct _EXT4_HDR {
    UINT16 Magic;
    UINT16 Extends;
    UINT16 Max;
    UINT16 Depth;
    UINT32 Generation;
} EXT4_HDR;

typedef struct _EXT4_EXT {
    UINT32 LogBlk;
    UINT16 Length;
    UINT16 BlockHi;
    UINT32 BlockLo;
} EXT4_EXT;

typedef struct _EXT_INODE_TABLE {
    UINT16 Type;
    UINT16 UID;
    UINT32 SizeLo;
    UINT32 ATime;
    UINT32 CTime;
    UINT32 MTime;
    UINT32 DTime;
    UINT16 GID;
    UINT16 Links;
    UINT32 ListSize;
    UINT32 Flags;
    UINT32 OSD1;
    union {
        struct {
            UINT32 Blocks[15];
        } Ext2;
        struct {
            EXT4_HDR Header;
            EXT4_EXT Extend[4];
        } Ext4;
    } Alloc;
    UINT32 Version;
    UINT32 FileACL;
    UINT32 SizeHi;
    UINT32 Fragment;
    UINT32 OSD2[3];
} EXT_INODE_ENTRY;

#pragma pack()

/**
 * Check if the super blocks looks correct by making sure the magic and numbers
 * match up nicely
 */
static BOOLEAN IsExt(EXT_SUPER_BLOCK* Sb) {
    return Sb->Magic == 0xEF53 &&
            Sb->BlockSize < 4 &&
            Sb->FirstBlock < 2 &&
            Sb->FreeBlocks < Sb->TotalBlocks &&
            Sb->FreeInodes < Sb->TotalInodes;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Simple file system interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct _EXT_VOLUME {
    // The file system instance
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL VolumeInterface;

    // the handle of this protocol
    EFI_HANDLE Handle;

    // the protocols to access the disk
    UINT32 MediaId;
    EFI_DISK_IO_PROTOCOL* DiskIo;
    EFI_BLOCK_IO_PROTOCOL* BlockIo;
    UINT32 BlockSize;

    // the super block, cache it cause why not
    EXT_SUPER_BLOCK SuperBlock;
    EXT_BGDT Bgdt;
} EXT_VOLUME;

typedef struct _EXT_FILE {
    // the file interface
    EFI_FILE_PROTOCOL FileInterface;

    // the volume
    EXT_VOLUME* Volume;

    // the inode
    EXT_INODE_ENTRY INode;

    // the file info, derived from the inode
    EFI_FILE_INFO* FileInfo;

    // offset in the file
    UINT64 Offset;
} EXT_FILE;

static EFI_STATUS ExtReadINode(EXT_VOLUME* Volume, int inode, EXT_INODE_ENTRY* INode) {
    EFI_STATUS Status = EFI_SUCCESS;

    // 1 based
    inode -= 1;

    // The root directory inode is always the seconds inode
    // in the inode table
    UINTN InodeBlock = Volume->Bgdt.INodeTableBlk;
    UINTN InodeSize = Volume->SuperBlock.INodeStructSize;
    EFI_CHECK(Volume->DiskIo->ReadDisk(Volume->DiskIo, Volume->MediaId,
                                       Volume->BlockSize * InodeBlock + InodeSize * inode, sizeof(EXT_INODE_ENTRY), INode));


cleanup:
    return Status;
}

static EFI_STATUS ExtSetFileInfo(EXT_FILE* File) {
    EFI_STATUS Status = EFI_SUCCESS;

    // Set the file info struct
    File->FileInfo->FileSize = File->INode.SizeLo + ((UINT64)File->INode.SizeHi << 0x20);
    File->FileInfo->PhysicalSize = File->FileInfo->FileSize;
    EpochToEfiTime(File->INode.CTime, &File->FileInfo->CreateTime);
    EpochToEfiTime(File->INode.ATime, &File->FileInfo->LastAccessTime);
    EpochToEfiTime(File->INode.MTime, &File->FileInfo->ModificationTime);

    // setup the type properly
    UINTN Type = File->INode.Type >> 12;
    if (Type == 0x4) {
        File->FileInfo->Attribute |= EFI_FILE_DIRECTORY;
    } else {
        CHECK_STATUS(Type == 0x8, EFI_UNSUPPORTED, "Unknown file type `%x`", Type);
    }

cleanup:
    return Status;
}


static EFI_STATUS ResolveBlock(EXT_INODE_ENTRY* INode, UINT32 BlockNo, UINT32* Block) {
    EFI_STATUS Status = EFI_SUCCESS;

    // process direct blocks (0 - 11)
    if (BlockNo < 12) {
        *Block = INode->Alloc.Ext2.Blocks[BlockNo];
    } else {
        // Process isngle indirect blocks (12 - (256 + 11))
        CHECK_FAIL_STATUS(EFI_UNSUPPORTED, "TODO: Support indirect blocks");
    }

    if (*Block == 0) {
        Status = EFI_END_OF_FILE;
    }

    cleanup:
    return Status;
}

#define ALIGN_DOWN(addr, size) ((addr)&(~((size)-1)))

static EFI_STATUS InternalExtRead(EXT_VOLUME* Volume, EXT_INODE_ENTRY* INode, UINTN ReadOffset, UINTN* BufferSize, VOID* Buffer) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK(INode != NULL);
    CHECK(BufferSize != NULL);

    UINT64 TotalRead = 0;
    UINT64 LeftToRead = *BufferSize;


    if (INode->Alloc.Ext4.Header.Magic == 0xF30A && INode->Alloc.Ext4.Header.Max == 4) {
        // this is an ext4 allocation method

        UINT64 OffsetFromFile = 0;

        for (int i = 0; i < INode->Alloc.Ext4.Header.Extends; i++) {
            // get the extend and block for this big block
            EXT4_EXT* Extend = &INode->Alloc.Ext4.Extend[i];
            UINT64 BigBlock = Extend->BlockLo + ((UINT64)Extend->BlockHi << 32);
            UINT64 Offset = BigBlock * Volume->BlockSize;
            UINT64 Length = Volume->BlockSize * Extend->Length;

            // skip this big block if it is before the file offset
            if (OffsetFromFile + Length < ReadOffset) {
                OffsetFromFile += Length;
                continue;
            }

            // recalc the offset and length according to file offset
            UINTN OffsetFromStartOfBlock = ReadOffset - ALIGN_DOWN(ReadOffset, Volume->BlockSize);
            Offset += OffsetFromStartOfBlock;
            Length -= OffsetFromStartOfBlock;
            Length = MIN(Length, LeftToRead);

            // Do the actual read and increment the buffer
            EFI_CHECK(Volume->DiskIo->ReadDisk(Volume->DiskIo, Volume->MediaId, Offset, Length, Buffer));

            // increment everything
            Buffer += Length;
            TotalRead += Length;
            LeftToRead -= Length;
            OffsetFromFile += Length;
            ReadOffset += Length;

            // are we done?
            if (LeftToRead == 0) {
                break;
            }
        }
    } else {
        // this is either ext2 or ext3 allocation method

        // get the starting block
        UINT32 BlockNo = ALIGN_DOWN(ReadOffset, Volume->BlockSize) / Volume->BlockSize;
        do {
            // get the block number
            UINT32 Block = 0;
            Status = ResolveBlock(INode, BlockNo, &Block);
            if (Status == EFI_END_OF_FILE) {
                Status = EFI_SUCCESS;
                break;
            }

            // get the length to read and offset to read from
            UINT64 OffsetFromStartOfBlock = ReadOffset - ALIGN_DOWN(ReadOffset, Volume->BlockSize);
            UINT64 Offset = Block * Volume->BlockSize + OffsetFromStartOfBlock;
            UINT64 Length = MIN(LeftToRead, Volume->BlockSize - OffsetFromStartOfBlock);
            Length = MIN(LeftToRead, Volume->BlockSize);

            // Do the actual read
            EFI_CHECK(Volume->DiskIo->ReadDisk(Volume->DiskIo, Volume->MediaId, Offset, Length, Buffer));

            // add to the total read, decrement from the left
            // to read and increment the offset, also get the
            // next block for the next read
            TotalRead += Length;
            LeftToRead -= Length;
            ReadOffset += Length;
            BlockNo++;
        } while(LeftToRead != 0);
    }

    // set the total amount we read
    *BufferSize = TotalRead;

    // if we read nothing we reached the end of the file
    if (TotalRead == 0 && LeftToRead != 0) {
        Status = EFI_END_OF_FILE;
    }

cleanup:
    return Status;
}

/**
 * Given a path and a root file this will search for the file/folder in the
 * relative path
 *
 * @param File          [IN]        The File to search from
 * @param Filename      [IN,OUT]    The filename to search for, on exit the pointer to the actual file
 * @param FinalEntry    [OUT]       The inode of the file
 * @return
 */
static EFI_STATUS IterateExtDir(EXT_FILE* File, CHAR8** Filename, EXT_INODE_ENTRY* FinalEntry) {
    EFI_STATUS Status = EFI_SUCCESS;
    CHAR8* Name = *Filename;
    void* DirBuffer = NULL;
    BOOLEAN IsEnd = FALSE;
    CHAR8* End = NULL;
    UINTN NameLength = 0;
    UINTN DirSize = 0;

    // copy aside
    EXT_INODE_ENTRY INode = File->INode;

reiterate:
    End = AsciiStrStr(Name, "\\");
    if (End == NULL) {
        // this is the last file
        End = Name + AsciiStrLen(Name);
        IsEnd = TRUE;
    }
    NameLength = (End - Name);

    // read all of the entries
    DirSize = INode.SizeLo + ((UINT64)INode.SizeHi << 32);
    if (DirBuffer != NULL) {
        FreePool(DirBuffer);
    }
    DirBuffer = AllocatePool(DirSize);
    CHECK_STATUS(DirBuffer != NULL, EFI_OUT_OF_RESOURCES);
    CHECK_AND_RETHROW(InternalExtRead(File->Volume, &INode, 0, &DirSize, (void*)DirBuffer));

    do {
        // take the current dir
        EXT_DIRECTORY_ENTRY* Dir = DirBuffer;

        if (Dir->NameLength == 0) {
            // End of directory
            Status = EFI_NOT_FOUND;
            goto cleanup;
        }

        // check the length
        if (Dir->NameLength != NameLength) {
            goto next;
        }

        // check if the name match
        if (AsciiStrnCmp(Dir->Name, Name, NameLength) != 0) {
            goto next;
        }

        // read the inode
        CHECK_AND_RETHROW(ExtReadINode(File->Volume, Dir->INode, &INode));

        if (IsEnd) {
            // last entry, copy the name and final entry
            *FinalEntry = INode;
            *Filename = Name;
            goto cleanup;
        } else {
            // Found this one! now go to the next folder...
            Name = End + 1;
            goto reiterate;
        }

    next:
        // we done with this one
        DirBuffer += Dir->EntryLength;
        DirSize -= Dir->EntryLength;
    } while(DirSize != 0);

cleanup:
    if (DirBuffer != NULL) {
        FreePool(DirBuffer);
    }
    return Status;
}

//----------------------------------------------------------------------------------------------------------------------
// File interface implemenation
//----------------------------------------------------------------------------------------------------------------------

static EFI_STATUS ExtClose(EFI_FILE_PROTOCOL* This) {
    EFI_STATUS Status = EFI_SUCCESS;
    EXT_FILE* File = (EXT_FILE*)This;

    CHECK(This != NULL);

    // just free it all
    FreePool(File->FileInfo);
    FreePool(File);

cleanup:
    return Status;
}

static EFI_STATUS ExtDelete(EFI_FILE_PROTOCOL* This) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK_FAIL_STATUS(EFI_UNSUPPORTED);

cleanup:
    return Status;
}

static EFI_STATUS ExtRead(EFI_FILE_PROTOCOL* This, UINTN* BufferSize, VOID* Buffer) {
    EFI_STATUS Status = EFI_SUCCESS;
    EXT_FILE* File = (EXT_FILE*)This;

    CHECK(This != NULL);
    CHECK_STATUS(!(File->FileInfo->Attribute & EFI_FILE_DIRECTORY), EFI_UNSUPPORTED,
                 "Tried to read on directory handle");

    CHECK_AND_RETHROW(InternalExtRead(File->Volume, &File->INode, File->Offset, BufferSize, Buffer));
    File->Offset += *BufferSize;

cleanup:
    return Status;
}

EFI_STATUS ExtWrite(EFI_FILE_PROTOCOL* This, UINTN* BufferSize, VOID* Buffer) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK_FAIL_STATUS(EFI_UNSUPPORTED);

cleanup:
    return Status;
}

EFI_STATUS ExtSetPosition(EFI_FILE_PROTOCOL* This, UINT64 Position) {
    EFI_STATUS Status = EFI_SUCCESS;
    EXT_FILE* File = (EXT_FILE*)This;

    CHECK(This != NULL);
    CHECK_STATUS(!(File->FileInfo->Attribute & EFI_FILE_DIRECTORY), EFI_UNSUPPORTED,
                 "Tried to set position on directory handle");

    File->Offset = MIN(Position, File->FileInfo->FileSize);

cleanup:
    return Status;
}

EFI_STATUS ExtGetPosition(EFI_FILE_PROTOCOL* This, UINT64* Position) {
    EFI_STATUS Status = EFI_SUCCESS;
    EXT_FILE* File = (EXT_FILE*)This;

    CHECK(Position != NULL);

    CHECK(This != NULL);
    CHECK_STATUS(!(File->FileInfo->Attribute & EFI_FILE_DIRECTORY), EFI_UNSUPPORTED,
                 "Tried to get position on directory handle");

    *Position = File->Offset;

cleanup:
    return Status;
}

EFI_STATUS ExtGetInfo(EFI_FILE_PROTOCOL* This, EFI_GUID* InformationType, UINTN* BufferSize, VOID* Buffer) {
    EFI_STATUS Status = EFI_SUCCESS;
    EXT_FILE* File = (EXT_FILE*)This;

    CHECK(This != NULL);
    CHECK(InformationType != NULL);
    CHECK(BufferSize != NULL);

    if (CompareGuid(InformationType, &gEfiFileInfoGuid)) {
        // this is the file info guid

        if (*BufferSize < File->FileInfo->Size) {
            // we don't have enough space, fail quitely
            *BufferSize = File->FileInfo->Size;
            Status = EFI_BUFFER_TOO_SMALL;
            goto cleanup;
        } else {
            // we have enough space, copy it
            CHECK(Buffer != NULL);
            CopyMem(Buffer, File->FileInfo, File->FileInfo->Size);
        }

    } else {
        CHECK_FAIL_STATUS(EFI_UNSUPPORTED, "Unknown information type %g", InformationType);
    }

cleanup:
    return Status;
}

EFI_STATUS ExtSetInfo(EFI_FILE_PROTOCOL* This, EFI_GUID* InformationType, UINTN BufferSize, VOID* Buffer) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK_FAIL_STATUS(EFI_UNSUPPORTED);

cleanup:
    return Status;
}

EFI_STATUS ExtFlush(EFI_FILE_PROTOCOL* This) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK_FAIL_STATUS(EFI_UNSUPPORTED);

cleanup:
    return Status;
}

static EFI_STATUS ExtOpen(EFI_FILE_PROTOCOL* This, EFI_FILE_PROTOCOL** NewHandle, CHAR16* FileName, UINT64 OpenMode, UINT64 Attributes) {
    EFI_STATUS Status = EFI_SUCCESS;
    EXT_FILE* ThisFile = (EXT_FILE*)This;
    CHAR8* Filename = NULL;
    EXT_FILE* NewFile = NULL;

    // check params
    CHECK(ThisFile != NULL);
    CHECK(FileName != NULL);
    CHECK(NewHandle != NULL);
    *NewHandle = NULL;

    // check the open mode
    CHECK_STATUS(OpenMode == 0 || OpenMode == EFI_FILE_MODE_READ, EFI_UNSUPPORTED, "We do not support file read/write");

    // check this is a directory
    CHECK_STATUS(ThisFile->FileInfo->Attribute & EFI_FILE_DIRECTORY, EFI_UNSUPPORTED, "Tries to open from a file handle");

    // convert the path to ASCII for ease of use
    UINTN FilenameLen = StrLen(FileName) + 1;
    Filename = AllocatePool(FilenameLen);
    CHECK_STATUS(Filename != NULL, EFI_OUT_OF_RESOURCES);
    EFI_CHECK(UnicodeStrToAsciiStrS(FileName, Filename, FilenameLen));

    // find the correct dir
    EXT_INODE_ENTRY NewFileInode;
    CHAR8* TmpFileName = Filename;
    Status = IterateExtDir(ThisFile, &TmpFileName, &NewFileInode);
    if (Status == EFI_NOT_FOUND) {
        goto cleanup;
    }
    CHECK_AND_RETHROW(Status);

    // now prepare a new file structure
    NewFile = AllocateZeroPool(sizeof(EXT_FILE));
    CHECK_STATUS(NewFile != NULL, EFI_OUT_OF_RESOURCES);

    // Allocate the file info struct
    FilenameLen = AsciiStrLen(TmpFileName);
    NewFile->FileInfo = AllocateZeroPool(sizeof(EFI_FILE_INFO) + (FilenameLen + 1) * sizeof(CHAR16));
    CHECK_STATUS(NewFile->FileInfo != NULL, EFI_OUT_OF_RESOURCES);
    NewFile->FileInfo->Size = sizeof(EFI_FILE_INFO) + (FilenameLen + 1) * sizeof(CHAR16);

    // copy the filename to the info struct
    StrCpyS(NewFile->FileInfo->FileName, FilenameLen + 1, &FileName[TmpFileName - Filename]);

    // set the parent volume
    NewFile->Volume = ThisFile->Volume;
    NewFile->Offset = 0;

    // read the inode and set the file info
    NewFile->INode = NewFileInode;
    CHECK_AND_RETHROW(ExtSetFileInfo(NewFile));

    // set all of the functions properly
    NewFile->FileInterface.Revision = EFI_FILE_REVISION;
    NewFile->FileInterface.Open = ExtOpen;
    NewFile->FileInterface.Close = ExtClose;
    NewFile->FileInterface.Delete = ExtDelete;
    NewFile->FileInterface.Read = ExtRead;
    NewFile->FileInterface.Write = ExtWrite;
    NewFile->FileInterface.GetPosition = ExtGetPosition;
    NewFile->FileInterface.SetPosition = ExtSetPosition;
    NewFile->FileInterface.GetInfo = ExtGetInfo;
    NewFile->FileInterface.SetInfo = ExtSetInfo;
    NewFile->FileInterface.Flush = ExtFlush;

    // set the new handle
    *NewHandle = (EFI_FILE_PROTOCOL*)NewFile;

cleanup:
    if (Status != EFI_SUCCESS) {
        if (NewFile != NULL) {
            FreePool(NewFile);
        }
    }
    if (Filename != NULL) {
        FreePool(Filename);
    }
    return Status;
}

//----------------------------------------------------------------------------------------------------------------------
// File system interface implemenation
//----------------------------------------------------------------------------------------------------------------------

/**
 * This is the open volume protocol interface, it will return the handle to the root file entry
 *
 * @param This  [IN]    The filesystem instance
 * @param Root  [OUT]   The root handle
 */
static EFI_STATUS ExtOpenVolume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* This, EFI_FILE_PROTOCOL** Root) {
    EFI_STATUS Status = EFI_SUCCESS;
    EXT_VOLUME* Volume = (EXT_VOLUME*)This;

    CHECK(Root != NULL);
    *Root = NULL;

    // Allocate it
    EXT_FILE* File = AllocateZeroPool(sizeof(EXT_FILE));
    CHECK_STATUS(File != NULL, EFI_OUT_OF_RESOURCES);

    // Allocate the file info struct
    File->FileInfo = AllocateZeroPool(sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 1);
    File->FileInfo->Size = sizeof(EFI_FILE_INFO) + sizeof(CHAR16) * 1;
    File->FileInfo->FileName[0] = L'\\';

    // set the parent volume
    File->Volume = Volume;

    // read the inode and set the file info
    CHECK_AND_RETHROW(ExtReadINode(Volume, 2, &File->INode));
    CHECK_AND_RETHROW(ExtSetFileInfo(File));

    // set all of the functions properly
    File->FileInterface.Revision = EFI_FILE_REVISION;
    File->FileInterface.Open = ExtOpen;
    File->FileInterface.Close = ExtClose;
    File->FileInterface.Delete = ExtDelete;
    File->FileInterface.Read = ExtRead;
    File->FileInterface.Write = ExtWrite;
    File->FileInterface.GetPosition = ExtGetPosition;
    File->FileInterface.SetPosition = ExtSetPosition;
    File->FileInterface.GetInfo = ExtGetInfo;
    File->FileInterface.SetInfo = ExtSetInfo;
    File->FileInterface.Flush = ExtFlush;

    // return it
    *Root = (EFI_FILE_PROTOCOL*)File;

cleanup:
    return Status;
}

/**
 * This will process the EXT Volume and set it up
 *
 * @param Volume    [IN] The volume to setup
 */
static EFI_STATUS ProcessExtVolume(EXT_VOLUME* Volume) {
    EFI_STATUS Status = EFI_SUCCESS;

    // Read the super block, it is 1024 bytes in the volume
    Status = Volume->DiskIo->ReadDisk(Volume->DiskIo, Volume->MediaId, 1024, sizeof(EXT_SUPER_BLOCK), &Volume->SuperBlock);
    if (Status == EFI_NO_MEDIA) {
        goto cleanup;
    }
    EFI_CHECK(Status);

    // check if this is an ext drive, if it is not then we will
    // return an error and go back to cleanup
    if (!IsExt(&Volume->SuperBlock)) {
        Status = EFI_NO_MEDIA;
        goto cleanup;
    }

    // calculate the real block size
    Volume->BlockSize = 1024 << Volume->SuperBlock.BlockSize;

    // read bgdt and cache it
    UINTN BgdtBlock = 1;
    if (Volume->BlockSize == 1024) {
        BgdtBlock = 2;
    }
    EFI_CHECK(Volume->DiskIo->ReadDisk(Volume->DiskIo, Volume->MediaId,
                                       Volume->BlockSize * BgdtBlock, sizeof(EXT_BGDT), &Volume->Bgdt));

    // set the volume interface stuff
    Volume->VolumeInterface.Revision = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_REVISION;
    Volume->VolumeInterface.OpenVolume = ExtOpenVolume;

    // Install the file system driver
    EFI_CHECK(gBS->InstallMultipleProtocolInterfaces(&Volume->Handle,
                         &gEfiSimpleFileSystemProtocolGuid, &Volume->VolumeInterface,
                         NULL));

    // we done!
    TRACE("Found EXT drive!");

cleanup:
    return Status;
}

static void CleanupExtVolume(EFI_DRIVER_BINDING_PROTOCOL* This, EXT_VOLUME* Volume) {
    if (Volume == NULL) {
        // nothing to clean
        return;
    }

    if (Volume->DiskIo != NULL) {
        WARN_ON_ERROR(gBS->CloseProtocol(Volume->Handle, &gEfiDiskIoProtocolGuid, This->DriverBindingHandle, Volume->Handle),
                      "Failed to close DiskIo protocol");
    }

    if (Volume->BlockIo != NULL) {
        WARN_ON_ERROR(gBS->CloseProtocol(Volume->Handle, &gEfiBlockIoProtocolGuid, This->DriverBindingHandle, Volume->Handle),
                      "Failed to close DiskIo protocol");
    }

    // finally free it
    FreePool(Volume);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Setup the driver binding for this protocol
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_STATUS ExtFsSupported(EFI_DRIVER_BINDING_PROTOCOL* This, EFI_HANDLE ControllerHandle, EFI_DEVICE_PATH_PROTOCOL* RemainingDevicePath) {
    EFI_STATUS Status = EFI_SUCCESS;

    // Test that we have the disk io protocol guid
    Status = gBS->OpenProtocol(ControllerHandle,
                      &gEfiDiskIoProtocolGuid, NULL,
                      This->DriverBindingHandle, ControllerHandle,
                      EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
    if (Status == EFI_UNSUPPORTED || Status == EFI_ACCESS_DENIED) {
        goto cleanup;
    }
    EFI_CHECK(Status);

    // Test that we have the block io protocol guid
    Status = gBS->OpenProtocol(ControllerHandle,
                               &gEfiBlockIoProtocolGuid, NULL,
                               This->DriverBindingHandle, ControllerHandle,
                               EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
    if (Status == EFI_UNSUPPORTED || Status == EFI_ACCESS_DENIED) {
        goto cleanup;
    }
    EFI_CHECK(Status);

cleanup:
    return Status;
}

EFI_STATUS ExtFsStart(EFI_DRIVER_BINDING_PROTOCOL* This, EFI_HANDLE ControllerHandle, EFI_DEVICE_PATH_PROTOCOL* RemainingDevicePath) {
    EFI_STATUS Status = EFI_SUCCESS;
    EXT_VOLUME* ExtVolume = NULL;

    // allocate it
    ExtVolume = AllocateZeroPool(sizeof(EXT_VOLUME));
    CHECK_STATUS(ExtVolume != NULL, EFI_OUT_OF_RESOURCES);

    // set the handle
    ExtVolume->Handle = ControllerHandle;

    // open the disk io protocol
    EFI_CHECK(gBS->OpenProtocol(ControllerHandle,
                                &gEfiDiskIoProtocolGuid, (void**)&ExtVolume->DiskIo,
                                This->DriverBindingHandle, ControllerHandle,
                                EFI_OPEN_PROTOCOL_GET_PROTOCOL));

    // open the block io protocol
    EFI_CHECK(gBS->OpenProtocol(ControllerHandle,
                                &gEfiBlockIoProtocolGuid, (void**)&ExtVolume->BlockIo,
                                This->DriverBindingHandle, ControllerHandle,
                                EFI_OPEN_PROTOCOL_GET_PROTOCOL));

    // set the media id
    ExtVolume->MediaId = ExtVolume->BlockIo->Media->MediaId;

    // Process the volume, if returns EFI_NO_MEDIA it means this is not an EXT
    // drive and we should go back
    Status = ProcessExtVolume(ExtVolume);
    if (Status == EFI_NO_MEDIA) {
        goto cleanup;
    }
    CHECK_AND_RETHROW(Status);

cleanup:
    if (Status != EFI_SUCCESS) {
        CleanupExtVolume(This, ExtVolume);
    }
    return Status;
}

EFI_STATUS ExtFsStop(EFI_DRIVER_BINDING_PROTOCOL* This, EFI_HANDLE ControllerHandle, UINTN NumberOfChildren, EFI_HANDLE* ChildHandleBuffer) {
    EFI_STATUS Status = EFI_SUCCESS;

    TRACE("TODO");

    return Status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Register everything
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * The binding protocol
 */
static EFI_DRIVER_BINDING_PROTOCOL gExtFsDriverBinding = {
    .Supported = ExtFsSupported,
    .Start = ExtFsStart,
    .Stop = ExtFsStop,
    .Version = 0x10,
    .ImageHandle = NULL,
    .DriverBindingHandle = NULL
};

// TODO: component name/component name 2 protocols

EFI_STATUS LoadExtFs() {
    EFI_STATUS Status = EFI_SUCCESS;

    // setup the handles
    gExtFsDriverBinding.DriverBindingHandle = gImageHandle;
    gExtFsDriverBinding.ImageHandle = gImageHandle;

    // install the binding
    EFI_CHECK(gBS->InstallMultipleProtocolInterfaces(&gExtFsDriverBinding.DriverBindingHandle,
            &gEfiDriverBindingProtocolGuid, &gExtFsDriverBinding,
            NULL));

cleanup:
    return Status;
}
