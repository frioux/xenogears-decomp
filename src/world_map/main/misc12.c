#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

extern s32 func_80095414(s32 *pPos, s32 *pRot, s32 *pOut, s32 nSpeed, s32 nMode);
extern void func_800848B4(int, int);
extern s16 D_8009BD40;
extern s16 D_8009BD44;
extern s32 D_8009BE28;
extern s32 D_8009BE30;
extern s32 D_8009D560;
extern s32 D_8009BCDC;
extern s32 D_8009B214[];
extern u16 D_1F8000A8;
extern void func_80096F18(s16 *pOut, s32 *pSrc, s32 nHeight, u16 *pAngles);
extern void func_80093354(void *p);
extern u8 *func_80093660(s32 x, s32 z);
extern u8 D_8009BD60;
extern s16 D_8009BD24;
extern u8 D_8009D738;
extern void *D_8009D7D8;
extern u32 D_8009B6C4[3];
extern u16 D_8009B6D0;
extern u32 D_8009B6D4[3];
extern u16 D_8009B6E0;
extern u16 D_8009CD4C;
extern s32 D_8009CEC0;
extern s32 D_8009C7E8;
extern s32 D_8009BD34;
extern s32 D_8009BE0C;
extern s32 D_8009BE10;
extern s32 D_8009D3F0;
extern s16 D_8009BD38;
extern s16 D_8009BD3A;
extern s16 D_8009BD3C;
extern u16 D_8009D52C;
extern u32 D_8009D55C[];
extern u32 D_8009B224[];
extern u32 D_8009B22C[];
extern u32 D_8009B234[];
extern u32 D_8009B23C[];

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u8 unk22[0x2];
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u8 unk26[0x2];
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ u32 unk38;
    /* 0x3C */ u32 unk3C;
    /* 0x40 */ u32 unk40;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ u16 unk48;
    /* 0x4A */ u16 unk4A;
    /* 0x4C */ u8 unk4C[0x4];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ u32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ u32 unk60;
    /* 0x64 */ void *unk64;
    /* 0x68 */ void *unk68;
    /* 0x6C */ u32 unk6C;
    /* 0x70 */ u8 unk70[0x4];
    /* 0x74 */ u32 unk74;
    /* 0x78 */ u8 unk78[0x8];
} WorldMapPlayer; // Size: 0x80

typedef struct {
    /* 0x000 */ u8 unk0[0x8];
    /* 0x008 */ u32 unk8[4];
    /* 0x018 */ u8 unk18[0x98];
    /* 0x0B0 */ u32 unk0B0[4];
    /* 0x0C0 */ u8 unk0C0[0x44];
    /* 0x104 */ u32 unk104[4];
} WorldMapSource; // Size: 0x114

extern WorldMapPlayer *D_8009BE24;
extern WorldMapSource *D_8009C620;

typedef struct {
    /* 0x0 */ u32 w[4];
} Quad; // Size: 0x10

typedef struct {
    /* 0x00 */ u32 w[9];
} WorldMapG4; // Size: 0x24, two POLY_G4-shaped primitives are not it

typedef struct {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ u16 unk1C;
    /* 0x1E */ u8 unk1E[0x2];
    /* 0x20 */ MATRIX matrix;
    /* 0x40 */ u8 unk40[0x14];
} WorldMapRot; // Size: 0x54

void func_8008DFF4(s32 *pOut) {
    pOut[0] = g_GameState.unk182C << 12;
    pOut[1] = g_GameState.unk182E << 12;
    pOut[2] = g_GameState.unk1830 << 12;
}

void func_8008E034(s32 *pIn) {
    g_GameState.unk182C = pIn[0] >> 12;
    g_GameState.unk182E = pIn[1] >> 12;
    g_GameState.unk1830 = pIn[2] >> 12;
}

