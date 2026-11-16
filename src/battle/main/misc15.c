#include "common.h"
#include "main/game.h"

// Command slot descriptor: one 0x40-byte entry per battle actor slot, in the
// head array of the block D_800C3EAC points at.
typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ u8 unk3C; // actor slot this entry stands for
    /* 0x3D */ u8 pad3D[3];
} BattleCommandSlot; // Size: 0x40

// Partial view of the battle command queue (D_800C3EAC): the head slot array
// and the queued-command counter. src/battle/main/misc4.c holds the same block
// under the same name.
typedef struct {
    /* 0x000 */ BattleCommandSlot slots[4];
    /* 0x100 */ u8 unk100[0x1C4];
    /* 0x2C4 */ u8 unk2C4[8];
    /* 0x2CC */ u8 unk2CC[0xE];
    /* 0x2DA */ u8 count; // queued commands
    /* 0x2DB */ u8 unk2DB;
    /* 0x2DC */ u8 unk2DC;
    /* 0x2DD */ u8 unk2DD[9];
    /* 0x2E6 */ u8 unk2E6;
} BattleCommandQueue; // Size: 0x2E7

// Partial view of the per-frame battle display state (D_800D2D28); the shared
// type is BattleWork in include/battle/battle_work.h.
typedef struct {
    /* 0x000 */ u8 unk0[0x9C];
    /* 0x09C */ u8 unk9C;
    /* 0x09D */ u8 unk9D;
    /* 0x09E */ u8 unk9E;
    /* 0x09F */ u8 unk9F[0x10];
    /* 0x0AF */ u8 unkAF;
    /* 0x0B0 */ u8 unkB0[4]; // prim flags the *PrimFlags*Slots pairs below raise
    /* 0x0B4 */ u8 unkB4[3];
    /* 0x0B7 */ u8 unkB7;
    /* 0x0B8 */ u8 unkB8[0xE];
    /* 0x0C6 */ u8 unkC6;
    /* 0x0C7 */ u8 unkC7[4];
    /* 0x0CB */ u8 unkCB;
} BattleDisplayState; // Size: 0xCC

extern BattleCommandQueue *D_800C3EAC;
extern u8 D_800C3CF4[];
extern BattleDisplayState *D_800D2D28;
extern u8 D_800D366C;
extern int ArchiveDataSync(void);
extern void BattlePlaySoundWithBaseId(u8 arg0);
extern void BattleUpdateFrame(void);
extern void BattleFreePacketPrimitives(void);
extern void BattleClearFlagC6(void);
extern void func_8008FA60(s32 arg0);
extern u16 BattleSlotBitMask(u8 arg0);
extern void BattleBeginAnimTask(u16 arg0);
extern void func_800BCD98(u16 arg0);
extern u16 BattleMaskHasSlotBit(u16 arg0, u8 arg1);
extern u16 BattleSlotBitMaskInverted(u8 arg0);
extern u8 D_800C400B[];
extern u16 D_800C4022[];
extern u8 D_800C402F[];
extern u8 D_800CCD1C[];
extern u8 D_800D2CE0[];
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u8 pad2[0xE];
} BattleCommandEntry; // Size: 0x10

