#include "common.h"
#include "main/game.h"
#include "system/menu.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/sound.h"
#include "psyq/libgpu.h"
#include "psyq/libcd.h"

extern s32 D_8004F330;
extern s32 D_8004F334;
extern s32 g_GamePartySkinsInitialized;
extern s32 D_8004F320;
extern s32 g_GameSceneMapNum;
extern s32 D_8004F31C;
extern s32 D_8004F32C;
extern s32 D_8004F344;
extern void *D_8005A4A0;
extern void *D_8005A4BC;
extern StreamDataQueueEntry g_PartyStreamDataQueue[4];
extern int g_GamePartyMemberSkins[3];
extern int g_PartyIsWaitingForStreamData;
extern void* g_PartyDataBuffers[];
extern void* g_PartyStreamDataPointers[];

extern u8* D_800592D4;
extern u8* D_800592D8;
extern void* D_800592DC;
extern void* D_800592E0;

void func_8001A5CC(void) {
    int i;
    int j;

    D_800592DC = HeapAlloc(0x3480, 1);
    D_800592E0 = HeapAlloc(0x3480, 1);
    D_800592D4 = HeapAlloc(0x460, 1);
    D_800592D8 = HeapAlloc(0x460, 1);

    for (i = 0; i < 0x1C; i++) {
        for (j = 0; j < 0x28; j++) {
            D_800592D4[i * 0x28 + j] = 0;
            D_800592D8[i * 0x28 + j] = 0;
        }
    }
}

void func_8001A684(s32 arg0, s32 arg1) {
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

    D_800592D8[arg0 * 0x28 + arg1] += 1;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3", func_8001A6E8);

extern s32 D_8004F364;
extern s32 D_8004F36C;
extern s32 D_8004F33C;
extern s32 D_8004F338;
extern s32 D_8004F328;
extern s32 D_8004F324;
extern s32 D_8004F2FC;
extern s32 D_8004F2F8;
extern s32 D_8004F314;
extern s32 D_8004F310;
extern s32 D_8004F370;
extern s32 D_8004F35C;
extern s32 g_GameHasLoadedWDS;
extern s32 D_8004F358;
extern s32 D_8004F354;
extern s32 D_8004F350;
extern s32 D_8004F2F4;
extern s32 D_8004F348;
extern s32 D_8004F304;
extern s32 D_8004F368;
extern s32 D_8004F300;
extern s32 D_8004F380;
extern s32 D_8004F37C;
extern s32 D_8004F378;
extern u8 D_8005942C;
extern u8 D_800594D0;
extern s16 D_8004F384;
extern s32 D_8004F318;
extern s32 D_8004F340;
extern s32 D_8004F308;
extern s32 D_8006F990[3];
extern s32 D_8005A444[3];
extern s32 D_80062524[1];

void func_8001AADC(void) {
    int i;

    D_8004F364 = 1;
    D_8004F328 = 0xFF;
    D_8004F324 = 0xFF;
    D_8004F2FC = 0;
    D_8004F36C = 0;
    D_8004F2F8 = 0;
    D_8004F31C = 0;
    D_8004F320 = 0;
    D_8004F314 = 0;
    D_8004F310 = 0;
    g_GamePartySkinsInitialized = 0;
    D_8004F370 = 0;
    D_8004F35C = 0;
    g_GameHasLoadedWDS = 0;
    g_PartyIsWaitingForStreamData = 0;
    D_8004F358 = 0;
    D_8004F354 = 0;
    D_8004F350 = 0;
    D_8004F2F4 = 0;
    D_8004F344 = 0;
    D_8004F348 = 0;
    D_8004F304 = 0;
    D_8004F368 = 0;
    D_8004F300 = 0;
    D_8004F380 = 0;
    D_8004F37C = 0;
    D_8004F378 = 0;
    D_8005942C = 0;
    D_800594D0 = 0;
    D_8004F384 = 0;
    D_8004F318 = 0;
    D_8004F334 = -1;
    g_GameSceneMapNum = -1;
    D_8004F33C = -1;
    D_8004F338 = -1;
    D_8004F330 = -1;
    D_8004F32C = -1;
    D_8004F340 = -1;
    D_8004F308 = -1;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        g_GamePartyMemberSkins[i] = 0;
        D_8006F990[i] = 0;
        D_8005A444[i] = 0;
        g_GamePartyMembers[i] = 0;
    }

    for (i = 3; i >= 0; i--) {
        D_80062524[i - 3] = 0;
    }
}

void GamePartySignalReinitialize(void) {
    g_GamePartySkinsInitialized = 0;
}

