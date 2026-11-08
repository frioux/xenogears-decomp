/*
 * Archive code is split into two C files due to a compiler mismatch.
 * libarchive.c contains two functions which seemingly only compiles on GCC 2.6.X.
 * Meanwhile, ArchiveReadFile only seemingly compiles on GCC 2.7.2.
 * This suggests that archive.c is compiled on its own, maybe as a library.
*/

#include "common.h"
#include "system/archive.h"
#include "psyq/pc.h"
#include "psyq/libcd.h"

extern s8* D_8004FE08;
extern s32 D_8004FE0C;
extern s32 D_8004FE00;
extern s32 D_8004FE10;
extern s16 D_8004FE24;
extern u16 D_8004FE26;
extern u16 D_8004FE28;
extern void* g_ArchiveCurStreamFile; // Current stream file struct
extern s32 D_80059F00;
extern u32 D_80059F0C;
extern s16 D_80059F60;
extern s32 D_8005A4DC;
extern void func_8002B084(u_char status);
extern void func_8002B2F0();
extern void func_8002BA58();
extern s32 g_ArchiveCurFileSector;
extern char D_80018944[];
extern char D_80018954[];
extern char D_80018964[];
extern char D_80018974[];
extern void func_800379C8(char*, ...);
extern s32 D_80059F3C;
extern int D_80059F04;
extern CdlLOC D_80059EF8;
extern u8 D_800596F8[];
extern s32 D_8004FDE4;
extern s16 D_80059F40;
extern s16 D_80059F44;
extern s16 D_80059F48;
extern u16* D_80059F4C;
extern s32 D_80059F50;