extern BattleCommandEntry D_800D2204[];
extern u8 D_800D3688[];
extern u8 D_800C3D70[];
extern u8 D_800C3FFE[];
extern u8 func_80085084(u16 arg0, u8 arg1, s32 arg2);
extern s32 BattleIsCharacterInParty(s32 arg0);
extern s32 BattleMaskHasStatusBit(s32 mask, u8 idx);
extern void func_8009A854(u8 arg0);
extern u16 D_800D2C94;
extern u8 D_800D2C98;
extern u8 D_800D32A1[];
extern u16 D_800C3D64;
extern u8 D_800C3E2C;
extern u8 D_800CDD40[];
extern u8 D_800CCD38[];
extern u8 D_800D2D24[];
extern u8 D_800CEF10[];
extern void BattleClearSlotCurveEntry(void);
extern void func_80079840(u8 arg0, u8 arg1);
extern void BattleQueueCommandFE(u8 arg0);
extern void BattleApplySlotCurveAndStep(u8 arg0);
extern void BattleBeginActorAction(u8 arg0, u16 arg1, u16 arg2);
extern s32 BattleFindMinValueOtherSlot(u8 arg0);
extern void func_800B89FC(s32 arg0, u8 arg1, s32 arg2, s32 arg3);
extern s32 func_8008C81C(u8 arg0);
extern void BattleNoOpCallback(s32 arg0, s32 arg1);
extern void func_800877E0(u8 arg0, u8 arg1);
extern void func_80087EDC(u8 arg0, u8 arg1);
extern void BattleSetupActorSlotDisplay(u8 arg0);
extern u8 D_800C4928;

void BattlePlaySoundIfEnabled(u8 arg0) {
    if (D_800D366C != 0) {
        BattlePlaySoundWithBaseId(arg0);
    }
}
void BattleParseNumberToDigits(u32 arg0) {
    s32 i;
    u32 divisor;

    divisor = 100000000;
    for (i = 0; i < 9; i++) {
        D_800C3CF4[i] = arg0 / divisor;
        arg0 %= divisor;
        divisor /= 10;
    }
    for (i = 1; i < 9; i++) {
        if (D_800C3CF4[i] == 0) {
            D_800C3CF4[i - 1] = 0xFF;
        } else {
            if (D_800C3CF4[i - 1] == 0) {
                D_800C3CF4[i - 1] = 0xFF;
            }
            break;
        }
    }
}
void BattleSetArchive20Entry0(void) {
    ArchiveSetIndex(0x20, 0);
}
void BattleSetArchive20Entry2(void) {
    ArchiveSetIndex(0x20, 2);
}
void BattleSetArchive20Entry3(void) {
    ArchiveSetIndex(0x20, 3);
}
s32 BattleAllocHeapBlock(s32 arg0, s32 arg1) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc(arg0, arg1);
}
s32 BattleAllocTextBuffer(s32 arg0) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc((arg0 + 3) * 26, 0);
}
void BattleWaitForArchiveSync(void) {
    while (ArchiveDataSync() != 0) {
        BattleUpdateFrame();
    }
}
void BattleQueueCommandF3WithSlotMask(u16 arg0, u8 arg1) {
    u16 mask;
    s32 i;
    s32 off;
    u8 idx;

    mask = arg0 & 0xFFF8;
    for (i = 0, off = 0x450; i < 8; i++) {
        idx = i + 3;
        if (BattleMaskHasSlotBit(mask, idx) != 0) {
            if ((*(u16 *)(D_800CCD1C + off) & 0x800) == 0) {
                mask &= BattleSlotBitMaskInverted(idx);
            }
        }
        off += 0x170;
    }
    if (mask == 0) {
        return;
    }
    D_800C400B[D_800C3EAC->count * 72] = arg1;
    D_800C402F[D_800C3EAC->count * 72] = 0xF3;
    D_800C4022[D_800C3EAC->count * 36] = mask;
    D_800C3EAC->count = D_800C3EAC->count + 1;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008ADD0);
