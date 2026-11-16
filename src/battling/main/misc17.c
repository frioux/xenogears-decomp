#include "common.h"
#include "psyq/libgpu.h"

// volatile: retail schedules this load after the argument setup, not before it.
extern volatile s32 g_SpuChannelMask;
extern s32 D_8009283C;
extern s32 D_80092840;
extern s32 D_80092844;
extern s32 D_8009284C;
extern u8 D_80092848;
extern u8 g_BattleDrawBufferIndex;
extern u8 D_80096DE0[];
extern u8 D_80096E00[];
extern u8 g_BattleAiBehaviorLevel;
extern u8 g_BattlePlayer2ComControlled;

typedef struct {
    /* 0x00 */ u8 soundId;
    /* 0x01 */ u8 active;
    /* 0x02 */ u8 elementIndex;
    /* 0x03 */ u8 flags;
    /* 0x04 */ u16 timer;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u32 spuAddr;
    /* 0x0C */ u32 elementMask;
    /* 0x10 */ u32 data[4];
    /* 0x20 */ u32* pData;
} BattleSlot; // Size: 0x24

typedef struct {
    /* 0x0 */ u32 w[4];
} SlotData; // Size: 0x10

struct BattleActor;

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
    /* 0x00 */ struct BattleActor* pOwner;
    /* 0x04 */ u16 delay;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u8 unk8;
    /* 0x09 */ u8 unk9;
    /* 0x0A */ s16 targetAngle;
    /* 0x0C */ s16 maxDistance;
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

