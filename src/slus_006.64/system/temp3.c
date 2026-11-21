#include "common.h"
#include "main/game.h"
#include "system/menu.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/sound.h"
#include "psyq/libgpu.h"
#include "psyq/libcd.h"

extern s32 g_GameStagedFileIndex;
extern s32 g_GameStagedFileTag;
extern s32 g_GamePartySkinsInitialized;
extern s32 g_GamePartyUsesGearSkins;
extern s32 g_GameSceneMapNum;
extern s32 g_GamePartyQueuedSkinKind;
extern s32 g_SoundSedsFileStaged;
extern s32 g_GameUITextureFileStaged;
extern void *g_GameUITextureFileBuffer;
extern void *g_SoundSedsFileBuffer;
extern StreamDataQueueEntry g_PartyStreamDataQueue[4];
extern int g_GamePartyMemberSkins[3];
extern int g_PartyIsWaitingForStreamData;
extern void* g_PartyDataBuffers[];
extern void* g_PartyStreamDataPointers[];

extern u8* g_SystemCellGrid;
extern u8* g_SystemCellNeighborCounts;
extern void* g_SystemCellPrimBuffers;
extern void* D_800592E0;

void SystemInitializeCellGridBuffers(void) {
    int i;
    int j;

    g_SystemCellPrimBuffers = HeapAlloc(0x3480, 1);
    D_800592E0 = HeapAlloc(0x3480, 1);
    g_SystemCellGrid = HeapAlloc(0x460, 1);
    g_SystemCellNeighborCounts = HeapAlloc(0x460, 1);

    for (i = 0; i < 0x1C; i++) {
        for (j = 0; j < 0x28; j++) {
            g_SystemCellGrid[i * 0x28 + j] = 0;
            g_SystemCellNeighborCounts[i * 0x28 + j] = 0;
        }
    }
}

void SystemIncrementCellNeighborCount(s32 arg0, s32 arg1) {
    if (arg0 < 0) {
        arg0 = 0x1C;
    }
    if (arg0 > 0x1C) {
        arg0 = 0;
    }

    if (arg1 < 0) {
        arg1 = 0x28;
    }
    if (arg1 > 0x28) {
        arg1 = 0;
    }

    g_SystemCellNeighborCounts[arg0 * 0x28 + arg1] += 1;
}

