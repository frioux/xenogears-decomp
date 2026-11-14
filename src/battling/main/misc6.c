#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"
#include "system/memory.h"

// .sdata/.bss of the overlay (supplied by splat's asm segments)
extern u8* D_800926C8;
extern u8* D_800926BC;
extern u8 D_800928A0;
extern u8 D_80092708;
extern u8 D_800926F0;
extern u8 D_800926F4;
extern u8 D_800926F8;
extern u8 D_8009273C;
extern u8 D_80092740;
extern s32 D_80092744;
extern s32 D_800912DC;
extern s32 D_80059488;
extern s16 D_800926E8;
extern s16 D_800926EC;
extern u8 D_8009791B;
extern u8 D_80099037;
extern u8 D_800954E0[];
extern s32 D_800928E8;

void func_8007E964(s32 c);
u8* func_8007E8AC(s32 c);
s32 func_8007EB6C(u8* pText);
extern s32 D_800926DC;

void func_8008EB4C(s32 arg);
void BattleAppendExtraQueueEntry(void);

extern u8* D_800926D4[];
extern u16 D_800926E0;
extern u16 D_800926E4;

// 0x20-byte entry of the D_80092874 table; func_8007EEE8 reads only the s16 at 0x4.
typedef struct {
    /* 0x00 */ u8 unk0[4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
} BattleFilterEntry; // Size: 0x20

// 0xC-byte record whose address func_8007EEE8 queues into D_800928EC.
typedef struct {
    /* 0x00 */ u8 unk0[0xC];
} BattleQueueRecord; // Size: 0xC

extern BattleFilterEntry* D_80092874;
extern s32 D_80091964[];
extern u8** D_800928EC;
extern s32 D_80092888;

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ u16 unk6;
    /* 0x8 */ u16 unk8;
    /* 0xA */ u16 unkA;
} BattleSlot; // Size: 0xC

typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u8 r;
    /* 0x05 */ u8 g;
    /* 0x06 */ u8 b;
    /* 0x07 */ u8 unk7;
    /* 0x08 */ s32 x;
    /* 0x0C */ s32 y;
    /* 0x10 */ SVECTOR v0;
    /* 0x18 */ SVECTOR v1;
} BattleQuad; // Size: 0x20

// An 8-byte, byte-aligned chunk: assigning one copies 8 bytes with lwl/lwr.
typedef struct {
    u8 unk[8];
} Bytes8;

extern BattleQuad D_80094818[100];
extern s32 D_800926B0;
extern s32 D_800926B4;

// 0x14-byte sprite record of the 7x7 table func_8007EFB4 allocates at D_8009270C.
typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u16 unk2;
    /* 0x04 */ u16 unk4;
    /* 0x06 */ u16 unk6;
    /* 0x08 */ u16 unk8;
    /* 0x0A */ u16 unkA;
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 unk12;
} BattleSprite; // Size: 0x14

extern BattleSprite* D_8009270C;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E020);

void func_8007E24C(void) {
    s32 i;

    for (i = 0; i < 0x21C; i++) {
        ((BattleSlot*)D_800926C8)[i].unk6 = 0;
        ((BattleSlot*)D_800926C8)[i].unk4 = 0;
        ((BattleSlot*)D_800926C8)[i].unk2 = 0;
        ((BattleSlot*)D_800926C8)[i].unk0 = 0;
    }
    D_8009791B = 0;
    D_80099037 = 0;
    for (i = 0; i < 0xFF; i++) {
        *(D_800926BC + i * 0xA + 6) = 0;
        *(u16*)(D_800926BC + i * 0xA + 4) = 0;
        *(u16*)(D_800926BC + i * 0xA + 2) = 0;
        *(u16*)(D_800926BC + i * 0xA + 0) = 0;
    }
}

void func_8007E2D8(void) {
    BattleQuad* pQuad = D_80094818;
    s32 i;

    for (i = 0; i < 100; i++) {
        pQuad[i].unk3 = 3;
        pQuad[i].unk7 = 0x40;
    }
    D_800926B0 = 0;
    D_800926B4 = 0;
}

