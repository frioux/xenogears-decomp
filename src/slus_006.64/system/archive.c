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
#include "psyq/libgpu.h"

extern s8* g_ArchiveStreamWritePointer;
extern StreamDataQueueEntry* g_ArchiveStreamQueue;
extern s32 g_ArchiveStreamQueueCount;
extern s32 g_ArchiveStreamQueueIndex;
extern s16 D_8004FE24;
extern u16 g_ArchiveStreamNextSectionId;
extern u16 g_ArchiveStreamCurSectionId;
extern void* g_ArchiveCurStreamFile; // Current stream file struct
extern s32 D_80059F00;
extern u32 g_ArchiveCurEntryIndex;
extern s16 D_80059F60;
extern s32 g_ArchiveReadRetryCount;
extern void ArchiveCdReadyReadFile(u_char status);
extern void func_8002B2F0(u_char status);
extern void ArchiveStreamFileDataCallback();
extern void ArchiveRestorePendingCount();
void ArchiveCdReadyReadQueuedFile(u_char status);
extern void ArchiveCdDataSync(int);
extern int ArchiveDecodeSizeAligned(int);
extern int ArchiveDecodeSectorAligned(int);
extern s32 g_ArchiveCurFileSector;
extern char g_StrArchiveDebugStreamState[];
extern char g_StrArchiveDebugCounters[];
extern char g_StrArchiveDebugImageRect[];
extern char g_StrArchiveDebugChunkHeights[];
extern void func_800379C8(char*, ...);
extern s32 g_ArchiveImageBlocksLeft;
extern int D_80059F04;
extern CdlLOC g_ArchiveSectorLocation;
extern u8 g_ArchiveSectorScratch[];
extern s32 g_ArchiveFilePositionErrorCount;
extern s32 D_8004FDEC;
extern s32 g_ArchiveStreamResumedFile;
extern s32 g_ArchiveStreamSeekWindow;
extern s32 g_ArchiveStreamPositionErrors;
extern s16 g_ArchiveImageRectX;
extern s16 g_ArchiveImageRectY;
extern s16 g_ArchiveImageRectW;
extern u16* g_ArchiveImageChunkHeights;
extern s32 g_ArchiveImageChunksLeft;
extern s16 D_80059F24;
extern u16 D_80059F28;
extern u16 D_80059F2C;
extern s16 D_80059F30;
extern u16 D_80059F34;
extern u16 D_80059F38;

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

    g_ArchiveCurEntryIndex = dbgEntryIndex;
    for (i = 2, pData = &D_80059F00; i >= 0; i--)
        *pData-- = 0;
    
    g_ArchivePendingReadCount = 1;
    g_ArchiveStreamWritePointer = pDestBuffer;
    g_ArchiveCdChannel = arg2 & 0xFFFF;
    g_ArchiveStreamQueueIndex = 0;
    g_ArchiveStreamQueue = NULL;
    g_ArchiveStreamStopFlag = 0;
    g_ArchiveReadRetryCount = 0;
    pLoc = &g_ArchiveCdCurLocation;
    CdIntToPos(g_ArchiveCurFileSector, pLoc);
    
    if (flags & CdlModeStream) {
        ArchiveChangeStreamingFile(pDestBuffer);
        nStreamSectors = *(u32*)g_ArchiveCurStreamFile;
        if (nStreamSectors) {
            g_ArchiveStreamWritePointer = g_ArchiveCurStreamFile + nStreamSectors * sizeof(ArchiveStreamFileSectionHeader) + STREAM_FILE_HEADER_SIZE;
            g_ArchiveStreamSections = g_ArchiveCurStreamFile + 4;
            g_ArchiveStreamSectionCount = nStreamSectors;
            g_ArchiveStreamNextSectionId = 0;
            g_ArchiveStreamCurSectionId = 0;
            D_8004FE24 = 0;
            ArchiveClearStreamFileSections();
            
            if (g_ArchiveDebugTable) {
                pFilePath2 = ArchiveGetFilePath(dbgEntryIndex);
                
                for (i = 0; i < RETRY_COUNT; i++) {
                    g_ArchiveDebugFileHandle = PCopen(pFilePath2, 0, 0);
                    if (g_ArchiveDebugFileHandle == -1)
                        GfxDrawArchiveReadRetry(i, 0xFF, 0, 0);
                    else
                        break;
                }

                return -(~g_ArchiveDebugFileHandle == 0) & -3;
            }

            g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
            CdDataCallback(&ArchiveStreamFileDataCallback);
            CdSyncCallback(&ArchiveCdDriveCommandHandler);
            pReadyCallback = &func_8002B2F0;
            CdReadyCallback(pReadyCallback);
            g_ArchiveCdReadStartCount += 1;
            CdControlF(CdlSetloc, (void*)pLoc);
            return 0;
        }
        return -4;
    }
    
    if (flags & 0x200) {
        ArchiveChangeStreamingFile(pDestBuffer);
        nStreamSectors2 = *(u32*)g_ArchiveCurStreamFile;
        if (nStreamSectors2) {
            g_ArchiveStreamWritePointer = g_ArchiveCurStreamFile + (nStreamSectors2 * sizeof(ArchiveStreamFileSectionHeader)) + STREAM_FILE_HEADER_SIZE;
            g_ArchiveStreamSections = g_ArchiveCurStreamFile + 4;
            g_ArchiveStreamSectionCount = nStreamSectors2;
            D_80059F60 = 0;
            g_ArchiveStreamNextSectionId = 0;
            g_ArchiveStreamCurSectionId = 0;
            D_8004FE24 = 0;
            ArchiveClearStreamFileSections();
            
            pCdMode = &D_80059F1B;
            for (i = 3; i >= 0; i--)
                *pCdMode-- = 0;

            // CD Mode - double speed, 2340 byte size
            g_ArchiveCdMode[0] = flags | CdlModeSpeed | CdlModeSize1;
            
            if (g_ArchiveDebugTable) {
                pFilePath2 = ArchiveGetFilePath(dbgEntryIndex);

                for (i = 0; i < RETRY_COUNT; i++) {
                    g_ArchiveDebugFileHandle = PCopen(pFilePath2, 0, 0);
                    if (g_ArchiveDebugFileHandle == -1)
                        GfxDrawArchiveReadRetry(i, 0xFF, 0, 0);
                    else
                        break;
                }
                
                return -(~g_ArchiveDebugFileHandle == 0) & -3;
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
                GfxDrawArchiveReadRetry(i, 0xFF, 0, 0);
            else
                break;
        }

        if (hFile == -1) {
            return -4;
        }

        if (pDestBuffer != NULL) {
            for (i = 0; i < RETRY_COUNT; i++) {
                if (PCread(hFile, pDestBuffer, g_ArchiveCurFileSize) == 0)
                    GfxDrawArchiveReadRetry(i, 0, 0xFF, 0);
                else
                    break;
            }
        }
        
        for (i = 0; i < RETRY_COUNT; i++) {
            if (PCclose(hFile) != 0) {
                GfxDrawArchiveReadRetry(i, 0, 0, 0xFF);
            } else {
                g_ArchiveCurFileSize = 0;
                g_ArchivePendingReadCount = 0;
                return 0;
            }
        }

        return -6;
    }
    
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
    CdDataCallback(NULL);
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    pReadyCallback = &ArchiveCdReadyReadFile;
    CdReadyCallback(pReadyCallback);
    g_ArchiveCdReadStartCount += 1;
    CdControlF(CdlSetloc, (void*)pLoc);
    return 0;
}

