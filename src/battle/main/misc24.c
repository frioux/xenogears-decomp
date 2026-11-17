#include "common.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/memory.h"
#include "battle/actor.h"
#include "libgear/main.h"

extern u8 g_BattleStateBit;
extern BattleActor *g_BattleActors[];
extern Temp8 *g_BattleSoundBankMain;
extern Temp8 *g_BattleSoundBankSecondary;
extern s16 BattleOffsetBySin(s16 angle, s16 divisor, s16 base);
extern s16 BattleOffsetByAngleClamped(s16 angle, s16 divisor, s16 base);
extern s16 BattleOffsetByNegatedAngle(s16 angle, s16 divisor, s16 base);
extern s16 BattleCountdownOffsetByAngle(s16 angle, s16 divisor, s16 base);
extern u16 g_BattleCurrentActorSlot;
extern u16 g_BattleCommandActorMask;
extern u8 g_BattleBlockPool[];
extern s32 BattleFindFirstActorInMask(void);
extern void BattleAppendActorCommandBytes(BattleActor *pActor, BattleActor *pSource, void *arg2, s32 arg3);
extern void func_800AAD54(void *arg0, void *arg1, s32 arg2, s32 arg3, s32 arg4);
extern void BattleSnapActorToGround(BattleActor *pActor);
extern s32 func_8009F1C4(BattleSprite *pSprites, s16 unk1C);
extern s32 BattleUpdateSpriteTransforms(BattleSprite *pSprites, s16 unk1C);
extern s32 func_800A0838(u8 *arg0, BattleSprite *pSprites, u16 unk3C, s16 unk1C);
extern void func_800AE2A4(BattleActor *pActor, u8 *arg1, u8 *arg2);
extern BattleBlock *BattleAllocBlock(void *pPool);
extern s32 BattleFreeBlock(void *pPool, BattleBlock *pBlock);

// One 0x48-byte row of the battle command queue at g_BattleActorCommandIds. A row whose
// command byte still reads 0xF7 has not been dispatched yet; 0xFF ends the
// queue. The same rows are reachable from the other battle units by other
// field offsets, so only the fields read here are named.
typedef struct {
    /* 0x00 */ u8 unk00[0x2F];
    /* 0x2F */ u8 unk2F;
    /* 0x30 */ u8 unk30[0x16];
    /* 0x46 */ u16 unk46;
} BattleCommandRow; // Size: 0x48

extern BattleCommandRow g_BattleActorCommandIds[];

// A queued move command: a point count at +0x12 and the point list at +0x14,
// which BattleSetupActorMoveCommand hands to the actor. 0x18 is a lower bound
// on its size; only the fields read here are typed.
typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u16 unk2;
    /* 0x04 */ u8 unk4[0xE];
    /* 0x12 */ u16 unk12;
    /* 0x14 */ u32 unk14;
} BattleMoveCommand; // Size: 0x18

// One 0x70-byte entry of the per-actor entry array BattleAllocActorEntryArray
// allocates; only the id and the word at +8 are initialised here.
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u8 unk02[0x6];
    /* 0x08 */ u32 unk8;
    /* 0x0C */ u8 unk0C[0x64];
} BattleActorEntry; // Size: 0x70

