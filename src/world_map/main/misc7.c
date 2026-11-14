#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/memory.h"

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
} Vec3; // Size: 0xC

typedef struct {
    /* 0x0 */ s32 w0;
    /* 0x4 */ s32 w1;
    /* 0x8 */ s32 w2;
    /* 0xC */ s32 w3;
} Quad; // Size: 0x10

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[0x4];
    /* 0x28 */ Vec3 pos;
    /* 0x34 */ u8 unk34[0x1C];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ u8 unk5C[0x20];
    /* 0x7C */ s32 unk7C;
} WorldMapSlot; // Size: 0x80

typedef struct {
    /* 0x00 */ u8 unk0[0x8];
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 unk14[0x2C];
    /* 0x40 */ u8 *unk40;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ u8 *unk48;
    /* 0x4C */ u8 *unk4C;
    /* 0x50 */ u8 unk50[0x4];
} WorldMapEntry; // Size: 0x54

extern WorldMapSlot *D_8009BE24;
extern WorldMapEntry *D_8009C620;
extern void func_800816DC(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 abr);
extern u8 D_8009A758[];
// externs used by the parked attempts below
extern Vec3 D_8009C5AC;
extern MATRIX D_8009A180;
extern s32 D_8009C5B0;
extern s32 D_8009C5B4;
extern s16 D_1F8000A0[];
extern void func_80089160(s32, s16 *, s32);
extern u16 D_8009A6C0;
extern u16 D_8009A70C[];
extern s32 D_8009D144;
extern s32 D_8009BE0C;
extern s32 D_8009BE28;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 D_8009D55C;
extern s32 D_8009D560;
extern s32 D_8009D564;
extern void func_8007A06C(void *, s32, s32);
extern s16 D_8009BD38;
extern s16 D_8009BD3A;
extern s16 D_8009BD3C;
extern s32 D_8009D3F0;
extern void *D_8006259C;
extern s16 D_8006F94E;
extern s16 D_8006F950;
extern s16 D_8006F954;
extern s32 D_8009BBC4;
extern s32 D_8009BC38;
extern s32 D_8009BCB0;
extern s32 D_8009BC3C;
extern s32 D_8009BCB4;
extern s32 D_8009C180;
extern void HeapFree(void *pMem);
extern void SoundStopAllElements(void);
extern void func_80086124(void);
extern s32 D_8009D7F0;
extern u8 *D_8009D158;
extern u8 *D_8009D15C;
extern u8 *D_8009D148;
extern void SoundRemoveSedsEntry(void *p);
extern void func_80074F04(void);
extern void func_800750DC(void);
extern void WorldMapFreeSpriteSets(void);
extern void func_80086568(void);
extern void func_800866C8(void);
extern void func_80088FF4(void);
extern void func_80089128(void);
extern void func_800976A0(void);
extern void func_80097D64(void);

