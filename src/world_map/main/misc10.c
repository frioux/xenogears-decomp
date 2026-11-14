#include "common.h"
#include "main/game.h"

extern void* D_8009BCC0;
extern void* D_8009BDF4;
extern s32 D_8009C610;
extern u16 D_8009B69C[];
extern u16 D_8009B6B0[];
extern void* D_8009BE1C;
extern void* D_8009BE20;
extern u8* D_8009BE24;
extern s16 D_8009AFDC[];
extern u8* D_8009C620;

extern void* D_8009CD34;

extern void HeapFree(void* pMem);
extern void* SpriteAllocFromAnimFile(void* pArg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5);
extern void SpritePlayAnimation(void* pSprite, s32 arg1);
extern void WorldMapSetSpriteSetParent(s32 arg0, s32 arg1);
extern void SpriteSetScale(void* pSprite, s16 scale);
extern void func_8008BFD4(s32 arg0, u32* pVec, s32 arg2, s32 arg3);
extern s32 WorldMapGetTilePlaneHeight(s32 x, s32 z);

typedef struct {
    /* 0x0 */ s32 w[4];
} Block16; // Size: 0x10

typedef struct {
    /* 0x00 */ undefined32 unk00;
    /* 0x04 */ u16 unk04;
    /* 0x06 */ u16 unk06;
    /* 0x08 */ undefined32 unk08[6]; // 0x08..0x1F
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ s16 unk24;
    /* 0x26 */ undefined16 unk26;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ undefined32 unk34;
    /* 0x38 */ s32 unk38;
    /* 0x3C */ u32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ undefined32 unk44;
    /* 0x48 */ s16 unk48;
    /* 0x4A */ s16 unk4A;
    /* 0x4C */ void* unk4C;
    /* 0x50 */ s32 unk50;
    /* 0x54 */ void* unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ undefined32 unk60[8]; // 0x60..0x7F
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x0 */ s32 unk00;
    /* 0x4 */ s32 unk04;
    /* 0x8 */ s32 unk08;
    /* 0xC */ s32 unk0C;
} WorldMapVec; // Size: 0x10

typedef struct {
    /* 0x00 */ WorldMapVec vec;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 pad12;
} WorldMapTrail; // Size: 0x14

typedef struct {
    /* 0x0 */ s8 r;
    /* 0x1 */ s8 g;
    /* 0x2 */ s8 b;
    /* 0x3 */ s8 a;
} WorldMapColorStep; // Size: 0x4

typedef union {
    /* 0x0 */ s32 flags;
    /* 0x0 */ u16 count;
    /* 0x2 */ s16 active;
} WorldMapTrailFlags; // Size: 0x4

typedef struct {
    /* 0x00 */ s16 unk00;
    /* 0x02 */ s16 unk02;
    /* 0x04 */ WorldMapTrailFlags unk04;
    /* 0x08 */ s32 unk08;
    /* 0x0C */ s32 unk0C;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ undefined32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ undefined32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ undefined32 unk34;
    /* 0x38 */ u16 unk38;
    /* 0x3A */ u16 unk3A;
    /* 0x3C */ u16 unk3C;
    /* 0x3E */ u16 unk3E;
    /* 0x40 */ u32 unk40;
    /* 0x44 */ WorldMapColorStep colorStep;
    /* 0x48 */ undefined32 unk48;
} WorldMapTrailEntry; // Size: 0x4C

typedef struct {
    /* 0x00 */ undefined32 unk00;
    /* 0x04 */ undefined32 unk04;
    /* 0x08 */ undefined16 unk08[2];
    /* 0x0A */ u16 unk0A;
    /* 0x0C */ undefined32 unk0C[5];
    /* 0x20 */ undefined32 unk20[13];
} WorldMapSprite; // Size: 0x54

