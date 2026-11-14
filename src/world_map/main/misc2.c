#include "common.h"
#include "main/game.h"
#include "psyq/libapi.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"

extern int Vsync(int mode);
extern int rand(void);
extern void SoundStopAllAudioManagers(void);
extern void SoundDestroyAudioManager(s32* pFile);
extern void* SoundCreateAudioManagerOnHeap(void* pFile);
extern void SoundPlayAudioManager(s32* pFile, s32 volume, s32 frames);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);

typedef struct {
    /* 0x0 */ s16 tile;
    /* 0x2 */ s16 duration;
} WorldMapFrame; // Size: 0x4

typedef struct {
    /* 0x0 */ RECT rect;
    /* 0x8 */ u32 unk8;
    /* 0xC */ WorldMapFrame* pFrames;
} WorldMapSprite; // Size: 0x10

typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ WorldMapSprite* pSprite;
    /* 0x8 */ s16 unk8;
    /* 0xA */ s16 unkA;
} WorldMapEntry; // Size: 0xC

typedef struct {
    /* 0x0 */ u32 data[0x800];
} ActorBlock; // Size: 0x2000

typedef struct {
    /* 0x0 */ s16 data[0x10];
} SlotBlock; // Size: 0x20

typedef struct {
    /* 0x0 */ u32 data[0xA0];
} UnkBlock; // Size: 0x280

typedef struct {
    /* 0x00 */ u8 pad0[0x24];
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u16 unk26;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ u8 pad34[0x24];
    /* 0x58 */ s32 unk58;
    /* 0x5C */ u8 pad5C[0x24];
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x0 */ s16 vx;
    /* 0x2 */ s16 vy;
    /* 0x4 */ s16 vz;
    /* 0x6 */ u16 pad6;
} Vector16; // Size: 0x8

extern void WorldMapInitGfxEnv(void);
extern void func_80072DB4(s32 arg0, s32 arg1, s32 arg2, s32 arg3);
extern void func_8008440C(void);
extern void func_800978FC(void);
extern void func_8008901C(void);
extern void func_800865A0(void);
extern void func_80085FE0(void);
extern void SystemTransferPaletteToVRAM(short xDest, short yDest);
extern void ControllerResetState(void);
extern void ArchiveCdDataSync(s32 arg0);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void* D_8005945C;
extern void* D_8009C7E4;
extern s32 D_8009C174;
extern void* D_8009BD20;
extern u8* D_8009BE3C;
extern u_long* D_8009C800;
extern u_long* D_8009C890;
extern s32 D_8009D14C;
extern s32 D_8009D804;
extern u_char D_80059179;

extern u32 D_8005A4E4[];
extern u8 D_8009CEC4[];
extern s32 D_8009BBB4;
extern s32 D_8009BBB8;
extern s32 D_8009BBBC;
extern s32 D_8009BD38;
extern s32 D_8009BD3C;
extern s32 D_8009BE0C;
extern s32 D_8009BE28;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 D_8009C838;
extern s32 D_8009C83C;
extern s16 D_8009D154;
extern s32 D_8009D3F0;
extern s16 D_8009D52C;
extern s32 D_8009D55C;
extern s32 D_8009D560;
extern s32 D_8009D564;

extern s32 func_80094028(s32 arg0);
extern s32 func_80093F18(s32 arg0);
extern s16 D_8009A3A0[];
extern u16 D_8009B578[];
extern u8* D_8009D73C[];
extern u8 D_800658DC[];
extern u8 D_80059508;

extern WorldMapActor* D_8009BE24;
extern s32 D_8009BE10;
extern u8* D_8009BE14;
extern u8* D_8009BE18;
extern s32 D_8009BE38;
extern u8* D_8009D30C;
extern u8* D_8009D780;
extern s32 D_8009C180;
extern s32 D_8009CD48;
extern s32 D_8009BD30;
extern s32 D_8009D308;
extern s32 D_8009BCC0;
extern s32 D_8009C7EC;
extern s32 D_8009D77C;
extern s32 D_8009D7C8;
extern void* LZSSHeapDecompress(void* pCompressedData, int flags);
extern s32 D_8009CC9C;
extern u8 D_8009A1E8[];
extern u8* D_8009D7D0;
extern s16 D_8009C872[1];
extern s16 D_8009C854[];
extern s32 D_8009BCC4;
extern s32 D_8009BE40;
extern s32 D_8009D64C;
extern s32 D_8009D80C;
extern s32* D_80062528;
extern s32 D_8009CD64;
extern u8 D_8009A250[];
extern u8 D_80062648[];

