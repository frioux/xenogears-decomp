#include "common.h"
#include "field/actor.h"
#include "battle/actor.h"
#include "psyq/libgpu.h"

// 16.16 fixed point; the integer part is the high half of the word.
typedef union {
    /* 0x0 */ s32 value;
    struct {
        /* 0x0 */ s16 frac;
        /* 0x2 */ s16 whole;
    } parts;
} BattleFixed16; // Size: 0x4

typedef struct {
    /* 0x0 */ u8 unk0[0x4];
    /* 0x4 */ u8 r;
    /* 0x5 */ u8 g;
    /* 0x6 */ u8 b;
    /* 0x7 */ u8 unk7[0x19];
} BattleEffectColor; // Size: 0x20

typedef struct {
    /* 0x00 */ u16 unk00;
    /* 0x02 */ u16 unk02;
    /* 0x04 */ u16 unk04;
    /* 0x06 */ u16 unk06;
    /* 0x08 */ u16 unk08;
    /* 0x0A */ u16 unk0A;
    /* 0x0C */ u16 unk0C;
    /* 0x0E */ u16 unk0E;
    /* 0x10 */ s32 posX;
    /* 0x14 */ s32 posY;
    /* 0x18 */ s32 posZ;
    /* 0x1C */ u8 unk1C[0x8];
    /* 0x24 */ BattleEffectColor colors[2];
    /* 0x64 */ s32 timer;
    /* 0x68 */ BattleFixed16 velX;
    /* 0x6C */ BattleFixed16 velY;
    /* 0x70 */ BattleFixed16 velZ;
    /* 0x74 */ u8 unk74[0x4];
    /* 0x78 */ s32 grav;
} BattleEffect; // Size: 0x7C

typedef struct {
    /* 0x00000 */ u8 unk00000[0x38];
    /* 0x00038 */ s32 unk38;
    /* 0x0003C */ BattleEffect entries[2][14][20];
} BattleEffectTable; // Size: 0x10F7C

typedef struct {
    /* 0x0 */ u8 unk0[0x4];
    /* 0x4 */ BattleEffectTable *pTable;
} BattleEffectTask; // Size: 0x8

// What SpriteData.field_0x6C points at: first the sprite's timer entry, then at
// 0x1C the entry the per-frame draw pass hangs its callback on.
typedef struct {
    /* 0x00 */ u8 timerEntry[0x1C];
    /* 0x1C */ u8 drawEntry[0x1C];
} BattleSpriteTask; // Size: 0x38

// A view of g_BattleStateWork that reaches the per-slot sprite pointer table at
// 0x8C8C; each entry is the slot's sprite, the one SpritePlayAnimation runs on.
typedef struct {
    /* 0x0000 */ u8 unk0000[0x8C8C];
    /* 0x8C8C */ SpriteData *sprites[BATTLE_SLOT_COUNT];
} BattleSpriteSlotTable;

