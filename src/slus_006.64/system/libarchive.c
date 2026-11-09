#include "common.h"
#include "system/archive.h"
#include "psyq/pc.h"
#include "psyq/libcd.h"

// Temp
extern int func_800286CC(void);

extern s32 g_ArchiveCurFileSector;

extern s32 D_8004FE0C;
extern s32 D_8004FE10;
extern u16 D_8004FE24;
extern u32 D_80059F0C;
extern void func_8002804C(int, int, int, int);
extern int ArchiveDecodeSizeAligned(int entryIndex);

void ArchiveInit(u32 pArchiveTable, u32 pHeaderTable, u32 pDebugTable) {
    D_8005A488 = 0;
    D_8005A48C = 0;
    D_8005A490 = 0;
    D_8005A494 = 0;
    D_8005A498 = 0;
    D_8005A49C = 0;
    D_8005A4A4 = 0;
    D_8005A4A8 = 0;
    D_8005A4B4 = 0;
    
    if ( (pDebugTable == NULL) || (pDebugTable == -1) ) {
        while (CdInit() == 0);
        CdSetDebug(0);
        CdDataCallback(0);
        CdSyncCallback(NULL);
        CdReadyCallback(NULL);
        CdControl(7, 0, &D_80059F1C);
        ArchiveCdSetMode(0xA0);
        ArchiveCdDataSync(0);
        Vsync(3);
    } else {
        PCinit();
    }

    if (pDebugTable != -1) {
        g_ArchiveDebugTable = pDebugTable;
    } else {
        g_ArchiveDebugTable = NULL;
    }
    
    g_ArchiveTable = pArchiveTable;
    g_ArchiveHeader = pHeaderTable;
    g_CurArchiveOffset = 0;
    D_8004FDFC = 0;
    g_ArchiveCurFileSize = 0;
    g_ArchiveCdDriveState = 0;
    D_8004FE4C = -1;
    
    if (!pDebugTable) {
        ArchiveReadFileFromCdSector(0x18, pArchiveTable, (0x10 * CD_SECTOR_SIZE), 0, 0);
        ArchiveCdDataSync(0);
        ArchiveReadFileFromCdSector(0x28, g_ArchiveHeader, 0x7A, 0, 0);
        ArchiveCdDataSync(0);
    }
}

void ArchiveReset(void) {
    func_8002A498(0);
    ArchiveCdDataSync(0);
    if (g_ArchiveDebugTable == 0) {
        while (CdControlB(9, 0, &D_80059F1C) == 0);
        ArchiveCdSetMode(0xA0);
        ArchiveCdDataSync(0);
        Vsync(3);
    }
    CdDataCallback(0);
    CdSyncCallback(NULL);
    CdReadyCallback(NULL);
    D_8004FDFC = 0;
    g_ArchiveCurFileSize = 0;
    g_ArchiveCdDriveState = 0;
}

int ArchiveSetIndex(int directoryIndex, int entryIndex) {
    s32 nOffset;

    nOffset = ((u_short*)g_ArchiveHeader)[directoryIndex + entryIndex] - 1;
    g_CurArchiveOffset = nOffset;
    if (nOffset < 0) {
        g_CurArchiveOffset = 0;
        return -1;
    }
    return nOffset;
}

int ArchiveGetArchiveOffsetIndices(int* alignedIndex, int* remainder) {
    int nAligned;
    int nIndex;
    u_short* pArchiveHeaderEntry;

    pArchiveHeaderEntry = g_ArchiveHeader;
    nIndex = 0;
    
    while(nIndex < ARCHIVE_MAX_SECTIONS) {
        if (*pArchiveHeaderEntry == g_CurArchiveOffset + 1) {
            // Round down to 4-byte boundary
            nAligned = (nIndex / 4) * 4;
            
            *alignedIndex = nAligned;
            *remainder = nIndex - nAligned;
            break;
        }
        
        nIndex += 1;
        pArchiveHeaderEntry += 1;
    }
    
    if (nIndex == ARCHIVE_MAX_SECTIONS) {
        *alignedIndex = 0;
        *remainder = 0;
    }
    
    return g_CurArchiveOffset;
}

