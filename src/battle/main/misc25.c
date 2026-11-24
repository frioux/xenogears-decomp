#include "common.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "battle/actor.h"
#include "system/math.h"

// Bits of a model-part transform command's flags byte: bits 0-2 pick the component,
// 0x20/0x40 add the node's current component to the start/target, 0x80 recurses.
#define BATTLE_NODE_COMPONENT_MASK 7
#define BATTLE_NODE_OFFSET_ADD_CURRENT 0x20
#define BATTLE_NODE_OFFSET_ADD_TARGET 0x40
#define BATTLE_NODE_RECURSIVE 0x80
// g_BattleBlockSlots holds the blocks BattleAllocBlock hands out, one per battle slot.
#define BATTLE_BLOCK_SLOT_COUNT 9

extern u8 D_800C3B84;
extern u8 D_800C3B88;
extern u8 D_800C3B8C;
extern s16 D_800C3B90;
extern s16 D_800C3B94;
extern s16 D_800C3B98;
extern u16 D_800C3B9C;
extern s16 D_800C3BA0;
extern s16 D_800C3BA4;
extern u16 D_800C3BA8;
extern SVECTOR g_BattleSlotVectors;
extern SVECTOR g_BattleSlotVectorsB;

extern u8 g_BattleAnimCommandsSuppressed;
extern u16 g_BattleCommandActorMask;
extern u8 g_BattleCurrentActorSlot;
extern u8 g_BattleExtraMatrixFlag;
extern BattleBlock *g_BattleBlockSlots[];
extern s32 D_800C3BCC[];
extern s32 g_BattlePrimCursor;
extern s32 g_BattlePrimCount;
extern s32 g_BattleCommandRowIndex;
extern u8 g_BattleActorCommandIds[];
extern void *g_BattleStageData;

extern void HeapFree(void *pMem);
extern BattleBlock *BattleAllocBlock(u8 *pPool);
extern void BattleFreeBlock(s32, u8 *);
extern s16 BattleFindTriangleContainingPoint(s16 *);
extern void BattleProjectPointOntoIndexedTriangle(s16 *, s16, void *);
extern s32 BattleFindGroundTriangle(s16 *, s16, s32);
extern void SoundRemoveSedsEntry(s32);
extern void BattleClearBlockSlots(void);
extern u8 g_BattleBlockPool[];
extern BattleActor *g_BattleActors[];
extern void func_800A9FF0(s32);
extern void BattleAppendActorCommandBytes(void *, void *, void *, void *);
extern void func_800BE790(void);
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ u16 z;
} BattleQueryPoint; // Size: 0x6

extern s32 BattleFindNearestActorForPoint(u16 *, BattleQueryPoint *);
extern s16 func_800B0B14(s32 index);

typedef struct BattleModelNode BattleModelNode;

// Node of a 0x7C-stride hierarchy whose element 0 heads the list.
typedef struct {
    /* 0x0 */ u8 pad0[0x4];
    /* 0x4 */ BattleModelNode *pNodes;
    /* 0x8 */ u8 pad8[0x58];
    /* 0x60 */ s16 ground; // replaces the node offset's y when the part asks for it
} BattleModel;

struct BattleModelNode {
    /* 0x00 */ BattleModelNode *pParent;
    /* 0x04 */ u8 unk4;
    /* 0x05 */ u8 unk5;
    /* 0x06 */ u8 pad6;
    /* 0x07 */ u8 unk7;
    /* 0x08 */ u8 pad8[0x2];
    /* 0x0A */ u16 unkA;
    /* 0x0C */ MATRIX localTransform;
    /* 0x2C */ MATRIX globalTransform;
    /* 0x4C */ u16 scale[3];
    /* 0x52 */ u16 pad52;
    /* 0x54 */ u16 rot[3];
    /* 0x5A */ u16 pad5A;
    /* 0x5C */ s32 pos[3];
    /* 0x68 */ u8 pad68[0x8];
    /* 0x70 */ BattleBlock *p70;
    /* 0x74 */ BattleBlock *p74;
    /* 0x78 */ BattleBlock *p78;
}; // Size: 0x7C

// One 0x1C-byte row of the per-slot position table at D_800C3EBE: x at +0, z at
// +2 and the height y at +4 (misc12.c reads the first two of a row as a 2D pair).
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 z;
    /* 0x4 */ u16 y;
    /* 0x6 */ u8 unk6[0x16];
} BattleSlotPosition; // Size: 0x1C

extern BattleSlotPosition D_800C3EBE[];

// Current frame published to the rest of the battle code.
typedef struct {
    /* 0x0 */ mem_addr unk0;
    /* 0x4 */ mem_addr unk4;
    /* 0x8 */ mem_addr unk8;
    /* 0xC */ mem_addr unkC;
    /* 0x10 */ mem_addr unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
} BattleFrame; // Size: 0x1C

// Streamed battle data; only the fields this unit reads are named.
typedef struct {
    /* 0x0 */ u8 pad0[0x34C];
    /* 0x34C */ s16 unk34C;
    /* 0x34E */ s16 unk34E;
    /* 0x350 */ s16 unk350;
    /* 0x352 */ s16 unk352;
} BattleStreamData;

extern BattleFrame D_800C3BD0[];

s16 BattleComputeScaledSine(s32 *pAxis, VECTOR *pV0, VECTOR *pV1, s32 divisor) {
    s32 vec[3];
    s32 dot;
    s32 len;

    OuterProduct12(pV0, pV1, (VECTOR *)vec);
    dot = vec[0] * pAxis[0] + vec[1] * pAxis[1] + vec[2] * pAxis[2];
    len = SquareRoot0(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]) + 1;
    dot = (dot << 4) / len;
    dot = (dot << 8) / divisor;
    return dot;
}