extern WorldMapVec D_8009D55C;
extern WorldMapTrail D_8009CEC4[];
extern u16 D_8009D52C;
extern u16 D_8009D154;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088B40);
/* 73.64%: structure confirmed (loop over D_8009AFDC calling WorldMapSetSpriteSetParent(0x45,id),
   sh 0 at pDst+0x20, the 0x99 branch, then the >>12 tail shared with
   func_80088D00). Two walls, both seen elsewhere in this unit: (a) the tail
   folds +0x16A4 into the store displacements (0x16AC/0x16B0/0x16B4) where
   retail does addiu a1,a1,0x16A4 once and stores at 8/0xC/0x10; (b) both id
   loops want an `s32 id` instead of a bare `*pIds` (see func_80088C90). The
   0x40 frame vs retail's 0x20 comes from (b).
s32 func_80088B40(s32 arg0) {
    u8* pDst;
    u8* pBase;
    s32* pSrc;
    s16* pIds;
    s32 ret;

    pIds = D_8009AFDC;
    while (*pIds != -1) {
        WorldMapSetSpriteSetParent(0x45, *pIds);
        pIds++;
    }
    pDst = D_8009BE24 + (arg0 << 7);
    *(s16*)(pDst + 0x20) = 0;
    pBase = D_8009C620;
    if (g_GameState.unk1930[0] == 0x99) {
        *(s32*)(pDst + 0x28) = 0x2000000;
        *(s32*)(pDst + 0x30) = 0x4120000;
        *(s32*)(pDst + 0x2C) = 0;
        ret = 1;
    } else {
        *(s16*)(pBase + 0x16A4) = 1;
        pIds = D_8009AFDC;
        while (*pIds != -1) {
            *(s16*)(pBase + *pIds * 0x54) = 1;
            pIds++;
        }
        ret = 3;
    }
    pSrc = (s32*)(pBase + 0x16A4);
    pSrc[2] = *(s32*)(pDst + 0x28) >> 12;
    pSrc[3] = *(s32*)(pDst + 0x2C) >> 12;
    pSrc[4] = *(s32*)(pDst + 0x30) >> 12;
    return ret;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088C90);
/* 85.32% (was 52.89): the load mode is the whole game. `s32 id` read once in
   the while condition gives retail's single `lh` and drops the 16-byte phantom
   frame; an `s16 id`, or a bare `*pIds * 0x54` with or without an `(s32)` cast,
   makes gcc load HImode (lhu) and sign-extend with sll/sra, which also costs a
   register (t0 where retail uses a3) and a 16-byte frame. Still missing:
   retail loads D_8009C620 before D_8009AFDC and re-loads `lh v0,0(a0)` after
   the branch, while mine reuses the tested value (108 vs 112 bytes).
   Reading the array twice in the body (`id = *pIds; pIds++; use id`) is the
   shape that produces retail's extra `lh v0,0(a0)` but it also makes gcc pick
   the unsigned HImode load (52.89%).
s32 func_80088C90(void) {
    u8* pBase;
    s16* pIds;
    s32 id;

    pBase = D_8009C620;
    *(s16*)(pBase + 0x16A4) = 1;
    pIds = D_8009AFDC;
    while ((id = *pIds) != -1) {
        *(s16*)(pBase + id * 0x54) = 1;
        pIds++;
    }
    return 3;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088D00);
/* 84.68%: with pSrc computed before the if (the only shape that keeps the
   D_8009C620 load above the branch, as retail does) gcc folds +0x16A4 into the
   three store displacements (0x16AC/0x16B0/0x16B4) instead of retail's
   addiu a1,a1,0x16A4 in the bne delay slot; the compare also lands in
   v0/v1 swapped. Needs a shape where the base+0x16A4 pointer is materialised.
s32 func_80088D00(s32 arg0) {
    u8* pDst;
    s32* pSrc;

    pDst = D_8009BE24 + (arg0 << 7);
    pSrc = (s32*)(D_8009C620 + 0x16A4);
    if (g_GameState.unk1930[0] == 0x99) {
        pSrc[2] = *(s32*)(pDst + 0x28) >> 12;
        pSrc[3] = *(s32*)(pDst + 0x2C) >> 12;
        pSrc[4] = *(s32*)(pDst + 0x30) >> 12;
    }
    return 1;
}
*/
/* 84.68%: retail loads the D_8009C620 pointer before the branch and keeps
   addiu a1,a1,0x16A4 in the bne delay slot, and compares with the id in v1
   against li v0,0x99; gcc 2.7.2 sinks the global load below the branch.
s32 func_80088D00(s32 arg0) {
    u8* pDst;
    u8* pBase;
    s32* pSrc;

    pDst = D_8009BE24 + (arg0 << 7);
    pBase = D_8009C620;
    if (g_GameState.unk1930[0] == 0x99) {
        pSrc = (s32*)(pBase + 0x16A4);
        pSrc[2] = *(s32*)(pDst + 0x28) >> 12;
        pSrc[3] = *(s32*)(pDst + 0x2C) >> 12;
        pSrc[4] = *(s32*)(pDst + 0x30) >> 12;
    }
    return 1;
}
*/