/* Best: 98.37% (205 insns vs the original's 206 -- ONE instruction short, so it
   cannot link yet; the 96.69% park was 204). The lever that produced the pointer
   register retail wants: assign `new_var = &D_800D2C94;` BEFORE the
   D_800D32A1 branch and copy it (`pMask = new_var;`) at the use site. Without
   that early assignment gcc 2.6 substitutes the constant address into every use
   (no saved register, 3 saved regs instead of retail's 4 s0-s3, 204 insns).
   What is left is only WHERE the address is formed, now that the register exists:
   retail puts `nop; bnez` after the D_800D32A1 load and forms s1 (= &D_800D2C94)
   AT the first use (BattleBeginAnimTask's argument); gcc forms s3 two
   instructions earlier, filling the slots retail leaves as `nop`, and so the two
   straight-line uses read 0(s3) instead of retail's fresh lui/addiu+lhu.
   Also still off: `li v1,0xff / li v0,0x8b8` comes out swapped (value/off tie),
   `li <i>,0x3` sits one call later than retail's BattleClearSlotCurveEntry
   delay slot, and the saved-register numbering is s3=pointer where retail has
   s1 (retail s3=arg0, s2=i). Permuter run from the 96.69% body found the
   new_var lever (base=1011 best=472); a second run from THIS body is the next
   step. Already ruled out: -G8/-G0 preset (99.73% under -G0, still short),
   u16 pointer, register and volatile pMask, pMask[0], moving the
   assignment site, -O1/-O3. */
/*
void func_8008ADD0(u8 arg0) {
    s32 off;
    s32 i;
    u16 mask;
    u16 sel;
    u8 value;
    u16 *new_var;
    u16 *pMask;

    D_800D366C = 0;
    func_800BCD98(0);
    value = 0xFF;
    for (off = 0x8B8; off >= 0; off -= 0x48) {
        D_800C402F[off] = value;
    }
    new_var = &D_800D2C94;
    if (D_800D32A1[arg0 * 8] == 0) {
        D_800C3EAC->unk2DC = D_800C3EAC->unk2E6 + 0x17;
        off = (D_800C3EAC->unk2E6 + 0x16) * 40 + arg0 * 1520;
        sel = *(u16 *)(D_800CDD40 + off) & 7;
    } else {
        D_800C3EAC->unk2DC = D_800C3EAC->unk2E6 + 0x16;
        off = (D_800C3EAC->unk2E6 + 0x15) * 40 + arg0 * 1680;
        sel = *(u16 *)(D_800CEF10 + off) & 7;
    }
    if (sel != 0) {
        mask = D_800C3D64;
    } else {
        mask = BattleSlotBitMask(D_800C3E2C);
    }
    pMask = new_var;
    BattleQueueCommandF3WithSlotMask(mask, arg0);
    BattleClearSlotCurveEntry();
    BattleBeginActorAction(arg0, mask, D_800C3EAC->unk2DC - 1);
    BattleApplySlotCurveAndStep(D_800C3EAC->count);
    mask = BattleSlotBitMask(arg0);
    BattleBeginAnimTask(*pMask | mask);
    func_800B89FC(1, arg0, D_800C3EAC->slots[arg0].unk3C, BattleFindMinValueOtherSlot(arg0));
    D_800C402F[D_800C3EAC->count * 72] = D_800D2C98;
    D_800C400B[D_800C3EAC->count * 72] = arg0;
    *(u16 *)(D_800C3FFE + D_800C3EAC->count * 72) = *pMask;
    for (i = 3; i < 11; i++) {
        if (BattleMaskHasSlotBit(*pMask, i) != 0) {
            func_80079840(arg0, i);
        }
    }
    D_800C3EAC->count = D_800C3EAC->count + 1;
    BattleQueueCommandFE(arg0);
    while (D_800C3EAC->unk2DB == 0) {
        BattleUpdateFrame();
    }
}
*/