void WorldMapFreePrimBuffers(void) {
    HeapFree(D_8009BE18);
    HeapFree(D_8009BE14);
    HeapFree(D_8009D30C);
}

void WorldMapAddSlotPosition(s16 arg0, s32* pVec) {
    Vector16* pEntry;
    s32 index;

    index = D_8009BE38;
    pEntry = (Vector16*)((index << 3) + (s32)D_8009D30C);
    pEntry->vx = pVec[0] >> 12;
    pEntry->vz = pVec[2] >> 12;
    pEntry->vy = arg0;
    D_8009BE38 = (index + 1) & 0xF;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_800747DC);
void WorldMapInitTileAnimEntries(void) {
    WorldMapEntry* pEntry;
    s32 count;
    s32 i;

    count = ((s32*)D_8009D77C)[0];
    D_8009CC9C = count;
    pEntry = (WorldMapEntry*)HeapAlloc(count * 0xC, 0);
    D_8009D780 = (u8*)pEntry;
    for (i = 0; i < D_8009CC9C; i++, pEntry++) {
        pEntry->unk0 = (u32)D_8009D77C + ((s32*)D_8009D77C)[i + 1];
        pEntry->pSprite = (WorldMapSprite*)(D_8009A1E8 + i * 0x10);
        pEntry->unk8 = 0;
        pEntry->unkA = 1;
    }
}

void WorldMapFreeTileAnimEntries(void) {
    HeapFree(D_8009D780);
}

void WorldMapUpdateTileAnimEntries(void) {
    WorldMapEntry* pEntry;
    s32 i;

    pEntry = (WorldMapEntry*)D_8009D780;
    for (i = 0; i < D_8009CC9C; i++, pEntry++) {
        pEntry->unkA--;
        if (pEntry->unkA == 0) {
            pEntry->unk8++;
            pEntry->unkA = pEntry->pSprite->pFrames[pEntry->unk8].duration;
            if (pEntry->unkA < 0) {
                pEntry->unk8 = 0;
                pEntry->unkA = pEntry->pSprite->pFrames[0].duration;
            }
            LoadImage(&pEntry->pSprite->rect,
                      (u_long*)(pEntry->unk0 + (pEntry->pSprite->pFrames[pEntry->unk8].tile << 4)));
        }
    }
}

void WorldMapInitFrameAnimEntries(void) {
    WorldMapEntry* pEntry;
    s32 count;
    s32 i;

    count = ((s32*)D_8009D7C8)[0];
    D_8009CD64 = count;
    pEntry = (WorldMapEntry*)HeapAlloc(count * 0xC, 0);
    D_8009D7D0 = (u8*)pEntry;
    for (i = 0; i < D_8009CD64; i++, pEntry++) {
        pEntry->unk0 = (u32)D_8009D7C8 + ((s32*)D_8009D7C8)[i + 1];
        pEntry->pSprite = (WorldMapSprite*)(D_8009A250 + i * 0x10);
        pEntry->unk8 = 0;
        pEntry->unkA = 1;
    }
}

void WorldMapFreeFrameAnimEntries(void) {
    HeapFree(D_8009D7D0);
}

void WorldMapUpdateFrameAnimEntries(void) {
    WorldMapEntry* pEntry;
    s32 offset;
    s32 i;

    pEntry = (WorldMapEntry*)D_8009D7D0;
    for (i = 0; i < D_8009CD64; i++, pEntry++) {
        pEntry->unkA--;
        if (pEntry->unkA == 0) {
            pEntry->unk8++;
            pEntry->unkA = pEntry->pSprite->pFrames[pEntry->unk8].duration;
            if (pEntry->unkA < 0) {
                pEntry->unk8 = 0;
                pEntry->unkA = pEntry->pSprite->pFrames[0].duration;
            }
            offset = pEntry->pSprite->rect.h * pEntry->pSprite->rect.w * 2;
            LoadImage(&pEntry->pSprite->rect,
                      (u_long*)(pEntry->unk0 +
                                pEntry->pSprite->pFrames[pEntry->unk8].tile * offset));
        }
    }
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075228);