extern int D_800592C8;

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3", func_8001A6E8);
/* Best attempt: 97.70%. Frame, callee-saved set, spill slots (gridOffset 0x20, rowOffset 0x18, pOt 0x10), count in
   fp and every instruction match; only the callee-saved numbering of three pseudos is
   rotated. The original allocates pField (the strength-reduced pPrim+8 induction pointer)
   s2, j s3, i s4; this C gets j s2, i s3, pField s4, so the ~200 differing words are all the
   same instruction with s2/s3/s4 renamed. Writing the three prim stores as offsets off
   pPrim (pPrim[3], pPrim+4, pPrim+8) is what reproduces the original single-IV shape -- an
   explicit pField = pPrim + 8 makes gcc split it into two IVs and spill count. gridOffset
   must be declared after rowOffset to get the 0x18/0x20 slots. The second loop reuses i
(both loops' counters are s4 in the original); with a separate k, k lands in a0 and the
   compared constant in a1 instead of the original a0. Refs-driven allocation order, so
   declaration order, explicit i-minus-one and i-plus-one locals, and a mask-style temp all
   failed to move it.

void func_8001A6E8(u_long* pOt) {
    s32 j;
    s32 i;
    s32 count;
    s32 rowOffset;
    s32 gridOffset;
    s32 x;
    s32 y;
    u8* pPrim;

    pPrim = (u8*)(&g_SystemCellPrimBuffers)[D_800592C8];
    count = 0;
    for (i = 0, gridOffset = 0; i < 0x1C; i++, gridOffset += 0x28) {
        rowOffset = gridOffset;
        for (j = 0; j < 0x28; j++) {
            if (g_SystemCellGrid[rowOffset + j] != 0) {
                *(u8*)(pPrim + 3) = 2;
                *(u32*)(pPrim + 4) = 0x70280000;
                *(u32*)(pPrim + 8) = (j << 3) | (i << 19);
                AddPrim(pOt, pPrim);
                pPrim += 0xC;
                count++;
                SystemIncrementCellNeighborCount(i - 1, j - 1);
                SystemIncrementCellNeighborCount(i - 1, j);
                SystemIncrementCellNeighborCount(i - 1, j + 1);
                SystemIncrementCellNeighborCount(i, j - 1);
                SystemIncrementCellNeighborCount(i, j + 1);
                SystemIncrementCellNeighborCount(i + 1, j - 1);
                SystemIncrementCellNeighborCount(i + 1, j);
                SystemIncrementCellNeighborCount(i + 1, j + 1);
            }
        }
    }

    for (i = 0; i < 0x460; i++) {
        if (g_SystemCellNeighborCounts[i] != 2) {
            g_SystemCellGrid[i] = g_SystemCellNeighborCounts[i] == 3;
        }
        g_SystemCellNeighborCounts[i] = 0;
    }

    if (count < 0x14) {
        count = 0;
        x = rand() % 0x1C;
        y = rand() % 0x28;
        do {
            x = x - 1 + rand() % 3;
            y = y - 1 + rand() % 3;
            if (x < 0) {
                x = 0x1C;
            }
            if (x > 0x1C) {
                x = 0;
            }
            if (y < 0) {
                y = 0x28;
            }
            if (y > 0x28) {
                y = 0;
            }
            g_SystemCellGrid[x * 0x28 + y] = 1;
            count++;
        } while (count < 0x14);
    }

    x = rand() % 0x1C;
    y = rand() % 0x28;
    SystemIncrementCellNeighborCount(x - 1, y - 1);
    SystemIncrementCellNeighborCount(x - 1, y);
    SystemIncrementCellNeighborCount(x - 1, y + 1);
    SystemIncrementCellNeighborCount(x, y - 1);
    SystemIncrementCellNeighborCount(x, y + 1);
    SystemIncrementCellNeighborCount(x + 1, y - 1);
    SystemIncrementCellNeighborCount(x + 1, y);
    SystemIncrementCellNeighborCount(x + 1, y + 1);
}
*/


extern s32 g_GameWdsFileLoadState;
extern s32 g_GameAudioAndWdsLoaded;
extern s32 g_GameLoadedWdsStreamId;
extern s32 g_GameLoadedWdsBankIndex;
extern s32 g_FieldScriptWalkFlags;
extern s32 g_GameRequestedWdsBankIndex;
extern s32 g_SoundPendingAudioManager;
extern s32 g_FieldScreenSetupDone;
extern s32 D_8004F314;
extern s32 g_GameStagedFileSyncCount;
extern s32 g_FieldSuppressStateChange;
extern s32 g_GameAudioManagerRunning;
extern s32 g_GameHasLoadedWDS;
extern s32 g_GameWdsBankHeaderReadPending;
extern s32 g_GameWdsStreamInProgress;
extern s32 g_FieldPendingRequestCount;
extern s32 D_8004F2F4;
extern s32 g_GameReusePendingAudioManager;
extern s32 g_WorldMapAudioManagerLoaded;
extern s32 g_GameWdsEntryFreed;
extern s32 g_FieldTextRenderEnabled;
extern s32 g_FieldDisableGearRender;
extern s32 D_8004F37C;
extern s32 g_FieldDisableCompassRender;
extern u8 g_FieldDiscChangeFade;
extern u8 g_GameDiscChangeRequest;
extern s16 g_GameWdsEntryReleased;
extern s32 g_FieldScriptWalkFrameCounter;
extern s32 g_GameAudioFadeInPending;
extern s32 g_GameWdsBankLoadState;
extern s32 g_FieldPartyActorIndices[3];
extern s32 g_FieldPartyActorSlots[3];
extern s32 D_80062524[1];