void BattleClearPrimFlagsFourSlots(u8 arg0) {
    D_800D2D28->unk9C = D_800D2D28->unk9D = D_800D2D28->unk9E = 0;
    D_800D2D28->unkB0[0] = D_800D2D28->unkB0[1] = D_800D2D28->unkB0[2] = D_800D2D28->unkB0[3] = 0;
    D_800D2D28->unkB7 = 0;
    if (arg0 == 0) {
        D_800D2D28->unkCB = 1;
    }
}
void BattleSetPrimFlagsFourSlots(u8 arg0) {
    u16 value;

    D_800D2D28->unk9C = D_800D2D28->unk9D = D_800D2D28->unk9E = 1;
    D_800D2D28->unkB0[0] = D_800D2D28->unkB0[1] = D_800D2D28->unkB0[2] = D_800D2D28->unkB0[3] = 1;
    D_800D2D28->unkB7 = 1;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask(D_800C3EAC->slots[arg0].unk3C);
    BattleBeginAnimTask(value);
    func_800BCD98(BattleSlotBitMask(D_800C3EAC->slots[arg0].unk3C));
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B224);
/* Best: 89.19% (584 bytes vs the original's 596). Same calls, same branch tree,
   same register set (s0=arg0, s1=arg0*0x170 offset, s2=inParty, s3=count,
   s4=entry, s5=canAct, s6=result); what is left is two shapes:
   1. the D_800CCD38 accesses. Retail keeps only the OFFSET in s1 and re-folds
      `lui at,%hi(D_800CCD38); addu at,at,s1; lhu/sh %lo(at)` at each of the
      three sites; gcc forms base+offset once into s1 (`lui/addiu` + `addu`).
      Tried: three separate full spellings, an s32 off local, a u32 off.
   2. the arg1 + arg2*2 sum: retail computes it into a1 (`addu a1,a1,v0`, the
      incoming arg1) before the offset sum; gcc leaves arg1 in t0 (`move
      t0,a1` at entry) and computes the call argument last.
   Everything else, including the asymmetric second branch (retail's else path
   is `if ((u16)BattleMaskHasStatusBit(...) != 0) inParty = 1;`, not an
   assignment -- that asymmetry is what stops gcc cross-jumping the two calls),
   already matches instruction for instruction. NEXT: permute this body. It
   also needs `#include "main/game.h"`, D_800CCD38/D_800D2D24 and
   BattleMaskHasStatusBit, which are declared above. */
