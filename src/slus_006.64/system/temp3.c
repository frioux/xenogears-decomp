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

// One byte per cell, 28 rows of 40.
#define SYSTEM_CELL_ROWS 28
#define SYSTEM_CELL_COLUMNS 40
#define SYSTEM_CELL_COUNT (SYSTEM_CELL_ROWS * SYSTEM_CELL_COLUMNS)

extern u8* g_SystemCellGrid;
extern u8* g_SystemCellNeighborCounts;
extern void* g_SystemCellPrimBuffers[2];

void SystemInitializeCellGridBuffers(void) {
    int i;
    int j;

    g_SystemCellPrimBuffers[0] = HeapAlloc(0x3480, 1);
    g_SystemCellPrimBuffers[1] = HeapAlloc(0x3480, 1);
    g_SystemCellGrid = HeapAlloc(SYSTEM_CELL_COUNT, 1);
    g_SystemCellNeighborCounts = HeapAlloc(SYSTEM_CELL_COUNT, 1);

    for (i = 0; i < SYSTEM_CELL_ROWS; i++) {
        for (j = 0; j < SYSTEM_CELL_COLUMNS; j++) {
            g_SystemCellGrid[i * SYSTEM_CELL_COLUMNS + j] = 0;
            g_SystemCellNeighborCounts[i * SYSTEM_CELL_COLUMNS + j] = 0;
        }
    }
}

void SystemIncrementCellNeighborCount(s32 arg0, s32 arg1) {
    if (arg0 < 0) {
        arg0 = SYSTEM_CELL_ROWS;
    }
    if (arg0 > SYSTEM_CELL_ROWS) {
        arg0 = 0;
    }

    if (arg1 < 0) {
        arg1 = SYSTEM_CELL_COLUMNS;
    }
    if (arg1 > SYSTEM_CELL_COLUMNS) {
        arg1 = 0;
    }

    g_SystemCellNeighborCounts[arg0 * SYSTEM_CELL_COLUMNS + arg1] += 1;
}

extern int g_RenderContextIndex;

/* Best: 98.51%, 253 insns, 1012 bytes = retail exact 0x3F4 (re-measured twice this round). The residual is a PURE
   CALLEE-SAVED PERMUTATION, nothing else: every differing word is the same instruction with the same
   operands, only retail i=$s4 / j=$s3 / pPrim+8=$s2 comes out here as i=$s3 / j=$s2 / pPrim+8=$s4 (the
   save slots 52/56 swap with them). Frame, callee-saved set, spill slots (gridOffset 0x20, rowOffset
   0x18, pOt 0x10) and count-in-$fp identical. PROVEN UNREACHABLE by the -da/-dl dump (round 433), which
   the note asked for: cc1 -O2 -G8 -da/-dl on this body gives `21 regs to allocate: 150 115 73 74 161 80
   230 79 ...` with 73 (j) 29 refs/52 insns, 74 (i) 33 refs/79, 79 (pPrim+8) 28 refs/95, so the
   allocno_compare quotients are j ~8.9, i ~8.4, pPrim+8 ~4.7 and the hand-out is j=s2, i=s3, prim+8=s4.
   Retail is the REVERSE (prim+8=s2, j=s3, i=s4), i.e. it needs a pseudo with half the quotient and twice
   the live range to be processed first - lever a-global-allocno-priority-is' 2x/10x case, not the
   near-tie case. Size is pinned (all three SI), the live length is the loop, and refs are a function of
   the emitted code, so no spelling of this instruction stream reaches it. Already measured and dead: two
   permuter runs (base 580 -> 375, then
   nothing; closed 97.70 -> 98.51 on the `one = 1` y hint), refs-driven allocation, declaration order,
   explicit i-minus-one/i-plus-one locals, a mask temp, and (this round) splitting the y update into
   `y -= one; y += rand() % 3;` to make retail's `addiu a1,s4,-1` precede the rand call = 97.75%,
   1016 bytes, frame 88 - worse, because the store to `y` needs a stack slot.
*/
/*
void func_8001A6E8(u_long* pOt) {
    s32 j;
    s32 i;
    s32 count;
    int one; // register-allocation hint: the y decrement must not fold into the add
    s32 rowOffset;
    s32 gridOffset;
    s32 x;
    s32 y;
    u8* pPrim;

    pPrim = (u8*)g_SystemCellPrimBuffers[g_RenderContextIndex];
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
            y = y - (one = 1) + rand() % 3;
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
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3", func_8001A6E8);




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
extern s32 g_FieldP2RenderToggle;
extern s32 g_FieldDisableCompassRender;
extern u8 g_FieldDiscChangeFade;
extern u8 g_GameDiscChangeRequest;
extern s16 g_GameWdsEntryReleased;
extern s32 g_FieldScriptWalkFrameCounter;
extern s32 g_GameAudioFadeInPending;
extern s32 g_GameWdsBankLoadState;
extern s32 g_FieldPartyActorIndices[3];
extern s32 g_FieldPartyActorSlots[3];
extern void* g_GameWdsEntries[];

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
    g_FieldP2RenderToggle = 0;
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
        g_GameWdsEntries[i] = NULL;
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

extern u8 g_DebugPageIndex;
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

    g_DebugPageIndex = 1;
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


// .bss
u8 g_DebugPageIndex;