void SystemResetGlobalState(void) {
    int i;

    g_GameWdsFileLoadState = 1;
    g_FieldScriptWalkFlags = 0xFF;
    g_GameRequestedWdsBankIndex = 0xFF;
    g_SoundPendingAudioManager = 0;
    g_GameAudioAndWdsLoaded = 0;
    g_FieldScreenSetupDone = 0;
    g_GamePartyQueuedSkinKind = 0;
    g_GamePartyUsesGearSkins = 0;
    D_8004F314 = 0;
    g_GameStagedFileSyncCount = 0;
    g_GamePartySkinsInitialized = 0;
    g_FieldSuppressStateChange = 0;
    g_GameAudioManagerRunning = 0;
    g_GameHasLoadedWDS = 0;
    g_PartyIsWaitingForStreamData = 0;
    g_GameWdsBankHeaderReadPending = 0;
    g_GameWdsStreamInProgress = 0;
    g_FieldPendingRequestCount = 0;
    D_8004F2F4 = 0;
    g_GameUITextureFileStaged = 0;
    g_GameReusePendingAudioManager = 0;
    g_WorldMapAudioManagerLoaded = 0;
    g_GameWdsEntryFreed = 0;
    g_FieldTextRenderEnabled = 0;
    g_FieldDisableGearRender = 0;
    D_8004F37C = 0;
    g_FieldDisableCompassRender = 0;
    g_FieldDiscChangeFade = 0;
    g_GameDiscChangeRequest = 0;
    g_GameWdsEntryReleased = 0;
    g_FieldScriptWalkFrameCounter = 0;
    g_GameStagedFileTag = -1;
    g_GameSceneMapNum = -1;
    g_GameLoadedWdsStreamId = -1;
    g_GameLoadedWdsBankIndex = -1;
    g_GameStagedFileIndex = -1;
    g_SoundSedsFileStaged = -1;
    g_GameAudioFadeInPending = -1;
    g_GameWdsBankLoadState = -1;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        g_GamePartyMemberSkins[i] = 0;
        g_FieldPartyActorIndices[i] = 0;
        g_FieldPartyActorSlots[i] = 0;
        g_GamePartyMembers[i] = 0;
    }

    for (i = 3; i >= 0; i--) {
        D_80062524[i - 3] = 0;
    }
}

void GamePartySignalReinitialize(void) {
    g_GamePartySkinsInitialized = 0;
}

void GamePartyReloadSkinsFromFieldArchive(void) {
    g_GameStagedFileTag = -1;
    g_GameStagedFileIndex = -1;
    HeapChangeCurrentUser(HEAP_USER_YOSI, 0);
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    GamePartyStreamLoadSkinData(1);
}

int GameCharacterGetGearID(int characterIndex) {
    return g_pGameState->characters[characterIndex].gearId;
}

void GameWaitForCdData(void) {
    while (ArchiveDataSync()) {};
    ArchiveCdDataSync(0);
}

// Set GameState pointer to g_GameState, set party members accordingly 
// to it and stream load character skins for them.
void GamePartyCharactersInitializeSkins(void) {
    int i;
    int curPartyMemberIndex;
    int entryCount;
    void* pBuffer;

    g_pGameState = &g_GameState;
    curPartyMemberIndex = 0;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        g_GamePartyMembers[i] = CHARACTER_ID_NONE;
        if (g_pGameState->partyMembers[i] != CHARACTER_ID_NONE) {
            g_GamePartyMembers[curPartyMemberIndex++] = g_pGameState->partyMembers[i];
        }
    }
    
    for (i = 0, entryCount = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_GamePartyMembers[i] != CHARACTER_ID_NONE) {
            g_PartyStreamDataQueue[entryCount].archiveIndex = g_GamePartyMembers[i] + 5;
            g_GamePartyMemberSkins[i] = g_GamePartyMembers[i];
            pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_GamePartyMembers[i] + 5), 0x0);
            g_PartyStreamDataPointers[entryCount] = pBuffer;
            g_PartyStreamDataQueue[entryCount].pData = pBuffer;
            HeapPinBlock(g_PartyStreamDataPointers[entryCount]);
            entryCount++;
        }
    }
    g_PartyStreamDataQueue[entryCount].pData = NULL;
    g_PartyStreamDataQueue[entryCount].archiveIndex = 0;
    ArchiveStartStreamQueue(g_PartyStreamDataQueue, 0, 0);
    g_GamePartyQueuedSkinKind = 1;
}