// Only matches on GCC 2.7.2
int ArchiveReadFile(u32 dbgEntryIndex, u8* pDestBuffer, s32 arg2, s32 flags) {
    void* pReadyCallback;
    int nStreamSectors;
    int nStreamSectors2; // The same as nStreamSectors, doesn't match unless it's two variables
    s32 hFile;
    int i;
    char* pFilePath;
    char* pFilePath2; // The same as nStreamSectors, doesn't match unless it's two variables
    u8* pCdMode;
    u32* pData;
    CdlLOC* pLoc;

    D_80059F0C = dbgEntryIndex;
    for (i = 2, pData = &D_80059F00; i >= 0; i--)
        *pData-- = 0;
    
    D_8004FDFC = 1;
    D_8004FE08 = pDestBuffer;
    D_8004FE38 = arg2 & 0xFFFF;
    D_8004FE10 = 0;
    D_8004FE0C = 0;
    D_8004FE34 = 0;
    D_8005A4DC = 0;
    pLoc = &g_ArchiveCdCurLocation;
    CdIntToPos(g_ArchiveCurFileSector, pLoc);
    
    if (flags & CdlModeStream) {
        ArchiveChangeStreamingFile(pDestBuffer);
        nStreamSectors = *(u32*)g_ArchiveCurStreamFile;
        if (nStreamSectors) {
            D_8004FE08 = g_ArchiveCurStreamFile + nStreamSectors * sizeof(ArchiveStreamFileSectionHeader) + STREAM_FILE_HEADER_SIZE;
            D_8004FE2C = g_ArchiveCurStreamFile + 4;
            D_8004FE40 = nStreamSectors;
            D_8004FE26 = 0;
            D_8004FE28 = 0;
            D_8004FE24 = 0;
            ArchiveClearStreamFileSections();
            
            if (g_ArchiveDebugTable) {
                pFilePath2 = ArchiveGetFilePath(dbgEntryIndex);
                
                for (i = 0; i < RETRY_COUNT; i++) {
                    D_8004FE4C = PCopen(pFilePath2, 0, 0);
                    if (D_8004FE4C == -1)
                        func_8002804C(i, 0xFF, 0, 0);
                    else
                        break;
                }

                return -(~D_8004FE4C == 0) & -3;
            }

            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
            CdDataCallback(&func_8002BA58);
            CdSyncCallback(&ArchiveCdDriveCommandHandler);
            pReadyCallback = &func_8002B2F0;
            CdReadyCallback(pReadyCallback);
            D_8005A488 += 1;
            CdControlF(CdlSetloc, (void*)pLoc);
            return 0;
        }
        return -4;
    }
    
    if (flags & 0x200) {
        ArchiveChangeStreamingFile(pDestBuffer);
        nStreamSectors2 = *(u32*)g_ArchiveCurStreamFile;
        if (nStreamSectors2) {
            D_8004FE08 = g_ArchiveCurStreamFile + (nStreamSectors2 * sizeof(ArchiveStreamFileSectionHeader)) + STREAM_FILE_HEADER_SIZE;
            D_8004FE2C = g_ArchiveCurStreamFile + 4;
            D_8004FE40 = nStreamSectors2;
            D_80059F60 = 0;
            D_8004FE26 = 0;
            D_8004FE28 = 0;
            D_8004FE24 = 0;
            ArchiveClearStreamFileSections();
            
            pCdMode = &D_80059F1B;
            for (i = 3; i >= 0; i--)
                *pCdMode-- = 0;

            // CD Mode - double speed, 2340 byte size
            D_80059F18[0] = flags | CdlModeSpeed | CdlModeSize1;
            
            if (g_ArchiveDebugTable) {
                pFilePath2 = ArchiveGetFilePath(dbgEntryIndex);

                for (i = 0; i < RETRY_COUNT; i++) {
                    D_8004FE4C = PCopen(pFilePath2, 0, 0);
                    if (D_8004FE4C == -1)
                        func_8002804C(i, 0xFF, 0, 0);
                    else
                        break;
                }
                
                return -(~D_8004FE4C == 0) & -3;
            }
            return 0;
        }
        return -4;
    }
    
    if (g_ArchiveDebugTable) {
        pFilePath = ArchiveGetFilePath(dbgEntryIndex);

        for (i = 0; i < RETRY_COUNT; i++) {
            hFile = PCopen(pFilePath, 0, 0);
            if (hFile == -1)
                func_8002804C(i, 0xFF, 0, 0);
            else
                break;
        }

        if (hFile == -1) {
            return -4;
        }

        if (pDestBuffer != NULL) {
            for (i = 0; i < RETRY_COUNT; i++) {
                if (PCread(hFile, pDestBuffer, g_ArchiveCurFileSize) == 0)
                    func_8002804C(i, 0, 0xFF, 0);
                else
                    break;
            }
        }
        
        for (i = 0; i < RETRY_COUNT; i++) {
            if (PCclose(hFile) != 0) {
                func_8002804C(i, 0, 0, 0xFF);
            } else {
                g_ArchiveCurFileSize = 0;
                D_8004FDFC = 0;
                return 0;
            }
        }

        return -6;
    }
    
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
    CdDataCallback(NULL);
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    pReadyCallback = &func_8002B084;
    CdReadyCallback(pReadyCallback);
    D_8005A488 += 1;
    CdControlF(CdlSetloc, (void*)pLoc);
    return 0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_80029AFC);
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_80029EB0);

int* ArchiveAllocStreamFile(int numEntries, int allocMode) {
    int* pStreamFile;

    if (numEntries > 0) {
        pStreamFile = HeapAlloc((numEntries * (CD_SECTOR_SIZE + sizeof(ArchiveStreamFileSectionHeader))) + STREAM_FILE_HEADER_SIZE, allocMode);
        if (pStreamFile) {
            *pStreamFile = numEntries;
            ArchiveChangeStreamingFile(pStreamFile);
            ArchiveClearStreamFileSections();
            return pStreamFile;
        }
    }
    return NULL;
}

void ArchiveCdSeekOrPause(int entryIndex) {
    u_char nCommand;
    u_char* nParam;
    int nSector;
    CdlLOC* pCdLocation;

    if ((g_ArchiveDebugTable == 0) && (ArchiveDataSync() == 0)) {
        D_8004FE18 = g_CurArchiveOffset;
        if (entryIndex > 0) {
            pCdLocation = &g_ArchiveCdCurLocation;
            nSector = ArchiveDecodeSector(entryIndex);
            CdIntToPos(nSector, pCdLocation);
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SEEK;
            CdSyncCallback(&ArchiveCdDriveCommandHandler);
            nCommand = CdlSetloc;
            nParam = (u_char*)pCdLocation;
        } else {
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
            CdSyncCallback(&ArchiveCdDriveCommandHandler);
            nCommand = CdlPause;
            nParam = NULL;
        }

        CdControlF(nCommand, nParam);
    }
}