extern void WorkListSetTaskCallback(void *pEntry, void *pCallback);
extern void WorkListRemoveTask(void *pEntry);
extern void TimerWorkListRemoveTask(void *pEntry);
extern void HeapFree(void *pMem);
extern void GfxQueueSpriteTileData(void *pArg);
extern void SpritePlayAnimation(SpriteData *pSpriteData, s32 animId);
extern void func_800B7160(void);
extern u_long *g_GfxCurOT;
extern u_long *g_BattleSavedGfxOT;
extern void SpriteDrawModelPartsInBaseMatrix(void *pTask);
extern u8 g_BattleStateWork[];
extern long ratan2(long y, long x);
extern void func_8002DDE4(void *pImg, s32 arg1, s32 arg2, s32 arg3);
extern s32 WorkListsAddTasks(s32 arg0, s32 arg1, void *pFn1, void *pFn2, void *pFn3);
extern void func_800B7424(s32 arg0);
extern void BattleCreateWorkTask(void);
extern void func_800B6F0C(void);
extern void BattleStoreGfxOrderingTable(void);
extern void BattleDestroyWorkTask(u8 *p);
extern s32 SpriteComputeAngleFromPoints(s32 arg0, s32 arg1);
extern s32 SpriteScaleValue(void *pSprite, s32 arg1);
extern void func_801FC4C4(void *pArg0, void *pArg1, void *pBase, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7);
extern void SpriteSetDirection(u8 *pActor, s16 arg1);
extern void func_800223B0(u8 *pActor, s16 arg1);
extern void SpriteSetSVec(SVECTOR *pVector, s16 x, s16 y, s16 z);
extern void func_801FC53C(void *pArg, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6);
extern u8 SpriteColorAddClamp(u8 arg0, s16 arg1);
void BattleScriptLoadDataAtWordOffset(s32 arg0, u8 *p) {
    s32 pad[3];
    u8 *pData;

    pData = p + ((((s8 *)p)[1] << 8) | p[0]);
    pad[0] = 0;
    pad[1] = 0;
    pad[2] = 0;
    func_8002DDE4(pData, 0, 0, 0);
}
void BattleSetTaskCallback(SpriteData *pSprite) {
    BattleSpriteTask *pTask;

    pTask = pSprite->field_0x6C;
    WorkListSetTaskCallback(&pTask->drawEntry, SpriteDrawModelPartsInBaseMatrix);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6464);
/* best 59.79% (s32 b0/b1 locals below) / 58.00% (inline p[0], p[1]).
   retail clusters all three loads at the block top (lhu v0,0(a3); lbu a0,0(t1);
   lbu a1,1(t1)) and only then does the andi/srl/addu work; gcc sinks the second
   lbu below the srl.  Retail also chains both pointer loads in the same register
   (lw v1,0x20(a0); lw v1,0x30(v1)) because the count lives in v0, while gcc
   allocates the count to v1 and the chain to v0.  Tried: u16 v local, s32 byte
   locals, i less-than n (gives slt instead of the rotated bne), signed and
   unsigned packet pointer types, pointer chain before the count.
void func_800B6464(u8 *pActor, u8 *p) {
    u8 *pEntry;
    s16 n;
    s16 i;

    n = *(u8 *)(pActor + 0x40) >> 2;
    pEntry = (u8 *)(*(u32 *)(*(u32 *)(pActor + 0x20) + 0x30)) + 0xC;
    for (i = 0; i != n; i++) {
        u16 v = *(u16 *)pEntry;
        s32 b0 = p[0];
        s32 b1 = p[1];

        *(u16 *)pEntry = ((v & 0x3F) + b0) | ((((v >> 6) & 0x1FF) + b1) << 6);
        pEntry += 0x18;
    }
}
*/
void BattleScriptPlaySpriteAnimation(s32 arg0, u8 *p) {
    BattleSpriteSlotTable *pState;

    pState = (BattleSpriteSlotTable *)g_BattleStateWork;
    SpritePlayAnimation(pState->sprites[p[1]], p[0]);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6518);
