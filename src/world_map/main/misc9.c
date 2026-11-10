#include "common.h"
#include "system/memory.h"
#include "psyq/libgpu.h"

extern u_char* D_8009BE24;
extern s32 D_8009C610;
extern u16 D_8009B674[];
extern u_char* D_8009C7EC;
extern u_char* D_8009CEB4;
extern u_char* D_8009D150;
extern u16 D_8009D478[];
extern u_char* D_8009D7E8;
extern u_char* D_8009D7EC;
extern u_char* D_8009D7F8;
extern u_char* D_8009D7FC;
extern u_char* D_8009C620;
extern s16 D_8009AFA0[];
extern u16 D_8009B688[];

void func_800848B4(s32 arg0, s32 arg1);
s32 func_800879E0(s32 arg0);

// 0x80-byte slot in the D_8009BE24 table
typedef struct {
    /* 0x00 */ u8 pad0[0x50];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ u8 pad58[0x28];
} WorldMapSlot; // Size: 0x80

typedef struct {
    /* 0x0 */ s32 offset;
    /* 0x4 */ s32 unk4;
} WorldMapTableEntry; // Size: 0x8

void func_80085F58(void) {
    WorldMapTableEntry* pEntry;
    u_char* pBase;
    s32 i;

    pBase = D_8009C7EC;
    pEntry = (WorldMapTableEntry*)pBase;
    for (i = 0; i < 0x100; i++, pEntry++) {
        if (pEntry->offset != 0) {
            pEntry->offset += (s32)pBase;
        }
    }
    for (i = 0; i < 0x10; i++) {
        D_8009D478[i] = GetClut(0xF0, i + 0x1F0);
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80085FE0);
void func_80086124(void) {
    HeapFree(D_8009D7EC);
    HeapFree(D_8009D7E8);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_8008615C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_800863E0);
void func_80086568(void) {
    HeapFree(D_8009CEB4);
    HeapFree(D_8009D150);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_800865A0);
void func_800866C8(void) {
    HeapFree(D_8009D7FC);
    HeapFree(D_8009D7F8);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80086700);
// Best: 92.29%. Verified shape: 0x50 iterations, s16 deltas at 0/4 of a 0x8-byte
// entry added to the two s32 angles at 0/8 of a 0x10-byte entry, each wrapped by
// (+-0x2000000). Retail keeps one destination pointer (`lw/sw 8(a2)`); gcc gives
// the y field its own induction pointer and shifts the three wrap constants up a
// register. Struct access, raw *(s32*)(pDst+8), pointer and index forms all agree.
/*
void func_80086700(void) {
    u8* pDst = D_8009D150;
    s16* pSrc = (s16*)D_8009CEB4;
    s32 i;

    for (i = 0; i < 0x50; i++) {
        s32 x = *(s32*)pDst + pSrc[0];
        s32 y = *(s32*)(pDst + 8) + pSrc[2];

        if (x > 0x1FFFFFF) {
            x -= 0x2000000;
        }
        if (x < 0) {
            x += 0x2000000;
        }
        if (y > 0x1FFFFFF) {
            y -= 0x2000000;
        }
        if (y < 0) {
            y += 0x2000000;
        }
        *(s32*)pDst = x;
        *(s32*)(pDst + 8) = y;
        pDst += 0x10;
        pSrc += 4;
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80086798);
s32 func_80087710(s32 index) {
    WorldMapSlot* pSlot = (WorldMapSlot*)(D_8009BE24 + index * 0x80);

    pSlot->unk50 = 0;
    pSlot->unk54 = 8;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087734);
// Best: 63.72%. Two scratchpad vectors (0x1F8000A0 / 0x1F8000A8) built from
// D_8009C620+0x40A, +0x4B2 and the slot angle (fields are unsigned: retail uses
// lhu), then RotMatrix(v, pSrc+0x410) and RotMatrix(v, pSrc+0x4B8). Retail loads
// D_8009BE24 first and D_8009C620 only after the slot angle update; gcc hoists the
// D_8009C620 load above the frame and stores 0xA8/0xA0/0xA2/0xAA/0xAC/0xA4 in an
// order that neither the source order nor a struct walk reproduces.
/*
s32 func_80087734(s32 index) {
    WorldMapSlot* pSlot = (WorldMapSlot*)(D_8009BE24 + index * 0x80);
    u8* pSrc;

    pSlot->unk50 = (pSlot->unk50 + pSlot->unk54) & 0xFFF;
    pSrc = D_8009C620;
    *(u16*)0x1F8000A8 = 0;
    *(u16*)0x1F8000A0 = 0;
    *(u16*)0x1F8000A2 = *(u16*)(pSrc + 0x40A);
    *(u16*)0x1F8000AA = *(u16*)(pSrc + 0x4B2);
    *(u16*)0x1F8000AC = ((u16*)&pSlot->unk50)[0];
    *(u16*)0x1F8000A4 = ((u16*)&pSlot->unk50)[0];
    RotMatrix((SVECTOR*)0x1F8000A0, (MATRIX*)(pSrc + 0x410));
    RotMatrix((SVECTOR*)0x1F8000A8, (MATRIX*)(pSrc + 0x4B8));
    return 1;
}
*/
s32 func_800877E0(s32 index) {
    WorldMapSlot* pSlot = (WorldMapSlot*)(D_8009BE24 + index * 0x80);

    pSlot->unk50 = 0;
    pSlot->unk54 = 8;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087804);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087904);