s32 WorldMapSetActorPosFromSpriteSetWithHeightOffset(s32 arg0) {
    u8* pDst;
    u8* pSrc;

    pDst = D_8009BE24 + (arg0 << 7);
    pSrc = D_8009C620 + D_8009B69C[D_8009C610] * 0x54;
    *(s32*)(pDst + 0x28) = *(s32*)(pSrc + 0x8) << 12;
    *(s32*)(pDst + 0x2C) = (*(s32*)(pSrc + 0xC) << 12) + 0x40000;
    *(s32*)(pDst + 0x30) = *(s32*)(pSrc + 0x10) << 12;
    return 1;
}

s32 WorldMapQueueZoneEntryHeight60(s32 arg0) {
    func_8008BFD4(arg0, (u32*)(D_8009BE24 + (arg0 << 7) + 0x28), 0x68, 0x60);
    return 1;
}

s32 WorldMapSetActorPosFromSpriteSet75(s32 arg0) {
    u8* pDst;
    s32* pSrc;

    pDst = D_8009BE24 + (arg0 << 7);
    pSrc = (s32*)(D_8009C620 + 0x18A4);
    *(s32*)(pDst + 0x28) = pSrc[0] << 12;
    *(s32*)(pDst + 0x2C) = pSrc[1] << 12;
    *(s32*)(pDst + 0x30) = pSrc[2] << 12;
    return 1;
}

s32 WorldMapQueueZoneEntryHeight1A6(s32 arg0) {
    func_8008BFD4(arg0, (u32*)(D_8009BE24 + (arg0 << 7) + 0x28), 0x10C, 0x1A6);
    return 1;
}

s32 WorldMapSetActorPosFromSpriteSet(s32 arg0) {
    u8* pDst;
    u8* pSrc;

    pSrc = D_8009C620 + D_8009B6B0[D_8009C610] * 0x54;
    pDst = D_8009BE24 + (arg0 << 7);
    *(s32*)(pDst + 0x28) = *(s32*)(pSrc + 0x8) << 12;
    *(s32*)(pDst + 0x2C) = *(s32*)(pSrc + 0xC) << 12;
    *(s32*)(pDst + 0x30) = *(s32*)(pSrc + 0x10) << 12;
    return 1;
}

s32 WorldMapQueueZoneEntryHeight392(s32 arg0) {
    func_8008BFD4(arg0, (u32*)(D_8009BE24 + (arg0 << 7) + 0x28), 0xC9, 0x392);
    return 1;
}

s32 WorldMapReturnThreeA(void) {
    return 3;
}

s32 WorldMapReturnThreeB(void) {
    return 3;
}

