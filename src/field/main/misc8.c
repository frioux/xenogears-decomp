#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"
#include "field/effects.h"
#include "field/script_vm.h"
#include "system/math.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/sound.h"
#include "psyq/libetc.h"
#include "psyq/libcd.h"
#include "field/particles.h"

extern int g_FieldActiveActorCount;
void FieldFreeActorData(int actorIndex) {
    ActorData* pActor;

    if (actorIndex < g_FieldActiveActorCount) {
        pActor = g_FieldActors[actorIndex].pActorData;
        if (pActor->flags134 & 0x80) {
            HeapFree(pActor->pTargetTrack);
        }
        if (pActor->flags12C_0xD) {
            HeapFree(pActor->pScriptArgs);
        }
        if (g_FieldActors[actorIndex].status & 0x2000) {
            HeapFree(pActor->pScriptWorkWords);
        }
        if (pActor->specialAnimArchiveIndex != -1) {
            HeapFree(pActor->pSpecialAnimFile);
        }
        HeapFree(pActor);
        HeapFree(g_FieldActors[actorIndex].pShadow);
        SpriteDestroy(g_FieldActors[actorIndex].pSpriteData);
    }
}

typedef struct {
    /* 0x0 */ undefined8 unk0[0xC];
    /* 0xC */ u8 material;
    /* 0xD */ undefined8 padD;
} FieldWalkmeshTriangle; // Size: 0xE

extern FieldWalkmeshTriangle* g_FieldWalkmeshTriangles[];
extern u_int* g_FieldWalkmeshMaterialFlags;

u_int FieldActorGetWalkmeshTriMaterial(ActorData* pActor) {
    if ((pActor->flags >> (pActor->walkmeshId + 3)) & 1) {
        return 0;
    }
    return g_FieldWalkmeshMaterialFlags[g_FieldWalkmeshTriangles[pActor->walkmeshId][pActor->walkmeshTriIds[pActor->walkmeshId]].material];
}

extern u_short g_FieldRampStepCount;

s32 FieldSumRampSteps(s32* pData) {
    s32 value = -0x14D000;
    s32 step = pData[7];
    s32 acc = 0;
    u_short count;

    g_FieldRampStepCount = 0;
    do {
        count = g_FieldRampStepCount;
        count += 2;
        g_FieldRampStepCount = count;
        acc += value;
    } while ((value += step) <= 0);
    return acc >> 16;
}

extern s32 g_FieldModelScriptActorIndex;
extern s32 g_FieldModelScriptWordIndex;

u_int FieldPopActorScriptWorkWord(void) {
    ActorData* pActor = g_FieldActors[g_FieldModelScriptActorIndex].pActorData;

    return ((u_int*)pActor->pScriptWorkWords)[g_FieldModelScriptWordIndex++];
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80080A74);

/* 90.26% (468 bytes vs retail 456: three instructions too many). The levers
   that got it from 71.39% here: g_FieldActorAllocCount is indexed as g_FieldActorAllocCount[0]++
   (a plain scalar is emitted as lui at + sw sym), and the whole tail from the
   pActor reload to the pShadow store is wrapped in a one-iteration do-while
   so cc1 schedules it as a single unit. Residue: retail stores pActorData and
   only THEN re-forms the g_FieldActors base for the reload (sw v0,76(a0);
   lui v0; lw v0; move a0,zero; addu v1,v1,v0; lw v1,76(v1)), while gcc forms
   both bases before the store and so puts the loop counter in a2 instead of
   a0, plus the unk5A/status pair gets a third base load (gcc reloads the
   volatile g_FieldActors where retail reuses one register for two reads).
   Tried: pActor/pFieldActor locals, indexed vs pointer forms, removing the
   pActor local, permuter (found the do-while).
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80080F44);
/*
96.36% (468 bytes = retail's 468, register assignment only). The volatile store
is load-bearing: writing `*(ActorData* volatile*)&g_FieldActors[i].pActorData =
HeapAlloc(0x138, 0);` makes cc1 keep that store ahead of the volatile reload of
g_FieldActors that follows it (a plain store lets gcc hoist the second base load
above it and costs the loop counter its a0 - 90.26%). Residue: retail gives the
store's element a0, the reload's v1 and reloads pActorData through a0, ours uses
v0/a2/v1 for the same three; three earlier statements (the `for` body, the
spriteFlags store and the pData reload) are otherwise identical.
Tried: the volatile store above; pActor/pFieldActor locals; indexed vs pointer
forms; removing the pActor local; permuter (found the do-while in the tail).
extern int g_FieldActorAllocCount[];

void func_80080F44(int actorIndex) {
    int numWords = sizeof(ActorData) / sizeof(u_int);
    int i;
    u_int* pData;
    ActorData* pActor;
    FieldActorModelData* pModelData;
    FieldActor* pFieldActor;
    u8 unused[0x60];

    if (actorIndex < g_FieldActiveActorCount) {
        g_FieldActorAllocCount[0]++;
        *(ActorData* volatile*)&g_FieldActors[actorIndex].pActorData = HeapAlloc(0x138, 0);
        pData = (u_int*)g_FieldActors[actorIndex].pActorData;
        for (i = 0; i < numWords; i++) {
            *pData++ = 0;
        }
        g_FieldActors[actorIndex].spriteFlags = 0;
        do {
            pActor = g_FieldActors[actorIndex].pActorData;
            pFieldActor = &g_FieldActors[actorIndex];
            if (pFieldActor->status & 0x2000) {
                pModelData = pFieldActor->pModelData;
                pActor->pScriptWorkWords = HeapAlloc(0x80, 0);
                if (pModelData->pScripts != NULL) {
                    for (i = 0; i < pModelData->pScripts->numScripts; i++) {
                        pModelData->pScripts->pScripts[i].pCallback = FieldPopActorScriptWorkWord;
                        ((u_int*)pActor->pScriptWorkWords)[i] = 0;
                    }
                }
            }
            func_80080A74(actorIndex);
            g_FieldActors[actorIndex].pShadow = HeapAlloc(0x70, 0);
            // the one-iteration do-while keeps cc1 scheduling the tail as one unit
        } while (0);
        FieldInitShadowQuad((Quad*)g_FieldActors[actorIndex].pShadow);
    }
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008110C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800815F0);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081C54);
/* 63.38% (772 vs retail 776 bytes): the ring layout is settled - 32 entries of
   0x48 bytes based at D_800B14F0, filled from the player actor
   (pActorData = FieldActor 0x4C, pSpriteData = 0x4; the earlier note had those
   two swapped) and indexed by the volatile s32 D_800B2360, which retail
   re-reads once per statement (13 reads). Then D_800C3910 = 0 and
   D_800B2360 = (D_800B2360 - 1) & 0x1F. Two early outs:
   actorIndex != g_PlayerActorIndex and D_800B21CC != 0.
   Our instruction ORDER per store is value-load before the index read where
   retail reads the index first, gcc hoists &D_800B1506 (the loop's tri-id
   base) to the top where retail derives it mid-body from la a1,D_800B1510
   (-0xA and +0x10), and the 0x18 frame plus retail's addiu sp,-0x18 in the bne
   delay slot are missing (retail has a dead ~0x18 local). Every store's reloc
   is D_800B14F0+off here against a splat label there, which diff_func folds -
   so this one can only be judged on a MATCH line plus equal byte count, then
   check_bin.sh. Struct below (FieldActorRenderPacket) is declared in this .c.
typedef struct {
    // 0x00 u_int scriptFlags;
    // 0x04 u_int flags;
    // 0x08 s16 positionX;
    // 0x0A s16 positionY;
    // 0x0C s16 positionZ;
    // 0x0E s16 unk0E;
    // 0x10 u_short spriteField84;
    // 0x12 u_short animationId;
    // 0x14 u_short rotation;
    // 0x16 s16 walkmeshTriIds[4];
    // 0x1E u_short unk1E;
    // 0x20 IVEC3 step;
    // 0x2C s32 unk2C;
    // 0x30 IVEC3 normal;
    // 0x3C s32 unk3C;
    // 0x40 u_int walkmeshTriMaterial;
    // 0x44 u8 walkmeshId;
    // 0x45 undefined8 pad45[3];
} FieldActorRenderPacket; // Size: 0x48

extern FieldActorRenderPacket D_800B14F0[];
extern volatile s32 D_800B2360;
extern s32 D_800C3910;
extern u_char D_800B21CC;
extern s32 g_PlayerActorIndex;

void func_80081C54(int actorIndex) {
    FieldActor* pFieldActor = &g_FieldActors[actorIndex];
    ActorData* pActor = pFieldActor->pActorData;
    SpriteData* pSprite = pFieldActor->pSpriteData;
    int i;

    if (actorIndex != g_PlayerActorIndex) {
        return;
    }
    if (D_800B21CC != 0) {
        return;
    }
    D_800B14F0[D_800B2360].step.x = pSprite->step.x;
    D_800B14F0[D_800B2360].step.y = pSprite->step.y;
    D_800B14F0[D_800B2360].step.z = pSprite->step.z;
    D_800B14F0[D_800B2360].normal.x = pActor->curTriNormal.vx;
    D_800B14F0[D_800B2360].normal.y = pActor->curTriNormal.vy;
    D_800B14F0[D_800B2360].normal.z = pActor->curTriNormal.vz;
    D_800B14F0[D_800B2360].rotation = (u_short)pActor->rotation.vy & 0xFFF;
    D_800B14F0[D_800B2360].spriteField84 = (u_short)pSprite->field_0x84;
    D_800B14F0[D_800B2360].positionX = ((s16*)&pActor->position)[1];
    D_800B14F0[D_800B2360].positionY = ((s16*)&pActor->position)[3];
    D_800B14F0[D_800B2360].positionZ = ((s16*)&pActor->position)[5];
    D_800B14F0[D_800B2360].animationId = (u_short)pActor->curAnimationId;
    D_800B14F0[D_800B2360].walkmeshTriMaterial = pActor->curWalkmeshTriMaterial;
    D_800B14F0[D_800B2360].scriptFlags = pActor->scriptFlags.flags;
    D_800B14F0[D_800B2360].flags = pActor->flags;
    for (i = 0; i < 4; i++) {
        D_800B14F0[D_800B2360].walkmeshTriIds[i] = pActor->walkmeshTriIds[i];
    }
    D_800B14F0[D_800B2360].walkmeshId = (u8)pActor->walkmeshId;
    D_800C3910 = 0;
    D_800B2360 = (D_800B2360 - 1) & 0x1F;
}
*/
int FieldActorTestMaterialScriptMask(ActorData* pActor) {
    u_int actorType = (pActor->scriptFlags.flags >> 9) & 3;

    return -((actorType & (pActor->curWalkmeshTriMaterial >> 3)) != 0);
}

