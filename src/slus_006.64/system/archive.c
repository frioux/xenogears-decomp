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
extern s16 g_ArchiveStreamReadSectionId;
extern u16 g_ArchiveStreamNextSectionId;
extern u16 g_ArchiveStreamCurSectionId;
extern void* g_ArchiveCurStreamFile; // Current stream file struct
extern s32 D_80059F00;
extern u32 g_ArchiveCurEntryIndex;
extern s16 g_ArchiveDebugStreamSectionCount;
extern s32 g_ArchiveReadRetryCount;
extern void ArchiveCdReadyReadFile(u_char status);
extern void func_8002B2F0(u_char status);
extern void func_8002B5D0(u_char status);
extern void ArchiveStreamLoadSection(s32 unused, s32 nQueueIndex);
extern void ArchiveImageStreamSectorCallback();
extern void ArchiveImageStreamProcessSector();
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
extern int g_ArchiveStreamFileHandle;
extern CdlLOC g_ArchiveSectorLocation;
extern u8 g_ArchiveSectorScratch[];
extern s32 g_ArchiveFilePositionErrorCount;
extern s32 g_ArchiveStreamSectionPositionErrors;
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
            g_ArchiveStreamReadSectionId = 0;
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
            g_ArchiveDebugStreamSectionCount = 0;
            g_ArchiveStreamNextSectionId = 0;
            g_ArchiveStreamCurSectionId = 0;
            g_ArchiveStreamReadSectionId = 0;
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

int ArchiveStartStreamQueue(StreamDataQueueEntry* pQueue, s32 channel) {
    StreamDataQueueEntry* pEntry;
    StreamDataQueueEntry* pCurEntry;
    s32* pWord;
    char* pFilePath;
    void* pTempData;
    void* pData;
    CdlLOC* pLoc;
    s32 hFile;
    s32 nCount;
    s32 i;
    s32 j;
    s32 nMinIndex;
    u32 nIndex;
    u16 nEntryIndex;

    if (pQueue == NULL) {
        return -3;
    }

    nCount = 0;
    if (pQueue->archiveIndex != 0) {
        pEntry = pQueue;
        do {
            pEntry++;
            nCount++;
        } while (pEntry->archiveIndex != 0);
    }

    if (nCount == 0) {
        return -3;
    }

    for (i = 0; i < nCount - 1; i++) {
        nMinIndex = i;
        nEntryIndex = pQueue[i].archiveIndex;
        for (j = i + 1; j < nCount; j++) {
            if (pQueue[j].archiveIndex < nEntryIndex) {
                nMinIndex = j;
                /* volatile: the original reloads the field instead of reusing the tested value */
                nEntryIndex = *(volatile u16*)&pQueue[nMinIndex].archiveIndex;
            }
        }
        nEntryIndex = pQueue[i].archiveIndex;
        pTempData = pQueue[i].pData;
        pQueue[i].archiveIndex = pQueue[nMinIndex].archiveIndex;
        pQueue[i].pData = pQueue[nMinIndex].pData;
        pQueue[nMinIndex].archiveIndex = nEntryIndex;
        pQueue[nMinIndex].pData = pTempData;
    }

    ArchiveCdDataSync(0);
    g_ArchiveHeaderOffsetBase = g_CurArchiveOffset;
    for (i = 2, pWord = &D_80059F00; i >= 0; i--) {
        *pWord-- = 0;
    }

    pData = pQueue->pData;
    nIndex = pQueue->archiveIndex;
    g_ArchiveStreamQueueIndex = 0;
    g_ArchiveStreamQueue = pQueue;
    g_ArchivePendingReadCount = nCount;
    g_ArchiveStreamQueueCount = nCount;
    g_ArchiveStreamWritePointer = pData;

    if (nIndex == 0 || pData == NULL) {
        ArchiveCdSeekToFile(channel);
        g_ArchiveCurFileSize = 0;
        g_ArchivePendingReadCount = 0;
        return 0;
    }

    g_ArchiveCurEntryIndex = nIndex;
    g_ArchiveCurFileSector = ArchiveDecodeSector(nIndex);
    g_ArchiveCurFileSize = ArchiveDecodeSizeAligned(nIndex);
    g_ArchiveCdChannel = channel & 0xFFFF;
    g_ArchiveStreamResumedFile = 0;
    g_ArchiveStreamStopFlag = 0;
    g_ArchiveReadRetryCount = 0;
    pLoc = &g_ArchiveCdCurLocation;
    CdIntToPos(g_ArchiveCurFileSector, pLoc);

    if (g_ArchiveDebugTable != 0) {
        for (i = 0; i < nCount; i++) {
            nEntryIndex = pQueue[i].archiveIndex;
            g_ArchiveCurEntryIndex = nEntryIndex;
            pFilePath = ArchiveGetFilePath(nEntryIndex);

            /* the gotos keep the PCread block out of line and the exhaustion edge on CdClose */
            for (j = 0; j < RETRY_COUNT; j++) {
                hFile = PCopen(pFilePath, 0, 0);
                if (hFile != -1)
                    goto opened;
                GfxDrawArchiveReadRetry(j, 0xFF, 0, 0);
            }
            goto close;

        opened:
            if (pQueue[i].pData != NULL) {
                pCurEntry = &pQueue[i];
                for (j = 0; j < RETRY_COUNT; j++) {
                    if (PCread(hFile, pCurEntry->pData, ArchiveDecodeSizeAligned(nEntryIndex)) != 0)
                        break;
                    GfxDrawArchiveReadRetry(j, 0, 0xFF, 0);
                }
            }

        close:
            for (j = 0; j < RETRY_COUNT; j++) {
                if (PCclose(hFile) == 0)
                    break;
                GfxDrawArchiveReadRetry(j, 0, 0, 0xFF);
            }
        }

        g_ArchiveCurFileSize = 0;
        g_ArchivePendingReadCount = 0;
        return 0;
    }

    g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
    CdDataCallback(&ArchiveRestorePendingCount);
    CdSyncCallback(&ArchiveCdDriveCommandHandler);
    CdReadyCallback(&ArchiveCdReadyReadQueuedFile);
    g_ArchiveCdReadStartCount += 1;
    CdControlF(CdlSetloc, (void*)pLoc);
    return 0;
}


INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/archive", func_80029EB0);
/*
 * Best attempt: objdiff 93.58%, 940 bytes against retail's 944 (one instruction
 * SHORT, so it cannot link as written). Structure, frame (0x50, vars=24) and
 * the store block are right; what is left is which three of the six u16 stack
 * arguments take the frame slots (retail spills arg7/arg8/arg9 to 0x10/0x18/0x20
 * and keeps arg4/arg5/arg6 in $s6/$s7/$fp; this draft spills arg4's neighbours),
 * plus retail's dedicated callee-saved register for the constant 1
 * (`ori s5,zero,0x1` at the head of the store block, stored to
 * g_ArchivePendingReadCount and g_ArchiveCdDriveState) which this cc1 always
 * coalesces into a dead one ($17) instead of burning $s5 - that coalescing is
 * what leaves one register too many for the arguments.
 * Two levers that did move it (both kept below): a second copy of `index`
 * (nIndex, used at one call) buys the third frame slot, and wrapping the store
 * group from g_ArchiveStreamSections to ArchiveClearStreamFileSections in a
 * do/while(0) fixes the store order (the plain block emits D_80059F2C two
 * statements early). `if (nClose != (nClose = 0))` is what stops the PCclose
 * loop's s16 value being sign-extended twice.
 * Permuter: 3 runs, 2587 -> 1370 -> 1249 -> 1214 (still falling).
 * The six u16 stack args are the image chunk descriptors
 * ArchiveImageStreamProcessSector consumes (D_80059F24..D_80059F38).
 *
 * int func_80029EB0(int index, u32* pStreamFile, int channel, int unused, u16 arg4, u16 arg5, u16 arg6, u16 arg7, u16 arg8, u16 arg9) {
 *      s16 i;
 *      s16 nClose;
 *      char* pFilePath;
 *      u32 nSections;
 *      s32 nIndex;
 *      CdlLOC* pLoc;
 *
 *      if (pStreamFile == NULL) {
 *          return -4;
 *      }
 *
 *      nSections = pStreamFile[0];
 *
 *      if (nSections < 2) {
 *          return -4;
 *      }
 *
 *      if (index <= 0) {
 *          return -3;
 *      }
 *
 *      if (ArchiveDecodeSize(index) <= 0) {
 *          return -3;
 *      }
 *
 *      ArchiveCdDataSync(0);
 *      g_ArchiveHeaderOffsetBase = g_CurArchiveOffset;
 *      nIndex = index;
 *      for (i = 0; i < 3; i++) {
 *          ((s32*)&g_ArchiveSectorLocation)[i] = 0;
 *      }
 *
 *      ArchiveChangeStreamingFile(pStreamFile);
 *      g_ArchiveCurEntryIndex = index;
 *      g_ArchiveCurFileSector = ArchiveDecodeSector(nIndex);
 *      g_ArchiveCurFileSize = ArchiveDecodeAlignedSize(index);
 *
 *      g_ArchiveStreamWritePointer = (s8*)((u8*)pStreamFile + (nSections * sizeof(ArchiveStreamFileSectionHeader)) + STREAM_FILE_HEADER_SIZE);
 *      D_80059F30 = arg7;
 *      do {
 *      g_ArchiveStreamSections = (ArchiveStreamFileSectionHeader*)(pStreamFile + 1);
 *      D_80059F34 = arg8;
 *      g_ArchivePendingReadCount = 1;
 *      g_ArchiveCdChannel = channel & 0xFFFF;
 *      g_ArchiveStreamQueueIndex = 0;
 *      g_ArchiveStreamSectionCount = nSections;
 *      g_ArchiveStreamNextSectionId = 0;
 *      g_ArchiveStreamCurSectionId = 0;
 *      g_ArchiveStreamQueue = NULL;
 *      g_ArchiveStreamStopFlag = 0;
 *      g_ArchiveReadRetryCount = 0;
 *      D_80059F24 = arg4;
 *      D_80059F28 = arg5;
 *      D_80059F2C = arg6;
 *      g_ArchiveImageBlocksLeft = 0;
 *      g_ArchiveImageRectX = 0;
 *      g_ArchiveImageRectY = 0;
 *      g_ArchiveImageRectW = 0;
 *      g_ArchiveImageChunkHeights = NULL;
 *      g_ArchiveImageChunksLeft = 0;
 *      D_80059F38 = arg9;
 *      ArchiveClearStreamFileSections();
 *      } while (0);
 *
 *      pLoc = &g_ArchiveCdCurLocation;
 *      CdIntToPos(g_ArchiveCurFileSector, pLoc);
 *
 *      if (g_ArchiveDebugTable) {
 *          pFilePath = ArchiveGetFilePath(index);
 *
 *          for (i = 0; i < RETRY_COUNT; i++) {
 *              g_ArchiveStreamFileHandle = PCopen(pFilePath, 0, 0);
 *              if (g_ArchiveStreamFileHandle == -1)
 *                  GfxDrawArchiveReadRetry(i, 0xFF, 0, 0);
 *              else
 *                  break;
 *          }
 *
 *          do {
 *              ArchiveStreamLoadSection(0, 0);
 *              ArchiveImageStreamProcessSector(0, 0);
 *          } while (g_ArchivePendingReadCount > 0);
 *
 *          while (nClose = PCclose(g_ArchiveStreamFileHandle)) {
 *              GfxDrawArchiveReadRetry(nClose, 0, 0, 0xFF);
 *              nClose = nClose + 1;
 *              if (nClose >= RETRY_COUNT)
 *                  break;
 *          }
 *
 *          if (nClose != (nClose = 0)) {
 *              return -6;
 *          }
 *
 *          g_ArchivePendingReadCount = 0;
 *          g_ArchiveCurFileSize = 0;
 *          return 0;
 *      }
 *
 *      g_ArchiveCdDriveState = ARCHIVE_CD_DRIVE_READ_SECTOR;
 *      CdDataCallback(&ArchiveImageStreamSectorCallback);
 *      CdSyncCallback(&ArchiveCdDriveCommandHandler);
 *      CdReadyCallback(&func_8002B5D0);
 *      g_ArchiveCdReadStartCount += 1;
 *      CdControlF(CdlSetloc, (void*)pLoc);
 *      return 0;
 *  }
 */

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