/* 94.80%: the clear loop and the 0x300/0x180 selection match; every remaining
   slot is gcc's constant CSE (`li v1,1` for D_8009D64C then a fresh `li v0,1`
   for D_8009BCC4 in retail, one shared register here, which also moves the
   width value from v0 to v1). Reading g_GameState.unk1834 into a `u16 flags`
   local *before* the D_8009D64C store is what puts the `lhu` ahead of it, and
   `if (!(flags & 0x4000)) w = 0x180; else w = 0x300;` is what produces retail's
   `bnez` polarity. Also tried: ternary, direct store per branch, -O3 (85.8),
   gcc-2.7.2-cdk (85.8), local vs literal constant, declaration order. The clear
   loop must decrement a `s16*` local from D_8009C872, not index D_8009C854.
void func_80075228(void) {
    s32 i;
    s32 screenWidth;
    u16 flags;
    s16* p;

    i = 0xF;
    p = D_8009C872;
    for (; i >= 0; i--) {
        *p = 0;
        p--;
    }
    flags = g_GameState.unk1834;
    D_8009D64C = 1;
    if (!(flags & 0x4000)) {
        screenWidth = 0x180;
    } else {
        screenWidth = 0x300;
    }
    D_8009BE40 = screenWidth;
    D_8009BCC4 = 1;
    D_8009D80C = 0;
}
*/

void WorldMapUpdateRandomTimers(void) {
    s32 value;
    s32 j;
    s32 i;

    if (--D_8009D64C == 0) {
        for (i = 0; i < D_8009BCC4; i++) {
            if (i != 0) {
                do {
                    value = rand() % D_8009BE40 + 1;
                    for (j = 0; j < i; j++) {
                        if (D_8009C854[j] == value) {
                            break;
                        }
                    }
                } while (j < i);
            } else {
                value = rand() % D_8009BE40 + 1;
            }
            D_8009C854[i] = value;
        }
        D_8009D64C = D_8009BE40;
    }
    D_8009D80C = 0;
    for (i = 0; i < D_8009BCC4; i++) {
        if (--D_8009C854[i] == 0) {
            D_8009D80C++;
        }
    }
}
void WorldMapSaveState(void) {
    u8* pSave;

    pSave = D_8005A4E4;
    *(ActorBlock*)pSave = *(ActorBlock*)D_8009BE24;
    *(s32*)(pSave + 0x2000) = D_8009D55C;
    *(s32*)(pSave + 0x2004) = D_8009D560;
    *(s32*)(pSave + 0x2008) = D_8009D564;
    *(s32*)(pSave + 0x2010) = D_8009D52C;
    *(s32*)(pSave + 0x2014) = D_8009BE40;
    *(s32*)(pSave + 0x2018) = D_8009BCC4;
    *(s32*)(pSave + 0x201C) = D_8009D64C;
    *(SlotBlock*)(pSave + 0x2020) = *(SlotBlock*)D_8009C854;
    *(UnkBlock*)(pSave + 0x2040) = *(UnkBlock*)D_8009CEC4;
    *(s32*)(pSave + 0x22C0) = D_8009D154;
    *(s32*)(pSave + 0x22C4) = D_8009BD38;
    *(s32*)(pSave + 0x22C8) = D_8009BD3C;
    *(s32*)(pSave + 0x22CC) = D_8009D3F0;
    *(s32*)(pSave + 0x22D0) = D_8009BE0C;
    *(s32*)(pSave + 0x22E4) = D_8009C838;
    *(s32*)(pSave + 0x22E8) = D_8009C83C;
    *(s32*)(pSave + 0x22D4) = D_8009BBB4;
    *(s32*)(pSave + 0x22D8) = D_8009BBB8;
    *(s32*)(pSave + 0x22DC) = D_8009BBBC;
    *(s32*)(pSave + 0x22EC) = D_8009BE28;
    *(s32*)(pSave + 0x22F0) = D_8009BE2C;
    *(s32*)(pSave + 0x22F4) = D_8009BE30;
}

typedef struct {
    /* 0x0 */ s32 w0;
    /* 0x4 */ s32 w1;
    /* 0x8 */ s32 w2;
    /* 0xC */ s32 w3;
} WorldMapQuad; // Size: 0x10

