#include "common.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "battle/actor.h"

extern u8 D_800C37C8;
extern u16 D_800C3E30;
extern u8 D_800C3D40;
extern u8 D_800C3D6C;
extern u8 *D_800C3BAC[];
extern s32 D_800C3BCC[];
extern s32 D_800C3BEC;
extern s32 D_800C3BF0;
extern s32 D_800C360C;
extern u8 D_800C4000[];
extern void *D_800658C8;

extern void HeapFree(void *pMem);
extern BattleBlock *BattleAllocBlock(u8 *pPool);
extern void BattleFreeBlock(s32, u8 *);
extern s16 BattleFindTriangleContainingPoint(s16 *);
extern void BattleProjectPointOntoIndexedTriangle(s16 *, s16, void *);
extern s32 BattleFindGroundTriangle(s16 *, s16, s32);
extern void SoundRemoveSedsEntry(s32);
extern void BattleClearBlockSlots(void);
extern u8 D_800C3D0C[];
extern BattleActor *D_800D3368[];
extern void func_800A9FF0(s32);
extern void func_800AA934(void *, void *, void *, void *);
extern void func_800BE790(void);
extern void func_800B0D70(s16 *, u16 *);

typedef struct BattleModelNode BattleModelNode;

// Node of a 0x7C-stride hierarchy whose element 0 heads the list.
typedef struct {
    /* 0x0 */ u8 pad0[0x4];
    /* 0x4 */ BattleModelNode *pNodes;
} BattleModel;

struct BattleModelNode {
    /* 0x00 */ BattleModelNode *pParent;
    /* 0x04 */ u8 pad4[0x3];
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

extern BattleFrame D_800C3BD0;

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
        if ((D_800C3E30 >> i) & 1) {
            break;
        }
    }
    return i;
}
s32 BattleResolveTargetSlot(BattleActor *pActor, u32 id, u16 *pOut) {
    if ((id & 0xFF) == 0xFF) {
        id = BattleFindFirstActorInMask();
    } else if ((id & 0xFF) == 0xFE) {
        id = D_800C3D40;
    } else if ((id & 0xFF) == 0xFD || (id & 0xFF) == 0xF9) {
        id = pActor->unk20[0];
    } else if ((id & 0xFF) == 0xFC) {
        id = pActor->unk20[1];
    } else if ((id & 0xFF) == 0xFA) {
        id = 0x1F;
    } else if ((id & 0xFF) == 0xF8) {
        id = pActor->unk20[0] * 2 + 0xD;
    } else if ((id & 0xFF) == 0xF7) {
        id = pActor->unk20[0] * 2 + 0xE;
    }
    *pOut = 1 << (id & 0xFF);
    return id & 0xFF;
}
extern BattleTableEntry D_800CCD1E[];
extern u8 func_8009C050(u8);