/* Best attempt: objdiff 81.29% (952 bytes vs retail 940), first draft, decoded
 * from the .s. Structure confirmed: count entries (nCount = index of the first
 * entry whose archiveIndex is 0), selection-sort pQueue[0..nCount-2] by
 * archiveIndex (both fields swapped; the outer bound really is nCount-1), then
 * the CD setup copied from ArchiveReadFile and either the PCopen/PCread/PCclose
 * debug loop over all nCount entries or the streaming callback path
 * (CdDataCallback(ArchiveRestorePendingCount), CdReadyCallback(ArchiveCdReadyReadQueuedFile), CdlSetloc).
 * Signature should be `int func_80029AFC(StreamDataQueueEntry*, int)` - retail
 * returns -3 / 0 (callers in src/battle/main/misc23.c, world_map/main/misc1.c
 * and archive_b.c pass (pQueue, 0, 0), so a third unused parameter is fine).
 * Known deltas of this draft:
 *  1. the header declares StreamDataQueueEntry.archiveIndex as `short`, retail
 *     loads it with lhu and compares with sltu -> the field is really u16.
 *     Do not flip the shared header type blindly; index through
 *     ((u16*)pQueue)[i * 4] or check the other users first.
 *  2. frame 0x50 vs retail 0x40 and one extra saved register: too many live
 *     locals across the debug loop (pFilePath/hFile/nRetry/pLoc). Retail keeps
 *     pEntry in s4, pFilePath in s3, hFile in s2, i in s0 and re-reads nothing.
 *  3. retail returns via `bnez nCount,1f; j epilogue; li v0,-3`.
 */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_80029AFC);
/*
 * void func_80029AFC(StreamDataQueueEntry* pQueue, int channel) {
 *     StreamDataQueueEntry* pEntry;
 *     StreamDataQueueEntry* pMinor;
 *     s32* pWord;
 *     void* pTempData;
 *     char* pFilePath;
 *     int hFile;
 *     int nCount;
 *     int i;
 *     int j;
 *     int nMinIndex;
 *     int nRetry;
 *     CdlLOC* pLoc;
 *     u16 nTempIndex;
 *
 *     if (pQueue == NULL) {
 *         return -3;
 *     }
 *
 *     nCount = 0;
 *     if (pQueue->archiveIndex != 0) {
 *         pEntry = pQueue;
 *         do {
 *             pEntry++;
 *             nCount++;
 *         } while (pEntry->archiveIndex != 0);
 *     }
 *
 *     if (nCount == 0) {
 *         return -3;
 *     }
 *
 *     for (i = 0; i < nCount - 1; i++) {
 *         nMinIndex = i;
 *         nTempIndex = pQueue[i].archiveIndex;
 *         for (j = i + 1; j < nCount; j++) {
 *             if (pQueue[j].archiveIndex < nTempIndex) {
 *                 nMinIndex = j;
 *                 nTempIndex = pQueue[j].archiveIndex;
 *             }
 *         }
 *         pTempData = pQueue[i].pData;
 *         pQueue[i].archiveIndex = pQueue[nMinIndex].archiveIndex;
 *         pQueue[i].pData = pQueue[nMinIndex].pData;
 *         pQueue[nMinIndex].archiveIndex = nTempIndex;
 *         pQueue[nMinIndex].pData = pTempData;
 *     }
 *
 *     ArchiveCdDataSync(0);
 *     g_ArchiveHeaderOffsetBase = g_CurArchiveOffset;
 *     for (i = 2, pWord = &D_80059F00; i >= 0; i--) {
 *         *pWord-- = 0;
 *     }
 *
 *     g_ArchiveStreamQueueIndex = 0;
 *     g_ArchiveStreamQueue = pQueue;
 *     g_ArchivePendingReadCount = nCount;
 *     g_ArchiveStreamQueueCount = nCount;
 *     g_ArchiveStreamWritePointer = pQueue->pData;
 *
 *     if (pQueue->archiveIndex == 0 || pQueue->pData == NULL) {
 *         ArchiveCdSeekToFile(channel);
 *     } else {
 *         g_ArchiveCurEntryIndex = pQueue->archiveIndex;
 *         g_ArchiveCurFileSector = ArchiveDecodeSector(pQueue->archiveIndex);
 *         g_ArchiveCurFileSize = ArchiveDecodeSizeAligned(pQueue->archiveIndex);
 *         g_ArchiveCdChannel = channel & 0xFFFF;
 *         g_ArchiveStreamResumedFile = 0;
 *         g_ArchiveStreamStopFlag = 0;
 *         g_ArchiveReadRetryCount = 0;
 *         pLoc = &g_ArchiveCdCurLocation;
 *         CdIntToPos(g_ArchiveCurFileSector, pLoc);
 *
 *         if (g_ArchiveDebugTable != 0) {
 *             for (i = 0, pEntry = pQueue; i < nCount; i++, pEntry++) {
 *                 g_ArchiveCurEntryIndex = pEntry->archiveIndex;
 *                 pFilePath = ArchiveGetFilePath(pEntry->archiveIndex);
 *                 for (nRetry = 0; nRetry < RETRY_COUNT; nRetry++) {
 *                     hFile = PCopen(pFilePath, 0, 0);
 *                     if (hFile == -1) {
 *                         GfxDrawArchiveReadRetry(nRetry, 0xFF, 0, 0);
 *                     } else {
 *                         break;
 *                     }
 *                 }
 *
 *                 if (pEntry->pData != NULL) {
 *                     for (nRetry = 0; nRetry < RETRY_COUNT; nRetry++) {
 *                         if (PCread(hFile, pEntry->pData, ArchiveDecodeSizeAligned(pEntry->archiveIndex)) != 0) {
 *                             break;
 *                         }
 *                         GfxDrawArchiveReadRetry(nRetry, 0, 0xFF, 0);
 *                     }
 *                 }
 *
 *                 for (nRetry = 0; nRetry < RETRY_COUNT; nRetry++) {
 *                     if (PCclose(hFile) == 0) {
 *                         break;
 *                     }
 *                     GfxDrawArchiveReadRetry(nRetry, 0, 0, 0xFF);
 *                 }
 *             }
 *         } else {
 *             g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
 *             CdDataCallback(&ArchiveRestorePendingCount);
 *             CdSyncCallback(&ArchiveCdDriveCommandHandler);
 *             CdReadyCallback(&ArchiveCdReadyReadQueuedFile);
 *             g_ArchiveCdReadStartCount += 1;
 *             CdControlF(CdlSetloc, (void*)pLoc);
 *             return 0;
 *         }
 *     }
 *
 *     g_ArchiveCurFileSize = 0;
 *     g_ArchivePendingReadCount = 0;
 *     return 0;
 * }
 */

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
        g_ArchiveHeaderOffsetBase = g_CurArchiveOffset;
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
    
    g_ArchiveCdMode[0] = mode;
    CdControlF(CdlSetmode, &g_ArchiveCdMode);
}