void func_8008E078(void) {
    u16 nEntry;
    void *pRect;

    if (D_8009D738 == 0) {
        return;
    }
    switch (D_8009BD60) {
    case 0xF:
        nEntry = D_8009B6D0;
        pRect = D_8009B6C4;
        break;
    case 0x10:
        nEntry = D_8009B6E0;
        pRect = D_8009B6D4;
        break;
    default:
        return;
    }
    D_8009D7D8 = pRect;
    D_8009BD24 = nEntry;
}
s32 func_8008E0F0(s32 *pPos, s32 *pDir, s32 nSpeed) {
    s32 nAngle;
    s32 out[4];
    s32 rot[4];

    for (nAngle = 0; nAngle < 0x1000; nAngle += 0x100) {
        rot[0] = rcos(nAngle);
        rot[2] = -rsin(nAngle);
        if (func_80095414(pPos, rot, out, nSpeed, 2) == 1) {
            return nAngle;
        }
    }
    return -1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E190);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E4F4);
s32 func_8008E680(s32 idx) {
    WorldMapPlayer *pPlayer;
    u16 nAngle;

    pPlayer = D_8009BE24 + idx;
    func_8008DFF4(&pPlayer->unk28);
    pPlayer->unk2C = 0x80000;
    nAngle = g_GameState.unk1832;
    pPlayer->unk4A = 0x20;
    pPlayer->unk74 = 3;
    pPlayer->unk68 = (void *)0xFFD80000;
    pPlayer->unk40 = 0;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk24 = 0;
    pPlayer->unk60 = 0;
    pPlayer->unk64 = 0;
    pPlayer->unk6C = 0;
    pPlayer->unk20 = 8;
    pPlayer->unk48 = nAngle;
    *(Quad *)D_8009D55C = *(Quad *)&pPlayer->unk28;
    D_8009D52C = pPlayer->unk48;
    *(Quad *)D_8009C620->unk8 = *(Quad *)&pPlayer->unk28;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E76C);
s32 func_800906E0(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nMode;

    func_800848B4(0, 2);
    func_800848B4(0, 3);
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk20 = 0;
    *(Quad *)&pPlayer->unk28 = *(Quad *)D_8009C620->unk0B0;
    *(Quad *)&pPlayer->unk38 = *(Quad *)D_8009C620->unk104;
    nMode = D_8009BE10;
    pPlayer->unk50 = 0;
    pPlayer->unk54 = 0x40;
    if (nMode < 8) {
        if (nMode >= 4) {
            pPlayer->unk20 = 3;
            pPlayer->unk5C = 0x80;
            pPlayer->unk58 = 0x80;
        }
    }
    return 1;
}

