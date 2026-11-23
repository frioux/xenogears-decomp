#include "common.h"
#include "field/actor.h"
#include "battle/actor.h"
#include "psyq/libgpu.h"

// An effect record is only drawn once its projected depth reaches this.
#define BATTLE_EFFECT_MIN_DRAW_DEPTH 64

// How much each colour channel drops per frame while a record fades out.
#define BATTLE_EFFECT_FADE_STEP 6

// 16.16 fixed point; the integer part is the high half of the word.
typedef union {
    /* 0x0 */ s32 value;
    struct {
        /* 0x0 */ s16 frac;
        /* 0x2 */ s16 whole;
    } parts;
} BattleFixed16; // Size: 0x4

typedef struct {
    /* 0x0 */ u8 tag[0x4]; // AddPrim links the prim here
    /* 0x4 */ u8 r;
    /* 0x5 */ u8 g;
    /* 0x6 */ u8 b;
    /* 0x7 */ u8 pad7;
    /* 0x8 */ long sxy[3][2]; // RotTransPers3 writes one projected screen pair per vertex
} BattleEffectColor; // Size: 0x20

typedef struct {
    /* 0x00 */ u16 rotX; // the SVECTOR RotMatrix reads at 0x0 of the record in the draw pass
    /* 0x02 */ u16 rotY;
    /* 0x04 */ u16 rotZ;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u16 rotStepX; // added to rotX every update
    /* 0x0A */ u16 rotStepY;
    /* 0x0C */ u16 rotStepZ;
    /* 0x0E */ u16 padE;
    /* 0x10 */ s32 posX;
    /* 0x14 */ s32 posY;
    /* 0x18 */ s32 posZ;
    /* 0x1C */ u8 pad1C[0x4]; // the unused 4th word of the VECTOR TransMatrix reads at 0x10
    /* 0x20 */ u8 unk20[0x4];
    /* 0x24 */ BattleEffectColor colors[2];
    /* 0x64 */ s32 timer;
    /* 0x68 */ BattleFixed16 velX;
    /* 0x6C */ BattleFixed16 velY;
    /* 0x70 */ BattleFixed16 velZ;
    /* 0x74 */ u8 pad74[0x4]; // the unused 4th word of the VECTOR the velocities form
    /* 0x78 */ s32 grav;
} BattleEffect; // Size: 0x7C

// One row of a record set: 20 records per row, 14 rows per set.
typedef BattleEffect BattleEffectRow[20];

typedef struct {
    /* 0x00000 */ u8 unk00000[0x38];
    /* 0x00038 */ s32 frameCount; // zeroed by the spawner, incremented once per update pass
    /* 0x0003C */ BattleEffect entries[2][14][20];
} BattleEffectTable; // Size: 0x10F7C

typedef struct {
    /* 0x0 */ u8 unk0[0x4];
    /* 0x4 */ BattleEffectTable *pTable;
} BattleEffectTask; // Size: 0x8