void BattleQueueLineQuad(s32* pPos, s32* pPos2, u8* pColor) {
    BattleQuad* pQuad;

    if (D_800926B0 < 100) {
        pQuad = &D_80094818[D_800926B0];
        pQuad->v0.vx = pPos[0];
        pQuad->v0.vy = pPos[1];
        pQuad->v0.vz = pPos[2];
        pQuad->v1.vx = pPos2[0];
        pQuad->v1.vy = pPos2[1];
        pQuad->v1.vz = pPos2[2];
        pQuad->r = pColor[0];
        pQuad->g = pColor[1];
        pQuad->b = pColor[2];
        D_800926B0++;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E3CC);
/* best 88.67%: the 16-byte want/gte work block, the two Bytes8 copies into
   0x1F800018/0x1F800020, all six u16 translation subtractions, gte_ldv01 +
   RTPT, gte_stsxy01 into quad+8/quad+0xC, the SZ2 read and the 0x03000000 OT
   link are all exact (frame 0x10 too). Residual: retail keeps TWO induction
   bases (t0 = &D_80094818 and a3 = t0+0xC, both +0x20 per iteration) and so
   reaches the copy with 4(a3)/0xC(a3); ours folds them into one base and uses
   0x10/0x18 off it. Retail also builds &D_80094818 (lui/addiu) before the
   scratchpad loads, ours after. Tried: an explicit pPack = quad+0xC pointer for
   the copies and the stsxy01 (worse, 88.64 - gcc re-associates back onto the
   quad base), a literal 0x3000000 in the link store with a long sz[4] local
   (89.17 but frame 0x18), declaring the copies through BattleQuad->v0/v1.
   Note gte_stsz1/gte_stsz2 were added to include/psyq/inline_c.h for this.
void func_8007E3CC(u32* pOt) {
    u8* pScratch;
    BattleQuad* pQuad;
    u32 dx;
    u32 dy;
    u32 dz;
    u32 link;
    u32 addr;
    u32 tag;
    u32* pLink;
    long sz[2];
    s32 i;

    pScratch = (u8*)0x1F800000;
    tag = 0x3000000;
    dx = *(u32*)(pScratch + 0x0);
    dy = *(u32*)(pScratch + 0x4);
    dz = *(u32*)(pScratch + 0x8);
    for (i = 0; i < D_800926B4; i++) {
        pQuad = &D_80094818[i];
        *(Bytes8*)(pScratch + 0x18) = *(Bytes8*)((u8*)pQuad + 0x10);
        *(Bytes8*)(pScratch + 0x20) = *(Bytes8*)((u8*)pQuad + 0x18);
        ((u16*)(pScratch + 0x18))[0] -= dx;
        ((u16*)(pScratch + 0x18))[1] -= dy;
        ((u16*)(pScratch + 0x18))[2] -= dz;
        ((u16*)(pScratch + 0x20))[0] -= dx;
        ((u16*)(pScratch + 0x20))[1] -= dy;
        ((u16*)(pScratch + 0x20))[2] -= dz;
        gte_ldv01(pScratch + 0x18, pScratch + 0x20);
        gte_rtpt();
        gte_stsxy01((u8*)pQuad + 0x8, (u8*)pQuad + 0xC);
        gte_stsz2(sz);
        pLink = &pOt[sz[0] >> 4];
        link = *pLink;
        addr = (u32)pQuad & 0xFFFFFF;
        *pLink = addr;
        *(u32*)addr = link | tag;
    }
}
 */

void func_8007E528(s32 mode) {
    if (D_80092708 == 0 && mode == 0xA) {
        func_8008EB4C(0x24);
    }
    D_80092708 = mode;
}

void func_8007E574(void* pOt) {
    if (D_80092708 != 0) {
        if (D_800928E8 & 2) {
            AddPrim(pOt, &D_800954E0[D_800928A0 * 0x1C]);
            AddPrim(pOt, &D_800954E0[D_800928A0 * 0x1C - 8]);
        }
        D_80092708--;
    }
}

s32 func_8007E624(void) {
    return D_800926DC;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E634);

void func_8007E894(s16 a, s16 b) {
    D_800926E8 = a;
    D_800926EC = b;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E8AC);

void func_8007E954(s32 value) {
    D_800912DC = value;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007E964);
/* best 84.08%. Body shape is right: the 0x28-byte POLY_FT4 glyph quad, the
   UV slot widths (u1 = u0 + (glyph[2]|3), v2/v3 = v1 + glyph[3] + 1), the
   colour word and the x advance all reproduce. Four residual differences, all
   in gcc's hands:
   1. prologue: retail loads D_800926DC into v1 BEFORE addiu sp and multiplies
      the packet index with v0; ours loads it into a0, multiplies into v1 and
      therefore needs an extra `move a0,s1` before the jal (c must survive the
      call in s1 either way). Tried: an s32 i local, D_800926DC read twice, a
      pBase local, (POLY_FT4*)D_800926D4[idx] + i and + i*0x28 - all identical
      or worse.
   2. the lbu of glyph[2] is hoisted above the first sw in retail and not here
      (retail also keeps (glyph[2]|3) in s1 across the stores).
   3. the tail wants lhu for D_800926E8 and keeps x+2 in v0; ours emits lh and
      a fresh register even with an explicit (u16) cast.
   4. no extra stack slot: the 0x20 frame with s0/s1/ra is exact.
   Also proved here: func_8007E8AC takes an s32, not a u8 - retail does no andi
   on a0 in its own body or at these call sites, and func_8007EB6C still matches
   100.00% with the s32 prototype.
void func_8007E964(s32 c) {
    POLY_FT4* pPkt;
    POLY_FT4* pBase;
    u8* pGlyph;
    s16 x;
    s16 y;
    s32 gw;
    s32 dx;

    if (D_800926DC >= 0x65) {
        return;
    }
    pBase = (POLY_FT4*)D_800926D4[D_800928A0];
    pPkt = pBase + D_800926DC;
    pGlyph = func_8007E8AC(c);
    if (pGlyph == 0) {
        return;
    }
    if (c != 0x28) {
        y = D_800926EC;
        x = D_800926E8;
        *(u_long*)&pPkt->x0 = x | (y << 16);
        gw = pGlyph[2] | 3;
        dx = (gw * D_800912DC) >> 8;
        *(u_long*)&pPkt->x1 = (x + dx) | (y << 16);
        *(u_long*)&pPkt->x2 = x | ((y + pGlyph[3]) << 16);
        *(u_long*)&pPkt->x3 = (x + dx) | ((y + pGlyph[3]) << 16);
        *(u_short*)&pPkt->u0 = pGlyph[0] | (pGlyph[1] << 8);
        *(u_short*)&pPkt->u1 = (pGlyph[0] + gw) | (pGlyph[1] << 8);
        *(u_short*)&pPkt->u2 = pGlyph[0] | ((pGlyph[1] + (pGlyph[3] + 1)) << 8);
        *(u_short*)&pPkt->u3 = (pGlyph[0] + gw) | ((pGlyph[1] + (pGlyph[3] + 1)) << 8);
        pPkt->tpage = D_800926E0;
        pPkt->clut = D_800926E4;
        setlen(pPkt, 9);
        *(u_long*)&pPkt->r0 = D_800926F0 | (D_800926F4 << 8) | (D_800926F8 << 16) | 0x2C000000;
        D_800926DC++;
    }
    D_800926E8 = (u16)D_800926E8 + 2 + ((pGlyph[2] * D_800912DC) >> 8);
}
 */

s32 func_8007EB6C(u8* pText) {
    s32 width = 0;

    while (*pText != 0) {
        u8* pGlyph = func_8007E8AC(*pText);

        pText++;
        width += 2 + ((pGlyph[2] * D_800912DC) >> 8);
    }
    return width;
}

void func_8007EBE0(u8* pText) {
    s32 y = D_800926E8;

    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

void BattleByteRegTestReg(u8* pText) {
    s32 y = D_800926E8;

    D_800926E8 -= func_8007EB6C(pText) / 2;
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

void func_8007ECF0(u8* pText) {
    s32 y = D_800926E8;

    D_800926E8 -= func_8007EB6C(pText);
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

void func_8007ED84(u8* pText, s32 dy) {
    s32 tmp[2]; // unused: reproduces the original's 8-byte frame slot
    s32 y = D_800926E8;

    D_800926E8 = y - dy;
    while (*pText != 0) {
        func_8007E964(*pText);
        pText++;
    }
    D_800926E8 = y;
    D_800926EC += 0x14;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EE08);
/* best 95.83%, one instruction short. The original materialises 0xFF twice per
   path: the else arm's li sits in the beqz delay slot and the then arm keeps its
   own li after the D_80059488 load. gcc 2.7.2 cse-hoists one QImode 0xFF to
   before the branch, reorg drops it in the delay slot, and both arms then read
   it, so the then arm loses its li. Everything else (load first, sb order
   F4/F8/F0, x*20 as sll/addu/sll, both branch targets) is exact.
   Tried and identical or worse: an s32 local for the then arm's 0xFF (two li,
   but cse still hoists the SImode one into the delay slot and leaves the else
   arm's li in place - 91.46%, the two li swapped); the same for the else arm
   (same bytes); chained `D_800926F8 = D_800926F4 = D_800926F0 = 0xFF;` in the
   else arm (correct store order, still 95.83%). The delay slot is already
   occupied by the hoisted li, so reorg never gets to steal the else arm's li.
void func_8007EE08(s32 arg) {
    if (arg != 0) {
        D_800926F4 = 0xFF;
        D_800926F8 = 0;
        D_800926F0 = D_80059488 * 20;
    } else {
        D_800926F0 = 0xFF;
        D_800926F4 = 0xFF;
        D_800926F8 = 0xFF;
    }
}
 */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EE68);
/* best 72.66%: the original loads D_80059488 TWICE into two registers (v1 for
   0xF0, a0 for 0xF4) and subtracts with li 0xFF + subu; gcc CSEs the two
   identical loads of the one extern and then strength-reduces 0xFF - (x * 20)
   to nor, since only the stored byte matters. The original subu means its two
   MEMs were two different symbols that both resolve to 0x80059488 (aliases),
   and splat can only name one of them, so the second name cannot be spelled
   without a hand-edited linker/undefined_syms_auto.battling.txt (generated).
   A `*(volatile s32*)&D_80059488` second read does produce two lw, but the
   volatile one cannot be hoisted to the block head and the nor is still there:
   72.03%, no better.
void func_8007EE68(s32 arg) {
    if (arg != 0) {
        D_800926F8 = 0xFF;
        D_800926F0 = 0xFF - D_80059488 * 20;
        D_800926F4 = 0xFF - D_80059488 * 20;
    } else {
        D_800926F0 = 0xFF;
        D_800926F4 = 0xFF;
        D_800926F8 = 0xFF;
    }
}
 */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007EEE8);
/* best 90.45%: every instruction and relocation matches except which of the two
   loop induction pointers gets a0 and which gets a1 (the original keeps the
   0x20-byte table pointer in a1 and the 0xC-byte record pointer in a0) and, as
   a consequence, where the HeapAlloc result is copied into a3 in the loop
   preheader. Frame (0x20, ra at 0x18), the s0/s1 split, the
   g_GameState+0x1930 lhu, the filter compare and the D_80092888
   read-modify-write are exact. Tried: index and pointer forms for both
   induction variables and every mixture, i*3 over an s32 array, a nested if
   with continue, split increment, six declaration orders, and -G0 / -O3 /
   gcc 2.6.0 / gcc 2.7.2-cdk (90.45 or worse under all). What is left is a
   global.c allocator tie-break between two allocnos with identical ref counts
   and live ranges.
void func_8007EEE8(s32 arg) {
    s32 i;
    s32 limit;
    u8** pQueue;

    limit = g_GameState.unk1930[0];
    pQueue = HeapAlloc(0xC4, 1);
    D_800928EC = pQueue;
    D_80092888 = 0;
    for (i = 0; i < 0x31; i++) {
        if (arg == 0 || D_80092874[i].unk4 <= limit) {
            pQueue[D_80092888++] = (u8*)&D_80091964[i];
        }
    }
    if (arg != 0) {
        BattleAppendExtraQueueEntry();
    }
}
 */

void func_8007EFB4(void) {
    s32 tmp[12]; // unused: reproduces the original's 0x48 frame
    BattleSprite* pSprite;
    s32 count;
    s32 row;
    s32 col;
    s16 y;
    s16 x;

    D_8009270C = HeapAlloc(0x3D4, 0);
    pSprite = D_8009270C;
    count = 0x1FF;
    for (row = 0; row < 7; row++) {
        y = 0x1A0 + row * 0x20;
        for (col = 0; col < 7; col++) {
            x = col * 0x40;
            pSprite->unk0 = 0x200;
            pSprite->unk2 = count--;
            pSprite->unk4 = 0x80;
            pSprite->unk6 = 1;
            pSprite->unk8 = y;
            pSprite->unkA = x;
            pSprite->unkC = 0x1E;
            pSprite->unkE = 0x40;
            pSprite++;
        }
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007F05C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc6", func_8007F258);

void func_8007F834(void) {
    D_80092740 = 0;
    D_8009273C = 0;
    D_80092744 = 0;
}