// Preset screen: every body parked below was also compiled with this unit's
// maspsx flags under gcc-2.7.2-cdk and gcc-2.6.0, and none reaches 100% (best
// alternative 95.26, equal to its current score), so the parks are C problems
// and not this unit's compiler. Per-function alternative scores are in the
// notes beside each park.
s32 func_80080900(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->pos.x = D_8009C5AC.x;
    pSlot->pos.y = D_8009C5AC.y;
    pSlot->pos.z = D_8009C5AC.z;
    return 1;
}
// Best: 84.76%. The branch, every immediate, every relocation and the three
// func_80089160 calls match once the three scratchpad stores use the literal
// address 0x1F8000A0: retail writes them as `lui $at,0x1F80` + `sh $v0,0xA0($at)`
// with no relocation, while naming D_1F8000A0 emits %hi/%lo and forces the base
// into a saved register (that shape scores 76.71%). A 36-row try_flags sweep
// (gcc 2.6.0/2.7.2/cdk x -O1..-O3 x -G0/-G4/-G8 x mcpu 3000/r8000) tops out at
// 84.76, so the preset is not the problem. Residues: gcc sinks `sh zero,0x4(base)`
// below the pos.x/pos.z loads and hoists the pos.z load where retail keeps
// statement order, and the slot base lands in $a3 instead of $a2. Addressing the
// slot as D_8009BE24[index] per statement scores 81.95; moving the unk4 store to
// either end of the three stays at 84.76.
/*
s32 func_80080944(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];
    s32 v;

    if (pSlot->unk4 != 1) {
        return 1;
    }
    pSlot->unk4 = 0;
    v = pSlot->pos.x >> 12;
    *(s16 *)0x1F8000A0 = v;
    v = pSlot->pos.y >> 12;
    *(s16 *)0x1F8000A2 = v;
    v = pSlot->pos.z >> 12;
    *(s16 *)0x1F8000A4 = v;
    func_80089160(0x28, (s16 *)0x1F8000A0, 0);
    func_80089160(0x29, (s16 *)0x1F8000A0, 0);
    func_80089160(0x2A, (s16 *)0x1F8000A0, 0);
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080944);

void func_800809EC(u8 *p, s32 count, s32 a, s32 b, s32 c) {
    s32 i;

    for (i = 0; i < count; i++, p += 0x28) {
        p[4] = a;
        p[5] = b;
        p[6] = c;
    }
}

// Best: 92.64% (two shapes tie). Instruction multiset, relocations and every
// store offset match; two residues. (1) retail emits `sll a0,a0,0x7` on the
// parameter register and `addu v0,v0,a0`, gcc allocates a fresh pseudo
// (`sll a1,a0,0x7`). (2) retail's `move a0,s0` (the first call argument) lands
// between the two pos stores, gcc emits it just before them; writing the slot
// stores in retail's emission order (x, y, unk54, unk50, unk58, z) gives this
// shape, the x/y/z-first order gives the same 92.64%.
// The parameter shift cannot be re-created either: `index *= 0x80` followed by
// `(u8 *)D_8009BE24 + index`, `index << 7`, and a u32/int local all still give
// `sll a1,a0,0x7`; retail's `sll a0,a0,0x7` needs gcc to hand the sum pseudo the
// dying argument register, which no source spelling reached.
// Preset screen: cdk 87.51, gcc-2.6.0 87.15.
/*
s32 func_80080A28(s32 index) {
    u8 *p = D_8009C620;
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->pos.x = D_8009C5AC.x;
    pSlot->pos.y = 0xFFF80000;
    pSlot->unk54 = -0x800;
    pSlot->unk50 = 0;
    pSlot->unk58 = 0x80;
    pSlot->pos.z = D_8009C5B4;
    func_8007A06C(p, *(s32 *)(p + 0x48), *(u16 *)(*(s32 *)(p + 0x40) + 4));
    p += 0x54;
    func_8007A06C(p, *(s32 *)(p + 0x48), *(u16 *)(*(s32 *)(p + 0x40) + 4));
    return 3;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080A28);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080AC4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80080D00);
void func_8008106C(void) {
    u16 v;

    SoundStopAllElements();
    SoundRemoveSedsEntry(D_8006259C);
    HeapFree(D_8006259C);
    WorldMapFreeSpriteSets();
    func_80086568();
    func_800866C8();
    func_80074F04();
    func_800750DC();
    func_80088FF4();
    func_80089128();
    func_80097D64();
    HeapFree(D_8009BC38);
    HeapFree(D_8009BCB0);
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    HeapFree(D_8009C180);
    func_800976A0();
    v = *(u16 *)&D_8009BD3A;
    D_8006F94E = 0x1FA;
    D_8006F954 = 0;
    D_8009BBC4 = 1;
    D_8006F950 = v;
}
// Best: 95.26%. Statement order matches retail (the slot base is formed before
// the D_8009A6C0 load, the counter is reloaded twice); only the register choice
// differs: retail keeps the base in $v1 and the index in $a0, gcc puts the base
// in $v0 and reuses $a0 for the index. A pointer local restores $v1 but
// reorders the statements (65.26%); the post-increment spelling scores 56.84%.
// A pointer local (WorldMapSlot *pSlot = D_8009BE24 + index, or &D_8009BE24[index])
// does get retail's registers ($v1 for the base, $a0 for the index temp) but lets
// gcc hoist the D_8009A6C0 load above the address formation (65.26%), so the
// direct-index form below remains the best shape.
// Preset screen: cdk 48.89, gcc-2.6.0 95.26.
/*
s32 func_80081174(s32 index) {
    D_8009BE24[index].unk50 = 0;
    D_8009BE24[index].unk20 = D_8009A6C0;
    D_8009BE24[index].unk22 = D_8009A70C[D_8009BE24[index].unk50];
    D_8009BE24[index].unk50 = D_8009BE24[index].unk50 + 1;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081174);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800811C0);
// Best: 76.59%, same class as before: instruction multiset and relocations are
// identical (the three source words are the separate globals D_8009C5AC,
// D_8009C5B0 and D_8009C5B4, not offsets off a Vec3 - retail names all three),
// only the schedule of the `sh` pairs differs. With the two slot halfword
// stores written first gcc emits them right after the base addu and pushes the
// BE28/D55C pair down (this shape, 76.59%); moving them last instead reorders
// the whole tail (65.71%, and 33.94% while y/z were still struct offsets).
// Reading y/z as D_8009C5AC.y/.z scores 77.76% but emits lui/lw 4(a0)/8(a0)
// relocations retail does not have.
// Preset screen: cdk 76.59, gcc-2.6.0 46.47.
/*
s32 func_800813E8(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];
    s32 x;
    s32 y;
    s32 z;

    pSlot->unk4 = 1;
    pSlot->unk20 = 0;
    D_8009D144 = 0;
    pSlot->unk7C = 0x1000;
    x = D_8009C5AC.x;
    y = D_8009C5B0;
    z = D_8009C5B4;
    D_8009BE0C = 0x78;
    D_8009BE28 = x;
    D_8009D55C = x;
    D_8009BE2C = y;
    D_8009D560 = y;
    D_8009BE30 = z;
    D_8009D564 = z;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800813E8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081470);
void func_800816DC(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 abr) {
    s32 i;

    for (i = 0; i < count; i++) {
        pDst[3] = 9;
        pDst[7] = 0x2C;
        *(u16 *)(pDst + 0x16) = GetTPage(0, abr, 0x180, 0);
        pDst[7] |= 2;
        pDst += 0x28;
    }
    memcpy(pEntry->unk4C, pEntry->unk48, count * 0x28);
}

// Best: 66.00%. Structure, relocations and offsets are all right (the call,
// the three >>12 stores, the six slot stores); gcc's scheduler hoists the
// D_8009C5B0/D_8009C5B4 loads and the D_8009C620 load above the `sh 0x20`
// store and sinks the D_8009C5AC load to its store, retail keeps statement
// order. Locals x/y/z and a late pEntry assignment were no better.
// Preset screen: cdk 50.54, gcc-2.6.0 58.56.
/*
s32 func_800817A0(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];
    WorldMapEntry *pEntry = &D_8009C620[2];

    pSlot->unk20 = 0;
    pSlot->pos.x = D_8009C5AC.x;
    pSlot->pos.y = D_8009C5B0 - 0x100000;
    pSlot->pos.z = D_8009C5B4;
    pSlot->unk50 = 0;
    pSlot->unk54 = 0;
    pSlot->unk58 = 0;
    func_800816DC(pEntry, pEntry->unk48, *(u16 *)(pEntry->unk40 + 4), 1);
    pEntry->unk8 = pSlot->pos.x >> 12;
    pEntry->unkC = pSlot->pos.y >> 12;
    pEntry->unk10 = pSlot->pos.z >> 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800817A0);
s32 func_80081868(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];
    WorldMapEntry *pEntry = &D_8009C620[2];
    u8 *p;
    s32 i;

    switch (pSlot->unk4) {
    case 1:
        pSlot->unk4 = 0;
        pSlot->unk20 = 1;
        break;
    case 2:
        pSlot->unk4 = 0;
        pSlot->unk20 = 0;
        p = *(u8 **)((u8 *)pEntry + D_8009D7F0 * 4 + 0x48);
        for (i = 0; i < *(u16 *)(pEntry->unk40 + 4); i++, p += 0x28) {
            p[7] &= ~2;
            p[4] = 0x80;
            p[5] = 0x80;
            p[6] = 0x80;
        }
        break;
    }
    if (pSlot->unk20 != 0) {
        if (pSlot->unk20 == 1) {
            pSlot->unk50 += 1;
            pSlot->unk54 += 1;
            pSlot->unk58 += 1;
            if (pSlot->unk50 >= 0xFF) {
                pSlot->unk58 = 0xFF;
                pSlot->unk54 = 0xFF;
                pSlot->unk50 = 0xFF;
                pSlot->unk20 = 0;
            }
        }
    }
    func_800809EC(*(u8 **)((u8 *)pEntry + D_8009D7F0 * 4 + 0x48),
                  *(u16 *)(pEntry->unk40 + 4),
                  pSlot->unk50, pSlot->unk54, pSlot->unk58);
    return 1;
}
// Best: 87.03%. Every call, offset and relocation is right (the 8-word matrix
// copy from D_8009A180, the three scratchpad words, ScaleMatrix with the
// 0x1F800000 scale vector, the matrix copy to +0x74, both func_800816DC
// calls). The only delta is the block of pSlot stores: gcc hoists the
// D_8009C5B0/D_8009C5B4 loads and the lui 0x1F80 to the top of the block and
// sinks the D_8009C5AC store below the three zero stores, while retail keeps
// statement order and reuses $v1 for all three loads. Same blocker as
// func_800817A0 above; pointer local, zero-store order and x/y/z order were
// all no better (vec3 through a pointer local: 87.03, zeros first: 84.43).
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800819C8);
/*
s32 func_800819C8(s32 index) {
    u8 *pEntry = (u8 *)D_8009C620;
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->unk20 = 0;
    pSlot->pos.x = D_8009C5AC.x;
    pSlot->pos.y = D_8009C5B0;
    pSlot->pos.z = D_8009C5B4;
    pSlot->unk50 = 0;
    pSlot->unk54 = 0;
    pSlot->unk58 = 0;
    *(MATRIX *)(pEntry + 0x20) = D_8009A180;
    *(s32 *)0x1F800008 = 0x1000;
    *(s32 *)0x1F800000 = 0x1000;
    *(s32 *)0x1F800004 = 0x7000;
    ScaleMatrix((MATRIX *)(pEntry + 0x20), (VECTOR *)0x1F800000);
    *(MATRIX *)(pEntry + 0x74) = *(MATRIX *)(pEntry + 0x20);
    func_800816DC((WorldMapEntry *)pEntry, *(u8 **)(pEntry + 0x48),
                  *(u16 *)(*(u8 **)(pEntry + 0x40) + 4), 3);
    pEntry += 0x54;
    func_800816DC((WorldMapEntry *)pEntry, *(u8 **)(pEntry + 0x48),
                  *(u16 *)(*(u8 **)(pEntry + 0x40) + 4), 3);
    return 1;
}
*/
s32 func_80081B24(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];
    u8 *pEntry = (u8 *)D_8009C620;

    if (pSlot->unk4 == 1) {
        pSlot->unk4 = 0;
        pSlot->unk20 = 1;
    }
    if (pSlot->unk20 != 0) {
        if (pSlot->unk20 == 1) {
            pSlot->unk50 += 4;
            pSlot->unk54 += 2;
            pSlot->unk58 += 1;
            if (pSlot->unk50 >= 0xFC) {
                pSlot->unk50 = 0xFC;
                pSlot->unk20 = 0;
            }
        }
    }
    func_800809EC(*(u8 **)(pEntry + D_8009D7F0 * 4 + 0x48),
                  *(u16 *)(*(u8 **)(pEntry + 0x40) + 4),
                  pSlot->unk50, pSlot->unk54, pSlot->unk58);
    pEntry += 0x54;
    func_800809EC(*(u8 **)(pEntry + D_8009D7F0 * 4 + 0x48),
                  *(u16 *)(*(u8 **)(pEntry + 0x40) + 4),
                  pSlot->unk50, pSlot->unk54, pSlot->unk58);
    return 1;
}
/*
s32 func_80081C3C(void) {
    Quad *pSrc;
    Quad *pDst;
    u16 *pFill;
    u8 *p;
    s32 i;

    D_8009D158 = HeapAlloc(0x1E00, 0);
    D_8009D15C = HeapAlloc(0x1E00, 0);
    D_8009D148 = HeapAlloc(0x180, 0);
    p = D_8009D158;
    for (i = 0; i < 0xC0; i++) {
        p[7] = 0x2C;
        p[3] = 9;
        p[4] = 0x80;
        p[5] = 0x80;
        p[6] = 0x80;
        p[7] |= 1;
        *(u16 *)(p + 0x16) = GetTPage(2, 0, 0x280, 0x100);
        p += 0x28;
    }
    pSrc = (Quad *)D_8009D158;
    pDst = (Quad *)D_8009D15C;
    for (i = 0; i < 0x1E0; i++) {
        *pDst++ = *pSrc++;
    }
    pFill = (u16 *)D_8009D148;
    for (i = 0xBF; i >= 0; i--) {
        *pFill++ = 1;
    }
    return 1;
}
*/
// Best: 87.09%. Allocation, both loops' shapes and GetTPage are right; the
// differences are (1) retail keeps &D_8009D158 in $s0 across the three
// HeapAlloc calls (store and later load both go through it), gcc rematerialises
// lui/$at for each; (2) retail materialises 9 into $s4 before 0x2C into $s3.
// Preset screen: cdk 71.46, gcc-2.6.0 87.09.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081C3C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081D80);

