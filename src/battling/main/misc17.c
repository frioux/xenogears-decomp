#include "common.h"
#include "psyq/libgpu.h"

extern s32 D_80059488;
extern s32 D_8009284C;
extern u8 D_800928A0;
extern u8 D_80096DE0[];
extern u8 D_80096E00[];

typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u16 unk4;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u32 unk8;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u32 unk10[4];
    /* 0x20 */ u32* unk20;
} BattleSlot; // Size: 0x24

typedef struct {
    /* 0x0 */ u32 w[4];
} SlotData; // Size: 0x10

extern BattleSlot D_80096EA0[];
extern BattleSlot D_80096F0C[];

extern u16 D_80096EA4;
extern u16 D_80096EC8;
extern u16 D_80096EEC;
extern u16 D_80096F10;

extern u8 D_80091EE0[];
extern u8 D_80091F60[];
extern u8 D_80091F70[];
extern u8 D_80091F80[];
extern u8 D_80091F90[];
extern u8 D_80091FA0[];

extern void func_8007639C(u8*, u8);
extern void func_800767C8(u8*, u8*);
extern void func_8008E78C(u8*, u32, u8*, u32);
extern s32 func_8008B650(s32, s16, s16);
extern void SoundStopAllElements(void);
extern u32 func_8003A5D0(u32);
extern void func_8003A20C(u8);
extern void func_8008FE80(u8*, u8*, s32);
extern void func_80090E10(u8*);
extern void SoundPlaySoundEffectOnPairWithLevels(s32, s32, s32, s32);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E120);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E2B8);
/* best: objdiff 89.48% - first half exact; retail expands the `flag ? 2 : 1` argument as an if/else that repeats
   `addu a0,zero,zero` in both arms and joins with a jump, and re-forms &D_80096E00 with a fresh lui/addiu for the
   second AddPrim instead of keeping it in a callee-saved register.
void func_8008E2B8(u8* p0, s32 color, s32 flag) {
    u8* pPkt;
    u_short tpage;

    pPkt = D_80096DE0 + D_800928A0 * 0x10;
    *(u32*)(pPkt + 0x4) = (color | (color << 8) | (color << 16)) | 0x60000000;
    *(u8*)(pPkt + 0x7) |= 2;
    *(u8*)(pPkt + 0x3) = 3;
    *(u32*)(pPkt + 0x8) = 0;
    *(u32*)(pPkt + 0xC) = 0xDA0140;
    AddPrim(p0, pPkt);
    tpage = GetTPage(0, flag ? 2 : 1, 0, 0);
    SetDrawMode((DR_MODE*)(D_80096E00 + D_800928A0 * 0xC), 0, 1, tpage, 0);
    AddPrim(p0, D_80096E00 + D_800928A0 * 0xC);
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E3CC);
/* best: objdiff 99.39% - only difference: gcc folds the address of the loop body's last store into the induction pointer (base = &unk2, offsets 10/-1/2/0) instead of using &unk0 with 12/1/4/2. Statement order does not change the choice; the base always follows the last store. An indexed form (D_80096EA0[i].unkC) loses the strength reduction entirely (45%).
void BattleInitSoundSlots(void) {
    BattleSlot* pSlot;
    s32 timer;
    u32 i;

    timer = 0x300;
    SoundStopAllElements();
    pSlot = D_80096EA0;
    for (i = 0; i < 4; i++) {
        pSlot->unkC = timer;
        timer <<= 2;
        pSlot->unk1 = 0;
        pSlot->unk4 = 0;
        pSlot->unk2 = i * 2;
        pSlot++;
    }
}*/
void BattleInitSoundSlots(void) {
    BattleSlot* pSlot;
    s32 timer;
    u32 i;

    timer = 0x300;
    SoundStopAllElements();
    pSlot = D_80096EA0;
    for (i = 0; i < 4; i++) {
        pSlot[i].unkC = timer;
        timer <<= 2;
        pSlot[i].unk1 = 0;
        pSlot[i].unk4 = 0;
        pSlot[i].unk2 = i * 2;
    }
}
void BattleIncrementSoundSlotTimers(void) {
    if (D_80096EA0[0].unk4 != 0xFFFF) {
        D_80096EA0[0].unk4++;
    }
    if (D_80096EC8 != 0xFFFF) {
        D_80096EC8++;
    }
    if (D_80096EEC != 0xFFFF) {
        D_80096EEC++;
    }
    if (D_80096F10 != 0xFFFF) {
        D_80096F10++;
    }
}
u8* BattleSetActorSoundTable(u8* p) {
    u8* pTable;

    switch (*(u8*)(p + 0x909)) {
    case 9:
        pTable = D_80091FA0;
        break;
    case 0x1D:
        pTable = D_80091F60;
        break;
    case 0x1B:
        pTable = D_80091F80;
        break;
    case 0x24:
        pTable = D_80091F70;
        break;
    default:
        pTable = D_80091F90;
        break;
    }
    *(u8**)(p + 0x1664) = pTable;
    return pTable;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E78C);