s32 BattleFindFirstActorInMask(void) {
    s32 i;

    for (i = 0; i < 13; i++) {
        if ((g_BattleCommandActorMask >> i) & 1) {
            break;
        }
    }
    return i;
}
s32 BattleResolveTargetSlot(BattleActor *pActor, u32 id, u16 *pOut) {
    if ((id & 0xFF) == 0xFF) {
        id = BattleFindFirstActorInMask();
    } else if ((id & 0xFF) == 0xFE) {
        id = g_BattleCurrentActorSlot;
    } else if ((id & 0xFF) == 0xFD || (id & 0xFF) == 0xF9) {
        id = pActor->actorSlot;
    } else if ((id & 0xFF) == 0xFC) {
        id = pActor->linkedActorSlot;
    } else if ((id & 0xFF) == 0xFA) {
        id = 0x1F;
    } else if ((id & 0xFF) == 0xF8) {
        id = pActor->actorSlot * 2 + 0xD;
    } else if ((id & 0xFF) == 0xF7) {
        id = pActor->actorSlot * 2 + 0xE;
    }
    *pOut = 1 << (id & 0xFF);
    return id & 0xFF;
}
extern BattleTableEntry D_800CCD1E[];
extern u8 BattleGetActorActionConditionMask(u8);

s32 BattleGetActorActionEntry(BattleActor *pActor, u32 id, s32 *pOut) {
    u32 sel = id;
    u8 kind;

    *pOut = 0;
    if ((sel & 0xFF) >= 0xFE) {
        if ((sel & 0xFF) == 0xFF) {
            kind = BattleGetActorActionConditionMask(pActor->actorSlot);
            if ((kind & 4) && (pActor->flags4A & 0x100)) {
                pActor->commands[0] = 0x1B;
            } else if ((kind & BATTLE_ACTION_CONDITION_LOW_HP) && (pActor->flags4A & 0x80)) {
                if (!(D_800CCD1E[pActor->actorSlot].flags & 1)) {
                    pActor->commands[0] = 6;
                }
            } else if (!(pActor->flags4A & 0x400)) {
                pActor->commands[0] = 1;
            }
            if (kind & 1) {
                pActor->commands[0] |= 0x80;
            }
        }
        sel = pActor->commands[0] & 0x7F;
        *pOut = pActor->commands[0] & 0x80;
    }
    if ((sel & 0xFF) < 0x40) {
        return ((s32 *)(u32)pActor->pActionTableLow)[(sel & 0xFF) + 1];
    }
    return ((s32 *)(u32)pActor->pActionTableHigh)[(sel & 0xFF) - 0x3F];
}
// Queues a transform command on pNode (and, with flag 0x80, on every child) and
// applies the command's target transform to the node when it is issued directly
// (command < 2). flags & 7 selects which component the command drives: 0 rotation,
// 1 position, otherwise scale; flags & 0x20 / & 0x40 add the node's current
// component to the target / reference vector.
void BattleModelQueueTransformCommand(BattleModel *pModel, u8 *pPool, BattleModelNode *pNode, u8 flags, u8 command, u8 unk3,
                   u8 unk1, s16 x, s16 y, s16 z, s16 dx, s16 dy, s16 dz, s16 duration) {
    u8 mode;
    BattleBlock *pBlock;
    BattleModelNode *pChild;
    u16 addX;
    u16 addY;
    u16 addZ;
    u16 dstX;
    u16 dstY;
    u16 dstZ;
    s32 i;

    mode = flags & BATTLE_NODE_COMPONENT_MASK;
    if (mode == 0) {
        pBlock = pNode->p70;
    } else if (mode == 1) {
        pBlock = pNode->p74;
    } else {
        pBlock = pNode->p78;
    }
    if (pBlock == NULL) {
        pBlock = BattleAllocBlock(pPool);
    }
    if (pBlock != NULL) {
        pBlock->inUse = 1;
        pBlock->flags = unk1;
        pBlock->opcode = command + 3;
        pBlock->tag = unk3;
        if (flags & BATTLE_NODE_OFFSET_ADD_CURRENT) {
            if (mode == 0) {
                addX = pNode->rot[0];
                addY = pNode->rot[1];
                addZ = pNode->rot[2];
            } else if (mode == 1) {
                addX = pNode->pos[0];
                addY = pNode->pos[1];
                addZ = pNode->pos[2];
            } else {
                addX = pNode->scale[0];
                addY = pNode->scale[1];
                addZ = pNode->scale[2];
            }
        } else {
            addX = 0;
            addY = 0;
            addZ = 0;
        }
        if (flags & BATTLE_NODE_OFFSET_ADD_TARGET) {
            if (mode == 0) {
                dstX = pNode->rot[0];
                dstY = pNode->rot[1];
                dstZ = pNode->rot[2];
            } else if (mode == 1) {
                dstX = pNode->pos[0];
                dstY = pNode->pos[1];
                dstZ = pNode->pos[2];
            } else {
                dstX = pNode->scale[0];
                dstY = pNode->scale[1];
                dstZ = pNode->scale[2];
            }
        } else {
            dstX = 0;
            dstY = 0;
            dstZ = 0;
        }
        pBlock->startX = x + addX;
        pBlock->startY = y + addY;
        pBlock->startZ = z + addZ;
        if (command == 0) {
            pBlock->targetX = dx + dstX - pBlock->startX;
            pBlock->targetY = dy + dstY - pBlock->startY;
            pBlock->targetZ = dz + dstZ - pBlock->startZ;
        } else {
            pBlock->targetX = dx + dstX;
            pBlock->targetY = dy + dstY;
            pBlock->targetZ = dz + dstZ;
        }
        pBlock->curFrame = 0;
        pBlock->endFrame = duration;
        if (mode == 0) {
            if (command < 2) {
                pNode->rot[0] = pBlock->startX;
                pNode->rot[1] = pBlock->startY;
                pNode->rot[2] = pBlock->startZ;
            }
            pNode->p70 = pBlock;
        } else if (mode == 1) {
            if (command < 2) {
                pNode->pos[0] = pBlock->startX;
                pNode->pos[1] = pBlock->startY;
                pNode->pos[2] = pBlock->startZ;
            }
            pNode->p74 = pBlock;
        } else {
            if (command < 2) {
                pNode->scale[0] = pBlock->startX;
                pNode->scale[1] = pBlock->startY;
                pNode->scale[2] = pBlock->startZ;
            }
            pNode->p78 = pBlock;
        }
    }
    if (flags & BATTLE_NODE_RECURSIVE) {
        pChild = pModel->pNodes;
        for (i = 1; i < pModel->pNodes->unkA; i++) {
            pChild++;
            if (pChild->pParent == pNode) {
                BattleModelQueueTransformCommand(pModel, pPool, pChild, flags & 0xFF, command, unk3, unk1, x, y, z, dx, dy, dz,
                              duration);
            }
        }
    }
}
void BattleSetModelNodeFlagRecursive(BattleModel *pModel, BattleModelNode *pNode, s32 flags) {
    BattleModelNode *pChild;
    s32 i;

    pNode->unk7 = flags & 1;
    if (flags & BATTLE_NODE_RECURSIVE) {
        pChild = pModel->pNodes;
        for (i = 1; i < pModel->pNodes->unkA; i++) {
            pChild++;
            if (pChild->pParent == pNode) {
                BattleSetModelNodeFlagRecursive(pModel, pChild, flags);
            }
        }
    }
}

