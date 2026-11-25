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
    /* 0xD */ s8 stepHeight;
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

ROUND 441 re-measure of the parked body above (REDO lane): reproduces 82.88 /
1228 / 307 exactly, so the note is not stale. Four scored runs this round:
- The script-slot loop's store order WAS source order after all: moving
  `scripts[i].flags_0 = 0xFFFF;` from fourth to LAST in the loop body (after
  state/isInUse/flags_0x17/flags_0x12) makes cc1 emit retail's
  lw 0x90 / sb 0x8E / sh 0x8C / sb 0x8F / and,and,and,or / sw 0x90 / sh 0x90
  order instead of sh 0x90 first. Only edit that helped: 82.88 -> 83.17
  (1228 bytes, 307 insns). KEPT in this body.
- The curTriNormal triple spelled as a cast over an offset-first sum,
  ((VECTOR*)(pActor->walkmeshId * sizeof(VECTOR) + (u32)normals))->vx, gives
  retail's 'addu v1,v1,a0' (offset in the destination) where the plain
  'normals[pActor->walkmeshId].vx' subscript gives 'addu v1,a0,v1':
  83.17 -> 83.39 but 1224 bytes / 306 insns, i.e. 8 bytes FURTHER from retail.
  KEPT in this body (highest score measured); the 83.17 shape is the fallback
  if the size is what matters.
- Per-store do-while blocks around the six walkmesh-loop zero stores (the
  round-428 recipe that closed the tail): 79.21 / 1216 / 304. Dead.
- Broken-records measure of the same body under other presets: gcc 2.7.2-cdk
  69.55, gcc 2.6.0 70.98, Default at -O3 75.58 - Default -O2 is the right
  compiler, so the residual is not a preset question.
RESIDUE (unchanged in kind, from the 83.39 body): still 2 instructions short
and the flag-mask constants land swapped (retail a2 = 0xFFFCFFFF + a1 =
0xF007FFFF + t7 = 0x3C0000 + t6 = 0xFF, ours a1/a2 and t6/t7), the same source
order producing both, so it is allocator numbering; retail's flags12C clear /
reload / second clear block keeps its lw in a0 where ours re-uses v1; and the
tail recomputes actorIndex * 0x1C into a2 where retail uses a0.
*/

/*
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
        pActor->scripts[i].state = 0;
        pActor->scripts[i].isInUse = 0;
        pActor->scripts[i].flags_0x17 = 0;
        pActor->scripts[i].flags_0x12 = 0xF;
        pActor->scripts[i].flags_0 = 0xFFFF;
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
    pActor->curTriNormal.vx = ((VECTOR*)(pActor->walkmeshId * sizeof(VECTOR) + (u32)normals))->vx;
    pActor->curTriNormal.vy = ((VECTOR*)(pActor->walkmeshId * sizeof(VECTOR) + (u32)normals))->vy;
    pActor->curTriNormal.vz = ((VECTOR*)(pActor->walkmeshId * sizeof(VECTOR) + (u32)normals))->vz;
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
}
*/



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
   Since R432 the five debug strings are NOT needed in C: the unit's rodata is
   linked as an anonymous blob (config/field.yaml [0x11C, rodata]), so defining
   them here would duplicate the blob's symbols - keep them extern. */
// PARKED R432 98.13% (315 insns / 1260 bytes vs retail 313 / 1252) - unchanged after
// unpark and re-measure; the R419 analysis below stands, plus one new mechanism:
// site A's nop is MASPSX LOAD-DELAY FILLING, not a scheduler slot. cc1 emits
// lw $2,g_FieldActors / #nop / addu $2,$19,$2 / lw $4,4($2) / lh $5,234($16) / jal
// (build/src/field/main/misc8.c.s, at the SpritePlayAnimation call), i.e. the second
// argument is emitted LAST because expand_call loads a0 before a1; retail has
// lw v0,0(v0) / lh a1,0xEA(s0) / addu v0,s3,v0 / lw a0,4(v0) / jal, so its toolchain
// moved an independent instruction up into the load-delay slot and maspsx inserted a
// nop instead. No source order produces retail's: a pre-call local for the second
// argument puts the lh BEFORE the lui %hi(g_FieldActors) that opens the call, and
// pSprite = g_FieldActors[i].pSpriteData; SpritePlayAnimation(pSprite, ...) is folded
// back to the same 98.13 / 315 (both measured this round).
// Site 2's move s1,a1 is the other two-pseudo CSE: retail's status test and the body
// pointer are different pseudos (cse1 copies one into the other), while every spelling
// of the address gives one pseudo allocated to $s1.
// R432 preset screen (whole unit, try_flags --all): Default = every other C function
// 100.00 and this one 98.13; gcc-2.6.0 = this 93.29 AND seven matched functions drop
// (FieldSetGearModelAnimationForActor 95.51, FieldGetDistanceScaledValue 98.33,
// FieldUpdateMotionSlotSteps 97.58, FieldUpdateMotionSlots 95.43,
// FieldScriptConfigureDefaultParticleBank 96.29, FieldScriptSoftReset 77.50), so the
// unit stays Default - no preset lead. This function alone: -O3 90.1, -G0 98.13 (no
// change), 2.7.2-cdk 93.99, 2.6.0 93.29. Do not reopen without a new lever.
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
            FieldSetSpriteStepFromAngle(pSpriteData, D_800B14F0[pUpdateIndex[0]].rotation, pFieldActor);
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
void FieldSetSpriteStepFromAngle(SpriteData*, s16, FieldActor*);

// This unit only tests the party slot against zero, and the original loads it signed.
typedef struct {
    /* 0x0 */ s16 value;
} FieldPlayerActorPartySlotView;

extern FieldPlayerActorPartySlotView g_FieldPlayerActorPartySlot;

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081C54);

/* R440 redo: 92.75% (768 bytes / 192 insns; retail 776 / 194). Ring layout is
   settled: 32 entries of 0x48 at D_800B14F0, every singleton store is
   D_800B14F0[index].field, the step/normal group wants a u8* to entry 0's
   step (retail's la a1,D_800B1510 and addiu a0,a1,0x10) and the walkmeshTriIds
   loop wants a copy of the index address taken before it (retail's move
   t1,a0 + lw v1,0(t1)), which pLoopIndex reproduces.
   RETAIL MIXES TWO INDEX SPELLINGS and that is the residual. Its first eight
   groups load the index absolute (lui, then lw %lo(sym)(reg), the maspsx
   expansion of a scalar volatile read) and its tail (walkmeshId store, the
   loop, the final read-modify-write) goes through a register holding
   &g_FieldSpriteUpdateIndex, which is loop.c hoisting the address for the loop
   and cse then rewriting the later reads as lw 0(reg). This unit reads
   g_FieldSpriteUpdateIndex as an unbounded array (needed by the matched
   neighbours: BattleUpdateSpriteRing indexes it by party id), and an array
   extern makes gcc hold the array address in a register for the WHOLE body
   (a0) and load with lw 0(a0), one instruction per group instead of retail's
   two - see levers k-unbounded-array-extern-global.
   MEASURED: declaring a scalar alias, extern volatile s32
   g_FieldSpriteUpdateCursor __asm__("g_FieldSpriteUpdateIndex"), does give
   retail's absolute lui+lw for every group and the body comes out at retail's
   exact 194 insns / 776 bytes - but it scores 71.21, and 74.34 with i assigned
   early, because with the index in v1 and nothing live in a0 the value loads
   are then free to fill the load-delay slot (lw a0,0xC(a2) instead of retail's
   nop) and every store comes out one instruction early. In the array form the
   array address keeps a0 busy, so the value takes v1 exactly as retail does and
   every group keeps its nop; that is what buys 92.75 over 71.
   Other measured spellings this round: the u8* bases assigned at declaration
   instead of before the normal group (89-90, la hoisted to the top of the
   body); the normal group spelled pRingStep+0x10/+0x14/+0x18 with no
   pRingNormal local (90.13, gcc folds the offsets into the store displacement,
   sw v1,16(v0) instead of retail's addiu a0,a1,0x10 + sw v1,0(v0)); the three
   position reads through s32 intermediates (91.59, DOES give retail's lh
   instead of lhu, but the extra live values rotate the s-block); 4-minute
   permuter runs from both bodies found only the i-early hint (92.23 / 74.34).
   Remaining, in cascade order: the two delay slots (retail puts the frame's
   addiu sp,sp,-24 in the bne slot and move a3,zero in the bnez slot, which is
   where its 2 extra instructions live), the value-temp register phase, and the
   three position reads' lh vs lhu. */
