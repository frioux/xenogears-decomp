#include "common.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/memory.h"
#include "battle/actor.h"

extern u8 D_800C3B74;
extern u8 D_800C402F[];
extern BattleActor *D_800D3368[];
extern u8 *D_8005919C;
extern u8 *D_800C4924;
extern s16 BattleOffsetBySin(s16 angle, s16 divisor, s16 base);
extern s16 BattleOffsetByAngleClamped(s16 angle, s16 divisor, s16 base);
extern s16 BattleOffsetByNegatedAngle(s16 angle, s16 divisor, s16 base);
extern s16 BattleCountdownOffsetByAngle(s16 angle, s16 divisor, s16 base);
extern u16 D_800C3D40;
extern u16 D_800C3E30;
extern u8 D_800C3D0C[];
extern s32 BattleFindFirstActorInMask(void);
extern void func_800AA934(u8 *arg0, u8 *arg1, void *arg2, s32 arg3);
extern void func_800AAD54(void *arg0, void *arg1, s32 arg2, s32 arg3, s32 arg4);
extern void BattleSnapActorToGround(u8 *arg0);
extern s32 func_8009F1C4(u32 arg0, s16 arg1);
extern s32 func_8009EF3C(u32 arg0, s16 arg1);
extern s32 func_800A0838(u8 *arg0, u32 arg1, u16 arg2, s16 arg3);
extern void func_800AE2A4(u8 *arg0, u8 *arg1, u8 *arg2);
extern u8 *BattleAllocBlock(u8 *arg0);

u8 BattleClampScaledByte(s16 arg0, s16 arg1, s32 arg2) {
    s16 v;

    v = arg2 + arg1 * arg0 / 256;
    if (v >= 0x100) {
        v = 0xFF;
    }
    return v;
}
void BattleSetCurrentActorAndQueueCommand(u8 *arg0, u16 arg1, u16 arg2, void *arg3) {
    u8 *p;

    p = (u8 *)D_800D3368[arg1];
    D_800C3D40 = arg1;
    D_800C3E30 = arg2;
    *(u8 *)(p + 0x35) = 0;
    if (D_800D3368[arg1] != 0) {
        if (!(arg1 == BattleFindFirstActorInMask())) {
            func_800AA934((u8 *)D_800D3368[arg1], arg0, D_800C3D0C, arg3);
        }
    }
}
s32 BattleGetActorHeight(s32 arg0) {
    u8 *p;

    p = (u8 *)D_800D3368[arg0];
    if (p != 0) {
        return (*(s16 *)(p + 0x24) * ((*(s16 *)(p + 0x1C) * *(s16 *)(*(u8 **)(p + 4) + 0x4E)) >> 12)) >> 12;
    }
    return 0;
}
s32 BattleGetActorRadius(s32 arg0) {
    u8 *p;
    s32 a;
    s32 b;
    s32 r;

    r = 0;
    p = (u8 *)D_800D3368[arg0];
    if (p != 0) {
        if (*(u16 *)(p + 0x4A) & 8) {
            a = *(s16 *)(p + 0x26);
            b = *(s16 *)(*(u8 **)(p + 4) + 0x4C);
        } else {
            a = *(s16 *)(p + 0x28);
            b = *(s16 *)(*(u8 **)(p + 4) + 0x50);
        }
        r = (*(s16 *)((u8 *)D_800D3368[arg0] + 0x1C) * b) >> 12;
        r = (a * r) >> 12;
    }
    return r;
}
void BattleAllocActorEntryArray(u8 *arg0) {
    u8 *p;
    s32 i;

    if (*(u8 *)(arg0 + 0x10C) != 0) {
        p = HeapAlloc(*(u8 *)(arg0 + 0x10C) * 0x70, 0);
        for (i = 0; i < *(u8 *)(arg0 + 0x10C); i++) {
            *(s16 *)(p + i * 0x70) = -1;
            *(u32 *)(p + i * 0x70 + 8) = 0;
        }
        *(u32 *)(arg0 + 0x110) = (u32)p;
    }
}
void BattleSetActorPrimaryCommand(s32 arg0, u8 arg1) {
    s32 p;

    p = D_800D3368[arg0];
    if (p != 0) {
        *(u8 *)(p + 0x2A) = arg1;
    }
}
void BattleSetBattleStateBit(u32 arg0) {
    D_800C3B74 = arg0 & 1;
}
void BattleSwapActorSlots(s32 arg0, s32 arg1) {
    s32 tmp;

    ((u8 *)D_800D3368[arg0])[0x34] = 0;
    ((u8 *)D_800D3368[arg1])[0x34] = 1;
    tmp = D_800D3368[arg0];
    D_800D3368[arg0] = D_800D3368[arg1];
    D_800D3368[arg1] = tmp;
}
u8 func_800AA7DC(s32 arg0) {
    s32 o;
    s32 stop = 0xF7;
    u8 v;

    o = arg0 * 0x48;
    do {
        v = D_800C402F[o];
        o += 0x48;
    } while (v == stop);
    if (v == 0xFF) {
        v = 0xFE;
    }
    return v;
}
s16 (*BattleGetAngleOffsetFunction(s32 arg0))(s16 angle, s16 divisor, s16 base) {
    switch (arg0) {
    case 1:
        return BattleOffsetByAngleClamped;
    case 2:
        return BattleOffsetByNegatedAngle;
    case 3:
        return BattleCountdownOffsetByAngle;
    }
    return BattleOffsetBySin;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA898);