void WorldMapInitSpriteGroupTables(void) {
    s32 i;
    u8* p;

    p = (u8*)D_8009BCC0;
    for (i = 0x1FF; i != -1; i--) {
        *(s32*)(p + 0x4) = 0;
        *(s16*)(p + 0xA) = 0;
        *(s16*)(p + 0x12) = 0;
        *(s16*)(p + 0x18) = 0;
        *(s16*)(p + 0x16) = 0;
        *(s16*)(p + 0x14) = 0;
        *(s16*)(p + 0x20) = 0;
        *(s16*)(p + 0x1E) = 0;
        *(s16*)(p + 0x1C) = 0;
        p += 0x54;
    }
    D_8009BDF4 = HeapAlloc(0x4C00, 0);
    p = (u8*)D_8009BDF4;
    for (i = 0xFF; i != -1; i--) {
        *(s16*)(p + 0x6) = 0;
        *(s16*)(p + 0x4) = 0;
        p += 0x4C;
    }
}

void WorldMapFreeSpriteGroupTable(void) {
    HeapFree(D_8009BDF4);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008901C);
/* 90.66%: every instruction matches except the address of D_8009BE1C, which
   retail keeps in s0 (lui/addiu once, used for the store across the second
   HeapAlloc and for the reload); gcc materialises it twice (lui $at for the
   store, then lui/lw for the reload) and reorg therefore fills the second jal's
   delay slot with li a1,1 instead of the store. An explicit
   `void** pSlot = &D_8009BE1C` compiles to the identical bytes; so does
   `D_8009BE1C = HeapAlloc(); pEntry = D_8009BE1C; D_8009BE20 = HeapAlloc();`
   (gcc then forwards the alloc result instead of reloading, 268 bytes either
   way). Only cse's choice between an address register and $at differs.
void func_8008901C(void) {
    u8* pEntry;
    s32* pSrc;
    s32* pDst;
    s32* pEnd;
    s32 i;

    D_8009BE1C = HeapAlloc(0x2800, 1);
    D_8009BE20 = HeapAlloc(0x2800, 1);
    pEntry = (u8*)D_8009BE1C;
    for (i = 0xFF; i != -1; i--) {
        *(u8*)(pEntry + 3) = 9;
        *(u8*)(pEntry + 7) = 0x2C;
        *(u16*)(pEntry + 0x16) = GetTPage(1, 1, 0x340, 0x100);
        *(u16*)(pEntry + 0xE) = GetClut(0x100, 0x1FF);
        *(u8*)(pEntry + 7) |= 2;
        pEntry += 0x28;
    }
    pSrc = (s32*)D_8009BE1C;
    pDst = (s32*)D_8009BE20;
    pEnd = pSrc + 0xA00;
    do {
        *(Block16*)pDst = *(Block16*)pSrc;
        pSrc += 4;
        pDst += 4;
    } while (pSrc != pEnd);
}
*/

void WorldMapFreePrimTemplateBuffers(void) {
    HeapFree(D_8009BE1C);
    HeapFree(D_8009BE20);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089160);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_800893E0);
/* 96.03%: every instruction matches except that retail holds the entry pointer
   in a3 and this shape holds it in v1, which also pulls the lhu of pEntry+0x10
   one slot up (a3 is still free in ours). The address must be computed twice,
   once per loop (writing it once gives 82.16%: gcc then keeps it in a register
   across the first loop and drops retail's second lui/lw D_8009BCC0 + *0x2A0).
void func_800893E0(s32 arg0, u8* arg1, u8* arg2) {
    u8* pBase;
    u8* pEntry;
    s32 i;
    s32 count;
    s32 w;
    u16 h;

    pBase = D_8009BCC0 + arg0 * 0x2A0;
    count = 0;
    for (i = 7; i != -1; i--) {
        if (*(u8*)(pBase + 0x4F) & 0x80) {
            count++;
            break;
        }
    }
    pEntry = D_8009BCC0 + arg0 * 0x2A0;
    for (i = 7; i != -1; i--) {
        if (count == 0) {
            *(u8*)(pEntry + 0x4F) |= 0x80;
            h = *(u16*)(pEntry + 0x10);
            w = *(s32*)(pEntry + 0x0);
            *(s16*)(pEntry + 0xA) = 0;
            *(s16*)(pEntry + 0x12) = h;
            *(s32*)(pEntry + 0x4) = w;
        }
        w = *(s32*)(arg1 + 0);
        *(s32*)(pEntry + 0x14) = 0;
        *(s32*)(pEntry + 0x2C) = w;
        w = *(s32*)(arg2 + 0);
        *(s32*)(pEntry + 0x1C) = w;
        h = *(u16*)(arg1 + 4);
        *(s16*)(pEntry + 0x18) = 0;
        *(s16*)(pEntry + 0x30) = h;
        h = *(u16*)(arg2 + 4);
        *(s16*)(pEntry + 0x20) = h;
        pEntry += 0x54;
    }
}
*/

