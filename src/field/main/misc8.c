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

extern s16 g_FieldWalkmeshTriCount;
extern s32 D_800AFB44[];
extern int func_8007B1C4(int x, int z, int walkmeshId, SVECTOR* pOut, VECTOR* pNormal);
extern int rand(void);

/* 82.88% (1228 bytes against retail's 1232, 307 insns against 308; frame 0xA0
   and the saved set s0-s6+ra are retail's exactly). Round 428 raised this from
   77.88 by ONE edit: the four tail reads of g_FieldActors[actorIndex].transformMatrix
   (position.vx / .vy / .vz / curYPos) each get their own `do { <read> = ...; } while (0)`
   block. Before that gcc pipelined each statement's base load into the previous
   statement's load-delay slot, so every chain lost its two `nop`s and the body was
   8 insns short; inside a block cc1 cannot hoist the next base, and all four chains
   come out retail's serial lui/lw/nop/addu/lw/nop/sll/sw with the byte count right.
   THE SAME RECIPE DID NOT TRANSFER: wrapping the curTriNormal triple instead costs
   2 insns (1240 bytes, 82.80 - it frees a0 for the stride but re-reads walkmeshId
   one extra time), and wrapping the flags12C/flags130 block drops to 70.81
   (the hoisted mask constants lose their registers). Measured at 77.88 and still
   dead here: per-site `pFieldActor = &g_FieldActors[actorIndex];` pointer locals
   (76.89, same 1200), a reused s32 local via a comma expression (77.40).
   RESIDUE at 82.88 (all measured, none source-controllable by hand so far):
   (a) one instruction / 4 bytes short. (b) The two hoisted flag masks land swapped:
   retail a2 = 0xFFFCFFFF and a1 = 0xF007FFFF, ours a1/a2 (same for t6/t7 =
   0x3C0000 / 0xFF); the same source order produces both, so it is the allocator's
   numbering, not the statement order. (c) retail's flags12C clear / reload / second
   clear block (`lw a0,0x12C / and a0,a0,a2 / and v1,a1 / and v1,v0 / sw a0 /
   lw a0,0x12C / and v1,a3 / sw v1,0x130 / and a0,t0 / sw a0,0x12C`) against ours,
   which re-stores the same register once. (d) the zero group
   pActor->positionOffset stores 0x64, 0x60, 0x62 in retail (reordering the source
   to vz,vx,vy measures 82.89 - noise, so it is reorg stealing, not source order).
   (e) the walkmesh loop keeps retail's two walking pointers only at 75.14 (aggregate
   struct) as recorded before this round.
   Permuter: four runs total including this round's 4-minute run from the 82.88 body
   (base 3818 -> 3413, PERMUTER_BETTER); its find moved dialogPixelHeight's store up
   with an embedded assignment and scored 82.29 in objdiff, so it was not ported.
*//*
void func_80080A74(int actorIndex) {
    ActorData* pActor;
    FieldActor* pFieldActor;
    VECTOR normals[4];
    SVECTOR points[4];
    s32 i;
    s16 tri;

    pActor = g_FieldActors[actorIndex].pActorData;

    pActor->scriptFlags.flags = 0xB0;
    pActor->flags = 0x800;
    pActor->width = 0x10;
    pActor->zWidth = 0x10;
    pActor->height = 0x60;
    pActor->canInteract = 0xFF;
    pActor->parentActorId = 0xFF;

    pActor->move.vx = 0;
    pActor->move.vy = 0;
    pActor->move.vz = 0;
    pActor->moveModified.vx = 0;
    pActor->moveModified.vy = 0;
    pActor->moveModified.vz = 0;
    pActor->positionOffset.vx = 0;
    pActor->positionOffset.vy = 0;
    pActor->positionOffset.vz = 0;
    pActor->moveStep.vx = 0;
    pActor->moveStep.vy = 0;
    pActor->moveStep.vz = 0;
    pActor->defaultAnimationId = 0;
    pActor->animationOverride = 0xFF;
    pActor->curDoorStep = 0;
    pActor->scriptInstructionPointer = 0;
    pActor->stuckFrameCount = 0;

    pActor->flags12C_0x5 = 0;
    pActor->updateAngle = 0x200;
    pActor->solidRange = pActor->width;
    pActor->flags12C_0 = 0;
    pActor->blue2 = 0x80;
    pActor->green2 = 0x80;
    pActor->red2 = 0x80;
    pActor->blue = 0x80;
    pActor->green = 0x80;
    pActor->red = 0x80;
    pActor->modelAnimation = -1;

    pActor->flags12C_0x10 = 0;
    pActor->flags130 &= ~0xFF80000;
    pActor->flags130 &= ~0x1FF;
    pActor->flags130 &= ~0x7FE00;
    pActor->flags12C_0x12 = 0;

    for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        pActor->scripts[i].currentIP = 0xFFFF;
        pActor->scripts[i].waitTimer = 0;
        pActor->scripts[i].scriptId = 0xFF;
        pActor->scripts[i].flags_0 = 0xFFFF;
        pActor->scripts[i].state = 0;
        pActor->scripts[i].isInUse = 0;
        pActor->scripts[i].flags_0x17 = 0;
        pActor->scripts[i].flags_0x12 = 0xF;
    }

    pActor->pSpecialAnimFile = NULL;
    pActor->characterId = 0xFF;
    pActor->moveSpeed = 0x100;
    pActor->flags12C_0x6 = 0;
    pActor->flags134 &= ~0x80;
    pActor->dialogHeight = 0;
    pActor->dialogWidth = 0;
    pActor->dialogPixelHeight = 0;
    pActor->dialogPixelWidth = 0;
    pActor->dialogFlags = 0;
    pActor->waitScriptIndex = 0;
    pActor->curScriptIndex = 0;
    pActor->curAnimationId = 0;
    pActor->walkmeshId = 0;
    pActor->scriptYPos = 0;
    pActor->direction = 0;
    pActor->flags12C_0xD = 0;
    pActor->flags134 &= ~0x60;

    pActor->frameCounter = rand();
    pActor->scaleX = 0x1000;
    pActor->scaleY = 0x1000;
    pActor->scaleZ = 0x1000;
    pActor->soundState = 0xFF;
    pActor->faceId = 0xFF;
    pActor->rotation.vx = -0x8000;
    pActor->rotation.vy = -0x8000;
    pActor->rotation.vz = -0x8000;
    pActor->specialAnimArchiveIndex = -1;
    pActor->flags12C_0x2 = 0;
    pActor->hitTimer = 0;
    pActor->walkmeshTriIds[3] = 0;
    pActor->walkmeshTriIds[2] = 0;
    pActor->walkmeshTriIds[1] = 0;
    pActor->walkmeshTriIds[0] = 0;

    for (i = 0; i < g_FieldWalkmeshTriCount - 1; i++) {
        pFieldActor = &g_FieldActors[actorIndex];
        tri = func_8007B1C4((s16)pFieldActor->transformMatrix.t[0], (s16)pFieldActor->transformMatrix.t[2],
                            i, &points[i], &normals[i]);
        pActor->walkmeshTriIds[i] = tri;
        if (tri != -1 && (u32)tri >= D_800AFB44[i]) {
            D_800AFB44[i] = 0;
            normals[i].vx = 0;
            do {
                normals[i].vy = 0;
                normals[i].vz = 0;
            } while (0);
            points[i].vx = 0;
            points[i].vy = 0;
            points[i].vz = 0;
        }
    }

    pActor->curWalkmeshTriMaterial = FieldActorGetWalkmeshTriMaterial(pActor);
    pActor->curTriNormal.vx = normals[pActor->walkmeshId].vx;
    pActor->curTriNormal.vy = normals[pActor->walkmeshId].vy;
    pActor->curTriNormal.vz = normals[pActor->walkmeshId].vz;
    pFieldActor = &g_FieldActors[actorIndex];
    if (!(pFieldActor->status & 0x80)) {
        pFieldActor->transformMatrix.t[1] = points[pActor->walkmeshId].vy;
    }
    do {
        pActor->position.vx = g_FieldActors[actorIndex].transformMatrix.t[0] << 16;
    } while (0);
    do {
        pActor->position.vy = g_FieldActors[actorIndex].transformMatrix.t[1] << 16;
    } while (0);
    do {
        pActor->position.vz = g_FieldActors[actorIndex].transformMatrix.t[2] << 16;
    } while (0);
    do {
        pActor->curYPos = g_FieldActors[actorIndex].transformMatrix.t[1];
    } while (0);
}*/



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