/* Parked (round 403): objdiff 97.47%, 736 bytes - retail's EXACT size, 184 vs 184
 * insns, same instruction multiset, 8 differing lines in 3 rows. func_8002B5D0 is
 * instruction-identical (twin_for 1.000), so this body parks both; its note below
 * is the older 96.81 attempt.
 *
 * The three rows (all inside the scan loop head and the found-block):
 *  1. scan loop head: ours `addiu v1,v0,1` then `move s2,v0`, retail `move s2,v0`
 *     then `addiu v1,s2,1` (local_alloc coalesced the add with the load temp).
 *  2. the beq after CdPosToInt: retail fills the slot with `li v0,0x1` (the state
 *     store's value), ours with `li a1,0x200` (the CdGetSector size argument).
 *     This is levers.md c-a-call-argument-constant-beats: reorg takes the FIRST
 *     scheduled insn of the target block and sched2 ranks the jal's arg constant
 *     ahead of the store-only constant, so no source order reaches it.
 *  3. consequence of 2: `sll a0,s2,0xb` lands after the write-pointer `lw`
 *     instead of in the slot after `lhu a2`.
 *
 * Measured this round (all 736 bytes unless stated): two-statement queue index
 * (`entryIndex = g_ArchiveStreamQueueIndex; g_ArchiveStreamQueueIndex =
 * entryIndex + 1;`) scores 97.93 but is 732 bytes / 183 insns - the copy
 * disappears, so it cannot link; first id read plain + second volatile 96.48;
 * `pCurrent = NULL` initialiser 92.50 / 740 bytes / 185 insns (the duplicated
 * tail's cross-jump no longer happens). Reading the write pointer through a
 * `volatile u32*` local measures the same 97.47 as the inline `s8* volatile*`
 * cast and is the spelling parked here.
 *
 * permute.sh still cannot run this body: the permuter's parser reports
 * 'conversion to non-scalar type requested' on the write-pointer assignment line
 * for every spelling of it tried (`s8* volatile*` inline, `s8* volatile*` local,
 * `volatile u32*` local), so the body is hand-only until that is fixed. The lever
 * above says row 2 is unreachable anyway; park it, do not re-hunt.
 *
 * void func_8002B2F0(u_char status) {
 *     int entryIndex;
 *     ArchiveStreamFileSectionHeader* pCurrent;
 *     volatile u32* pWritePointer;
 *     CdlLOC* pLoc;
 *     u8* pBuf;
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
 *             pCurrent->id = *(volatile u16*)&g_ArchiveStreamNextSectionId;
 *             g_ArchiveStreamNextSectionId = *(volatile u16*)&g_ArchiveStreamNextSectionId + 1;
 *             pWritePointer = (volatile u32*)&g_ArchiveStreamWritePointer;
 *             CdGetSector((u8*)(entryIndex * CD_SECTOR_SIZE + (mem_addr)*pWritePointer), CD_SECTOR_SIZE / 4);
 *             g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *             g_ArchiveCurFileSector += 1;
 *             if (g_ArchiveCurFileSize > 0)
 *                 return;
 *         } else {
 *             g_ArchiveStreamSectionPositionErrors += 1;
 *             pBuf = g_ArchiveSectorScratch;
 *             CdGetSector(pBuf, CD_SECTOR_SIZE / 4);
 *             goto error;
 *         }
 *     }
 *
 *     if (pCurrent) {
 *         CdReadyCallback(NULL);
 *         g_ArchiveCurFileSize = 0;
 *         return;
 *     } else {
 *         CdReadyCallback(NULL);
 *         g_ArchiveCurFileSize = 0;
 *         return;
 *     }
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
 *
 */

