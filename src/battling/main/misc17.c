#include "common.h"
#include "psyq/libgpu.h"

extern s32 D_80059488;
extern s32 D_8009283C;
extern s32 D_80092840;
extern s32 D_80092844;
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

// A battle participant as this unit drives it. Same layout as the BattleActor
// in src/battling/main/misc18.c; the fields this unit does not read stay
// opaque.
typedef struct BattleActor {
    /* 0x000 */ u8 unk0[0x48];
    /* 0x048 */ s32 unk48;
    /* 0x04C */ u8 unk4C[0xC];
    /* 0x058 */ s32 unk58;
    /* 0x05C */ u8 unk5C[0x3C];
    /* 0x098 */ s32 unk98;
    /* 0x09C */ s32 unk9C;
    /* 0x0A0 */ u8 unkA0[0x14];
    /* 0x0B4 */ s16 unkB4;
    /* 0x0B6 */ s16 unkB6;
    /* 0x0B8 */ u8 unkB8[0x4];
    /* 0x0BC */ s16 unkBC;
    /* 0x0BE */ s16 unkBE;
    /* 0x0C0 */ u8 unkC0[0xE];
    /* 0x0CE */ s16 unkCE;
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u8 unkD4[0x835];
    /* 0x909 */ u8 unk909;
    /* 0x90A */ u8 unk90A[0xD5A];
    /* 0x1664 */ u8* pSoundTable;
    /* 0x1668 */ u16 unk1668;
} BattleActor; // Size: 0x166A

// The flags word at +0x2C of a BattleWork (see src/battling/main/misc18.c).
typedef union {
    /* 0x0 */ u32 flags;
    /* 0x0 */ struct {
        /* 0x0 */ u32 unk2C_0 : 8;
        /* 0x0 */ u32 unk2C_8 : 1;
        /* 0x0 */ u32 unk2C_9 : 1;
        /* 0x0 */ u32 unk2C_10 : 1;
        /* 0x0 */ u32 unk2C_11 : 1;
        /* 0x0 */ u32 unk2C_12 : 1;
        /* 0x0 */ u32 unk2C_13 : 1;
        /* 0x0 */ u32 pad2C : 18;
    } bits;
    /* 0x0 */ struct {
        /* 0x0 */ u16 pad2C;
        /* 0x2 */ u8 unk2E;
        /* 0x3 */ u8 pad2F;
    } bytes;
} BattleWorkFlags; // Size: 0x4

// The per-slot action state the battle main loop runs on. +0x04 counts down to
// the next state change and is stored as a byte elsewhere, so it is read
// through a u16 here and compared signed.
typedef struct {
    /* 0x00 */ BattleActor* pOwner;
    /* 0x04 */ u16 unk4;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u8 unk8;
    /* 0x09 */ u8 unk9;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ u8 unkE;
    /* 0x0F */ u8 unkF;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ s32 unk20;
    /* 0x24 */ s32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ BattleWorkFlags unk2C;
    /* 0x30 */ u16 unk30;
    /* 0x32 */ u16 pad32;
} BattleWork; // Size: 0x34

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