/* 58.17% (ours 644 bytes; retail 640). Structure is right - status & 0x40
   selects the two halves, the scale is (N / pActorData->moveSpeed >> 8) << 5
   with N = 0x40000 (status bit clear, and the 0x20000-set path stores
   moveSpeed = 0x4000000 / moveSpeed) or 0x80000 (flags & 0x2000, 0x20000
   clear), and the shared step.x/step.z computation is duplicated in the
   original: one copy inline after the 0x40000 case, one at 0x800820FC that the
   0x40000-free paths jump into. arg0 is SpriteData* (step at 0xC/0x14,
   moveSpeed at 0x18), arg2 is FieldActor*, and scaleX/scaleZ must be read
   signed (lh, so cast the header's u_short to s16).
   Round 166 PARK REPAIR: this body now COMPILES against today's header (it was
   written when the 0x12C word was read through a now-gone ActorFlags12CView
   typedef; bits 13-15 are ActorData::flags12C_0xE, and g_FieldActorModelPointers
   is declared here because only field/main/main.c has it) and the real score is
   the 58.17% below, so the old number was not fiction.
   Residue: gcc lays the flags & 0x2000 then-block out as fall-through where the
   original jumps forward over the 0x80000 block, uses at to form
   g_FieldActorModelPointers + (flags12C_0xE * 4) where the original keeps it in
   v1 via lui/addiu, and spills one extra register (frame 0x38 vs 0x28).
   else if (!(flags & 0x2000)) first gives 55.87%.
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081F80);
/*
extern void* g_FieldActorModelPointers[];

void func_80081F80(SpriteData* pSprite, s16 angle, FieldActor* pActor) {
    int scale;
    int a;

    if (!(pActor->status & 0x40)) {
        scale = (0x40000 / pActor->pActorData->moveSpeed >> 8) << 5;
        if (angle & 0x8000) {
            pSprite->step.x = 0;
            pSprite->step.z = 0;
        } else {
            a = angle & 0xFFF;
            pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
            pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
        }
    } else if (angle & 0x8000) {
        pSprite->step.x = 0;
        pSprite->step.z = 0;
    } else if (pActor->pActorData->flags & 0x2000) {
        if (pActor->pActorData->flags & 0x20000) {
            pSprite->step.x = -((s32*)g_FieldActorModelPointers[pActor->pActorData->flags12C_0xE])[0x128 / 4] << 16;
            pSprite->step.z = -((s32*)g_FieldActorModelPointers[pActor->pActorData->flags12C_0xE])[0x130 / 4] << 16;
        } else {
            scale = (0x80000 / pActor->pActorData->moveSpeed >> 8) << 5;
            a = angle & 0xFFF;
            pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
            pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
        }
    } else if (pActor->pActorData->flags & 0x80000) {
        scale = (0x40000 / pActor->pActorData->moveSpeed >> 8) << 5;
        a = angle & 0xFFF;
        pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
        pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
        pSprite->moveSpeed = 0x4000000 / pActor->pActorData->moveSpeed;
    } else {
        SpriteSetDirection(pSprite, angle);
    }
    pSprite->step.x &= -0x1000;
    pSprite->step.z &= -0x1000;
}
*/

extern s16 g_Unknown_800B2344;
extern s16 g_FieldCurrentModelAnimationId[];
extern u8 g_FieldModelAnimationIdsByActor[];
void SpritePlayAnimation(int, int);
void LibGearSetModelAnimation(u16, u16, s32);

// This unit reads the 0x12C word whole; the ActorData bitfields there make gcc
// narrow the load to a byte.
typedef struct {
    /* 0x0   */ u8 unk0[0x12C];
    /* 0x12C */ u_int flags12C;
} ActorFlags12CView; // Size: 0x130

void FieldSetGearModelAnimationForActor(int arg0, int actorId, FieldActor* pActor) {
    if (!(pActor->status & 0x40)) {
        return;
    }
    if (actorId != 3 && g_Unknown_800B2344 == 0) {
        pActor->pActorData->scriptFlags.flags &= ~0x800;
    }
    if (actorId == 0xFF) {
        actorId = 0;
    }
    // The model-id table is the run of shorts ending at g_FieldCurrentModelAnimationId, which holds
    // the id currently in use, so this file indexes it from that symbol.
    if (actorId != g_FieldCurrentModelAnimationId[0]) {
        pActor->pActorData->scriptFlags.flags &= ~0x800;
    }
    if (!(pActor->pActorData->flags & 0x2000)) {
        if (pActor->pActorData->flags & 0x1000000) {
            return;
        }
        SpritePlayAnimation(arg0, actorId);
        return;
    }
    if (actorId < 0x10) {
        LibGearSetModelAnimation(pActor->pActorData->flags12C_0xE, 0, g_FieldModelAnimationIdsByActor[actorId]);
        g_FieldCurrentModelAnimationId[pActor->pActorData->flags12C_0xE - 0xB1] = g_FieldModelAnimationIdsByActor[actorId];
    } else {
        actorId -= 0x10;
        LibGearSetModelAnimation(pActor->pActorData->flags12C_0xE, 0, actorId);
        g_FieldCurrentModelAnimationId[pActor->pActorData->flags12C_0xE - 0xB1] = actorId;
    }
}

// Rectangular trigger zone given as a centre and per-axis half-extents, unlike
// the quad-corner FieldTriggerZone.
typedef struct {
    /* 0x0  */ u8 unk0[0x18];
    /* 0x18 */ u_short halfExtentZ;
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ u_short halfExtentX;
    /* 0x1E */ u8 unk1E[0x4];
    /* 0x22 */ s16 centerZ;
    /* 0x24 */ u8 unk24[0x6];
    /* 0x2A */ s16 centerX;
} FieldExtentZone; // Size: 0x2C

void func_80281678(FieldExtentZone* pZone);