/*
void func_80081C54(int actorIndex) {
    FieldActor* pFieldActor = &g_FieldActors[actorIndex];
    ActorData* pActor = pFieldActor->pActorData;
    SpriteData* pSprite = pFieldActor->pSpriteData;
    u8* pRingStep;
    u8* pRingNormal;
    volatile s32* pLoopIndex;
    IVEC3 unused[2];
    int i;

    if (actorIndex != g_PlayerActorIndex) {
        return;
    }
    if (g_FieldPartyMoveInProgress != 0) {
        return;
    }
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].step.x = pSprite->step.x;
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].step.y = pSprite->step.y;
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].step.z = pSprite->step.z;
    pRingStep = (u8*)&D_800B14F0[0].step;
    pRingNormal = pRingStep + 0x10;
    *(s32*)(pRingNormal + g_FieldSpriteUpdateIndex[0] * 0x48) = pActor->curTriNormal.vx;
    *(s32*)(pRingNormal + 4 + g_FieldSpriteUpdateIndex[0] * 0x48) = pActor->curTriNormal.vy;
    *(s32*)(pRingNormal + 8 + g_FieldSpriteUpdateIndex[0] * 0x48) = pActor->curTriNormal.vz;
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].rotation = pActor->rotation.vy & 0xFFF;
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].spriteField84 = pSprite->field_0x84;
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].positionX = ((s16*)&pActor->position)[1];
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].positionY = ((s16*)&pActor->position)[3];
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].positionZ = ((s16*)&pActor->position)[5];
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].animationId = pActor->curAnimationId;
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].walkmeshTriMaterial = pActor->curWalkmeshTriMaterial;
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].scriptFlags = pActor->scriptFlags.flags;
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].flags = pActor->flags;
    pLoopIndex = g_FieldSpriteUpdateIndex;
    for (i = 0; i < 4; i++) {
        *(u_short*)(pRingStep - 0xA + pLoopIndex[0] * 0x48 + i * 2) = pActor->walkmeshTriIds[i];
    }
    D_800B14F0[g_FieldSpriteUpdateIndex[0]].walkmeshId = pActor->walkmeshId;
    D_800C3910 = 0;
    g_FieldSpriteUpdateIndex[0] = (g_FieldSpriteUpdateIndex[0] - 1) & 0x1F;
}
*/

int FieldActorTestMaterialScriptMask(ActorData* pActor) {
    u_int actorType = (pActor->scriptFlags.flags >> 9) & 3;

    return -((actorType & (pActor->curWalkmeshTriMaterial >> 3)) != 0);
}

// The pointer array of model records this unit's step blocks index; only
// field/main/main.c had it declared in a header-less place, so declare it here.
extern void* g_FieldActorModelPointers[];

// Retail merges this function's two "step = 0" copies into one out-of-line block
// placed after the model arm, so the zero case has to be written as the outer
// else of the flag chain for the merged copy to land there; written as the
// then-block of the angle test it lands first and the model arm falls into the
// mask tail instead of jumping over it. The 0x40000 arm writes pSprite->moveSpeed
// (0x18), not step.y.
void FieldSetSpriteStepFromAngle(SpriteData* pSprite, s16 angle, FieldActor* pActor) {
    s16 a;
    int scale;

    if (!(pActor->status & 0x40)) {
        scale = (0x40000 / pActor->pActorData->moveSpeed >> 8) << 5;
        a = angle & FIELD_ANGLE_MASK;
        if (!(angle & 0x8000)) {
            pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
            pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
        } else {
            pSprite->step.x = 0;
            pSprite->step.z = 0;
        }
    } else {
        a = angle;
        if (!(a & 0x8000)) {
            if (!(pActor->pActorData->flags & 0x2000)) {
                if (!(pActor->pActorData->flags & 0x80000)) {
                    SpriteSetDirection(pSprite, a);
                } else {
                    scale = (0x40000 / pActor->pActorData->moveSpeed >> 8) << 5;
                    a = angle & FIELD_ANGLE_MASK;
                    pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
                    pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
                    pSprite->moveSpeed = 0x4000000 / pActor->pActorData->moveSpeed;
                }
            } else if (!(pActor->pActorData->flags & 0x20000)) {
                scale = (0x80000 / pActor->pActorData->moveSpeed >> 8) << 5;
                a = angle & FIELD_ANGLE_MASK;
                pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
                pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
            } else {
                u_int modelSlot = pActor->pActorData->flags12C_0xE;

                pSprite->step.x = -((s32*)g_FieldActorModelPointers[modelSlot])[0x128 / 4] << 16;
                pSprite->step.z = -((s32*)g_FieldActorModelPointers[modelSlot])[0x130 / 4] << 16;
            }
        } else {
            pSprite->step.x = 0;
            pSprite->step.z = 0;
        }
    }
    pSprite->step.x &= -0x1000;
    pSprite->step.z &= -0x1000;
}

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
extern s32 g_FieldActorModelPointerIndex;
extern u_char g_FieldPartyMoveInProgress;

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

/* 96.26% (1436 bytes vs retail's 1424, 359 vs 358 insns: ONE instruction over,
   so it cannot link). Round 461 re-measured this body LIVE and found the lever
   that the old 95.34% note was missing: do NOT keep a dyAngle local -
   write `delta.vy = <rotation.y - track->rotationY>;` and use `delta.vy` in the
   angle expression. That reloads the halfword (`lh v0,0x52(sp)`) but frees the
   callee-saved register the local held, which drops retail's 9th saved word
   (the old body pushed $s8 for actorIndex and was 362 insns / 1448 bytes).
   Measured, all 358-362 insns: `s16 dyAngle` local 95.34 / 1448 / 362 (+$s8),
   the same with `s32 dyAngle = (s16)(...)` 95.19 / 1448 / 362, the chained
   `delta.vy = dyAngle = ...` 95.34 / 1448 / 362, `delta.vy` alone 96.26 / 1436 /
   359 - the park. So the two are mutually exclusive from source: retail truncates
   the delta into a register EARLY (`sll a0,a0,16` right after the delta stores,
   `sra s0,a0,16` in the `bnez` delay slot before FieldGetActorDistance2D) and
   reuses $s0, which is dead stepX in that arm; every C body here either keeps the
   value in a saved register of its own or reloads it from the stack.
   Settled (from the round-448 note, still true): arg1 is FieldActor* used only for
   pSpriteData; 0x74 is re-read at each of its six uses; g_FieldMaterialAngles is
   u_short[8]; the height clamp must be written condition-inverted; the move
   updates must re-form `targetPos + step - pos`, not `(targetPos - pos) + step`.
   Diffs left on this body: the `lh` reload above; an allocator permutation in the
   two zeroed stepX/stepZ registers and in `pActor->flags` (this body keeps flags
   in $s1 and gets dead `andi v0,s1,0x8000` fillers in two delay slots where retail
   has `sll v0,v1,0x1` / `nop`); and two store orders (the 0x400000 arm reads
   move.vz before the rotation.vx `lhu`, where retail does the `lhu`/`ori`/`sh`
   between the two `sw`s). */
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80082620);
/*
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
    FieldActor* pTracked;
    ActorData* pTarget;

    if (!((pActor->flags >> (pActor->walkmeshId + 3)) & 1)) {
        if (g_FieldPartyMoveInProgress == 0) {
            triMaterial = pActor->curWalkmeshTriMaterial;
        }
    }
    pSprite = (SpritePhysicsView*)pFieldActor->pSpriteData;
    FieldGetVec2FromPolar(&moveVec, D_800ADFC4[(triMaterial >> 9) & 3],
        (g_FieldMaterialAngles[(triMaterial >> 11) & 7] + (u_short)g_FieldControl.angle) & 0xFFF);
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
            delta.vy = pTracked[pActor->canInteract].rotation.y -
                ((FieldActorTargetTrack*)pActor->pTargetTrack)->rotationY;
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
            angle = (s16)ratan2(targetPosZ - posZ, targetPosX - posX) - delta.vy - 0x800;
            pActor->move.vx += targetPosX + ((rsin(angle) * distance) << 4) - posX;
            pActor->move.vz += targetPosZ + ((rcos(angle) * distance) << 4) - posZ;
        }
    }

    if ((pActor->flags & 0x22000) == 0x22000) {
        pActor->move.vx -= (((s32*)g_FieldActorModelPointers[g_FieldActorModelPointerIndex])[0x128 / 4] << 16) /
            pActor->moveSpeed << 8;
        pActor->move.vz -= (((s32*)g_FieldActorModelPointers[g_FieldActorModelPointerIndex])[0x130 / 4] << 16) /
            pActor->moveSpeed << 8;
        g_FieldActorModelPointerIndex++;
    }
}
*/




extern s32 g_FieldUpdatingActorIndex;
extern s32 D_800ADB68;
extern s32 g_FieldPartyActorSlots[];
extern u16 g_FieldP1ButtonsPressed;
int func_8008492C(ActorData* pActor);
int FieldProbeWalkmeshAtAngle(VECTOR* pVec, ActorData* pActor, SVECTOR* pPoints, s16 angle);
int FieldProbeWalkmeshSlideAlongWall(VECTOR* pVec, ActorData* pActor, SVECTOR* pPoints, s16 angle);