int func_800907C4(void) {
    func_800848B4(0, 2);
    func_800848B4(0, 3);
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800907F4);
/* best: 98.27%. Structurally identical (same frame, same registers, two pointers
   at D_8009C620+0xA8/+0xFC, one scratchpad base in $s0 live across both calls).
   The only delta is the value in $s0: retail holds 0x1F800000 and so stores at
   0xA0/0xA2/0xA4/0xA8/0xAA/0xAC and passes the two RotMatrixZYX vectors with
   `ori a0,s0,0xA0` / `ori a0,s0,0xA8`; gcc 2.7.2 rematerialises the one-insn
   constant and derives the vectors as base+0xA0 with `addiu` (98.27), while a
   base of 0x1F8000A0 keeps a register but shifts all eight offsets (98.27 too,
   mirrored). Literal 0x1F8000xx addresses make reload use $at per store (74.29);
   cdk 72.92, so the preset is not the answer.

s32 func_800907F4(s32 idx) {
    WorldMapPlayer *pPlayer;
    WorldMapRot *pRotA;
    WorldMapRot *pRotB;

    pPlayer = D_8009BE24 + idx;
    pRotA = (WorldMapRot *)((u8 *)D_8009C620 + 0xA8);
    pRotB = (WorldMapRot *)((u8 *)D_8009C620 + 0xFC);
    if (pPlayer->unk4 == 9) {
        pPlayer->unk4 = 0;
        pPlayer->unk20 = 1;
    } else if (pPlayer->unk4 == 0xA) {
        pPlayer->unk4 = 0;
        pPlayer->unk20 = 2;
    }
    switch (pPlayer->unk20) {
    case 0:
        pPlayer->unk58 = 0;
        pPlayer->unk5C = 0;
        break;
    case 1:
        pPlayer->unk58 += 4;
        if (pPlayer->unk58 >= 0x11) {
            pPlayer->unk5C += 4;
        }
        if (pPlayer->unk58 >= 0x80) {
            pPlayer->unk58 = 0x80;
        }
        if (pPlayer->unk5C >= 0x80) {
            pPlayer->unk5C = 0x80;
        }
        if ((pPlayer->unk58 >= 0x80) & (pPlayer->unk5C >= 0x80)) {
            pPlayer->unk20 = 3;
        }
        break;
    case 2:
        pPlayer->unk58 -= 4;
        if (pPlayer->unk58 < 0x70) {
            pPlayer->unk5C -= 4;
        }
        if (pPlayer->unk58 < 0) {
            pPlayer->unk58 = 0;
        }
        if (pPlayer->unk5C < 0) {
            pPlayer->unk5C = 0;
        }
        if ((pPlayer->unk58 == 0) & (pPlayer->unk5C == 0)) {
            pPlayer->unk20 = 0;
        }
        break;
    }
    pPlayer->unk50 += pPlayer->unk58;
    pPlayer->unk54 -= pPlayer->unk5C;
    *(u16 *)0x1F8000A8 = 0;
    *(u16 *)0x1F8000A0 = 0;
    *(u16 *)0x1F8000A2 = pPlayer->unk50;
    *(u16 *)0x1F8000AA = pPlayer->unk54;
    *(u16 *)0x1F8000A4 = pRotA->unk1C;
    *(u16 *)0x1F8000AC = pRotB->unk1C;
    RotMatrixZYX((SVECTOR *)0x1F8000A0, &pRotA->matrix);
    RotMatrixZYX((SVECTOR *)0x1F8000A8, &pRotB->matrix);
    return 1;
}*/
void func_80090A18(void) {
    u16 keys;
    s32 state;

    keys = D_8009CD4C;
    if ((keys & 1) && (keys & 2)) {
        D_8009CEC0 = 1;
    } else {
        D_8009CEC0 = 0;
    }
    state = D_8009CEC0;
    D_8009BD34 = (state ^ D_8009C7E8) & state;
    D_8009C7E8 = state;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090A84);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090C68);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090E14);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090FB4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091430);