/*
s32 func_8008B224(u8 arg0, u8 arg1, u8 arg2) {
    s32 result;
    u8 canAct;
    u8 inParty;
    u16 entry;
    u8 count;
    s32 off;

    result = 0;
    canAct = 1;
    inParty = 0;
    if (D_800D32A1[arg0 * 8] == 0) {
        off = (arg2 * 2 + arg1 + 0x16) * 40 + arg0 * 1520;
        entry = *(u16 *)(D_800CDD40 + off);
        count = *(u8 *)(D_800CDD40 + off + 0x13);
        inParty = (u16)BattleMaskHasStatusBit((u16)g_GameState.unk16BC[D_800D2D24[arg0]].unk6, arg1 + arg2 * 2) != 0;
    } else {
        off = (arg2 * 2 + arg1 + 0x15) * 40 + arg0 * 1680;
        entry = *(u16 *)(D_800CEF10 + off);
        count = *(u8 *)(D_800CEF10 + off + 0x13);
        if ((u16)BattleMaskHasStatusBit((u16)g_GameState.unk16BC[D_800D2D24[arg0]].unkA, arg1 + arg2 * 2) != 0) {
            inParty = 1;
        }
    }
    if (*(u16 *)(D_800CCD38 + arg0 * 0x170) >= count && inParty != 0) {
        BattleClearPrimFlagsFourSlots(1);
        D_800D2D28->unkC6 = 1;
        if (func_80085084(entry, arg0, 0) != 0) {
            *(u16 *)(D_800CCD38 + arg0 * 0x170) -= count;
            result = 1;
        } else {
            D_800D2D28->unkC6 = 0;
            BattleSetPrimFlagsFourSlots(arg0);
        }
        canAct = 0;
    }
    if (canAct != 0) {
        BattlePlaySoundIfEnabled(0x4F);
    }
    return result;
}
*/
extern u8 D_800D3014;
extern u8 D_800D39D4;
extern s32 D_800D3288;
extern u8 func_8008B224(u8 arg0, u8 arg1, u8 arg2);
extern void func_8008F8F4(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4, u8 arg5, u8 arg6);
extern void func_80091064(u8 arg0);
extern void func_80077698(void);
extern void BattleDrawCharStatusIfFlagged(u8 arg0, char arg1, u8 arg2);
extern void BattleBuildSpriteColumn8(s32 arg0);
extern void func_80091EC4(s32 arg0);
extern void BattleStepAnimatedCursorPrim(s32 arg0, s32 arg1, s32 *pIndex, u8 *pCount);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B478);
/* Best: 98.42% (1176 bytes vs the original's 1164). Written from scratch this
   round; everything matches instruction for instruction EXCEPT two shapes:
   1. the fall-through state body (case 4 of the switch on D_800D39D4). Retail
      keeps the compare's `(j - 1) * 16` value in $v0 across the `j--` and reuses
      it for the shared `sw $v0, D_800D3288` at .L8008B878; gcc 2.6 rematerialises
      it in that one path (`andi v0,(u8)j; sll v0,v0,4`), which is the 2 extra
      instructions and the 12-byte oversize. Cases 1..3 of the same switch reuse
      the value fine (they reach the store through a `j`), so it is the
      fall-through that makes gcc remat -- a `s32 scrollY` local for the body
      made it worse (1192). Tried: `D_800D3288 = (j - 1) * 16` before the
      decrement (sw then precedes the addiu, 1192), a scrollY local (1192),
      `(j - 1 + 1) * 16` (1176, same).
   2. the i/j register tie: retail keeps i in $s2 and j in $s3, gcc gives i $s3.
      Everything else (s4=result, s5=arg0, s8=prevI, s6=prevJ, s7=prevY, s0=1,
      s1=0x18, the 0x50 frame, both switches' case order 5,4,0,2,1,3 and 1,2,3,4,
      the andi placement, all delay slots) is identical. Permuter run from this
      body found nothing (base=439 best=439, 898 iters).
   Lessons already applied here: case bodies must be listed in TEXT order
   (5,4,0,2,1,3 -- gcc 2.6 emits them in source order, confirmed against
   BattleSelectTargetSlotLoop); each guard is written `if (x == N) { state }
   else { index }` so gcc branches on the false arm; `s32 iv = i` is required to
   get retail's `andi a1,(u8)i` before `andi v0,a1,1`, and the coordinates need
   the two-term spelling `(iv & 1) * 128 + (iv & 1) * 4 + 0x2A` for retail's
   128+4 shift pair; the state bodies need `j` changed before `i`. */