/* 85.15% (1456 bytes against retail's 1472 - four instructions short, and the
   frame is retail's exactly: 0x58 = vars 40 + 8 saved (s0-s6+ra) + 16 arg save
   area, .mask 0x807f0000). The body below is instruction-for-instruction retail's
   except for these, all of them block PLACEMENT rather than shape:
   1. gcc puts the rotation-locked arm (rotation.vx |= 0x8000; animationId =
      defaultAnimationId) inline where the source branches to it; retail has it out
      of line at 0x2F5C, falling into the reset block at 0x2F70, and uses `beqz v0`
      plus a `nop` delay where this body inverts to `bnez v0` with no delay.
   2. Because retail's reset block is the fall-through of that locked arm, its
      `if (result == -1)` comes out as `beq -> reset; lui v0,0x10000 (delay);
      j -> join; addiu v0,-0x1001 (delay)` (4 insns); here the reset is the
      fall-through of the test, so it is `bne -> join; li v0,-0x1001 (delay)` (2).
   3. Same for the probe/BAC0 ternary: retail keeps FieldProbeWalkmeshSlideAlongWall inline and the
      cross-jumped FieldProbeWalkmeshAtAngle arm out of line (`bnez v0` with an
      `addiu a0,sp,0x10` delay); this body inverts to `beqz v0 -> FieldProbeWalkmeshSlideAlongWall`.
   4. The 0x1800 chain needs the two-case `switch` spelling below: as an
      if/else-if chain gcc inverts it into `bne` tests and loses the trailing
      `j`+`nop` (82.82 -> 84.61).
   Also unsettled: retail gives $s5 to actorIndex (2 refs) and $s6 to pFieldActor
   (3 refs) where global_alloc gives pFieldActor the lower number here, so every
   s5/s6 operand is swapped (0 bytes, about ten diff rows), and the two party-slot
   tests read g_FieldPartyActorSlots[1]/[2] (+4/+8 addends) where retail's relocs
   name D_8005A448/D_8005A44C - the linked bytes are identical, so diff_func folds
   them but objdiff cannot reach 100.00 there.
   Everything else is settled: the 0x1000000 early out with g_FieldUpdatingActorIndex = actorIndex
   written before it, hitTimer's unsigned `>= 9` test, moving = move.vx|vy|vz with
   func_8008492C(pActor) == -1 forcing 1, the 0x40800 lock test, pos = pSprite->step
   then three adds (that struct copy is what reproduces retail's double store of
   step.x), the ratan2 angle wanting the (s16) cast before the negate, savedFlags
   restoring the 0x600 bits around the probe, the `== 2 ? 0x60 : 0x30` moveSpeed
   arms, the 0x200000 material arm (`&&` keeps the second rotation.vx test inline),
   the animationOverride / 0x2000000 gate, and
   FieldSetGearModelAnimationForActor(pSprite, animationId, pFieldActor). */
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80082BB8);
/*
void func_80082BB8(int actorIndex, FieldActor* pFieldActor, ActorData* pActor) {
    VECTOR pos;
    SVECTOR points[2];
    SpriteData* pSprite;
    s32 savedFlags;
    s32 moving;
    s16 animationId;
    int angle;
    int result;

    pSprite = pFieldActor->pSpriteData;
    angle = pActor->rotation.vx;
    g_FieldUpdatingActorIndex = actorIndex;
    if (pActor->scriptFlags.flags & 0x1000000) {
        return;
    }
    animationId = 1;
    if ((pActor->scriptFlags.flags & 0x4000) && (g_FieldP1ButtonsPressed & 0x40) && D_800ADB68 == 1) {
        animationId = 2;
    }
    if (pActor->scriptFlags.flags & 0x1800) {
        if (pActor->curAnimationId != animationId) {
            switch (pActor->curAnimationId) {
            case 1:
                animationId = 1;
                break;
            case 2:
                animationId = 2;
                break;
            }
        }
    }
    if (pActor->hitTimer >= 9) {
        pActor->hitTimer--;
    }
    moving = pActor->move.vx | pActor->move.vy | pActor->move.vz;
    if (func_8008492C(pActor) == -1) {
        moving = 1;
    }
    if ((angle & 0x8000) && !moving && !(pActor->scriptFlags.flags & 0x40800)) {
        pActor->rotation.vx |= 0x8000;
        animationId = pActor->defaultAnimationId;
        goto stopped;
    }
    if (angle & 0x8000) {
        pos.vx = pActor->move.vx;
        pos.vy = pActor->move.vy;
        pos.vz = pActor->move.vz;
        angle = pActor->rotation.vy & FIELD_ANGLE_MASK;
    } else {
        FieldSetSpriteStepFromAngle(pSprite, angle, pFieldActor);
        pos.vx = pSprite->step.x;
        pos.vy = pSprite->step.y;
        pos.vz = pSprite->step.z;
        pos.vx += pActor->move.vx;
        pos.vy += pActor->move.vy;
        pos.vz += pActor->move.vz;
        pActor->rotation.vy = angle;
    }
    if (FieldCheckActorInZone2D((IVEC3*)&pos, pActor) != 0) {
        goto stopped;
    }
    if (pos.vx != 0 || pos.vz != 0) {
        angle = -(s16)ratan2(pos.vz, pos.vx) & FIELD_ANGLE_MASK;
    }
    result = -1;
    if (pActor->walkmeshTriIds[pActor->walkmeshId] != -1) {
        savedFlags = pActor->scriptFlags.flags;
        if (actorIndex == g_PlayerActorIndex) {
            if (g_FieldPartyActorSlots[1] != ACTOR_ID_INVALID) {
                pActor->scriptFlags.flags =
                    savedFlags | (g_FieldActors[g_FieldPartyActorSlots[1]].pActorData->scriptFlags.flags & 0x600);
            }
            if (g_FieldPartyActorSlots[2] != ACTOR_ID_INVALID) {
                pActor->scriptFlags.flags |=
                    g_FieldActors[g_FieldPartyActorSlots[2]].pActorData->scriptFlags.flags & 0x600;
            }
        }
        if ((pActor->scriptFlags.flags & 0x41800) || pActor->canInteract != ACTOR_ID_INVALID || D_800ADB98 != 0) {
            result = FieldProbeWalkmeshAtAngle(&pos, pActor, points, angle);
        } else {
            result = FieldProbeWalkmeshSlideAlongWall(&pos, pActor, points, angle);
        }
        pActor->scriptFlags.flags = (pActor->scriptFlags.flags & ~0x600) | (savedFlags & 0x600);
    }
    if (result == -1) {
stopped:
        pActor->fallSpeed = 0x10000;
        pActor->move.vx = 0;
        pActor->move.vy = 0;
        pActor->move.vz = 0;
        pos.vx = 0;
        pos.vy = 0;
        pos.vz = 0;
        pSprite->step.x = 0;
        pSprite->step.z = 0;
        pActor->rotation.vy |= 0x8000;
    }
    pActor->flags &= ~0x1000;
    if (pActor->scriptFlags.flags & 0x800) {
        if (g_Unknown_800B2344 == 0) {
            if (((s16*)&pSprite->position)[3] == pSprite->field_0x84) {
                pSprite->step.y = 0;
            } else if (animationId == 2) {
                pSprite->step.y = pSprite->field_0x82 * 0x60;
            } else {
                pSprite->step.y = pSprite->field_0x82 * 0x30;
            }
        }
        animationId = g_FieldCurrentModelAnimationId[0];
    } else {
        if (pActor->rotation.vx & 0x8000) {
            animationId = pActor->defaultAnimationId;
        }
        if (FieldActorGetWalkmeshTriMaterial(pActor) & 0x200000) {
            if ((pActor->rotation.vx & 0x8000) && pActor->curAnimationId == 6) {
                pActor->flags |= 0x1000;
            }
            animationId = 6;
        }
    }
    if (pActor->animationOverride != ACTOR_ID_INVALID) {
        animationId = pActor->animationOverride;
    }
    if ((pActor->curAnimationId != animationId) && !(pActor->scriptFlags.flags & 0x2000000)) {
        pActor->curAnimationId = animationId;
        FieldSetGearModelAnimationForActor(pSprite, animationId, pFieldActor);
    }
    if (pActor->curWalkmeshTriMaterial & 0x100) {
        pos.vx >>= 1;
        pos.vz >>= 1;
    }
    pActor->moveModified.vx = pos.vx;
    pActor->moveModified.vy = pos.vy;
    pActor->move.vx = 0;
    pActor->move.vy = 0;
    pActor->move.vz = 0;
    pActor->moveModified.vz = pos.vz;
}
*/

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

void SpriteSetDirection(SpriteData*, s16);

void FieldSetSpriteStepFromAngle(SpriteData*, s16, FieldActor*);

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
   Round 294: retail's 0x20 frame is vars= 8 (gcc prints it in its own .frame
   line), i.e. the original had 8 bytes of stack locals it never touched; one
   unused u32 (4 bytes) is NOT enough (still 0x18). `u32 spare2[2];` as the last
   declarations reproduces the frame exactly and the body below is 97.03% (144
   vs 148) with every instruction but retail's move in place; gcc loads into v1
   and ANDs into v0 where retail loaded into v0, copied to v1 and ANDed v0.
   Third permuter run on this body (1796 iters) also found nothing.