void WorldMapDeactivateSpriteGroup(s32 arg0) {
    u8* p;
    s32 i;

    p = (u8*)D_8009BCC0 + arg0 * 0x2A0 + 0x4F;
    for (i = 7; i != -1; i--) {
        *p &= 0x7F;
        p += 0x54;
    }
}

void WorldMapClearGroupEntryFlag(s32 arg0) {
    s16* pEntry;
    s32 i;
    s32 j;

    pEntry = D_8009BDF4;
    for (i = 0xFF; i != -1; i--) {
        for (j = 0; j < 8; j++) {
            if (pEntry[0] == (arg0 << 3) + j && pEntry[3] != 0) {
                pEntry[2] = 0;
                break;
            }
        }
        pEntry += 0x26;
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089580);
/* 63.10%: struct layout recovered and the head/tests/arithmetic are right; two
   differences remain.
   (1) The count/active word at entry+0x04: retail loads it ONCE as a word and
   splits it with sra 16 (active, jump to loop end when zero) and sll 16/sra 16
   (count, blez to the detach path), so the source reads it as an s32 and the
   WorldMapTrailFlags union below reproduces that head exactly. Retail still
   decrements the count with a plain lhu/sh at +0 and zeroes both halves with a
   single sw in the detach path, which is why the union needs all three members.
   (2) sched2 ties: retail loads all six pos/vel words first, does the six adds,
   and sinks all six stores to the end of the green path; gcc here emits
   load/add/store per statement, and gcc also hoists the D_8009BCC0 load out of
   the loop whereas retail reloads it inside the out-of-line detach block.
   Tried: six separate += statements in retail's own add order, nested if/else vs
   continue, u16/s16 for the two halves (the unsigned form is what makes gcc emit
   the separate lhu+beqz instead of the word split).
   The colour step at entry+0x44 is a word in retail: r comes from lb +0, g and b
   from (word<<16)>>24 and (word<<8)>>24, i.e. the source truncates a loaded
   word rather than reading three s8 fields (three lb's was 46.61%).
void func_80089580(void) {
    WorldMapTrailEntry* pEntry;
    s32 i;
    s32 flags;
    s32 r;
    s32 g;
    s32 b;

    pEntry = (WorldMapTrailEntry*)D_8009BDF4;
    for (i = 0xFF; i != -1; i--, pEntry++) {
        flags = pEntry->unk04.flags;
        if ((flags >> 16) != 0) {
            if ((s16)flags > 0) {
                pEntry->unk08 += pEntry->unk18;
                pEntry->unk0C += pEntry->unk1C;
                pEntry->unk10 += pEntry->unk20;
                pEntry->unk18 += pEntry->unk28;
                pEntry->unk1C += pEntry->unk2C;
                pEntry->unk20 += pEntry->unk30;
                pEntry->unk04.count = pEntry->unk04.count - 1;
                pEntry->unk38 += pEntry->unk3C;
                pEntry->unk3A += pEntry->unk3E;
                r = (pEntry->unk40 & 0xFF) + pEntry->colorStep.r;
                g = ((pEntry->unk40 >> 8) & 0xFF) + pEntry->colorStep.g;
                b = ((pEntry->unk40 >> 16) & 0xFF) + pEntry->colorStep.b;
                if (r < 0) {
                    r = 0;
                }
                if (r >= 0x100) {
                    r = 0xFF;
                }
                if (g < 0) {
                    g = 0;
                }
                if (g >= 0x100) {
                    g = 0xFF;
                }
                if (b < 0) {
                    b = 0;
                }
                if (b >= 0x100) {
                    b = 0xFF;
                }
                pEntry->unk40 = (pEntry->unk40 & 0xFF000000) | (b << 16) | (g << 8) | r;
            } else {
                ((WorldMapSprite*)D_8009BCC0)[pEntry->unk00].unk0A--;
                pEntry->unk00 = 0;
                pEntry->unk04.flags = 0;
            }
        }
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089748);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089C78);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A2C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A52C);
/* 76.69%: only two insns are transposed (everything else is byte-identical).
   Retail emits sll v1,a0,0x7 at the top and defers the 0x40 stack argument's
   store (sw v0,0x14(sp)) to just before the jal; gcc puts the arg store first
   and then sinks the sll into the argument region, which needs v0 and is why
   the shift lands in v0 where retail has v1. Identical bytes for: the scaled
   index inline, as a named s32 local, pActor declared first or last, and the
   call written directly as the store's rhs.
s32 func_8008A52C(s32 arg0) {
    u8* pSprite;
    u8* pActor;

    pActor = D_8009BE24 + (arg0 << 7);
    pSprite = SpriteAllocFromAnimFile(D_8009CD34, 0x100, 0x1E0, 0x140, 0x100, 0x40);
    *(u8**)(pActor + 0x4C) = pSprite;
    SpritePlayAnimation(pSprite, 0);
    SpriteSetScale(*(u8**)(pActor + 0x4C), 0x1800);
    *(u32*)(*(u8**)(pActor + 0x4C) + 0x3C) &= ~4;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A5B8);
/* 90.48% (372 bytes, retail 372): identical instruction multiset, only the
   prologue rotates - retail computes (D_8009BE24 + arg0*0x80) into a2 before the
   SpriteAllocFromAnimFile arg setup and keeps li a2,0x1E0 for the jal delay slot;
   gcc shifts into v1, emits li a2 early and lets reorg steal the addu into the
   slot. Same wall as the func_8008B54C / func_8008BDD0 parks in misc11 ("the 0x40
   stack store lands just before the jal"). Tried: call written inline as the
   store rhs, a void* pSprite temp, (u8*)D_8009BE24 + arg0*0x80 spelling - all
   byte-identical at 90.48%. Everything from the SpritePlayAnimation jal on
   matches, including the 32-iteration 0x14-stride copy and the D_8009D55C tail.
s32 func_8008A5B8(s32 arg0) {
    WorldMapActor* pActor;
    WorldMapTrail* pTrail;
    void* pSprite;
    s32 i;

    pActor = (WorldMapActor*)((u8*)D_8009BE24 + arg0 * 0x80);
    pSprite = SpriteAllocFromAnimFile(D_8009CD34, 0x100, 0x1E0, 0x140, 0x100, 0x40);
    pActor->unk4C = pSprite;
    SpritePlayAnimation(pActor->unk4C, 0);
    SpriteSetScale(pActor->unk4C, 0x1800);
    *(u32*)((u8*)pActor->unk4C + 0x3C) &= ~4;
    pActor->unk28 = g_GameState.unk1820 << 12;
    pActor->unk30 = g_GameState.unk1822 << 12;
    pActor->unk2C = WorldMapGetTilePlaneHeight(pActor->unk28, pActor->unk30);
    {
        u16 unk1824 = g_GameState.unk1824;

        pActor->unk24 = 1;
        pActor->unk4A = 8;
        pActor->unk40 = 0;
        pActor->unk3C = 0;
        pActor->unk38 = 0;
        pActor->unk20 = 2;
        D_8009D154 = 0;
        pActor->unk48 = unk1824;
    }
    pTrail = D_8009CEC4;
    for (i = 0; i < 0x20; i++, pTrail++) {
        pTrail->vec = *(WorldMapVec*)&pActor->unk28;
        pTrail->unk10 = pActor->unk48;
    }
    D_8009D55C = *(WorldMapVec*)&pActor->unk28;
    D_8009D52C = pActor->unk48;
    return 1;
}
*/