int FieldCheckPointInExtentZone(int z, int x, FieldExtentZone* pZone, int radius) {
    long actorPosition2D;
    long triggerPos1;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;

    actorPosition2D = (z << 0x10) + x;
    triggerPos1 = ((pZone->centerZ - pZone->halfExtentZ - radius) << 0x10) + (pZone->centerX + pZone->halfExtentX + radius);
    triggerPos2 = ((pZone->centerZ + pZone->halfExtentZ + radius) << 0x10) + (pZone->centerX + pZone->halfExtentX + radius);
    triggerPos3 = ((pZone->centerZ + pZone->halfExtentZ + radius) << 0x10) + (pZone->centerX - pZone->halfExtentX - radius);
    triggerPos4 = ((pZone->centerZ - pZone->halfExtentZ - radius) << 0x10) + (pZone->centerX - pZone->halfExtentX - radius);
    if (NormalClip(triggerPos1, triggerPos2, actorPosition2D) < 0) {
        return -1;
    }
    if (NormalClip(triggerPos2, triggerPos3, actorPosition2D) < 0) {
        return -1;
    }
    if (NormalClip(triggerPos3, triggerPos4, actorPosition2D) < 0) {
        return -1;
    }
    if (NormalClip(triggerPos4, triggerPos1, actorPosition2D) < 0) {
        return -1;
    }
    if (g_FieldSystemMode == 0) {
        func_80281678(pZone);
    }
    return 0;
}

// Four 2D trigger-zone corners packed one per short pair as (z << 16) + x.
typedef struct {
    /* 0x0 */ s16 z0;
    /* 0x2 */ s16 x0;
    /* 0x4 */ s16 z1;
    /* 0x6 */ s16 x1;
    /* 0x8 */ s16 z2;
    /* 0xA */ s16 x2;
    /* 0xC */ s16 z3;
    /* 0xE */ s16 x3;
} FieldZone2D; // Size: 0x10

int FieldCheckActorInZone2D(IVEC3* pOffset, ActorData* pActor) {
    long actorPosition2D;
    long triggerPos1;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;
    FieldZone2D* pZone;

    if (!pActor->flags12C_0xD) {
        return 0;
    }
    actorPosition2D = (CONV_TO_GTE(pActor->position.vx + pOffset->x) << 0x10) + CONV_TO_GTE(pActor->position.vz + pOffset->z);
    pZone = pActor->pScriptArgs;
    triggerPos1 = (pZone->z0 << 0x10) + pZone->x0;
    triggerPos2 = (pZone->z1 << 0x10) + pZone->x1;
    triggerPos3 = (pZone->z2 << 0x10) + pZone->x2;
    triggerPos4 = (pZone->z3 << 0x10) + pZone->x3;
    if (NormalClip(triggerPos1, triggerPos2, actorPosition2D) < 0 ||
        NormalClip(triggerPos2, triggerPos3, actorPosition2D) < 0) {
        return -1;
    }
    if (NormalClip(triggerPos3, triggerPos4, actorPosition2D) >= 0) {
        return NormalClip(triggerPos4, triggerPos1, actorPosition2D) >> 31;
    }
    return -1;
}

long FieldGetVec2Magnitude(long, long);

int FieldGetActorDistance2D(int actorIndexA, int actorIndexB) {
    FieldActor* pActors = g_FieldActors;
    ActorData* pActorB = pActors[actorIndexB].pActorData;
    ActorData* pActorA = pActors[actorIndexA].pActorData;
    int posXB = CONV_TO_GTE(pActorB->position.vx);
    int posZB = CONV_TO_GTE(pActorB->position.vz);
    int posXA = CONV_TO_GTE(pActorA->position.vx);
    int posZA = CONV_TO_GTE(pActorA->position.vz);

    return FieldGetVec2Magnitude(posXB - posXA, posZB - posZA);
}

extern u_short g_FieldMaterialAngles[];
extern s16 D_800ADFC4[];
extern s32 D_800AF858;
extern u_char D_800B21CC;
extern void* g_FieldActorModelPointers[];

// This unit writes one word at SpriteData+0x10, which include/field/actor.h
// splits into two 16-bit texture coordinates.
typedef struct {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 unk14[0x8];
    /* 0x1C */ s32 gravity;
} SpritePhysicsView; // Size: 0x20

// pActor->pTargetTrack: 0xC bytes holding the copy of the tracked actor's rotation
// the deltas below are taken against, then the last measured distance.
typedef struct {
    /* 0x0 */ u_short rotationX;
    /* 0x2 */ u_short rotationY;
    /* 0x4 */ u_short rotationZ;
    /* 0x6 */ u_short unk6;
    /* 0x8 */ s16 distance;
    /* 0xA */ u_short unkA;
} FieldActorTargetTrack; // Size: 0xC

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80082620);
/* 95.34% (1448 bytes vs retail's 1424, 362 vs 356 insns). Everything structural
   is settled; what is left is register allocation plus two orderings.
   Settled:
   - arg1 is FieldActor* and arg2 is the actor's ActorData* (the caller
     func_8008110C passes &g_FieldActors[i] and its 0x4C field). arg1 is used
     for exactly one load, pSpriteData.
   - The 0x74 field is re-read from memory at every one of its six uses, and the
     three delta blocks each re-read it and pActor->pTargetTrack; only the
     g_FieldActors pointer is loaded once for those three, so the source read
     that global into a local (pTracked) for the deltas and spelled it out again
     for the target positions, which is what retail does (a1 kept across the
     three deltas, a fresh lui/lw before the target position loads).
   - g_FieldMaterialAngles is u_short[8] and g_FieldControl.unkAngle is read unsigned (lhu
     in retail), hence the (u_short) cast and the u_short extern.
   - The height clamp must be written condition-inverted (limit <= unkF0 >> 16
     storing limit << 16 first, gravity add in the else): 94.74% -> retail's
     exact delay-slot shape, where the or-else value is what reorg steals into
     the beqz slot.
   - The move updates must NOT be written as (targetPos - pos) + step; retail
     re-forms the subtraction at each site, so the term order is
     targetPos + step - pos.
   Diffs left:
   (1) One extra callee-saved register: mine keeps the s16 rotation delta in s7
       and pushes actorIndex to s8 (10 saved words vs retail's 9), while retail
       puts the s16 delta in s0 (reusing dead stepX) and actorIndex in s7. Tried
       s32/s16 deltas, a separate truncated local assigned right after the
       subtraction, and dropping the intermediate entirely - always 95.34%.
   (2) The triMaterial & 0x400000 block: retail emits move.vx += stepX; then
       rotation.x |= 0x8000; then move.vz += stepZ, mine merges the two move
       updates and stores rotation.x last (source order is already retail's).
   (3) One duplicated andi triMaterial,0x8000 lands in a branch delay slot.
void func_80082620(int actorIndex, FieldActor* pFieldActor, ActorData* pActor) {
    IVEC3 moveVec;
    VECTOR normal;
    VECTOR unitNormal;
    VECTOR unused;
    SVECTOR delta;
    SpritePhysicsView* pSprite;
    u_int triMaterial = 0;
    s32 stepX = 0;
    s32 stepZ = 0;
    s32 limit;
    s32 angle;
    s32 distance;
    s32 posX;
    s32 posZ;
    s32 targetPosX;
    s32 targetPosZ;
    s16 dyAngle;
    FieldActor* pTracked;
    ActorData* pTarget;

    if (!((pActor->flags >> (pActor->walkmeshId + 3)) & 1)) {
        if (D_800B21CC == 0) {
            triMaterial = pActor->curWalkmeshTriMaterial;
        }
    }
    pSprite = (SpritePhysicsView*)pFieldActor->pSpriteData;
    FieldGetVec2FromPolar(&moveVec, D_800ADFC4[(triMaterial >> 9) & 3],
        (g_FieldMaterialAngles[(triMaterial >> 11) & 7] + (u_short)g_FieldControl.unkAngle) & 0xFFF);
    if (pActor->scriptFlags.flags & 0x41800) {
        if (triMaterial & 0x4000) {
            pActor->move.vx += moveVec.x;
            pActor->move.vy += moveVec.y;
            pActor->move.vz += moveVec.z;
        }
    } else {
        s32 height = pActor->fallSpeed;

        if (triMaterial & 0x420000) {
            normal.vx = -(pActor->curTriNormal.vx * pActor->curTriNormal.vy) >> 15;
            normal.vy = 0;
            normal.vz = -(pActor->curTriNormal.vz * pActor->curTriNormal.vy) >> 15;
            if (normal.vx == 0) {
                normal.vx = 1;
            }
            if (normal.vy == 0) {
                normal.vy = 1;
            }
            if (normal.vz == 0) {
                normal.vz = 1;
            }
            VectorNormal(&normal, &unitNormal);
            if (unitNormal.vx == 0) {
                unitNormal.vx = 1;
            }
            if (unitNormal.vy == 0) {
                unitNormal.vy = 1;
            }
            if (unitNormal.vz == 0) {
                unitNormal.vz = 1;
            }
            stepX = unitNormal.vx * (height >> 17) << 4;
            stepZ = unitNormal.vz * (height >> 17) << 4;
            limit = 0xC;
            if (triMaterial & 0x400000) {
                limit = 0x18;
            }
            if (limit <= (pActor->fallSpeed >> 16)) {
                pActor->fallSpeed = limit << 16;
            } else {
                pActor->fallSpeed += pSprite->gravity;
            }
            pSprite->unk10 = pActor->fallSpeed >> 1;
        }
        if (triMaterial & 0x400000) {
            pActor->move.vx += stepX;
            ((u_short*)&pActor->rotation.vx)[0] |= 0x8000;
            pActor->move.vz += stepZ;
        }
        if (pActor->canInteract == 0xFF) {
            if (triMaterial & 0x20000) {
                pActor->move.vx += stepX;
                pActor->move.vz += stepZ;
            }
        }
        if (triMaterial & 0x8000) {
            pActor->move.vx += moveVec.x;
            pActor->move.vy += moveVec.y;
            pActor->move.vz += moveVec.z;
        }
    }

    if (pActor->canInteract != 0xFF) {
        if ((g_FieldActors[pActor->canInteract].pActorData->flags & 0xC0) == 0xC0) {
            if (!(pActor->flags134 & 0x80)) {
                pActor->pTargetTrack = HeapAlloc(0xC, 0);
                pActor->flags134 |= 0x80;
            }
            pTracked = g_FieldActors;
            delta.vx = pTracked[pActor->canInteract].rotation.x -
                ((FieldActorTargetTrack*)pActor->pTargetTrack)->rotationX;
            dyAngle = pTracked[pActor->canInteract].rotation.y -
                ((FieldActorTargetTrack*)pActor->pTargetTrack)->rotationY;
            delta.vy = dyAngle;
            delta.vz = pTracked[pActor->canInteract].rotation.z -
                ((FieldActorTargetTrack*)pActor->pTargetTrack)->rotationZ;
            ((FieldActorTargetTrack*)pActor->pTargetTrack)->rotationY =
                pTracked[pActor->canInteract].rotation.y;
            pTarget = g_FieldActors[pActor->canInteract].pActorData;
            posX = pActor->position.vx;
            posZ = pActor->position.vz;
            targetPosX = pTarget->position.vx;
            targetPosZ = pTarget->position.vz;
            if (!(((u_short*)&pActor->rotation.vx)[0] & 0x8000)) {
                ((FieldActorTargetTrack*)pActor->pTargetTrack)->distance =
                    FieldGetActorDistance2D(actorIndex, pActor->canInteract);
            }
            distance = ((FieldActorTargetTrack*)pActor->pTargetTrack)->distance;
            angle = (s16)ratan2(targetPosZ - posZ, targetPosX - posX) - dyAngle - 0x800;
            pActor->move.vx += targetPosX + ((rsin(angle) * distance) << 4) - posX;
            pActor->move.vz += targetPosZ + ((rcos(angle) * distance) << 4) - posZ;
        }
    }

    if ((pActor->flags & 0x22000) == 0x22000) {
        pActor->move.vx -= (((s32*)g_FieldActorModelPointers[D_800AF858])[0x128 / 4] << 16) /
            pActor->moveSpeed << 8;
        pActor->move.vz -= (((s32*)g_FieldActorModelPointers[D_800AF858])[0x130 / 4] << 16) /
            pActor->moveSpeed << 8;
        D_800AF858++;
    }
}
*/


INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80082BB8);

int FieldClampAngleDelta(int angle, int limit) {
    if (angle < 0) {
        angle += 0x4000;
        limit = -limit;
        if (angle < limit) {
            angle = limit;
        }
        if (angle > 0) {
            angle = 0;
        }
    } else {
        angle -= 0x4000;
        if (limit < angle) {
            angle = limit;
        }
        if (angle < 0) {
            angle = 0;
        }
    }
    return angle;
}

void FieldConvertIVec3ToSVector(SVECTOR* pDst, IVEC3* pSrc) {
    pDst->vx = pSrc->x >> 0x10;
    pDst->vy = pSrc->y >> 0x10;
    pDst->vz = pSrc->z >> 0x10;
}

extern void* g_FieldActorModelPointers[];
void SpriteSetDirection(SpriteData*, s16);

void func_80081F80(SpriteData*, s16, FieldActor*);

/* 96.86% (144 vs retail 148 bytes): the frame is 0x18 instead of 0x20 and the
   one missing instruction is retail's copy of rotation.vy out of the load
   register before the 0x8000 mask test (lh v0 / move v1,v0 / andi v0,v0,0x8000)
   where gcc loads straight into v1 and masks from it. Retail's RTL therefore
   tested the load result and did the arithmetic on a SECOND variable, but every
   two-variable spelling (s16/s16, s16/s32, u16 test variable) is copy-propagated
   back into one pseudo. Everything else, including the do/while(0) that holds
   the vy store out of the jal delay slot, is identical. Tried: s16 vs s32 vs u16
   variables, the test reading the field directly (90.92%: gcc then does not CSE
   the second load), an unused int local (frame only), gcc-2.7.2-cdk (89.27%),
   2.6.0 (92.51%), permuter 2 runs / 1896 iters (base 113 = best 113).
void func_800831F4(SpriteData* pSprite, ActorData* pActorData, FieldActor* pActor, int direction) {
    s16 angle;
    s16 current;
    s16 angleCopy;

    if (pActorData->moveModified.vx != 0 || pActorData->moveModified.vz != 0) {
        current = pActorData->rotation.vy;
        if (!(current & 0x8000)) {
            angle = (current + ((direction & 1) ? -0x400 : 0x400)) & 0xFFF;
            // the do/while(0) holds the vy store out of the jal delay slot; a bare block does not
            do {
                angleCopy = angle;
                pActorData->rotation.vx = angle;
            } while (0);
            pActorData->rotation.vy = angleCopy;
            func_80081F80(pSprite, pActorData->rotation.vx, pActor);
            angle = pActorData->rotation.vy | 0x8000;
            pActorData->rotation.vx = angle;
            pActorData->rotation.vy = angle;
        }
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800831F4);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80083288);

void FieldNoOp(void) {
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008399C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084158);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008492C);
/* 91.16%: every check matches; the only residual is the `flags & 2` early
   return. The original keeps it as its own `li v0,-1` block after the
   `bltz`-style tail (280 bytes), so the `beqz` above it keeps a nop delay slot
   and the later `beqz flags&4` gets `move v0,zero` from its target; gcc here
   cross-jumps that return into the shared epilogue and fills the delay slots
   itself (276 bytes). Re-measured: the merge also costs the ordering of the
   `flags & 2` block (ours hoists `li v0,0x1` above `lh v1,0x10($a0)`, the
   original loads first) and leaves the `flags & 4` load in v0, where the
   original uses v1 - v1 is only dead there once the `flags & 2` return is its
   own block. Same body, same 276 bytes.
extern s32 D_800ADB98;
extern s32 D_800ADC0C;

int func_8008492C(ActorData* pActor) {
    if (pActor->curWalkmeshTriMaterial & 0x420000) {
        return -1;
    }
    if (D_800ADB98 != 0) {
        return -1;
    }
    if (pActor->moveModified.vx != 0) {
        return -1;
    }
    if (pActor->moveModified.vy != 0) {
        return -1;
    }
    if (pActor->moveModified.vz != 0) {
        return -1;
    }
    if (D_800ADC0C != 1) {
        return -1;
    }
    if (pActor->canInteract != 0xFF) {
        return -1;
    }
    if (pActor->scriptFlags.flags & 0x401800) {
        return -1;
    }
    if (pActor->flags & 1 && pActor->walkmeshId == 0) {
        return -1;
    }
    if (pActor->flags & 2 && pActor->walkmeshId == 1) {
        return -1;
    }
    if (pActor->flags & 4) {
        return -(pActor->walkmeshId == 2);
    }
    return 0;
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084A40);

extern s32 g_FieldArchiveStreamStatus;
extern s32 g_FieldArchiveStreamActive;
extern void* g_FieldArchiveStreamBlock;
extern void* g_FieldArchiveStreamCallback;
int func_80028B14(void);
int* ArchiveAllocStreamFile(int numEntries, int allocMode);
void ArchiveReadFileToBuffer(int fileIndex, void* pBuffer, int arg2, int size);

int FieldStepArchiveStream(void) {
    int result = func_80028B14();

    g_FieldArchiveStreamStatus = result;
    if (result != 0) {
        ((void (*)(int))g_FieldArchiveStreamCallback)(result);
        return 0;
    }
    if (ArchiveDataSync() != 0) {
        return 0;
    }
    if (g_FieldArchiveStreamStatus != 0) {
        return 0;
    }
    HeapFree(g_FieldArchiveStreamBlock);
    g_FieldArchiveStreamActive = 0;
    return -1;
}

void FieldStartArchiveStream(int archiveIndex, int allocMode, void* pCallback) {
    int* pStreamFile;

    g_FieldArchiveStreamActive = 1;
    pStreamFile = ArchiveAllocStreamFile(8, allocMode);
    g_FieldArchiveStreamBlock = pStreamFile;
    ArchiveReadFileToBuffer(archiveIndex, pStreamFile, 0, 0x100);
    g_FieldArchiveStreamCallback = pCallback;
}

void SoundStopElementsByIndex(int);
void SoundPlaySoundEffectOnPairWithLevels(void*, int, int, int);

void FieldPlaySoundElementPair(int arg0, int arg1, int arg2, int arg3) {
    int slot;
    int temp;

    temp = (u32)arg3 & 7;
    SoundStopElementsByIndex(temp * 2);
    slot = arg3 & 7;
    temp = (s16)arg1;
    SoundPlaySoundEffectOnPairWithLevels(arg0, slot * 2, temp, (s16)arg2);
}

extern s32 g_FieldSoundSlotId;

void FieldSetSoundSlotElement(int arg0, int arg1) {
    int offset = arg1 & 7;

    if (arg0 == 0) {
        SoundStopElementsByIndex(offset * 2);
    } else {
        g_FieldSoundSlotId = arg0;
        FieldPlaySoundElementPair(arg0, 0x7F, 0x40, offset);
    }
}

extern u_short g_FieldStreamFrameBase;
extern s32 g_FieldSedsEntryIndex;
extern s32 g_FieldStreamFramePosition;
extern s16 g_FieldStreamId;
extern SoundFile* g_FieldSedsFile;
void SoundPlaySoundEffectOnPair(s32 arg0, s32 arg1);

typedef struct {
    /* 0x0 */ u_short frameOffset;
    /* 0x2 */ u_short soundEffect;
} FieldUnk800AE060; // Size: 0x4