/* best: 91.20%. Retail keeps the address of D_8009BD3A in one register (sh then
   lh off the same base); gcc forwards the stored value so the address is used
   once. A volatile STORE kills the forwarding (88.22 -> 91.20) but the address
   still splits: `lui $at/%lo` for the store plus a fresh lui for the load, and
   it also hoists the D_8009BE10 load. Also tried: volatile load (78.10, lhu +
   sll16/sra4), s16 pointer local for both accesses (same split), `extern
   volatile s16 D_8009BD3A` plus a volatile pointer local (78.10, lhu + the
   split), cdk 75.97, -O3/-G0 85.22. maspsx fuses any single-use symbol address
   into `%lo(sym)($at)`, so both memory refs must survive cse AND share the
   lazily-loaded address - a plain store lets cse forward the value, volatile
   stops cse pooling the address. Untried: a shape where the two refs cannot be
   value-numbered for another reason.
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800914D0);
s32 func_80091B54(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nMode;

    pPlayer = D_8009BE24 + idx;
    nMode = D_8009BE10;
    switch (nMode) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        pPlayer->unk50 = 3;
        D_8009D3F0 = 0x460000;
        D_8009BD38 = -0x260;
        pPlayer->unk64 = D_8009B224;
        pPlayer->unk68 = D_8009B234;
        break;
    case 6:
        break;
    case 7:
        pPlayer->unk50 = 3;
        D_8009D3F0 = 0x280000;
        D_8009BD38 = -0x260;
        pPlayer->unk64 = D_8009B22C;
        pPlayer->unk68 = D_8009B23C;
        break;
    }
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091C18);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091FF8);
/* best: 92.10%, same size (580 bytes). The whole structure is decoded and every
   relocation is right; what is left is gcc allocation/scheduling:
   - the three callee-saved roles are permuted (retail nX=s3, nBest=s4, i=s5;
     this compile nBest=s3, i=s4, nX=s5 - the loop counter lands in a different
     register, so the tail `move v0,s5`/`lw s5,16(sp)` pair differs),
   - `lui v0,0xfff8` (the 0xFFF80000 mask) is hoisted to the top of the block
     here and used as an early t0, retail emits it late and uses it twice,
   - retail fills the `beqz` delay slot of the inner `if (nZ < nBest)` with the
     `sw v1,4(s1)`; this compile leaves a nop there,
   - the final distance is `tbl + nBase - (nBest*8 - 0x50)`; gcc folds the 0x50
     onto the minuend here (`addiu v0,v0,80`) unless the subtrahend is held in a
     local, and then the second `slti` is duplicated into a delay slot.
   The 4th call argument must be `&D_1F8000A8` (a symbol reloc) - writing
   `pScratch + 0xA8` gives the same bytes with no reloc, and retail keeps the
   symbol *and* reuses $s1 as the high half.

s32 func_80091FF8(s32 nLimit, u16 *pIn, s16 *pOfs) {
    s32 *pTrack;
    u8 *pScratch;
    s32 nBase;
    s32 nX;
    s32 nY;
    s32 nBest;
    s32 nZ;
    s32 i;
    s32 j;
    s32 k;

    u16 v = *(u16 *)&D_8009BD3A;
    u16 a = *(u16 *)&D_8009BD3C;

    nBase = D_8009D560 >> 12;
    *(u16 *)0x1F8000AA = v;
    *(u16 *)0x1F8000AC = a;
    pScratch = (u8 *)0x1F800000;
    pTrack = D_8009B214;
    for (i = 0; i < 3; i++) {
        nBest = 0;
        *(u16 *)(pScratch + 0xA8) = *pIn;
        func_80096F18(&D_8009BD40, &D_8009BE28, *pTrack - (D_8009BCDC / 2 << 12), (u16 *)&D_1F8000A8);
        nX = (D_8009BE28 + ((D_8009BD40 - 0x180) << 12)) & 0xFFF80000;
        nY = (D_8009BE30 - ((D_8009BD44 + 0x180) << 12)) & 0xFFF80000;
        *(s32 *)(pScratch + 8) = nY;
        for (j = 0; j < 6; j++) {
            *(s32 *)pScratch = nX;
            for (k = 0; k < 6; k++) {
                func_80093354(pScratch);
                nZ = *(s8 *)func_80093660(*(s32 *)pScratch, *(s32 *)(pScratch + 8));
                if (nZ < nBest) {
                    *(s32 *)(pScratch + 4) = nZ;
                    nBest = nZ;
                }
                *(s32 *)pScratch += 0x80000;
            }
            *(s32 *)(pScratch + 8) += 0x80000;
        }
        if (*(s16 *)pOfs + nBase + 0x50 < (nBest << 3)) {
            break;
        }
        pIn++;
        pTrack++;
        pOfs++;
    }
    if (i < nLimit) {
        s32 nDist = (nBest << 3) - 0x50;
        nZ = ((s16 *)D_8009B234)[nLimit] + nBase - nDist;
        if (nZ < 0) {
            nZ = -nZ;
        }
        if (nZ < 0x41) {
            i = nLimit;
        }
    }
    return i;
}
*/