u8 BattleClampScaledByte(s16 arg0, s16 arg1, s32 arg2) {
    s16 v;

    v = arg2 + arg1 * arg0 / 256;
    if (v >= 0x100) {
        v = 0xFF;
    }
    return v;
}
void BattleSetCurrentActorAndQueueCommand(u8 *arg0, u16 arg1, u16 arg2, void *arg3) {
    BattleActor *p;

    p = g_BattleActors[arg1];
    g_BattleCurrentActorSlot = arg1;
    g_BattleCommandActorMask = arg2;
    p->unk35 = 0;
    if (g_BattleActors[arg1] != 0) {
        if (!(arg1 == BattleFindFirstActorInMask())) {
            BattleAppendActorCommandBytes(g_BattleActors[arg1], (BattleActor *)arg0, g_BattleBlockPool, arg3);
        }
    }
}
s32 BattleGetActorHeight(s32 arg0) {
    BattleActor *p;

    p = g_BattleActors[arg0];
    if (p != 0) {
        return (p->height * ((p->scale * p->pSprites->scale.vy) >> 12)) >> 12;
    }
    return 0;
}
s32 BattleGetActorRadius(s32 arg0) {
    BattleActor *p;
    s32 a;
    s32 b;
    s32 r;

    r = 0;
    p = g_BattleActors[arg0];
    if (p != 0) {
        if (p->flags4A & 8) {
            a = p->width;
            b = p->pSprites->scale.vx;
        } else {
            a = p->depth;
            b = p->pSprites->scale.vz;
        }
        r = (g_BattleActors[arg0]->scale * b) >> 12;
        r = (a * r) >> 12;
    }
    return r;
}
void BattleAllocActorEntryArray(BattleActor *pActor) {
    BattleActorEntry *pEntries;
    s32 i;

    if (pActor->numEntries != 0) {
        pEntries = HeapAlloc(pActor->numEntries * sizeof(BattleActorEntry), 0);
        for (i = 0; i < pActor->numEntries; i++) {
            pEntries[i].unk0 = -1;
            pEntries[i].unk8 = 0;
        }
        pActor->pEntries = (mem_addr)pEntries;
    }
}
void BattleSetActorPrimaryCommand(s32 arg0, u8 arg1) {
    BattleActor *p;

    p = g_BattleActors[arg0];
    if (p != 0) {
        p->commands[0] = arg1;
    }
}
void BattleSetBattleStateBit(u32 arg0) {
    g_BattleStateBit = arg0 & 1;
}
void BattleSwapActorSlots(s32 arg0, s32 arg1) {
    BattleActor *tmp;

    g_BattleActors[arg0]->doUpdate = 0;
    g_BattleActors[arg1]->doUpdate = 1;
    tmp = g_BattleActors[arg0];
    g_BattleActors[arg0] = g_BattleActors[arg1];
    g_BattleActors[arg1] = tmp;
}
u8 BattleGetQueuedCommandCode(s32 arg0) {
    s32 stop = 0xF7;
    u8 v;

    do {
        v = g_BattleActorCommandIds[arg0].unk2F;
        arg0++;
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
/*
// 74.36%, 156 bytes (retail is also 156). Every store and every constant is
// right; the ONLY difference is where the single s16 -1 is materialised. Retail
// emits "addiu v1,zero,-1" as insn 1 (before the 0xFFFF load); gcc puts it at
// the boundary of the constant-store cluster: with the source order 0x3C, 0x5C,
// 0x39 it lands just before the sb 0x39, and listing the stores in any other
// order just moves the boundary with it. Measured: 0x5C first (74.36), 0x3C
// first, i.e. retail's own instruction order (74.36), 0x8E = 1 moved into the
// constant cluster so the cluster is four stores - the -1 then lands after the
// cluster, immediately before the word-store group, and takes $2 not $3. A
// "s16 m1 = -1;" local, a separate "m1 = -1;" statement and raw -1 literals all
// still land the li at that boundary. Permuter from this body: base 210, best
// 180, 1325 iterations - its only gain was moving the 0x3C store below the 0xC
// store, which contradicts retail's own li 0xFFFF-first order, so it is not
// taken. Untested lead: retail's placement is what a load hoisted to its
// DEAD-store position looks like, i.e. the original source may have had a
// redundant -1 assignment early in the function whose store gcc then deleted
// (cse keeps the constant in the register from insn 1). Note 0x3C is unsigned
// (ori 0xFFFF) while 0x98/0x3A/0x1E are signed -1 (addiu), so the two constants
// must stay spelled differently.
void func_800AA898(u8 *arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(u16 *)(arg0 + 0x3C) = 0xFFFF;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA898);

// Appends one command byte to the actor's pending command queue, or, when the
// queue is empty, seeds the actor's current command from the source actor's
// command tables. The two per-command byte queues sit at 0x2A (command) and
// 0x2E (argument); 0x2B counts the entries already queued.
void BattleAppendActorCommandBytes(BattleActor *pActor, BattleActor *pSource, void *arg2, s32 arg3) {
    s32 *pGroupTable;
    u16 n;

    if (pActor != 0) {
        if (pSource != 0) {
            if (pActor->commandCount != 0) {
                if (pActor->commandCount < 5) {
                    pActor->commandCount = pActor->commandCount + 1;
                }
                pActor->commands[pActor->commandCount] = pSource->unk20[0];
                pActor->commandArgs[pActor->commandCount] = arg3;
            } else {
                if (arg3 < 0x50) {
                    pActor->unk10 = ((s32 *)pSource->unk8)[arg3];
                } else {
                    pGroupTable = *(s32 **)((u8 *)pSource->pBuffer + 4);
                    pActor->unk10 = pGroupTable[arg3 - 0x4E];
                }
                n = g_BattleCommandActorMask;
                pActor->unk42 = 0;
                pActor->unk40 = 0;
                pActor->unk50 = 0;
                pActor->unk54 = 0;
                pActor->unk4C = 0;
                pActor->unk23 = 0;
                pActor->modelBitMask = n;
                func_800AAD54(pActor, arg2, -1, 1, 0);
            }
        }
    }
}
s32 BattleStepActorAnimation(BattleActor *pActor, u8 *arg1, s32 arg2, u8 *arg3, s32 arg4) {
    s32 r;
    s32 i;

    if (pActor->pMesh != 0) {
        r = 0;
        if (pActor->doUpdate != 0) {
            BattleSnapActorToGround(pActor);
            if (pActor->unk37 != 0) {
                func_8009F1C4(pActor->pSprites, pActor->scale);
            } else {
                BattleUpdateSpriteTransforms(pActor->pSprites, pActor->scale);
            }
            for (i = 0; i < arg2; i++) {
                r |= func_800A0838(arg1, pActor->pSprites, pActor->unk3C, pActor->scale);
                func_800AE2A4(pActor, arg1, arg3);
            }
        }
        func_800AAD54(pActor, arg1, r, arg2, arg4);
    }
    return r;
}
void BattleSpriteUpdateWorldPosition(BattleActor *pActor) {
    BattleSprite *pSprites;
    MATRIX *pMatrix;
    s32 index;
    SVECTOR vec;

    pMatrix = (MATRIX *)PSX_SCRATCH;
    if (g_BattleActors[pActor->parentActorSlot] != NULL) {
        if (!(pActor->flags4A & 0x10)) {
            pActor->doUpdate = g_BattleActors[pActor->parentActorSlot]->doUpdate;
        }
        if (pActor->doUpdate != 0) {
            if (pActor->isAttached != 0) {
                index = pActor->parentJointIndex;
                if (index != 0) {
                    pSprites = g_BattleActors[pActor->parentActorSlot]->pSprites;
                    MulMatrix0(&pSprites->globalTransform,
                               &((BattleSprite *)(index * sizeof(BattleSprite) + (s32)pSprites))
                                    ->globalTransform,
                               pMatrix);
                } else {
                    pMatrix = &g_BattleActors[pActor->parentActorSlot]->pSprites->globalTransform;
                }
                MulMatrix2(pMatrix, &pActor->pSprites->localTransform);
                MulMatrix2(pMatrix, &pActor->pSprites->globalTransform);
            }
            index = pActor->parentJointIndex;
            if (index != 0) {
                pSprites = g_BattleActors[pActor->parentActorSlot]->pSprites;
                CompMatrix(&pSprites->localTransform,
                           &((BattleSprite *)(index * sizeof(BattleSprite) + (s32)pSprites))
                                ->globalTransform,
                           pMatrix);
            } else {
                pMatrix = &g_BattleActors[pActor->parentActorSlot]->pSprites->localTransform;
            }
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);
            vec.vx = pActor->attachOffsetX;
            vec.vy = pActor->attachOffsetY;
            vec.vz = pActor->attachOffsetZ;
            gte_ldv0(&vec);
            gte_mvmva(1, 0, 0, 0, 0);
            gte_stlvnl((u8 *)pActor->pSprites + 0x20);
            gte_stlvnl((u8 *)pActor->pSprites + 0x5C);
        }
    } else {
        pActor->parentActorSlot = 0xFF;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AAD54);
void BattleQueueSpriteRotationTween(u8 *pPool, BattleSprite *pSprite, s32 arg2, s32 arg3, s32 arg4, s32 arg5) {
    BattleBlock *pNode;

    if (arg2 < 2) {
        pSprite->rot[0] = arg3;
        pSprite->rot[1] = arg4;
        pSprite->rot[2] = arg5;
        pSprite->doRotate = 1;
    } else if (pSprite->rot[0] != arg3 || pSprite->rot[1] != arg4 ||
               pSprite->rot[2] != arg5) {
        if (pSprite->p70 == NULL) {
            pNode = BattleAllocBlock(pPool);
        } else {
            pNode = pSprite->p70;
        }
        if (pNode != NULL) {
            pNode->inUse = 1;
            pNode->opcode = 3;
            pNode->flags = 0;
            pNode->tag = 0xFE;
            pNode->startX = pSprite->rot[0];
            pNode->startY = pSprite->rot[1];
            pNode->startZ = pSprite->rot[2];
            arg3 = (arg3 - pSprite->rot[0]) & 0xFFF;
            if (arg3 >= 0x800) {
                arg3 -= 0x1000;
            }
            pNode->targetX = arg3;
            arg4 = (arg4 - pSprite->rot[1]) & 0xFFF;
            if (arg4 >= 0x800) {
                arg4 -= 0x1000;
            }
            pNode->targetY = arg4;
            arg5 = (arg5 - pSprite->rot[2]) & 0xFFF;
            if (arg5 >= 0x800) {
                arg5 -= 0x1000;
            }
            pNode->targetZ = arg5;
            pNode->curFrame = 0;
            pNode->endFrame = arg2;
            pSprite->p70 = pNode;
        }
    }
}
void BattleQueueSpriteCommandToPoint(u8 *pPool, BattleSprite *pSprite, s32 arg2, s32 arg3, s32 arg4,
                                     s32 arg5, s32 arg6, s32 arg7, s32 arg8) {
    BattleBlock *pNode;
    s32 dx;
    s32 dy;
    s32 dz;

    if (pSprite->p70 == NULL) {
        pNode = BattleAllocBlock(pPool);
    } else {
        pNode = pSprite->p70;
    }
    if (pNode != NULL) {
        pNode->inUse = 1;
        pNode->opcode = arg2 + 7;
        pNode->flags = 0;
        pNode->tag = 0xFE;
        dx = arg6 - pSprite->pos[0];
        dy = arg7 - pSprite->pos[1];
        dz = arg8 - pSprite->pos[2];
        pNode->startX = SquareRoot0(dx * dx + dy * dy + dz * dz) + 1;
        pNode->startY = arg3;
        pNode->startZ = arg4;
        pNode->targetX = arg6;
        pNode->targetY = arg7;
        pNode->targetZ = arg8;
        pNode->curFrame = 0;
        pNode->endFrame = arg5;
        pSprite->p70 = pNode;
    }
}
void BattleSetupActorMoveCommand(BattleActor *pActor, BattleMoveCommand *pCommand, s32 arg2) {
    u8 *p;

    if (pCommand->unk12 != 0) {
        pActor->stepIndex = 0;
        if (arg2 != 0) {
            pActor->stepCount = pCommand->unk2;
        } else {
            pActor->stepCount = -1;
        }
        pActor->pointIndex = 0;
        pActor->pointCount = pCommand->unk12;
        p = (u8 *)pCommand + pCommand->unk14;
        pActor->pPointCursor = (mem_addr)p;
        pActor->pPointList = (mem_addr)p;
    } else {
        pActor->stepIndex = -1;
    }
}
s32 BattleGetSoundBankId(BattleActor *pActor, s32 arg1) {
    Temp8 *pBank;

    if (arg1 == 0) {
        return g_BattleSoundBankMain->unk14 << 16;
    }
    if (arg1 == 1) {
        pBank = ((Temp8 *)pActor->unkB0)->pUnk8;
        return pBank->unk14 << 16;
    }
    if (arg1 == 2) {
        pBank = ((Temp8 *)pActor->unkB4)->pUnk8;
        return pBank->unk14 << 16;
    }
    if (arg1 == 3) {
        return g_BattleSoundBankSecondary->unk14 << 16;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AE2A4);
void BattleClearActorCommand(BattleActor *pActor) {
    pActor->stepIndex = -1;
}
s32 BattleGetActorSpriteDistance(BattleActor *pActor) {
    s32 dx;
    s32 dy;
    s32 dz;

    dx = pActor->posX - pActor->pSprites->pos[0];
    dy = pActor->posY - pActor->pSprites->pos[1];
    dz = pActor->posZ - pActor->pSprites->pos[2];
    return SquareRoot0(dx * dx + dy * dy + dz * dz);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AEF68);
void BattleFreeActorEntryAndChildren(void *pPool, s32 index, BattleSprite *pSprites, BattleSprite *pOther) {
    BattleSprite *p;
    BattleSprite *q;
    BattleSprite *pChild;
    s32 n;
    s32 i;

    p = (BattleSprite *)(index * sizeof(BattleSprite) + (s32)pSprites);
    q = (BattleSprite *)(index * sizeof(BattleSprite) + (s32)pOther);
    n = pSprites->count;
    p->isVisible = 0;
    q->isVisible = 1;
    BattleFreeBlock(pPool, p->p70);
    do {
        p->p70 = NULL;
        BattleFreeBlock(pPool, p->p74);
        p->p74 = NULL;
    } while (0);
    BattleFreeBlock(pPool, p->p78);
    p->p78 = NULL;
    pChild = pSprites;
    for (i = 1; i < n; i++) {
        pChild++;
        if (pChild->pParent == p) {
            BattleFreeActorEntryAndChildren(pPool, pChild->count, pSprites, pOther);
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