s32 BattleGetActorActionEntry(BattleActor *pActor, u32 id, s32 *pOut) {
    u32 sel = id;
    u8 kind;

    *pOut = 0;
    if ((sel & 0xFF) >= 0xFE) {
        if ((sel & 0xFF) == 0xFF) {
            kind = func_8009C050(pActor->unk20[0]);
            if ((kind & 4) && (pActor->flags4A & 0x100)) {
                pActor->unk2A[0] = 0x1B;
            } else if ((kind & 2) && (pActor->flags4A & 0x80)) {
                if (!(D_800CCD1E[pActor->unk20[0]].flags & 1)) {
                    pActor->unk2A[0] = 6;
                }
            } else if (!(pActor->flags4A & 0x400)) {
                pActor->unk2A[0] = 1;
            }
            if (kind & 1) {
                pActor->unk2A[0] |= 0x80;
            }
        }
        sel = pActor->unk2A[0] & 0x7F;
        *pOut = pActor->unk2A[0] & 0x80;
    }
    if ((sel & 0xFF) < 0x40) {
        return ((s32 *)(u32)pActor->unk14)[(sel & 0xFF) + 1];
    }
    return ((s32 *)(u32)pActor->unk18)[(sel & 0xFF) - 0x3F];
}
// Queues a transform command on pNode (and, with flag 0x80, on every child) and
// applies the command's target transform to the node when it is issued directly
// (command < 2). flags & 7 selects which component the command drives: 0 rotation,
// 1 position, otherwise scale; flags & 0x20 / & 0x40 add the node's current
// component to the target / reference vector.
void func_800AF678(BattleModel *pModel, u8 *pPool, BattleModelNode *pNode, u8 flags, u8 command, u8 unk3,
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

    mode = flags & 7;
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
        pBlock->unk0 = 1;
        pBlock->unk1 = unk1;
        pBlock->unk2 = command + 3;
        pBlock->unk3 = unk3;
        if (flags & 0x20) {
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
        if (flags & 0x40) {
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
        pBlock->unk4 = x + addX;
        pBlock->unk6 = y + addY;
        pBlock->unk8 = z + addZ;
        if (command == 0) {
            pBlock->unkA = dx + dstX - pBlock->unk4;
            pBlock->unkC = dy + dstY - pBlock->unk6;
            pBlock->unkE = dz + dstZ - pBlock->unk8;
        } else {
            pBlock->unkA = dx + dstX;
            pBlock->unkC = dy + dstY;
            pBlock->unkE = dz + dstZ;
        }
        pBlock->unk10 = 0;
        pBlock->unk12 = duration;
        if (mode == 0) {
            if (command < 2) {
                pNode->rot[0] = pBlock->unk4;
                pNode->rot[1] = pBlock->unk6;
                pNode->rot[2] = pBlock->unk8;
            }
            pNode->p70 = pBlock;
        } else if (mode == 1) {
            if (command < 2) {
                pNode->pos[0] = pBlock->unk4;
                pNode->pos[1] = pBlock->unk6;
                pNode->pos[2] = pBlock->unk8;
            }
            pNode->p74 = pBlock;
        } else {
            if (command < 2) {
                pNode->scale[0] = pBlock->unk4;
                pNode->scale[1] = pBlock->unk6;
                pNode->scale[2] = pBlock->unk8;
            }
            pNode->p78 = pBlock;
        }
    }
    if (flags & 0x80) {
        pChild = pModel->pNodes;
        for (i = 1; i < pModel->pNodes->unkA; i++) {
            pChild++;
            if (pChild->pParent == pNode) {
                func_800AF678(pModel, pPool, pChild, flags & 0xFF, command, unk3, unk1, x, y, z, dx, dy, dz,
                              duration);
            }
        }
    }
}
void BattleSetModelNodeFlagRecursive(BattleModel *pModel, BattleModelNode *pNode, s32 flags) {
    BattleModelNode *pChild;
    s32 i;

    pNode->unk7 = flags & 1;
    if (flags & 0x80) {
        pChild = pModel->pNodes;
        for (i = 1; i < pModel->pNodes->unkA; i++) {
            pChild++;
            if (pChild->pParent == pNode) {
                BattleSetModelNodeFlagRecursive(pModel, pChild, flags);
            }
        }
    }
}
extern u8 *func_80023FD8(s32, s32, s32, s32);
extern void SpriteSetDirection(void *, s16);
extern void func_800223B0(void *, s16);
extern void SpriteSetScale(void *, s16);
extern s32 WorkListTaskGetTaskCallback(void *);
extern void TimerWorkListSetTaskCallback(void *, void *);
extern void func_800AFC68();

// Appended block of the per-part task func_80023FD8 allocates (same layout as
// lib_gear's LibGearJointTask). Its byte offset is the s16 at +0xBE of the task,
// which is the sprite's allocatedDataSize.
typedef struct {
    /* 0x0 */ u8 unk0[0x4];
    /* 0x4 */ void *pChainCallback;
    /* 0x8 */ s32 unk8;
    /* 0xC */ s16 partIndex;
    /* 0xE */ s16 unkE;
    /* 0x10 */ u16 offset[3];
} BattlePartNode; // Size: 0x18

// Per-part data BattleCreateModelPartTask copies into the node it creates.
typedef struct {
    /* 0x0 */ u8 unk0[0x5];
    /* 0x5 */ u8 partIndex;
    /* 0x6 */ u16 offsetX;
    /* 0x8 */ u16 offsetY;
    /* 0xA */ u16 offsetZ;
    /* 0xC */ u8 unkC;
    /* 0xD */ u8 padD[0x6];
    /* 0x13 */ u8 unk13; // non-zero: installs func_800AFC68 as the task callback
} BattlePartInfo; // Size: 0x14

void BattleCreateModelPartTask(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, BattlePartInfo *pInfo,
                               s32 arg6) {
    u8 *pTask;
    u8 *pSprite;
    BattlePartNode *pNode;

    pTask = func_80023FD8(arg1, arg0, arg2, 0x18);
    pSprite = pTask + 0x38;
    SpriteSetDirection(pSprite, arg3);
    func_800223B0(pSprite, arg3);
    SpriteSetScale(pSprite, arg4);
    pNode = (BattlePartNode *)(pTask + *(s16 *)(pTask + 0xBE));
    pNode->unk8 = arg6;
    pNode->partIndex = pInfo->partIndex;
    if (pInfo->unk13 != 0) {
        pNode->pChainCallback = (void *)WorkListTaskGetTaskCallback(pTask);
        TimerWorkListSetTaskCallback(pTask, func_800AFC68);
        pNode->offset[0] = pInfo->offsetX;
        pNode->offset[1] = pInfo->offsetY;
        pNode->offset[2] = pInfo->offsetZ;
        pNode->unkE = pInfo->unkC;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFC68);
/* 91.71%: structure, the GTE block (gte_ldv0/gte_mvmva/gte_stlvnl are an exact
   match) and the callback tail are right; the only difference is which temp
   register holds pMat: original `lw $v0,4($v0)` in $v0 + offset in $a1, ours
   the other way round. Tried: offset as a local before the call, pMat local.
extern void CompMatrix(void *, void *, void *);
extern void SetRotMatrix(void *);
extern void SetTransMatrix(void *);

void func_800AFC68(u8 *p) {
    u8 *pNode;
    u8 *pMatrix;
    s32 index;
    s32 vec[3];

    pNode = p + *(s16 *)(p + 0xBE);
    index = *(s16 *)(pNode + 0xC);
    pMatrix = PSX_SCRATCH;
    if (index != 0) {
        s32 offset = index * 0x7C;
        u8 *pMat = *(u8 **)(*(u8 **)(pNode + 0x8) + 4);
        CompMatrix(pMat + 0xC, pMat + offset + 0x2C, PSX_SCRATCH);
    } else {
        pMatrix = *(u8 **)(*(u8 **)(pNode + 0x8) + 4) + 0xC;
    }
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    gte_ldv0(pNode + 0x10);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(vec);
    if (*(s16 *)(pNode + 0xE) != 0) {
        vec[1] = *(s16 *)(*(u8 **)(pNode + 0x8) + 0x60);
    }
    *(s32 *)(p + 0x38) = vec[0] << 16;
    *(s32 *)(p + 0x3C) = vec[1] << 16;
    *(s32 *)(p + 0x40) = vec[2] << 16;
    ((void (*)(u8 *)) * (u32 *)(pNode + 0x4))(p);
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800AFD98);
/* 98.68% (516 bytes, identical instruction multiset): the whole body, the three
   mode blocks, the two sb writes and the child walk are exact; only the
   callee-saved assignment of the pModel and flags pseudos differs (original
   pModel=s6 / flags=s7, ours pModel=s7 / flags=s3; original keeps a second copy
   of flags in a2 for the three `flags & 0x20` tests, which needs the source to
   hold flags in two variables - not reproduced). Tried: s32 vs u8 vs unsigned
   flags, recursive call with and without `& 0xFF`, s16 vs s32 x/y/z with casts,
   mode/i/pChild declaration orders - all leave the ranking untouched.
void func_800AFD98(BattleModel *pModel, BattleModelNode *pNode, s32 flags, s16 x, s16 y, s16 z) {
    BattleModelNode *pChild;
    s32 mode;
    s32 i;

    mode = flags & 7;
    if (mode == 0) {
        if (flags & 0x20) {
            *(u16 *)((u8 *)pNode + 0x54) += x;
            *(u16 *)((u8 *)pNode + 0x56) += y;
            *(u16 *)((u8 *)pNode + 0x58) += z;
        } else {
            *(u16 *)((u8 *)pNode + 0x54) = x;
            *(u16 *)((u8 *)pNode + 0x56) = y;
            *(u16 *)((u8 *)pNode + 0x58) = z;
        }
    } else if (mode == 1) {
        if (flags & 0x20) {
            *(s32 *)((u8 *)pNode + 0x5C) += x;
            *(s32 *)((u8 *)pNode + 0x60) += y;
            *(s32 *)((u8 *)pNode + 0x64) += z;
        } else {
            *(s32 *)((u8 *)pNode + 0x5C) = x;
            *(s32 *)((u8 *)pNode + 0x60) = y;
            *(s32 *)((u8 *)pNode + 0x64) = z;
        }
    } else if (flags & 0x20) {
        *(u16 *)((u8 *)pNode + 0x4C) += x;
        *(u16 *)((u8 *)pNode + 0x4E) += y;
        *(u16 *)((u8 *)pNode + 0x50) += z;
    } else {
        *(u16 *)((u8 *)pNode + 0x4C) = x;
        *(u16 *)((u8 *)pNode + 0x4E) = y;
        *(u16 *)((u8 *)pNode + 0x50) = z;
    }
    *(u8 *)((u8 *)pNode + 4) = 1;
    *(u8 *)((u8 *)pNode + 5) = 1;
    if (flags & 0x80) {
        pChild = pModel->pNodes;
        for (i = 1; i < pModel->pNodes->unkA; i++) {
            pChild++;
            if (pChild->pParent == pNode) {
                func_800AFD98(pModel, pChild, flags & 0xFF, x, y, z);
            }
        }
    }
}
*/

void BattleSnapActorToGround(BattleActor *pActor) {
    u8 unk10[0x10];
    s16 vec[3];
    s16 index;

    vec[0] = ((u16 *)pActor->pSprites->pos)[0];
    vec[1] = ((u16 *)pActor->pSprites->pos)[2];
    vec[2] = ((u16 *)pActor->pSprites->pos)[4];
    index = BattleFindGroundTriangle(vec, pActor->unk1E, 4);
    pActor->unk1E = index;
    if (index < 0) {
        index = BattleFindTriangleContainingPoint(vec);
        pActor->unk1E = index;
    }
    BattleProjectPointOntoIndexedTriangle(vec, pActor->unk1E, unk10);
    ((u16 *)pActor->unk60)[0] = vec[1];
    if (pActor->unk36 == 0) {
        pActor->pSprites->pos[1] = vec[1];
    }
}

void BattleFreeActorBuffer(BattleActor *pActor) {
    if (pActor->unkC != 0) {
        if (pActor->unk60[3] != 0) {
            SoundRemoveSedsEntry(((s32 *)pActor->unkB4)[2]);
            pActor->unk60[3] = 0;
        }
        HeapFree((void *)pActor->unkC);
        pActor->unkC = 0;
        pActor->unk18 = 0;
    }
}

void BattleClearBlockSlots(void) {
    s32 i = 8;
    s32 *p = D_800C3BCC;

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}

void BattleReleaseBlockSlots(s32 arg0) {
    s32 i;

    for (i = 0; i < 9; i++) {
        if (D_800C3BAC[i] != NULL) {
            BattleFreeBlock(arg0, D_800C3BAC[i]);
        }
    }
    BattleClearBlockSlots();
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0164);
/* 99.70%: everything right except the scratch register gcc uses to spill the
   7th stack argument at entry (original `lhu $t0,104(sp)` / `sh $t0,16(sp)`,
   ours $a0); a0 is free here because arg0 is unused, so it is a global.c
   tie-break. Tried: s16 vs u16 params, &D_800C3BAC[index] vs index+base
   (needed a `pBase` local to get the original lui/addiu-then-sll order).
void func_800B0164(s32 arg0, s32 index, u8 arg2, u8 arg3, u16 arg4, u16 arg5, u16 arg6,
                   u16 arg7, u16 arg8, u16 arg9, u16 arg10) {
    u8 **pBase;
    u8 **pp;
    u8 *p;

    if (D_800C37C8 == 0) {
        pBase = D_800C3BAC;
        pp = &pBase[index];
        if (*pp == NULL) {
            *pp = (u8 *)BattleAllocBlock();
        }
        p = *pp;
        if (p != NULL) {
            p[0] = 1;
            p[1] = 0;
            p[2] = arg2;
            p[3] = arg3;
            *(s16 *)(p + 0x4) = arg4;
            *(s16 *)(p + 0x6) = arg5;
            *(s16 *)(p + 0x8) = arg6;
            *(s16 *)(p + 0xA) = arg7;
            *(s16 *)(p + 0xC) = arg8;
            *(s16 *)(p + 0xE) = arg9;
            *(s16 *)(p + 0x10) = 0;
            *(s16 *)(p + 0x12) = arg10;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B026C);

s32 BattleIsPointWithinBounds(s16 *pPos) {
    BattleStreamData *pData = D_800658C8;

    if (pPos[0] > pData->unk34E && pPos[0] < pData->unk34C && pPos[2] > pData->unk350 &&
        pPos[2] < pData->unk352) {
        return 1;
    }
    return 0;
}
extern u16 D_800D3354;
extern s16 D_800D3356;
extern u16 D_800D3358;
extern u16 D_800D335C;
extern s16 D_800D335E;
extern u16 D_800D3360;
extern s16 D_800C3542;
extern u16 D_800C3544;
extern s16 D_800C3546;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0B14);
/* 74.97% (588 bytes vs 584): the control flow - the two-condition test, the
   BattleFindGroundTriangle/BattleFindTriangleContainingPoint retry, the D_800C3546/D_800C3542/D_800C3544
   bookkeeping - is in place; what is left is the head: retail loads
   D_800D335C and D_800D335E first, then reads D_800D3354/3356/3358 through one
   base pointer s5 (= &D_800D3354) both for the deltas and for the vec sums, and
   it ranks dx in s1 / dy in s2 where ours swaps them. Tried: dy-first vs dx-first,
   s16 vec[3] vs vec[4], direct globals vs one base pointer local.
s16 func_800B0B14(s32 index) {
    u8 work[0x10];
    s16 vec[4];
    s32 dx;
    s32 dy;
    s32 dz;
    s32 len;
    s32 result;

    dy = (D_800D335E - D_800D3356) / 4;
    dx = (s16)(D_800D335C - D_800D3354);
    dz = (s16)(D_800D3360 - D_800D3358);
    len = SquareRoot0(dx * dx + dy * dy + dz * dz) + 1;
    vec[0] = D_800D3354 + (dx << 9) / len;
    vec[1] = D_800D3356 + (dy << 9) / len;
    vec[2] = D_800D3358 + (dz << 9) / len;
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
    if (result < 0) {
        vec[1] = D_800C3544;
    } else {
        BattleProjectPointOntoIndexedTriangle(vec, D_800C3542, work);
        D_800C3544 = vec[1];
    }
    return vec[1];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B0D70);
void BattleSteerActorsTowardPoint(u16 *pA, u16 *pB) {
    s16 vec[3];
    s32 len;

    vec[0] = pA[0] - pB[0];
    vec[2] = pA[2] - pB[2];
    len = SquareRoot0(vec[0] * vec[0] + vec[2] * vec[2]) + 1;
    vec[0] = (vec[0] << 9) / len;
    vec[2] = (vec[2] << 9) / len;
    func_800B0D70(vec, pB);
}
void BattlePushActorOutOfCircle(s32 index, s32 x, s32 z, s32 dist) {
    BattleActor **pBase = D_800D3368;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B12D0);
/* 99.74% (156 bytes, identical instruction stream): only the switch's jump
   table operand differs (the original names jtbl_800707C4). It cannot be
   converted as-is: that table lives in the splat-owned .rodata
   (asm/battle/data/0.rodata.s) and its entries point at this stub's local
   labels (.L800B132C/1334/133C/1344), so `make build` fails to link with
   "undefined reference to .L800B132C" the moment the stub becomes C.
   The mapping is kind -> flag mask:
   0,1 -> 1; 2,3 -> 8; 4 -> 4; 5 -> 2; else -> id & 0xFF.
s32 func_800B12D0(s32 arg0, s32 arg1) {
    s32 result = 0;
    s32 v;
    u8 *pRow = &D_800C4000[(D_800C360C - 1) * 0x48];
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
*/
extern s32 BattleGetAdjustedWaitCountdown(void);
extern s32 ArchiveDataSync(void);
extern void func_8002A498(s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc25", func_800B136C);
/* 96.96% (332 bytes vs 328): the whole control flow, the two 11-entry polls and
   the ArchiveDataSync wait loop match; only two register-allocation ties remain.
   1. prologue save order: original `sw ra,32 / sw s1,20 / sw s0,16 / move s1,zero
      / move s0,zero`, ours emits `sw s1,20 / move s1,zero` before `sw ra,32`.
   2. first poll loop: original keeps the array walker in $a0 and the element in
      $v1, ours the other way round.
   Tried: D_800D3368[i] and *(pp++) walkers, one base pointer local (pBase) vs
   two (pBase + ppBase), comma for-init, flagA declared before/after the base.
   Shape notes for whoever retries: phase 1 is
     for (;;) { poll 11 entries; if (BattleGetAdjustedWaitCountdown()) flagA = 1;
                if (flagA) { func_800BE790(); flagA = 0; continue; } break; }
   phase 2 needs BOTH a base local (loaded once into $s2 before the while) and a
   walker reset from it each pass, and a `if (flagA == 0) break;` after the
   sync test so gcc shares the loop test with the break.
void func_800B136C(void) {
    s32 flagB = 0;
    u8 **pBase = D_800D3368;
    s32 flagA = 0;
    u8 *pEntry;
    s32 i;
    u8 **pp;
    u8 **ppBase;

    for (;;) {
        for (i = 0; i < 11; i++) {
            pEntry = pBase[i];
            if (pEntry != NULL) {
                if (pEntry[0x38] != 0) {
                    flagA = 1;
                }
                if (*(s32 *)(pEntry + 0xC) != 0) {
                    flagB = 1;
                }
            }
        }
        if (BattleGetAdjustedWaitCountdown() != 0) {
            flagA = 1;
        }
        if (flagA != 0) {
            func_800BE790();
            flagA = 0;
            continue;
        }
        break;
    }
    if (flagB == 0) {
        return;
    }
    func_8002A498(0);
    flagA = 1;
    ppBase = D_800D3368;
    while (flagA != 0) {
        if (ArchiveDataSync() == 0) {
            for (i = 0, pp = ppBase; i < 11; i++) {
                pEntry = *pp++;
                if (pEntry != NULL && *(s32 *)(pEntry + 0xC) != 0) {
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
*/
void BattleSetExtraMatrixFlag(void) {
    D_800C3D6C = 1;
}
void BattleWaitForOtherActorAnimations(s32 arg0) {
    s32 i;
    s32 flag;
    BattleActor *pActor;

    for (i = 0; i < 3; i++) {
        if (i != arg0) {
            func_800AA934(D_800D3368[i], D_800D3368[i], D_800C3D0C, 5);
        }
    }
    do {
        flag = 0;
        for (i = 0; i < 3; i++) {
            if (i != arg0) {
                pActor = D_800D3368[i];
                if (pActor != NULL && (pActor->unk34 != 0 || pActor->unk38 != 0)) {
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
/* 71.84% (192 bytes, same size as the original): the 7-word copy and the frame
   arithmetic are right. Remaining differences: the original keeps the copy
   destination in $a2 and the lui high half in $a3 (so the later global accesses
   are `lw v0,%lo(D_800C3BD0)($a3)`), and blocks the copy 3+3+1 instead of 4+3.
   Tried: pDst pointer local, direct global access on every reference,
   `s32 offset` local folding the +0xC into the register (that one matches the
   original's `addiu v0,v0,0xC` but the rest is unchanged).
s32 func_800B15D8(u8 *p, s32 index) {
    BattleFrame *pFrame = (BattleFrame *)(p + index * 0x1C + 0xC);
    BattleFrame *pDst = &D_800C3BD0;

    *pDst = *pFrame;
    if ((*(u32 *)(p + 4) & 1) == 0) {
        pDst->unk0 += (mem_addr)pFrame;
        pDst->unk8 += (mem_addr)pFrame;
        pDst->unk10 += (mem_addr)pFrame;
    }
    D_800C3BF0 = 0;
    D_800C3BEC = pDst->unk10;
    return pFrame->unk14;
}
*/