// func_8008110C owns this unit's five debug strings in .rodata (EVENT CODE,
// MOV CHECK0-3); converting it needs them defined here as const char[0xC].
extern s32 D_800C3910;
extern s32 g_FieldActorModelPointerIndex;
extern s32 D_800ADC0C;
extern u_char g_FieldPartyMoveInProgress;
extern s32 g_PlayerActorIndex;
void func_800A2030(void);
void func_80281B00(const char* pName);
// the five strings this stub owns in .rodata (0x11C, misc8's range); the body
// below needs them declared, and converting the stub means defining them here
// as const char[0xC] or the unit's .rodata range comes up 0x54 bytes short.
extern const char D_8006FC0C[];
extern const char D_8006FC18[];
extern const char D_8006FC24[];
extern const char D_8006FC30[];
extern const char D_8006FC3C[];
void func_80082620(int actorIndex, FieldActor* pFieldActor, ActorData* pActor);
void func_80082BB8(int actorIndex, FieldActor* pFieldActor, ActorData* pActor);
void func_80084158(int actorIndex, FieldActor* pFieldActor, ActorData* pActor);
void func_80084A40(int actorIndex, int y, FieldActor* pFieldActor, ActorData* pActor, int arg4);
void func_8008399C(int actorIndex, FieldActor* pFieldActor, ActorData* pActor);
void func_800815F0(void);
void FieldSetGearModelAnimationForActor(int arg0, int actorId, FieldActor* pActor);
void SpritePlayAnimation(int, int);

