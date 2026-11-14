#include "common.h"

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} Vec4; // Size: 0x10

// The work word at 0x2C is written as individual flags, read as a byte and as a
// whole word (the flag RMW idiom).
typedef union {
    /* 0x0 */ u32 flags;
    /* 0x0 */ u8 bytes[4];
    struct {
        /* 0x0 */ u32 unk0 : 8;
        /* 0x0 */ u32 unk8 : 1;
        /* 0x0 */ u32 unk9 : 1;
        /* 0x0 */ u32 unk10 : 1;
        /* 0x0 */ u32 unk11 : 1;
        /* 0x0 */ u32 unk12 : 1;
        /* 0x0 */ u32 unk13 : 1;
        /* 0x0 */ u32 pad : 18;
    } bits;
} BattleWorkFlags; // Size: 0x4

// Per-actor action parameters, the block an actor's pParams points at.
typedef struct {
    /* 0x0 */ u8 unk0[0x6];
    /* 0x6 */ u8 unk6;
    /* 0x7 */ u8 unk7;
    /* 0x8 */ u8 unk8;
    /* 0x9 */ u8 unk9;
} BattleActorParams; // Size: 0xA (lower bound: 0xA is indexed by other units)

// The per-actor AI work block. Its owning actor is pOwner.
typedef struct {
    /* 0x00 */ BattlingActor* pOwner;
    /* 0x04 */ s16 unk4;
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

// A battle participant of this overlay. Only the fields this unit reads are
// named; the gaps are the rest of a struct whose full size is unknown.
typedef struct BattlingActor {
    /* 0x000 */ u8 unk0[0x48];
    /* 0x048 */ s32 unk48;
    /* 0x04C */ u8 unk4C[0xC];
    /* 0x058 */ s32 unk58;
    /* 0x05C */ u8 unk5C[0x58];
    /* 0x0B4 */ s16 unkB4;
    /* 0x0B6 */ s16 unkB6;
    /* 0x0B8 */ u8 unkB8[0x6];
    /* 0x0BE */ s16 unkBE;
    /* 0x0C0 */ u8 unkC0[0x4];
    /* 0x0C4 */ u8 unkC4;
    /* 0x0C5 */ u8 unkC5[0xB];
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u8 unkD4[0x4];
    /* 0x0D8 */ BattlingActor* pTarget;
    /* 0x0DC */ u8 unkDC[0x835];
    /* 0x911 */ u8 unk911;
    /* 0x912 */ u8 unk912[0xCEA];
    /* 0x15FC */ BattleWork* pWork;
    /* 0x1600 */ BattleActorParams* pParams;
    /* 0x1604 */ u8 unk1604[0x64];
    /* 0x1668 */ u16 unk1668;
} BattlingActor; // Size: 0x166A (lower bound)
extern s32 func_80073DE4(BattlingActor* p, s32 arg1);
extern void func_800767C8(BattlingActor* p);
extern void BattleQueueMessage(BattlingActor* p, s32 arg1);
extern u8 D_80092884;
extern u32 BattleGetGridCellAtPosition(s32* p);
extern void func_8008F7B8(BattleWork* p);
extern s32 BattleRollActorFlag8000(BattlingActor* p, s32 arg1);
extern s32 BattleGetActorBreakProximity(BattlingActor* p);
extern s32 D_8009284C;
extern s32 BattleRollActorActionChance(BattlingActor* p, s32 arg1);
extern void func_8008FFEC(BattlingActor* p, BattleWork* arg1);
extern void BattleResetMessageQueue(BattlingActor* p);
extern u8 D_80099D98;
extern void func_8008FCC8(BattlingActor* p, BattleWork* arg1);
extern void func_8009031C(BattlingActor* p, BattleWork* arg1);
extern void func_80090580(BattlingActor* p, BattleWork* arg1);
extern void func_80090990(BattlingActor* p, BattleWork* arg1);
extern void BattleUpdateActorApproach(BattlingActor* p, s32 arg1, s32 arg2);
extern s32 BattleCheckValueAboveScaledMax(BattlingActor* p, s32 arg1);


s32 BattleCheckActorBelowBreakThreshold(BattlingActor* pActor, s32 arg1) {
    if (arg1 != 0) {
        return func_80073DE4(pActor, pActor->unkBE);
    }
    return pActor->unkB6 < 0x1000 - pActor->unkBE;
}

s32 BattleGetActorBreakThreshold(BattlingActor* pActor) {
    return 0x1000 - pActor->unkBE;
}

s32 BattleGetActorBreakProximity(BattlingActor* pActor) {
    s32 v1;
    s32 v0;

    v1 = pActor->unkB6;
    v0 = pActor->pWork->unk24;
    return (v0 - 0x200 < v1) ? !(v0 < v1) : 2;
}

// retail spells each limit as (x * k) << 6 >> 4; folding it to x * (4 * k) is 2 insns shorter
s32 BattleRollActorActionChance(BattlingActor* pActor, s32 arg1) {
    BattleWork* p;

    p = pActor->pWork;
    if ((rand() & 0xFF) < ((p->unk10 * 5) << 6) >> 4) {
        if (BattleCheckActorBelowBreakThreshold(pActor, 0) != 0) {
            goto tail;
        }
        if (BattleCheckValueAboveScaledMax(pActor, 0xC0) != 0) {
            goto tail;
        }
        if ((rand() & 0xFF) < ((p->unk1C * 3) << 6) >> 4) {
            goto tail;
        }
        if (BattleCheckValueAboveScaledMax(pActor, 0x80) == 0) {
            return 0;
        }
        if (pActor->pTarget->unkB4 < pActor->unkB4) {
            goto tail;
        }
        return 0;
    }
    if ((rand() & 0xFF) >= ((p->unk1C * 5) << 6) >> 4) {
        return 0;
    }
tail:
    if ((rand() & 0xFF) < p->unk18) {
        if (pActor->pTarget->unkC4 == 4) {
            return 0;
        }
        if (BattleCheckValueAboveScaledMax(pActor->pTarget, 0x20) != 0) {
            return 1;
        }
        if (pActor->pTarget->unkB4 < pActor->unkB4) {
            return 0;
        }
    }
    return 1;
}
s32 BattleRollActorFlag8000(BattlingActor* pActor, s32 arg1) {
    BattleWork* pWork;

    pWork = pActor->pWork;
    if (arg1 != 0) {
        if ((rand() & 0xFF) < pWork->unk10) {
            if (D_8009284C < 0x600) {
                return 0;
            }
        }
    }
    if (BattleGetActorBreakProximity(pActor) != 0) {
        return 1;
    }
    return ((rand() & 0xFF) < pWork->unk1C);
}void func_8008F7B8(BattleWork* p) {
    p->unk2C.bits.unk9 = (rand() & 0xFF) < p->unk10;
    p->unk2C.bits.unk10 = (rand() & 0xFF) < p->unk14;
    p->unk2C.bits.unk12 = rand() & 1;
    p->unk2C.bits.unk11 = (rand() & 0xFF) < p->unk18;
    p->unk2C.bits.unk8 = ((rand() & 0xFF) < p->unk10) && ((rand() % 10) < 3);
    p->unk2C.bits.unk0 = rand();
    p->unk30 = p->pOwner->unk1668;
}
void BattleQueueMessages4Then3(BattlingActor* pActor) {
    if ((pActor->pTarget->flags & 0x60000000) == 0x20000000) {
        if ((rand() & 3) != 0) {
            return;
        }
    }
    if (D_80092884 != 0) {
        func_800767C8(pActor);
        BattleQueueMessage(pActor, 4);
        BattleQueueMessage(pActor, 3);
        return;
    }
    if ((pActor->flags & 0x60000000) != 0x20000000) {
        BattleQueueMessage(pActor, 3);
        return;
    }
    BattleQueueMessage(pActor, 4);
    BattleQueueMessage(pActor, 3);
}s32 BattleCheckPositionPastMapCenter(Vec4* pPos) {
    Vec4 pos;
    s32 a;
    s32 b;

    pos = *pPos;
    a = pos.x - 0x3F80;
    b = pos.z - 0x3F80;
    pos.x = a;
    pos.z = b;
    if (a > 0 && b > 0) {
        return 1;
    }
    return ((BattleGetGridCellAtPosition((s32*)pPos) & 0x3000000) ^ 0x1000000) < 1;
}extern long ratan2(long y, long x);
extern s32 D_80092934;

s32 BattleSetFacingFromPosition(Vec4* pPos, BattleWork* pWork) {
    Vec4 pos;
    s32 a;
    s32 b;
    s32 r;

    pos = *pPos;
    a = pos.x - 0x3F80;
    b = pos.z - 0x3F80;
    pos.x = a;
    pos.z = b;
    r = ratan2(pos.x, pos.z);
    pWork->unkA = (((r & 0xFFF) >= 0x201) ? 0x800 : 0xC00) - D_80092934;
    pWork->unkC = 0xFF;
    return 0;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FACC);
/* 89.78%: instruction multiset matches except gcc lays the two return 0 blocks out
   at the end of the function instead of merging them before the body, and puts
   li v0,1 in a branch delay slot. Also tried: one combined condition (same),
   splitting the two early returns (same), and hoisting the 0x8F4 load before the
   second BattleCheckPositionPastMapCenter call (worse: 78%, costs an extra saved register).
extern u8 D_800928C4;

s32 func_8008FACC(s32* pArg0, u8* pArg1) {
    s32 v;

    if (D_800928C4 == 0) {
        return 0;
    }
    if ((*(u32*)((u8*)pArg0 + 0xD0) & 0x8000000) == 0) {
        return 0;
    }
    if (BattleCheckPositionPastMapCenter(pArg0) != 0) {
        BattleSetFacingFromPosition(pArg0, pArg1);
        return 1;
    }
    if (BattleCheckPositionPastMapCenter(*(s32**)((u8*)pArg0 + 0xD8)) == 0) {
        return 0;
    }
    v = *(s32*)(*(u8**)((u8*)pArg0 + 0xD8) + 0x8F4);
    if (v < 0x800) {
        *(u32*)((u8*)pArg0 + 0xD0) |= 0x8000;
        pArg1[0xE] = 1;
        *(s16*)(pArg1 + 0xC) = 0xFF;
        *(s16*)(pArg1 + 0xA) = 0x800;
        *(s32*)((u8*)pArg0 + 0x58) = 0x800;
        return 1;
    }
    if (v < 0x1001) {
        return 1;
    }
    *(u32*)((u8*)pArg0 + 0xD0) &= 0xFFFF7FFF;
    pArg1[0xE] = 0;
    *(s16*)(pArg1 + 0xC) = 0;
    *(s32*)((u8*)pArg0 + 0x58) = 0x800;
    return 1;
}
*/
void BattleSetActorFlag2AndClearQueue(BattlingActor* pActor, BattleWork* pWork) {
    u8 v;

    v = pWork->unkF;
    if (v >= 2) {
        pActor->flags |= 2;
        BattleResetMessageQueue(pActor);
        return;
    }
    if (v != 0) {
        if (!(pWork->unk2C.bytes[0] & 1)) {
            return;
        }
    } else if (pWork->unk2C.bytes[0] % 6 != 0) {
        return;
    }
    pActor->flags |= 2;
    BattleResetMessageQueue(pActor);
}

void BattleEnterWorkState0(BattlingActor* pActor) {
    BattleWork* p;

    p = pActor->pWork;
    p->unk20 = 0;
    p->unk8 = 0;
    p->unk4 = (2 - p->unkF) * 30 + 0x5A;
    func_8008F7B8(p);
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FCC8);
void BattleQueueRandomMessages(BattlingActor* pActor) {
    s32 n;
    s32 v;

    v = rand() % 10;
    n = 1;
    if (v >= 2) {
        n = 2;
    }
    if (v >= 5) {
        n++;
    }
    for (; n != 0; n--) {
        BattleQueueMessage(pActor, (rand() & 1) + 1);
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FF24);
/* 94.70%: instruction multiset identical; two differences. gcc reassociates
   A + (rand() % 20 + 1) into (A + 1) + rand() % 20, so the +1 lands on the product
   side where retail adds it to the remainder, and it keeps the *20 product in $a0
   where retail has it in $a1 (retail's $a0 still holds the loaded 0xF byte).
   Also tried: a temp for the remainder, both parenthesised groupings, terms in the
   other order (worse: 67-75%) and gcc 2.7.2-cdk (95.49%).
void func_8008FF24(u8* pArg0, u8* pArg1) {
    if (*(u8*)(*(u8**)(pArg0 + 0xD8) + 0xC4) == 4) {
        if ((*(u32*)(pArg1 + 0x2C) & 0xA00) == 0x200) {
            BattleQueueMessages4Then3(pArg0);
            *(s16*)(pArg1 + 0x4) = 3;
        }
    } else {
        BattleQueueRandomMessages(pArg0);
        *(s16*)(pArg1 + 0x4) = (2 - *(u8*)(pArg1 + 0xF)) * 20 + (rand() % 20 + 1);
    }
    func_8008F7B8(pArg1);
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FFEC);
/* 95.41%: frame, saved regs (s0..s3) and both call-site chains already match.
   Two differences: (a) gcc keeps the loop counter`n` in $a1 whereas retail uses
   $s0, and so fills the `bnez` delay slot with `addiu s0,s0,-1` where retail has
   a nop plus a separate block; (b) the tail reassociates A + (rand() % 20 + 1)
   into (A + 1) + rand() % 20 - the same expression, and the same blocker, as
   func_8008FF24 below. Also tried: the tail as a separate `r = rand() % 20 + 1`
   statement (85.61%, the 0xF load moves after the division) and the sum written
   call-first (87.81%). Note the divisor is a local (`c`) so the 0x911 byte is
   loaded once, as retail does.
s32 func_8008FFEC(u8* pArg0, u8* pArg1) {
    s32 n;
    s32 i;
    s32 off;
    s32 c;

    c = *(u8*)(pArg0 + 0x911);
    if (c == 0) {
        return 1;
    }
    n = rand() % c;
    for (i = 0, off = 0; i < 0xE; i++, off += 3) {
        if (*(u8*)(*(u8**)(pArg0 + 0x1600) + (i + 0xA)) == 0) {
            continue;
        }
        if (*(u8*)(*(u8**)(pArg0 + 0x80) + i * 4 + 3) == 0) {
            continue;
        }
        if (n != 0) {
            n--;
            continue;
        }
        if (D_800925A4[off] != 0) {
            BattleQueueMessage(pArg0, D_800925A4[off]);
        }
        if (D_800925A5[off] != 0) {
            BattleQueueMessage(pArg0, D_800925A5[off]);
        }
        if (D_800925A6[off] != 0) {
            BattleQueueMessage(pArg0, D_800925A6[off]);
        }
        break;
    }
    *(s16*)(pArg1 + 0x4) = (2 - *(u8*)(pArg1 + 0xF)) * 20 + (rand() % 20 + 1);
    func_8008F7B8(pArg1);
    return 0;
}
*/
void BattleEnterWorkState1(BattlingActor* pActor) {
    BattleWork* p;
    u8 v;

    p = pActor->pWork;
    p->unk8 = 1;
    func_8008FF24(pActor, p);
    v = p->unkF;
    if (v >= 2) {
        p->unk9 = rand() % 8 + 1;
    } else if (v != 0) {
        p->unk9 = rand() % 6 + 1;
    } else {
        p->unk9 = rand() % 4 + 1;
    }
    p->unkC = 0;
    p->unkE = 0;
    func_8008F7B8(p);
}

s32 BattleDecideActorAction(BattlingActor* pActor, BattleWork* pWork) {
    if (pActor->unk911 != 0) {
        if ((rand() & 1) == 0) {
            return 1;
        }
        if ((rand() & 1) != 0 && BattleRollActorActionChance(pActor, 0) != 0) {
            BattleQueueMessages4Then3(pActor);
            return 0;
        }
        if (D_8009284C < 0x1001) {
            func_8008FFEC(pActor, pWork);
            return 0;
        }
        return 1;
    }
    if (BattleRollActorActionChance(pActor, 0) != 0) {
        BattleQueueMessages4Then3(pActor);
        return 0;
    }
    return 1;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8009031C);
void BattleEnterWorkState3(BattlingActor* pActor) {
    BattleWork* p;
    s32 r;

    p = pActor->pWork;
    p->unk8 = 3;
    r = rand();
    p->unk9 = r % 4 + 1;
    p->unk4 = 0;
    func_8008F7B8(p);
    p->unkE = BattleRollActorFlag8000(pActor, 1);
    p->unk2C.bytes[2] = 0;
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090580);
void BattleEnterWorkState2(BattlingActor* pActor) {
    BattleWork* p;

    p = pActor->pWork;
    p->unk8 = 2;
    if (rand() % 3 == 0) {
        func_800767C8(pActor);
    }
    p->unk28 = rand() % 1536 + 0x100;
    p->unk4 = 0;
    p->unk9 = rand() % 5 + 3;
    func_8008F7B8(p);
    p->unkE = BattleRollActorFlag8000(pActor, 0);
    p->unk2C.bytes[2] = 0;
}
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090990);
void BattleLoadWorkActionChances(BattlingActor* pActor) {
    BattleActorParams* p1;
    BattleWork* p0;

    p1 = pActor->pParams;
    p0 = pActor->pWork;
    p0->unk10 = p1->unk6;
    p0->unk14 = p1->unk7;
    p0->unk18 = p1->unk8;
    p0->unk1C = p1->unk9;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090CC0);
/* 97.38%: only the D_80099D98 load is off. gcc hoists it above the store of the
   BattleGetActorBreakThreshold result; retail keeps store-then-load and reuses $v0 for both.
   The two BattleEnterWorkState2/BattleEnterWorkState3 call sites and BattleGetActorBreakThreshold need a trailing
   argument retail passes and nobody reads (a1=0 / a1=p) - adding it to those three
   matched definitions is byte-neutral but was reverted with this park.
void func_80090CC0(u8* pArg0) {
    BattleWork* p;

    p = &D_80096F30;
    if ((*(u32*)(pArg0 + 0xD0) & 0x8000000) != 0) {
        p = &D_80096F64;
    }
    *(u8**)(pArg0 + 0x15FC) = (u8*)p;
    p->unk6 = 0x10;
    p->unk0 = pArg0;
    p->unk4 = 0;
    p->unk7 = 0xA;
    p->unkA = 0;
    p->unkC = 0;
    p->unk8 = 0;
    p->unk9 = 0;
    *(s32*)(pArg0 + 0x48) = 0;
    p->unk24 = BattleGetActorBreakThreshold(pArg0, p);
    p->unkF = D_80099D98;
    BattleLoadWorkActionChances(pArg0);
    switch (rand() % 3) {
    case 0:
        BattleEnterWorkState0(pArg0);
        break;
    case 1:
        BattleEnterWorkState1(pArg0);
        break;
    case 2:
        BattleEnterWorkState2(pArg0, 0);
        break;
    case 3:
        BattleEnterWorkState3(pArg0, 0);
        break;
    }
    BattleResetMessageQueue(pArg0);
}
*/
void BattleDispatchWorkState(BattlingActor* pActor) {
    BattleWork* p;

    if ((pActor->flags & 0x40) != 0) {
        p = pActor->pWork;
        p->unkF = D_80099D98;
        switch (p->unk8) {
        case 0:
            func_8008FCC8(pActor, p);
            break;
        case 1:
            func_8009031C(pActor, p);
            break;
        case 2:
            func_80090990(pActor, p);
            break;
        case 3:
            func_80090580(pActor, p);
            break;
        }
        if (p->unkE != 0) {
            pActor->flags |= 0x8000;
        }
        if ((p->unk2C.flags & 0x2000) != 0) {
            pActor->flags |= 2;
        }
        BattleUpdateActorApproach(pActor, p->unkA, p->unkC);
    }
}