*/
/* R369 note: re-measured 97.03% (144 vs retail 148). The one missing instruction
   is retail's `move v1,v0` after `lh v0,0x106`: retail loads into v0, copies to
   v1 for the arithmetic and masks the test in v0; gcc loads straight into v1.
   Also tried this round: the assignment embedded in the condition, an s32
   `current`, a separate s32 copy local (coalesced, frame shrank to 0x18) and an
   s16 `masked = current & 0x8000` local (90.14%, load became lhu plus sll/sra).
   gcc will not materialise an SImode subreg copy of a single-live-range HImode
   value; three permuter runs on this body found nothing. */
/* R396: re-measured 97.03% (144 vs retail 148). New this round: an s32 load
   local with an s16 copy (both directions - mask on the wide one, mask on the
   narrow one), which copy-propagates back to one pseudo; preset screen -O1
   91.46, -O3 97.03, cdk -O3 93.78, -G0 97.03. The missing instruction is
   retail's copy of the lh result into the arithmetic register; gcc merges the
   two because the load has one live range. */
/* PARKED R419 97.03% (144 bytes / 36 insns vs retail 148 / 37). The only residual
   is retail's HImode copy: retail loads into v0, copies to v1 for the ternary
   arithmetic and masks the test in v0 (`lh v0,0x106 / move v1,v0 / andi
   v0,v0,0x8000`); gcc loads straight into v1 (`lh v1 / andi v0,v1,0x8000`).
   gcc copy-propagates every two-variable spelling, so the copy is an SImode
   subreg the allocator never materialises. The 0x20 frame with 8 bytes of
   unused stack vars is reproduced exactly by the `u32 spare2[2];` declaration.
   Levers checked R419 (move/copy/missing/extra): clamp-copy-shape, sltiu-copy
   and live-value-costs-nothing all describe a boolean/narrow copy, none of them
   a HImode load promoted to SImode. Prior rounds: s16/s32/u16 variables both
   directions, mask read off the field directly (90.92), assignment in the
   condition, unused int local, cdk (89.27), 2.6.0 (92.51), -O1 (91.46), -O3 and
   -G0 (97.03), four permuter runs (base 113 = best 113). Not a hand job. */
/* PARKED R432 97.03% (144 bytes / 36 insns vs retail 148 / 37). Unchanged from R419:
   the only residual is retail's HImode copy of the lh result (lh v0,0x106 / move v1,v0 /
   andi v0,v0,0x8000); gcc loads into v1 and masks v0 from it. R432 re-measured after
   unpark: identical diff, and lever.py (move/copy/HImode/load/promoted plus the
   'value both arms compute' and 'combine folds mask' headings) names no mechanism for a
   HImode load promoted to SImode, so this stays a compiler-shape wall (4 permuter runs,
   base 113 = best 113). Do not reopen without a new lever. */
/*
void func_800831F4(SpriteData* pSprite, ActorData* pActorData, FieldActor* pActor, int direction) {
    s16 angle;
    s16 current;
    s16 angleCopy;
    u32 spare2[2];

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
            FieldSetSpriteStepFromAngle(pSprite, pActorData->rotation.vx, pActor);
            angle = pActorData->rotation.vy | 0x8000;
            pActorData->rotation.vx = angle;
            pActorData->rotation.vy = angle;
        }
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_800831F4);



void FieldGetTrianglePlaneHeight(SVECTOR* pA, SVECTOR* pB, SVECTOR* pC, SVECTOR* pOut, VECTOR* pNormal);

// Height the polygon check starts from: no polygon was under the test point
#define FIELD_POLY_HEIGHT_NONE 0x7FFFFFFF
// Command bit that says the primitive has four vertices instead of three
#define FIELD_POLY_COMMAND_QUAD 0x8

// One word of the command stream pBounds->pCommands points at: either the command
// word itself (primitiveCount << 16 | opcode) or a pair of vertex indices.
typedef union {
    /* 0x0 */ u32 command;
    /* 0x0 */ u16 indices[2];
} FieldModelCommandWord;

// The 0xB8-byte scratch block the polygon check runs in: it holds the polygon
// transformed to screen space, the matrices the GTE works from and the lowest
// plane height found so far.
typedef struct {
    /* 0x00 */ s32 sxy[4]; // Screen x/y of the four transformed vertices
    /* 0x10 */ s32 sxyRef; // Screen x/y of the point the check is made for
    /* 0x14 */ SVECTOR v[4]; // The polygon's vertices, transformed
    /* 0x34 */ SVECTOR planePoint; // x/z of the test point, y the plane's height there
    /* 0x3C */ long flag; // RotTransSV's flag word
    /* 0x40 */ MATRIX screenMatrix; // Actor to screen, the one handed to the GTE
    /* 0x60 */ MATRIX workMatrix;
    /* 0x80 */ MATRIX parentMatrix;
    /* 0xA0 */ s32 minHeight;
    /* 0xA4 */ SVECTOR* pVertices;
    /* 0xA8 */ undefined32 unkA8;
    /* 0xAC */ u32 command; // opcode of the command being walked
    /* 0xB0 */ SVECTOR rotation;
} FieldPolyCheckScratch; // Size: 0xB8

extern MATRIX D_800AFC30;