/* 74.36% - every store and constant is identical and in the same order; the
   only difference is where the "-1" constant is materialised: the original
   does "li v1,-1" as the very first instruction (before "li v0,0xffff"), this
   emits it just before the sh to 0x98. Tried: raw -1 literals for the three
   s16 fields (74.36), a "s16 m1 = -1;" decl initialiser at the top (74.36),
   a separate "m1 = -1;" statement first (74.36) - gcc sinks the constant to
   its first use here, and the 0xFFFF/0xFF/0x6B/0x1 loads already match.
   Note *(u16 *)(arg0 + 0x3C) is unsigned (ori 0xffff) while 0x98/0x3A/0x1E are
   signed -1 (addiu), so the two constants must stay spelled differently.
void func_800AA898(u8 *arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(u16 *)(arg0 + 0x3C) = -1;
    *(u8 *)(arg0 + 0x5C) = 0xFF;
    *(u8 *)(arg0 + 0x39) = 0x6B;
    *(s32 *)(arg0 + 0x8) = arg2;
    *(s32 *)(arg0 + 0xC) = 0;
    *(s32 *)(arg0 + 0x10) = 0;
    *(s32 *)(arg0 + 0x14) = arg3;
    *(s32 *)(arg0 + 0x18) = 0;
    *(u8 *)(arg0 + 0x2B) = 0;
    *(s16 *)(arg0 + 0x98) = -1;
    *(u16 *)(arg0 + 0x58) = 0;
    *(u8 *)(arg0 + 0x35) = 0;
    *(u8 *)(arg0 + 0x37) = 0;
    *(u8 *)(arg0 + 0x38) = 0;
    *(s16 *)(arg0 + 0x3A) = -1;
    *(u16 *)(arg0 + 0x70) = 0;
    *(u16 *)(arg0 + 0x72) = 0;
    *(u16 *)(arg0 + 0x74) = 0;
    *(u16 *)(arg0 + 0x76) = 0;
    *(u16 *)(arg0 + 0x78) = 0;
    *(u16 *)(arg0 + 0x7A) = 0;
    *(u16 *)(arg0 + 0x7C) = 0;
    *(u16 *)(arg0 + 0x7E) = 0;
    *(u16 *)(arg0 + 0x80) = 0;
    *(u16 *)(arg0 + 0x82) = 0;
    *(u16 *)(arg0 + 0x84) = 0;
    *(u16 *)(arg0 + 0x86) = 0;
    *(u16 *)(arg0 + 0x88) = 0;
    *(u16 *)(arg0 + 0x8A) = 0;
    *(u16 *)(arg0 + 0x8C) = 0;
    *(u16 *)(arg0 + 0x8E) = 1;
    *(u8 *)(arg0 + 0x36) = 0;
    *(s16 *)(arg0 + 0x1E) = -1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA934);
/* 88.71% - structure, branch layout, both table reads and the whole tail
   (the stores + func_800AAD54(arg0, arg2, -1, 1, 0)) already line up. Three
   differences left: (1) the original reloads arg0[0x2B] for the "++"
   (lbu, nop, addiu) while gcc CSEs it into one load and masks it
   ("andi v0,v1,0xff"), so the two blocks are 4 instructions apart;
   (2) the if-branch table pointer lands in $a1 instead of $v0;
   (3) the else branch folds the +4 of *(u8 **)(arg1 + 0xC) + 4 into the
   -0x138 displacement (-0x134) where the original keeps "lw v1,4(v0)".
   Tried: nested ifs with bare dereferences (64.61), a u8 *pCount = arg0 +
   0x2B local (88.71, no change), pointer locals for both table bases (88.71),
   and a u16 n = D_800C3E30 local which is needed to hoist the lhu before the
   stores. Same-family blocker as the note above: a constant/pointer being
   re-formed where the original keeps it live.
void func_800AA934(u8 *arg0, u8 *arg1, void *arg2, s32 arg3) {
    s32 *pTable;
    u8 *pCount;
    u16 n;

    pCount = arg0 + 0x2B;
    if (arg0 != 0) {
        if (arg1 != 0) {
            if (*pCount != 0) {
                if (*pCount < 5) {
                    *pCount = *pCount + 1;
                }
                ((u8 *)arg0)[*pCount + 0x2A] = *(u8 *)(arg1 + 0x20);
                ((u8 *)arg0)[*pCount + 0x2E] = arg3;
            } else {
                if (arg3 < 0x50) {
                    pTable = (s32 *)*(u8 **)(arg1 + 8);
                    *(s32 *)(arg0 + 0x10) = pTable[arg3];
                } else {
                    pTable = (s32 *)(*(u8 **)(arg1 + 0xC) + 4);
                    *(s32 *)(arg0 + 0x10) = *(s32 *)((u8 *)pTable + arg3 * 4 - 0x138);
                }
                n = D_800C3E30;
                *(u16 *)(arg0 + 0x42) = 0;
                *(u16 *)(arg0 + 0x40) = 0;
                *(s32 *)(arg0 + 0x50) = 0;
                *(s32 *)(arg0 + 0x54) = 0;
                *(s32 *)(arg0 + 0x4C) = 0;
                *(u8 *)(arg0 + 0x23) = 0;
                *(u16 *)(arg0 + 0x10A) = n;
                func_800AAD54(arg0, arg2, -1, 1, 0);
            }
        }
    }
}
*/
s32 BattleStepActorAnimation(u8 *arg0, u8 *arg1, s32 arg2, u8 *arg3, s32 arg4) {
    s32 r;
    s32 i;

    if (*(u32 *)arg0 != 0) {
        r = 0;
        if (*(u8 *)(arg0 + 0x34) != 0) {
            BattleSnapActorToGround(arg0);
            if (*(u8 *)(arg0 + 0x37) != 0) {
                func_8009F1C4(*(u32 *)(arg0 + 4), *(s16 *)(arg0 + 0x1C));
            } else {
                func_8009EF3C(*(u32 *)(arg0 + 4), *(s16 *)(arg0 + 0x1C));
            }
            for (i = 0; i < arg2; i++) {
                r |= func_800A0838(arg1, *(u32 *)(arg0 + 4), *(u16 *)(arg0 + 0x3C), *(s16 *)(arg0 + 0x1C));
                func_800AE2A4(arg0, arg1, arg3);
            }
        }
        func_800AAD54(arg0, arg1, r, arg2, arg4);
    }
    return r;
}
void BattleSpriteUpdateWorldPosition(BattleActor *pActor) {
    BattleSprite *pSprites;
    MATRIX *pMatrix;
    s32 index;
    SVECTOR vec;

    pMatrix = (MATRIX *)PSX_SCRATCH;
    if (D_800D3368[pActor->unk5C] != NULL) {
        if (!(pActor->flags4A & 0x10)) {
            pActor->unk34 = D_800D3368[pActor->unk5C]->unk34;
        }
        if (pActor->unk34 != 0) {
            if (pActor->unk5D != 0) {
                index = pActor->unk5E;
                if (index != 0) {
                    pSprites = D_800D3368[pActor->unk5C]->pSprites;
                    MulMatrix0(&pSprites->globalTransform,
                               (MATRIX *)(index * 0x7C + (s32)pSprites + 0x2C), (MATRIX *)PSX_SCRATCH);
                } else {
                    pMatrix = &D_800D3368[pActor->unk5C]->pSprites->globalTransform;
                }
                MulMatrix2(pMatrix, &pActor->pSprites->localTransform);
                MulMatrix2(pMatrix, &pActor->pSprites->globalTransform);
            }
            index = pActor->unk5E;
            if (index != 0) {
                pSprites = D_800D3368[pActor->unk5C]->pSprites;
                CompMatrix(&pSprites->localTransform,
                           (MATRIX *)(index * 0x7C + (s32)pSprites + 0x2C), pMatrix);
            } else {
                pMatrix = &D_800D3368[pActor->unk5C]->pSprites->localTransform;
            }
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);
            vec.vx = pActor->unk6A;
            vec.vy = pActor->unk6C;
            vec.vz = pActor->unk6E;
            gte_ldv0(&vec);
            gte_mvmva(1, 0, 0, 0, 0);
            gte_stlvnl((u8 *)pActor->pSprites + 0x20);
            gte_stlvnl((u8 *)pActor->pSprites + 0x5C);
        }
    } else {
        pActor->unk5C = 0xFF;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AAD54);