void ArchiveCdSeekToFile(int entryIndex) {
    u_char nCommand;
    u_char* nParam;
    int nSector;
    CdlLOC* pCdLocation;

    if (entryIndex > 0) {
        pCdLocation = &g_ArchiveCdCurLocation;
        nSector = ArchiveDecodeSector(entryIndex);
        CdIntToPos(nSector, pCdLocation);
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SEEK;
        CdSyncCallback(&ArchiveCdDriveCommandHandler);
        nCommand = CdlSetloc;
        nParam = &g_ArchiveCdCurLocation;
    } else {
        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
        CdSyncCallback(&ArchiveCdDriveCommandHandler);
        nCommand = CdlPause;
        nParam = NULL;
    }

    CdControlF(nCommand, nParam);
}

void ArchiveCdSetMode(u_char mode) {
    int i;
    u_char* pParam;

    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_RESET_MODE;
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    for (i = 3, pParam = &D_80059F1B; i >= 0; i -= 1) {
        *pParam-- = 0;
    }
    
    D_80059F18[0] = mode;
    CdControlF(CdlSetmode, &D_80059F18);
}

void func_8002A498(int channel) {
    int i;

    D_8004FE34 = 1;
    D_8004FE38 = channel;
    if (g_ArchiveDebugTable) {
        g_ArchiveCurFileSize = 0;
        D_8004FDFC = 0;

        // Close (debug) streaming file handle
        if (D_8004FE4C != -1) {
            while (i = PCclose(D_8004FE4C)) {
                if (i != 0) {
                    if (i + 1 < 4)
                        continue;
                }
                break;
            }
            D_8004FE4C = -1;
        }
    }
}

void ArchiveStreamFreeQueueData(StreamDataQueueEntry* pEntries) {
    StreamDataQueueEntry* pCurrent;
    void* pData;

    if ((unsigned short)pEntries->archiveIndex != 0) {
        pCurrent = pEntries;
        do {
            pData = pCurrent->pData;
            pCurrent++;
            if (pData) {
                HeapFree(pData);
            }
        } while ((unsigned short)pCurrent->archiveIndex != 0);
    }
}

StreamDataQueueEntry* ArchiveStreamAllocQueueData(int archiveIndex, StreamDataQueueEntry* pEntries) {
    int entryIndex;
    int numEntries;
    int i;
    s32 bHasAllocatedEntries;
    void* pBuffer;

    bHasAllocatedEntries = 0;
    numEntries = (short)ArchiveDecodeSizeAbsolute(archiveIndex);

    if (numEntries > 0) {
        if (pEntries == NULL) {
            pEntries = HeapAlloc((numEntries + 1) * sizeof(StreamDataQueueEntry), 0);
            bHasAllocatedEntries = 1;
            if (pEntries == NULL) {
                return NULL;
            }
        }

        for (i = 0; i < numEntries; i++) {
            entryIndex = archiveIndex + i + 1;
            pEntries[i].archiveIndex = entryIndex;

            pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(entryIndex), 0);
            pEntries[i].pData = pBuffer;
            if (pBuffer == NULL) {
                ArchiveStreamFreeQueueData(pEntries);
                if (bHasAllocatedEntries > 0) {
                    HeapFree(pEntries);
                }
                pEntries = NULL;
                return 0;
            }
        }

        pEntries[numEntries].archiveIndex = 0;
        pEntries[numEntries].pData = NULL;
    } else {
        pEntries = NULL;
    }

    return pEntries;
}