// Callback the work list runs every frame (TimerWorkListSetTaskCallback).
typedef void (*BattleWorkCallback)(void *pTask);

// Appended block of the per-part task func_80023FD8 allocates (same layout as
// lib_gear's LibGearJointTask). Its byte offset is the s16 at +0xBE of the task,
// which is the sprite's allocatedDataSize.
typedef struct {
    /* 0x0  */ u8 unk0[0x4];
    /* 0x4  */ BattleWorkCallback pChainCallback; // the callback BattlePartMatrixTask chains to
    /* 0x8  */ BattleModel *pModel;
    /* 0xC  */ s16 partIndex;
    /* 0xE  */ s16 useGroundY; // non-zero: the model's ground replaces the offset's y
    /* 0x10 */ SVECTOR offset;
} BattlePartNode; // Size: 0x18

// Per-part data BattleCreateModelPartTask copies into the node it creates.
typedef struct {
    /* 0x0  */ u8 unk0[0x5];
    /* 0x5  */ u8 partIndex;
    /* 0x6  */ u16 offsetX;
    /* 0x8  */ u16 offsetY;
    /* 0xA  */ u16 offsetZ;
    /* 0xC  */ u8 useGroundY;
    /* 0xD  */ u8 padD[0x6];
    /* 0x13 */ u8 unk13; // non-zero: installs BattlePartMatrixTask as the task callback
} BattlePartInfo; // Size: 0x14

// Task func_80023FD8 allocates for one part: a SpriteData at +0x38 and, at the
// byte offset the sprite records in its allocatedDataSize (+0xBE), the appended
// BattlePartNode above.
typedef struct {
    /* 0x00 */ u8 unk0[0x38];
    /* 0x38 */ SpriteData sprite;
} BattlePartTask;

extern BattlePartTask *func_80023FD8(s32 animIndex, void *pAnimFile, void *pOffset, s32 nodeSize);
extern void SpriteSetDirection(SpriteData *pSprite, s16 direction);
extern void func_800223B0(SpriteData *pSprite, s16 direction);
extern void SpriteSetScale(SpriteData *pSprite, s16 scale);
extern BattleWorkCallback WorkListTaskGetTaskCallback(void *pTask);
extern void TimerWorkListSetTaskCallback(void *pTask, BattleWorkCallback pCallback);
extern void BattlePartMatrixTask(void *pTask);

void BattleCreateModelPartTask(void *pAnimFile, s32 animIndex, void *pOffset, s16 direction, s16 scale,
                               BattlePartInfo *pInfo, BattleModel *pModel) {
    BattlePartTask *pTask;
    SpriteData *pSprite;
    BattlePartNode *pNode;

    pTask = func_80023FD8(animIndex, pAnimFile, pOffset, sizeof(BattlePartNode));
    pSprite = &pTask->sprite;
    SpriteSetDirection(pSprite, direction);
    func_800223B0(pSprite, direction);
    SpriteSetScale(pSprite, scale);
    pNode = (BattlePartNode *)((u8 *)pTask + pSprite->allocatedDataSize);
    pNode->pModel = pModel;
    pNode->partIndex = pInfo->partIndex;
    if (pInfo->unk13 != 0) {
        pNode->pChainCallback = WorkListTaskGetTaskCallback(pTask);
        TimerWorkListSetTaskCallback(pTask, BattlePartMatrixTask);
        pNode->offset.vx = pInfo->offsetX;
        pNode->offset.vy = pInfo->offsetY;
        pNode->offset.vz = pInfo->offsetZ;
        pNode->useGroundY = pInfo->useGroundY;
    }
}