// Set GameState pointer to g_GameState, set party members accordingly 
// to it and stream load gear skins for them.
void GamePartyGearsInitializeSkins(void) {
    int i;
    int curPartyMemberIndex;
    int entryCount;
    int gearId;
    void* pBuffer;

    g_pGameState = &g_GameState;
    curPartyMemberIndex = 0;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        g_GamePartyMembers[i] = CHARACTER_ID_NONE;
        if (g_pGameState->partyMembers[i] != CHARACTER_ID_NONE) {
            g_GamePartyMembers[curPartyMemberIndex++] = g_pGameState->partyMembers[i];
        }
    }

    for (i = 0, entryCount = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_GamePartyMembers[i] != CHARACTER_ID_NONE) {
            gearId = GameCharacterGetGearID(g_GamePartyMembers[i]);
            if (gearId == GEAR_ID_NONE) {
                gearId = 0;
            }
            gearId += 0x10;
            g_PartyStreamDataQueue[entryCount].archiveIndex = gearId + 5;
            g_GamePartyMemberSkins[i] = gearId;
            pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(gearId + 5), 0);
            g_PartyStreamDataPointers[entryCount] = pBuffer;
            g_PartyStreamDataQueue[entryCount].pData = pBuffer;
            HeapPinBlock(g_PartyStreamDataPointers[entryCount]);
            entryCount++;
        }
    }
    g_PartyStreamDataQueue[entryCount].pData = NULL;
    g_PartyStreamDataQueue[entryCount].archiveIndex = 0;
    ArchiveStartStreamQueue(g_PartyStreamDataQueue, 0, 0);
    g_GamePartyQueuedSkinKind = 2;
}

void GamePartySyncSkinData(void) {
    GameWaitForCdData();
    if (g_PartyIsWaitingForStreamData == 1) {
        GamePartySyncStreamedData();
        if (g_GamePartySkinsInitialized) return;
    } else if (g_GamePartySkinsInitialized) {
        GamePartyStreamLoadSkinData(0);
        return;
    }

    if (!g_GamePartySkinsInitialized) {
        if (!(g_GameSceneMapNum & 0xC000)) g_GamePartyUsesGearSkins = 0;
        else g_GamePartyUsesGearSkins = 1;
        
        g_PartyIsWaitingForStreamData = 0;
        if (g_GamePartyUsesGearSkins == 0) {
            if (g_GamePartyQueuedSkinKind != 1) {
                GamePartyCharactersInitializeSkins();
                g_PartyIsWaitingForStreamData = 1;
            }
        } else if (g_GamePartyQueuedSkinKind != 2) {
            GamePartyGearsInitializeSkins();
            g_PartyIsWaitingForStreamData = 1;
        }
    }
}

void GamePartyStreamLoadSkinData(int arg0) {
    int i;
    int entryIndex;

    HeapToggleErrorHandler(1);

    if (g_PartyIsWaitingForStreamData == 1) {
        GamePartySyncStreamedData();
    }
    GameWaitForCdData();
    
    for (i = 0, entryIndex = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_GamePartyMemberSkins[i] == CHARACTER_ID_NONE) {
            continue;
        }

        g_PartyStreamDataQueue[entryIndex].archiveIndex = g_GamePartyMemberSkins[i] + 5;
        g_PartyStreamDataPointers[entryIndex] = HeapAlloc(ArchiveDecodeAlignedSize(g_GamePartyMemberSkins[i] + 5), 0x1);
        g_PartyStreamDataQueue[entryIndex].pData = g_PartyStreamDataPointers[entryIndex];
        if (g_PartyStreamDataQueue[entryIndex].pData == 0) {
            for (i = 0; i < entryIndex; i++) {
                HeapUnpinBlock(g_PartyStreamDataPointers[i]);
                HeapFree(g_PartyStreamDataPointers[i]);
            }
            
            HeapToggleErrorHandler(0);
            return;
        }

        HeapPinBlock(g_PartyStreamDataPointers[entryIndex]);
        entryIndex++;
    }
        
    if (arg0 != 0) {
        g_GameUITextureFileBuffer = HeapAlloc(ArchiveDecodeAlignedSize(0xA7), 0x1);
        g_PartyStreamDataQueue[entryIndex].pData = g_GameUITextureFileBuffer;
        if (g_PartyStreamDataQueue[entryIndex].pData != 0) {
            HeapPinBlock(g_PartyStreamDataQueue[entryIndex].pData);
            g_PartyStreamDataQueue[entryIndex].archiveIndex = 0xA7;
            entryIndex++;
            g_GameUITextureFileStaged = 1;
        }

        g_SoundSedsFileBuffer = HeapAlloc(ArchiveDecodeAlignedSize(0xA8), 0x1);
        g_PartyStreamDataQueue[entryIndex].pData = g_SoundSedsFileBuffer;
        if (g_PartyStreamDataQueue[entryIndex].pData != 0) {
            HeapPinBlock(g_PartyStreamDataQueue[entryIndex].pData);
            g_PartyStreamDataQueue[entryIndex].archiveIndex = 0xA8;
            entryIndex++;
            g_SoundSedsFileStaged = 0;
        }
    }

    g_PartyStreamDataQueue[entryIndex].pData = 0;
    g_PartyStreamDataQueue[entryIndex].archiveIndex = 0;
    ArchiveStartStreamQueue(g_PartyStreamDataQueue, 0, 0); // Start streaming the queued requests
    g_PartyIsWaitingForStreamData = 1;
    HeapToggleErrorHandler(0);
}