/*
u8 func_8008B478(u8 arg0) {
    u8 j;
    u8 i;
    u8 result;
    u8 prevI;
    u8 prevJ;
    s32 prevY;
    s32 primIndex;
    u8 primCount;
    u32 iv;

    i = 0;
    j = 0;
    prevI = 0xFF;
    prevJ = 0xFF;
    prevY = 0xFFFF;
    result = 2;
    primIndex = 4;
    primCount = 0;
    D_800D3288 = 0;
    D_800D39D4 = 0;
    D_800D2D28->unkCB = 0;
    func_8008F8F4(0, 0x1C, 0xA0, 0xAC, 0x38, 0, 1);
    func_8008F8F4(1, 0x20, 0x2C, 0x118, 0x70, 0, 1);
    func_8008F8F4(2, 0xD0, 0xA4, 0x50, 0x18, 0, 1);
    func_8008F8F4(3, 0xE6, 0xC0, 0x4C, 0x18, 0, 1);
    func_80091064(arg0);
    func_80077698();
    do {
        if (i != prevI || j != prevJ) {
            BattleDrawCharStatusIfFlagged(arg0, i, j);
            prevI = i;
            prevJ = j;
        }
        if (D_800D3288 != prevY) {
            BattleBuildSpriteColumn8(D_800D3288);
            func_80091EC4(D_800D3288);
            prevY = D_800D3288;
        }
        iv = i;
        BattleStepAnimatedCursorPrim((iv & 1) * 128 + (iv & 1) * 4 + 0x2A, (iv >> 1) * 16 + 0x38, &primIndex, &primCount);
        BattleUpdateFrame();
        switch (D_800D3014) {
        case 5:
            result = 0;
            break;
        case 4:
            if (func_8008B224(arg0, i, j) != 0) {
                result = 1;
                D_800C3EAC->unk2E6 = i + j * 2;
            }
            break;
        case 0:
            if (j * 2 + i != 0xF) {
                if (i + 1 == 0xC) {
                    D_800D39D4 = 1;
                } else {
                    i++;
                }
            }
            break;
        case 2:
            if (j * 2 + i > 0) {
                if (i == 0 && j != 0) {
                    D_800D39D4 = 2;
                } else {
                    i--;
                }
            }
            break;
        case 1:
            if (j * 2 + i < 0xE) {
                if (i + 2 >= 0xC) {
                    D_800D39D4 = 3;
                } else {
                    i += 2;
                }
            }
            break;
        case 3:
            if (j * 2 + i >= 2) {
                if (i < 2 && j != 0) {
                    D_800D39D4 = 4;
                } else {
                    i -= 2;
                }
            }
            break;
        }
        switch (D_800D39D4) {
        case 1:
            if (D_800D3288 >= (j + 1) * 16) {
                j++;
                i--;
                D_800D3288 = j * 16;
                D_800D39D4 = 0;
            }
            break;
        case 2:
            if (D_800D3288 < (j - 1) * 16) {
                j--;
                i++;
                D_800D3288 = j * 16;
                D_800D39D4 = 0;
            }
            break;
        case 3:
            if (D_800D3288 >= (j + 1) * 16) {
                j++;
                D_800D3288 = j * 16;
                D_800D39D4 = 0;
            }
            break;
        case 4:
            if (D_800D3288 < (j - 1) * 16) {
                j--;
                D_800D3288 = (j - 1 + 1) * 16;
                D_800D39D4 = 0;
            }
            break;
        }
    } while (result == 2);
    BattleClearPrimFlagsFourSlots(result);
    BattleFreePacketPrimitives();
    BattleClearFlagC6();
    func_8008FA60(0);
    func_8008FA60(1);
    func_8008FA60(2);
    func_8008FA60(3);
    return result;
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B908);
void BattleClearPrimFlagsTwoSlots(u8 arg0) {
    D_800D2D28->unk9C = D_800D2D28->unk9D = D_800D2D28->unk9E = 0;
    D_800D2D28->unkB0[0] = D_800D2D28->unkB0[1] = 0;
    D_800D2D28->unkB7 = 0;
    if (arg0 == 0) {
        D_800D2D28->unkCB = 1;
    }
}
void BattleSetPrimFlagsTwoSlots(u8 arg0) {
    u16 value;

    D_800D2D28->unk9C = D_800D2D28->unk9D = D_800D2D28->unk9E = 1;
    D_800D2D28->unkB0[0] = D_800D2D28->unkB0[1] = 1;
    D_800D2D28->unkB7 = 2;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask(D_800C3EAC->slots[arg0].unk3C);
    BattleBeginAnimTask(value);
    func_800BCD98(BattleSlotBitMask(D_800C3EAC->slots[arg0].unk3C));
}
s32 BattleActivateTwoSlotCommand(u8 arg0, u8 arg1, u8 arg2, u8 arg3) {
    u8 value;
    u16 entry;
    s32 result;

    result = 0;
    entry = D_800D2204[D_800D2CE0[arg2 * 2 + arg1]].unk0;
    if (arg3 == 0) {
        value = D_800D2CE0[arg2 * 2 + arg1];
    } else {
        value = D_800C3D70[arg2 * 2 + arg1];
    }
    if (value != 0) {
        if (arg3 == 0) {
            BattleClearPrimFlagsTwoSlots(1);
            D_800D2D28->unkC6 = 1;
            if (func_80085084(entry, arg0, 0) != 0) {
                result = 1;
            } else {
                D_800D2D28->unkC6 = 0;
                BattleSetPrimFlagsTwoSlots(arg0);
            }
        } else if (BattleIsCharacterInParty(value - 0x32) != 0) {
            func_8009A854(value);
            value = D_800D3688[arg2 * 2 + arg1] - 1;
            D_800D3688[arg2 * 2 + arg1] = value;
            if (value == 0) {
                D_800C3D70[arg2 * 2 + arg1] = 0;
            }
            result = 2;
        }
    } else {
        BattlePlaySoundIfEnabled(0x4F);
    }
    return result;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BED8);
void BattleFreeSlotPrims(u8 arg0) {
    D_800D2D28->unk9C = D_800D2D28->unk9D = D_800D2D28->unk9E = 0;
    D_800D2D28->unkB7 = 0;
    if (arg0 != 0) {
        D_800D2D28->unkC6 = 0;
        func_8008FA60(0);
        func_8008FA60(1);
        BattleUpdateFrame();
        BattleFreePacketPrimitives();
        BattleClearFlagC6();
    } else {
        D_800D2D28->unkB0[0] = D_800D2D28->unkB0[1] = 0;
    }
}
void func_8008C3F0(u8 arg0) {
    u16 value;

    D_800D2D28->unk9C = D_800D2D28->unk9D = D_800D2D28->unk9E = 1;
    D_800D2D28->unkB7 = 3;
    D_800D2D28->unkB0[0] = D_800D2D28->unkB0[1] = 1;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask(D_800C3EAC->slots[arg0].unk3C);
    BattleBeginAnimTask(value);
    func_800BCD98(BattleSlotBitMask(D_800C3EAC->slots[arg0].unk3C));
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C4A8);
/* Best: 88.52% (932 bytes vs the original's 884). Same calls, same branch tree,
   same instruction multiset in every block except two shapes:
   1. loop-invariant base hoisting. Retail re-forms each of the three command
      table bases inside the loop (`lui at,%hi(D_800C402F); addu at,at,v1;
      sb %lo(D_800C402F)(at)`); gcc 2.6 hoists all three symbols into s3/s4/s5 in
      the loop preheader (L8008C66C) and reaches them with `addu v0,v0,sN`,
      which also grows the frame from 0x28 to 0x38. The same expression outside
      the loop compiles to retail's at-form, so the source spelling is right and
      it is the `loop` pass doing it. Tried: a `do { } while (0);` block around
      the three stores (no change), three separate index locals (loads of
      D_800C3EAC->count drop from three to one), `int slotStride = 0x40;`
      (fixes the addu operand order of item 2 but makes the tail index a real
      `mult` and costs 4 more registers), pointer locals.
   2. addu operand order at three sites: retail `addu v0,v0,s0` (base first),
      gcc `addu v0,s0,v0`. `(D_800C3EAC + arg0 * slotStride)[0x3C]` with a
      variable stride gives retail's order but breaks item 1 further (see above);
      with the 0x40 literal the order is wrong at exactly the three sites where
      the pointer lands in $v0 rather than $a0. Both `D_800C3EAC[arg0 * 0x40 +
      0x3C]` and `(D_800C3EAC + arg0 * 0x40)[0x3C]` measured identical.
   Everything else matches, including the shared epilogue, `D_800D2D28->unkCB =
   result` / `D_800C4928 = result` both storing $s3 (the source has to store the
   `result` local there, not the literal 1), the `sb` in BattleFreeSlotPrims's
   jal delay slot, the `addiu v0,v0,0xFF` (255, not -1) form of `x - 1`, and the
   `slti/bnez` for-loop tail. `make check` was NOT run on the parked body.
   Round 124: D_800C3EAC and D_800D2D28 are typed now (BattleCommandQueue /
   BattleDisplayState at the top of this file), so the body below compiles as
   written; the typed slot spelling `D_800C3EAC->slots[arg0].unk3C` is proven
   byte-exact by the four *PrimFlags*Slots setters that use it.
   NEXT: permute this body. */