/* 98.13% (315 insns / 1260 bytes vs retail 313 / 1252: two instructions too
   many). Everything structural matches - every branch, every call, both extra
   loops, the five debug strings and the D_800ADC0C tail (retail materialises
   the 1 in each of the three predecessors; a plain 'D_800ADC0C = 1;' does that
   by itself). What is left is register assignment and two schedules:
   (1) first loop: retail keeps the count in a1 and the byte-offset biv in a0 and
       the actor-data pointer in v1; this body gets count a0 / biv v1 / actor v0-s0.
   The THREE one-iteration do-while blocks there are load-bearing: without them
       cc1 hoists the next statement's g_FieldActors load above the previous
       lh/sh and the loop loses four load-delay nops. With them the loop is
       instruction-for-instruction retail's (89.82 -> 93.18 just for the blocks,
       and writing the copy as CONV_TO_GTE(x) - not a (s16*) cast - is what
       makes the load 'lh' instead of 'lhu').
   (2) arm 2 (flags & 0x1000000): the two volatile stores are load-bearing (they
       keep the store ahead of the reload of g_FieldActors), the do-while is
       there to keep the SpritePlayAnimation argument setup out of the pData
       group; one extra nop remains after the sprite base load.
   (3) arm 3 (flags & 0x200000): 'addu a2,s3,a2' (retail) vs 'addu a2,a2,s3'.
   (4) func_8008399C's two g_FieldActors loads come out in the other order
       (argument evaluation vs register priority).
   Tried: the plain three statements, one do-while around all three, three
   distinct pointer locals, the count as its own statement vs in the for-init,
   the actor chain inline vs in a local at each site. Permuter: 1673 iterations,
   base 751 -> best 591, but the candidate moves SpritePlayAnimation out of the
   curAnimationId != animationOverride test, which retail does not do (its beq
   jumps past the jal) - rejected as wrong code.
*/
/* PARKED R419 98.13% (315 insns / 1260 bytes vs retail 313 / 1252: two nops that
   retail fills with an instruction it placed earlier). Both excess sites are +4
   bytes, so every branch offset carries the +8 - that is why 40 of the 50 diff
   hunks are offsets, not code. The real residuals, in diff order:
   (1) loop 1 registers: retail count a1 / biv a0 / actor-data v1, ours a0 / v1 / s0
       (same instruction shapes, pure allocation tie; the count local and the
       for-init count were both measured).
   (2) arm 2: retail's `lh a1,234(s0)` sits in the load-delay slot of the
       `lw v0,0(v0)` that reloads g_FieldActors; ours loads it after
       `addu v0,s3,v0` / `lw a0,4(v0)`, so that slot is a nop.
   (3) arm 3: `addu a2,s3,a2` (retail, offset-first) vs ours `addu a2,a2,s3`; the
       a0/a1 pair of the store is mirrored too.
   (4) third loop: retail forms the walker in a temp (`addu a1,s3,v0`,
       offset-first), reads status through it and copies `move s1,a1` into the
       beqz slot; ours forms it in place (`addu s1,v0,s3`, base-first) and its
       slot is a nop.
   (5) func_8008399C: its two g_FieldActors loads come out in the other order.
   R419 measured: (3) written as plain subexpressions (`g_FieldActors[i].pSpriteData,
       g_FieldActors[i].pActorData->animationOverride, &g_FieldActors[i]`) is
       96.35 / 319 - cc1 re-forms the table address per argument instead of CSEing
       it, so the offset-first `addu` is not reachable that way; (4) written as the
       assignment embedded in the condition
       (`if (!((pFieldActor = &g_FieldActors[i])->status & 0xF00))`) is unchanged
       98.13 / 315 - cc1 still coalesces the temp into the walker. Prior rounds:
       splitting the status read onto `g_FieldActors[i]` (96.04 / 322), `while`
       for the first loop (96.22 / 319), three pointer locals, the plain three
       statements, one do-while around all three, permuter (best 591 but it moved
       SpritePlayAnimation out of the curAnimationId test - wrong code).
   Both excess instructions are scheduler slots (a load-delay nop retail fills
   with an independent load), so this is not a source-shape job.
   Converting it needs the five debug strings below defined in C in the same
   commit; they are at the head of the unit rodata range, so plain `const char`
   definitions in this file keep retail layout, but it is moot until the body is
   exact. */