extern FieldUnk800AE060 g_FieldSedsEntries[];

void FieldAdvanceSedsSequence(void) {
    u_short* pDuration;
    u_short* pValue;
    u_short value;

    if (g_FieldStreamId == 0xFF) {
        return;
    }
    pDuration = &g_FieldSedsEntries[0].frameOffset;
    pValue = &g_FieldSedsEntries[0].soundEffect;
loop:
    if (g_FieldStreamFramePosition < pDuration[g_FieldSedsEntryIndex * 2] + g_FieldStreamFrameBase) {
        return;
    }
    value = pValue[g_FieldSedsEntryIndex * 2];
    SoundPlaySoundEffectOnPair((value & 0xFF) | (g_FieldSedsFile->sedId << 16), (value >> 7) & 0xE);
    g_FieldSedsEntryIndex++;
    goto loop;
}

void FieldStopSedsPlayback(void) {
    if (g_FieldStreamId != 0xFF) {
        SoundStopAllElements();
        SoundRemoveSedsEntry(g_FieldSedsFile);
        HeapFree(g_FieldSedsFile);
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80085788);
/* 93.00% (252 vs retail 264 bytes): the inner scan is byte-identical to retail
   (while (1) { if (pEntry->frameOffset == 0xFFFF) break; pEntry++; i++; }) and
   only the stream-id handling differs. Retail loads g_FieldStreamId ONCE into
   s0, tests it against 0xFF, then COPIES it (move s1,s0) and does every later
   use on the copy: addiu s0,s1,0x115 once for the archive file index and
   addiu v0,s1,0x1 rematerialised on BOTH sides of the loop (head blez and the
   back-edge slt). gcc here copy-propagates every s32 intermediate back onto the
   loaded s16, so the copy vanishes, streamId+0x115 moves into the first jal
   delay slot and streamId+1 is hoisted into s0 for the loop. Tried: s16+s32
   (streamIndex = streamId) locals, a separate s32 fileIndex local, (s32)/(u16)
   casts, the bound as streamId + 1 and as streamIndex + 1.
void func_80085788(void) {
    s16 streamId;
    s32 streamIndex;
    s32 fileIndex;
    s32 count;
    s32 i;
    FieldUnk800AE060* pEntry;
    SoundFile* pSoundFile;

    streamId = g_FieldStreamId;
    if (streamId == 0xFF) {
        return;
    }
    streamIndex = streamId;
    SoundStopAllElements();
    ArchiveSetIndex(ARCHIVE_DIR_FIELD_WDS, 0);
    fileIndex = streamIndex + 0x115;
    pSoundFile = HeapAlloc(ArchiveDecodeAlignedSize(fileIndex), 1);
    g_FieldSedsFile = pSoundFile;
    ArchiveReadFileToBuffer(fileIndex, pSoundFile, 0, 0x80);
    ArchiveCdDataSync(0);
    SoundAddSedsEntry(g_FieldSedsFile);
    SoundGetPendingSpuCommandType(0x10);
    ArchiveSetIndex(4, 0);
    i = 0;
    for (count = 0; count < streamIndex + 1; count++) {
        pEntry = &g_FieldSedsEntries[i];
        while (1) {
            if (pEntry->frameOffset == 0xFFFF) {
                break;
            }
            pEntry++;
            i++;
        }
        i++;
        g_FieldSedsEntryIndex = i;
    }
}
*/

extern void* g_MenuSedsFile;
extern s32 g_SoundSedsFileStaged;
extern void* g_SoundSedsFileBuffer;
void SoundGetPendingSpuCommandType(int arg0);
void SoundAddSedsEntry(SoundFile* pSoundFile);

void FieldLoadSedsFile(void) {
    int size;

    ArchiveSetIndex(4, 0);
    size = ArchiveDecodeAlignedSize(0xA8);
    g_MenuSedsFile = HeapAlloc(size, 0);
    HeapPinBlock(g_MenuSedsFile);
    if (g_SoundSedsFileStaged == -1) {
        ArchiveReadFileToBuffer(0xA8, g_MenuSedsFile, 0, 0x80);
        ArchiveCdDataSync(0);
    } else {
        memcpy(g_MenuSedsFile, g_SoundSedsFileBuffer, size);
        HeapUnpinBlock(g_SoundSedsFileBuffer);
        HeapFree(g_SoundSedsFileBuffer);
    }
    SoundAddSedsEntry(g_MenuSedsFile);
    SoundGetPendingSpuCommandType(0x10);
    ArchiveSetIndex(4, 0);
    g_SoundSedsFileStaged = -1;
}

void FieldFreeSedsFile(void) {
    SoundRemoveSedsEntry(g_MenuSedsFile);
    HeapUnpinBlock(g_MenuSedsFile);
    HeapFree(g_MenuSedsFile);
    g_SoundSedsFileStaged = -1;
}

// WDS files are streamed in 0x800-byte blocks; the header occupies the first four.
typedef struct {
    /* 0x0 */ u_long unk0[0x200];
} FieldWdsBlock; // Size: 0x800

extern FieldWdsBlock* g_FieldWdsBlocks;
extern s32 g_FieldWdsBlockIndex;
extern SoundWDSEntry* g_GameCurLoadedWDS;
extern s32 g_GameWdsBankLoadState;
extern s32 g_GameLoadedWdsStreamId;
extern s32 g_GameWdsStreamInProgress;
extern s32 g_FieldWdsBankLoadQueued;
extern u8 D_800ADFCD[];
extern void ArchiveCdDataSync(int mode);
extern void SoundFreeLoadedWdsAndAudioManager(void);
void ArchiveReleaseStreamBlock(void* pData);
void SoundGetPendingSpuCommandType(int arg0);
void SoundTransferWdsPart(void* pBuffer, int size);
SoundWDSEntry* SoundLoadPartialWdsFile(void* pBuffer, int size, int arg2);
void FieldStartArchiveStream(int archiveIndex, int unused, void* pCallback);

void FieldWdsStreamCallback(FieldWdsBlock* pBlock) {
    int bankIndex = g_FieldWdsBlockIndex;

    switch (bankIndex) {
        case 0:
        case 1:
        case 2:
        case 3:
            g_FieldWdsBlocks[bankIndex] = *pBlock;
            g_FieldWdsBlockIndex++;
            ArchiveReleaseStreamBlock(pBlock);
            if (g_FieldWdsBlockIndex == 4) {
                g_GameCurLoadedWDS = SoundLoadPartialWdsFile(g_FieldWdsBlocks, 0x2000, 0);
            }
            break;
        case 4:
            SoundGetPendingSpuCommandType(0x10);
            g_FieldWdsBlocks[0] = *pBlock;
            SoundTransferWdsPart(g_FieldWdsBlocks, 0x800);
            ArchiveReleaseStreamBlock(pBlock);
            break;
    }
}

void FieldStartWdsBankLoad(int arg0) {
    u_short streamId;
    int index;

    ArchiveCdDataSync(0);
    SoundFreeLoadedWdsAndAudioManager();
    if (arg0 == 0xFF) {
        g_GameWdsBankLoadState = 0;
        return;
    }
    ArchiveSetIndex(ARCHIVE_DIR_FIELD_WDS, 0);
    if (D_800ADFCD[arg0 * 2] == 1) {
        FieldReleaseWdsEntry();
    }
    streamId = D_800ADFCD[arg0 * 2 - 1];
    if (streamId != 0xFF) {
        index = streamId * 2 + 0x13;
        if (g_GameLoadedWdsStreamId != streamId) {
            FieldStartArchiveStream(index, 1, (void*)FieldWdsStreamCallback);
            g_GameWdsStreamInProgress = 1;
            g_FieldWdsBlockIndex = 0;
            g_FieldWdsBlocks = HeapAlloc(0x2000, 1);
        }
    }
    ArchiveSetIndex(4, 0);
    g_GameWdsBankLoadState = -1;
    g_FieldWdsBankLoadQueued = 1;
}

int FieldRunArchiveStreamSteps(void) {
    int i;

    for (i = 0; i < 5; i++) {
        if (FieldStepArchiveStream() == -1) {
            return 0;
        }
    }
    return -1;
}

extern s32 g_GameWdsFileLoadState;
extern s32 g_GameLoadedWdsBankIndex;
extern s32 g_GameAudioFadeInPending;
extern s32 g_GameReusePendingAudioManager;
extern s32 g_GameWdsBankHeaderReadPending;
extern s32 g_GameAudioManagerRunning;
extern s32 g_GameAudioAndWdsLoaded;
extern s32 g_GameHasLoadedWDS;
extern u8 g_FieldWdsBankEntries[];
extern u8 g_SoundBankLoadBuffer[];
extern AudioManager* g_SoundAudioManager;
extern AudioManager* g_SoundPendingAudioManager;
void FieldBeginWdsFileLoad(void);
int FieldFinishWdsFileLoad(void);
int ArchiveDataSync(void);
void ArchiveReadFileToBuffer(int fileIndex, void* pBuffer, int arg2, int size);
AudioManager* SoundCreateAudioManagerOnHeap(void* pData);
void SoundPlayAudioManager(AudioManager* pManager, s32 arg1, s32 arg2);
void SoundSetAudioManagerVolumeWithFade(AudioManager* pManager, s32 arg1, s32 arg2);
void SoundReloadAudioManagerSamples(AudioManager* pManager, s32 arg1, s32 arg2);

int FieldUpdateWdsAndAudioLoad(s32 arg0) {
    s32 state = g_GameWdsStreamInProgress;

    if (state == 1) {
        if (FieldRunArchiveStreamSteps() == -1) {
            return -1;
        }
        SoundGetPendingSpuCommandType(0x10);
        HeapFree(g_FieldWdsBlocks);
        g_GameWdsStreamInProgress = 0;
        g_GameHasLoadedWDS = state;
        g_GameLoadedWdsStreamId = g_FieldWdsBankEntries[arg0 * 2];
    }
    if (D_800ADFCD[arg0 * 2] == 0) {
        if (g_GameWdsFileLoadState == 0) {
            FieldBeginWdsFileLoad();
            return -1;
        }
        if (g_GameWdsFileLoadState & 0x80) {
            if (FieldFinishWdsFileLoad() == -1) {
                return -1;
            }
        }
    }
    if (g_FieldWdsBankLoadQueued == 1) {
        if (g_GameLoadedWdsBankIndex != arg0) {
            ArchiveSetIndex(ARCHIVE_DIR_FIELD_WDS, 0);
            ArchiveReadFileToBuffer(arg0 * 2 + 0x14, g_SoundBankLoadBuffer, 0, 0x80);
            g_GameWdsBankHeaderReadPending = 1;
            ArchiveSetIndex(4, 0);
        }
        g_FieldWdsBankLoadQueued = 0;
        return -1;
    }
    if (ArchiveDataSync() != 0) {
        return -1;
    }
    if (g_GameWdsBankHeaderReadPending == 1) {
        if (g_GameReusePendingAudioManager == 0) {
            g_SoundAudioManager = SoundCreateAudioManagerOnHeap(g_SoundBankLoadBuffer);
            if (g_GameAudioFadeInPending == -1) {
                SoundPlayAudioManager(g_SoundAudioManager, 0x7F, 0);
            } else {
                SoundPlayAudioManager(g_SoundAudioManager, 0, 0);
                SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, 0, 0);
            }
        } else {
            g_SoundAudioManager = g_SoundPendingAudioManager;
            SoundReloadAudioManagerSamples(g_SoundPendingAudioManager, 0x7F, 0xF0);
            g_GameReusePendingAudioManager = 0;
            g_SoundPendingAudioManager = NULL;
        }
        g_GameWdsBankHeaderReadPending = 0;
        g_GameAudioManagerRunning = 1;
        g_GameLoadedWdsBankIndex = arg0;
    }
    g_GameAudioFadeInPending = -1;
    g_GameAudioAndWdsLoaded = 1;
    return 0;
}