void BattleQueueSpriteRotationTween(u8 *arg0, u8 *arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5) {
    u8 *pNode;

    if (arg2 < 2) {
        *(s16 *)(arg1 + 0x54) = arg3;
        *(s16 *)(arg1 + 0x56) = arg4;
        *(s16 *)(arg1 + 0x58) = arg5;
        *(u8 *)(arg1 + 5) = 1;
    } else if (*(s16 *)(arg1 + 0x54) != arg3 || *(s16 *)(arg1 + 0x56) != arg4 ||
               *(s16 *)(arg1 + 0x58) != arg5) {
        if (*(u8 **)(arg1 + 0x70) == 0) {
            pNode = BattleAllocBlock(arg0);
        } else {
            pNode = *(u8 **)(arg1 + 0x70);
        }
        if (pNode != 0) {
            *(u8 *)(pNode + 0) = 1;
            *(u8 *)(pNode + 2) = 3;
            *(u8 *)(pNode + 1) = 0;
            *(u8 *)(pNode + 3) = 0xFE;
            *(u16 *)(pNode + 4) = *(u16 *)(arg1 + 0x54);
            *(u16 *)(pNode + 6) = *(u16 *)(arg1 + 0x56);
            *(u16 *)(pNode + 8) = *(u16 *)(arg1 + 0x58);
            arg3 = (arg3 - *(s16 *)(arg1 + 0x54)) & 0xFFF;
            if (arg3 >= 0x800) {
                arg3 -= 0x1000;
            }
            *(s16 *)(pNode + 0xA) = arg3;
            arg4 = (arg4 - *(s16 *)(arg1 + 0x56)) & 0xFFF;
            if (arg4 >= 0x800) {
                arg4 -= 0x1000;
            }
            *(s16 *)(pNode + 0xC) = arg4;
            arg5 = (arg5 - *(s16 *)(arg1 + 0x58)) & 0xFFF;
            if (arg5 >= 0x800) {
                arg5 -= 0x1000;
            }
            *(s16 *)(pNode + 0xE) = arg5;
            *(u16 *)(pNode + 0x10) = 0;
            *(s16 *)(pNode + 0x12) = arg2;
            *(u32 *)(arg1 + 0x70) = pNode;
        }
    }
}
void BattleQueueSpriteCommandToPoint(u8 *arg0, u8 *arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, s32 arg8) {
    u8 *pNode;
    s32 dx;
    s32 dy;
    s32 dz;

    if (*(u32 *)(arg1 + 0x70) == 0) {
        pNode = BattleAllocBlock(arg0);
    } else {
        pNode = *(u8 **)(arg1 + 0x70);
    }
    if (pNode != 0) {
        *(u8 *)(pNode + 0) = 1;
        *(u8 *)(pNode + 2) = arg2 + 7;
        *(u8 *)(pNode + 1) = 0;
        *(u8 *)(pNode + 3) = 0xFE;
        dx = arg6 - *(s32 *)(arg1 + 0x5C);
        dy = arg7 - *(s32 *)(arg1 + 0x60);
        dz = arg8 - *(s32 *)(arg1 + 0x64);
        *(s16 *)(pNode + 4) = SquareRoot0(dx * dx + dy * dy + dz * dz) + 1;
        *(s16 *)(pNode + 6) = arg3;
        *(s16 *)(pNode + 8) = arg4;
        *(s16 *)(pNode + 0xA) = arg6;
        *(s16 *)(pNode + 0xC) = arg7;
        *(s16 *)(pNode + 0xE) = arg8;
        *(s16 *)(pNode + 0x10) = 0;
        *(s16 *)(pNode + 0x12) = arg5;
        *(u32 *)(arg1 + 0x70) = pNode;
    }
}
void BattleSetupActorMoveCommand(u8 *arg0, u8 *arg1, s32 arg2) {
    u8 *p;

    if (*(u16 *)(arg1 + 0x12) != 0) {
        *(s16 *)(arg0 + 0x98) = 0;
        if (arg2 != 0) {
            *(s16 *)(arg0 + 0x9A) = *(u16 *)(arg1 + 0x2);
        } else {
            *(s16 *)(arg0 + 0x9A) = -1;
        }
        *(s16 *)(arg0 + 0x9C) = 0;
        *(s16 *)(arg0 + 0x9E) = *(u16 *)(arg1 + 0x12);
        p = arg1 + *(u32 *)(arg1 + 0x14);
        *(u32 *)(arg0 + 0xA0) = (u32)p;
        *(u32 *)(arg0 + 0xA4) = (u32)p;
    } else {
        *(s16 *)(arg0 + 0x98) = -1;
    }
}
s32 BattleGetSoundBankId(u8 *arg0, s32 arg1) {
    if (arg1 == 0) {
        return *(u16 *)(D_8005919C + 0x14) << 16;
    }
    if (arg1 == 1) {
        return *(u16 *)(*(u8 **)(*(u8 **)(arg0 + 0xB0) + 8) + 0x14) << 16;
    }
    if (arg1 == 2) {
        return *(u16 *)(*(u8 **)(*(u8 **)(arg0 + 0xB4) + 8) + 0x14) << 16;
    }
    if (arg1 == 3) {
        return *(u16 *)(D_800C4924 + 0x14) << 16;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AE2A4);
void BattleClearActorCommand(u8 *pArg) {
    *(s16 *)(pArg + 0x98) = -1;
}
s32 BattleGetActorSpriteDistance(u8 *arg0) {
    s32 dx;
    s32 dy;
    s32 dz;

    dx = *(s16 *)(arg0 + 0x88) - *(s32 *)(*(u8 **)(arg0 + 4) + 0x5C);
    dy = *(s16 *)(arg0 + 0x8A) - *(s32 *)(*(u8 **)(arg0 + 4) + 0x60);
    dz = *(s16 *)(arg0 + 0x8C) - *(s32 *)(*(u8 **)(arg0 + 4) + 0x64);
    return SquareRoot0(dx * dx + dy * dy + dz * dz);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AEF68);
void func_800AF180(u8 *arg0, s32 arg1, u8 *arg2, u8 *arg3) {
    u8 *p;
    u8 *q;
    u8 *pChild;
    s32 n;
    s32 i;

    p = (u8 *)(arg1 * 0x7C + (s32)arg2);
    q = (u8 *)(arg1 * 0x7C + (s32)arg3);
    n = *(u16 *)(arg2 + 0xA);
    *(u8 *)(p + 7) = 0;
    *(u8 *)(q + 7) = 1;
    BattleFreeBlock(arg0, *(s32 *)(p + 0x70));
    do {
        *(s32 *)(p + 0x70) = 0;
        BattleFreeBlock(arg0, *(s32 *)(p + 0x74));
        *(s32 *)(p + 0x74) = 0;
    } while (0);
    BattleFreeBlock(arg0, *(s32 *)(p + 0x78));
    *(s32 *)(p + 0x78) = 0;
    pChild = arg2;
    for (i = 1; i < n; i++) {
        pChild += 0x7C;
        if (*(u8 **)pChild == p) {
            func_800AF180(arg0, *(u16 *)(pChild + 0xA), arg2, arg3);
        }
    }
}
/* 85.71% (80 bytes vs 84) - instruction multiset identical, only the preheader order differs:
   the original emits the hoisted "li a3,1" before "addiu a0,a0,7", here the
   arg0 += 7 statement lands there first (and reorg then fills the beqz delay
   slot with it, so the frame is one insn shorter). The increment is in the
   ENTRY block here, in the original it is in the preheader after the loop
   guard, which is why the original still has a nop in the beqz delay slot.
   Indexed forms arg0[i*0x7C+7] (90.33%) or p=arg0+7 / p[i*0x7C] (90.43%) put
   li a3 first but let gcc fold the +7 into the load displacement (lbu 7(a0)) or
   into the biv init (addiu a0,a0,0x83); a u8 one; local made it worse, the
   addiu hoisted above the slt.
   Permuter (base=163 best=60) found only a TRAP: swapping the two first
   statements so the count is read as *(u16*)(arg0+0xA) AFTER arg0 += 7, i.e.
   from +0x11 instead of +0xA. It scores 90.43% because it emits the same
   instruction count, but it is semantically wrong; a second 4-minute round
   from that body found nothing better (60 -> 60). Do not use it. *//*
void func_800AF270(u8 *arg0, u8 *arg1) {
    s32 n;
    s32 i;

    n = *(u16 *)(arg0 + 0xA);
    arg0 += 7;
    for (i = 1; i < n; i++) {
        arg1 += 0x7C;
        arg0 += 0x7C;
        if (*arg0 != 0) {
            *arg0 = 0;
            arg1[7] = 1;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AF270);