/* best: objdiff 99.25% - instruction multiset, size (292) and block layout identical; only $3 and $4 are
   swapped (retail: biv in v1, lhu temp in a0; gcc: biv in a0, temp in v1). The loop must be written with
   `pSlot = &D_80096F0C[i - 3]` (single biv, counter kept, base derived as `addiu v1,s0,-108`); the plain
   `pSlot[i]` form makes gcc replace the counter with an end-pointer compare, and `pSlot++` splits into two
   bivs. Tried: 5 local declaration orders, `best < unk4` vs `unk4 > best`, indexed vs pointer, struct copy
   of the 4 words (needed - it gives the retail load-load-load-load/store x4).
void func_8008E78C(u8* p0, u32 idx, u8* pData, u32 flags) {
    BattleSlot* pSlot;
    BattleSlot* pSel;
    u32* pSrc;
    s32 best;
    s32 i;

    best = 0;
    pSel = D_80096F0C;
    pSlot = &D_80096F0C[-3];
    for (i = 0; i < 4; i++) {
        pSlot = &D_80096F0C[i - 3];
        if (pSlot->unk1 == 0 || (idx == 0 && pSlot->unk0 == 0)) {
            pSel = pSlot;
            break;
        }
        if (best < pSlot->unk4) {
            best = pSlot->unk4;
            pSel = pSlot;
        }
    }
    BattleIncrementSoundSlotTimers();
    pSlot = pSel;
    pSlot->unk0 = idx;
    pSlot->unk8 = (u32)p0;
    pSlot->unk1 = 1;
    pSlot->unk3 = flags;
    pSlot->unk20 = (u32*)pData;
    if (pData != 0) {
        pSrc = (u32*)pData;
        *(SlotData*)pSlot->unk10 = *(SlotData*)pSrc;
    }
    pSlot->unk4 = 0;
    if (idx == 0) {
        SoundPlaySoundEffectOnPairWithLevels(pSlot->unk8, pSlot->unk2, 0x7F, 0x40);
    }
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E8B0);
void BattleReleaseFinishedSoundSlots(void) {
    BattleSlot* pSlot;
    s32 i;

    pSlot = D_80096EA0;
    for (i = 0; i < 4; i++) {
        if ((func_8003A5D0(pSlot[i].unk8) & pSlot[i].unkC) == 0) {
            pSlot[i].unk1 = 0;
        }
    }
    BattleIncrementSoundSlotTimers();
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008EB4C);
/* best: objdiff 73.33% - identical instruction multiset; retail emits lui a0,(0x60000 >> 16), then the arg-4
   global load, then addu a0,v0,a0 and addu a1,zero,zero; gcc 2.7.2 emits the lui/addu pair and the a1 zero
   before the arg-4 load instead. Tried: `(u8*)((s32)p + 0x60000)`, `&p[0x60000]`, `p + 0x60000`,
   `D_80059488[0]` (array decl), locals for the global and for the constant - all 73.33%.
void func_8008EB4C(u8* p) {
    if (p != NULL) {
        func_8008E78C(p + 0x60000, 0, NULL, D_80059488);
    }
}*/
void BattlePlaySoundEffectByIdx(u8* p, s32 idx, u8* pData, s32 flags) {
    if (idx != 0) {
        func_8008E78C((u8*)(idx + 0x60000), flags, pData,
                      (idx & 0x7F) | ((*(u32*)(p + 0xD0) >> 20) & 0x80));
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008EBD0);
/* best: objdiff 89.69% - size, frame (0x30, `s32 pad[2]` supplies retail's extra 8 bytes), branch layout and the
   whole instruction multiset match. Only difference: register numbering. Retail puts the table base in s0 and the
   byte index in s1 and the params in s2/s3/s4 (p/flags/pData); gcc gives p s0, i s1, flags s2, pData s3, base s4,
   so all five saves and the four-arg call's moves come out rotated. `u32 b` (not u8) is needed or gcc adds an
   `andi v1,v0,0xff` after each lbu. Also tried: gcc-2.6.0 (89.7), statement order, one table read only.
void func_8008EBD0(u8* p, s32 idx, u8* pData, s32 flags) {
    s32 pad[2];
    u8* pTable;
    s32 i;
    u32 b;

    if (idx == 0) {
        idx = rand() % 3 + 1;
    }
    b = *(u8*)(*(u32*)(p + 0x1664) + idx);
    if (b == 0xFF) {
        return;
    }
    i = b << 1;
    b = D_80091EE0[i];
    pTable = D_80091EE0;
    if (b != 0) {
        func_8008E78C((u8*)(b | 0x60000), flags, pData,
                      ((*(u32*)(p + 0xD0) >> 20) & 0x80) | (b & 0x7F));
    }
    b = pTable[i + 1];
    if (b != 0) {
        func_8008E78C((u8*)(b | 0x60000), flags, pData,
                      ((*(u32*)(p + 0xD0) >> 20) & 0x80) | (b & 0x7F));
    }
}*/
void BattleStopSoundSlotsByFlags(u8 kind) {
    BattleSlot* pSlot;
    s32 i;

    pSlot = D_80096EA0;
    for (i = 0; i < 4; i++) {
        if (pSlot[i].unk1 != 0 && pSlot[i].unk3 == kind) {
            func_8003A20C(pSlot[i].unk2);
            pSlot[i].unk1 = 0;
        }
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008ED6C);
/* best: objdiff 90.34% - everything but two things matches, including the 0x28 frame (the `s32 pad[2]` reproduces
   retail's extra 8 bytes of locals) and the $at-vs-base-pointer split of the two table reads. Left: retail keeps
   &D_80091EE0 in s1 and the byte index in s2; gcc gives the index s1 and the base s2. Also retail's second block
   emits `lw a0,208(s0)` before `andi v0,v0,0x7f`, gcc the reverse. Tried: reading D_80091EE0[] directly for both
   (68.98%), pTable assignment before/after `i = v << 1` (no change), -G0 and gcc-2.7.2-cdk (76.41%).
void func_8008ED6C(u8* p, s32 idx) {
    s32 pad[2];
    u8* pTable;
    s32 i;
    u8 v;

    v = *(u8*)(*(u32*)(p + 0x1664) + idx);
    if (v == 0xFF) {
        return;
    }
    i = v << 1;
    pTable = D_80091EE0;
    if (D_80091EE0[i] != 0) {
        BattleStopSoundSlotsByFlags(((*(u32*)(p + 0xD0) >> 20) & 0x80) | (D_80091EE0[i] & 0x7F));
    }
    if (pTable[i + 1] != 0) {
        BattleStopSoundSlotsByFlags(((*(u32*)(p + 0xD0) >> 20) & 0x80) | (pTable[i + 1] & 0x7F));
    }
}*/
void BattleUpdateActorApproach(u8* p, s16 arg1, s16 arg2) {
    s32 pad[4];
    s32 v98;
    s32 v9C;
    s32 flag;
    s32 i;

    v98 = *(s32*)(p + 0x98);
    v9C = *(s32*)(p + 0x9C);
    if (*(u32*)(p + 0xD0) & 0x100) {
        v9C = v9C * 2 / 3;
        flag = 0;
    } else {
        flag = 1;
    }
    for (i = 0; i < 2; i++) {
        if (arg2 != 0 && flag != 0) {
            *(s32*)(p + 0x48) += v98;
            if (*(s32*)(p + 0x48) > arg2) {
                *(s32*)(p + 0x48) = arg2;
            }
        } else {
            *(s32*)(p + 0x48) -= v9C;
            if (*(s32*)(p + 0x48) < 0) {
                *(s32*)(p + 0x48) = 0;
            }
        }
    }
    *(s32*)(p + 0x58) = func_8008B650(*(s32*)(p + 0x58), arg1, 0x40);
    *(u16*)(p + 0xCE) = 0;
}
void BattleCheckStateAndSetDelay60(u8* p0, u8* p1) {
    func_800767C8(p0, p1);
    *(u16*)(p1 + 4) = 0x3C;
}
void BattleQueueRandomMessagesAndDelay(u8* p0, u8* p1, s32 flag) {
    func_8008FE80(p0, p1, flag);
    *(u16*)(p1 + 4) = flag ? 0x1E : 0xA;
}
void BattleQueueMessage3AndDelay60(u8* p0, u8* p1) {
    func_8007639C(p0, 3);
    *(u16*)(p1 + 4) = 0x3C;
}
void BattleToggleActionFlag2000(u8* p0, u8* p1) {
    u32 flags;

    flags = *(u32*)(p1 + 0x2C);
    *(u32*)(p1 + 0x2C) = (flags & ~0x2000) | ((((flags >> 13) & 1) ^ 1) << 13);
    if (*(u32*)(p1 + 0x2C) & 0x2000) {
        *(u32*)(p0 + 0xD0) |= 2;
        *(u16*)(p1 + 4) = 0x3C;
    } else {
        *(u32*)(p0 + 0xD0) &= ~2;
        *(u32*)(p0 + 0xD0) &= ~0x38;
        *(u16*)(p1 + 4) = 0x1E;
    }
}
void BattleQueueMessages4Then3Delay60(u8* p0, u8* p1) {
    func_8007639C(p0, 4);
    func_8007639C(p0, 3);
    *(u16*)(p1 + 4) = 0x3C;
}
void BattleQueueMessage4AndDelay60(u8* p0, u8* p1) {
    func_8007639C(p0, 4);
    *(u16*)(p1 + 4) = 0x3C;
}
void BattleSetRandomFacingAround800(u8* p0, u8* p1) {
    u16 timer;

    if (D_8009284C >= 0x801) {
        *(u16*)(p1 + 0x4) = 1;
        *(u16*)(p1 + 0xC) = 0;
        return;
    }
    *(u16*)(p1 + 0x4) -= 1;
    if ((s16)*(u16*)(p1 + 0x4) != -1) {
        return;
    }
    *(u16*)(p1 + 0xA) = rand() % 0x600 + 0x500;
    timer = rand() % 50 + 0xA;
    *(u16*)(p1 + 0xC) = 0xFF;
    *(u16*)(p1 + 0x4) = timer;
}
void BattleSetRandomFacingAround0(u8* p0, u8* p1) {
    u16 timer;

    if (D_8009284C < 0x100) {
        *(u16*)(p1 + 0x4) = 1;
        *(u16*)(p1 + 0xC) = 0;
        return;
    }
    *(u16*)(p1 + 0x4) -= 1;
    if ((s16)*(u16*)(p1 + 0x4) != -1) {
        return;
    }
    *(u16*)(p1 + 0xA) = rand() % 0x600 - 0x300;
    timer = rand() % 120 + 0xA;
    *(u16*)(p1 + 0xC) = 0xFF;
    *(u16*)(p1 + 0x4) = timer;
}
void BattleSetActionFieldFAndDispatch(u8* p0, u8* p1, u8 value) {
    p1[0xF] = value;
    func_80090E10(p0);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008F280);
s32 BattleCheckValueAboveScaledMax(u8* p, s32 arg1) {
    return (*(s16*)(p + 0xBC) * arg1) / 255 < *(s16*)(p + 0xB4);
}
