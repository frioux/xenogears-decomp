#include "common.h"
#include "main/game.h"
#include "psyq/libapi.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"

extern int Vsync(int mode);
extern void func_80039CC4(void);
extern void func_800399D4(s32* pFile);
extern void* func_80039850(void* pFile);
extern void func_80039A80(s32* pFile, s32 volume, s32 frames);
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
    /* 0x0 */ s16 vx;
    /* 0x2 */ s16 vy;
    /* 0x4 */ s16 vz;
    /* 0x6 */ u16 pad6;
} Vector16; // Size: 0x8

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
extern s32 D_8009BCC4;
extern s32 D_8009BE40;
extern s32 D_8009D64C;
extern s32 D_8009D80C;
extern s32* D_80062528;
extern s32 D_8009CD64;
extern u8 D_8009A250[];
extern u8 D_80062648[];

void func_8007474C(void) {
    HeapFree(D_8009BE18);
    HeapFree(D_8009BE14);
    HeapFree(D_8009D30C);
}

void func_80074794(s16 arg0, s32* pVec) {
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80074E58);

/* 89.84%. func_80075030 below is the same routine with D_8009D77C->D_8009D7C8,
   D_8009CC9C->D_8009CD64, D_8009D780->D_8009D7D0 and D_8009A1E8->D_8009A250,
   so matching one matches both. The entry pointer has to be a real `pEntry++`
   walker (that is what makes retail anchor the s16 fields at a0+0xA with the
   -6/-2 displacements) and the loop bound is the global re-read after the
   HeapAlloc. Remaining: retail emits the desc walker's addiu a3,a3,16 right
   after sw a3,-6(v1) and ends the body
   `addiu v1,v1,12 / slt / bnez / (delay) addiu a0,a0,12`, while gcc's IV
   reduction puts a3's increment last (delay slot) and a0's before the slt.
   A real source walker (pDesc; pDesc += 0x10) reproduces the a3 placement
   exactly (89.07%) but flips the dst walker and the counter between a0/a1
   (retail: a0 = dst from HeapAlloc's result, a1 = counter). Also tried:
   (i << 4) for the desc index, *((s32*)D_8009D77C + i + 1), ++i, three
   declaration orders, s32/u32 counter, count re-read into a local.
void func_80074E58(void) {
    WorldMapEntry* pEntry;
    s32 count;
    s32 i;

    count = ((s32*)D_8009D77C)[0];
    D_8009CC9C = count;
    pEntry = (WorldMapEntry*)HeapAlloc(count * 0xC, 0);
    D_8009D780 = (u8*)pEntry;
    for (i = 0; i < D_8009CC9C; i++) {
        pEntry->unk0 = (s32)D_8009D77C + ((s32*)D_8009D77C)[i + 1];
        pEntry->unk4 = (s32)(D_8009A1E8 + i * 0x10);
        pEntry->unk8 = 0;
        pEntry->unkA = 1;
        pEntry++;
    }
}
*/

void func_80074F04(void) {
    HeapFree(D_8009D780);
}

void func_80074F2C(void) {
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075030);

/* Same routine as func_80074E58 above with the D_8009D7C8 / D_8009CD64 /
   D_8009D7D0 / D_8009A250 symbols swapped in; see that attempt (89.84%). */


void func_800750DC(void) {
    HeapFree(D_8009D7D0);
}

void func_80075104(void) {
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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_8007528C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075460);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_8007565C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_800758C0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075B58);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075D4C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80075E7C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076098);
void func_800762FC(void) {
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    DrawSync(0);
    Vsync(0);
    FlushCache();
    ExitCriticalSection();
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_8007634C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc2", func_80076594);
void func_800767D4(u8* pSrc, s32 index) {
    u8* pBuffer;

    func_80039CC4();
    func_800399D4(D_80062528);
    memcpy(D_80062648, pSrc, ArchiveDecodeAlignedSize(index));
    D_80062528 = func_80039850(D_80062648);
    func_80039A80(D_80062528, 0x7F, 0);
}
void func_80076858(s32 arg0, s16* pRow0, s16* pRow1, s16* pRow2, s32* pOut) {
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
void func_80076954(void) {
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

int func_80076A14(void) {
    return 1;
}