void ArchiveCdDriveCommandHandler(u_char status, u_char* pResult) {
    char channel;

    switch (g_ArchiveCdDriveState) {
        case ARCHIVE_CD_DRIVE_READ_SECTOR:
            if (status == CdlComplete) {
                D_8005A48C += 1;
                g_ArchiveCdDriveState += 1;
                CdControlF(CdlReadN, NULL);
            } else {
                D_8005A490 += 1;
                D_80059F08 = CdReadyCallback(NULL);
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_READ_DONE:
            if (status == CdlComplete) {
                D_8005A48C += 1;
                CdSyncCallback(NULL);
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
                return;
            }
            D_8005A490 += 1;
            D_80059F08 = CdReadyCallback(NULL);
            g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            CdControlF(CdlNop, NULL);
            break;

        case ARCHIVE_CD_DRIVE_SEEK:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState += 1;
                CdControlF(CdlSeekL, NULL);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_SEEK;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;
        
        case ARCHIVE_CD_DRIVE_SEEK_DONE:
            if (status == CdlComplete) {
                CdSyncCallback(NULL);
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
                return;
            }
            
            g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_SEEK;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            CdControlF(CdlNop, NULL);
            break;

        case ARCHIVE_CD_DRIVE_DONE:
            if (status == CdlComplete) {
                CdSyncCallback(NULL);
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
                return;
            }
            
            g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_GENERIC;
            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
            CdControlF(CdlNop, NULL);
            break;

        case ARCHIVE_CD_DRIVE_READ_DATA_SECTOR:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
                D_8005A488 += 1;
                D_8005A494 += 1;
                CdReadyCallback(D_80059F08);
                CdControlF(CdlSetloc, &g_ArchiveCdCurLocation);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
                D_8005A498 += 1;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;
        
        case ARCHIVE_CD_DRIVE_HALT:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_DATA_SECTOR;
                D_8005A4A8 += 1;
                CdControlF(CdlPause, NULL);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_HALT;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                D_8005A4B4 += 1;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR;
                channel = D_8004FE38;
                D_80059F15 = channel;
                D_80059F14[0] = 1;
                CdControlF(CdlSetfilter, &D_80059F14);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_ADPCM;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
                D_8005A488 += 1;
                D_8005A494 += 1;
                CdReadyCallback(D_80059F08);
                CdControlF(CdlSetloc, &g_ArchiveCdCurLocation);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_ADPCM;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                D_8005A498 += 1;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_RESET_MODE:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
                CdControlF(CdlPause, NULL);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_RESET_MODE;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                D_8005A4B4 += 1;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_ERROR:
            if (status == CdlComplete && !(*pResult & CdlStatShellOpen)) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_HANDLE_ERROR;
                CdControlF(CdlGetTN, NULL);
            } else {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;
        
        case ARCHIVE_CD_DRIVE_HANDLE_ERROR:
            // Error handler
            if (status == CdlComplete) {
                switch (g_ArchiveCdDriveError) {
                    case ARCHIVE_CD_DRIVE_ERR_SEEK:
                        // Set the target location and retry
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SEEK;
                        CdControlF(CdlSetloc, &g_ArchiveCdCurLocation);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_GENERIC:
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_DONE;
                        CdControlF(CdlPause, NULL);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_READ:
                        // Pause the read and retry
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_DATA_SECTOR;
                        CdControlF(CdlPause, NULL);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_HALT:
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_HALT;
                        CdControlF(CdlStop, NULL);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_ADPCM:
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL;
                        CdControlF(CdlSetmode, &D_80059F18);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_RESET_MODE:
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_RESET_MODE;
                        CdControlF(CdlSetmode, &D_80059F18);
                        break;
                    default:
                        return;
                }
            } else {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;
        
        case ARCHIVE_CD_DRIVE_IDLE:
        default:
            return;
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002AC24);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002B084);
/* Best attempt: objdiff 98.97%. Sole difference is where gcc materialises the
 * 0x200 constant for the last CdGetSector: the original hoists it into the delay
 * slot of the `bgez` that starts the (size + 3) / 4 division, this emits it after
 * the sra. Status is CdlDataReady (0x1) and the final command is CdlNop, not
 * CdlComplete/CdlPause.
 * void func_8002B084(u_char status) {
 *     s32 i;
 *     s32 j;
 *
 *     if (status != CdlDataReady)
 *         goto error;
 *
 *     if (D_8004FE34 > 0)
 *         goto halt;
 *
 *     if (g_ArchiveCurFileSize >= CD_SECTOR_SIZE) {
 *         CdGetSector(&D_80059EF8, 3);
 *         CdGetSector(D_8004FE08, CD_SECTOR_SIZE / 4);
 *     } else if (g_ArchiveCurFileSize > 0) {
 *         CdGetSector(&D_80059EF8, 3);
 *         CdGetSector(D_8004FE08, (g_ArchiveCurFileSize + 3) / 4);
 *         CdGetSector(D_800596F8, 512 - (g_ArchiveCurFileSize + 3) / 4);
 *     }
 *
 *     if (CdPosToInt(&D_80059EF8) != g_ArchiveCurFileSector) {
 *         D_8004FDE4 += 1;
 *         goto error;
 *     }
 *
 *     g_ArchiveCurFileSector += 1;
 *     D_8004FE08 += CD_SECTOR_SIZE;
 *     g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *     if (g_ArchiveCurFileSize > 0)
 *         return;
 *
 * halt:
 *     CdReadyCallback(NULL);
 *     g_ArchiveCurFileSize = 0;
 *     ArchiveCdSeekToFile(D_8004FE38);
 *     D_8004FDFC = 0;
 *     return;
 *
 * error:
 *     D_8005A4DC += 1;
 *     D_80059F08 = CdReadyCallback(NULL);
 *     CdIntToPos(g_ArchiveCurFileSector, &g_ArchiveCdCurLocation);
 *     if (D_8005A4DC < 3) {
 *         g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
 *     } else {
 *         for (i = 9999; i >= 0; i--) {
 *             for (j = 1999; j >= 0; j--) {
 *             }
 *         }
 *         D_8005A4DC = 0;
 *         g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_HALT;
 *         D_8005A4A4 += 1;
 *     }
 *     g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
 *     CdSyncCallback(&ArchiveCdDriveCommandHandler);
 *     CdControlF(CdlNop, NULL);
 * }
 */

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002B2F0);
/* Best attempt: objdiff 94.80%. Register allocation and the section-search loop
 * header match; the remaining deltas are (1) gcc hoists the D_8004FE2C load
 * above the loop-entry test where the original loads it after, (2) the original
 * reads D_8004FE26 twice (the increment reloads after the state store), and
 * (3) the beq delay slot holds `li v0,1` (the found-block state store) where
 * this emits the shared `li a1,0x200`. func_8002B5D0 is instruction-identical
 * to this function apart from its branch targets, so one solution fits both.
 * void func_8002B2F0(u_char status) {
 *     ArchiveStreamFileSectionHeader* pSection;
 *     ArchiveStreamFileSectionHeader* pCurrent;
 *     CdlLOC* pLoc;
 *     u8* pBuf;
 *     int entryIndex;
 *     int i;
 *     int j;
 *
 *     if (status != CdlDataReady)
 *         goto error;
 *
 *     if (D_8004FE34 > 0) {
 *         CdReadyCallback(NULL);
 *         CdDataCallback(NULL);
 *         g_ArchiveCurFileSize = 0;
 *         ArchiveCdSeekToFile(D_8004FE38);
 *         D_8004FDFC = 0;
 *         return;
 *     }
 *
 *     if (g_ArchiveCurFileSize > 0) {
 *         pSection = D_8004FE2C;
 *         for (i = 0; i < D_8004FE40; i++) {
 *             entryIndex = D_8004FE10;
 *             pCurrent = &pSection[entryIndex];
 *             D_8004FE10 = entryIndex + 1;
 *             if (D_8004FE10 >= D_8004FE40)
 *                 D_8004FE10 = 0;
 *             if (pCurrent->state == 0)
 *                 break;
 *         }
 *
 *         if (pCurrent->state != 0)
 *             goto errorNoInc;
 *
 *         pLoc = &D_80059EF8;
 *         CdGetSector(pLoc, 3);
 *         if (CdPosToInt(pLoc) == g_ArchiveCurFileSector) {
 *             pCurrent->id = D_8004FE26;
 *             pCurrent->state = 1;
 *             D_8004FE26 += 1;
 *             CdGetSector(D_8004FE08 + entryIndex * CD_SECTOR_SIZE, CD_SECTOR_SIZE / 4);
 *             g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *             g_ArchiveCurFileSector += 1;
 *             if (g_ArchiveCurFileSize > 0)
 *                 return;
 *         } else {
 *             D_8004FDEC += 1;
 *             pBuf = D_800596F8;
 *             CdGetSector(pBuf, CD_SECTOR_SIZE / 4);
 *             goto error;
 *         }
 *     }
 *
 *     CdReadyCallback(NULL);
 *     g_ArchiveCurFileSize = 0;
 *     return;
 *
 * error:
 *     D_8005A4DC += 1;
 * errorNoInc:
 *     D_80059F08 = CdReadyCallback(NULL);
 *     CdIntToPos(g_ArchiveCurFileSector, &g_ArchiveCdCurLocation);
 *     if (D_8005A4DC < 3) {
 *         g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
 *     } else {
 *         for (i = 9999; i >= 0; i--) {
 *             for (j = 1999; j >= 0; j--) {
 *             }
 *         }
 *         D_8005A4DC = 0;
 *         g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_HALT;
 *         D_8005A4A4 += 1;
 *     }
 *     g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
 *     CdSyncCallback(&ArchiveCdDriveCommandHandler);
 *     CdControlF(CdlNop, NULL);
 * }
 */

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002B5D0);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002B8B0);
/* Best attempt: objdiff 83.60%. Only the loop registers differ (gcc puts the
 * section count in v1 and the current entry in a0; the original uses a0 for the
 * count, a1 for the entry index and a2 for the pointer).
 * void func_8002B8B0(void) {
 *     ArchiveStreamFileSectionHeader* pSection;
 *     ArchiveStreamFileSectionHeader* pCurrent;
 *     s16 i;
 *     int entryIndex;
 *
 *     if (g_ArchiveCurFileSize <= 0) {
 *         g_ArchiveCurFileSize = 0;
 *         return;
 *     }
 *
 *     pSection = D_8004FE2C;
 *     for (i = 0; i < D_8004FE40; i++) {
 *         entryIndex = D_8004FE10;
 *         D_8004FE10 = entryIndex + 1;
 *         if (D_8004FE10 >= D_8004FE40) {
 *             D_8004FE10 = 0;
 *         }
 *         pCurrent = &pSection[entryIndex];
 *         if (pCurrent->state == 0) {
 *             break;
 *         }
 *     }
 *
 *     if (pCurrent->state == 0) {
 *         pCurrent->state = 1;
 *         pCurrent->id = D_8004FE26;
 *         D_8004FE26 += 1;
 *         for (i = 0; i < RETRY_COUNT; i++) {
 *             if (PCread(D_80059F04, D_8004FE08 + entryIndex * CD_SECTOR_SIZE, CD_SECTOR_SIZE) != 0)
 *                 break;
 *             func_8002804C(i, 0, 0xFF, 0);
 *         }
 *         g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *         g_ArchiveCurFileSector += 1;
 *     } else {
 *         g_ArchiveCurFileSize = 0;
 *     }
 *
 *     if (g_ArchiveCurFileSize <= 0) {
 *         g_ArchiveCurFileSize = 0;
 *     }
 * }

 */