typedef struct {
    /* 0x0 */ s16 h0;
    /* 0x2 */ s16 h2;
    /* 0x4 */ s16 h4;
    /* 0x6 */ s16 h6;
} WorldMapShortQuad; // Size: 0x8

extern s32 D_8009C5AC;

void WorldMapLoadState(void) {
    u8* pSave;

    pSave = (u8*)D_8005A4E4;
    *(ActorBlock*)D_8009BE24 = *(ActorBlock*)pSave;
    *(WorldMapQuad*)&D_8009C5AC = *(WorldMapQuad*)(pSave + 0x2000);
    *(WorldMapQuad*)&D_8009D55C = *(WorldMapQuad*)(pSave + 0x2000);
    *(SlotBlock*)D_8009C854 = *(SlotBlock*)(pSave + 0x2020);
    D_8009D52C = *(s32*)(pSave + 0x2010);
    D_8009BE40 = *(s32*)(pSave + 0x2014);
    D_8009BCC4 = *(s32*)(pSave + 0x2018);
    D_8009D64C = *(s32*)(pSave + 0x201C);
    *(UnkBlock*)D_8009CEC4 = *(UnkBlock*)(pSave + 0x2040);
    *(WorldMapShortQuad*)&D_8009BD38 = *(WorldMapShortQuad*)(pSave + 0x22C4);
    *(WorldMapQuad*)&D_8009BBB4 = *(WorldMapQuad*)(pSave + 0x22D4);
    *(WorldMapShortQuad*)&D_8009C838 = *(WorldMapShortQuad*)(pSave + 0x22E4);
    *(WorldMapQuad*)&D_8009BE28 = *(WorldMapQuad*)(pSave + 0x22EC);
    D_8009D154 = *(s32*)(pSave + 0x22C0);
    D_8009D3F0 = *(s32*)(pSave + 0x22CC);
    D_8009BE0C = *(s32*)(pSave + 0x22D0);
}

extern s32 D_8009D7F0;
extern u16 D_8009BD3A;
extern s16 D_8006F950;
extern void* D_8009BC3C;
extern void* D_8009BCB4;
extern void* D_8009D528;
extern void WorldMapWaitForQueuedLoads(void);
extern void func_80086124(void);
extern void func_800866C8(void);
extern void func_80089128(void);
extern void WorldMapQueueTwoFileLoads(void);
extern int ArchiveDataSync(void);
extern void LZSSDecompress(void* pSrc, void* pDst);

