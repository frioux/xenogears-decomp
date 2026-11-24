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
extern s32 BattleUpdateScaledSpriteTransforms(BattleSprite *pSprites, s16 unk1C);
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
    /* 0x2F */ u8 commandCode; // 0xF7: queued but not dispatched yet, 0xFF: end of the queue
    /* 0x30 */ u8 unk30[0x16];
    /* 0x46 */ u16 slotMask; // the slots the row's command acts on (misc33 BattleActorRow.slotMask)
} BattleCommandRow; // Size: 0x48

extern BattleCommandRow g_BattleActorCommandIds[];

// A queued move command: a step count at +0x2, a point count at +0x12 and the
// point list at the offset in +0x14, which BattleSetupActorMoveCommand hands to
// the actor. 0x18 is a lower bound on its size; only the fields read here are typed.
typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u16 stepCount;
    /* 0x04 */ u8 unk4[0xE];
    /* 0x12 */ u16 pointCount;
    /* 0x14 */ u32 pointListOffset; // byte offset of the point list from the record start
} BattleMoveCommand; // Size: 0x18

// The block pActor->pBuffer points at: its +4 word is where the action-value
// table continues for the command ids that do not fit +0x8's table, i.e. a
// pointer into the middle of the same table (id 0x4E is its first entry).
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ s32 *pTailTable;
} BattleActorBuffer; // Size: 0x8