/* best 91.71%: the array form gets the lhu read-back and the two-word
   recombination right; what is left is the two actor loads coming out as
   'lhu' instead of retail's 'lh' (gcc merges the s16 load with the halfword
   store into one HImode load, and an HImode load on mips is lhu) plus the
   8-byte-larger retail frame (an unused 8-byte local).  Tried: SVECTOR with
   *(s32*)&field recombination (81.08, gives lw), u16 fields (same lhu),
   ((u16*)a)[i] casts on the read-back, explicit (vy<<16)|vx with s16 fields
   (69.61).
void func_800B6518(u8 *pActor) {
    s16 a[4];
    s16 b[4];
    s16 angle;

    a[0] = *(s16 *)(pActor + 0x2);
    a[1] = *(s16 *)(pActor + 0xA);
    b[0] = *(u16 *)(pActor + 0xA0);
    b[1] = *(u16 *)(pActor + 0xA4);
    angle = SpriteComputeAngleFromPoints(((u16 *)b)[1] << 16 | ((u16 *)b)[0],
                          ((u16 *)a)[1] << 16 | ((u16 *)a)[0]);
    SpriteSetDirection(pActor, angle);
    func_800223B0(pActor, angle);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B65B0);
void BattleActorSetDirectionTowardPosition(BattleSpriteState *pSprite) {
    SVECTOR rot;
    VECTOR diff;
    VECTOR sq;
    SVECTOR vec;
    MATRIX mtx;
    VECTOR out;
    s32 len;
    s16 zero;

    zero = 0;
    diff.vx = pSprite->targetPosition.x - pSprite->position.xWhole;
    diff.vy = pSprite->targetPosition.y - pSprite->position.yWhole;
    diff.vz = pSprite->targetPosition.z - pSprite->position.zWhole;
    Square0(&diff, &sq);
    len = SquareRoot0(sq.vx + sq.vz);
    rot.vy = -ratan2(diff.vz, diff.vx);
    rot.vz = ratan2(diff.vy, len);
    rot.vx = 0;
    pSprite->direction = rot.vy;
    SpriteSetSVec(&vec, (pSprite->moveSpeed << 9) >> 16, zero, zero);
    RotMatrix(&rot, &mtx);
    ApplyMatrix(&mtx, &vec, &out);
    pSprite->step.x = out.vx << 7;
    pSprite->step.y = out.vy << 7;
    pSprite->step.z = out.vz << 7;
}
void BattleScriptLoadFixed3(u32 *pOut, s8 *p) {
    p += (p[1] << 8) | (u8)p[0];
    pOut[0] = ((p[1] << 8) | (u8)p[0]) << 16;
    pOut[1] = ((p[3] << 8) | (u8)p[2]) << 16;
    pOut[2] = ((p[5] << 8) | (u8)p[4]) << 16;
}
void BattleReadScriptHalfwords3(u16 *pOut, s8 *p) {
    s32 v0;
    s32 v1;
    s32 v2;

    p += (p[1] << 8) | (u8)p[0];
    v0 = (p[1] * 256) | (u8)p[0];
    pOut[0] = v0;
    v1 = (p[3] * 256) | (u8)p[2];
    pOut[1] = v1;
    v2 = (p[5] * 256) | (u8)p[4];
    pOut[2] = v2;
}
void BattleScriptAddHalfwords3(u16 *pOut, s8 *p) {
    s32 v;

    p += (p[1] << 8) | (u8)p[0];
    v = (p[1] << 8) | (u8)p[0];
    pOut[0] += v;
    v = (p[3] << 8) | (u8)p[2];
    pOut[1] += v;
    v = (p[5] << 8) | (u8)p[4];
    pOut[2] += v;
}
void BattleActorCopyVectorFromSource(SpriteData *pSprite) {
    SpriteData *pSrc;

    pSrc = pSprite->field_0x70;
    pSprite->step.x = pSrc->step.x;
    pSprite->step.y = pSrc->step.y;
    pSprite->step.z = pSrc->step.z;
}
void BattleScriptCall8001FC4C4WithActorState(SpriteData *pSprite, s8 *pArg) {
    s32 x2;
    s8 *p;
    s32 x1;
    s32 x3;

    p = pArg + ((pArg[1] << 8) | (u8)pArg[0]);
    if (pSprite->field_0x3A != 0) {
        x1 = SpriteScaleValue(pSprite, (p[1] << 3) | (u8)p[0]) << 8;
        x2 = SpriteScaleValue(pSprite, (u8)p[2]) << 16;
        x3 = SpriteScaleValue(pSprite, (u8)p[3]) << 16;
    } else {
        x1 = ((p[1] << 3) | (u8)p[0]) << 5;
        x2 = (u8)p[2] << 13;
        x3 = (u8)p[3] << 13;
    }
    func_801FC4C4(pSprite->pBase->pDirTransforms, pSprite->pBase->pFramesData,
                  &pSprite->pBase->transformMatrix, x1, x2, x3, (u8)p[4] << 4, (u8)p[5] << 2);
    pSprite->pBase->pDirTransforms = NULL;
    pSprite->pBase->pFramesData = NULL;
    pSprite->pBase->pCurRenderFramesData = NULL;
}
void BattleScriptCall8001FC53CWithScaledBytes(u8 *pArg, s8 *p) {
    p += (p[1] << 8) | (u8)p[0];
    func_801FC53C(pArg, (u8)p[0] << 4, (u8)p[1], p[2] << 3, (u8)p[3] << 3, p[4] << 3, (u8)p[5]);
}
void BattleMoveScreenImageToVram(void) {
    RECT rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = 0x140;
    rect.h = 0xE0;
    MoveImage(&rect, 0x2C0, 0x100);
    BattleCreateWorkTask();
}
void BattleSetRotationZFromVelocity(BattleSpriteState *pSprite) {
    pSprite->pBase->rotation.z = ratan2(pSprite->step.y >> 8, pSprite->step.x >> 8);
    pSprite->flags |= 0x10000000;
}
void BattleSetRotationXFromVelocity(BattleSpriteState *pSprite) {
    pSprite->pBase->rotation.x = ratan2(pSprite->step.z >> 8, pSprite->step.x >> 8);
    pSprite->flags |= 0x10000000;
}
void BattleSetRotationFromVelocity(BattleSpriteState *pSprite) {
    VECTOR vec;
    VECTOR sq;
    s32 len;

    vec.vx = pSprite->step.x >> 8;
    vec.vy = pSprite->step.y >> 8;
    vec.vz = pSprite->step.z >> 8;
    if (vec.vz == 0) {
        vec.vz = 4;
    }
    Square0(&vec, &sq);
    len = SquareRoot0(sq.vx + sq.vz);
    pSprite->pBase->rotation.y = -ratan2(vec.vz, vec.vx);
    pSprite->pBase->rotation.z = ratan2(vec.vy, len);
    pSprite->pBase->rotation.x = 0;
    pSprite->flags |= 0x10000000;
}
void BattleClearRecordTable8(SpriteData *pSprite) {
    s32 i;

    if (pSprite->pBase == NULL) {
        return;
    }
    if (pSprite->pBase->pDirTransforms == NULL) {
        return;
    }
    for (i = 0; i != 8; i++) {
        (i + pSprite->pBase->pDirTransforms->dirTransforms)->translationX = 0;
        (i + pSprite->pBase->pDirTransforms->dirTransforms)->translationY = 0;
        (i + pSprite->pBase->pDirTransforms->dirTransforms)->rotationX = 0;
        (i + pSprite->pBase->pDirTransforms->dirTransforms)->rotationY = 0;
        (i + pSprite->pBase->pDirTransforms->dirTransforms)->rotationZ = 0;
    }
    pSprite->pBase->offsetX = 0;
    pSprite->pBase->offsetY = 0;
}
void BattleActorRotateVectorZ(BattleSpriteState *pSprite, s8 *p) {
    SVECTOR rotation;
    MATRIX mtx;
    VECTOR vec;
    s16 zero;
    s32 z;

    zero = 0;
    z = p[0] << 4;
    SpriteSetSVec(&rotation, zero, zero, z);
    RotMatrix(&rotation, &mtx);
    ApplyMatrixLV(&mtx, (VECTOR *)&pSprite->step, &vec);
    pSprite->step.x = vec.vx;
    pSprite->step.y = vec.vy;
    pSprite->step.z = vec.vz;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B6F0C);
/* best 85.91% (540 vs 536 bytes).  Recovered shape: the work item's data pointer
   (arg + 4) heads a 0x10F7C-byte table - s32 counter at 0x38, then
   entries[2][14][20] of 0x7C-byte records from 0x3C (0x3C + 2*0x87A0 = 0x10F7C,
   the size BattleCreateWorkTask passes to WorkListsAddTasks).  Record: u16
   accumulators at 0x0/0x2/0x4 fed by 0x8/0xA/0xC, s32 pos XYZ at 0x10/0x14/0x18,
   0x24 is an array of 0x20-byte color records (r/g/b at +4/+5/+6, faded by
   SpriteColorAddClamp(x,-6)), s32 timer at 0x64, three 16.16 fixed velocities at
   0x68/0x6C/0x70 (integer part read as the high half at 0x6A/0x6E/0x72), s32
   gravity at 0x78 (added to 0x6C).  Damping order is 0x6C, 0x68, 0x70 - not
   0x68 first - and `for (x = 0; x != N; x++)` with != is required, `<` gives
   slti/bnez instead of retail's li+bne.
   Three things left, see the parked notes on BattleScriptPlaySpriteAnimation and misc30:
   (1) the g_BattleStateWork+0x8000+0xC84 read: retail materialises the symbol with
   lui+addiu (addend 0), then li 0x8000 + addu, then lw 0xC84(base); gcc either
   folds the whole 0x8C84 into one lui/addiu carry pair or, with pState a pointer
   local, into lui 0x1/addiu -32768.  It also leaves a dead `lui v0,%hi
   (g_BattleStateWork)` in retail's entry block, which nothing in our compile produces.
   (2) the three fixed-point reads: retail sign-extends at the load (lh 0x6A),
   gcc emits lhu plus an explicit sll/sra pair; a plain s16[3] local gives lhu,
   three separate s16 locals drop the frame to 0x40 (retail is 0x48 with the
   three spills at sp+0x10/0x12/0x14), and casting the stored value to s32 keeps
   the frame but not the lh.
   (3) retail's prologue saves fp first and runs `move fp,zero` between two
   stack stores; ours saves ra first and moves fp later.
   Tried: pointer local for the table base, colors[2] vs pointer arithmetic,
   velocity as a 16.16 union vs raw s16 casts vs three s16 locals, source-order
   permutations of the six position/accumulator updates.
void func_800B6F0C(BattleEffectTask *pTask) {
    BattleEffectTable *pTable;
    u8 *pState;
    BattleEffect *pEntry;
    BattleEffectColor *pColor;
    s16 vel[3];
    s32 i;
    s32 j;
    s32 k;

    pTable = pTask->pTable;
    pState = g_BattleStateWork + 0x8000;
    pTable->unk38++;
    for (i = 0; i != 2; i++) {
        for (j = 0; j != 14; j++) {
            for (k = 0; k != 20; k++) {
                pEntry = &pTable->entries[i][j][k];
                if (pEntry->timer != 0) {
                    pEntry->timer--;
                    continue;
                }
                pColor = &pEntry->colors[*(u32 *)(pState + 0xC84)];
                pColor->r = SpriteColorAddClamp(pColor->r, -6);
                pColor->g = SpriteColorAddClamp(pColor->g, -6);
                pColor->b = SpriteColorAddClamp(pColor->b, -6);
                vel[0] = (s32)pEntry->velX.parts.whole;
                vel[1] = (s32)pEntry->velY.parts.whole;
                vel[2] = (s32)pEntry->velZ.parts.whole;
                pEntry->posX += vel[0];
                pEntry->posY += vel[1];
                pEntry->unk00 += pEntry->unk08;
                pEntry->unk02 += pEntry->unk0A;
                pEntry->unk04 += pEntry->unk0C;
                pEntry->posZ += vel[2];
                pEntry->velY.value -= pEntry->velY.value / 16;
                pEntry->velX.value -= pEntry->velX.value / 16;
                pEntry->velZ.value -= pEntry->velZ.value / 16;
                pEntry->velY.value += pEntry->grav;
            }
        }
    }
}
*/
void BattleStoreGfxOrderingTable(void) {
    g_BattleSavedGfxOT = g_GfxCurOT;
    func_800B7160();
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B7160);
void BattleDrawSyncAndFree(void *p) {
    DrawSync(0);
    HeapFree(p);
}
void BattleDestroyWorkTask(u8 *p) {
    WorkListRemoveTask(p + 0x1C);
    TimerWorkListRemoveTask(p);
    GfxQueueSpriteTileData(p);
}
void BattleCreateWorkTask(void) {
    func_800B7424(WorkListsAddTasks(0x10F7C, 0, func_800B6F0C, BattleStoreGfxOrderingTable, BattleDestroyWorkTask));
}