/*
s32 func_8008C4A8(u8 arg0) {
    s32 result;
    s32 i;

    result = 1;
    if ((func_8008C81C(arg0) & 0xFF) != 0) {
        BattleNoOpCallback(arg0, 0x100);
        BattleFreeSlotPrims(1);
        D_800D2D28->unkCB = result;
        result = 0;
    } else {
        BattleNoOpCallback(arg0, 0x80);
        D_800D2D28->unkAF = 0;
        BattleFreeSlotPrims(1);
        D_800D366C = 0;
        func_800BCD98(0);
        D_800C3EAC->slots[arg0].unk3C = D_800C3E2C;
        func_800877E0(arg0, D_800C3EAC->slots[arg0].unk3C);
        D_800C4928 = result;
        BattleSetupActorSlotDisplay(arg0);
        if ((*(u16 *)(D_800CCD1C + D_800C3EAC->slots[arg0].unk3C * 0x170) & 0x800) != 0) {
            D_800C400B[D_800C3EAC->count * 72] = arg0;
            D_800C402F[D_800C3EAC->count * 72] = 0xF3;
            D_800C4022[D_800C3EAC->count * 36] = BattleSlotBitMask(D_800C3EAC->slots[arg0].unk3C);
            D_800C3EAC->count = D_800C3EAC->count + 1;
        }
        for (i = 0; i < 7; i++) {
            if (D_800C3EAC->unk2C4[i] != 0xFF) {
                D_800C3EAC->unk2DC = D_800C3EAC->unk2C4[i] + 8;
                BattleBeginActorAction(arg0, BattleSlotBitMask(D_800C3EAC->slots[arg0].unk3C), D_800C3EAC->unk2DC - 1);
                BattleApplySlotCurveAndStep(D_800C3EAC->count);
                D_800C402F[D_800C3EAC->count * 72] = D_800C3EAC->unk2DC - 1;
                D_800C400B[D_800C3EAC->count * 72] = arg0;
                *(u16 *)(D_800C3FFE + D_800C3EAC->count * 72) = D_800D2C94;
                D_800C3EAC->count = D_800C3EAC->count + 1;
            }
        }
        BattleQueueCommandFE(arg0);
        func_80087EDC(arg0, D_800C3EAC->slots[arg0].unk3C);
        while (D_800C3EAC->unk2DB == 0) {
            BattleUpdateFrame();
        }
    }
    return result;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C81C);
void BattleClearPrimFlagsThreeSlots(u8 arg0) {
    D_800D2D28->unk9C = D_800D2D28->unk9D = D_800D2D28->unk9E = 0;
    D_800D2D28->unkB0[0] = D_800D2D28->unkB0[1] = D_800D2D28->unkB0[2] = 0;
    D_800D2D28->unkB7 = 0;
    if (arg0 == 0) {
        D_800D2D28->unkCB = 1;
    }
}
void BattleSetPrimFlagsThreeSlots(u8 arg0) {
    u16 value;

    D_800D2D28->unk9C = D_800D2D28->unk9D = D_800D2D28->unk9E = 1;
    D_800D2D28->unkB0[0] = D_800D2D28->unkB0[1] = D_800D2D28->unkB0[2] = 1;
    D_800D2D28->unkB7 = 4;
    value = BattleSlotBitMask(arg0) | BattleSlotBitMask(D_800C3EAC->slots[arg0].unk3C);
    BattleBeginAnimTask(value);
    func_800BCD98(BattleSlotBitMask(D_800C3EAC->slots[arg0].unk3C));
}