void ArchiveStopTransfer(int channel) {
    int i;

    g_ArchiveStreamStopFlag = 1;
    g_ArchiveCdChannel = channel;
    if (g_ArchiveDebugTable) {
        g_ArchiveCurFileSize = 0;
        g_ArchivePendingReadCount = 0;

        // Close (debug) streaming file handle
        if (g_ArchiveDebugFileHandle != -1) {
            while (i = PCclose(g_ArchiveDebugFileHandle)) {
                if (i != 0) {
                    if (i + 1 < 4)
                        continue;
                }
                break;
            }
            g_ArchiveDebugFileHandle = -1;
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
                g_ArchiveCdReadCompleteCount += 1;
                g_ArchiveCdDriveState += 1;
                CdControlF(CdlReadN, NULL);
            } else {
                g_ArchiveCdReadStatusErrorCount += 1;
                g_ArchiveSavedReadyCallback = CdReadyCallback(NULL);
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_READ_DONE:
            if (status == CdlComplete) {
                g_ArchiveCdReadCompleteCount += 1;
                CdSyncCallback(NULL);
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_IDLE;
                return;
            }
            g_ArchiveCdReadStatusErrorCount += 1;
            g_ArchiveSavedReadyCallback = CdReadyCallback(NULL);
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
                g_ArchiveCdReadStartCount += 1;
                g_ArchiveCdDataSectorCompleteCount += 1;
                CdReadyCallback(g_ArchiveSavedReadyCallback);
                CdControlF(CdlSetloc, &g_ArchiveCdCurLocation);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
                g_ArchiveCdDataSectorErrorCount += 1;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;
        
        case ARCHIVE_CD_DRIVE_HALT:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_DATA_SECTOR;
                g_ArchiveCdHaltCompleteCount += 1;
                CdControlF(CdlPause, NULL);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_HALT;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                g_ArchiveCdHaltErrorCount += 1;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_SET_ADPCM_PLAY_CHANNEL:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR;
                channel = g_ArchiveCdChannel;
                D_80059F15 = channel;
                g_ArchiveCdFilter[0] = 1;
                CdControlF(CdlSetfilter, &g_ArchiveCdFilter);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_ADPCM;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                CdControlF(CdlNop, NULL);
            }
            break;

        case ARCHIVE_CD_DRIVE_READ_ADPCM_SECTOR:
            if (status == CdlComplete) {
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
                g_ArchiveCdReadStartCount += 1;
                g_ArchiveCdDataSectorCompleteCount += 1;
                CdReadyCallback(g_ArchiveSavedReadyCallback);
                CdControlF(CdlSetloc, &g_ArchiveCdCurLocation);
            } else {
                g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_ADPCM;
                g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
                g_ArchiveCdDataSectorErrorCount += 1;
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
                g_ArchiveCdHaltErrorCount += 1;
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
                        CdControlF(CdlSetmode, &g_ArchiveCdMode);
                        break;
                    case ARCHIVE_CD_DRIVE_ERR_RESET_MODE:
                        g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_RESET_MODE;
                        CdControlF(CdlSetmode, &g_ArchiveCdMode);
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

void ArchiveCdReadyReadQueuedFile(u_char status) {
    StreamDataQueueEntry* pEntry;
    void* pData;
    u16 nId;
    u32 nSector;
    s32 entryIndex;
    s32 nWords;
    s32 nHalfSector;
    s32 i;
    s32 j;

    if (status != CdlDataReady)
        goto error;

    if (g_ArchiveStreamStopFlag > 0) {
        CdReadyCallback(NULL);
        CdDataCallback(NULL);
        g_ArchiveCurFileSize = 0;
        ArchiveCdSeekToFile(g_ArchiveCdChannel);
        g_ArchiveStreamQueueCount = 0;
        g_ArchivePendingReadCount = 0;
        return;
    }

    if (g_ArchiveCurFileSize >= CD_SECTOR_SIZE) {
        if (g_ArchiveStreamResumedFile == 0) {
            CdGetSector(&g_ArchiveSectorLocation, 3);
            CdGetSector(g_ArchiveStreamWritePointer, CD_SECTOR_SIZE / 4);
        }
    } else if (g_ArchiveCurFileSize > 0) {
        if (g_ArchiveStreamResumedFile == 0) {
            CdGetSector(&g_ArchiveSectorLocation, 3);
            CdGetSector(g_ArchiveStreamWritePointer, (g_ArchiveCurFileSize + 3) / 4);
            nWords = (g_ArchiveCurFileSize + 3) / 4;
            nHalfSector = CD_SECTOR_SIZE / 4;
            nHalfSector = nHalfSector - nWords;
            CdGetSector(g_ArchiveSectorScratch, nHalfSector);
        }
    }

    if ((CdPosToInt(&g_ArchiveSectorLocation) != g_ArchiveCurFileSector) && (g_ArchiveStreamResumedFile == 0)) {
        g_ArchiveStreamPositionErrors += 1;
        goto error;
    }

    g_ArchiveStreamWritePointer += CD_SECTOR_SIZE;
    g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
    g_ArchiveCurFileSector += 1;
    if (g_ArchiveCurFileSize > 0)
        return;

    entryIndex = g_ArchiveStreamQueueIndex + 1;
    pEntry = (StreamDataQueueEntry*)((entryIndex * sizeof(StreamDataQueueEntry)) + (mem_addr)g_ArchiveStreamQueue);
    nId = pEntry->archiveIndex;
    g_ArchiveStreamQueueIndex = entryIndex;
    pData = pEntry->pData;
    g_ArchiveStreamWritePointer = pData;

    if ((nId == 0) || (pData == NULL))
        goto noEntry;

    nSector = ArchiveDecodeSectorAligned(nId);
    g_ArchiveCurFileSize = ArchiveDecodeSizeAligned(nId);

    if (g_ArchiveCurFileSector < nSector) {
        if (g_ArchiveCurFileSector + g_ArchiveStreamSeekWindow < nSector)
            goto seek;
        g_ArchiveStreamResumedFile = 1;
        g_ArchiveCurFileSize = (g_ArchiveCurFileSector - nSector) * CD_SECTOR_SIZE;
        g_ArchiveStreamQueueIndex -= 1;
        return;
    }

seek:
    if (nSector == g_ArchiveCurFileSector) {
        g_ArchiveStreamResumedFile = 0;
        g_ArchiveStreamQueueCount -= 1;
        return;
    }

    g_ArchiveStreamResumedFile = 0;
    g_ArchiveCurFileSector = nSector;
    g_ArchiveSavedReadyCallback = CdReadyCallback(NULL);
    CdIntToPos(g_ArchiveCurFileSector, &g_ArchiveCdCurLocation);
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_DATA_SECTOR;
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    CdControlF(CdlPause, NULL);
    g_ArchiveStreamQueueCount -= 1;
    return;

noEntry:
    g_ArchiveCurFileSize = 0;
    CdReadyCallback(NULL);
    ArchiveCdSeekToFile(g_ArchiveCdChannel);
    g_ArchiveStreamQueueCount = 0;
    g_ArchivePendingReadCount = 0;
    return;

error:
    g_ArchiveReadRetryCount += 1;
    g_ArchiveSavedReadyCallback = CdReadyCallback(NULL);
    CdIntToPos(g_ArchiveCurFileSector, &g_ArchiveCdCurLocation);
    if (g_ArchiveReadRetryCount < 3) {
        g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
    } else {
        for (i = 9999; i >= 0; i--) {
            for (j = 1999; j >= 0; j--) {
            }
        }
        g_ArchiveReadRetryCount = 0;
        g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_HALT;
        g_ArchiveCdReadRetryExhaustedCount += 1;
    }
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    CdControlF(CdlNop, NULL);
}

void ArchiveCdReadyReadFile(u_char status) {
    s32 nHalfSector;
    s32 nWords;
    s32 i;
    s32 j;

    if (status != CdlDataReady)
        goto error;

    if (g_ArchiveStreamStopFlag > 0)
        goto halt;

    if (g_ArchiveCurFileSize >= CD_SECTOR_SIZE) {
        CdGetSector(&g_ArchiveSectorLocation, 3);
        CdGetSector(g_ArchiveStreamWritePointer, CD_SECTOR_SIZE / 4);
    } else if (g_ArchiveCurFileSize > 0) {
        CdGetSector(&g_ArchiveSectorLocation, 3);
        CdGetSector(g_ArchiveStreamWritePointer, (g_ArchiveCurFileSize + 3) / 4);
        nWords = (g_ArchiveCurFileSize + 3) / 4;
        nHalfSector = CD_SECTOR_SIZE / 4;
        nHalfSector = nHalfSector - nWords;
        CdGetSector(g_ArchiveSectorScratch, nHalfSector);
    }

    if (CdPosToInt(&g_ArchiveSectorLocation) != g_ArchiveCurFileSector) {
        g_ArchiveFilePositionErrorCount += 1;
        goto error;
    }

    g_ArchiveCurFileSector += 1;
    g_ArchiveStreamWritePointer += CD_SECTOR_SIZE;
    g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
    if (g_ArchiveCurFileSize > 0)
        return;

halt:
    CdReadyCallback(NULL);
    g_ArchiveCurFileSize = 0;
    ArchiveCdSeekToFile(g_ArchiveCdChannel);
    g_ArchivePendingReadCount = 0;
    return;

error:
    g_ArchiveReadRetryCount += 1;
    g_ArchiveSavedReadyCallback = CdReadyCallback(NULL);
    CdIntToPos(g_ArchiveCurFileSector, &g_ArchiveCdCurLocation);
    if (g_ArchiveReadRetryCount < 3) {
        g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
    } else {
        for (i = 9999; i >= 0; i--) {
            for (j = 1999; j >= 0; j--) {
            }
        }
        g_ArchiveReadRetryCount = 0;
        g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_HALT;
        g_ArchiveCdReadRetryExhaustedCount += 1;
    }
    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    CdControlF(CdlNop, NULL);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002B2F0);

/* Best attempt: objdiff 96.81% (this was 96.40% before). func_8002B5D0 is
 * instruction-identical to this function (verified by diffing both .s files modulo
 * addresses), so one body matches both. Everything outside the found-block after
 * CdPosToInt matches byte for byte.
 *
 * What this attempt got right:
 *  - the search loop reads the globals directly (`for (i = 0; i < g_ArchiveStreamSectionCount; i++)`
 *    with `entryIndex = g_ArchiveStreamQueueIndex++` and g_ArchiveStreamSections in the body) so gcc hoists the
 *    loop-invariant copies (`lw v0,g_ArchiveStreamSectionCount` + `blez v0`, `move a1,v0`);
 *  - the two delay loops need their OWN counters (k/l); reusing the search loop's
 *    `i` puts the outer 9999 counter in a0 instead of v1;
 *  - `g_ArchiveStreamNextSectionId = *(volatile u16*)&g_ArchiveStreamNextSectionId + 1;` is the spelling that keeps
 *    retail's TWO lhu of the id counter. gcc's cse merges a plain second read with
 *    the `pCurrent->id = g_ArchiveStreamNextSectionId` read (the store through s1 in between is not
 *    treated as aliasing), so the increment read has to be volatile to survive.
 *
 * Remaining delta: same instruction set, same size, three ordering differences,
 * all inside the found block. Retail:
 *   li v0,0x1 <- (reorg steals it into the beq delay slot) ; li a1,0x200 ;
 *   lhu a2,g_ArchiveStreamNextSectionId ; sll a0,s2,0xb ; sh v0,0(s1) ; lhu v0,g_ArchiveStreamNextSectionId ;
 *   lui/lw v1,g_ArchiveStreamWritePointer ; addiu v0,v0,1 ; sh a2,2(s1) ; sh v0,g_ArchiveStreamNextSectionId ;
 *   jal CdGetSector ; addu a0,a0,v1
 * Ours:
 *   li a1,0x200 <- ; li v0,0x1 ; lhu a2,g_ArchiveStreamNextSectionId ; lui/lw a0,g_ArchiveStreamWritePointer ;
 *   sll v1,s2,0xb ; sh v0,0(s1) ; lhu v0,g_ArchiveStreamNextSectionId ; nop ; addiu v0,v0,1 ;
 *   sh a2,2(s1) ; sh v0,g_ArchiveStreamNextSectionId ; jal CdGetSector ; addu a0,v1,a0
 * All three are sched2's choice of insn order, not the C:
 *  1. the state constant is emitted after the call's 0x200 constant, so reorg steals
 *     the wrong one into the beq delay slot;
 *  2. `lw g_ArchiveStreamWritePointer` is scheduled before `sll`, which also hands a0 to the load and
 *     v1 to the shift (retail: sll into a0, load into v1, `addu a0,a0,v1`);
 *  3. with the load no longer sitting between the two id reads, MASPSX has to insert
 *     a load-delay nop after the volatile `lhu v0` (see
 *     `_handle_nop_before_next_instruction`: any `lhu $r,sym` whose very next insn
 *     reads $r gets a nop), which is why the size ends up equal while everything
 *     after the block sits 4 bytes early.
 * Measured and rejected: state-first WITHOUT volatile is 97.46% but has only one
 * lhu and is 728 bytes (one insn short - there is no way to make the second plain
 * read survive cse); both id reads volatile 96.81% (same block as here); volatile
 * read into a u16 local then `g_ArchiveStreamNextSectionId = nId + 1` 96.81%; volatile increment read
 * first and plain read second 96.48%; `nOffset = entryIndex * CD_SECTOR_SIZE;` as
 * its own statement before the call 96.81% (gcc folds the local back). Next idea:
 * make sched2 emit the shift before the load - that is the insn that decides the
 * register allocation and the nop - or find a non-volatile spelling of the second
 * read that cse cannot merge.
 *
 * void func_8002B2F0(u_char status) {
 *     ArchiveStreamFileSectionHeader* pCurrent;
 *     CdlLOC* pLoc;
 *     u8* pBuf;
 *     int entryIndex;
 *     int i;
 *     int k;
 *     int l;
 *
 *     if (status != CdlDataReady)
 *         goto error;
 *
 *     if (g_ArchiveStreamStopFlag > 0) {
 *         CdReadyCallback(NULL);
 *         CdDataCallback(NULL);
 *         g_ArchiveCurFileSize = 0;
 *         ArchiveCdSeekToFile(g_ArchiveCdChannel);
 *         g_ArchivePendingReadCount = 0;
 *         return;
 *     }
 *
 *     if (g_ArchiveCurFileSize > 0) {
 *         for (i = 0; i < g_ArchiveStreamSectionCount; i++) {
 *             entryIndex = g_ArchiveStreamQueueIndex++;
 *             pCurrent = (ArchiveStreamFileSectionHeader*)(entryIndex * sizeof(ArchiveStreamFileSectionHeader) + (mem_addr)g_ArchiveStreamSections);
 *             if (g_ArchiveStreamQueueIndex >= g_ArchiveStreamSectionCount)
 *                 g_ArchiveStreamQueueIndex = 0;
 *             if (pCurrent->state == 0)
 *                 break;
 *         }
 *
 *         if (pCurrent->state != 0)
 *             goto errorNoInc;
 *
 *         pLoc = &g_ArchiveSectorLocation;
 *         CdGetSector(pLoc, 3);
 *         if (CdPosToInt(pLoc) == g_ArchiveCurFileSector) {
 *             pCurrent->state = 1;
 *             pCurrent->id = g_ArchiveStreamNextSectionId;
 *             g_ArchiveStreamNextSectionId = *(volatile u16*)&g_ArchiveStreamNextSectionId + 1;
 *             CdGetSector((u8*)(entryIndex * CD_SECTOR_SIZE + (mem_addr)g_ArchiveStreamWritePointer), CD_SECTOR_SIZE / 4);
 *             g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *             g_ArchiveCurFileSector += 1;
 *             if (g_ArchiveCurFileSize > 0)
 *                 return;
 *         } else {
 *             D_8004FDEC += 1;
 *             pBuf = g_ArchiveSectorScratch;
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
 *     g_ArchiveReadRetryCount += 1;
 * errorNoInc:
 *     g_ArchiveSavedReadyCallback = CdReadyCallback(NULL);
 *     CdIntToPos(g_ArchiveCurFileSector, &g_ArchiveCdCurLocation);
 *     if (g_ArchiveReadRetryCount < 3) {
 *         g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_READ;
 *     } else {
 *         for (k = 9999; k >= 0; k--) {
 *             for (l = 1999; l >= 0; l--) {
 *             }
 *         }
 *         g_ArchiveReadRetryCount = 0;
 *         g_ArchiveCdDriveError = ARCHIVE_CD_DRIVE_ERR_HALT;
 *         g_ArchiveCdReadRetryExhaustedCount += 1;
 *     }
 *     g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_ERROR;
 *     CdSyncCallback(&ArchiveCdDriveCommandHandler);
 *     CdControlF(CdlNop, NULL);
 * }
 */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002B5D0);

/* Best attempt: objdiff 91.80% (was 87.60%). 396 bytes, instruction multiset
 * identical to retail; all remaining deltas are sched2 ordering + one register
 * copy. What this version fixed vs the old park: numSections and pSection are
 * read through the globals *inside* the loop body so gcc 2.6 loop.c hoists
 * them into the post-entry-test preheader exactly like retail (lw g_ArchiveStreamSectionCount,
 * blez, move s0,zero, lw g_ArchiveStreamSections, move a0,v0); and the sector offset is a
 * local (`nSectorOffset`) rather than `entryIndex * CD_SECTOR_SIZE` inside the
 * call.
 * Remaining: (a) retail has `lw v0,g_ArchiveStreamQueueIndex; nop; move a1,v0` (a copy of the
 * loaded entry index before addiu/sll) while we allocate the load directly;
 * (b) inside the found block retail emits `li v0,0x1; sh v0,0(p)` before
 * `lhu v0,g_ArchiveStreamNextSectionId` and its `addiu v1,v0,1` before the id store, ours emits
 * them after (same registers otherwise). Measured: `pCurrent->id =
 * g_ArchiveStreamNextSectionId++` 90.19%; old park 87.60%.
 */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002B8B0);
/* void func_8002B8B0(void) {
 *     ArchiveStreamFileSectionHeader* pCurrent;
 *     s16 i;
 *     int entryIndex;
 *     int nSectorOffset;
 *
 *     if (g_ArchiveCurFileSize > 0) {
 *         for (i = 0; i < g_ArchiveStreamSectionCount; i++) {
 *             entryIndex = g_ArchiveStreamQueueIndex;
 *             pCurrent = &g_ArchiveStreamSections[entryIndex];
 *             g_ArchiveStreamQueueIndex = entryIndex + 1;
 *             if (g_ArchiveStreamQueueIndex >= g_ArchiveStreamSectionCount) {
 *                 g_ArchiveStreamQueueIndex = 0;
 *             }
 *             if (pCurrent->state == 0) {
 *                 break;
 *             }
 *         }
 *
 *         if (pCurrent->state == 0) {
 *             pCurrent->state = 1;
 *             pCurrent->id = g_ArchiveStreamNextSectionId;
 *             g_ArchiveStreamNextSectionId += 1;
 *             nSectorOffset = entryIndex * CD_SECTOR_SIZE;
 *             for (i = 0; i < RETRY_COUNT; i++) {
 *                 if (PCread(D_80059F04, g_ArchiveStreamWritePointer + nSectorOffset, CD_SECTOR_SIZE) != 0) {
 *                     break;
 *                 }
 *                 GfxDrawArchiveReadRetry(i, 0, 0xFF, 0);
 *             }
 *             g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *             g_ArchiveCurFileSector += 1;
 *         } else {
 *             g_ArchiveCurFileSize = 0;
 *         }
 *     }
 *
 *     if (g_ArchiveCurFileSize <= 0) {
 *         g_ArchiveCurFileSize = 0;
 *     }
 * }
 */

void ArchiveRestorePendingCount(void) {
    g_ArchivePendingReadCount = g_ArchiveStreamQueueCount;
}

void ArchiveStreamFileDataCallback(void) {
    ArchiveStreamFileSectionHeader* pSection;
    ArchiveStreamFileSectionHeader* pCurrent;
    s16 i;

    pSection = g_ArchiveStreamSections;
    for (i = 0; i < g_ArchiveStreamSectionCount; i++, pSection++) {
        if (pSection->state == 1 && pSection->id == g_ArchiveStreamCurSectionId)
            break;
    }

    if (i == g_ArchiveStreamSectionCount)
        return;

    pSection->state = 3;
    g_ArchiveStreamCurSectionId += 1;
    if (g_ArchiveCurFileSize > 0)
        return;

    if (g_ArchivePendingReadCount >= 2)
        return;

    g_ArchiveCurFileSize = 0;
    CdDataCallback(NULL);
    ArchiveCdSeekToFile(g_ArchiveCdChannel);
    g_ArchivePendingReadCount = 0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002BB50);

/* Best attempt: objdiff 82.06% (1020 bytes vs retail 996). Structure is fully
 * decoded; every remaining delta is a register/block-placement tie, no missing
 * or extra instruction class. The function is a CD data callback: it finds the
 * stream-file section whose id matches D_80059F28's counter (state 1), marks it
 * state 2, then either feeds one queued LoadImage chunk from the sector or
 * parses the sector's 0x1200/0x1201 command block (position + size, scaling by
 * D_80059F24/D_80059F28-D_80059F2C for command 0x1200 and D_80059F30/
 * D_80059F34-D_80059F38 for 0x1201) and arms g_ArchiveImageChunkHeights/g_ArchiveImageChunksLeft for the
 * chunk loop.
 *
 * What is confirmed against retail:
 *  - the branch on g_ArchiveImageChunksLeft is `if (g_ArchiveImageChunksLeft == 0)` with the command path as
 *    the THEN block (retail bnez's out of line to the LoadImage block);
 *  - `nCommand >= 0x1200 && nCommand <= 0x1201` is retail's sltiu range check,
 *    and the two command bodies are two sequential ifs, not a switch with
 *    fallthrough (retail re-tests `nCommand != 0x1201` before the second body);
 *  - the inner dispatch on D_80059F24 / D_80059F30 is a 3-way switch whose
 *    case 1 and default share their tail (gcc cross-jumps them);
 *  - the section-clearing loop must be `pIter[j].state` (indexed) with a pointer
 *    local, not `pIter++`;
 *  - `pBuf = (u_long*)(i * CD_SECTOR_SIZE + (mem_addr)g_ArchiveStreamWritePointer);` (index first
 *    gives retail's addu operand order).
 * Remaining deltas: retail keeps i in a0 and the loop bound in v1 (ours the
 * other way round), keeps pBuf in a2 and the u16 alias in a3 (ours swapped), and
 * tests g_ArchiveImageBlocksLeft with bgtz to the shared state=0 tail where ours emits blez to
 * the error block (same blocks, opposite fallthrough). Tried: both tail shapes
 * (one shared `state = 0; g_ArchiveStreamCurSectionId++` after the if, and two copies that
 * cross-jump); gcc emits the same 82.06% for both.
 *
 * void func_8002BB50(void) {
 *     ArchiveStreamFileSectionHeader* pSection;
 *     ArchiveStreamFileSectionHeader* pIter;
 *     u_long* pBuf;
 *     u16* pData;
 *     u16* pChunk;
 *     RECT rect;
 *     s16 i;
 *     s16 j;
 *     s32 nCommand;
 *     s32 nChunks;
 *
 *     pSection = g_ArchiveStreamSections;
 *     for (i = 0; i < g_ArchiveStreamSectionCount; i++, pSection++) {
 *         if (pSection->state == 1 && pSection->id == g_ArchiveStreamCurSectionId)
 *             break;
 *     }
 *
 *     if (i == g_ArchiveStreamSectionCount)
 *         return;
 *
 *     pSection->state = 2;
 *     pBuf = (u_long*)(i * CD_SECTOR_SIZE + (mem_addr)g_ArchiveStreamWritePointer);
 *
 *     if (g_ArchiveImageChunksLeft == 0) {
 *         nCommand = *pBuf++;
 *         pData = (u16*)pBuf;
 *         if (nCommand >= 0x1200 && nCommand <= 0x1201) {
 *             if (nCommand == 0x1200) {
 *                 switch (D_80059F24) {
 *                 case 1:
 *                     g_ArchiveImageRectX = D_80059F28 + pData[2];
 *                     g_ArchiveImageRectY = D_80059F2C + pData[3];
 *                     break;
 *                 case 2:
 *                     g_ArchiveImageRectX = D_80059F28 + pData[0] + pData[2];
 *                     g_ArchiveImageRectY = D_80059F2C + pData[1] + pData[3];
 *                     break;
 *                 default:
 *                     g_ArchiveImageRectX = pData[0] + pData[2];
 *                     g_ArchiveImageRectY = pData[1] + pData[3];
 *                     break;
 *                 }
 *             }
 *             if (nCommand == 0x1201) {
 *                 switch (D_80059F30) {
 *                 case 1:
 *                     g_ArchiveImageRectX = D_80059F34 + pData[2];
 *                     g_ArchiveImageRectY = D_80059F38 + pData[3];
 *                     break;
 *                 case 2:
 *                     g_ArchiveImageRectX = D_80059F34 + pData[0] + pData[2];
 *                     g_ArchiveImageRectY = D_80059F38 + pData[1] + pData[3];
 *                     break;
 *                 default:
 *                     g_ArchiveImageRectX = pData[0] + pData[2];
 *                     g_ArchiveImageRectY = pData[1] + pData[3];
 *                     break;
 *                 }
 *             }
 *             pData += 4;
 *             g_ArchiveImageRectW = pData[0];
 *             pData += 4;
 *             if (g_ArchiveImageBlocksLeft == 0)
 *                 g_ArchiveImageBlocksLeft = *(u32*)pData;
 *             pData += 2;
 *             nChunks = *(u32*)pData;
 *             pData += 2;
 *             g_ArchiveImageChunkHeights = pData;
 *             g_ArchiveImageChunksLeft = nChunks;
 *             g_ArchiveStreamCurSectionId += 1;
 *             return;
 *         }
 *     } else {
 *         rect.x = g_ArchiveImageRectX;
 *         rect.y = g_ArchiveImageRectY;
 *         rect.w = g_ArchiveImageRectW;
 *         rect.h = g_ArchiveImageChunkHeights[0];
 *         LoadImage(&rect, pBuf);
 *         pChunk = g_ArchiveImageChunkHeights;
 *         g_ArchiveImageChunkHeights = pChunk + 1;
 *         g_ArchiveImageChunksLeft -= 1;
 *         g_ArchiveImageRectY += pChunk[0];
 *         if (g_ArchiveImageChunksLeft <= 0) {
 *             g_ArchiveImageBlocksLeft -= 1;
 *             g_ArchiveImageChunksLeft = 0;
 *             pIter = g_ArchiveStreamSections;
 *             for (j = 0; j < g_ArchiveStreamSectionCount; j++) {
 *                 pIter[j].state = 0;
 *                 pIter[j].id = 0;
 *             }
 *             if (g_ArchiveImageBlocksLeft > 0) {
 *                 pSection->state = 0;
 *                 g_ArchiveStreamCurSectionId += 1;
 *                 return;
 *             }
 *         } else {
 *             pSection->state = 0;
 *             g_ArchiveStreamCurSectionId += 1;
 *             return;
 *         }
 *     }
 *
 *     g_ArchiveCurFileSize = 0;
 *     CdDataCallback(NULL);
 *     ArchiveCdSeekToFile(g_ArchiveCdChannel);
 *     g_ArchivePendingReadCount = 0;
 * }
 */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_8002BF38);