// Returns 0 and the height of the lowest polygon under (refX, refZ) in *pHeight,
// -1 when the actor's model has no polygon there.
/* PARKED 83.18% (448 insns / 1792 bytes vs retail 451 / 1804). Structure measured
   this round, in order of what each one bought:
   62 -> 75.91 the min-height tail is written as if (minHeight == NONE) { free; return -1; }
   then the store/free/return 0; that is the only form whose beq lands on the -1 block
   (the negated if emits bne and puts the two blocks the other way round).
   75.91 -> 78.75 refX/refZ are int, not u16: retail spills them with sw and reloads with
   lw. The loop's planePoint.vx/vz stores still come out lhu/sh either way.
   78.75 -> 83.18 the case-0 (parent matrix) block must be emitted AFTER the rotation
   block, so the switch is switch (flags12C_0 and 3) with cases 1,2,3 where case 3 breaks
   into a rotate label, default is a single goto parent, and the parent block plus the
   shared CompMatrix follow. Retail's dispatch jumps straight to the parent block because
   that goto is threaded away.
   Residuals, all measured:
   (1) case 1 in retail reads lhu 0x70(a1) reusing the pActorData pointer the dispatch
       already loaded; this body reloads the g_FieldActors chain there (6 insns).
       Cases 2 and 3 reload in retail too, so no plain pActorData local reproduces it.
   (2) 3 instructions / 12 bytes short, all in the parent-matrix block: retail keeps
       s2 = g_FieldActors and a single pSourceMatrix in s0 across the two CompMatrix
       calls; here the address is re-formed each time.
   (3) actorIndex and pBounds land in s1/s5 here, retail has s5/s6 (the loop body below
       is otherwise instruction-identical, so this is purely the allocator's priority).
   (4) the six matrix t[] zero stores schedule around the first CompMatrix here, retail
       emits all six first.
   Tried and rejected: u16 refX/refZ (74.01), a pActorData local (kills the reloads in
   cases 2/3 too), duplicating the rotate tail per case (66.81). */
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80083288);
/*
s32 func_80083288(int actorIndex, FieldActorModelBounds* pBounds, int refX, int refZ, s32* pHeight, VECTOR* pNormal) {
    FieldPolyCheckScratch* pScratch;
    FieldModelCommandWord* pCmd;
    MATRIX* pSourceMatrix;
    u32 command;
    int numPrimitives;
    int i;
    int j;

    pScratch = (FieldPolyCheckScratch*)FieldAllocScratchpad(sizeof(FieldPolyCheckScratch));
    pCmd = (FieldModelCommandWord*)pBounds->pCommands;
    pScratch->minHeight = FIELD_POLY_HEIGHT_NONE;
    pScratch->pVertices = ((SVECTOR**)pBounds->unk8)[0];
    pScratch->sxyRef = (refX << 16) + refZ;
    switch (g_FieldActors[actorIndex].pActorData->flags12C_0 & 3) {
    case 1:
        pScratch->rotation.vy = 0;
        pScratch->rotation.vz = 0;
        pScratch->rotation.vx = g_FieldActors[actorIndex].pActorData->hideModeParam;
        goto rotate;
    case 2:
        pScratch->rotation.vx = 0;
        pScratch->rotation.vz = 0;
        pScratch->rotation.vy = g_FieldActors[actorIndex].pActorData->hideModeParam;
        goto rotate;
    case 3:
        pScratch->rotation.vx = 0;
        pScratch->rotation.vy = 0;
        pScratch->rotation.vz = g_FieldActors[actorIndex].pActorData->hideModeParam;
        break;
    default:
        goto parent;
    }
rotate:
    RotMatrix(&pScratch->rotation, &pScratch->workMatrix);
    MulMatrix2(&g_FieldActors[actorIndex].transformMatrix, &pScratch->workMatrix);
    pScratch->workMatrix.t[0] = g_FieldActors[actorIndex].transformMatrix.t[0];
    pScratch->workMatrix.t[1] = g_FieldActors[actorIndex].transformMatrix.t[1];
    pScratch->workMatrix.t[2] = g_FieldActors[actorIndex].transformMatrix.t[2];
    CompMatrix(&g_Scene.worldRotationMatrix, &pScratch->workMatrix, &pScratch->screenMatrix);
    goto setMatrices;
parent:
    pScratch->screenMatrix.t[2] = 0;
    pScratch->screenMatrix.t[1] = 0;
    pScratch->screenMatrix.t[0] = 0;
    pScratch->parentMatrix.t[2] = 0;
    pScratch->parentMatrix.t[1] = 0;
    pScratch->parentMatrix.t[0] = 0;
    pSourceMatrix = &pScratch->parentMatrix;
    CompMatrix(&g_Scene.worldRotationMatrix, &D_800AFC30, pSourceMatrix);
    if (g_FieldActors[actorIndex].pActorData->parentActorId != ACTOR_ID_INVALID) {
        pSourceMatrix = &pScratch->workMatrix;
        CompMatrix(&pScratch->parentMatrix,
            &g_FieldActors[g_FieldActors[actorIndex].pActorData->parentActorId].childMatrix,
            pSourceMatrix);
    }
    CompMatrix(pSourceMatrix, &g_FieldActors[actorIndex].transformMatrix, &pScratch->screenMatrix);
setMatrices:
    SetRotMatrix(&pScratch->screenMatrix);
    SetTransMatrix(&pScratch->screenMatrix);

    for (i = pBounds->numCommands; i > 0; i--) {
        command = pCmd->command;
        pScratch->command = command & 0xFF;
        numPrimitives = command >> 16;
        if ((command & 0xFF) == 0xC4 || (command & 0xFF) == 0xC8) {
            pCmd++;
            continue;
        }
        pCmd++;
        if (!(command & FIELD_POLY_COMMAND_QUAD)) {
            for (j = 0; j < numPrimitives; j++) {
                RotTransSV(&pScratch->pVertices[pCmd->indices[0]], &pScratch->v[0], &pScratch->flag);
                RotTransSV(&pScratch->pVertices[pCmd->indices[1]], &pScratch->v[1], &pScratch->flag);
                pCmd++;
                RotTransSV(&pScratch->pVertices[pCmd->indices[0]], &pScratch->v[2], &pScratch->flag);
                pCmd++;
                pScratch->sxy[0] = (pScratch->v[0].vx << 16) + pScratch->v[0].vz;
                pScratch->sxy[1] = (pScratch->v[1].vx << 16) + pScratch->v[1].vz;
                pScratch->sxy[2] = (pScratch->v[2].vx << 16) + pScratch->v[2].vz;
                if (NormalClip(pScratch->sxy[0], pScratch->sxy[1], pScratch->sxyRef) < 0 ||
                    NormalClip(pScratch->sxy[1], pScratch->sxy[2], pScratch->sxyRef) < 0 ||
                    NormalClip(pScratch->sxy[2], pScratch->sxy[0], pScratch->sxyRef) < 0 ||
                    NormalClip(pScratch->sxy[0], pScratch->sxy[1], pScratch->sxy[2]) < 0) {
                    continue;
                }
                pScratch->planePoint.vx = refX;
                pScratch->planePoint.vz = refZ;
                FieldGetTrianglePlaneHeight(&pScratch->v[0], &pScratch->v[1], &pScratch->v[2],
                    &pScratch->planePoint, pNormal);
                if (pScratch->planePoint.vy < pScratch->minHeight) {
                    pScratch->minHeight = pScratch->planePoint.vy;
                }
            }
        } else {
            for (j = 0; j < numPrimitives; j++) {
                RotTransSV(&pScratch->pVertices[pCmd->indices[0]], &pScratch->v[0], &pScratch->flag);
                RotTransSV(&pScratch->pVertices[pCmd->indices[1]], &pScratch->v[1], &pScratch->flag);
                pCmd++;
                RotTransSV(&pScratch->pVertices[pCmd->indices[0]], &pScratch->v[2], &pScratch->flag);
                RotTransSV(&pScratch->pVertices[pCmd->indices[1]], &pScratch->v[3], &pScratch->flag);
                pCmd++;
                pScratch->sxy[0] = (pScratch->v[0].vx << 16) + pScratch->v[0].vz;
                pScratch->sxy[1] = (pScratch->v[1].vx << 16) + pScratch->v[1].vz;
                pScratch->sxy[2] = (pScratch->v[2].vx << 16) + pScratch->v[2].vz;
                pScratch->sxy[3] = (pScratch->v[3].vx << 16) + pScratch->v[3].vz;
                if (NormalClip(pScratch->sxy[0], pScratch->sxy[1], pScratch->sxyRef) < 0 ||
                    NormalClip(pScratch->sxy[1], pScratch->sxy[3], pScratch->sxyRef) < 0 ||
                    NormalClip(pScratch->sxy[3], pScratch->sxy[2], pScratch->sxyRef) < 0 ||
                    NormalClip(pScratch->sxy[2], pScratch->sxy[0], pScratch->sxyRef) < 0 ||
                    NormalClip(pScratch->sxy[0], pScratch->sxy[1], pScratch->sxy[2]) < 0) {
                    continue;
                }
                pScratch->planePoint.vx = refX;
                pScratch->planePoint.vz = refZ;
                if (NormalClip(pScratch->sxy[1], pScratch->sxy[2], pScratch->sxyRef) >= 0) {
                    FieldGetTrianglePlaneHeight(&pScratch->v[0], &pScratch->v[1], &pScratch->v[2],
                        &pScratch->planePoint, pNormal);
                } else {
                    FieldGetTrianglePlaneHeight(&pScratch->v[1], &pScratch->v[3], &pScratch->v[2],
                        &pScratch->planePoint, pNormal);
                }
                if (pScratch->planePoint.vy < pScratch->minHeight) {
                    pScratch->minHeight = pScratch->planePoint.vy;
                }
            }
        }
    }
    if (pScratch->minHeight == FIELD_POLY_HEIGHT_NONE) {
        FieldFreeScratchpad(sizeof(FieldPolyCheckScratch));
        return -1;
    }
    *pHeight = pScratch->minHeight;
    FieldFreeScratchpad(sizeof(FieldPolyCheckScratch));
    return 0;
}
*/

void FieldNoOp(void) {
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008399C);

int FieldAllocScratchpad(int size);
void FieldFreeScratchpad(int size);
extern void func_800379C8(char* pFormat, ...);
extern const char D_8006FC48[]; // "POLYCHECK %d\n"
extern const char D_8006FC58[]; // "HITOFF\n"
extern s32 D_800ADB94;
extern s32 D_800ADB98;
extern void ControllerResetState(void);
s32 func_80083288(int actorIndex, FieldActorModelBounds* pBounds, int refX, int refZ, s32* pHeight,
                  VECTOR* pNormal);

// The 0x20 bytes this function asks FieldAllocScratchpad for: the vector GTE
// Square0 measures, then the squared vector it writes back.
typedef struct {
    /* 0x00 */ VECTOR position;
    /* 0x10 */ VECTOR squared;
} FieldSquareScratch; // Size: 0x20

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084158);
/* R445 park: 90.12%, 503 insns / 2012 bytes against retail's 501 / 2004, 216
   differing lines. STRUCTURE IS SETTLED - every arm, call, store and flag mask
   below is retail's, and the frame now matches exactly (0xC8, vars=0x88,
   saves fp,s0-s7,ra). What is left is one register permutation plus one block
   layout, both allocator/layout choices:
   (1) REGISTERS: retail keeps i in s2, pScratch in s3 and the other actor's
       scriptFlags in s4 (s5 = the 0x5C bias, s6 = my Y from lh 0x26(s1), s7 =
       nearest, fp/s8 = actorIndex, and the 0xFF3FFFFF mask SPILLED at 0x88).
       This body gets s2 = otherFlags, s3 = i, s4 = pScratch, fp = actorIndex.
       The saved set and every slot offset otherwise agree (0x40 groundY,
       0x48 pFieldActor, 0x50 refX, 0x58 refZ, 0x60 floorY, 0x68 scriptFlags,
       0x70 contact, 0x78 hitCount, 0x80 savedCanInteract, 0x88/0x90 the two
       masks). Tried: reading pOther->scriptFlags.flags inline three times
       (that body is 89.83% / 502 insns and puts the mask in fp), adding
       otherFlags (89.83 -> 90.08 -> 90.12 with the declaration order below),
       moving otherFlags after i (no change).
   (2) BLOCK LAYOUT / +2 insns: retail lays the blocks out touch-B6, hitTimer,
       lower-B7 (B7 at 0x504/0x508) and takes `bnez` to B7 from both of its
       tests; this body lays B7 out inline after B4 (so the second test comes
       out `beqz` and B7 is emitted twice: +2 insns, +8 bytes). Writing the
       two B7 tests as separate `if (...) goto lower;` with lower placed last
       measured 87.85% / 499 insns, so the `||` form below is the better one.
       The gotos are load-bearing: if/else duplicates the 20-insn B6 block,
       which retail has once (a single `pActor->canInteract == i &&
       !(scriptFlags & 0x40800)` entry and two `goto touch` entries).
   (3) THE moveModified RELOAD in the push block is retail's, not a mistake: it
       stores zero to moveModified.vx/.vy/.vz and then LOADS them back for the
       /4 into move.vx/.vz, a sequence gcc folds away here (that is part of the
       byte gap, its two divisions are the 2 insns that survive). If a later
       run finds the spelling that keeps the reload, that is the remaining win.
   Next lever to try: permuter from this body (structure is exact, so only
   allocation is left), or a 2.6.0/cdk screen if the allocator turns out to be
   the other compiler's. Settled facts kept from R432: the frame/signature, the
   `lh 0x26(s1)` = (s16)(pActor->position.vy >> 16) spelling (gcc folds the
   lw+sra into lh, it does NOT cost an instruction), and that
   FieldCheckPointInExtentZone's 3rd argument is the ActorData* cast to
   FieldExtentZone* (retail passes s0 = pOther unchanged). */