// A battle participant as this unit drives it. Same layout as the BattleActor
// in src/battling/main/misc18.c; the fields this unit does not read stay
// opaque.
typedef struct BattleActor {
    /* 0x000 */ u8 unk0[0x48];
    /* 0x048 */ s32 distance;
    /* 0x04C */ u8 unk4C[0xC];
    /* 0x058 */ s32 angle;
    /* 0x05C */ u8 unk5C[0x3C];
    /* 0x098 */ s32 stepForward;
    /* 0x09C */ s32 stepBack;
    /* 0x0A0 */ u8 unkA0[0x14];
    /* 0x0B4 */ s16 unkB4;
    /* 0x0B6 */ s16 unkB6;
    /* 0x0B8 */ u8 unkB8[0x4];
    /* 0x0BC */ s16 unkBC;
    /* 0x0BE */ s16 unkBE;
    /* 0x0C0 */ u8 unkC0[0xC];
    /* 0x0CC */ u16 unkCC;
    /* 0x0CE */ s16 unkCE;
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u8 unkD4[0x835];
    /* 0x909 */ u8 unk909;
    /* 0x90A */ u8 unk90A[0xCF2];
    /* 0x15FC */ BattleWork* pWork;
    /* 0x1600 */ u8 unk1600[0x64];
    /* 0x1664 */ u8* pSoundTable;
    /* 0x1668 */ u16 unk1668;
} BattleActor; // Size: 0x166A

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
extern u32 SoundGetActiveElementMask(u32);
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

    pPkt = D_80096DE0 + g_BattleDrawBufferIndex * 0x10;
    *(u32*)(pPkt + 0x4) = (color | (color << 8) | (color << 16)) | 0x60000000;
    *(u8*)(pPkt + 0x7) |= 2;
    *(u8*)(pPkt + 0x3) = 3;
    *(u32*)(pPkt + 0x8) = 0;
    *(u32*)(pPkt + 0xC) = 0xDA0140;
    AddPrim(p0, pPkt);
    tpage = GetTPage(0, flag ? 2 : 1, 0, 0);
    SetDrawMode((DR_MODE*)(D_80096E00 + g_BattleDrawBufferIndex * 0xC), 0, 1, tpage, 0);
    AddPrim(p0, D_80096E00 + g_BattleDrawBufferIndex * 0xC);
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
        pSlot->elementMask = timer;
        timer <<= 2;
        pSlot->active = 0;
        pSlot->timer = 0;
        pSlot->elementIndex = i * 2;
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
        pSlot[i].elementMask = timer;
        timer <<= 2;
        pSlot[i].active = 0;
        pSlot[i].timer = 0;
        pSlot[i].elementIndex = i * 2;
    }
}
void BattleIncrementSoundSlotTimers(void) {
    if (D_80096EA0[0].timer != 0xFFFF) {
        D_80096EA0[0].timer++;
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
        if (pSlot->active == 0 || (idx == 0 && pSlot->soundId == 0)) {
            do { pSel = pSlot; } while (0); // the block keeps the copy out of the beq delay slot
            break;
        }
        if (best < pSlot->timer) {
            best = pSlot->timer;
            pSel = pSlot;
        }
    }
    BattleIncrementSoundSlotTimers();
    pSlot = pSel;
    pSlot->soundId = idx;
    pSlot->spuAddr = (u32)p0;
    pSlot->active = 1;
    pSlot->flags = flags;
    pSlot->pData = (u32*)pData;
    if (pData != 0) {
        pSrc = (u32*)pData;
        *(SlotData*)pSlot->data = *(SlotData*)pSrc;
    }
    pSlot->timer = 0;
    if (idx == 0) {
        SoundPlaySoundEffectOnPairWithLevels(pSlot->spuAddr, pSlot->elementIndex, 0x7F, 0x40);
    }
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008E8B0);
void BattleReleaseFinishedSoundSlots(void) {
    BattleSlot* pSlot;
    s32 i;

    pSlot = D_80096EA0;
    for (i = 0; i < 4; i++) {
        if ((SoundGetActiveElementMask(pSlot[i].spuAddr) & pSlot[i].elementMask) == 0) {
            pSlot[i].active = 0;
        }
    }
    BattleIncrementSoundSlotTimers();
}
void BattlePlaySoundEffectById(u8* p) {
    if (p != NULL) {
        BattleAcquireSoundSlot(p + 0x60000, 0, NULL, g_SpuChannelMask);
    }
}
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
        if (pSlot[i].active != 0 && pSlot[i].flags == kind) {
            SoundStopElementsByIndex(pSlot[i].elementIndex);
            pSlot[i].active = 0;
        }
    }
}
/* best: objdiff 99.43% - size (176), frame (0x28, `s32 pad[2]` supplies retail's extra 8 bytes), the whole
   instruction multiset, both loads' position and the $at-vs-base split of the two table reads all match. Only
   difference: register numbering. Retail keeps &D_80091EE0 in s1 and the byte index in s2; gcc gives the index
   s1 and the base s2. The non-void return is what fixed the arg-4 load order: plain void was 90.34%.
   Permuter from the 99.43 body: 1404 iterations, no gain (base 25, best 25). Also tried: reading D_80091EE0[]
   directly for both (78.93%), pTable assigned before the byte load (92.50%) or right after `i = v << 1` (no
   change), an initializer on the pointer (92.50%), a u32 index (no change), pad declared last (no change),
   -G0 and gcc-2.7.2-cdk (76.41%).
/*
int func_8008ED6C(u8* p, s32 idx) {
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
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008ED6C);
void BattleUpdateActorApproach(BattleActor* pActor, s16 arg1, s16 arg2) {
    s32 pad[4];
    s32 step;
    s32 backStep;
    s32 flag;
    s32 i;

    step = pActor->stepForward;
    backStep = pActor->stepBack;
    if (pActor->flags & 0x100) {
        backStep = backStep * 2 / 3;
        flag = 0;
    } else {
        flag = 1;
    }
    for (i = 0; i < 2; i++) {
        if (arg2 != 0 && flag != 0) {
            pActor->distance += step;
            if (pActor->distance > arg2) {
                pActor->distance = arg2;
            }
        } else {
            pActor->distance -= backStep;
            if (pActor->distance < 0) {
                pActor->distance = 0;
            }
        }
    }
    pActor->angle = func_8008B650(pActor->angle, arg1, 0x40);
    pActor->unkCE = 0;
}
void BattleCheckStateAndSetDelay60(BattleActor* pActor, BattleWork* pWork) {
    BattleTryQueueMessage5(pActor, pWork);
    pWork->delay = 0x3C;
}
void BattleQueueRandomMessagesAndDelay(BattleActor* pActor, BattleWork* pWork, s32 flag) {
    BattleQueueRandomMessages(pActor, pWork, flag);
    pWork->delay = flag ? 0x1E : 0xA;
}
void BattleQueueMessage3AndDelay60(BattleActor* pActor, BattleWork* pWork) {
    BattleQueueMessage(pActor, 3);
    pWork->delay = 0x3C;
}
void BattleToggleActionFlag2000(BattleActor* pActor, BattleWork* pWork) {
    u32 flags;

    flags = pWork->unk2C.flags;
    pWork->unk2C.flags = (flags & ~0x2000) | ((((flags >> 13) & 1) ^ 1) << 13);
    if (pWork->unk2C.flags & 0x2000) {
        pActor->flags |= 2;
        pWork->delay = 0x3C;
    } else {
        pActor->flags &= ~2;
        pActor->flags &= ~0x38;
        pWork->delay = 0x1E;
    }
}
void BattleQueueMessages4Then3Delay60(BattleActor* pActor, BattleWork* pWork) {
    BattleQueueMessage(pActor, 4);
    BattleQueueMessage(pActor, 3);
    pWork->delay = 0x3C;
}
void BattleQueueMessage4AndDelay60(BattleActor* pActor, BattleWork* pWork) {
    BattleQueueMessage(pActor, 4);
    pWork->delay = 0x3C;
}
void BattleSetRandomFacingAround800(BattleActor* pActor, BattleWork* pWork) {
    u16 timer;

    if (D_8009284C >= 0x801) {
        pWork->delay = 1;
        pWork->maxDistance = 0;
        return;
    }
    pWork->delay -= 1;
    if ((s16)pWork->delay != -1) {
        return;
    }
    pWork->targetAngle = rand() % 0x600 + 0x500;
    timer = rand() % 50 + 0xA;
    pWork->maxDistance = 0xFF;
    pWork->delay = timer;
}
void BattleSetRandomFacingAround0(BattleActor* pActor, BattleWork* pWork) {
    u16 timer;

    if (D_8009284C < 0x100) {
        pWork->delay = 1;
        pWork->maxDistance = 0;
        return;
    }
    pWork->delay -= 1;
    if ((s16)pWork->delay != -1) {
        return;
    }
    pWork->targetAngle = rand() % 0x600 - 0x300;
    timer = rand() % 120 + 0xA;
    pWork->maxDistance = 0xFF;
    pWork->delay = timer;
}
void BattleSetActionFieldFAndDispatch(BattleActor* pActor, BattleWork* pWork, u8 value) {
    pWork->unkF = value;
    BattleDispatchWorkState(pActor);
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc17", func_8008F280);
/* best: objdiff 93.31%, size exact (628 bytes) and both jump tables exact: 12
   entries for the 2..0xD switch (default shares the decrement arm) and 14 for
   the 0..0xD one (case 0 shares the default). From the first switch to the end
   the instruction stream is identical; the whole remaining difference is the
   ENTRY BLOCK order. Retail emits: sw s0 / sw ra / sw s1 / lw s1,0x15FC(s0) /
   lbu g_BattleAiBehaviorLevel / lbu D_80092848 / ori+lui+sb g_BattlePlayer2ComControlled / beq. gcc emits:
   sw s0 / lbu g_BattleAiBehaviorLevel / li+sb g_BattlePlayer2ComControlled / lbu D_80092848 / sw ra / sw s1 /
   lw s1,0x15FC(s0) / beq - it sinks the pWork load below the two global reads
   and splits them around the flag store, and it also steals the flag-test
   body's lw v0,0xD0(s0) into the beqz delay slot where retail has a nop.
   Tried: pWork load first, after the flag store, and as a declaration
   initialiser; the state read in a local; both globals read into locals (gcc
   folds a single-use local back into the branch); the compare written in both
   operand orders; a boolean local for the compare (also folded back); s32
   instead of void (that one does add retail's nop after the condition's lw but
   keeps the stolen beq delay slot and does not change the percentage).
   Permuter, 1168 iterations: one better shape (605 -> 300, objdiff 94.39%),
   assigning pWork inside the reset block after its own uses - no source retail
   can have written, and it trades the adjacent global loads for the pWork load
   and its save, so it is not worth keeping.
// The per-state action handler. A change of state (g_BattleAiBehaviorLevel) resets the work
// slot, the actor's flags and the work flags word, then the state's handler
// runs and most states advance the actor's approach animation at the end.
void func_8008F280(BattleActor* pActor) {
    BattleWork* pWork;

    pWork = pActor->pWork;
    g_BattlePlayer2ComControlled = 1;
    if (D_80092848 != g_BattleAiBehaviorLevel) {
        pWork->delay = 0;
        pWork->maxDistance = 0;
        pActor->flags &= ~2;
        pActor->distance = 0;
        pActor->flags &= ~0x38;
        pWork->unk2C.flags &= ~0x2000;
        D_80092848 = g_BattleAiBehaviorLevel;
        pActor->unkCE = pActor->unkCC + 0x800;
    }
    if (pWork->unk2C.flags & 0x2000) {
        pActor->flags |= 2;
    }
    switch (g_BattleAiBehaviorLevel) {
    case 2:
    case 8:
    case 9:
    case 11:
    case 12:
    case 13:
        break;
    default:
        pWork->delay -= 1;
        if ((s16)pWork->delay != -1) {
            return;
        }
        break;
    }
    switch (g_BattleAiBehaviorLevel) {
    case 3:
        BattleQueueRandomMessagesAndDelay(pActor, pWork, 0);
        break;
    case 4:
        BattleQueueRandomMessagesAndDelay(pActor, pWork, 1);
        break;
    case 5:
        BattleQueueMessage3AndDelay60(pActor, pWork);
        break;
    case 6:
        BattleQueueMessages4Then3Delay60(pActor, pWork);
        break;
    case 7:
        BattleQueueMessage4AndDelay60(pActor, pWork);
        break;
    case 9:
        BattleSetRandomFacingAround800(pActor, pWork);
        break;
    case 8:
        BattleSetRandomFacingAround0(pActor, pWork);
        break;
    case 10:
        BattleCheckStateAndSetDelay60(pActor, pWork);
        break;
    case 11:
        BattleSetActionFieldFAndDispatch(pActor, pWork, 0);
        return;
    case 12:
        BattleSetActionFieldFAndDispatch(pActor, pWork, 1);
        return;
    case 13:
        BattleSetActionFieldFAndDispatch(pActor, pWork, 2);
        return;
    case 2:
        g_BattlePlayer2ComControlled = 0;
        return;
    case 1:
        BattleToggleActionFlag2000(pActor, pWork);
        break;
    case 0:
    default:
        pWork->delay = 1;
        break;
    }
    BattleUpdateActorApproach(pActor, pWork->targetAngle, pWork->maxDistance);
}*/
s32 BattleCheckValueAboveScaledMax(BattleActor* pActor, s32 arg1) {
    return (pActor->unkBC * arg1) / 255 < pActor->unkB4;
}