/*
void func_8008110C(void) {
    FieldActor* pFieldActor;
    FieldActor* pCurActor;
    ActorData* pActor;
    ActorData* pData;
    SpriteData* pSprite;
    int fieldActorCount;
    int i;

    D_800C3910 = -1;
    func_800A2030();
    fieldActorCount = g_FieldActiveActorCount;
    for (i = 0; i < fieldActorCount; i++) {
        do {
            pActor = g_FieldActors[i].pActorData;
            pActor->prevPosition.x = CONV_TO_GTE(pActor->position.vx);
        } while (0);
        do {
            pActor = g_FieldActors[i].pActorData;
            pActor->prevPosition.y = CONV_TO_GTE(pActor->position.vy);
        } while (0);
        do {
            pActor = g_FieldActors[i].pActorData;
            pActor->prevPosition.z = CONV_TO_GTE(pActor->position.vz);
        } while (0);
    }
    if (g_FieldSystemMode == 0) {
        func_80281B00(D_8006FC0C);
    }
    g_FieldActorModelPointerIndex = 0;
    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pFieldActor = &g_FieldActors[i];
        pActor = pFieldActor->pActorData;
        if ((pFieldActor->status & 0xF80) != 0x200) {
            continue;
        }
        if (!(pActor->scriptFlags.flags & 0x10001)) {
            if ((pActor->flags & 0x600) != 0x200) {
                pActor->curWalkmeshTriMaterial = FieldActorGetWalkmeshTriMaterial(pActor);
                func_80082620(i, pFieldActor, pActor);
                func_80082BB8(i, pFieldActor, pActor);
            }
            continue;
        }
        if (pActor->flags & 0x1000000) {
            if (!(pActor->scriptFlags.flags & 0x10000)) {
                if (pActor->curAnimationId != pActor->animationOverride) {
                    do {
                        // volatile: the stores have to stay ahead of the reloads of g_FieldActors below
                        *(volatile s16*)&pActor->animationOverride = 2;
                        pData = g_FieldActors[i].pActorData;
                        pData->curAnimationId = pData->animationOverride;
                    } while (0);
                    SpritePlayAnimation(g_FieldActors[i].pSpriteData, pActor->animationOverride);
                }
                continue;
            }
        }
        if (pActor->flags & 0x200000) {
            pData = g_FieldActors[i].pActorData;
            if (pData->curAnimationId != pData->animationOverride) {
                *(volatile s16*)&pData->curAnimationId = pData->animationOverride;
                pCurActor = &g_FieldActors[i];
                FieldSetGearModelAnimationForActor(pCurActor->pSpriteData,
                    pCurActor->pActorData->animationOverride, pCurActor);
            }
        }
    }
    if (g_FieldSystemMode == 0) {
        func_80281B00(D_8006FC18);
    }
    pFieldActor = &g_FieldActors[g_PlayerActorIndex];
    func_80084158(g_PlayerActorIndex, pFieldActor, pFieldActor->pActorData);
    if (g_FieldSystemMode == 0) {
        func_80281B00(D_8006FC24);
    }
    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pFieldActor = &g_FieldActors[i];
        if (!(pFieldActor->status & 0xF00)) {
            continue;
        }
        pActor = pFieldActor->pActorData;
        if ((pActor->flags & 0x600) == 0x200) {
            continue;
        }
        if ((pFieldActor->status & 0xF80) != 0x200) {
            continue;
        }
        if (pActor->scriptFlags.flags & 0x10001) {
            continue;
        }
        if (i == g_PlayerActorIndex) {
            continue;
        }
        func_80084A40(i, 0x7FFFFFFF, pFieldActor, pActor, 0);
        pSprite = g_FieldActors[i].pSpriteData;
        if (*(u_short*)&((SpriteDataI3*)pSprite->field_0x7C)->field_0xC == 1) {
            pActor->scriptFlags.flags &= ~0x800;
        }
    }
    if (g_FieldSystemMode == 0) {
        func_80281B00(D_8006FC30);
    }
    if (g_FieldControl.isRandomEncountersEnabled == 0) {
        if (g_FieldPartyMoveInProgress == 0) {
            func_8008399C(g_PlayerActorIndex, &g_FieldActors[g_PlayerActorIndex],
                g_FieldActors[g_PlayerActorIndex].pActorData);
        }
    }
    D_800ADC0C = 1;
    func_800815F0();
    if (g_FieldSystemMode == 0) {
        func_80281B00(D_8006FC3C);
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008110C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800815F0);

typedef struct {
    /* 0x00 */ u_int scriptFlags;
    /* 0x04 */ u_int flags;
    /* 0x08 */ s16 positionX;
    /* 0x0A */ s16 positionY;
    /* 0x0C */ s16 positionZ;
    /* 0x0E */ s16 unk0E;
    /* 0x10 */ u_short spriteField84;
    /* 0x12 */ s16 animationId;
    /* 0x14 */ s16 rotation;
    /* 0x16 */ u_short walkmeshTriIds[4];
    /* 0x1E */ u_short unk1E;
    /* 0x20 */ IVEC3 step;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ IVEC3 normal;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ u_int walkmeshTriMaterial;
    /* 0x44 */ u8 walkmeshId;
    /* 0x45 */ undefined8 pad45[3];
} FieldActorRenderPacket; // Size: 0x48