void func_8002BA40(void) {
    D_8004FDFC = D_8004FE00;
}

void func_8002BA58(void) {
    ArchiveStreamFileSectionHeader* pSection;
    ArchiveStreamFileSectionHeader* pCurrent;
    s16 i;

    pSection = D_8004FE2C;
    for (i = 0; i < D_8004FE40; i++, pSection++) {
        if (pSection->state == 1 && pSection->id == D_8004FE28)
            break;
    }

    if (i == D_8004FE40)
        return;

    pSection->state = 3;
    D_8004FE28 += 1;
    if (g_ArchiveCurFileSize > 0)
        return;

    if (D_8004FDFC >= 2)
        return;

    g_ArchiveCurFileSize = 0;
    CdDataCallback(NULL);
    ArchiveCdSeekToFile(D_8004FE38);
    D_8004FDFC = 0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002BB50);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002BF38);

void func_8002C310(void) {
    func_800379C8(D_80018944, g_ArchiveTable, D_8004FE08, D_8004FE2C);
    func_800379C8(D_80018954, D_8004FE40, D_8004FE10, D_80059F3C, D_80059F50);
    func_800379C8(D_80018964, D_80059F40, D_80059F44, D_80059F48, D_80059F4C);
    func_800379C8(D_80018974, D_80059F4C[0], D_80059F4C[1], D_80059F4C[2]);
}

u32 ArchiveGetDebugTable(void) {
    return g_ArchiveDebugTable;
}