void GamePartySyncStreamedData(void) {
    int i;
    
    if (g_PartyIsWaitingForStreamData) {
        GameWaitForCdData();
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            HeapUnpinBlock(g_PartyDataBuffers[i]);
            if (g_GamePartyMembers[i] != CHARACTER_ID_NONE) {
                HeapUnpinBlock(g_PartyStreamDataPointers[i]);
                LZSSDecompress(g_PartyStreamDataPointers[i], g_PartyDataBuffers[i]);
                HeapFree(g_PartyStreamDataPointers[i]);
            }
        }
        g_PartyIsWaitingForStreamData = 0;
    }
}


extern int g_GameStagedFileSize; // File size
extern void* g_GameStagedFileBuffer; // File buffer

extern s32 g_GameStagedFileIndex;
extern s32 g_GameStagedFileTag;

int ArchiveLoadStagedFileIfChanged(int fileIndex, s32 arg1) {
    if (g_GameStagedFileTag == arg1 && g_GameStagedFileIndex == fileIndex) {
        return 0;
    }

    if (ArchiveDataSync() == 0) {
        ArchiveCdDataSync(0);
        if (g_GameStagedFileTag != -1) {
            HeapUnpinBlock(g_GameStagedFileBuffer);
            HeapFree(g_GameStagedFileBuffer);
        }
        ArchiveLoadFileToPinnedBuffer(fileIndex);
        g_GameStagedFileTag = arg1;
        g_GameStagedFileIndex = fileIndex;
    }
    
    return -1;
}

void ArchiveLoadFileToPinnedBuffer(int index) {
    g_GameStagedFileSize = ArchiveDecodeAlignedSize(index + 0xB8);
    g_GameStagedFileBuffer = HeapAlloc(g_GameStagedFileSize, 1);
    HeapPinBlock(g_GameStagedFileBuffer);
    ArchiveReadFileToBuffer(index + 0xB8, g_GameStagedFileBuffer, 0, CdlModeSpeed);
}

extern int g_GameHasLoadedWDS;
extern SoundWDSEntry* g_GameCurLoadedWDS;

void GameFreeLoadedWdsEntry(void) {
    if (g_GameHasLoadedWDS == 1) {
        SoundFreeWdsEntry(g_GameCurLoadedWDS);
        g_GameHasLoadedWDS = 0;
    }
}

extern s32 g_GameAudioManagerRunning;
extern s32 g_SoundAudioManager;
extern s32 g_GameReusePendingAudioManager;
extern s32 g_SoundPendingAudioManager;

extern void SoundStopAudioManager(s32);
extern void SoundDestroyAudioManager(s32);

void SoundStopAndReleaseAudioManager(void) {
    if (g_GameAudioManagerRunning == 1) {
        SoundStopAudioManager(g_SoundAudioManager);
        if (g_GameReusePendingAudioManager == 0) {
            SoundDestroyAudioManager(g_SoundAudioManager);
        } else {
            g_SoundPendingAudioManager = g_SoundAudioManager;
        }
        g_GameAudioManagerRunning = 0;
        g_GameReusePendingAudioManager = 0;
    }
}

extern s32 g_GameAudioAndWdsLoaded;
extern s32 g_GameLoadedWdsStreamId;
void SoundStopAndReleaseAudioManager(void);