extern FieldActorRenderPacket D_800B14F0[];
extern volatile s32 g_FieldSpriteUpdateIndex[];
extern s32 D_800C3910;
extern u_char g_FieldPartyMoveInProgress;
extern s32 g_PlayerActorIndex;
int FieldCharacterIdToPartyId(int characterId);
void func_80081F80(SpriteData*, s16, FieldActor*);

// This unit only tests the party slot against zero, and the original loads it signed.
typedef struct {
    /* 0x0 */ s16 value;
} FieldPlayerActorPartySlotView;

extern FieldPlayerActorPartySlotView g_FieldPlayerActorPartySlot;

/* R430 redo: 80.80% (ours 1672 bytes / 418 insns; retail 1636 / 409). Body written
   fresh from the .s this round; the previous note had no body at all. STRUCTURE IS
   SETTLED and every branch of the original is present: the two halves selected by
   g_FieldPlayerActorPartySlot (s16, loaded with 'lh', hence the s16 view below), the
   common guards (scriptFlags & 0x1000000, i == g_PlayerActorIndex, status & 0x20),
   FieldCharacterIdToPartyId == -1, the 0x48-byte ring at D_800B14F0 indexed by the
   per-party cursor g_FieldSpriteUpdateIndex[partyId] (18 volatile index reads: 17
   ring accesses plus the trailing (x-1) & 0x1F), the moveInProgress == 1 / (ring
   flags & 0x800) / curWalkmeshTriMaterial & 0x420000 / D_800C3910 == -1 gating of
   the animation reset, the second animation sync from ring.animationId, the
   walkmeshTriIds[4] copy loop, the flags 0x800 set/clear, and the tail that writes
   transformMatrix.t[] then <<16 into sprite+actor position.
   RESIDUALS, biggest first (all measured, none closed):
   (1) retail re-forms '&g_FieldSpriteUpdateIndex[partyId]' at EVERY ring access
       (sll v0,s2,2 / addu v0,v0,s6 / lw v1,0(v0) = 3 insns) where our
       'volatile s32* pUpdateIndex' local keeps the pointer in s1 (1 insn) - about
       36 instructions we do not spend, which is why the +45/-36 mismatch shows up
       as 9 excess instructions overall and every branch offset is off by 4-8.
   (2) retail hoists the two constant bases D_800B1520 (normal) and D_800B1510
       (step) into s8/t1 at the loop preheader and then uses 'addu v0,v0,s8' +
       'lw 0/4/8(v0)'; we emit 'lui at,%hi(sym) / addu at,at,v0 / lw %lo(sym)(at)'
       per store (6 stores). gcc's loop-invariant constant choice, not source.
   (3) retail's pFieldActor/pActorData live in a2/a0 through the guards and are
       COPIED into s4/s0 for the body ('move s4,a2' / 'move s0,a0' in the bnez
       delay slot), so each loop is 2 instructions longer than ours, where gcc
       colours them call-saved from birth. A local declared at the top of the loop
       gives s2 (75.87); one declared after the guards gives 79.28.
   (4) 'pSpriteData' is loaded before the animation-id guards in retail
       ('lw s3,4(s4)') and reused at the call; gcc sinks our local to the call.
   (5) the ring scriptFlags mask is carried in a2 across the whole middle in retail
       ('andi v0,a2,0x800' in two delay slots); we reload it per use.
   Measured variations this round: guards written as 'g_FieldActors[i].field'
   expressions 75.04 (3 volatile reloads of g_FieldActors per guard); pFieldActor
   local from the top of the loop 75.87; locals assigned after the guards 79.28;
   '&g_FieldActors[i]' -> offset-first '(FieldActor*)(i * sizeof(FieldActor) +
   (int)g_FieldActors)' 79.28 -> 80.33 (retail's 'addu a2,s1,v0' operand order);
   adding the ring.animationId sync block I had missed and making the ring's
   animationId/rotation s16 so they load 'lh' -> 80.80.
   FOR THE NEXT AGENT: drop the pUpdateIndex local and write the index expression
   inline (residual 1) first - it is ~36 instructions of shape, and it also makes
   the flags/anim blocks sit at retail's offsets. Excerpts of the ring struct usage
   are above; the FieldActorRenderPacket typedef, the s16 cast on 0x10/0x12/0x14 and
   the FieldPlayerActorPartySlotView decl in the stub's preamble are all needed.
*/
/*
void func_800815F0(void) {
    FieldActor* pFieldActor;
    ActorData* pActorData;
    SpriteData* pSpriteData;
    volatile s32* pUpdateIndex;
    u_int ringFlags;
    int partyId;
    int i;
    int j;

    if (g_FieldPlayerActorPartySlot.value != 0) {
        for (i = 0; i < g_FieldActiveActorCount; i++) {
            pFieldActor = (FieldActor*)(i * sizeof(FieldActor) + (int)g_FieldActors);
            if (!(pFieldActor->pActorData->scriptFlags.flags & 0x1000000)) {
                continue;
            }
            if (i == g_PlayerActorIndex) {
                continue;
            }
            if (pFieldActor->status & ACTOR_STATUS_INVISIBLE) {
                continue;
            }
            pActorData = pFieldActor->pActorData;
            pSpriteData = pFieldActor->pSpriteData;
            if (pActorData->curAnimationId != pActorData->defaultAnimationId) {
                pActorData->curAnimationId = pActorData->defaultAnimationId;
                if (pActorData->curAnimationId < 0) {
                    pActorData->curAnimationId = 0;
                }
                FieldSetGearModelAnimationForActor(pSpriteData, pActorData->curAnimationId, pFieldActor);
            }
        }
    } else {
        for (i = 0; i < g_FieldActiveActorCount; i++) {
            pFieldActor = (FieldActor*)(i * sizeof(FieldActor) + (int)g_FieldActors);
            if (!(pFieldActor->pActorData->scriptFlags.flags & 0x1000000)) {
                continue;
            }
            if (i == g_PlayerActorIndex) {
                continue;
            }
            if (pFieldActor->status & ACTOR_STATUS_INVISIBLE) {
                continue;
            }
            pActorData = pFieldActor->pActorData;
            pSpriteData = pFieldActor->pSpriteData;
            partyId = FieldCharacterIdToPartyId(pActorData->characterId);
            if (partyId == -1) {
                continue;
            }
            pUpdateIndex = &g_FieldSpriteUpdateIndex[partyId];
            func_80081F80(pSpriteData, D_800B14F0[pUpdateIndex[0]].rotation, pFieldActor);
            ringFlags = D_800B14F0[pUpdateIndex[0]].scriptFlags;
            if (g_FieldPartyMoveInProgress == 1) {
                g_FieldSpriteUpdateIndex[0] = (g_FieldSpriteUpdateIndex[0] + 1) & 0x1F;
            } else if (!(ringFlags & 0x800)) {
                pActorData->flags &= ~0x1000;
                if (!(pActorData->curWalkmeshTriMaterial & 0x420000)) {
                    if (D_800C3910 == -1) {
                        if (pSpriteData->field_0x84 == ((s16*)&pActorData->position)[3]) {
                            if (pActorData->curAnimationId == 6) {
                                pActorData->flags |= 0x1000;
                                continue;
                            }
                            if (pActorData->curAnimationId != pActorData->defaultAnimationId) {
                                pActorData->curAnimationId = pActorData->defaultAnimationId;
                                if (pActorData->curAnimationId < 0) {
                                    pActorData->curAnimationId = 0;
                                }
                                FieldSetGearModelAnimationForActor(pSpriteData, pActorData->curAnimationId,
                                    pFieldActor);
                            }
                            continue;
                        }
                    } else {
                        int offset;

                        if (partyId == 1) {
                            offset = 10;
                        } else {
                            offset = 20;
                        }
                        if (((g_FieldSpriteUpdateIndex[0] + offset) & 0x1F) != pUpdateIndex[0]) {
                            continue;
                        }
                    }
                }
            }
            if (pUpdateIndex[0] == g_FieldSpriteUpdateIndex[0]) {
                pActorData->scriptFlags.flags &= ~0x800;
                pActorData->curAnimationId = pActorData->defaultAnimationId;
                if (pActorData->curAnimationId < 0) {
                    pActorData->curAnimationId = 0;
                }
                FieldSetGearModelAnimationForActor(pSpriteData, pActorData->curAnimationId, pFieldActor);
                continue;
            }
            if (ringFlags & 0x800) {
                pActorData->scriptFlags.flags |= 0x800;
            } else {
                pActorData->scriptFlags.flags &= ~0x800;
            }
            if (D_800B14F0[pUpdateIndex[0]].animationId != pActorData->curAnimationId) {
                s16 animId = D_800B14F0[pUpdateIndex[0]].animationId;

                pActorData->curAnimationId = animId;
                if (animId < 0) {
                    pActorData->curAnimationId = 0;
                }
                FieldSetGearModelAnimationForActor(pSpriteData, pActorData->curAnimationId, pFieldActor);
            }
            for (j = 0; j < 4; j++) {
                pActorData->walkmeshTriIds[j] = D_800B14F0[pUpdateIndex[0]].walkmeshTriIds[j];
            }
            pActorData->walkmeshId = D_800B14F0[pUpdateIndex[0]].walkmeshId;
            pActorData->curTriNormal.vx = D_800B14F0[pUpdateIndex[0]].normal.x;
            pActorData->curTriNormal.vy = D_800B14F0[pUpdateIndex[0]].normal.y;
            pActorData->curTriNormal.vz = D_800B14F0[pUpdateIndex[0]].normal.z;
            pSpriteData->step.x = D_800B14F0[pUpdateIndex[0]].step.x;
            pSpriteData->step.y = D_800B14F0[pUpdateIndex[0]].step.y;
            pSpriteData->step.z = D_800B14F0[pUpdateIndex[0]].step.z;
            pFieldActor->transformMatrix.t[0] = D_800B14F0[pUpdateIndex[0]].positionX;
            pFieldActor->transformMatrix.t[1] = D_800B14F0[pUpdateIndex[0]].positionY;
            pFieldActor->transformMatrix.t[2] = D_800B14F0[pUpdateIndex[0]].positionZ;
            pSpriteData->position.x = pFieldActor->transformMatrix.t[0] << 16;
            pActorData->position.vx = pFieldActor->transformMatrix.t[0] << 16;
            pSpriteData->position.y = pFieldActor->transformMatrix.t[1] << 16;
            pActorData->position.vy = pFieldActor->transformMatrix.t[1] << 16;
            pSpriteData->position.z = pFieldActor->transformMatrix.t[2] << 16;
            pActorData->position.vz = pFieldActor->transformMatrix.t[2] << 16;
            pSpriteData->field_0x84 = D_800B14F0[pUpdateIndex[0]].spriteField84;
            pActorData->rotation.vx = pActorData->rotation.vy = D_800B14F0[pUpdateIndex[0]].rotation;
            pUpdateIndex[0] = (pUpdateIndex[0] - 1) & 0x1F;
        }
    }
}
*/