void WorldMapEnterMenuMode(void) {
    RECT rect;
    void* pTemp;
    u_int size;
    s16* pOut;
    u16* pFlags;
    s32 i;

    i = 0;
    pFlags = &g_GameState.unk1836;
    pOut = (s16*)(pFlags + 3);
    *pFlags = 1;
    D_8006F950 = (D_8009BD3A + 0x2000) & 0x3FFF;
    for (; i < 3; i++, pOut++) {
        *pOut = g_GameState.gearRide[i];
    }
    D_8009D14C = D_80059179;
    if ((s16)func_80093F18((s32)&D_8009D55C) == 4) {
        D_80059179 = 1;
    }
    WorldMapWaitForQueuedLoads();
    func_80086124();
    func_800866C8();
    func_80089128();
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    pTemp = HeapAlloc(4, 1);
    HeapFree(pTemp);
    pTemp = (void*)((u32)pTemp & 0xFFFFFF);
    size = (u32)pTemp + 0xFFE3B004;
    D_8009C7E4 = HeapAlloc(size, 1);
    WorldMapQueueTwoFileLoads();
    D_8009C800 = HeapAlloc(0x10000, 0);
    D_8009C890 = HeapAlloc(0xC800, 0);
    rect.x = 0x180;
    rect.y = 0x100;
    rect.w = 0x80;
    rect.h = 0x100;
    StoreImage(&rect, D_8009C800);
    rect.x = 0;
    rect.y = 0x1B0;
    rect.w = 0x140;
    rect.h = 0x50;
    StoreImage(&rect, D_8009C890);
    if (D_8009D7F0 == 0) {
        rect.x = 0;
        rect.y = 0xD8;
        rect.w = 0x140;
        rect.h = 0xD8;
        MoveImage(&rect, 0, 0);
    }
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0x2C0, 0x100);
    DrawSync(0);
    func_80072DB4(0x10, 0, 8, 2);
    do {
    } while (ArchiveDataSync() >= 2);
    LZSSDecompress(D_8009D528, D_8009C7E4);
    HeapFree(D_8009D528);
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0, 0xE0);
    DrawSync(0);
    ArchiveCdDataSync(0);
}
void WorldMapExitMenuMode(void) {
    RECT rect;
    void* pBuffer;
    s32 mode;
    s32 w;
    s32 h;

    w = 0x100;
    h = 0x140;
    HeapChangeCurrentUser(3, 0);
    ArchiveSetIndex(0x24, 0);
    HeapFree(D_8005945C);
    HeapFree(D_8009C7E4);
    WorldMapInitGfxEnv();
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_8009C174), 1);
    D_8009BD20 = pBuffer;
    ArchiveReadFileToBuffer(D_8009C174, pBuffer, 0, 0);
    func_80072DB4(0x10, 0x80, -8, 2);
    rect.x = 0x2C0;
    rect.y = w;
    rect.w = h;
    rect.h = 0xD8;
    MoveImage(&rect, 0, 0);
    MoveImage(&rect, 0, 0xD8);
    ClearOTagR(*(u_long**)(D_8009BE3C + 0x70), 0x400);
    ArchiveCdDataSync(0);
    func_8008440C();
    rect.x = 0x180;
    rect.y = w;
    rect.w = 0x80;
    rect.h = w;
    LoadImage(&rect, D_8009C800);
    rect.x = 0;
    rect.y = 0x1B0;
    rect.w = h;
    rect.h = 0x50;
    LoadImage(&rect, D_8009C890);
    DrawSync(0);
    HeapFree(D_8009C800);
    HeapFree(D_8009C890);
    func_800978FC();
    func_8008901C();
    func_800865A0();
    func_80085FE0();
    SystemTransferPaletteToVRAM(0x130, 0x1E0);
    Vsync(0);
    ControllerResetState();
    mode = D_8009D14C;
    D_8009D804 = 0;
    g_GameState.unk1836 = 0;
    D_80059179 = mode;
    WorldMapSyncGearRideActors();
}