void BattlePartMatrixTask(void *pTaskRaw) {
    BattlePartTask *pTask;
    BattlePartNode *pNode;
    MATRIX *pMatrix;
    LVEC3 vec;

    pTask = pTaskRaw;
    pNode = (BattlePartNode *)((u8 *)pTask + pTask->sprite.allocatedDataSize);
    pMatrix = PSX_SCRATCH;
    if (pNode->partIndex != 0) {
        CompMatrix(&pNode->pModel->pNodes[0].localTransform,
                   &pNode->pModel->pNodes[pNode->partIndex].globalTransform, pMatrix);
    } else {
        pMatrix = &pNode->pModel->pNodes[0].localTransform;
    }
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    gte_ldv0(&pNode->offset);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(&vec);
    if (pNode->useGroundY != 0) {
        vec.y = pNode->pModel->ground;
    }
    pTask->sprite.position.x = CONV_FROM_GTE(vec.x);
    pTask->sprite.position.y = CONV_FROM_GTE(vec.y);
    pTask->sprite.position.z = CONV_FROM_GTE(vec.z);
    pNode->pChainCallback(pTask);
}


void BattleApplyNodeOffset(BattleModel *pModel, BattleModelNode *pNode, char flags, s16 x, s16 y, s16 z) {
    BattleModelNode *pChild;
    s32 mode;
    s32 i;

    mode = flags & BATTLE_NODE_COMPONENT_MASK;
    if (mode == 0) {
        if (flags & BATTLE_NODE_OFFSET_ADD_CURRENT) {
            pNode->rot[0] += x;
            pNode->rot[1] += y;
            pNode->rot[2] += z;
        } else {
            pNode->rot[0] = x;
            pNode->rot[1] = y;
            pNode->rot[2] = z;
        }
    } else if (mode == 1) {
        if (flags & BATTLE_NODE_OFFSET_ADD_CURRENT) {
            pNode->pos[0] += x;
            pNode->pos[1] += y;
            pNode->pos[2] += z;
        } else {
            pNode->pos[0] = x;
            pNode->pos[1] = y;
            pNode->pos[2] = z;
        }
    } else if (flags & BATTLE_NODE_OFFSET_ADD_CURRENT) {
        pNode->scale[0] += x;
        pNode->scale[1] += y;
        pNode->scale[2] += z;
    } else {
        pNode->scale[0] = x;
        pNode->scale[1] = y;
        pNode->scale[2] = z;
    }
    pNode->unk4 = 1;
    pNode->unk5 = 1;
    if (flags & BATTLE_NODE_RECURSIVE) {
        pChild = pModel->pNodes;
        for (i = 1; i < pModel->pNodes->unkA; i++) {
            pChild++;
            if (pChild->pParent == pNode) {
                BattleApplyNodeOffset(pModel, pChild, flags, x, y, z);
            }
        }
    }
}

void BattleSnapActorToGround(BattleActor *pActor) {
    u8 unk10[0x10];
    s16 vec[3];
    s16 index;

    vec[0] = ((u16 *)pActor->pSprites->pos)[0];
    vec[1] = ((u16 *)pActor->pSprites->pos)[2];
    vec[2] = ((u16 *)pActor->pSprites->pos)[4];
    index = BattleFindGroundTriangle(vec, pActor->groundTriangleIndex, 4);
    pActor->groundTriangleIndex = index;
    if (index < 0) {
        index = BattleFindTriangleContainingPoint(vec);
        pActor->groundTriangleIndex = index;
    }
    BattleProjectPointOntoIndexedTriangle(vec, pActor->groundTriangleIndex, unk10);
    pActor->groundY = vec[1];
    if (pActor->skipGroundSnap == 0) {
        pActor->pSprites->pos[1] = vec[1];
    }
}

void BattleFreeActorBuffer(BattleActor *pActor) {
    if (pActor->pBuffer != 0) {
        if (pActor->hasSedsEntry2 != 0) {
            SoundRemoveSedsEntry(((s32 *)pActor->pSoundBank2)[2]);
            pActor->hasSedsEntry2 = 0;
        }
        HeapFree((void *)pActor->pBuffer);
        pActor->pBuffer = 0;
        pActor->pActionTableHigh = 0;
    }
}

void BattleClearBlockSlots(void) {
    s32 i = BATTLE_BLOCK_SLOT_COUNT - 1;
    s32 *p = D_800C3BCC;

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}

void BattleReleaseBlockSlots(s32 arg0) {
    s32 i;

    for (i = 0; i < BATTLE_BLOCK_SLOT_COUNT; i++) {
        if (g_BattleBlockSlots[i] != NULL) {
            BattleFreeBlock(arg0, g_BattleBlockSlots[i]);
        }
    }
    BattleClearBlockSlots();
}