s32 func_80081FB4(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->unk20 = 0;
    pSlot->unk50 = 1;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80081FD8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_80082324);
s32 func_800826B4(void) {
    u16 v;

    SoundStopAllElements();
    SoundRemoveSedsEntry(D_8006259C);
    HeapFree(D_8006259C);
    WorldMapFreeSpriteSets();
    func_80086124();
    func_80086568();
    func_800866C8();
    func_80074F04();
    func_800750DC();
    func_80088FF4();
    func_80089128();
    func_80097D64();
    HeapFree(D_8009BC38);
    HeapFree(D_8009BCB0);
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    HeapFree(D_8009C180);
    func_800976A0();
    v = *(u16 *)&D_8009BD3A;
    D_8006F94E = 0x269;
    D_8006F954 = 2;
    D_8009BBC4 = 1;
    D_8006F950 = v;
    return 1;
}

// The live body directly below this block matches 100% (index-then-symbol
// order): the pointer local is what misallocates here.
// Parked alternative: 51.67%. Instruction multiset is identical; the original computes the
// slot pointer (sll/addu) before forming the address of D_8009A758, gcc forms
// the address first. 46.11% for the slot-pointer-first order; declaring pData
// before pSlot puts the symbol address first and reaches 51.67%. Also tried:
// two separate statements, a s32 value temp, `D_8009BE24 + index`, and an
// explicit offset local.
/*
s32 func_800827C8(s32 index) {
    u8 *pData = D_8009A758;
    WorldMapSlot *pSlot = &D_8009BE24[index];

    pSlot->unk50 = (s32)pData;
    return 1;
}
*/
s32 func_800827C8(s32 index) {
    D_8009BE24[index].unk50 = (u32)D_8009A758;
    return 1;
}
// Best: 39.15%. Blocker is gcc's store-to-load forwarding: retail stores 0 to
// D_8009BD3A/D_8009BD3C and reads them back with `lh` to build the two `<< 12`
// slot stores, while this gcc folds the read to 0 and drops the sll (which also
// shuffles the whole tail). Same blocker as misc8's func_8008390C (84.75%).
// Preset screen: cdk 36.73, gcc-2.6.0 40.43.
/*
s32 func_800827EC(s32 index) {
    WorldMapSlot *pSlot = &D_8009BE24[index];
    s32 a;
    s32 b;

    pSlot->unk7C = 0x1000;
    D_8009BE28 = D_8009C5AC.x;
    D_8009D55C = D_8009C5AC.x;
    pSlot->pos.x = D_8009C5AC.x;
    D_8009BE2C = D_8009C5B0;
    D_8009D560 = D_8009C5B0;
    pSlot->pos.y = D_8009C5B0;
    D_8009BE30 = D_8009C5B4;
    D_8009D564 = D_8009C5B4;
    pSlot->pos.z = D_8009C5B4;
    pSlot->unk20 = 0;
    pSlot->unk4 = 0;
    *(s32 *)pSlot->unk5C = 0x500000;
    D_8009BD38 = -0x220;
    D_8009BD3A = 0;
    D_8009BD3C = 0;
    pSlot->unk50 = 0xFFDE0000;
    ((s32 *)pSlot->unk34)[1] = 0xFFDE0000;
    a = D_8009BD3A;
    D_8009D144 = 0;
    D_8009D3F0 = 0x500000;
    pSlot->unk54 = a << 12;
    ((s32 *)pSlot->unk34)[2] = a << 12;
    b = D_8009BD3C;
    D_8009BE0C = 0x78;
    pSlot->unk58 = b << 12;
    ((s32 *)pSlot->unk34)[3] = b << 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800827EC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc7", func_800828DC);