unsigned short ArchiveGetDiscNumber(void) {
    return *((unsigned short*)(g_ArchiveHeader + 0x78));
}

int func_80028548(int directoryIndex, int entryIndex) {
    int nOffset;
    nOffset = ((u_short*)g_ArchiveHeader)[directoryIndex + entryIndex];
    return nOffset - g_CurArchiveOffset;
}

// Only matches on GCC 2.6.X
char* ArchiveReadHddFile(char* pFilePath, int* pFileSize) {
    int nFileSize;
    int hFile;
    int i;
    int nBytesRead;
    char* pFileBuffer;

    for (i = 0; i < RETRY_COUNT; i++) {
        hFile = PCopen(pFilePath, 0, O_RDONLY);
        if (hFile != -1)
            break;
    }

    if (hFile == -1) {
        pFileBuffer = NULL;
        goto error;
    }

    nFileSize = PClseek(hFile, 0, SEEK_END);
    if (pFileSize)
        *pFileSize = nFileSize;
    PClseek(hFile, 0, SEEK_SET);
    
    pFileBuffer = (char*) HeapAlloc(nFileSize, 0x0);
    nBytesRead = 0;
    if (pFileBuffer) {
        for (i = 0; i < RETRY_COUNT; i++) {
            nBytesRead = PCread(hFile, pFileBuffer, nFileSize);
            if (nBytesRead != 0)
                break;
        }
    } 
    
    if (nBytesRead == 0) {
        if (pFileBuffer)
            HeapFree(pFileBuffer);
        pFileBuffer = NULL;
    }

    i = 0;
    while (PCclose(hFile) != 0) {
        i += 1;
        if (i >= RETRY_COUNT) {
            if (pFileBuffer)
                HeapFree(pFileBuffer);
            pFileBuffer = NULL;
            break;
        }        
    }

    error:
    return pFileBuffer;
}

int ArchiveGetCurFileSize(void) {
    return g_ArchiveCurFileSize;
}

int ArchiveDataSync(void) {
    int nResult;

    nResult = D_8004FDFC;
    if (nResult == 0) {
        if (g_ArchiveDebugTable == NULL && CdDataSync(1)) {
            return 1;
        }
        if (g_ArchiveCdDriveState != 0) {
            return 1;
        }
    }
    return nResult;
}

int ArchiveDecodeSize(int entryIndex) {
    char* pFilepath;
    int hArchiveFile;
    int nFileSize;
    u8* pArchiveEntry;
    u32 nOffset;
    
    if (g_ArchiveDebugTable) {
        pFilepath = ArchiveGetFilePath(entryIndex);
        hArchiveFile = PCopen(pFilepath, O_RDONLY, 0);
        nFileSize = PClseek(hArchiveFile, 0, SEEK_END);
        PCclose(hArchiveFile);
        if (nFileSize > 0) {
            goto exit;
        }
    }
    
    nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (
        (pArchiveEntry[6] << 0x18) + 
        (pArchiveEntry[5] << 0x10) + 
        (pArchiveEntry[4] << 0x8) + 
        pArchiveEntry[3]
    );

    exit:
    return nFileSize;
}