/* NOTE: the park below func_8002B5D0 is the same body taken to 97.47% (retail's
 * exact 736 bytes) - read that note first, it supersedes the measurements here.
 *
 * Best attempt: objdiff 96.81% (this was 96.40% before). func_8002B5D0 is
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
 *             g_ArchiveStreamSectionPositionErrors += 1;
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

/* Parked: objdiff 97.47%, 736 bytes (retail's exact size), instruction multiset
 * identical, 184 vs 184 insns. Three delta rows: one in the scan loop and two in
 * the found-block after CdPosToInt, all of them sched2/local_alloc's choice.
 * func_8002B2F0 is instruction-identical to this function, so one body closes both.
 *
 *  1. The beq on the CdPosToInt result: retail puts `li v0,0x1` (the state value)
 *     in the delay slot, we put `li a1,0x200` (the CdGetSector size). The then-block
 *     is the branch target's only predecessor, so reorg fills eagerly from its first
 *     scheduled insn - and ours is the call-argument constant, because it feeds the
 *     jal which sits on the block's critical path while the state constant only feeds
 *     a store. rank_for_schedule compares priority first, so source order does not
 *     reach this one: the dependency graph has to change.
 *  2. Consequence: `sll a0,s2,0xb` (entryIndex * CD_SECTOR_SIZE) lands in the
 *     `lw g_ArchiveStreamWritePointer` load slot instead of in `lhu a2`'s.
 *  3. Loop head: retail `move s2,v0` then `addiu v1,s2,1`, ours `addiu v1,v0,1`
 *     then the copy (local_alloc coalesced the add with the load temp).
 *
 * Measured this round, all at 736 bytes unless stated: volatile state store
 * (`*(volatile u16*)&pCurrent->state = 1;`, the spelling ArchiveStreamLoadSection
 * needs) 96.67; a `s8* volatile* pWritePointer` local holding the write pointer's
 * address 97.47, same rows, and it does NOT make the body permuter-parsable; a u16
 * id local between the two counter reads 97.47, same rows.
 * Earlier rounds: `entryIndex = g_ArchiveStreamQueueIndex++;` is what makes the copy
 * to $s2 exist (two statements: 97.47); both id reads must be volatile or cse merges
 * them (both plain 96.40 / 732 bytes); the write pointer read must be volatile
 * (plain 96.81 / 740 bytes - sched2 hoists the load above the state store and maspsx
 * then inserts a load-delay nop); `pCurrent = &g_ArchiveStreamSections[entryIndex]`
 * 97.00 (base-first addu); `pCurrent = NULL;` before the scan 93.83; sector-offset
 * and destination-pointer locals 96.48-96.67 (gcc folds them back); gcc-2.7.2-cdk
 * 95.67, -O3 82.52, gcc-2.6.0 -O2 -G8 identical 97.47.
 *
 * permute.sh cannot run this body: the permuter's C parser rejects it ('conversion
 * to non-scalar type requested' on the preprocessed body), so a re-spelled write
 * pointer / id read is a prerequisite for any permuter attempt.
 *
 * The `if (pCurrent) {...} else {...}` tail duplicates the cleanup arm and reads
 * pCurrent uninitialised when the scan loop never runs - retail's asm does the same
 * (it loads 0($s1) with $s1 never set at .L8002B6C4), so the original source had
 * that bug too. This shape is worth 93.83 -> 97.47 through register allocation
 * alone; prefer an initialised spelling if one ever measures the same.
 *
 * void func_8002B5D0(u_char status) {
 *     int entryIndex;
 *     ArchiveStreamFileSectionHeader* pCurrent;
 *     CdlLOC* pLoc;
 *     u8* pBuf;
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
 *             pCurrent->id = *(volatile u16*)&g_ArchiveStreamNextSectionId;
 *             g_ArchiveStreamNextSectionId = *(volatile u16*)&g_ArchiveStreamNextSectionId + 1;
 *             CdGetSector((u8*)(entryIndex * CD_SECTOR_SIZE + (mem_addr)*(s8* volatile*)&g_ArchiveStreamWritePointer), CD_SECTOR_SIZE / 4);
 *             g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
 *             g_ArchiveCurFileSector += 1;
 *             if (g_ArchiveCurFileSize > 0)
 *                 return;
 *         } else {
 *             g_ArchiveStreamSectionPositionErrors += 1;
 *             pBuf = g_ArchiveSectorScratch;
 *             CdGetSector(pBuf, CD_SECTOR_SIZE / 4);
 *             goto error;
 *         }
 *     }
 *
 *     if (pCurrent) {
 *         CdReadyCallback(NULL);
 *         g_ArchiveCurFileSize = 0;
 *         return;
 *     } else {
 *         CdReadyCallback(NULL);
 *         g_ArchiveCurFileSize = 0;
 *         return;
 *     }
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


// param0 is unused by the body. The scan's index variable IS the second parameter:
// retail keeps its value live into the loop in $a1, which a local born inside the
// loop cannot reproduce (it folds into the load temp), and an initialised local
// costs the `li` retail does not have.
void ArchiveStreamLoadSection(s32 unused, s32 nQueueIndex) {
    s16 i;
    s32 nSectorOffset;
    u16 nSectionId;
    u16 nNextSectionId;
    ArchiveStreamFileSectionHeader* pHeader;

    if (g_ArchiveCurFileSize > 0) {
        for (i = 0; i < g_ArchiveStreamSectionCount; i++) {
            nQueueIndex = g_ArchiveStreamQueueIndex++;
            pHeader = (ArchiveStreamFileSectionHeader*)(nQueueIndex * sizeof(ArchiveStreamFileSectionHeader) + (mem_addr)g_ArchiveStreamSections);
            if (g_ArchiveStreamQueueIndex >= g_ArchiveStreamSectionCount)
                g_ArchiveStreamQueueIndex = 0;
            if (pHeader->state == ARCHIVE_STREAM_FILE_NOT_LOADED)
                break;
        }

        if (pHeader->state == ARCHIVE_STREAM_FILE_NOT_LOADED) {
            i = 0;
            // volatile: retail stores the state before it reloads the id counter
            *(volatile u16*)&pHeader->state = 1;
            nSectionId = g_ArchiveStreamNextSectionId;
            nNextSectionId = nSectionId + 1;
            pHeader->id = nSectionId;
            g_ArchiveStreamNextSectionId = nNextSectionId;
            nSectorOffset = nQueueIndex * CD_SECTOR_SIZE;
            for (; i < RETRY_COUNT; i++) {
                if (PCread(g_ArchiveStreamFileHandle, (u8*)(nSectorOffset + (mem_addr)g_ArchiveStreamWritePointer), CD_SECTOR_SIZE) != 0)
                    break;
                GfxDrawArchiveReadRetry(i, 0, 0xFF, 0);
            }
            g_ArchiveCurFileSize -= CD_SECTOR_SIZE;
            g_ArchiveCurFileSector += 1;
        } else {
            g_ArchiveCurFileSize = 0;
        }
    }

    if (g_ArchiveCurFileSize <= 0)
        g_ArchiveCurFileSize = 0;
}
/* The queue-index copy in this body (round 334 posed it as a live-range question;
 * round 335 closed it). Retail's loop head is
 *     lw v0,q / addu a1,v0,zero / addiu v1,a1,1 / sll v0,a1,3 / sw v1,q
 * The copy only comes out FIRST when the index variable is already live at the load.
 * A local born inside the loop is coalesced into the load temp, and the copy then
 * lands after the addiu with the add reading the temp (that is the 400-byte
 * measurement with a wrong order); an initialised local gives retail's loop byte
 * for byte but costs a `li a1,-1` retail has no room for (404 bytes). The live-in
 * has to come from an instruction that already exists, and it does: the function's
 * SECOND PARAMETER, which the caller (func_80029EB0) clears in the jal delay slot -
 * so the scan's index variable IS that parameter and its initial value is free.
 * 400 bytes / 100.00%; param0 is unused, which is why the caller passes two zeros.
 * `*(volatile u16*)&pHeader->state = 1;` is load-bearing (93.90% without it): retail
 * stores the state before reloading the id counter and the volatile blocks the CSE
 * that would otherwise sink the store.
 * func_8002B2F0 / func_8002B5D0 hold the same loop, but there the copy goes to $s2
 * (the index lives across the CdGetSector call, so it is callee-saved and cannot be
 * coalesced anyway) - the live-in question does not arise, and their residual is the
 * sched2 delay-slot tie documented above func_8002B5D0.
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

void ArchiveImageStreamSectorCallback(void) {
    ArchiveStreamFileSectionHeader* pSection;
    ArchiveStreamFileSectionHeader* pIter;
    ArchiveStreamFileSectionHeader* pSections;
    u_long* pBuf;
    RECT rect;
    u16* pData;
    u16* pChunk;
    u32 nCommand;
    u32 nChunks;
    s32 sectorOffset;
    s32 nCoord;
    s16 i;

    // Two variables for one address: gcc's allocator only gives the scan loop retail's registers this way.
    pSections = g_ArchiveStreamSections;
    pSection = pSections;
    for (i = 0; i < g_ArchiveStreamSectionCount; i++, pSection++) {
        if (pSection->state == 1 && pSection->id == g_ArchiveStreamCurSectionId)
            break;
    }

    if (i == g_ArchiveStreamSectionCount)
        return;

    pSection->state = 2;
    sectorOffset = i * CD_SECTOR_SIZE;
    pBuf = (u_long*)(sectorOffset + (mem_addr)g_ArchiveStreamWritePointer);

    if (g_ArchiveImageChunksLeft == 0) {
        nCommand = *pBuf++;
        pData = (u16*)pBuf;
        if (nCommand < 0x1200 || nCommand > 0x1201) {
            g_ArchiveCurFileSize = 0;
            CdDataCallback(NULL);
            ArchiveCdSeekToFile(g_ArchiveCdChannel);
            g_ArchivePendingReadCount = 0;
            return;
        }

        if (nCommand == 0x1200) {
            switch (D_80059F24) {
            case 1:
                g_ArchiveImageRectX = D_80059F28 + pData[2];
                g_ArchiveImageRectY = D_80059F2C + pData[3];
                break;
            case 2:
                nCoord = D_80059F28 + pData[0] + pData[2];
                g_ArchiveImageRectX = nCoord;
                nCoord = D_80059F2C + pData[1] + pData[3];
                g_ArchiveImageRectY = nCoord;
                break;
            default:
                g_ArchiveImageRectX = pData[0] + pData[2];
                g_ArchiveImageRectY = pData[1] + pData[3];
                break;
            }
        }

        if (nCommand == 0x1201) {
            switch (D_80059F30) {
            case 1:
                g_ArchiveImageRectX = D_80059F34 + pData[2];
                g_ArchiveImageRectY = D_80059F38 + pData[3];
                break;
            case 2:
                nCoord = D_80059F34 + pData[0] + pData[2];
                g_ArchiveImageRectX = nCoord;
                nCoord = D_80059F38 + pData[1] + pData[3];
                g_ArchiveImageRectY = nCoord;
                break;
            default:
                g_ArchiveImageRectX = pData[0] + pData[2];
                g_ArchiveImageRectY = pData[1] + pData[3];
                break;
            }
        }

        pBuf += 2;
        g_ArchiveImageRectW = *(u16*)pBuf;
        pBuf += 2;
        if (g_ArchiveImageBlocksLeft == 0)
            g_ArchiveImageBlocksLeft = *pBuf;
        pBuf += 1;
        nChunks = *pBuf;
        pBuf += 1;
        g_ArchiveImageChunkHeights = (u16*)pBuf;
        g_ArchiveImageChunksLeft = nChunks;
    } else {
        rect.x = g_ArchiveImageRectX;
        rect.y = g_ArchiveImageRectY;
        rect.w = g_ArchiveImageRectW;
        rect.h = g_ArchiveImageChunkHeights[0];
        LoadImage(&rect, pBuf);

        pChunk = g_ArchiveImageChunkHeights;
        g_ArchiveImageChunkHeights = pChunk + 1;
        g_ArchiveImageRectY += pChunk[0];
        g_ArchiveImageChunksLeft -= 1;

        if (g_ArchiveImageChunksLeft <= 0) {
            g_ArchiveImageChunksLeft = 0;
            g_ArchiveImageBlocksLeft -= 1;
            for (i = 0; i < g_ArchiveStreamSectionCount; i++) {
                pIter = g_ArchiveStreamSections;
                pIter[i].state = 0;
                pIter[i].id = 0;
            }

            if (g_ArchiveImageBlocksLeft <= 0) {
                g_ArchiveCurFileSize = 0;
                CdDataCallback(NULL);
                ArchiveCdSeekToFile(g_ArchiveCdChannel);
                g_ArchivePendingReadCount = 0;
                return;
            }
        }

        pSection->state = 0;
    }

    g_ArchiveStreamCurSectionId += 1;
}

void ArchiveImageStreamProcessSector(void) {
    ArchiveStreamFileSectionHeader* pSection;
    ArchiveStreamFileSectionHeader* pIter;
    ArchiveStreamFileSectionHeader* pSections;
    u_long* pBuf;
    RECT rect;
    u16* pData;
    u16* pChunk;
    u32 nCommand;
    u32 nChunks;
    s32 sectorOffset;
    s32 nCoord;
    s16 i;

    // Two variables for one address: gcc's allocator only gives the scan loop retail's registers this way.
    pSections = g_ArchiveStreamSections;
    pSection = pSections;
    for (i = 0; i < g_ArchiveStreamSectionCount; i++, pSection++) {
        if (pSection->state == 1 && pSection->id == g_ArchiveStreamCurSectionId)
            break;
    }

    if (i == g_ArchiveStreamSectionCount)
        return;

    pSection->state = 2;
    sectorOffset = i * CD_SECTOR_SIZE;
    pBuf = (u_long*)(sectorOffset + (mem_addr)g_ArchiveStreamWritePointer);

    if (g_ArchiveImageChunksLeft == 0) {
        nCommand = *pBuf++;
        pData = (u16*)pBuf;
        if (nCommand < 0x1200 || nCommand > 0x1201)
            return;

        if (nCommand == 0x1200) {
            switch (D_80059F24) {
            case 1:
                g_ArchiveImageRectX = D_80059F28 + pData[2];
                g_ArchiveImageRectY = D_80059F2C + pData[3];
                break;
            case 2:
                nCoord = D_80059F28 + pData[0] + pData[2];
                g_ArchiveImageRectX = nCoord;
                nCoord = D_80059F2C + pData[1] + pData[3];
                g_ArchiveImageRectY = nCoord;
                break;
            default:
                g_ArchiveImageRectX = pData[0] + pData[2];
                g_ArchiveImageRectY = pData[1] + pData[3];
                break;
            }
        }

        if (nCommand == 0x1201) {
            switch (D_80059F30) {
            case 1:
                g_ArchiveImageRectX = D_80059F34 + pData[2];
                g_ArchiveImageRectY = D_80059F38 + pData[3];
                break;
            case 2:
                nCoord = D_80059F34 + pData[0] + pData[2];
                g_ArchiveImageRectX = nCoord;
                nCoord = D_80059F38 + pData[1] + pData[3];
                g_ArchiveImageRectY = nCoord;
                break;
            default:
                g_ArchiveImageRectX = pData[0] + pData[2];
                g_ArchiveImageRectY = pData[1] + pData[3];
                break;
            }
        }

        pBuf += 2;
        g_ArchiveImageRectW = *(u16*)pBuf;
        pBuf += 2;
        if (g_ArchiveImageBlocksLeft == 0)
            g_ArchiveImageBlocksLeft = *pBuf;
        pBuf += 1;
        nChunks = *pBuf;
        pBuf += 1;
        g_ArchiveImageChunkHeights = (u16*)pBuf;
        g_ArchiveImageChunksLeft = nChunks;
    } else {
        rect.x = g_ArchiveImageRectX;
        rect.y = g_ArchiveImageRectY;
        rect.w = g_ArchiveImageRectW;
        rect.h = g_ArchiveImageChunkHeights[0];
        LoadImage(&rect, pBuf);
        DrawSync(0);

        pChunk = g_ArchiveImageChunkHeights;
        g_ArchiveImageChunkHeights = pChunk + 1;
        g_ArchiveImageRectY += pChunk[0];
        g_ArchiveImageChunksLeft -= 1;

        if (g_ArchiveImageChunksLeft <= 0) {
            g_ArchiveImageChunksLeft = 0;
            g_ArchiveImageBlocksLeft -= 1;
            for (i = 0; i < g_ArchiveStreamSectionCount; i++) {
                pIter = g_ArchiveStreamSections;
                pIter[i].state = 0;
                pIter[i].id = 0;
            }

            if (g_ArchiveImageBlocksLeft <= 0) {
                g_ArchiveCurFileSize = 0;
                g_ArchivePendingReadCount = 0;
                return;
            }
        }

        pSection->state = 0;
    }

    g_ArchiveStreamCurSectionId += 1;
}

void ArchivePrintDebugState(void) {
    func_800379C8(g_StrArchiveDebugStreamState, g_ArchiveTable, g_ArchiveStreamWritePointer, g_ArchiveStreamSections);
    func_800379C8(g_StrArchiveDebugCounters, g_ArchiveStreamSectionCount, g_ArchiveStreamQueueIndex, g_ArchiveImageBlocksLeft, g_ArchiveImageChunksLeft);
    func_800379C8(g_StrArchiveDebugImageRect, g_ArchiveImageRectX, g_ArchiveImageRectY, g_ArchiveImageRectW, g_ArchiveImageChunkHeights);
    func_800379C8(g_StrArchiveDebugChunkHeights, g_ArchiveImageChunkHeights[0], g_ArchiveImageChunkHeights[1], g_ArchiveImageChunkHeights[2]);
}

u32 ArchiveGetDebugTable(void) {
    return g_ArchiveDebugTable;
}