void BattleSetBlockSlotCommand(u8 *pPool, s32 index, u8 unk2, u8 unk3, s16 x, s16 y, s16 z, s16 dx,
                   s16 dy, s16 dz, s16 duration) {
    BattleBlock *pBlock;

    if (g_BattleAnimCommandsSuppressed == 0) {
        if (g_BattleBlockSlots[index] == NULL) {
            g_BattleBlockSlots[index] = BattleAllocBlock(pPool);
        }
        pBlock = g_BattleBlockSlots[index];
        if (pBlock != NULL) {
            pBlock->inUse = 1;
            pBlock->flags = 0;
            pBlock->opcode = unk2;
            pBlock->tag = unk3;
            pBlock->startX = x;
            pBlock->startY = y;
            pBlock->startZ = z;
            pBlock->targetX = dx;
            pBlock->targetY = dy;
            pBlock->targetZ = dz;
            pBlock->curFrame = 0;
            pBlock->endFrame = duration;
        }
    }
}
// PARKED 96.08% (2120 bytes = retail's exact size, 192 differing lines against the
// previous park's 236). The one residual is the callee-saved permutation: retail hands
// out $s1=x $s2=y $s3=z $s4=orbitY $s5=pBlock $s6=pTween $s7=outX $s0=orbitR, ours gives
// pBlock $s3, z $s4, orbitY $s5 (x, y, pTween, outX and orbitR already match). That
// permutation is also why retail's prologue has two block-pointer moves where ours has
// `move s6,sX` plus a `nop`, and why the two midpoint blocks re-schedule their
// sll/sra/lhu order.
// KEPT THIS ROUND: the tween pointer also carries the midpoint reads - `pTween->targetZ`
// for the ">= 0" test and `pTween->targetY` in both midpoint arms - which is exactly
// retail's $s6 access set (lh 0xE(s6) and lh 0xC(s6) twice, plus the startX/Y/Z group)
// and measured 95.81 -> 96.08% at the same 2120 bytes.
// MEASURED DEAD THIS ROUND, in one line each: cc1 -da/-dl on this body says pBlock 66
// refs/276 insns, pTween 48/241, x 48/160, y 39/135, z 48/180, and pBlock is still
// processed before z and orbitY, so it keeps taking an earlier register (the allocation
// order is not a monotone function of that quotient, so the dump can say where pBlock
// sits but not how to move it); moving the curFrame/endFrame group (8 sites) to pTween
// 95.55% at 2120; s32 x/y/z 95.53% at 2108 (also 12 bytes small, so retail's x/y/z are
// the 2-byte locals the lhu loads already say); `kind = pTween->opcode` with the
// assignment reordered is byte-identical because copy propagation folds it back.
// Preset screen this round: 2.6.0 -O2 -G8 95.81 (the unit preset, best), 2.7.2-psx
// 94.12, 2.7.2-cdk 82.19, -O3 95.80, so there is no preset lead.
// Older parks measured dead: y through a u16 local, z += vs z = z +, outX/Y/Z as
// separate results, declaration order, and five permuter runs (none reached 0).
// Runs every queued movement command for arg1 frames. A block interpolates its sprite
// position from its start to its target and publishes the result: slots 0..6 into their
// own word, 7 and 8 into the two slot vectors. A block whose tag is D_800C3B84 reports
// to D_800C3B88 - bit 1 while it still runs, bit 2 once it has finished and been freed.
/*
void func_800B026C(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    u16 vec[3];
    s32 i;
    u16 x;
    u16 y;
    u16 z;
    u16 outX;
    u16 outY;
    u16 outZ;
    u8 kind;
    s32 slot;
    s32 frame;
    s32 stepX;
    s32 stepY;
    s32 stepZ;
    s16 rateX;
    s16 rateY;
    s16 rateZ;
    s32 dy;
    s32 orbitY;
    s32 orbitR;
    s32 height;
    BattleBlock *pBlock;
    BattleBlock *pTween;
    BattleActor *pActor;
    BattleSprite *pSprite;

    D_800C3B88 = 0;
    for (i = 0; i < arg1; i++) {
        for (slot = 0; slot < 9; slot++) {
            pBlock = g_BattleBlockSlots[slot];
            if (pBlock == NULL) {
                continue;
            }
            kind = pBlock->opcode;
            pTween = pBlock;
            if ((kind & 0xF) < 2) {
                // These two commands aim between two actors: targetX and targetZ
                // name them, targetY is a height taken off the result.
                pActor = g_BattleActors[pBlock->targetX];
                if (pActor != NULL) {
                    pSprite = pActor->pSprites;
                    x = (u16)pSprite->pos[0];
                    dy = (u16)pSprite->pos[1] - (u16)pBlock->targetY;
                    z = (u16)pSprite->pos[2];
                } else {
                    x = D_800C3EBE[pBlock->targetX].x;
                    dy = D_800C3EBE[pBlock->targetX].y - (u16)pBlock->targetY;
                    z = D_800C3EBE[pBlock->targetX].z;
                }
                y = dy;
                if (pTween->targetZ >= 0) {
                    pActor = g_BattleActors[pTween->targetZ];
                    if (pActor != NULL) {
                        pSprite = pActor->pSprites;
                        x = ((s16)x + pSprite->pos[0]) / 2;
                        y = ((s16)y + pSprite->pos[1] - pTween->targetY) / 2;
                        z = ((s16)z + pSprite->pos[2]) / 2;
                    } else {
                        x = ((s16)x + D_800C3EBE[pTween->targetZ].x) / 2;
                        y = ((s16)y + D_800C3EBE[pTween->targetZ].y - pTween->targetY) / 2;
                        z = ((s16)z + D_800C3EBE[pTween->targetZ].z) / 2;
                    }
                }
            } else {
                x = pBlock->targetX;
                y = pBlock->targetY;
                z = pBlock->targetZ;
            }
            if (slot == 7) {
                y -= D_800C3BA8;
                x += (-D_800C3BA4 * rcos(D_800C3BA0)) / 4096;
                z += (-D_800C3BA4 * rsin(D_800C3BA0)) / 4096;
            }
            if (slot == 8) {
                orbitY = (D_800C3B98 * rcos(D_800C3B94)) / 4096;
                orbitR = -((D_800C3B98 * rsin(D_800C3B94)) / 4096);
                x += (orbitR * rcos(D_800C3B90)) / 4096;
                y += -orbitY - D_800C3B9C;
                z = z + ((orbitR * rsin(D_800C3B90)) / 4096);
                vec[0] = x;
                vec[1] = y;
                vec[2] = z;
                if (BattleSteerActorsTowardPoint((u16 *)&g_BattleSlotVectorsB, vec) != 0) {
                    x = vec[0];
                    z = vec[2];
                }
                height = func_800B0B14(arg3);
                if ((s16)height < (s16)y) {
                    y = height;
                }
            }
            if (D_800C3B8C != 0 && slot >= 7) {
                pTween->startX = x;
                pTween->startY = y;
                pTween->startZ = z;
            }
            switch (kind & 1) {
            case 0:
                frame = (s16)((u16)pBlock->curFrame + 1);
                stepX = ((s16)x - pTween->startX) * frame / pBlock->endFrame;
                stepY = ((s16)y - pTween->startY) * frame / pBlock->endFrame;
                stepZ = ((s16)z - pTween->startZ) * frame / pBlock->endFrame;
                outX = pTween->startX + stepX;
                outY = pTween->startY + stepY;
                outZ = pTween->startZ + stepZ;
                break;
            case 1:
                rateX = ((s16)x - pTween->startX) / pBlock->endFrame;
                rateY = ((s16)y - pTween->startY) / pBlock->endFrame;
                rateZ = ((s16)z - pTween->startZ) / pBlock->endFrame;
                outX = pTween->startX;
                outY = pTween->startY;
                outZ = pTween->startZ;
                if (!(rateX != 0 || rateY != 0 || rateZ != 0)) {
                    pBlock->curFrame = pBlock->endFrame;
                } else {
                    outX += rateX;
                    outY += rateY;
                    outZ += rateZ;
                    pBlock->curFrame = 0;
                }
                pTween->startX = outX;
                pTween->startY = outY;
                pTween->startZ = outZ;
                break;
            }
            pBlock->curFrame = (u16)pBlock->curFrame + 1;
            if ((s16)pBlock->curFrame >= pBlock->endFrame) {
                if (!(slot < 7)) {
                    pBlock->curFrame = pBlock->endFrame - 1;
                } else {
                    if (pBlock->tag == D_800C3B84) {
                        D_800C3B88 |= 2;
                    }
                    BattleFreeBlock(arg0, (u8 *)pBlock);
                    g_BattleBlockSlots[slot] = NULL;
                }
            } else {
                if (pBlock->tag == D_800C3B84) {
                    D_800C3B88 |= 1;
                }
            }
            switch (slot) {
            case 0:
                D_800C3B90 = outX;
                break;
            case 1:
                D_800C3BA0 = outX;
                break;
            case 2:
                D_800C3B94 = outX;
                break;
            case 3:
                D_800C3B98 = outX;
                break;
            case 4:
                D_800C3BA4 = outX;
                break;
            case 5:
                D_800C3BA8 = outX;
                break;
            case 6:
                D_800C3B9C = outX;
                break;
            case 7:
                g_BattleSlotVectorsB.vx = outX;
                g_BattleSlotVectorsB.vy = outY;
                g_BattleSlotVectorsB.vz = outZ;
                break;
            case 8:
                g_BattleSlotVectors.vx = outX;
                g_BattleSlotVectors.vy = outY;
                g_BattleSlotVectors.vz = outZ;
                break;
            }
        }
    }
    D_800C3B8C = 0;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B026C);


u16 BattleIsPointWithinBounds(s16 *pPos) {
    BattleStreamData *pData = g_BattleStageData;

    if (pPos[0] > pData->unk34E && pPos[0] < pData->unk34C && pPos[2] > pData->unk350 &&
        pPos[2] < pData->unk352) {
        return 1;
    }
    return 0;
}
extern s16 D_800C3542;
extern u16 D_800C3544;
extern s16 D_800C3546;
// Unsigned-x / signed-y view of the current slot position vector, so the two
// halfword reads keep the widths the original loads use.
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ s16 vy;
    /* 0x4 */ u16 vz;
} BattleSlotVectorWords; // Size: 0x8
extern BattleSlotVectorWords g_BattleSlotVectorWords[] __asm__("g_BattleSlotVectors");
extern u16 D_800D3358;
extern u16 D_800D335C;
extern s16 D_800D335E;
extern u16 D_800D3360;