void SoundFreeLoadedWdsAndAudioManager(void) {
    if (g_GameAudioAndWdsLoaded != 0) {
        SoundStopAndReleaseAudioManager();
        GameFreeLoadedWdsEntry();
    }
    g_GameLoadedWdsStreamId = -1;
    g_GameLoadedWdsBankIndex = -1;
    g_GameAudioAndWdsLoaded = 0;
}

void SystemNop(void) {}

extern u8 D_8005959C;
extern s32* g_DebuggerState;
extern u8 D_800C48EA;
extern u8 D_800D3338;
extern u8 g_GameResumeScene;
extern u8 g_GameSelectedSkinLoadPending;
extern u16 g_NextMapId;
extern s16 g_NextMapRotationY;
extern s16 D_8006F952;
extern s16 g_NextMapFlags;
extern void func_80070F40(void);
extern void ChangeGameState(unsigned int state);
extern void MainLoop(s32 arg0);
extern void* FontLoadFont(s32 startX, s32 startY, s32 width, s32 height, s32 maxLetters, u32 flags, s32 texpageX, s32 texpageY, s32 clutX, s32 clutY, void* pCompressedFontFile);
extern void SystemInitializeDisplayEnvironments(void);

void GameSetUpDisplayAndChangeState(void) {
    int gameState;

    D_8005959C = 1;
    ArchiveCdDataSync(0);
    ArchiveSetIndex(0xC, 0);

    if (*g_DebuggerState != -1) {
        FontSetExternalBuffer((void*)0x80200000);
        FontLoadFont(0x10, 0x10, 0x140, 0x100, 0x3E8, 0, 0x340, 0, 0x340, 0x20, 0);
    }

    SystemInitializeDisplayEnvironments();
    func_80070F40();

    if (D_800C48EA == 1 || D_800C48EA == 0x40 || D_800C48EA == 0x21) {
        if (D_800D3338 != 0) {
            gameState = 6;
        } else if (g_GameResumeScene == 0) {
            if ((g_NextMapId & 0x7FF) < 0x400) {
                gameState = 1;
            } else {
                gameState = 3;
            }
        } else {
            gameState = 2;
        }
        ChangeGameState(gameState);
    } else if (D_800C48EA == 0x81) {
        GamePartySignalReinitialize();
        g_NextMapId = 0x1EA;
        g_NextMapRotationY = 0;
        D_8006F952 = 0;
        g_NextMapFlags = 0;
        ChangeGameState(1);
    }

    if (g_GameResumeScene == 0) {
        g_GameSelectedSkinLoadPending = 1;
    }

    MainLoop(0);
}

extern DISPENV D_800C4A7C;
extern s32 ArchiveNoOp();

void SystemInitializeDisplayEnvironments(void) {
    u8* pEnv;
    DRAWENV* pDrawEnv;

    ResetGraph(1);
    ArchiveNoOp(0x300, 0);
    ArchiveNoOp(ArchiveNoOp(8, 0x10, 0x140, 0xF0, 0, 0x1000));
    InitGeom();
    SetGeomOffset(0xA0, 0xB4);
    SetGeomScreen(0x200);

    pEnv = (u8*)&D_800C4A7C;
    SetDefDispEnv((DISPENV*)pEnv, 0, 0xE0, 0x140, 0xE0);
    pDrawEnv = (DRAWENV*)pEnv - 1;
    SetDefDrawEnv(pDrawEnv, 0, 0, 0x140, 0xE0);
    SetDefDispEnv((DISPENV*)(pEnv + 0x4070), 0, 0, 0x140, 0xE0);
    pEnv += 0x4014;
    SetDefDrawEnv((DRAWENV*)pEnv, 0, 0xE0, 0x140, 0xE0);
    SystemSetDrawEnvBackgroundColor(pDrawEnv);
    SystemSetDrawEnvBackgroundColor((DRAWENV*)pEnv);
}