/*
void func_80084158(int actorIndex, FieldActor* pFieldActor, ActorData* pActor) {
    FieldSquareScratch* pScratch;
    FieldActorModelData* pModelData;
    SpriteData* pSprite;
    ActorData* pOther;
    IVEC3 targetPos;
    SVECTOR ref;
    VECTOR normal;
    s32 refX;
    s32 refZ;
    s32 myY;
    s32 floorY;
    u_int scriptFlags;
    s32 contact;
    s32 hitCount;
    s32 savedCanInteract;
    s32 nearest;
    s32 groundY;
    s32 otherY;
    int i;
    u_int otherFlags;

    pScratch = (FieldSquareScratch*)FieldAllocScratchpad(sizeof(FieldSquareScratch));
    targetPos.x = pActor->position.vx;
    targetPos.y = pActor->position.vy;
    targetPos.z = pActor->position.vz;
    targetPos.x += pActor->moveModified.vx;
    targetPos.y += pActor->moveModified.vy;
    targetPos.z += pActor->moveModified.vz;
    FieldConvertIVec3ToSVector(&ref, &targetPos);
    refX = ref.vx;
    refZ = ref.vz;
    myY = (s16)(pActor->position.vy >> 16);
    floorY = myY - pActor->height;
    scriptFlags = pActor->scriptFlags.flags;
    savedCanInteract = pActor->canInteract;
    nearest = FIELD_POLY_HEIGHT_NONE;
    hitCount = 0;
    contact = 0;
    for (i = 0; i < g_FieldActiveActorCount; i++) {
        if (i == actorIndex) {
            continue;
        }
        pOther = g_FieldActors[i].pActorData;
        otherFlags = pOther->scriptFlags.flags;
        if (otherFlags & 1) {
            continue;
        }
        pOther->flags &= ~0xC100;
        if (pOther->flags & 0x80) {
            pModelData = g_FieldActors[i].pModelData;
            if (func_80083288(i, pModelData->pBounds, refX, refZ, &groundY, &normal) != 0) {
                pOther->flags &= ~0xC00000;
                continue;
            }
            if (g_FieldSystemMode == 0) {
                func_800379C8(D_8006FC48, i);
            }
            pOther->flags |= 0x100;
            otherY = groundY + pOther->height;
            if (pActor->canInteract == i) {
                pActor->curTriNormal.vx = normal.vx;
                pActor->curTriNormal.vy = normal.vy;
                pActor->curTriNormal.vz = normal.vz;
                pOther->flags |= 0x4000;
            }
        } else {
            if (otherFlags & 0x2000) {
                if (FieldCheckPointInExtentZone(refX, refZ, (FieldExtentZone*)pOther, 0) != 0) {
                    pOther->flags &= ~0xC00000;
                    continue;
                }
            } else {
                pScratch->position.vx =
                    CONV_TO_GTE(pOther->position.vx + pOther->moveModified.vx) - refX;
                pScratch->position.vz =
                    CONV_TO_GTE(pOther->position.vz + pOther->moveModified.vz) - refZ;
                pScratch->position.vy = pActor->solidRange + pOther->solidRange;
                Square0(&pScratch->position, &pScratch->squared);
                if (pScratch->squared.vx + pScratch->squared.vz >= pScratch->squared.vy) {
                    pOther->flags &= ~0xC00000;
                    continue;
                }
            }
            if (pActor->curWalkmeshTriMaterial & 0x400000) {
                if (g_FieldSystemMode == 0) {
                    func_800379C8(D_8006FC58);
                }
                continue;
            }
            if ((otherFlags | scriptFlags) & 0x80) {
                continue;
            }
            if (g_FieldPartyMoveInProgress) {
                continue;
            }
            otherY = (s16)(pOther->position.vy >> 16);
            groundY = otherY - pOther->height;
        }
        if (pActor->canInteract == i && !(scriptFlags & 0x40800)) {
            goto touch;
        }
        if (otherY < floorY || myY < groundY) {
            pOther->flags &= ~0x100;
            if (myY < groundY) {
                pOther->flags |= 0x800000;
                if (groundY < nearest) {
                    nearest = groundY;
                }
            } else {
                pOther->flags &= ~0x800000;
            }
            goto next;
        }
        if (myY < groundY + 0x10 || (pOther->flags & 0x800000)) {
            goto touch;
        }
        if ((scriptFlags & 0x10) == 0) {
            if (pOther->hitTimer < 0x30) {
                pOther->hitTimer += 2;
            }
            if (pOther->hitTimer >= 0x21) {
                pOther->move.vx += pActor->moveModified.vx / 4;
                pOther->move.vz += pActor->moveModified.vz / 4;
                pActor->moveModified.vx = 0;
                pActor->moveModified.vy = 0;
                pActor->moveModified.vz = 0;
                pActor->move.vx = pActor->moveModified.vx / 4;
                pActor->move.vy = 0;
                pActor->move.vz = pActor->moveModified.vz / 4;
                goto next;
            }
        }
        pOther->move.vx = 0;
        pOther->move.vy = 0;
        pOther->move.vz = 0;
        pOther->moveModified.vx = 0;
        pOther->moveModified.vy = 0;
        pOther->moveModified.vz = 0;
        pActor->moveModified.vx = 0;
        pActor->moveModified.vy = 0;
        pActor->moveModified.vz = 0;
        pActor->move.vx = 0;
        pActor->move.vy = 0;
        pActor->move.vz = 0;
        goto next;
    touch:
        nearest = groundY;
        pOther->flags |= 0x800000;
        pActor->move.vx = pOther->moveModified.vx;
        pActor->move.vy = pOther->moveModified.vy;
        pActor->move.vz = pOther->moveModified.vz;
        hitCount = 2;
        if (!(scriptFlags & 0x40800)) {
            pActor->canInteract = i;
            contact = 1;
        }
    next:
        pOther->flags |= 0x400000;
    }

    if (D_800ADB98 != 0) {
        nearest = D_800ADB94;
        contact = 0;
        hitCount++;
    }
    if (contact == 0) {
        pActor->canInteract = ACTOR_ID_INVALID;
    } else {
        g_FieldActors[pActor->canInteract].pActorData->flags |= 0x8000;
        if (savedCanInteract == ACTOR_ID_INVALID) {
            if (!(pActor->flags134 & 0x80)) {
                pActor->pTargetTrack = HeapAlloc(0xC, 0);
                pActor->flags134 |= 0x80;
            }
            ((FieldActorTargetTrack*)pActor->pTargetTrack)->rotationX =
                g_FieldActors[pActor->canInteract].rotation.x;
            ((FieldActorTargetTrack*)pActor->pTargetTrack)->rotationY =
                g_FieldActors[pActor->canInteract].rotation.y;
            ((FieldActorTargetTrack*)pActor->pTargetTrack)->rotationZ =
                g_FieldActors[pActor->canInteract].rotation.z;
            ((FieldActorTargetTrack*)pActor->pTargetTrack)->distance =
                FieldGetActorDistance2D(actorIndex, pActor->canInteract);
        }
    }
    if (!(pActor->scriptFlags.flags & 0x10000) && !(pActor->flags & 0x200000)) {
        func_80084A40(actorIndex, nearest, pFieldActor, pActor, hitCount);
    }
    pSprite = g_FieldActors[actorIndex].pSpriteData;
    if (((SpriteDataI3*)pSprite->field_0x7C)->field_0xC == 1) {
        ControllerResetState();
        pActor->scriptFlags.flags &= ~0x800;
    }
    FieldFreeScratchpad(sizeof(FieldSquareScratch));
}
*/

extern s32 D_800ADC0C;