// Uses D_8004FE18 as archive offset
int ArchiveDecodeSizeAligned(int entryIndex) {
    char* pFilepath;
    int hArchiveFile;
    int nFileSize;
    u8* pArchiveEntry;
    u32 nOffset;
    
    if (g_ArchiveDebugTable) {
        pFilepath = ArchiveGetFilePath(entryIndex);
        hArchiveFile = PCopen(pFilepath, O_RDONLY, 0);
        nFileSize = PClseek(hArchiveFile, 0, SEEK_END);
        PCclose(hArchiveFile);
        if (nFileSize > 0) {
            goto exit;
        }
    }
    
    nOffset = (entryIndex + D_8004FE18 - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (
        (pArchiveEntry[6] << 0x18) + 
        (pArchiveEntry[5] << 0x10) + 
        (pArchiveEntry[4] << 0x8) + 
        pArchiveEntry[3]
    );

    exit:
    nFileSize = ((nFileSize + 3) / 4) * 4;
    return nFileSize;
}

// Only matches on GCC 2.6.X
int ArchiveDecodeAlignedSize(unsigned int entryIndex) {
    s32 nSize;
    s32 nAlignedSize;
    
    nSize = ArchiveDecodeSize(entryIndex);
    nAlignedSize = ((nSize + 3 ) / 4);
    return nAlignedSize * 4;
}

int ArchiveDecodeSizeAbsolute(int entryIndex) {
    int nFileSize;
    u8* pArchiveEntry;
    u32 nOffset;
    
    nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    nFileSize = (
        (pArchiveEntry[6] << 0x18) + 
        (pArchiveEntry[5] << 0x10) + 
        (pArchiveEntry[4] << 0x8) + 
        pArchiveEntry[3]
    );

    if (nFileSize >= 0)
        nFileSize = 0;
    else
        nFileSize = (s16) (nFileSize * -1);

    return nFileSize;
}

char* ArchiveGetFilePath(int entryIndex) {
    unsigned int nOffset;
    
    if (g_ArchiveDebugTable) {
        nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_ENTRY_NAME_BUFFER_SIZE;
        return nOffset + g_ArchiveDebugTable;
    }

    return NULL;
}

int ArchiveDecodeSector(int entryIndex) {
    u_char* pArchiveEntry;
    unsigned int nOffset;

    nOffset = (entryIndex + g_CurArchiveOffset - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    return (pArchiveEntry[2] << 0x10) + (pArchiveEntry[1] << 0x8) + pArchiveEntry[0];
}

// Same as ArchiveDecodeSector, but uses D_8004FE18 as current archive offset
int func_80028A18(int entryIndex) {
    u_char* pArchiveEntry;
    unsigned int nOffset;

    nOffset = (entryIndex + D_8004FE18 - 1) * ARCHIVE_HEADER_ENTRY_SIZE;
    pArchiveEntry = nOffset + g_ArchiveTable;
    return (pArchiveEntry[2] << 0x10) + (pArchiveEntry[1] << 0x8) + pArchiveEntry[0];
}



// Possible start of archive2.c

void ArchiveCdDataSync(int mode) {
    int nResult;

    if (mode == 0) {
        do {
            nResult = ArchiveDataSync();
        } while (0 < nResult);
    }
    ArchiveDataSync();
}

void* ArchiveChangeStreamingFile(void* pStreamFile) {
    void* pPrevStreamFile;

    pPrevStreamFile = g_ArchiveCurStreamFile;
    g_ArchiveCurStreamFile = pStreamFile;
    return pPrevStreamFile;
}

int ArchiveClearStreamFileSections(void) {
    int nEntries;
    int i;
    void* pStreamData;
    ArchiveStreamFileSectionHeader* pCurSectionHeader;
    ArchiveStreamFileSectionHeader* pSectionHeaderStart;

    pStreamData = g_ArchiveCurStreamFile;
    if (pStreamData == NULL) {
        return -1;
    }
    
    i = 0; 
    nEntries = *(int*)pStreamData;
    pSectionHeaderStart = (ArchiveStreamFileSectionHeader*)(pStreamData + 4);
    
    pCurSectionHeader = pSectionHeaderStart;
    while (i < nEntries) {
        pCurSectionHeader[i].state = 0;
        pCurSectionHeader[i].id = 0;
        pCurSectionHeader[i].size = 0;
        pCurSectionHeader[i].unk2 = 0;
        i++;
    }
    pSectionHeaderStart->size = nEntries;
    
    return nEntries;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/libarchive", func_80028B14);

/* Best: 93.87%. Remaining diff is register allocation and scheduling, not
 * structure: the original loads g_ArchiveCurStreamFile into v0 before the
 * frame setup and copies it to s2 after the NULL test, and its debug loop
 * keeps the section count in a1, D_8004FE10 in v1, the header base in a3
 * and an extra copy of the old entry index in a2.
 * u_char* func_80028B14(void) {
 *     u_char* pData;
 *     ArchiveStreamFileSectionHeader* pHeader;
 *     ArchiveStreamFileSectionHeader* pHeaders;
 *     StreamDataQueueEntry* pEntry;
 *     u_char* pPath;
 *     int nSections;
 *     int nLimit;
 *     int nIndex;
 *     int nId;
 *     int i;
 *
 *     pData = g_ArchiveCurStreamFile;
 *     if (pData == NULL) {
 *         return NULL;
 *     }
 *
 *     pHeader = (ArchiveStreamFileSectionHeader*)(pData + 4);
 *     nSections = *(s32*)pData;
 *     pData = pData + nSections * sizeof(ArchiveStreamFileSectionHeader) + STREAM_FILE_HEADER_SIZE;
 *
 *     if (g_ArchiveDebugTable != 0) {
 *         if (D_8004FE4C == -1) {
 *             return NULL;
 *         }
 *
 *         if (g_ArchiveCurFileSize <= 0) {
 *             return NULL;
 *         }
 *
 *         pHeaders = D_8004FE2C;
 *         nLimit = D_8004FE40;
 *
 *         for (i = 0; i < nLimit; i++) {
 *             nIndex = D_8004FE10;
 *             pHeader = &pHeaders[nIndex];
 *             D_8004FE10 = nIndex + 1;
 *             if (D_8004FE10 >= nLimit) {
 *                 D_8004FE10 = 0;
 *             }
 *             if (pHeader->state == ARCHIVE_STREAM_FILE_NOT_LOADED) {
 *                 break;
 *             }
 *         }
 *
 *         if (pHeader->state != ARCHIVE_STREAM_FILE_NOT_LOADED) {
 *             return NULL;
 *         }
 *
 *         pHeader->state = 3;
 *         pData += nIndex * CD_SECTOR_SIZE;
 *
 *         for (i = 0; i < RETRY_COUNT; i++) {
 *             if (PCread(D_8004FE4C, pData, CD_SECTOR_SIZE) != 0) {
 *                 goto readDone;
 *             }
 *             func_8002804C(i, 0, 0xFF, 0);
 *         }
 *         return NULL;
 *
 *     readDone:
 *
 *         g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *         if (g_ArchiveCurFileSize > 0) {
 *             return pData;
 *         }
 *
 *         g_ArchiveCurFileSize = 0;
 *         for (i = 0; i < RETRY_COUNT; i++) {
 *             if (PCclose(D_8004FE4C) == 0) {
 *                 break;
 *             }
 *             func_8002804C(i, 0, 0, 0xFF);
 *         }
 *
 *         D_8004FE4C = -1;
 *         if (D_8004FE0C != 0) {
 *             nIndex = D_8004FE10 + 1;
 *             pEntry = ((StreamDataQueueEntry*)D_8004FE0C) + nIndex;
 *             nId = *(u_short*)pEntry;
 *             D_8004FE10 = nIndex;
 *             D_80059F0C = nId;
 *
 *             if (nId > 0 && pEntry->pData != NULL) {
 *                 pPath = ArchiveGetFilePath(nId);
 *                 for (i = 0; i < RETRY_COUNT; i++) {
 *                     D_8004FE4C = PCopen(pPath, 0, 0);
 *                     if (D_8004FE4C != -1) {
 *                         break;
 *                     }
 *                     func_8002804C(i, 0xFF, 0, 0);
 *                 }
 *                 g_ArchiveCurFileSize = ArchiveDecodeSizeAligned(nId);
 *                 D_8004FDFC -= 1;
 *                 return pData;
 *             }
 *
 *             g_ArchiveCurFileSize = 0;
 *         }
 *
 *         D_8004FDFC = 0;
 *         return pData;
 *     }
 *
 *     for (i = 0; i < nSections; i++, pHeader++) {
 *         if (pHeader->state == 3 && pHeader->id == D_8004FE24) {
 *             break;
 *         }
 *     }
 *
 *     if (i == D_8004FE40) {
 *         return NULL;
 *     }
 *
 *     D_8004FE24 += 1;
 *     return pData + i * CD_SECTOR_SIZE;
 * }
 */

int func_80028E60(int sectionIndex, int maxEntries, int targetState) {
    int i;
    ArchiveStreamFileSectionHeader* pSectionHeader;
    int nSections;
    
    i = 0;
    while (i < maxEntries) {
        pSectionHeader = D_8004FE2C + sectionIndex;
        nSections = D_8004FE40;

        while (1) {
            if (pSectionHeader->state == targetState) {
                sectionIndex++;
                pSectionHeader++;
                
                if (nSections < sectionIndex)
                    return 1;
                
                i++;
                
                if (i >= maxEntries)
                    return 0;
                
                continue;
            }
            return 1;
        }
    }

    return 0;
}

void ArchiveConsolidateStreamFileEntry(int sectionIndex) {
    short nSize;
    int nNextIndex;
    ArchiveStreamFileSectionHeader* pSectionHeader;
    ArchiveStreamFileSectionHeader* pNextSectionHeader;

    pSectionHeader = D_8004FE2C + sectionIndex;
    nSize = pSectionHeader->size;
    nNextIndex = sectionIndex + nSize;
    
    if (nNextIndex < D_8004FE40) {
        pNextSectionHeader = D_8004FE2C + nNextIndex;
        if (pNextSectionHeader->state == ARCHIVE_STREAM_FILE_NOT_LOADED) {
            pSectionHeader->size += pNextSectionHeader->size;
        }
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/libarchive", func_80028F30);

u_short func_8002945C(u_char* pSection) {
    u_char* pStream;
    u_char* pStreamEnd;
    ArchiveStreamFileSectionHeader* pSectionHeader;
    u32 nIndex;
    u16 nState;

    pStream = g_ArchiveCurStreamFile;
    if (pStream == NULL) {
        return -1;
    }
    if (pSection == NULL) {
        return 0;
    }

    pSectionHeader = (ArchiveStreamFileSectionHeader*)(pStream + 4);
    pStreamEnd = pStream + *(s32*)pStream * 8 + 0x24;
    nIndex = (u32)(pSection - pStreamEnd) >> 11;
    nState = pSectionHeader[nIndex].state;
    pSectionHeader[nIndex].state = 0;
    return nState;
}

u_short func_800294B4(u_char* pSection) {
    u_char* pStream;
    u_char* pStreamEnd;
    ArchiveStreamFileSectionHeader* pSectionHeader;
    u32 nIndex;
    u16 nState;
    s32 nCount;

    pStream = g_ArchiveCurStreamFile;
    if (pStream == NULL) {
        return -1;
    }

    if (pSection == NULL) {
        return 0;
    }

    nCount = *(u16*)(pSection + 6);
    pSectionHeader = (ArchiveStreamFileSectionHeader*)(pStream + 4);
    pStreamEnd = pStream + *(s32*)pStream * 8 + 0x24;
    nIndex = (u32)(pSection - pStreamEnd) >> 11;
    nState = pSectionHeader[nIndex].state;

    while (nCount > 0) {
        pSectionHeader[nIndex + nCount - 1].state = 0;
        nCount--;
    }

    ArchiveConsolidateStreamFileEntry(nIndex);

    return nState;
}

s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags) {
    if (g_ArchiveDebugTable) {
        return -1;
    }
    
    ArchiveCdDataSync(0);
    g_ArchiveCurFileSector = sector;
    g_ArchiveCurFileSize = fileSize;
    return ArchiveReadFile(0, pDestBuffer, arg3, flags);
}

s32 ArchiveReadFileToBuffer(s32 index, s32 pBuffer, u32 arg2, u32 flags) {
    if ((index <= 0) || (ArchiveDecodeSize(index) <= 0) || (pBuffer == NULL)) {
        return -3;
    }
    ArchiveCdDataSync(0);
    D_8004FE18 = g_CurArchiveOffset;
    g_ArchiveCurFileSector = ArchiveDecodeSector(index);
    g_ArchiveCurFileSize = ArchiveDecodeAlignedSize(index);
    return ArchiveReadFile(index, pBuffer, arg2, flags);
}