extern void BattleQueueMessage(BattleActor*, s32);
extern void BattleTryQueueMessage5(BattleActor*, BattleWork*);
extern void BattleAcquireSoundSlot(u8*, u32, u8*, u32);
extern s32 func_8008B650(s32, s16, s16);
extern void SoundStopAllElements(void);
extern u32 func_8003A5D0(u32);
extern void SoundStopElementsByIndex(u8);
extern void BattleQueueRandomMessages(BattleActor*, BattleWork*, s32);
extern void BattleDispatchWorkState(BattleActor*);
extern void SoundPlaySoundEffectOnPairWithLevels(s32, s32, s32, s32);
extern void BattleEffectCopyBuffers(void);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E120);
/* best: objdiff 87.18% - 392 vs 320 bytes. Frame (0x30, s5 = mfhi), the
   five 16-bit grid taps, the /5 magic and the outer 3-IV tail all match.
   Still off by: (1) loop 1, retail fills the beq delay slot with a copy of
   the persistent value (`move v0,s2`) and stores through v0 in both rows,
   gcc stores s2 directly (tried: switch vs if/else-if, nested if, temp
   local); (2) loop 2, retail keeps the shared row offset in ELEMENT units
   (t4 = y*0x70, s1 = t4+1, sll by 1) and forms the five tap bases in the
   reverse of declaration order, gcc scales the offset into bytes and drops
   the sll (tried: inline y*0x70+1 per pointer, reversed declarations, plain
   indexed taps - 78-87%, the local `s32 off` + five base pointers is the
   best 87.18%).
void func_8008E120(void) {
    s16* pGridA;
    s16* pGridB;
    s16* pUpBase;
    s16* pMidRBase;
    s16* pMidLBase;
    s16* pDnRBase;
    s16* pDnLBase;
    s16* pRow;
    s32 value;
    s32 sum;
    s32 i;
    s32 x;
    s32 y;

    if (D_80092844 == 0) {
        return;
    }
    pGridB = (s16*)D_80092840;
    value = 0;
    pRow = pGridB + 0x1490;
    for (i = 0; i < 0x70; i++) {
        switch (rand() & 3) {
        case 0:
            value = 0x180;
            break;
        case 1:
            value = 0;
            break;
        }
        pRow[0x70] = value;
        pRow[0] = value;
        pRow++;
    }
    pGridA = (s16*)D_8009283C;
    pUpBase = pGridA - 0x70;
    pMidRBase = pGridA + 1;
    pMidLBase = pGridA - 1;
    pDnRBase = pGridA + 0x71;
    pDnLBase = pGridA + 0x6F;
    for (y = 0x2F; y >= 2; y--) {
        s32 off = y * 0x70 + 1;
        s16* pUp = pUpBase + off;
        s16* pMidR = pMidRBase + off;
        s16* pMidL = pMidLBase + off;
        s16* pDnR = pDnRBase + off;
        s16* pDnL = pDnLBase + off;
        s16* pOut = &pGridB[(y - 1) * 0x70 + 1];

        for (x = 1; x < 0x70; x++) {
            sum = *pUp++ + *pMidR++ + *pMidL++ + *pDnR++ + *pDnL++;
            value = sum / 5;
            if (value >= 4) {
                value -= 3;
            }
            *pOut++ = value;
        }
    }
    BattleEffectCopyBuffers();
}
*/
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
u8* BattleSetActorSoundTable(BattleActor* pActor) {
    u8* pTable;

    switch (pActor->unk909) {
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
    pActor->pSoundTable = pTable;
    return pTable;
}
void BattleAcquireSoundSlot(u8* p0, u32 idx, u8* pData, u32 flags) {
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
            do { pSel = pSlot; } while (0); // the block keeps the copy out of the beq delay slot
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
        BattleAcquireSoundSlot(p + 0x60000, 0, NULL, D_80059488);
    }
}*/
void BattlePlaySoundEffectByIdx(BattleActor* pActor, s32 idx, u8* pData, s32 flags) {
    if (idx != 0) {
        BattleAcquireSoundSlot((u8*)(idx + 0x60000), flags, pData,
                      (idx & 0x7F) | ((pActor->flags >> 20) & 0x80));
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
        BattleAcquireSoundSlot((u8*)(b | 0x60000), flags, pData,
                      ((*(u32*)(p + 0xD0) >> 20) & 0x80) | (b & 0x7F));
    }
    b = pTable[i + 1];
    if (b != 0) {
        BattleAcquireSoundSlot((u8*)(b | 0x60000), flags, pData,
                      ((*(u32*)(p + 0xD0) >> 20) & 0x80) | (b & 0x7F));
    }
}*/
void BattleStopSoundSlotsByFlags(u8 kind) {
    BattleSlot* pSlot;
    s32 i;

    pSlot = D_80096EA0;
    for (i = 0; i < 4; i++) {
        if (pSlot[i].unk1 != 0 && pSlot[i].unk3 == kind) {
            SoundStopElementsByIndex(pSlot[i].unk2);
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
void BattleUpdateActorApproach(BattleActor* pActor, s16 arg1, s16 arg2) {
    s32 pad[4];
    s32 step;
    s32 backStep;
    s32 flag;
    s32 i;

    step = pActor->unk98;
    backStep = pActor->unk9C;
    if (pActor->flags & 0x100) {
        backStep = backStep * 2 / 3;
        flag = 0;
    } else {
        flag = 1;
    }
    for (i = 0; i < 2; i++) {
        if (arg2 != 0 && flag != 0) {
            pActor->unk48 += step;
            if (pActor->unk48 > arg2) {
                pActor->unk48 = arg2;
            }
        } else {
            pActor->unk48 -= backStep;
            if (pActor->unk48 < 0) {
                pActor->unk48 = 0;
            }
        }
    }
    pActor->unk58 = func_8008B650(pActor->unk58, arg1, 0x40);
    pActor->unkCE = 0;
}
void BattleCheckStateAndSetDelay60(BattleActor* pActor, BattleWork* pWork) {
    BattleTryQueueMessage5(pActor, pWork);
    pWork->unk4 = 0x3C;
}
void BattleQueueRandomMessagesAndDelay(BattleActor* pActor, BattleWork* pWork, s32 flag) {
    BattleQueueRandomMessages(pActor, pWork, flag);
    pWork->unk4 = flag ? 0x1E : 0xA;
}
void BattleQueueMessage3AndDelay60(BattleActor* pActor, BattleWork* pWork) {
    BattleQueueMessage(pActor, 3);
    pWork->unk4 = 0x3C;
}
void BattleToggleActionFlag2000(BattleActor* pActor, BattleWork* pWork) {
    u32 flags;

    flags = pWork->unk2C.flags;
    pWork->unk2C.flags = (flags & ~0x2000) | ((((flags >> 13) & 1) ^ 1) << 13);
    if (pWork->unk2C.flags & 0x2000) {
        pActor->flags |= 2;
        pWork->unk4 = 0x3C;
    } else {
        pActor->flags &= ~2;
        pActor->flags &= ~0x38;
        pWork->unk4 = 0x1E;
    }
}
void BattleQueueMessages4Then3Delay60(BattleActor* pActor, BattleWork* pWork) {
    BattleQueueMessage(pActor, 4);
    BattleQueueMessage(pActor, 3);
    pWork->unk4 = 0x3C;
}
void BattleQueueMessage4AndDelay60(BattleActor* pActor, BattleWork* pWork) {
    BattleQueueMessage(pActor, 4);
    pWork->unk4 = 0x3C;
}
void BattleSetRandomFacingAround800(BattleActor* pActor, BattleWork* pWork) {
    u16 timer;

    if (D_8009284C >= 0x801) {
        pWork->unk4 = 1;
        pWork->unkC = 0;
        return;
    }
    pWork->unk4 -= 1;
    if ((s16)pWork->unk4 != -1) {
        return;
    }
    pWork->unkA = rand() % 0x600 + 0x500;
    timer = rand() % 50 + 0xA;
    pWork->unkC = 0xFF;
    pWork->unk4 = timer;
}
void BattleSetRandomFacingAround0(BattleActor* pActor, BattleWork* pWork) {
    u16 timer;

    if (D_8009284C < 0x100) {
        pWork->unk4 = 1;
        pWork->unkC = 0;
        return;
    }
    pWork->unk4 -= 1;
    if ((s16)pWork->unk4 != -1) {
        return;
    }
    pWork->unkA = rand() % 0x600 - 0x300;
    timer = rand() % 120 + 0xA;
    pWork->unkC = 0xFF;
    pWork->unk4 = timer;
}
void BattleSetActionFieldFAndDispatch(BattleActor* pActor, BattleWork* pWork, u8 value) {
    pWork->unkF = value;
    BattleDispatchWorkState(pActor);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008F280);
s32 BattleCheckValueAboveScaledMax(BattleActor* pActor, s32 arg1) {
    return (pActor->unkBC * arg1) / 255 < pActor->unkB4;
}