// Best: 92.44%. Loop body, GetTPage(0, abr, 0x1A0, 0xA0), the |2 on the byte at
// p+7 and the trailing memcpy(pDst[0x4C], pDst[0x48], count*0x28) are exact. Only
// the prologue differs: retail leaves `p = pSrc + 7` out of the entry block (blez,
// `sw s0` in its delay slot, then `addiu s0,a1,7`), gcc sets s0 up before `sw ra`
// and steals `sw ra` into the delay slot instead.
/*
void func_80087904(u8* pDst, u8* pSrc, s32 count, s32 abr) {
    u8* p;
    s32 i;

    for (i = 0, p = pSrc + 7; i < count; i++, p += 0x28) {
        *(u16*)(p + 0xF) = GetTPage(0, abr, 0x1A0, 0xA0);
        *p |= 2;
    }
    memcpy(*(u8**)(pDst + 0x4C), *(u8**)(pDst + 0x48), count * 0x28);
}
*/
s32 func_800879A8(s32 index) {
    WorldMapSlot* pSlot = (WorldMapSlot*)(D_8009BE24 + index * 0x80);

    pSlot->unk50 = 0;
    pSlot->unk54 = 0x10;
    func_800879E0(index);
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_800879E0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087A8C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087B84);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087C6C);
s32 func_80087F60(void) {
    s32 value = D_8009B674[D_8009C610];

    func_800848B4(value, value - 4);
    func_800848B4(value, value - 3);
    func_800848B4(value, value - 2);
    func_800848B4(value, value - 1);
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087FD0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80088570);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_8008868C);
// Best: 76.81%. Two-pointer walk over the interleaved s16 pairs of D_8009AFA0
// (pX[0], pX[1] with pX += 2) offset by base = D_8009B688[D_8009C610], ending at
// a -1 sentinel. Retail materialises the sentinel as `li v0,-1` inside the loop
// and as `li a0,-1` before it, i.e. loop.c did not hoist the constant; every
// decl order / do-while / pointer-vs-index shape tried so far gets it hoisted
// into a callee-saved s3 (frame 0x28 instead of 0x20).
/*
s32 func_8008868C(void) {
    s32 base = D_8009B688[D_8009C610];
    s16* pX = D_8009AFA0;
    s16* pY = D_8009AFA0 + 1;

    if (*pX != -1) {
        do {
            s32 vx = *pX;
            s32 vy = *pY;

            func_800848B4(base + vx, base + vy);
            pX += 2;
            pY += 2;
        } while (*pX != -1);
    }
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80088720);