void SystemSetDrawEnvBackgroundColor(DRAWENV* pDrawEnv) {
    pDrawEnv->isbg = 1;
    pDrawEnv->dtd = 1;
    pDrawEnv->r0 = 60;
    pDrawEnv->g0 = 120;
    pDrawEnv->b0 = 120;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3", func_8001B970);
/*
Best: 85.15%. The tail needs g_GameResumeScene/g_MenuSavedMainMenuChoice defined in this unit to get
%gp_rel. That is safe now: maspsx only relaxes the `lui $at` form, and
GameSetUpDisplayAndChangeState reads g_GameResumeScene through `lui $v0`, so it keeps %hi/%lo (verified
by GameSetUpDisplayAndChangeState matching 100% while D_8005959C is defined here). The remaining
diff is the inner loop: the original keeps the even byte on an
advancing pointer and recomputes the odd byte as (i + j) + (g_GameState + 1),
gcc merges the two into one base with 0/1 offsets.
A restructured attempt (gp_rel tail confirmed OK by defining g_GameResumeScene/g_MenuSavedMainMenuChoice here)
left three gaps: gcc hoists the 0xF literal and the odd base out of the outer loop into
s4/s5 (the original reloads both per row into t1/t2 and saves only s0-s3), and the copy-back
loop comes out `sltu` where the original has `slt`, so the source compared something signed.
void func_8001B970(void) {
    u8 inBuf[0x14];
    u8 outBuf[0x14];
    u8* pSrc;
    u8* pEven;
    u8* pOdd;
    u8* pDst;
    u8* pOut;
    void* pBuffer;
    int i;
    int j;

    ArchiveSetIndex(ARCHIVE_DIR_MENUS, 0);
    HeapChangeCurrentUser(HEAP_USER_HIG, 0);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);
    ArchiveReadFileToBuffer(3, pBuffer, 0, 0x80);
    ArchiveCdDataSync(0);
    memmove(&g_GameState, pBuffer, 0x2358);
    HeapFree(pBuffer);

    pSrc = (u8*)&g_GameState;
    for (i = 0; i < 0x26C; i += 0x14) {
        pEven = inBuf;
        pOdd = inBuf + 1;
        for (j = 0; j < 0x14; j += 2) {
            *pEven = pSrc[i + j];
            *pOdd = pSrc[i + j + 1];
            if (pSrc[i + j] == 0xF && pSrc[i + j + 1] == 0) {
                break;
            }
            pEven += 2;
            pOdd += 2;
        }
        SystemDecodeGlyphCodesN((u16*)inBuf, outBuf, j / 2);
        pOut = outBuf;
        pDst = pSrc + i;
        while (pDst < pSrc + i + 0x14) {
            *pDst++ = *pOut++;
        }
    }

    for (i = 19; i >= 0; i--) {
        ((s16*)&g_SoundVolumeController)[i - 16] = 0;
    }
    g_MenuSavedMainMenuChoice = 6;
    g_GameResumeScene = 0;
} */

extern u8 g_SystemStreamedFileIndex;
extern s32 g_SystemStreamedFileBuffer;
extern s32 D_80059520;
extern s32 g_SystemStreamedFileData;

extern s32 func_800379D8(s32, s32, s32*, s32*, s32*);

void ArchiveLoadStreamedFilePair(void) {
    func_800379D8(g_SystemStreamedFileIndex, 0, &g_SystemStreamedFileBuffer, &D_80059520, &g_SystemStreamedFileData);
}

extern u8 g_GameSelectedSkinLoadPending;
extern u8 D_8005946C;
extern u8 g_SystemBackdropColor;
extern u8 D_800594D5;
extern u8 D_800594D6;
extern s32 g_SystemSemiTransMode;

void func_8001B970(void);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3", func_8001BB50);
/*
Best: 80.87%. The original writes these globals through %gp_rel, which maspsx only
emits for symbols the unit defines itself (a file-scope definition here turns each
reference into %gp_rel). This file also contains functions that use %lo for the same
symbols (e.g. GameSetUpDisplayAndChangeState on g_GameSelectedSkinLoadPending/g_GameResumeScene), so the unit cannot have both.
The mechanism is visible in the original asm: maspsx only rewrites the `lui $at`
form to gp_rel, and GameSetUpDisplayAndChangeState reaches those two symbols through a pointer
held in $v0 (`lui $v0,%hi; lbu %lo($v0)`), so it keeps %hi/%lo while the same
symbols are gp_rel elsewhere. That means the unit does define them; the reference
form is source-level and the two can coexist. Worth retrying with declarations
for g_GameSelectedSkinLoadPending/D_8005946C/g_SystemBackdropColor/g_SystemSemiTransMode in this unit and
g_GameResumeScene/g_GameSelectedSkinLoadPending read through a pointer local inside GameSetUpDisplayAndChangeState;
deciding who owns this .bss data is a unit-level call.
Confirmed in maspsx (__init__.py, store/load gp-rel branch): a symbol is relaxed to
%gp_rel only when it is in sbss_entries AND not in comm_symbols, i.e. only a file-local
(.lcomm) definition does it; a plain definition here becomes .comm (fcommon) and is never
relaxed. So making func_8001BB50 match would turn GameSetUpDisplayAndChangeState's store to g_GameSelectedSkinLoadPending
(gp_rel here impossible - retail has lui $at %hi there) into gp_rel and break that
function's verified 100%, which is the unit-level conflict.
void func_8001BB50(void) {
    g_GameSelectedSkinLoadPending = 1;
    D_8005946C = 0;
    func_8001B970();
    ArchiveCdDataSync(0);
    g_SystemBackdropColor = 0x88;
    D_800594D5 = 0x76;
    D_800594D6 = 0x54;
    g_SystemSemiTransMode = 2;
} */

// .bss
u32 g_SoundStreamReserveStart;
u32 g_SoundStreamReserveBlock;
u8 g_SoundEffectBankIndex;
u8 D_8005959C;
SoundFile* g_SoundSedsBank;
void* g_SoundStreamBuffer;

extern StreamDataQueueEntry g_SoundSedsStreamQueue[];
extern u8 g_SoundEffectIds[];
extern void SoundAddSedsEntry(SoundFile* pSoundFile);
extern void SoundPlaySoundEffectOnLastPair(s32 arg0);

void SoundLoadAndPlaySedsBank(void) {
    int i;
    u8 soundEffectId;
    void* pEndAddress;

    HeapChangeCurrentUser(HEAP_USER_HIG, 0);
    ArchiveSetIndex(0xC, 0);

    g_SoundStreamReserveStart = HeapAlloc(4, 1);
    g_SoundStreamReserveBlock = HeapAlloc(g_SoundStreamReserveStart + 0x7FE1C000, 1);
    pEndAddress = (void*)0x801E4000;
    g_SoundSedsBank = HeapAlloc(ArchiveDecodeAlignedSize(2), 1);
    g_SoundStreamBuffer = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);

    g_SoundSedsStreamQueue[0].archiveIndex = 2;
    g_SoundSedsStreamQueue[1].archiveIndex = 3;
    g_SoundSedsStreamQueue[1].pData = g_SoundStreamBuffer;
    g_SoundSedsStreamQueue[2].archiveIndex = 4;
    g_SoundSedsStreamQueue[2].pData = pEndAddress;
    g_SoundSedsStreamQueue[3].archiveIndex = 0;
    g_SoundSedsStreamQueue[3].pData = NULL;
    g_SoundSedsStreamQueue[0].pData = g_SoundSedsBank;
    ArchiveStartStreamQueue(g_SoundSedsStreamQueue, 0, 0x80);

    while (ArchiveDataSync() == 3) {}

    SoundAddSedsEntry(g_SoundSedsBank);

    if (g_SoundEffectBankIndex != 4) {
        for (i = 0; i < 3;) {
            soundEffectId = g_SoundEffectIds[g_SoundEffectBankIndex * 3 + i];
            i++;
            if (soundEffectId != 0xFF) {
                SoundPlaySoundEffectOnLastPair((g_SoundSedsBank->sedId << 16) | soundEffectId);
            }
        }
    }
}

extern int rand(void);

s32 GameRandomByteRange(u32 arg0, u32 arg1) {
    u32 arg0Byte;
    u32 arg1Byte;
    s32 range;
    s32 result;

    arg0Byte = arg0 & 0xFF;
    if (arg0Byte == 0xFF) {
        return 0xFF;
    }
    arg1Byte = arg1 & 0xFF;
    if (arg1Byte == 0) {
        return 0;
    }
    if (arg0Byte == arg1Byte) {
        return arg0Byte;
    }
    range = arg1Byte - arg0Byte;
    if (range < 0xFF) {
        result = (arg0 + (rand() & 0xFF) % (range + 1)) & 0xFF;
    } else {
        result = rand() & 0xFF;
    }
    return result;
}