void func_8001ACA4(void) {
    D_8004F334 = -1;
    D_8004F330 = -1;
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
    func_80029AFC(g_PartyStreamDataQueue, 0, 0);
    D_8004F31C = 1;
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
    func_80029AFC(g_PartyStreamDataQueue, 0, 0);
    D_8004F31C = 2;
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
        if (!(g_GameSceneMapNum & 0xC000)) D_8004F320 = 0;
        else D_8004F320 = 1;
        
        g_PartyIsWaitingForStreamData = 0;
        if (D_8004F320 == 0) {
            if (D_8004F31C != 1) {
                GamePartyCharactersInitializeSkins();
                g_PartyIsWaitingForStreamData = 1;
            }
        } else if (D_8004F31C != 2) {
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
        D_8005A4A0 = HeapAlloc(ArchiveDecodeAlignedSize(0xA7), 0x1);
        g_PartyStreamDataQueue[entryIndex].pData = D_8005A4A0;
        if (g_PartyStreamDataQueue[entryIndex].pData != 0) {
            HeapPinBlock(g_PartyStreamDataQueue[entryIndex].pData);
            g_PartyStreamDataQueue[entryIndex].archiveIndex = 0xA7;
            entryIndex++;
            D_8004F344 = 1;
        }

        D_8005A4BC = HeapAlloc(ArchiveDecodeAlignedSize(0xA8), 0x1);
        g_PartyStreamDataQueue[entryIndex].pData = D_8005A4BC;
        if (g_PartyStreamDataQueue[entryIndex].pData != 0) {
            HeapPinBlock(g_PartyStreamDataQueue[entryIndex].pData);
            g_PartyStreamDataQueue[entryIndex].archiveIndex = 0xA8;
            entryIndex++;
            D_8004F32C = 0;
        }
    }

    g_PartyStreamDataQueue[entryIndex].pData = 0;
    g_PartyStreamDataQueue[entryIndex].archiveIndex = 0;
    func_80029AFC(g_PartyStreamDataQueue, 0, 0); // Start streaming the queued requests
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


extern int D_8005A4C0; // File size
extern void* D_8005A4E0; // File buffer

extern s32 D_8004F330;
extern s32 D_8004F334;

int func_8001B484(int fileIndex, s32 arg1) {
    if (D_8004F334 == arg1 && D_8004F330 == fileIndex) {
        return 0;
    }

    if (ArchiveDataSync() == 0) {
        ArchiveCdDataSync(0);
        if (D_8004F334 != -1) {
            HeapUnpinBlock(D_8005A4E0);
            HeapFree(D_8005A4E0);
        }
        func_8001B53C(fileIndex);
        D_8004F334 = arg1;
        D_8004F330 = fileIndex;
    }
    
    return -1;
}

void func_8001B53C(int index) {
    D_8005A4C0 = ArchiveDecodeAlignedSize(index + 0xB8);
    D_8005A4E0 = HeapAlloc(D_8005A4C0, 1);
    HeapPinBlock(D_8005A4E0);
    ArchiveReadFileToBuffer(index + 0xB8, D_8005A4E0, 0, CdlModeSpeed);
}

extern int g_GameHasLoadedWDS;
extern SoundWDSEntry* g_GameCurLoadedWDS;

void func_8001B5A8(void) {
    if (g_GameHasLoadedWDS == 1) {
        SoundFreeWdsEntry(g_GameCurLoadedWDS);
        g_GameHasLoadedWDS = 0;
    }
}

extern s32 D_8004F35C;
extern s32 D_80062528;
extern s32 D_8004F348;
extern s32 D_8004F2FC;

extern void func_80039C4C(s32);
extern void func_800399D4(s32);

void func_8001B5E8(void) {
    if (D_8004F35C == 1) {
        func_80039C4C(D_80062528);
        if (D_8004F348 == 0) {
            func_800399D4(D_80062528);
        } else {
            D_8004F2FC = D_80062528;
        }
        D_8004F35C = 0;
        D_8004F348 = 0;
    }
}

extern s32 D_8004F36C;
extern s32 D_8004F33C;
void func_8001B5E8(void);

void func_8001B66C(void) {
    if (D_8004F36C != 0) {
        func_8001B5E8();
        func_8001B5A8();
    }
    D_8004F33C = -1;
    D_8004F338 = -1;
    D_8004F36C = 0;
}

void func_8001B6BC(void) {}

extern u8 D_8005959C;
extern s32* D_8005917C;
extern u8 D_800C48EA;
extern u8 D_800D3338;
extern u8 D_8005947C;
extern u8 D_800594F8;
extern u16 D_8006F94E;
extern s16 D_8006F950;
extern s16 D_8006F952;
extern s16 D_8006F954;
extern void func_80070F40(void);
extern void ChangeGameState(unsigned int state);
extern void MainLoop(s32 arg0);
extern void* FontLoadFont(s32 startX, s32 startY, s32 width, s32 height, s32 maxLetters, u32 flags, s32 texpageX, s32 texpageY, s32 clutX, s32 clutY, void* pCompressedFontFile);
extern void func_8001B844(void);

void func_8001B6C4(void) {
    int gameState;

    D_8005959C = 1;
    ArchiveCdDataSync(0);
    ArchiveSetIndex(0xC, 0);

    if (*D_8005917C != -1) {
        func_8003747C((void*)0x80200000);
        FontLoadFont(0x10, 0x10, 0x140, 0x100, 0x3E8, 0, 0x340, 0, 0x340, 0x20, 0);
    }

    func_8001B844();
    func_80070F40();

    if (D_800C48EA == 1 || D_800C48EA == 0x40 || D_800C48EA == 0x21) {
        if (D_800D3338 != 0) {
            gameState = 6;
        } else if (D_8005947C == 0) {
            if ((D_8006F94E & 0x7FF) < 0x400) {
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
        D_8006F94E = 0x1EA;
        D_8006F950 = 0;
        D_8006F952 = 0;
        D_8006F954 = 0;
        ChangeGameState(1);
    }

    if (D_8005947C == 0) {
        D_800594F8 = 1;
    }

    MainLoop(0);
}

extern DISPENV D_800C4A7C;
extern s32 func_800379D0();

void func_8001B844(void) {
    u8* pEnv;
    DRAWENV* pDrawEnv;

    ResetGraph(1);
    func_800379D0(0x300, 0);
    func_800379D0(func_800379D0(8, 0x10, 0x140, 0xF0, 0, 0x1000));
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
    func_8001B94C(pDrawEnv);
    func_8001B94C((DRAWENV*)pEnv);
}

void func_8001B94C(DRAWENV* pDrawEnv) {
    pDrawEnv->isbg = 1;
    pDrawEnv->dtd = 1;
    pDrawEnv->r0 = 60;
    pDrawEnv->g0 = 120;
    pDrawEnv->b0 = 120;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3", func_8001B970);
/*
Best: 85.15%. The tail needs D_8005947C/D_800594CC defined in this unit to get
%gp_rel. That is safe now: maspsx only relaxes the `lui $at` form, and
func_8001B6C4 reads D_8005947C through `lui $v0`, so it keeps %hi/%lo (verified
by func_8001B6C4 matching 100% while D_8005959C is defined here). The remaining
diff is the inner loop: the original keeps the even byte on an
advancing pointer and recomputes the odd byte as (i + j) + (g_GameState + 1),
gcc merges the two into one base with 0/1 offsets.
A restructured attempt (gp_rel tail confirmed OK by defining D_8005947C/D_800594CC here)
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
        func_80033B34((u16*)inBuf, outBuf, j / 2);
        pOut = outBuf;
        pDst = pSrc + i;
        while (pDst < pSrc + i + 0x14) {
            *pDst++ = *pOut++;
        }
    }

    for (i = 19; i >= 0; i--) {
        ((s16*)&g_SoundVolumeController)[i - 16] = 0;
    }
    D_800594CC = 6;
    D_8005947C = 0;
} */

extern u8 D_8006F9DE;
extern s32 D_80059470;
extern s32 D_80059520;
extern s32 D_8005949C;

extern s32 func_800379D8(s32, s32, s32*, s32*, s32*);

void func_8001BB0C(void) {
    func_800379D8(D_8006F9DE, 0, &D_80059470, &D_80059520, &D_8005949C);
}

extern u8 D_800594F8;
extern u8 D_8005946C;
extern u8 D_800594D4;
extern u8 D_800594D5;
extern u8 D_800594D6;
extern s32 D_800595A0;

void func_8001B970(void);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3", func_8001BB50);
/*
Best: 80.87%. The original writes these globals through %gp_rel, which maspsx only
emits for symbols the unit defines itself (a file-scope definition here turns each
reference into %gp_rel). This file also contains functions that use %lo for the same
symbols (e.g. func_8001B6C4 on D_800594F8/D_8005947C), so the unit cannot have both.
The mechanism is visible in the original asm: maspsx only rewrites the `lui $at`
form to gp_rel, and func_8001B6C4 reaches those two symbols through a pointer
held in $v0 (`lui $v0,%hi; lbu %lo($v0)`), so it keeps %hi/%lo while the same
symbols are gp_rel elsewhere. That means the unit does define them; the reference
form is source-level and the two can coexist. Worth retrying with declarations
for D_800594F8/D_8005946C/D_800594D4/D_800595A0 in this unit and
D_8005947C/D_800594F8 read through a pointer local inside func_8001B6C4;
deciding who owns this .bss data is a unit-level call.
void func_8001BB50(void) {
    D_800594F8 = 1;
    D_8005946C = 0;
    func_8001B970();
    ArchiveCdDataSync(0);
    D_800594D4 = 0x88;
    D_800594D5 = 0x76;
    D_800594D6 = 0x54;
    D_800595A0 = 2;
} */

// .bss
u32 D_80059480;
u32 D_800594AC;
u8 D_8005954C;
u8 D_8005959C;
SoundFile* D_800595D0;
void* D_800595A8;

extern StreamDataQueueEntry D_8006F9BC[];
extern void* D_8006F9C0;
extern u16 D_8006F9C4;
extern void* D_8006F9C8;
extern u16 D_8006F9CC;
extern void* D_8006F9D0;
extern u16 D_8006F9D4;
extern void* D_8006F9D8;
extern u8 D_8004F388[];
extern void SoundAddSedsEntry(SoundFile* pSoundFile);
extern void func_80039DB8(s32 arg0);

void func_8001BBAC(void) {
    int i;
    u8 soundEffectId;
    void* pEndAddress;

    HeapChangeCurrentUser(HEAP_USER_HIG, 0);
    ArchiveSetIndex(0xC, 0);

    D_80059480 = HeapAlloc(4, 1);
    D_800594AC = HeapAlloc(D_80059480 + 0x7FE1C000, 1);
    pEndAddress = (void*)0x801E4000;
    D_800595D0 = HeapAlloc(ArchiveDecodeAlignedSize(2), 1);
    D_800595A8 = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);

    D_8006F9BC[0].archiveIndex = 2;
    D_8006F9C4 = 3;
    D_8006F9C8 = D_800595A8;
    D_8006F9CC = 4;
    D_8006F9D0 = pEndAddress;
    D_8006F9D4 = 0;
    D_8006F9D8 = NULL;
    D_8006F9C0 = D_800595D0;
    func_80029AFC(D_8006F9BC, 0, 0x80);

    while (ArchiveDataSync() == 3) {}

    SoundAddSedsEntry(D_800595D0);

    if (D_8005954C != 4) {
        for (i = 0; i < 3;) {
            soundEffectId = D_8004F388[D_8005954C * 3 + i];
            i++;
            if (soundEffectId != 0xFF) {
                func_80039DB8((D_800595D0->sedId << 16) | soundEffectId);
            }
        }
    }
}

extern int rand(void);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp3", func_8001BD40);
/*
Best: 90.64%. Only the then-branch is short one `andi v0, v1, 0xff`: with a single
`return result` gcc keeps the if/else block order of the original but drops that
mask, and with two returns it reorders the two blocks.
u8 func_8001BD40(u32 arg0, u32 arg1) {
    u32 arg0Byte;
    u32 arg1Byte;
    s32 range;
    u8 result;

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
        result = (arg0 + rand() % (range + 1)) & 0xFF;
    } else {
        result = rand() & 0xFF;
    }
    return result;
} */

/*
Best: 89.10%. Structure and branch polarity match; what is left is gcc reusing
the incoming argument register a1 as a temporary for the masked arg0 (the
original keeps masked arg0 in v1) and merging the trailing `& 0xFF` so the
epilogue is shared instead of ending each block with `j ... ; andi v0,v1,0xff`.
Measured cause of the merge: our compile masks the sum in place in v0 and the
else path masks rand() in place in v0 too, so gcc 2.7 cross-jumps them onto one
shared `andi v0,v0,0xff`. The original sums into v1 (`addu v1,s1,v1; j Lx;
andi v0,v1,0xff`), a different insn, so its tails stay separate. A u8 result
local, an explicit trailing `& 0xFF`, and an `s32` return type all still merge.
u8 func_8001BD40(u8 arg0, u8 arg1) {
    s32 diff;

    if (arg0 == 0xFF) {
        return 0xFF;
    }
    if (arg1 == 0) {
        return 0;
    }
    if (arg0 == arg1) {
        return arg0;
    }
    diff = arg1 - arg0;
    if (diff >= 0xFF) {
        return rand() & 0xFF;
    }
    return arg0 + (rand() & 0xFF) % (diff + 1);
}
*/