/* 96.01% (276 bytes / 69 insns = retail's exactly). Re-measured LIVE in round 461,
   so the number is not a stale object. Every check matches; the residual is three
   spots in the tail:
   1. retail has a `nop` in the `beqz flags&2` delay slot where reorg steals this
      body's `move a1,a0` (the `new_var = pActor;` copy - that copy is the lever:
      91.16 -> 96.01; the same assignment at the top of the function 87.25/284,
      dropped 91.16).
   2. retail keeps `addiu v0,zero,-1` as a separate block (.L80084A34, reached by
      the `beq v1,v0` whose delay slot is a `nop`) where every body here threads
      `li v0,-1` into that delay slot.
   3. retail threads `move v0,zero` into the `beqz flags&4` delay slot where this
      body keeps a zero block.
   Round 461 re-measured the inverted tail with the new_var copy in place
   (`if (!(pActor->flags & 4)) return 0; return -(pActor->walkmeshId == 2);`):
   90.07 / 268 / 67 - it HAS retail's `move v0,zero` slot and no zero block, and is
   one instruction short (no out-of-line -1 block), so it cannot link.
   Round 448: else-return-0 91.16/276/69, a nested `if` for the flags&2 test
   compiles byte-identically to `&&`, and `s32 result = 0;` before the flags&4 test
   puts the second flags load in retail's $v1 but allocates result to $a1
   (86.74/280/70). Lever c-there-source-ordering-lever applies: there is no source
   ordering lever for delay-slot fill, so the -1 block (a reorg fill decision) is
   not reachable from source. Permuter run from 91.16 found the new_var copy
   (base 461 -> 407). */
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008492C);
/*
int func_8008492C(ActorData* pActor) {
    ActorData* new_var;

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
    new_var = pActor;
    if (new_var->flags & 2) {
        if (pActor->walkmeshId == 1) {
            return -1;
        }
    }
    if (new_var->flags & 4) {
        return -(pActor->walkmeshId == 2);
    } else {
        return 0;
    }
}
*/


extern s32 D_800ADB98;
extern u8 D_800B21CF;
extern u16 g_FieldButtonPollMask;
extern void func_800379C8(char* pFormat, ...);
extern s32 func_8008492C(ActorData* pActor);
extern s32 func_8007D3D4(ActorData* pActor, s32 nSetIndex, s32* pY, VECTOR* pNormal, s16* pTriIndex,
                         s32* pYMax);
extern void func_80081C54(int actorIndex);
extern const char D_8006FC60[]; // "ERROR ID1 ACT=%d\n"
extern const char D_8006FC74[]; // "ERROR ID0 ACT=%d\n"

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084A40);

/* Best attempt: 84.61%, 2596 bytes against retail's 2700 (649 insns against 676, so
   ~27 instructions are still missing). Body below is the fourth measured version;
   every block and every call is present and in retail's order.
   THE FACTS THIS FUNCTION ESTABLISHED (they are the expensive part):
   - signature is s32 (returns 0 on the walk, -1 when it bails), not the void the
     unit's extern says. Callers pass (actorIndex, y, pFieldActor, pActor, arg4);
     arg4 is the 5th argument, read at 0x110(sp) inside a 0x100 frame.
   - SpriteData+0x10 is step.y (NOT moveSpeed) and +0x1C is gravity; the code
     accumulates step.y += gravity and stores it into ActorData::fallSpeed.
   - the frame is 0x100 with locals at 0x18 heights[4] (s32), 0x28 ranges[4],
     0x38 tris[4], 0x48 indices[4] (s16), 0x50 normals[4] (VECTOR), 0x90/0x94/0x98
     saved position x/y/z, 0xA0 savedTriIds[4] (s16); 0xA8..0xBF is dead, i.e. the
     original source held more locals than the code reads - the same 24-byte dead
     tail as func_8007D3D4 and func_8007BEF4 in main/misc4.
   - func_8007D3D4's real prototype is in main/misc4.c's parked body:
     s32 func_8007D3D4(ActorData*, s32 nSetIndex, s32* pY, VECTOR* pNormal,
                       s16* pTriIndex, s32* pYMax).
   - the sort is a 2x2 bubble over the three parallel arrays (heights, ranges,
     tris) - indices are NOT swapped - and nStep is read out of heights[walkmeshId]
     BEFORE the sort.
   - `flags &= ~0x401000` is the 0xFFBFEFFF mask (NOT ~0x40001000, which is 0xBFFF).
     gcc emits srl where retail has sra once `material` is u_int, and retail's
     `sll/sra` pair on the saved walkmesh id means the local is a u16 compared
     against the s16 field: `pActor->walkmeshId != (s16)savedWalkmeshId`.
   WHAT IS STILL WRONG: (1) the array bases land in the wrong slots whichever order
   I declare them in (gcc 2.7.2 gives normals 0x20 / indices 0x60 / heights 0x88
   here; three declaration orders were measured - 78.63 / 82.97 / 84.61 - and the
   offsets never came out ascending as retail's do), (2) actorIndex/y/pActor/pSprite
   are s5/s6/s1/s4 here against retail's s6/s7/s1/s5, and that tie drives every
   later operand. Fixing (1) by hand is the next thing to try; then the ~27 missing
   instructions (compare the retail tail at 0x85248/0x85398 with the body's). */