void FieldFreePendingAudioManager(void) {
    if (g_SoundPendingAudioManager != NULL) {
        SoundStopAudioManager(g_SoundPendingAudioManager);
        SoundDestroyAudioManager(g_SoundPendingAudioManager);
        g_SoundPendingAudioManager = NULL;
    }
}

extern s32 g_GameWdsFileLoadState;
extern s32 g_GameWdsEntryFreed;
extern s16 g_GameWdsEntryReleased;
extern s32 g_SoundWdsEntryFile3;
extern SoundWDSEntry* g_FieldWdsFileEntry;
extern void* g_FieldWdsFileBuffer; // WDS File Buffer

int FieldFinishWdsFileLoad(void) {
    void* pWdsEntry;

    if (ArchiveDataSync()) {
        return -1;
    }
    
    pWdsEntry = SoundLoadWdsFile(g_FieldWdsFileBuffer, 0);
    g_FieldWdsFileEntry = pWdsEntry;
    g_SoundWdsEntryFile3 = pWdsEntry;
    SoundGetPendingSpuCommandType(0x10);
    HeapFree(g_FieldWdsFileBuffer);
    g_GameWdsFileLoadState = 1;
    g_GameWdsEntryReleased = 0;
    g_GameWdsEntryFreed = 0;
    return 0;
}

void FieldBeginWdsFileLoad(void) {
    void* pWdsFileBuffer;

    ArchiveSetIndex(ARCHIVE_DIR_FIELD_WDS, 0);
    pWdsFileBuffer = HeapAlloc(ArchiveDecodeAlignedSize(3), 1);
    g_FieldWdsFileBuffer = pWdsFileBuffer;
    ArchiveReadFileToBuffer(3, pWdsFileBuffer, 0, CdlModeSpeed);
    ArchiveSetIndex(4, 0);
    g_GameWdsFileLoadState = 0x80;
}