// The battle state block 0x8000 bytes into g_BattleStateWork, as misc29 types it:
// only the field this unit reads is carried here.
typedef struct {
    /* 0x000 */ u8 unk0[0xC84];
    /* 0xC84 */ s32 colorSetIndex; // selects the effect colour set
} BattleRenderState;

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
extern SVECTOR D_800C3594[3];
extern SVECTOR D_800C35AC[3];
extern void ReadGeomOffset(long *ofx, long *ofy);
extern long ReadGeomScreen(void);
extern void SetGeomOffset(long ofx, long ofy);
extern void SetGeomScreen(long h);
extern void SetRotMatrix(MATRIX *m);
extern void SetTransMatrix(MATRIX *m);
extern long RotTransPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, long *sxy0, long *sxy1, long *sxy2, long *p, long *flag);
extern void AddPrim(void *ot, void *p);
extern void BattleDrawEffectTable(BattleEffectTask *pTask);
extern u_long *g_GfxCurOT;
extern u_long *g_BattleSavedGfxOT;
extern void SpriteDrawModelPartsInBaseMatrix(void *pTask);
extern u8 g_BattleStateWork[];
extern long ratan2(long y, long x);
extern void func_8002DDE4(void *pImg, s32 arg1, s32 arg2, s32 arg3);
extern s32 WorkListsAddTasks(s32 arg0, s32 arg1, void *pFn1, void *pFn2, void *pFn3);
extern void func_800B7424(s32 arg0);
extern void BattleCreateWorkTask(void);
extern void BattleUpdateEffectTable(BattleEffectTask *pTask);
extern void BattleStoreGfxOrderingTable(BattleEffectTask *pTask);
extern void BattleDestroyWorkTask(u8 *p);
extern s16 SpriteComputeAngleFromPoints(DVECTOR from, DVECTOR to);
extern s32 SpriteScaleValue(void *pSprite, s32 arg1);
extern void func_801FC4C4(void *pArg0, void *pArg1, void *pBase, s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7);
extern void SpriteSetDirection(BattleSpriteState *pActor, s16 direction);
extern void func_800223B0(BattleSpriteState *pActor, s16 direction);
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
void BattleScriptOffsetFrameCluts(BattleSpriteState *pActor, u8 *p) {
    SpriteFrameData *pFrame;
    u8 n;
    s16 i;
    u16 v;
    s32 b0;
    s32 b1;
    s32 lo;
    s32 hi;

    n = pActor->unk40[0] >> 2;
    pFrame = pActor->pBase->pCurRenderFramesData;
    for (i = 0; i != n; i++) {
        v = pFrame->clutID;
        b0 = p[0];
        b1 = p[1];
        lo = v & 0x3F;
        hi = (v >> 6) & 0x1FF;
        lo += b0;
        hi += b1;
        pFrame->clutID = lo | (hi << 6);
        pFrame++;
    }
}
void BattleScriptPlaySpriteAnimation(s32 arg0, u8 *p) {
    BattleSpriteSlotTable *pState;

    pState = (BattleSpriteSlotTable *)g_BattleStateWork;
    SpritePlayAnimation(pState->sprites[p[1]], p[0]);
}
// The actor's own position goes through s32 locals so the loads stay signed
// (retail reads targetPosition with lhu and its own position with lh).
void BattleActorFaceTarget(BattleSpriteState *pActor) {
    DVECTOR vec0;
    DVECTOR vec1;
    s32 c0;
    s32 c1;
    s16 angle;

    c0 = pActor->position.xWhole;
    vec0.vx = c0;
    c1 = pActor->position.zWhole;
    vec0.vy = c1;
    vec1.vx = pActor->targetPosition.x;
    vec1.vy = pActor->targetPosition.z;
    angle = SpriteComputeAngleFromPoints(vec1, vec0);
    SpriteSetDirection(pActor, angle);
    func_800223B0(pActor, angle);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc28", func_800B65B0);
/* 98.03% at the original's exact 600 bytes / 150 instructions (re-measured).
   Two residuals, both register-choice ties, neither a size difference:
   1) the magnitude: retail is bgez $v1 / move $v0,$v1 / negu $v0,$v0, ours is
      move $v0,$v1 / negu $v0,$v1, i.e. it negates the source instead of the
      copy. Lever a-ternary-abs-over-a (world_map/misc12 func_80090FB4) parks at
      the same 98.03 on this same shape and could not close it either.
   2) the z block: retail truncates the angle difference into its own $v0 and
      copies it twice (move $a3,$v0 for the turn, move $v1,$v0 for the
      magnitude); ours folds the sra into $a3, so the three quantities collapse
      to two.
   Measured this round, y block left as it is and z varied:
   - z as three plain statements: 97.47, the magnitude took $a0 and the delta
     still folded into $a3.
   - z with the magnitude first and turn = delta after it: 95.85, the delta
     moved to $a0 in BOTH blocks (one pseudo, two defs) and a load nop appeared.
   - z with its own deltaZ/magZ locals: 92.87 / 596 bytes. That DOES give the z
     block the y block's shape (delta in $v1, magnitude coalesced with it, the
     turn copy in the beqz slot) and is one instruction short of retail's third
     quantity. So a shared delta conflicts the two blocks while a split one
     makes gcc drop the magnitude copy; both keep residual 1.
   Earlier: plain form in either block (98.03 only when y is plain and z is an
   embedded assignment in a do-while magnitude block), a separate delta temp
   for the (u16) subtraction, turn=delta before/after the magnitude,
   delta = turn, two do-while blocks, a pointer local to vec (98.70 but 596
   bytes, one instruction short), and three permuter runs (2063 -> 641 -> 296).
   Round 419 re-measured this body at the same 98.03 / 600 bytes and tried the
   two abs levers:
   - __builtin_abs(delta) shared by both blocks: 95.50 / 596 bytes / 149 insns.
     It DOES give retail's per-block abs shape (bgez $v1 / move rd,$v1 /
     negu rd,rd, where this body has negu rd,$v1), but reorg then hoists the
     rate sll past the bgez and the body is one instruction short.
   - per-site locals deltaY/deltaZ/magY/magZ/turnY/turnZ with __builtin_abs:
     95.29 / 604 / 151. Both abs shapes are retail's and the z copy
     move $v1,$v0 survives, but rate lands in $a0 (retail $a1), the y turn in
     $a1 (retail $a3), and sched1 inserts two load nops where retail has none.
   - plain turn = delta; mag = delta; (turn before the magnitude, plain
     statements, no do-while) in BOTH blocks: 93.70 / 604 / 151 - one pseudo for
     both blocks' delta makes gcc copy it twice.
   So the abs shapes want __builtin_abs while the size and the rate/turn
   registers want this do-while body; no one spelling has both, and the unit
   runs -fschedule-insns, so the z block's two load nops are the scheduler.
   Retail's y delta in $v1 against its z delta in $v0 does say the original had
   two delta locals (one pseudo per variable), but every split spelling measured
   here or earlier loses either a register or a byte.
   Round 434 re-measured at 98.03 / 600 / 150 and re-ran the abs levers with the
   per-site-local recipe of d-an-abs-needs-one-local-per-site (shared delta,
   magY/magZ). All three keep retail's 600 bytes / 150 insns:
   - magY/magZ = __builtin_abs(delta), shared delta: 93.32. This DOES give
     retail's per-block abs shape in BOTH blocks (bgez / move v0,X / negu v0,v0)
     but the delta pseudo then lands in $a0 in BOTH blocks (retail has $v1 in y
     and $v0 in z) and the z block's turn/mag copies collapse with it; the
     builtin also takes $v0 for both sites, so the per-site locals did not split
     them here the way they do on world_map/misc12.
   - magY/magZ = delta with if (mag < 0) mag = -mag: 92.92, the three copies
     collapse and the second block's curRot.vy reload moves above the store.
   - the same per-site pair behind the do-while barrier: 95.85, delta back in
     $a0 in both blocks (the barrier does not restore the winning body's $v1
     once mag has two pseudos).
   So the 98.03 body's SHARED single `mag` is what holds delta in retail's $v1;
   every split of it costs the y block more than it can win in the z block.
   Preset screened as well: gcc-2.6.0 -G0 scores 95.47 here and drops
   BattleActorFaceTarget to 76.34; gcc-2.7.2 -G0 scores 98.00 and drops the same
   function, so XenoBattleTailCdk0 is the right preset for the unit.
void func_800B65B0(BattleSpriteState *pSprite, u8 *p) {
    SVECTOR rot;
    SVECTOR curRot;
    VECTOR diff;
    VECTOR sq;
    VECTOR move;
    SVECTOR vec;
    MATRIX mtx;
    VECTOR out;
    s32 speed;
    s32 len;
    s32 len2;
    s32 rate;
    s32 delta;
    s32 mag;
    s32 turn;
    s16 zero;

    zero = 0;
    diff.vx = pSprite->targetPosition.x - pSprite->position.xWhole;
    diff.vy = pSprite->targetPosition.y - pSprite->position.yWhole;
    diff.vz = pSprite->targetPosition.z - pSprite->position.zWhole;
    Square0(&diff, &sq);
    len = SquareRoot0(sq.vx + sq.vz);
    rot.vy = -ratan2(diff.vz, diff.vx);
    rot.vz = ratan2(diff.vy, (s16)len);
    rot.vx = 0;

    move.vx = pSprite->step.x >> 7;
    move.vy = pSprite->step.y >> 7;
    move.vz = pSprite->step.z >> 7;
    Square0(&move, &sq);
    speed = SquareRoot0(sq.vy + sq.vx + sq.vz);
    len2 = SquareRoot0(sq.vx + sq.vz);
    curRot.vy = -ratan2(move.vz, move.vx);
    curRot.vz = ratan2(move.vy, (s16)len2);
    curRot.vx = 0;

    rate = p[0] << 2;
    delta = ((u16)rot.vy - (u16)curRot.vy << 20) >> 20;
    do {
        mag = delta;
    } while (0);
    if (mag < 0) {
        mag = -mag;
    }
    turn = delta;
    if (mag > rate) {
        turn = rate;
        if (delta < 0) {
            turn = -rate;
        }
    }
    curRot.vy += turn;

    turn = (delta = ((u16)rot.vz - (u16)curRot.vz << 20) >> 20);
    do {
        mag = delta;
    } while (0);
    if (mag < 0) {
        mag = -mag;
    }
    if (mag > rate) {
        turn = rate;
        if (delta < 0) {
            turn = -rate;
        }
    }
    curRot.vz += turn;

    SpriteSetSVec(&vec, (s16)speed, 0, 0);
    RotMatrix(&curRot, &mtx);
    ApplyMatrix(&mtx, &vec, &out);
    pSprite->step.x = out.vx << 7;
    pSprite->step.y = out.vy << 7;
    pSprite->step.z = out.vz << 7;
}
*/
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
// Ages every live record of the table one frame: a record with a timer still
// running only counts down, the rest fade their colours toward the clamp, drift
// by their fixed-point velocity, spin by their step, and damp those velocities
// by a sixteenth of themselves before gravity is added back.
// The three velocities go through s32 temporaries: read straight into the
// SVECTOR gcc widens with lhu + sll/sra where retail sign-extends at the load.
// pRows is re-derived inside the middle loop because retail re-reads the spilled
// table pointer once per row set; hoisting it to the outer loop keeps the table
// in a register and loses retail's extra frame slot, reload and biv chain.
void BattleUpdateEffectTable(BattleEffectTask *pTask) {
    BattleEffectTable *pTable;
    BattleEffectRow *pRows;
    BattleEffect *pEntry;
    BattleEffectColor *pColor;
    BattleRenderState *pState;
    u8 *pStateWork;
    SVECTOR vel;
    s32 vx;
    s32 vy;
    s32 vz;
    s32 i;
    s32 j;
    s32 k;

    pTable = pTask->pTable;
    pTable->frameCount++;
    for (i = 0; i != 2; i++) {
        for (j = 0; j != 14; j++) {
            // retail re-derives the row base from the spilled table once per row
            // set, and does it after the inner loop's counter is cleared
            for (k = 0, pRows = pTable->entries[i]; k != 20; k++) {
                pEntry = &pRows[j][k];
                if (pEntry->timer != 0) {
                    pEntry->timer--;
                    continue;
                }
                pStateWork = g_BattleStateWork;
                pState = (BattleRenderState *)(pStateWork + 0x8000);
                pColor = &pEntry->colors[pState->colorSetIndex];
                pColor->r = SpriteColorAddClamp(pColor->r, -BATTLE_EFFECT_FADE_STEP);
                pColor->g = SpriteColorAddClamp(pColor->g, -BATTLE_EFFECT_FADE_STEP);
                pColor->b = SpriteColorAddClamp(pColor->b, -BATTLE_EFFECT_FADE_STEP);
                vx = pEntry->velX.parts.whole;
                vel.vx = vx;
                vy = pEntry->velY.parts.whole;
                vel.vy = vy;
                vz = pEntry->velZ.parts.whole;
                vel.vz = vz;
                pEntry->posX += vel.vx;
                pEntry->posY += vel.vy;
                pEntry->posZ += vel.vz;
                pEntry->rotX += pEntry->rotStepX;
                pEntry->rotY += pEntry->rotStepY;
                pEntry->rotZ += pEntry->rotStepZ;
                pEntry->velY.value -= pEntry->velY.value / 16;
                pEntry->velX.value -= pEntry->velX.value / 16;
                pEntry->velZ.value -= pEntry->velZ.value / 16;
                pEntry->velY.value += pEntry->grav;
            }
        }
    }
}

void BattleStoreGfxOrderingTable(BattleEffectTask *pTask) {
    g_BattleSavedGfxOT = g_GfxCurOT;
    BattleDrawEffectTable(pTask);
}
/* Draws every live record of the effect table under its own 160/112/512 GTE
   setup: each record's rotation and position are transformed, and its
   colours[idx] prim (tag, r/g/b, then a screen pair per vertex at +8/+0x10/+0x18)
   is projected by RotTransPers3 and linked into the saved ordering table at
   depth >> 6.  A record whose posZ is nearer than 0x40 is skipped; the dimmer
   index is the word at 0x8C84 of g_BattleStateWork, and i selects the vertex
   set D_800C3594 or D_800C35AC.
   pStateWork holds the same base as g_BattleStateWork because retail's two-step
   g_BattleStateWork + 0x8000 address formation only survives as two assignments;
   otIndex likewise keeps the >> 6 ahead of the ordering-table load. */

void BattleDrawEffectTable(BattleEffectTask *pTask) {
    long geomOffset[2];
    MATRIX mtx;
    long depth;
    long flag;
    BattleEffectTable *pTable;
    BattleEffect *pEntry;
    BattleEffectColor *pColor;
    SVECTOR *pVertices;
    BattleRenderState *pState;
    u8 *pStateWork;
    s32 screen;
    s32 otIndex;
    s32 i;
    s32 j;
    s32 k;

    pTable = pTask->pTable;
    ReadGeomOffset(geomOffset, geomOffset + 1);
    screen = ReadGeomScreen();
    SetGeomOffset(160, 112);
    SetGeomScreen(512);
    for (i = 0; i != 2; i++) {
        for (j = 0; j != 14; j++) {
            for (k = 0; k != 20; k++) {
                pEntry = &pTable->entries[i][j][k];
                pStateWork = g_BattleStateWork;
                pState = (BattleRenderState *)(pStateWork + 0x8000);
                pColor = &pEntry->colors[pState->colorSetIndex];
                if (pEntry->posZ < BATTLE_EFFECT_MIN_DRAW_DEPTH) {
                    continue;
                }
                RotMatrix((SVECTOR *)pEntry, &mtx);
                TransMatrix(&mtx, (VECTOR *)&pEntry->posX);
                SetRotMatrix(&mtx);
                SetTransMatrix(&mtx);
                if (i == 0) {
                    pVertices = D_800C3594;
                } else {
                    pVertices = D_800C35AC;
                }
                otIndex = RotTransPers3(pVertices, pVertices + 1, pVertices + 2, pColor->sxy[0],
                                        pColor->sxy[1], pColor->sxy[2], &depth, &flag) >> 6;
                AddPrim(g_BattleSavedGfxOT + otIndex, pColor);
            }
        }
    }
    SetGeomOffset(geomOffset[0], geomOffset[1]);
    SetGeomScreen(screen);
}

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
    func_800B7424(WorkListsAddTasks(0x10F7C, 0, BattleUpdateEffectTable, BattleStoreGfxOrderingTable, BattleDestroyWorkTask));
}