// 96.47% (612 bytes vs retail's 604, 153 insns vs 151). The residual is TWO INSTRUCTIONS, one per A.vy site: retail reads the live symbol base with a displacement (lh 0x2(s5) / lhu 0x2(s5)) while cc1 2.6.0 prints every non-zero constant offset off a known symbol address as a bare symbol+offset macro, which maspsx expands into lui plus load (2 insns). Proved by cc1_probe.sh on this unit: struct field, mem_addr arithmetic (base and base+2) and s16-pointer index spellings ALL emit the same lhu 0($17) for offset 0 and lhu g_A+2 for offset 2, so no source form moves the +2 off a register under 2.6.0/2.7.2. cc1 2.7.2-cdk does emit 2($18), but its la is lui %hi into one register plus addiu %lo into a second with the loads written %lo(hi) - retail has one register holding the full address - so cdk does not close it either. Measured: unit flags 96.47, 2.7.2 96.21, cdk 97.36, cdk -G0 83.71, 2.6.0 -O1 61.64, -O3 70.92. The unit cannot switch to cdk regardless: its matched functions drop (BattleComputeScaledSine 88.95, BattleModelQueueTransformCommand 89.43, BattleGetActorActionEntry 96.08, BattleSetBlockSlotCommand 92.73, BattleIsPointWithinBounds 79.38, BattleFindNearestActorForPoint 84.91). What the body has and must not be lost: (1) the asm-label array alias extern BattleSlotVectorWords g_BattleSlotVectorWords[] __asm__("g_BattleSlotVectors") plus pVectors = g_BattleSlotVectorWords is what puts the address in a callee-saved register at all - scalar &g_BattleSlotVectors, a plain pointer local, a u16 pointer cast and a mem_addr assignment all fold it away and score 93.07-93.27; (2) the B-vector and A.vz reads are spelled D_800D335C/335E/3360/3358, which is what the expected object relocates to. Measured and dead: struct field vs u16 pointer plus cast (identical code), a volatile pointer (probe: still folds), inlining the vector, chopping dx/dy/dz types, and casting the BattleIsPointWithinBounds result to s16 (the andi 0xffff stays - the callee is u16 in this TU). Left over with no size effect: that andi 0xffff against retail's sll 0x10, and the register field of the sll after BattleFindTriangleContainingPoint (retail sll v0,v0 vs our sll v0,s3).
/*
s16 func_800B0B14(s32 index) {
    u8 work[0x10];
    s16 vec[3];
    s16 result;
    s16 dx;
    s32 dy;
    s16 dz;
    s32 sx;
    s32 sy;
    s32 sz;
    BattleSlotVectorWords *pVectors;
    s32 len;

    pVectors = g_BattleSlotVectorWords;
    dx = D_800D335C - pVectors->vx;
    dy = (D_800D335E - pVectors->vy) / 4;
    dz = D_800D3360 - D_800D3358;
    len = SquareRoot0(dx * dx + dy * dy + dz * dz) + 1;
    sx = (dx << 9) / len;
    sy = (dy << 9) / len;
    sz = (dz << 9) / len;
    vec[0] = pVectors->vx + sx;
    vec[1] = pVectors->vy + sy;
    vec[2] = D_800D3358 + sz;
    result = -1;
    if (BattleIsPointWithinBounds(vec) != 0 && index != D_800C3546) {
        D_800C3546 = index;
        result = BattleFindGroundTriangle(vec, D_800C3542, 5);
        if (result < 0) {
            result = BattleFindTriangleContainingPoint(vec);
        }
        if (result >= 0) {
            D_800C3542 = result;
        }
    }
    if (result >= 0) {
        BattleProjectPointOntoIndexedTriangle(vec, D_800C3542, work);
        D_800C3544 = vec[1];
    } else {
        vec[1] = D_800C3544;
    }
    return vec[1];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0B14);



extern s32 BattleGetActorHeight(s32 index);
extern s32 BattleGetActorRadius(s32 index);

s32 BattleFindNearestActorForPoint(u16 *pPos, BattleQueryPoint *pOut) {
    s32 i;
    u16 radius;
    s16 bestSlot;
    u16 bestRadius;
    u16 bestX;
    u16 bestZ;
    s32 x;
    s32 px;
    s32 pz;
    s32 z;
    s16 dx;
    s16 dz;
    s32 refY;
    s32 len;
    BattleActor **ppActor;
    BattleActor **pBase;

    bestSlot = -1;
    for (i = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        radius = BattleGetActorRadius(i) + 0x80;
        pBase = g_BattleActors;
        ppActor = &pBase[i];
        if (*ppActor == NULL) {
            continue;
        }
        if ((*ppActor)->doUpdate == 0) {
            continue;
        }
        refY = pOut->y;
        if ((*ppActor)->pSprites->pos[1] - BattleGetActorHeight(i) >= refY) {
            continue;
        }
        if (bestSlot >= 0 && (s16)radius <= (s16)bestRadius) {
            continue;
        }
        px = (s16)pPos[0];
        pz = (s16)pPos[2];
        x = ((u16 *)(*ppActor)->pSprites->pos)[0] - px / 6;
        z = ((u16 *)(*ppActor)->pSprites->pos)[4] - pz / 6;
        dx = pOut->x - x;
        dz = pOut->z - z;
        len = SquareRoot0(dx * dx + dz * dz) + 1;
        if (len >= (s16)radius) {
            continue;
        }
        bestSlot = i;
        bestRadius = radius;
        bestX = x + dx * (s16)radius / len;
        bestZ = z + dz * (s16)radius / len;
    }
    if (bestSlot >= 0) {
        pOut->x = bestX;
        pOut->z = bestZ;
        return bestSlot + 1;
    } else {
        return 0;
    }
}
s32 BattleSteerActorsTowardPoint(u16 *pA, u16 *pB) {
    s16 vec[3];
    s32 len;

    vec[0] = pA[0] - pB[0];
    vec[2] = pA[2] - pB[2];
    len = SquareRoot0(vec[0] * vec[0] + vec[2] * vec[2]) + 1;
    vec[0] = (vec[0] << 9) / len;
    vec[2] = (vec[2] << 9) / len;
    return BattleFindNearestActorForPoint(vec, pB);
}
void BattlePushActorOutOfCircle(s32 index, s32 x, s32 z, s32 dist) {
    BattleActor **pBase = g_BattleActors;
    BattleActor **pp = &pBase[index];
    BattleSprite *pSprite;
    s32 dx;
    s32 dz;
    s32 len;
    s32 rem;
    s32 angle;
    u32 diff;

    pSprite = (*pp)->pSprites;
    dx = x - pSprite->pos[0];
    dz = z - pSprite->pos[2];
    len = SquareRoot0(dx * dx + dz * dz) + 1;
    if (len >= dist) {
        return;
    }
    angle = ratan2(dz, -dx);
    diff = (u32)(((*pp)->pSprites->rot[1] - angle) & 0xFFF) - 0x401;
    if (diff < 0x7FF) {
        angle += 0x800;
    }
    rem = dist - len;
    (*pp)->pSprites->pos[0] = x - dx * dist / len - rcos(angle) * rem / 4096;
    (*pp)->pSprites->pos[2] = z - dz * dist / len - rsin(angle) * rem / 4096;
}
s32 BattleTestActorCommandMask(s32 arg0, s32 arg1) {
    s32 result = 0;
    s32 v;
    u8 *pRow = &g_BattleActorCommandIds[(g_BattleCommandRowIndex - 1) * 0x48];
    u8 kind = pRow[arg0];

    if (kind < 6) {
        switch (kind) {
        case 0:
        case 1:
            v = arg1 & 1;
            break;
        case 5:
            v = arg1 & 2;
            break;
        case 4:
            v = arg1 & 4;
            break;
        case 2:
        case 3:
            v = arg1 & 8;
            break;
        }
        if (v == 0) {
            result = 1;
        }
    }
    if ((arg1 & 0xFF) == 0) {
        result = 1;
    }
    return result;
}
extern s32 BattleGetAdjustedWaitCountdown(void);
extern s32 ArchiveDataSync(void);
extern void ArchiveStopTransfer(s32);

void BattleWaitAndFreeActorBuffers(void) {
    s32 flagB = 0;
    BattleActor **pBase = g_BattleActors;
    s32 flagA = 0;
    BattleActor *pEntry;
    s32 i;
    BattleActor **pp;
    BattleActor **ppBase;

    for (;;) {
        flagA = 0;
        for (i = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
            if (pBase[i] != NULL) {
                if (pBase[i]->isBusy != 0) {
                    flagA = 1;
                }
                if (pBase[i]->pBuffer != 0) {
                    flagB = 1;
                }
            }
        }
        if (BattleGetAdjustedWaitCountdown() != 0) {
            flagA = 1;
        }
        if (flagA != 0) {
            func_800BE790();
            continue;
        }
        break;
    }
    if (flagB == 0) {
        return;
    }
    ArchiveStopTransfer(0);
    flagA = 1;
    ppBase = g_BattleActors;
    while (flagA != 0) {
        if (ArchiveDataSync() == 0) {
            for (i = 0, pp = ppBase; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
                pEntry = *pp++;
                if (pEntry != NULL && pEntry->pBuffer != 0) {
                    BattleFreeActorBuffer(pEntry);
                }
            }
            flagA = 0;
        }
        if (flagA == 0) {
            break;
        }
        func_800BE790();
    }
}
void BattleSetExtraMatrixFlag(void) {
    g_BattleExtraMatrixFlag = 1;
}
void BattleWaitForOtherActorAnimations(s32 arg0) {
    s32 i;
    s32 flag;
    BattleActor *pActor;

    for (i = 0; i < 3; i++) {
        if (i != arg0) {
            BattleAppendActorCommandBytes(g_BattleActors[i], g_BattleActors[i], g_BattleBlockPool, 5);
        }
    }
    do {
        flag = 0;
        for (i = 0; i < 3; i++) {
            if (i != arg0) {
                pActor = g_BattleActors[i];
                if (pActor != NULL && (pActor->doUpdate != 0 || pActor->isBusy != 0)) {
                    flag = 1;
                }
            }
        }
        func_800BE790();
    } while (flag != 0);
    func_800BE790();
    for (i = 0; i < 3; i++) {
        if (i != arg0) {
            func_800A9FF0(i);
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B15D8);
/* PRESET: 71.38% under this unit's flags (XenoBattle260Div, gcc 2.6.0 -G8),
   100.00% under `--gcc gcc-2.7.2-cdk-psx --gp -G0` with this exact body.
   Re-measured round 305 with tools/scripts/try_flags.sh (the old note's provenance
   was suspect, the number stands), and the unit cannot switch: under cdk -G0 the
   already-matched functions fall to 55-99.9 (BattleSnapActorToGround/
   BattleFreeActorBuffer/BattleClearBlockSlots stay 100, BattleSetExtraMatrixFlag
   55.00, BattleIsPointWithinBounds 78.96), so this stub needs a yaml split.
   Why it is compiler-bound, from the diff: cc1 2.7.2-cdk splits the symbol address
   itself, so retail gets `lui $v0,%hi(g_BattlePrimCount)` + `sw $zero,%lo(...)($v0)`
   and reads D_800C3BD0's tail through `lw 8($a0)`/`lw 16($a0)` off one address
   register, where 2.6.0 emits the bare `sw $zero,sym` macro (maspsx expands it
   through $at) and re-forms the +8/+0x10 addresses as absolute D_800C3BD8/
   D_800C3BE0 symbol refs; cdk also chunks the 7-word copy 3+3+1 like retail where
   2.6.0 uses 4+3. Source shape confirmed either way: offset = index*0x1C + 0xC in
   its own local (without it gcc folds +0xC into every displacement), unk0 read
   through the plain symbol so its %hi is CSE'd with the copy's, unk8/unk10 through
   the pDst local. Tried under 2.6.0: pointer local, whole-body local, global name
   at every reference, no offset local, -G0 (identical 71.38), array extern
   (identical).
s32 func_800B15D8(u8 *p, s32 index) {
    BattleFrame *pSrc;
    BattleFrame *pDst;
    s32 offset;

    offset = index * 0x1C + 0xC;
    pSrc = (BattleFrame *)(p + offset);
    pDst = D_800C3BD0;
    *pDst = *pSrc;
    if ((*(u32 *)(p + 4) & 1) == 0) {
        D_800C3BD0[0].unk0 = D_800C3BD0[0].unk0 + (mem_addr)pSrc;
        pDst->unk8 = pDst->unk8 + (mem_addr)pSrc;
        pDst->unk10 = pDst->unk10 + (mem_addr)pSrc;
    }
    g_BattlePrimCount = 0;
    g_BattlePrimCursor = pDst->unk10;
    return pSrc->unk14;
}
*/