/*
// Places the actor on the walkmesh under its next step: probes every triangle set
// (func_8007D3D4), keeps the three lowest plane heights in sorted order, moves the
// actor onto the set it belongs to and either commits the new position or puts the
// saved one back. y is the height the caller wants, arg4 the number of matched sets.
s32 func_80084A40(int actorIndex, int y, FieldActor* pFieldActor, ActorData* pActor, s32 arg4) {
    SpriteData* pSprite;
    s16 savedTriIds[4];
    s32 savedZ;
    s32 savedY;
    s32 savedX;
    VECTOR normals[4];
    s16 setIndices[4];
    s32 setTris[4];
    s32 setRanges[4];
    s32 setHeights[4];
    u16 savedWalkmeshId;
    s32 nStep;
    u_int material;
    s32 tmp;
    s32 i;
    s32 j;
    s32 k;

    pSprite = g_FieldActors[actorIndex].pSpriteData;
    if (actorIndex == g_PlayerActorIndex) {
        g_FieldButtonPollMask = 0xFFFF;
    }
    if (pActor->scriptFlags.flags & 0x1000000) {
        return -1;
    }
    if (pActor->flags & 0x200000) {
        return -1;
    }
    if (!(pActor->scriptFlags.flags & 0x10000)) {
        if (!(actorIndex == g_PlayerActorIndex && D_800B21CF == 1) && pSprite->step.y == 0 &&
            func_8008492C(pActor) == 0 && pSprite->field_0x84 == ((s16*)&pActor->position)[3]) {
            return -1;
        }
    }
    savedX = pActor->position.vx;
    savedY = pActor->position.vy;
    savedZ = pActor->position.vz;
    savedWalkmeshId = pActor->walkmeshId;
    for (i = 0; i < 4; i++) {
        savedTriIds[i] = pActor->walkmeshTriIds[i];
    }
    for (i = 0; i < 4; i++) {
        setTris[i] = i;
        setHeights[i] = 0x7FFFFFFF;
        setRanges[i] = 0x7FFFFFFF;
    }
    for (i = 0; i < g_FieldWalkmeshTriCount - 1; i++) {
        if (func_8007D3D4(pActor, i, &setHeights[i], &normals[i], &setIndices[i], &setRanges[i]) != 0) {
            break;
        }
    }
    if (pActor->flags & 1) {
        setHeights[0] = 0x7FFFFFFF;
        setRanges[0] = 0x7FFFFFFF;
    }
    if (pActor->flags & 2) {
        setHeights[1] = 0x7FFFFFFF;
        setRanges[1] = 0x7FFFFFFF;
    }
    if (pActor->flags & 4) {
        setHeights[2] = 0x7FFFFFFF;
        setRanges[2] = 0x7FFFFFFF;
    }
    nStep = setHeights[pActor->walkmeshId];
    for (j = 0; j < 2; j++) {
        for (k = 0; k < 2; k++) {
            if (setHeights[k + 1] < setHeights[k]) {
                tmp = setHeights[k];
                setHeights[k] = setHeights[k + 1];
                setHeights[k + 1] = tmp;
                tmp = setRanges[k];
                setRanges[k] = setRanges[k + 1];
                setRanges[k + 1] = tmp;
                tmp = setTris[k];
                setTris[k] = setTris[k + 1];
                setTris[k + 1] = tmp;
            }
        }
    }
    if (i == g_FieldWalkmeshTriCount - 1) {
        if (i > 0) {
            for (k = 0; k < g_FieldWalkmeshTriCount - 1; k++) {
                pActor->walkmeshTriIds[k] = setIndices[k];
            }
        }
        if ((((s16*)&pActor->position)[3] < nStep) || (pActor->scriptFlags.flags & 0x1800)) {
            for (k = 0; k < g_FieldWalkmeshTriCount - 1; k++) {
                if (!(setHeights[k] < ((s16*)&pActor->position)[3])) {
                    pActor->walkmeshId = setTris[k];
                    break;
                }
            }
        } else {
            for (k = 0; k < g_FieldWalkmeshTriCount - 1; k++) {
                if (pActor->walkmeshId == setTris[k]) {
                    break;
                }
            }
        }
        if ((FieldActorGetWalkmeshTriMaterial(pActor) & 4) && k != 0 &&
            pActor->walkmeshId <= g_FieldWalkmeshTriCount - 1) {
            pActor->walkmeshId = setTris[k - 1];
        }
        material = FieldActorGetWalkmeshTriMaterial(pActor);
        if (((pActor->scriptFlags.flags >> 8) & 7) & (material >> 5)) {
            if (g_FieldSystemMode == 0) {
                func_800379C8(D_8006FC60, actorIndex);
            }
            goto finishWalk;
        }
        if (material & 0x800000) {
            if (g_FieldSystemMode == 0) {
                func_800379C8(D_8006FC74, actorIndex);
            }
            goto finishWalk;
        }
        pActor->position.vx += pActor->moveModified.vx;
        pActor->position.vz += pActor->moveModified.vz;
        for (k = 0; k < g_FieldWalkmeshTriCount - 1; k++) {
            if (pActor->walkmeshId == setTris[k]) {
                pSprite->field_0x84 = setHeights[k];
                break;
            }
        }
        VectorNormal(&normals[pActor->walkmeshId], &pActor->curTriNormal);
    } else {
        pActor->fallSpeed = 0;
    }
    if (D_800ADB98 != 0) {
        if ((u32)arg4 < 2) {
            pSprite->field_0x84 = y;
        }
    } else if (arg4 != 0) {
        if (pSprite->field_0x84 < y + 0xA) {
            pActor->canInteract = 0xFF;
        }
        pSprite->field_0x84 = y;
        pActor->position.vy = y << 16;
    }
    if (pActor->scriptFlags.flags & 0x40000) {
        pActor->position.vy = pActor->scriptYPos << 16;
        pSprite->step.y = 0;
    }
    pActor->position.vy += pSprite->step.y;
    material = FieldActorGetWalkmeshTriMaterial(pActor);
    if (pActor->walkmeshId != (s16)savedWalkmeshId) {
        pActor->scriptFlags.flags &= ~0x4000000;
    }
    if (!(pActor->scriptFlags.flags & 0x4000000) &&
        pSprite->field_0x84 > ((s16*)&pActor->position)[3]) {
        if (pSprite->field_0x84 != ((s16*)&pActor->position)[3]) {
            pSprite->step.y += pSprite->gravity;
        }
        pActor->scriptFlags.flags |= 0x1000;
        pActor->fallSpeed = pSprite->step.y;
    } else {
        if (!(material & 0x420000)) {
            pActor->fallSpeed = 0;
        }
        if (pSprite->step.y > 0) {
            pSprite->step.y = 0;
        }
        pActor->scriptFlags.flags &= ~0x401000;
        pActor->position.vy = pSprite->field_0x84 << 16;
    }
    pActor->scriptFlags.flags &= ~0x4000000;
    for (k = 0; k < g_FieldWalkmeshTriCount - 1; k++) {
        if ((setHeights[k] < ((s16*)&pActor->position)[3]) &&
            ((((s16*)&pActor->position)[3] - pActor->height) < setRanges[k])) {
            if (setHeights[k] != setRanges[k]) {
                break;
            }
        }
    }
    nStep = 0;
    if (k == g_FieldWalkmeshTriCount - 1) {
        nStep = g_FieldWalkmeshTriangles[pActor->walkmeshId]
                    [pActor->walkmeshTriIds[pActor->walkmeshId]]
                        .stepHeight *
                4;
        if ((nStep < 0) &&
            !((((s16*)&pActor->position)[3] - pActor->height) < (pSprite->field_0x84 + nStep))) {
            goto restorePosition;
        }
        pSprite->position.x = pActor->position.vx;
        pSprite->position.y = pActor->position.vy;
        pSprite->position.z = pActor->position.vz;
        g_FieldActors[actorIndex].transformMatrix.t[0] = ((s16*)&pActor->position)[1];
        g_FieldActors[actorIndex].transformMatrix.t[1] = ((s16*)&pActor->position)[3];
        g_FieldActors[actorIndex].transformMatrix.t[2] = ((s16*)&pActor->position)[5];
        pActor->curWalkmeshTriMaterial = FieldActorGetWalkmeshTriMaterial(pActor);
        func_80081C54(actorIndex);
        return 0;
    }
restorePosition:
    pActor->position.vx = savedX;
    pActor->position.vz = savedZ;
    pActor->walkmeshId = savedWalkmeshId;
    pActor->fallSpeed = 0;
    for (k = 0; k < 4; k++) {
        pActor->walkmeshTriIds[k] = savedTriIds[k];
    }
    if (pSprite->field_0x84 != ((s16*)&pActor->position)[3]) {
        pSprite->step.y += pSprite->gravity;
    }
    if (pSprite->step.y < 0) {
        pSprite->step.y = 0;
        pActor->position.vy = savedY;
    }
    pSprite->position.x = pActor->position.vx;
    pSprite->position.y = pActor->position.vy;
    pSprite->position.z = pActor->position.vz;
    g_FieldActors[actorIndex].transformMatrix.t[1] = ((s16*)&pActor->position)[3];
    func_80081C54(actorIndex);
    return 0;

finishWalk:
    if (actorIndex == g_PlayerActorIndex) {
        g_FieldButtonPollMask = 0xFFF;
    }
    pActor->position.vy += pSprite->moveSpeed;
    pActor->position.vx = savedX;
    pActor->position.vz = savedZ;
    pActor->walkmeshId = savedWalkmeshId;
    pActor->fallSpeed = 0;
    for (k = 0; k < 4; k++) {
        pActor->walkmeshTriIds[k] = savedTriIds[k];
    }
    if (((s16*)&pActor->position)[3] < pSprite->field_0x84) {
        if (pSprite->field_0x84 != ((s16*)&pActor->position)[3]) {
            pSprite->moveSpeed += pSprite->gravity;
        }
    } else {
        if (pSprite->moveSpeed > 0) {
            pSprite->moveSpeed = 0;
        }
        pActor->scriptFlags.flags &= ~0x40001000;
        pActor->position.vy = pSprite->field_0x84 << 16;
    }
    pSprite->position.x = pActor->position.vx;
    pSprite->position.y = pActor->position.vy;
    pSprite->position.z = pActor->position.vz;
    g_FieldActors[actorIndex].transformMatrix.t[1] = ((s16*)&pActor->position)[3];
    func_80081C54(actorIndex);
    return 0;
}
*/

extern s32 g_FieldArchiveStreamStatus;
extern s32 g_FieldArchiveStreamActive;
extern void* g_FieldArchiveStreamBlock;
extern void* g_FieldArchiveStreamCallback;
int ArchiveReadStreamSector(void);
int* ArchiveAllocStreamFile(int numEntries, int allocMode);
void ArchiveReadFileToBuffer(int fileIndex, void* pBuffer, int arg2, int size);

int FieldStepArchiveStream(void) {
    int result = ArchiveReadStreamSector();

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
   R369: also tried the guard-plus-bottom-test shape (count = 0; if (streamIndex
   + 1 > 0) { do {...; count++;} while (count < streamIndex + 1); }) - 92.91%,
   same 252 bytes, loop.c still hoists streamIndex+1 - and moving `streamIndex =
   streamId;` above the 0xFF early return (93.00%, no change).
   R473 arithmetic of the residual (63 insns / 252 bytes vs retail 66 / 264): an
   hoisted `addiu s0,s1,1` counts as ONE instruction where retail's two
   rematerialisations are two (+1), retail's redundant `move s1,s0` is (+1) --
   mine loads the s16 straight into s1 and computes fileIndex into s0, so the
   copy never exists -- and the third is retail's `nop` in the
   SoundAddSedsEntry delay slot, which the hoist fills in mine. So the body gcc
   wants here is strictly shorter than retail's; the gap cannot be closed by
   making the C better.
   R473 measured dead ends: gcc copy-propagates the s16/s32 pair in EVERY
   spelling, so the s32 local is not the copy retail has; making the loop bound
   use streamId and the file index use streamIndex (and the reverse) is
   byte-identical to the current body; try_flags gcc-2.6.0 93.00 (== unit
   flags) and gcc-2.7.2-cdk 92.68, so this is not a preset question. The
   hoisted bound sits in the register fileIndex frees at SoundAddSedsEntry,
   which is why loop.c/gcse can hoist it at all; no source spelling found that
   keeps that register live through the latch.
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80085788);
/*
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

void FieldActorWorldToScreenPosition(int actorIndex, int* pScreenX, int* pScreenY) {
    SVECTOR vertex;
    MATRIX transformMatrix;
    long sxy;
    long p;
    long flag;
    FieldActor* volatile* pActorList;

    FieldScriptVMGetActorIndex(1);
    // The actor list pointer sits 0xAC bytes after g_Scene.worldToScreenMatrix
    // (0x800AFB10 = the g_FieldActors symbol); retail anchors both on that one base.
    pActorList = (FieldActor* volatile*)((u8*)&g_Scene.worldToScreenMatrix + 0xAC);
    CompMatrix((MATRIX*)((u8*)pActorList - 0xAC), &(*pActorList)[actorIndex].transformMatrix, &transformMatrix);
    vertex.vx = 0;
    vertex.vy = 0;
    vertex.vz = 0;
    SetRotMatrix(&transformMatrix);
    SetTransMatrix(&transformMatrix);
    RotTransPers(&vertex, &sxy, &p, &flag);
    *pScreenY = ((short*)&sxy)[1];
    *pScreenX = ((short*)&sxy)[0];
}

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