s32 func_80092234(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nMode;

    pPlayer = D_8009BE24 + idx;
    nMode = D_8009BE10;
    if (nMode > 0) {
        if (nMode < 6) {
            D_8009BE0C = 0x8C;
        } else if (nMode < 8) {
            D_8009BE0C = 0x78;
            pPlayer->unk20 = 1;
        }
    }
    pPlayer->unk50 = D_8009BE0C << 12;
    return 1;
}
s32 func_800922AC(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nRet;

    pPlayer = D_8009BE24 + idx;
    nRet = 1;
    if (pPlayer->unk4 == 9) {
        pPlayer->unk4 = 0;
        pPlayer->unk20 = 1;
    } else if (pPlayer->unk4 == 0xA) {
        pPlayer->unk4 = 0;
        pPlayer->unk20 = 2;
    }
    switch (pPlayer->unk20) {
    case 0:
        nRet = 3;
        break;
    case 1:
        pPlayer->unk50 -= 0x1000;
        D_8009BE0C = pPlayer->unk50 >> 12;
        if (D_8009BE0C < 0x78) {
            D_8009BE0C = 0x78;
            pPlayer->unk50 = 0x78000;
            pPlayer->unk20 = 0;
        }
        break;
    case 2:
        pPlayer->unk50 += 0x1000;
        D_8009BE0C = pPlayer->unk50 >> 12;
        if (D_8009BE0C >= 0x8C) {
            D_8009BE0C = 0x8C;
            pPlayer->unk50 = 0x8C000;
            pPlayer->unk20 = 0;
        }
        break;
    }
    return nRet;
}
// The original touches every field of the two primitives at 0x8009CE6C, so
// splat gave each field its own address symbol; naming them here keeps the
// %lo relocations identical to the original object.
extern WorldMapG4 D_8009CE6C;
extern u8 D_8009CE6F;
extern u8 D_8009CE70;
extern u8 D_8009CE71;
extern u8 D_8009CE72;
extern u8 D_8009CE73;
extern u16 D_8009CE74;
extern u16 D_8009CE76;
extern u8 D_8009CE78;
extern u8 D_8009CE79;
extern u8 D_8009CE7A;
extern u16 D_8009CE7C;
extern u16 D_8009CE7E;
extern u8 D_8009CE80;
extern u8 D_8009CE81;
extern u8 D_8009CE82;
extern u16 D_8009CE84;
extern u16 D_8009CE86;
extern u8 D_8009CE88;
extern u8 D_8009CE89;
extern u8 D_8009CE8A;
extern u16 D_8009CE8C;
extern u16 D_8009CE8E;
extern u16 D_8009CE98;
extern u16 D_8009CE9A;
extern u16 D_8009CEA0;
extern u16 D_8009CEA2;
extern u16 D_8009CEA8;
extern u16 D_8009CEAA;
extern u16 D_8009CEB0;
extern u16 D_8009CEB2;
extern DR_TPAGE D_8009D310;
extern s32 D_8009CCA4;

s32 func_800923A8(s32 idx) {
    WorldMapPlayer *pPlayer;
    WorldMapG4 *pG4;
    u_short nTpage;

    pPlayer = D_8009BE24 + idx;
    pPlayer->unk50 = 0xF8;
    nTpage = GetTPage(0, D_8009CCA4, 0x380, 0x100);
    SetDrawTPage(&D_8009D310, 1, 0, nTpage);
    pG4 = &D_8009CE6C;
    // the original re-reads unk50 for all twelve colour bytes; volatile stops
    // gcc folding them into one load.
    D_8009CE6F = 8;
    D_8009CE73 = 0x38;
    D_8009CE70 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE71 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE72 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE78 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE79 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE7A = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE80 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE81 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE82 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE88 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE89 = *(volatile s32 *)&pPlayer->unk50;
    D_8009CE8A = *(volatile s32 *)&pPlayer->unk50;
    SetSemiTrans(pG4, 1);
    pG4[1] = pG4[0];
    D_8009CE74 = 0;
    D_8009CE76 = 0;
    D_8009CE7C = 0x140;
    D_8009CE7E = 0;
    D_8009CE84 = 0;
    D_8009CE86 = 0xD8;
    D_8009CE8C = 0x140;
    D_8009CE8E = 0xD8;
    D_8009CE98 = 0;
    D_8009CE9A = 0;
    D_8009CEA0 = 0x140;
    D_8009CEA2 = 0;
    D_8009CEA8 = 0;
    D_8009CEAA = 0xD8;
    D_8009CEB0 = 0x140;
    D_8009CEB2 = 0xD8;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800925A0);