/* Best attempt: objdiff 85.60% (992 bytes vs retail 984). The stream command record is
 * [0] cmd 0x1200/0x1201, [4..0xA] four u16 offsets, [0xC] width, [0x14] frames left,
 * [0x18] chunk count, [0x1C] chunk-height table; the section search loop and the
 * LoadImage strip loop match block for block. Remaining deltas: the search loop is
 * rotated the other way (retail ends with bnez, ours with beqz plus a j), the sector
 * address addu operand order (write i * CD_SECTOR_SIZE + (mem_addr)g_ArchiveStreamWritePointer), the
 * load order inside the two rect switches, and the g_ArchiveImageRectY/g_ArchiveImageChunkHeights/g_ArchiveImageChunksLeft
 * update order after DrawSync. Next step is a permuter run, not more hand shapes.
 *
 * void func_8002BF38(void) {
 *     ArchiveStreamFileSectionHeader* pSection;
 *     RECT rect;
 *     u32* pData;
 *     u16* pArgs;
 *     u32 cmd;
 *     s16 i;
 *
 *     i = 0;
 *     pSection = g_ArchiveStreamSections;
 *     if (g_ArchiveStreamSectionCount > 0) {
 *         do {
 *             if ((pSection->state == 1) && (pSection->id == g_ArchiveStreamCurSectionId))
 *                 break;
 *             i++;
 *             pSection++;
 *         } while (i < g_ArchiveStreamSectionCount);
 *     }
 *     if (i == g_ArchiveStreamSectionCount)
 *         return;
 *
 *     pSection->state = 2;
 *     pData = (u32*)(g_ArchiveStreamWritePointer + (i * CD_SECTOR_SIZE));
 *
 *     if (g_ArchiveImageChunksLeft == 0) {
 *         cmd = *pData++;
 *         if ((cmd != 0x1200) && (cmd != 0x1201))
 *             return;
 *
 *         pArgs = (u16*)pData;
 *         if (cmd == 0x1200) {
 *             switch (D_80059F24) {
 *                 case 1:
 *                     g_ArchiveImageRectX = D_80059F28 + pArgs[2];
 *                     g_ArchiveImageRectY = D_80059F2C + pArgs[3];
 *                     break;
 *                 case 2:
 *                     g_ArchiveImageRectX = D_80059F28 + pArgs[0] + pArgs[2];
 *                     g_ArchiveImageRectY = D_80059F2C + pArgs[1] + pArgs[3];
 *                     break;
 *                 default:
 *                     g_ArchiveImageRectX = pArgs[0] + pArgs[2];
 *                     g_ArchiveImageRectY = pArgs[1] + pArgs[3];
 *                     break;
 *             }
 *         }
 *
 *         if (cmd == 0x1201) {
 *             switch (D_80059F30) {
 *                 case 1:
 *                     g_ArchiveImageRectX = D_80059F34 + pArgs[2];
 *                     g_ArchiveImageRectY = D_80059F38 + pArgs[3];
 *                     break;
 *                 case 2:
 *                     g_ArchiveImageRectX = D_80059F34 + pArgs[0] + pArgs[2];
 *                     g_ArchiveImageRectY = D_80059F38 + pArgs[1] + pArgs[3];
 *                     break;
 *                 default:
 *                     g_ArchiveImageRectX = pArgs[0] + pArgs[2];
 *                     g_ArchiveImageRectY = pArgs[1] + pArgs[3];
 *                     break;
 *             }
 *         }
 *
 *         pData += 2;
 *         g_ArchiveImageRectW = *(u16*)pData;
 *         pData += 2;
 *         if (g_ArchiveImageBlocksLeft == 0)
 *             g_ArchiveImageBlocksLeft = *pData;
 *         pData++;
 *         g_ArchiveImageChunksLeft = *pData;
 *         pData++;
 *         g_ArchiveImageChunkHeights = (u16*)pData;
 *     } else {
 *         rect.x = g_ArchiveImageRectX;
 *         rect.y = g_ArchiveImageRectY;
 *         rect.w = g_ArchiveImageRectW;
 *         rect.h = *g_ArchiveImageChunkHeights;
 *         LoadImage(&rect, (u_long*)pData);
 *         DrawSync(0);
 *
 *         g_ArchiveImageRectY = g_ArchiveImageRectY + *g_ArchiveImageChunkHeights;
 *         g_ArchiveImageChunkHeights += 1;
 *         g_ArchiveImageChunksLeft -= 1;
 *         if (g_ArchiveImageChunksLeft <= 0) {
 *             g_ArchiveImageChunksLeft = 0;
 *             g_ArchiveImageBlocksLeft -= 1;
 *             for (i = 0; i < g_ArchiveStreamSectionCount; i++) {
 *                 g_ArchiveStreamSections[i].state = 0;
 *                 g_ArchiveStreamSections[i].id = 0;
 *             }
 *             if (g_ArchiveImageBlocksLeft <= 0) {
 *                 g_ArchiveCurFileSize = 0;
 *                 g_ArchivePendingReadCount = 0;
 *                 return;
 *             }
 *         }
 *         pSection->state = 0;
 *     }
 *
 *     g_ArchiveStreamCurSectionId += 1;
 * }
 */

void ArchivePrintDebugState(void) {
    func_800379C8(g_StrArchiveDebugStreamState, g_ArchiveTable, g_ArchiveStreamWritePointer, g_ArchiveStreamSections);
    func_800379C8(g_StrArchiveDebugCounters, g_ArchiveStreamSectionCount, g_ArchiveStreamQueueIndex, g_ArchiveImageBlocksLeft, g_ArchiveImageChunksLeft);
    func_800379C8(g_StrArchiveDebugImageRect, g_ArchiveImageRectX, g_ArchiveImageRectY, g_ArchiveImageRectW, g_ArchiveImageChunkHeights);
    func_800379C8(g_StrArchiveDebugChunkHeights, g_ArchiveImageChunkHeights[0], g_ArchiveImageChunkHeights[1], g_ArchiveImageChunkHeights[2]);
}

u32 ArchiveGetDebugTable(void) {
    return g_ArchiveDebugTable;
}