void FieldReleaseWdsEntry(void) {
    if (g_GameWdsEntryFreed == 0) {
        g_GameWdsEntryReleased = 1;
        SoundFreeWdsEntry(g_FieldWdsFileEntry);
        g_GameWdsEntryFreed = 1;
    }
    g_GameWdsFileLoadState = 0;
}


// Three 6-byte entries; splat names the second field's address D_800AFE8A.
typedef struct {
    /* 0x0 */ u_short unk0;
    /* 0x2 */ u_short unk2;
    /* 0x4 */ u_short unk4;
} FieldUnk800AFE88; // Size: 0x6

extern FieldUnk800AFE88 g_FieldActorSoundSlots[3];
extern FieldUnk800AFE88 D_800AFE8A[3];

extern s16 g_FieldSoundMaxDistance;

void FieldGetDistanceScaledValue(int arg0, int* pOut, int arg2) {
    int limit = g_FieldSoundMaxDistance;
    int factor;
    u_int diff;
    int scaled;

    if (limit < arg0) {
        arg0 = limit;
    }
    factor = (0x7F0000 / limit * arg0) >> 16;
    diff = (0x80 - factor) << 16;
    scaled = diff / 127;
    *pOut = (u_int)(scaled * arg2) >> 16;
}

extern void SoundSetElementVolumeByIndex(int, int);
extern void SoundSetElementPanByIndex(int, int);
void FieldActorWorldToScreenPosition(int, int*, int*);