// One 0x70-byte entry of the per-actor entry array BattleAllocActorEntryArray
// allocates; only the sprite index and the word at +8 are initialised here.
typedef struct {
    /* 0x00 */ s16 spriteIndex; // negative = the entry is idle
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
            pEntries[i].spriteIndex = -1;
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
        v = g_BattleActorCommandIds[arg0].commandCode;
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
// PARKED, best 74.36% / 156 bytes = retail's exact size, 39 vs 39 insns (round 441
// re-measured the honest typed body). The only difference is where the -1 constant
// home lands: retail's `addiu v1,zero,-1` is insn 1, ahead of the 0x3C/0x5C/0x39
// constant triplet, while gcc 2.6.0 puts it at slot 5 (between `li v0,0x6B` and its
// `sb`). Every other instruction is in retail's order, so this is one displaced
// load, not a structural distance. Measured and rejected: a duplicate early store
// of -1 to 0x98/0x3A (97.44, li at slot 1 but a surplus store at slot 9 - gcc 2.6
// keeps the redundant store), one store MOVED to the top (94.87 / 93.33), a
// `do {} while (0)` around the group, volatile, `0xFFFF` spelling, `*(s32*)0x98`,
// `arg1 = -1;`, a dead `if (n != -1)`, and locals `s16 n = -1;` / `s32 negOne = -1;`
// used by all three -1 stores (cse rematerialises a constant home at its first use,
// so a local cannot hoist it). Permuter from this body: 875 iterations, base 100
// best 100. `try_flags.sh --sweep` on THIS body: -O1 94.87 and -O2/-O3 74.36 for
// all of 2.6.0 / 2.7.2 / 2.7.2-cdk x -G4/-G8 x 3000/r8000, so no preset reaches 100.
// The twin lib_gear/main/main func_801E3534 is this same function (identical 39
// insns) and is parked at 94.87 after 370 rounds, so the wall is confirmed from both
// binaries. The one clue: drop the 0x3C/0x5C/0x39 triplet and the same body hoists
// both homes to slots 0 (-1) and 1 (1), so retail's -1 home was created ahead of
// the triplet by something no source spelling has produced. 0x3C is unsigned
// (ori 0xFFFF) while 0x98/0x3A/0x1E are signed -1 (addiu), so the two constants
// must stay spelled differently. The typed body scores the same 74.36 as the raw
// pointer one, so the realistic spelling costs nothing and is what is parked.
// Round 454 re-measure of the honest typed body: still 74.36%, 39 insns and 156
// bytes (retail's exact size), one displaced `li v1,-1` and nothing else. The
// coupled alternative is re-confirmed in THIS binary as well: `s16 negOne = -1;
// s16 one = 1;` with the 0x98 store moved to the front puts the li at retail's slot
// 0 but the store at slot 8 instead of 15, and reads 94.87 - still exactly one
// displaced insn, so that number is objdiff's fuzzy wobble, not a smaller distance.
// Source order == store order here (gcc 2.6 keeps redundant stores, so no DCE'd
// statement can explain retail's early home) and retail's own store order only fits
// the honest body, so this is the body to keep parked. Not chased further: same
// wall in two binaries, 2 permuter runs, 4 presets screened.
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA898);
/*
void func_800AA898(BattleActor *pActor, s32 arg1, s32 arg2, s32 arg3) {
    pActor->blockTag = 0xFFFF;
    pActor->parentActorSlot = 0xFF;
    pActor->depthShade = 0x6B;
    pActor->pActionTable = arg2;
    pActor->pBuffer = 0;
    pActor->actionValue = 0;
    pActor->pActionTableLow = arg3;
    pActor->pActionTableHigh = 0;
    pActor->commandCount = 0;
    pActor->stepIndex = -1;
    pActor->followTargetId = 0;
    pActor->unk35 = 0;
    pActor->useScaledMatrices = 0;
    pActor->isBusy = 0;
    pActor->savedActorMask = -1;
    pActor->rotOffset[0] = 0;
    pActor->rotOffset[1] = 0;
    pActor->rotOffset[2] = 0;
    pActor->rotStep[0] = 0;
    pActor->rotStep[1] = 0;
    pActor->rotStep[2] = 0;
    pActor->posOffset[0] = 0;
    pActor->posOffset[1] = 0;
    pActor->posOffset[2] = 0;
    pActor->posStep[0] = 0;
    pActor->posStep[1] = 0;
    pActor->posStep[2] = 0;
    pActor->posX = 0;
    pActor->posY = 0;
    pActor->posZ = 0;
    pActor->moveDistance = 1;
    pActor->skipGroundSnap = 0;
    pActor->groundTriangleIndex = -1;
}
*/

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
                pActor->commands[pActor->commandCount] = pSource->actorSlot;
                pActor->commandArgs[pActor->commandCount] = arg3;
            } else {
                if (arg3 < 0x50) {
                    pActor->actionValue = ((s32 *)(u32)pSource->pActionTable)[arg3];
                } else {
                    pGroupTable = ((BattleActorBuffer *)(u32)pSource->pBuffer)->pTailTable;
                    pActor->actionValue = pGroupTable[arg3 - 0x4E];
                }
                n = g_BattleCommandActorMask;
                pActor->repeatCount = 0;
                pActor->frameTimer = 0;
                pActor->pWaitScript = 0;
                pActor->pGroundScript = 0;
                pActor->pArrivalScript = 0;
                pActor->isModelLoading = 0;
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
            if (pActor->useScaledMatrices != 0) {
                BattleUpdateScaledSpriteTransforms(pActor->pSprites, pActor->scale);
            } else {
                BattleUpdateSpriteTransforms(pActor->pSprites, pActor->scale);
            }
            for (i = 0; i < arg2; i++) {
                r |= func_800A0838(arg1, pActor->pSprites, pActor->blockTag, pActor->scale);
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
    if (pCommand->pointCount != 0) {
        pActor->stepIndex = 0;
        if (arg2 != 0) {
            pActor->stepCount = pCommand->stepCount;
        } else {
            pActor->stepCount = -1;
        }
        pActor->pointIndex = 0;
        pActor->pointCount = pCommand->pointCount;
        pActor->pPointCursor = (mem_addr)pCommand + pCommand->pointListOffset;
        pActor->pPointList = pActor->pPointCursor;
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
        pBank = ((Temp8 *)pActor->pSoundBank1)->pUnk8;
        return pBank->unk14 << 16;
    }
    if (arg1 == 2) {
        pBank = ((Temp8 *)pActor->pSoundBank2)->pUnk8;
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
void BattleSpriteUpdatePartVisibility(BattleSprite *pParts, BattleSprite *pOther) {
    s32 n;
    s32 i;
    u8 *pVisible;
    u8 visible;

    n = pParts->count;
    i = 1;
    // retail's guard plus the bottom test: a single for loop compiles to a countdown (74.52%)
    if (i < n) {
        visible = TRUE;
        pVisible = &pParts->isVisible;
        do {
            pVisible += sizeof(BattleSprite);
            pOther++;
            if (*pVisible != 0) {
                *pVisible = 0;
                pOther->isVisible = visible;
            }
            i++;
        } while (i < n);
    }
}