void WorldMapSyncGearRideActors(void) {
    WorldMapActor* pActors;
    u8 ride;
    s32 count;
    s32 i;

    pActors = D_8009BE24;
    for (i = 0; i < 3; i++) {
        ride = g_GameState.gearRide[i];
        if (ride != ((u8*)g_GameState.unk1838)[i * 2 + 4]) {
            if (ride == 0) {
                pActors[i + 1].unk28 = pActors[i + 4].unk28;
                pActors[i + 1].unk2C = pActors[i + 4].unk2C;
                pActors[i + 1].unk30 = pActors[i + 4].unk30;
                pActors[i + 1].unk58 = pActors[i + 4].unk58;
            } else {
                ((s16*)((u8*)g_GameState.unk1838 + 0x122))[i * 3] = 0x400;
                pActors[i + 4].unk28 = pActors[i + 1].unk28;
                pActors[i + 4].unk2C = pActors[i + 1].unk2C;
                pActors[i + 4].unk30 = pActors[i + 1].unk30;
                pActors[i + 4].unk24 = 0;
                pActors[i + 4].unk58 = pActors[i + 1].unk58;
            }
        }
    }
    count = 0;
    for (i = 0; i < 3; i++) {
        if (g_GameState.partyMembers[i] != 0xFF && g_GameState.gearRide[i] == 1) {
            count++;
        }
    }
    if (!(g_GameState.unk1834 & 0x4000)) {
        D_8009BE10 = count != 0 ? 2 : 1;
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075E7C);
/* 89.46%: the two calls, the s16 index read and the memcpy at the end all
   match. Remaining: the first limit test uses base+2 instead of the folded
   symbol+2, the 0x10-byte sum loop folds the (zone - 1) << 4 into the
   displacement instead of retail's addiu -1 then sll 4, and the pick loop
   bases at buf+1 rather than buf (giv re-basing, t0/a1 vs t1/a0). Also
   tried: u8* instead of s32 addresses (sltu vs retail's slt, 88.02/82.67),
   while instead of do-while (extra entry guard), indexing the u16 symbol
   directly instead of a pointer local.
s32 func_80075E7C(s32 arg0, s32 arg1) {
    u8 buf[0x10];
    u16* pLimit;
    s32 pSrc;
    s32 pDst;
    s32 pEnd;
    s32 index;
    s32 sum;
    s32 pick;
    s32 i;
    s32 zone;

    index = (s16)func_80094028(arg0);
    if ((s16)func_80093F18(arg0) == 4) {
        index = D_8009A3A0[index];
    }
    pLimit = D_8009B578;
    zone = 1;
    while (arg1 >= pLimit[zone]) {
        zone++;
    }
    zone--;
    sum = 0;
    pSrc = (s32)D_8009D73C[index] + 0x200 + (zone << 4);
    pDst = (s32)buf;
    pEnd = pSrc + 0x10;
    do {
        *(u8*)pDst = *(u8*)pSrc;
        sum += *(u8*)pSrc;
        pSrc++;
        pDst++;
    } while (pSrc < pEnd);
    if (sum <= 0) {
        return 0;
    }
    pick = rand() % sum + 1;
    i = 0;
    pSrc = (s32)buf;
    do {
        pick--;
        while (*(u8*)pSrc == 0) {
            pSrc++;
            i++;
        }
        (*(u8*)pSrc)--;
    } while (pick > 0);
    memcpy(D_800658DC, D_8009D73C[index], 0x200);
    D_80059508 = i;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076098);
void WorldMapFlushDrawAndCache(void) {
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    DrawSync(0);
    Vsync(0);
    FlushCache();
    ExitCriticalSection();
}
extern int D_80059488;
extern DISPENV D_8009BC9C;
extern DISPENV D_8009BC24;
extern DRAWENV D_8009BC40;
extern u16 D_8009BD10;
extern u16 D_8009BD14;
extern u16 D_8009BD18;
extern u16 D_8009BD1C;
extern u16 D_8009CD4C;
extern u16 D_8009CD50;
extern u_short g_C1ButtonState;
extern u_short g_C1ButtonStatePressedOnce;
extern u_short g_C1ButtonStateReleased;
extern u_short g_C2ButtonState;
extern u_short g_C2ButtonStatePressedOnce;
extern u_short g_C2ButtonStateReleased;
extern int ControllerPopState(void);
extern void GraphicsDrawPauseLetters(int x, int y);
extern void SoundMuteAllSpuChannels(void);
extern void SoundEnableAllSpuChannels(void);

void WorldMapWaitForButtonRelease(void) {
    RECT rect;
    DISPENV* pDispEnv;
    s32 savedValue;

    savedValue = D_80059488;
    DrawSync(0);
    Vsync(0);
    if (D_8009D7F0 == 0) {
        rect.x = 0;
        rect.y = 0xD8;
        rect.w = 0x140;
        rect.h = 0xD8;
        MoveImage(&rect, 0, 0);
    }
    pDispEnv = &D_8009BC9C;
    PutDispEnv(pDispEnv);
    PutDrawEnv((DRAWENV*)((u8*)pDispEnv - 0x5C));
    SoundMuteAllSpuChannels();
    do {
        DrawSync(0);
        Vsync(0);
        GraphicsDrawPauseLetters(0x88, 0x64);
        D_8009BD1C = 0;
        D_8009BD14 = 0;
        D_8009CD50 = 0;
        D_8009BD18 = 0;
        D_8009BD10 = 0;
        D_8009CD4C = 0;
        while (ControllerPopState() != 0) {
            D_8009CD4C |= g_C1ButtonState;
            D_8009CD50 |= g_C2ButtonState;
            D_8009BD10 |= g_C1ButtonStateReleased;
            D_8009BD14 |= g_C2ButtonStateReleased;
            D_8009BD18 |= g_C1ButtonStatePressedOnce;
            D_8009BD1C |= g_C2ButtonStatePressedOnce;
        }
    } while ((D_8009BD10 & 0x800) == 0);
    SoundEnableAllSpuChannels();
    DrawSync(0);
    Vsync(0);
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0, 0xD8);
    pDispEnv = &D_8009BC24;
    PutDispEnv((DISPENV*)((u8*)pDispEnv + D_8009D7F0 * 0x78));
    pDispEnv = (DISPENV*)((u8*)pDispEnv - 0x5C);
    PutDrawEnv((DRAWENV*)((u8*)pDispEnv + D_8009D7F0 * 0x78));
    D_80059488 = savedValue;
}

extern int ControllerGetType(int controllerIndex);

void WorldMapWaitForControllerConnected(void) {
    RECT rect;
    DISPENV* pDispEnv;
    s32 savedValue;

    savedValue = D_80059488;
    DrawSync(0);
    Vsync(0);
    if (D_8009D7F0 == 0) {
        rect.x = 0;
        rect.y = 0xD8;
        rect.w = 0x140;
        rect.h = 0xD8;
        MoveImage(&rect, 0, 0);
    }
    pDispEnv = &D_8009BC9C;
    PutDispEnv(pDispEnv);
    PutDrawEnv((DRAWENV*)((u8*)pDispEnv - 0x5C));
    SoundMuteAllSpuChannels();
    do {
        DrawSync(0);
        Vsync(0);
        GraphicsDrawPauseLetters(0x88, 0x64);
        D_8009BD1C = 0;
        D_8009BD14 = 0;
        D_8009CD50 = 0;
        D_8009BD18 = 0;
        D_8009BD10 = 0;
        D_8009CD4C = 0;
        while (ControllerPopState() != 0) {
            D_8009CD4C |= g_C1ButtonState;
            D_8009CD50 |= g_C2ButtonState;
            D_8009BD10 |= g_C1ButtonStateReleased;
            D_8009BD14 |= g_C2ButtonStateReleased;
            D_8009BD18 |= g_C1ButtonStatePressedOnce;
            D_8009BD1C |= g_C2ButtonStatePressedOnce;
        }
    } while (ControllerGetType(0) == 0);
    SoundEnableAllSpuChannels();
    DrawSync(0);
    Vsync(0);
    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xD8;
    MoveImage(&rect, 0, 0xD8);
    pDispEnv = &D_8009BC24;
    PutDispEnv((DISPENV*)((u8*)pDispEnv + D_8009D7F0 * 0x78));
    pDispEnv = (DISPENV*)((u8*)pDispEnv - 0x5C);
    PutDrawEnv((DRAWENV*)((u8*)pDispEnv + D_8009D7F0 * 0x78));
    D_80059488 = savedValue;
}
void WorldMapPlayBgmFromBuffer(u8* pSrc, s32 index) {
    u8* pBuffer;

    SoundStopAllAudioManagers();
    SoundDestroyAudioManager(D_80062528);
    memcpy(D_80062648, pSrc, ArchiveDecodeAlignedSize(index));
    D_80062528 = SoundCreateAudioManagerOnHeap(D_80062648);
    SoundPlayAudioManager(D_80062528, 0x7F, 0);
}
void WorldMapInterpolateQuadraticVector(s32 arg0, s16* pRow0, s16* pRow1, s16* pRow2, s32* pOut) {
    s32 s;
    s32 iz;
    s32 zv;
    s32 xs;

    s = 0x1000 - arg0;
    iz = ((s * s) << 3) >> 12;
    zv = ((s * arg0) >> 8) + 0x8000;
    xs = ((arg0 * arg0) << 3) >> 12;
    pOut[0] = pRow0[0] * iz + pRow1[0] * zv + pRow2[0] * xs;
    pOut[1] = pRow0[1] * iz + pRow1[1] * zv + pRow2[1] * xs;
    pOut[2] = pRow0[2] * iz + pRow1[2] * zv + pRow2[2] * xs;
}
void WorldMapDecompressMapData(void) {
    s32* pHeader;
    u8* pBase;

    pHeader = (s32*)D_8009C180;
    D_8009C180 = (s32)LZSSHeapDecompress(pHeader, 0);
    HeapFree(pHeader);
    pBase = (u8*)D_8009C180;
    pHeader = (s32*)pBase;
    D_8009CD48 = (s32)(pBase + pHeader[2]);
    D_8009D308 = (s32)(pBase + pHeader[3]);
    D_8009BD30 = (s32)(pBase + pHeader[4]);
    D_8009C7EC = (s32)(pBase + pHeader[5]);
    D_8009BCC0 = (s32)(pBase + pHeader[6]);
    D_8009D77C = (s32)(pBase + pHeader[8]);
    D_8009D7C8 = (s32)(pBase + pHeader[9]);
}

int WorldMapInitNop(void) {
    return 1;
}