void FieldUpdateActorSoundSlot(int arg0, int arg1, int arg2, int arg3, int actorIndex) {
    int i;
    int value;
    int screenX;
    int screenY;
    int scaled;

    for (i = 0; i < 3; i++) {
        if (g_FieldActorSoundSlots[i].unk0 == actorIndex) {
            FieldGetDistanceScaledValue(arg3, &value, arg1);
            FieldActorWorldToScreenPosition(actorIndex, &screenX, &screenY);
            if (screenX >= 0x141) {
                screenX = 0x13F;
            }
            if (screenX < 0) {
                screenX = 0;
            }
            scaled = screenX * 0x6666 >> 16;
            SoundSetElementVolumeByIndex(i * 2, value);
            SoundSetElementPanByIndex(i * 2, scaled);
        }
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", FieldActorWorldToScreenPosition);
/* 95.25%: everything matches but the base address (204 bytes, same size).
   The original materialises `&g_FieldActors` into v1 (`lui v1,%hi;
   addiu v1,v1,%lo`, offset 0, after the FieldScriptVMGetActorIndex call),
   takes the matrix from it with `addiu a0,v1,-0xAC` and loads the actor
   pointer with `lw a1,0(v1)`; gcc folds -0xAC into one %lo and emits a
   separate lui/lw for the pointer. The -0xAC is exact and identifies the
   original layout: &g_FieldActors (0x800AFB10) - 0xAC ==
   g_Scene.worldToScreenMatrix (0x800AF990 + 0xD4), so the matrix and the
   actor pointer were members of one struct in the original source. The anchor
   must stay the g_FieldActors symbol, so the fix is a parent struct that the
   symbol file can express without overlapping g_Scene. Holding the base in a
   local pointer, in a `u_int` local, or writing `&g_FieldActors - 0xAC`
   directly all give 95.25%: gcc const-propagates and never reuses the base
   register for both addresses.
   Re-tried: taking the matrix straight from g_Scene.worldToScreenMatrix (the
   address -0xAC resolves to) is also 95.25% and the same 204 bytes - ours then
   emits `lui a0,%hi(g_Scene); addiu a0,a0,0xD4` plus a separate
   `lui a1,%hi(g_FieldActors); lw a1,0(a1)` where retail has `lui v1,%hi
   (g_FieldActors); addiu v1,%lo; addiu a0,v1,-0xAC; lw a1,0(v1)`. So the
   instruction count is identical and only the base register differs: retail
   anchors BOTH accesses on &g_FieldActors, which needs the matrix at a NEGATIVE
   offset from that symbol (one parent struct with the matrix at 0x0 and the
   actor-pointer array at 0xAC). Any parent struct declared here would make gcc
   base on the parent symbol with `+0xAC`, i.e. the mirrored shape.
void FieldActorWorldToScreenPosition(int actorIndex, int* pScreenX, int* pScreenY) {
    SVECTOR vertex;
    MATRIX transformMatrix;
    long sxy;
    long p;
    long flag;

    FieldScriptVMGetActorIndex(1);
    CompMatrix(&g_Scene.worldToScreenMatrix, &g_FieldActors[actorIndex].transformMatrix, &transformMatrix);
    vertex.vx = 0;
    vertex.vy = 0;
    vertex.vz = 0;
    SetRotMatrix(&transformMatrix);
    SetTransMatrix(&transformMatrix);
    RotTransPers(&vertex, &sxy, &p, &flag);
    *pScreenY = ((short*)&sxy)[1];
    *pScreenX = ((short*)&sxy)[0];
}
*/

void FieldAssignActorSoundSlot(int arg0, int arg1, int arg2, int arg3, int actorIndex) {
    short value;
    int screenX;
    int screenY;
    int i;
    int scaled;

    for (i = 0; i < 3; i++) {
        if (D_800AFE8A[i].unk0 == 0xFFFF) {
            D_800AFE8A[i].unk0 = arg0;
            g_FieldActorSoundSlots[i].unk0 = actorIndex;
            FieldGetDistanceScaledValue(arg3, (int*)&value, arg1);
            FieldActorWorldToScreenPosition(actorIndex, &screenX, &screenY);
            if (screenX >= 0x141) {
                screenX = 0x13F;
            }
            if (screenX < 0) {
                screenX = 0;
            }
            scaled = screenX * 0x6666 >> 16;
            SoundStopElementsByIndex(i * 2);
            SoundPlaySoundEffectOnPairWithLevels((void*)arg0, (short)(i * 2), value, scaled);
            break;
        }
    }
}

void FieldReleaseActorSoundSlot(u_int arg0) {
    int i;

    for (i = 0; i < 3; i++) {
        if (g_FieldActorSoundSlots[i].unk0 == arg0) {
            SoundStopElementsByIndex(i * 2);
            D_800AFE8A[i].unk0 = 0xFFFF;
            g_FieldActorSoundSlots[i].unk0 = 0xFFFF;
            break;
        }
    }
}

int FieldFindActorSoundSlot(int arg0, u_int arg1) {
    int i;

    if (arg0 == -1) {
        return -1;
    }
    for (i = 0; i < 3; i++) {
        if (g_FieldActorSoundSlots[i].unk0 == arg1) {
            return i;
        }
    }
    return -1;
}

void FieldClearActorSoundSlots(void) {
    int i;

    for (i = 0; i < 3; i++) {
        g_FieldActorSoundSlots[i].unk0 = 0xFFFF;
        D_800AFE8A[i].unk0 = 0xFFFF;
    }
}

extern u_short g_FieldSoundElementMask;

void FieldResetActorSoundSlots(void) {
    u_short* pFlags;
    int i;

    for (i = 0; i < 3; i++) {
        D_800AFE8A[i].unk0 = 0xFFFF;
        g_FieldActorSoundSlots[i].unk0 = 0xFFFF;
    }
    i = 0;
    pFlags = &g_FieldSoundElementMask;
    for (; i < 4; i++) {
        if (!(*pFlags & 1)) {
            SoundStopElementsByIndex(i * 2);
        }
        *pFlags >>= 1;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80086590);
/* 85.55% (864 bytes vs 872): the struct and all five loops are right, and the
   whole first block (0x10D test, the three magnitude args, the slot pick and
   the distance compare) is instruction-for-instruction. What got it there:
   g_FieldActors is volatile, so the one pActorData load the original shares
   between the 0x10D test and the three position reads needs a loop-body local
   (ActorData* pActor = g_FieldActors[i].pActorData), while every access after
   the magnitude call must spell the global out inline to reproduce the
   original's per-store reloads. gcc then keeps pActor in a2, as retail does.
   Residuals: (1) the delta stores come out lhu+subu+sh, the original
   lh+subu+sh (gcc narrows the loads once only the low halfword is live).
   (2) gcc hoists the g_FieldActors reload above the actorIndex/distance
   stores; retail stores first. (3) the slot*8 and distance-slot base land in
   a1/a0 the other way round. (4) the last loop: retail duplicates the
   increments after the FieldUpdateActorSoundSlot call (addiu s0,s0,8 in the jal delay
   slot, addiu s1,s1,1 in the j delay slot) and keeps them again at the shared
   continue block; gcc merges them into one copy, 8 bytes short.
typedef struct {
    s32 distance[4]; // 0x00
    s32 unk10[4]; // 0x10
    s32 unk20[4]; // 0x20
    s32 inUse[4]; // 0x30
    s32 unk40[4]; // 0x40
    s32 actorIndex[4]; // 0x50
    s16 delta[3][4]; // 0x60
} FieldActorProximitySlots; // Size: 0x78

void func_80086590(VECTOR* pPosition) {
    FieldActorProximitySlots slots;
    s16* pDelta;
    s16* pPos = (s16*)pPosition;
    s16* pActorPos;
    s32 distance;
    s32 slot;
    int i;

    for (i = 0; i < 3; i++) {
        slots.distance[i] = 0xFFFF;
        slots.unk10[i] = -1;
        slots.unk40[i] = 0;
        slots.inUse[i] = 0;
        slots.actorIndex[i] = 0;
        slots.unk20[i] = 0;
    }

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        ActorData* pActor = g_FieldActors[i].pActorData;

        if (pActor->soundState != 0xFF) {
            pActorPos = (s16*)&pActor->position;
            distance = FieldGetVec3Magnitude(
                pPos[1] - pActorPos[1],
                pPos[3] - pActorPos[3],
                pPos[5] - pActorPos[5]);
            if (slots.distance[0] < slots.distance[1]) {
                slot = (slots.distance[1] < slots.distance[2]) ? 2 : 1;
            } else {
                slot = (slots.distance[0] < slots.distance[2]) ? 2 : 0;
            }
            if (distance < slots.distance[slot]) {
                slots.actorIndex[slot] = i;
                slots.distance[slot] = distance;
                slots.unk10[slot] = g_FieldActors[i].pActorData->soundId;
                slots.unk20[slot] = g_FieldActors[i].pActorData->soundVolume;
                slots.delta[slot][0] = pPos[1] - ((s16*)&g_FieldActors[i].pActorData->position)[1];
                slots.delta[slot][1] = pPos[3] - ((s16*)&g_FieldActors[i].pActorData->position)[3];
                slots.delta[slot][2] = pPos[5] - ((s16*)&g_FieldActors[i].pActorData->position)[5];
            }
        } else {
            pActor->soundState = 0xFF;
        }
    }

    for (i = 0; i < 3; i++) {
        slot = FieldFindActorSoundSlot(slots.unk10[i], slots.actorIndex[i]);
        if (slot != -1) {
            slots.inUse[slot] = 1;
            slots.unk40[i] = 1;
        }
    }

    for (i = 0; i < 3; i++) {
        if (slots.inUse[i] == 0) {
            if (D_800AFE8A[i].unk0 != 0xFFFF) {
                SoundStopElementsByIndex(i * 2);
                D_800AFE8A[i].unk0 = 0xFFFF;
                g_FieldActorSoundSlots[i].unk0 = 0xFFFF;
            }
        }
    }

    pDelta = slots.delta[0];
    for (i = 0; i < 3; i++, pDelta += 4) {
        if (slots.unk10[i] == -1) {
            continue;
        }
        if (slots.unk40[i] == 1) {
            FieldUpdateActorSoundSlot(slots.unk10[i], slots.unk20[i], pDelta[0], slots.distance[i],
                slots.actorIndex[i]);
        } else {
            FieldAssignActorSoundSlot(slots.unk10[i], slots.unk20[i], pDelta[0], slots.distance[i],
                slots.actorIndex[i]);
        }
    }
}
*/

extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern s16 g_FieldSoundProximitySource;
extern s32 g_PlayerActorIndex;

void FieldUpdateProximitySoundSlots(void) {
    switch (g_FieldSoundProximitySource) {
        case 0:
            func_80086590(&g_FieldActors[g_PlayerActorIndex].pActorData->position);
            return;
        case 1:
            func_80086590(&g_CameraEye);
            return;
        case 2:
            func_80086590(&g_CameraAt);
            return;
    }
}

void FieldScriptVM2Run(void) {
    char *script = (char *)g_FieldScriptVMCurScriptData;
    size_t vmIP = ++(g_FieldScriptVMCurActor->scriptInstructionPointer);

    ScriptVMHandler *handler = &g_FieldScriptVMHandlers2[script[vmIP]];
    (*handler)();
}

extern s16 g_FieldMotionSlotActorIds[];

extern s16 g_FieldMotionSlotPathStart[];
extern s16 D_800B22EA[];
extern s16 D_800B22EC[];
extern s16 g_FieldMotionSlotPathEnd[];
extern s16 D_800B2302[];
extern s16 D_800B2304[];
extern s32 g_FieldMotionSlotFrameCount[];
extern s16 g_FieldMotionSlotTargetPositions[];
extern s16 D_800B2326[];
extern s16 D_800B2328[];
long FieldGetVec3Magnitude(long x, long y, long z);

void FieldUpdateMotionSlotSteps(int index, VECTOR* pPosition) {
    s16* pPos = (s16*)pPosition;
    s32 stepX = ((g_FieldMotionSlotPathStart[index * 4] - g_FieldMotionSlotPathEnd[index * 4]) << 16) / g_FieldMotionSlotFrameCount[index];
    s32 stepY = ((D_800B22EA[index * 4] - D_800B2302[index * 4]) << 16) / g_FieldMotionSlotFrameCount[index];
    s32 stepZ = ((D_800B22EC[index * 4] - D_800B2304[index * 4]) << 16) / g_FieldMotionSlotFrameCount[index];
    s32 remaining;

    remaining = g_FieldMotionSlotFrameCount[index] - FieldGetVec3Magnitude(
        g_FieldMotionSlotTargetPositions[index * 4] - pPos[1],
        D_800B2326[index * 4] - pPos[3],
        D_800B2328[index * 4] - pPos[5]);

    ((s16*)g_FieldMotionSlotFrameCount)[index - 0x6E] = (stepX * remaining) >> 16;
    ((s16*)g_FieldMotionSlotFrameCount)[index - 0x6B] = (stepY * remaining) >> 16;
    ((s16*)g_FieldMotionSlotFrameCount)[index - 0x68] = (stepZ * remaining) >> 16;
}

void FieldUpdateMotionSlots(void) {
    int i;

    for (i = 0; i < 3; i++) {
        if (g_FieldMotionSlotActorIds[i] != -1) {
            FieldUpdateMotionSlotSteps(i, &g_FieldActors[g_FieldMotionSlotActorIds[i]].pActorData->position);
        }
    }
}

extern s32 g_FieldCurActorIndex;
void FieldInitializeDefaultParticleBanks(int targetActorID);
int FieldInitializeParticleBanks(int actorIndex);

void FieldScriptConfigureDefaultParticleBank(void) {
    int arg0;
    int arg1;
    int arg2;

    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
        case 1:
            arg0 = FieldScriptVMGetArgument(2);
            arg1 = FieldScriptVMGetArgument(4);
            arg2 = FieldScriptVMGetArgument(6);
            FieldInitializeDefaultParticleBanks(g_FieldCurActorIndex);
            g_FieldDefaultParticleBanks[0].flags = 0x14;
            g_FieldDefaultParticleBanks[0].unk0 = 0x1;
            g_FieldDefaultParticleBanks[0].max = 0x10;
            g_FieldDefaultParticleBanks[0].unk72 = 0x0;
            g_FieldDefaultParticleBanks[0].unk74 = arg1;
            g_FieldDefaultParticleBanks[0].unk74 = (arg1 == 0x27) ? 0x20 : 0x22;
            g_FieldDefaultParticleBanks[0].ewait = 0x1000;
            FieldInitializeParticleBanks(g_FieldCurActorIndex);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
            break;
    }
}

void GameSoftReset(void);

void FieldScriptSoftReset(void) {
    GameSoftReset();
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
