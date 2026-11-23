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
/* 77.88% (1200 bytes against retail's 1232; frame 0xA0 and the saved set
   s0-s6+ra are retail's exactly, so the body is the right function and what is
   left is register allocation plus store scheduling).
   WHAT IS SETTLED:
   - arg0 is the actor index; pActor = g_FieldActors[actorIndex].pActorData
     (FieldActor +0x4C) is loaded once and every store in the body goes through
     it. Retail's 0xA0 frame and the sp+0x18 / sp+0x58 slots come free from
     `VECTOR normals[4]; SVECTOR points[4];` (points ends at 0x78, the saved
     registers start at 0x80) - no spare local is needed.
   - the 0x12C word is cleared through the header's anonymous bitfields
     (flags12C_0x5, _0, _0x10, _0x12, _0x6, direction, _0xD, _0x2) and each one
     reproduces retail's single lw/and/sw pair; flags130 has no bitfields in the
     header, and THREE consecutive `flags130 &= ~0xFF80000; &= ~0x1FF;
     &= ~0x7FE00;` statements reproduce retail's three ANDs with its masks
     (0xF007FFFF, 0xFFFFFE00, 0xFFF801FF) in its own order - gcc does not fold
     them, and it stores once.
   - the 8-slot script reset loop (actorData+0x8C) is byte-identical: currentIP
     0xFFFF, waitTimer 0, scriptId 0xFF, flags_0 0xFFFF, state/isInUse/
     flags_0x17 cleared, flags_0x12 = 0xF.
   - the triangle loop's range test is `tri != -1 && (u32)tri >= D_800AFB44[i]`
     with the cast on the LEFT operand only; casting both sides emits the same
     sltu but scores 1.7 lower (it re-schedules the loop).
   - g_FieldActors must be read ONCE per iteration (one lui/lw serving both the
     `(s16)transformMatrix.t[0]` and `.t[2]` loads), which needs the
     `pFieldActor = &g_FieldActors[actorIndex];` local inside the loop.
   RESIDUE (all measured, none source-controllable by hand so far):
   (a) the loop: retail keeps TWO walking pointers (s1 stride 0x10 for normals,
       s2 = sp+0x18 stride 8 with points at 0x40(s2)) plus addiu s2,/s1, in the
       tail. That is what ONE aggregate `struct { VECTOR normals[4]; SVECTOR
       points[4]; }` produces (measured: it removes all four per-iteration
       address recomputations) - but the same body then over-allocates s7 (the
       aggregate base) and s8 (the 8i walker for &points[i]), grows the frame to
       0xA8 and drops to 75.14%. The two-array form keeps retail's frame and
       saved set and pays 2 instructions (`sll`+`addu` twice per iteration).
       Typing the argument as pPoint/pNormal pointer locals: 75.32%.
   (b) the four tail reads (position.vx/vy/vz, curYPos) must stay four separate
       volatile g_FieldActors reads. Hoisting `transformMatrix.t` into a `long*`
       scores 78.88% but deletes 12 of retail's instructions (1180 bytes), so it
       is a false lead: retail's chains are lui/lw/nop/addu/lw/nop/sll/sw with
       the load-delay nops present, and ours interleave two chains so the nops
       disappear (that is ~8 of the missing 32 bytes).
   (c) the 32-bit mask constants land in swapped registers (retail a2 = 0xFFFCFFFF
       before a1 = 0xF007FFFF) which re-orders the whole independent store run
       that follows; retail also re-reads flags12C (lw a0,0x12C after storing it)
       for flags12C_0x12 where gcc forwards the register from flags12C_0.
   (d) the curTriNormal triple re-derives `lh 0x10(s0)` (walkmeshId) three times
       in retail; ours does too, but retail's `addu v0,v0,a0` is index-first
       where ours is `addu a0,a0,v0` (base-first) for the third site.
   Permuter: three 4-minute runs, ~5000 iterations. It found the
   `(u32)tri >=` cast (76.21 -> 77.88) and two statement moves that measured
   worse in objdiff (dialogFlags between rotation.vx/vy: 77.31).
   Round 294 re-measured the whole body from this note (it compiles, 77.88%).
   The 8 missing instructions are ALL in the segment after the walkmesh loop:
   retail emits each of the four transformMatrix tail reads as a strictly
   serial chain through v0 (lui/lw/nop/addu/lw/nop/sll/sw, 8 insns each = 32),
   while gcc always prefetches the NEXT statement's volatile lw g_FieldActors
   into the field load's delay slot (6 insns each = 24). Measured dead: making
   the four field reads volatile (1204 bytes, 77.32), handing each read to one
   reused s32 local via a comma expression (1204, 77.40). The rest of the diff
   is register naming plus the flags12C store/reload pair (retail stores after
   the 0xFFFCFFFF AND, reloads, ANDs 0xF003FFFF and stores again; gcc forwards
   the register), and the mask constants landing in swapped registers.
   ROUND 411 re-measured the body exactly (un-parked: 77.88%, 1200 bytes, 300
   against retail's 308) and pinned the residual: the whole 8-instruction
   deficit is the two load-delay nops per tail read. Retail is
   lui/lw/nop/addu/lw/nop/sll/sw; ours is
   lw(base of the PREVIOUS statement)/lui/lw/addu/sll/sw, which has no load
   delay to fill, so the two nops can never appear. cc1's options header has
   neither -fschedule-insns nor -fschedule-insns2, so that interleave is not a
   scheduler: it is cse/gcse hoisting the next statement's
   `idx*0x5C + g_FieldActors[0]` base into the previous statement's window.
   Wrapping the four reads in do { } while (0) changed nothing (1200 bytes, same
   instruction count). Any spelling that keeps that base out of gcse's window is
   the remaining lead. m2c's whole-function draft is structurally identical to
   this body (it spells the tri-normal triple `(&normals)[walkmeshId].vx` and the
   transform write `points[walkmeshId].vy`), so the structure is settled.
*/
/*
extern s16 g_FieldWalkmeshTriCount;
extern s32 D_800AFB44[];
extern int func_8007B1C4(int x, int z, int walkmeshId, SVECTOR* pOut, VECTOR* pNormal);
extern int rand(void);

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
    pActor->position.vx = g_FieldActors[actorIndex].transformMatrix.t[0] << 16;
    pActor->position.vy = g_FieldActors[actorIndex].transformMatrix.t[1] << 16;
    pActor->position.vz = g_FieldActors[actorIndex].transformMatrix.t[2] << 16;
    pActor->curYPos = g_FieldActors[actorIndex].transformMatrix.t[1];
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
    /* 0x12 */ u_short animationId;
    /* 0x14 */ u_short rotation;
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
extern volatile s32 g_FieldSpriteUpdateIndex;
extern s32 D_800C3910;
extern u_char g_FieldPartyMoveInProgress;
extern s32 g_PlayerActorIndex;

/* R428 redo: 69.69% (ours 768 bytes / 192 insns; retail 776 / 194). Parked body
   is the best of four spellings measured this round. Ring layout is settled (see
   the R400 note in the stub above): 32 entries of 0x48 at D_800B14F0 indexed by
   the volatile s32 g_FieldSpriteUpdateIndex, one read per store (18 reads: 17
   stores plus the trailing index update), and IVEC3 unused[2] reproduces retail's
   dead 0x18 frame.
   THE RESIDUAL IS A VALUE-TEMPORARY ALLOCATOR TIE, not statement order. With the
   typed spelling D_800B14F0[i].field gcc gives the stored value $a0 while the
   index sits in $v1, so sched2 fills the index load's delay slot with the value
   load and every one of the 15 singleton stores comes out one instruction early
   (63.95%, 194/194 insns, 776 bytes = retail's exact size, but ~200 differing
   lines). Writing the same store as a cast to a computed address puts the value
   in $v1, i.e. retail's phase, because retail's value load and its index load
   share $v1 (that is also why retail has an unfilled nop after every index
   load) - but gcc then materialises the ring base in a register and derives
   addu $5,$4,<offset> per statement (69.56%, 780 bytes).
   RETAIL MIXES THE TWO FORMS, which is why neither pure spelling closes it: the
   nine singleton groups use lui $at,%hi(sym) / addu $at,$at,$v0 / sw %lo(sym)($at)
   (the typed spelling), while the three normal stores go through a0 = D_800B1510
   + 0x10 with displacements 0/4/8 and the walkmeshTriIds loop through t2 = a1
   - 0xA, both derived from ONE la a1,D_800B1510 placed mid-body. The parked body
   reproduces that normal/loop base derivation from u8* pRingStep = (u8*)&D_800B14F0[0].step
   and keeps the typed spelling elsewhere, which is what buys 69.69 over 69.56,
   but it is 2 instructions short of retail's 194.
   Measured this round: all-typed 63.95 (776, 194); all-cast to (u8*)D_800B14F0 +
   offset 69.56 (780, 195); typed + pRingStep for normal/loop 69.69 (768, 192);
   all-cast + a pRingNormal base at +0x10 67.32 (776, 194).
   Next attempt: find a spelling that keeps retail's phase AND the $at form - i.e.
   the value temp in $v1 with the index while the address stays symbol+offset. A
   shared source-level value local was not tried. */
/*
void func_80081C54(int actorIndex) {
    FieldActor* pFieldActor = &g_FieldActors[actorIndex];
    ActorData* pActor = pFieldActor->pActorData;
    SpriteData* pSprite = pFieldActor->pSpriteData;
    u8* pRingStep = (u8*)&D_800B14F0[0].step;
    IVEC3 unused[2];
    int i;

    if (actorIndex != g_PlayerActorIndex) {
        return;
    }
    if (g_FieldPartyMoveInProgress != 0) {
        return;
    }
    D_800B14F0[g_FieldSpriteUpdateIndex].step.x = pSprite->step.x;
    D_800B14F0[g_FieldSpriteUpdateIndex].step.y = pSprite->step.y;
    D_800B14F0[g_FieldSpriteUpdateIndex].step.z = pSprite->step.z;
    *(s32*)(pRingStep + 0x10 + g_FieldSpriteUpdateIndex * 0x48) = pActor->curTriNormal.vx;
    *(s32*)(pRingStep + 0x14 + g_FieldSpriteUpdateIndex * 0x48) = pActor->curTriNormal.vy;
    *(s32*)(pRingStep + 0x18 + g_FieldSpriteUpdateIndex * 0x48) = pActor->curTriNormal.vz;
    D_800B14F0[g_FieldSpriteUpdateIndex].rotation = pActor->rotation.vy & 0xFFF;
    D_800B14F0[g_FieldSpriteUpdateIndex].spriteField84 = pSprite->field_0x84;
    D_800B14F0[g_FieldSpriteUpdateIndex].positionX = ((s16*)&pActor->position)[1];
    D_800B14F0[g_FieldSpriteUpdateIndex].positionY = ((s16*)&pActor->position)[3];
    D_800B14F0[g_FieldSpriteUpdateIndex].positionZ = ((s16*)&pActor->position)[5];
    D_800B14F0[g_FieldSpriteUpdateIndex].animationId = pActor->curAnimationId;
    D_800B14F0[g_FieldSpriteUpdateIndex].walkmeshTriMaterial = pActor->curWalkmeshTriMaterial;
    D_800B14F0[g_FieldSpriteUpdateIndex].scriptFlags = pActor->scriptFlags.flags;
    D_800B14F0[g_FieldSpriteUpdateIndex].flags = pActor->flags;
    for (i = 0; i < 4; i++) {
        *(u_short*)(pRingStep - 0xA + g_FieldSpriteUpdateIndex * 0x48 + i * 2) = pActor->walkmeshTriIds[i];
    }
    D_800B14F0[g_FieldSpriteUpdateIndex].walkmeshId = pActor->walkmeshId;
    D_800C3910 = 0;
    g_FieldSpriteUpdateIndex = (g_FieldSpriteUpdateIndex - 1) & 0x1F;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081C54);

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
/* R400 redo: fresh body from the .s, re-measured 58.17% (644 vs retail 628
   bytes = 0x274, 161 vs 157 insns). The three rsin/rcos copies and the two
   0x40000 scale sites are right - gcc emits the same two merged copies retail
   has. Residuals, in the order they cascade:
   (1) the else-branch's angle test: retail materialises (s16)angle into a2
       (sll v0,a1,16 / sra a2,v0,16) and ANDs a2 with 0x8000, then keeps a3 free
       for the mult results; we fold it to `bgez` on the sll result and then need
       a3 for the extension, which pushes every `mflo a3` to `mflo t0`.
       Tried this round: the bare test, `(s32)angle & 0x8000`, `(angle & 0x8000)
       != 0` - all three fold to bgez. The park below is the bare form.
   (2) the two zero-blocks: retail funnels BOTH angle tests into one out-of-line
       block at 0x800821B0 (sw zero,0xC / sw zero,0x14 / fall through to
       addiu v0,-0x1000); ours keeps the then-branch's copy inline and ends it
       with a `j` over, so cross-jumping cannot merge them.
   (3) the 0x2000 body: retail lays it out-of-line (bnez over the 0x80000 test
       and SpriteSetDirection); ours falls through into it.
   (4) model-pointer block: retail CSEs base+index into v1 before the first
       store and reads the pointer twice through it; ours recomputes the index
       after the store (pActor->pActorData / 0x12C reloads) through $at.
   (5) frame 0x38 vs retail 0x28. */
/* 93.81% (616 bytes against retail's 640, 154 vs 157 insns). Frame 0x30 vs
   retail's 0x28 (vars=8, from a local kept across the two jal sites).
   STRUCTURE SETTLED (block order is retail's, gcc 2.7.2 does not reorder blocks):
   `if (!(status & 0x40))` is the inline then-block and the 0x40-set half follows
   out of line, which is why retail's test is `bnez` to the model half; the
   0x80000 test inside the 0x2000-clear half needs the CALL as the then-block
   (`if (!(flags & 0x80000)) SpriteSetDirection(...) else compute`) and the
   0x20000 test needs the 0x80000-scale compute first and the model arm as the
   else, or gcc inlines the wrong side. Read angle as s16 at the call site
   (`s16 a = angle`) so the SET half truncates with sll/sra into a2; a 2.7.2
   param of int changes nothing (see residual 1).
   RESIDUALS, in cascade order:
   (1) SET-half angle test: retail materialises (s16)angle in a2 and ANDs it with
       0x8000 (sll/sra/andi/bnez/nop = 5 insns); gcc folds (s16)x & 0x8000 to
       `bgez` on the sll result (3 insns), so we are 2 insns short there. Tried:
       bare test, (s32)(s16)angle & 0x8000, `!= 0`, an int parameter with an s16
       local, -O1/-O3/cdk/-G0 screens - the fold always fires.
   (2) model arm: retail reloads the table ENTRY through the CSE'd address
       (`lw v0,0(v1)` twice, v1 = &g_FieldActorModelPointers[idx]) because the
       step.x store sits between the two reads; the pointer-local spelling
       (`s32* pModel = g_Field...) collapses it to one load (-3) and the
       `&table[idx]` spelling this body uses leaves one load and orders the srl
       before the lui base pair where retail has it after (plus retail's load nop).
   (3) tail: retail's model arm ends in `j` over the shared two-store zero block;
       here the zero block sits in the SET half's fall-through, so the arm falls
       into the -0x1000 tail directly (1 insn). Cross-jumping does not move it.
   Tried in all: local vs repeated subscript (648/616/608 bytes), pModel local,
   int vs s16 parameter, the two inner tests in both polarities. */
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80081F80);
/*
void func_80081F80(SpriteData* pSprite, s16 angle, FieldActor* pActor) {
    s16 a;
    int scale;

    if (!(pActor->status & 0x40)) {
        scale = (0x40000 / pActor->pActorData->moveSpeed >> 8) << 5;
        a = angle & 0xFFF;
        if (!(angle & 0x8000)) {
            pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
            pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
        } else {
            pSprite->step.x = 0;
            pSprite->step.z = 0;
        }
    } else {
        a = angle;
        if (a & 0x8000) {
            pSprite->step.x = 0;
            pSprite->step.z = 0;
        } else if (!(pActor->pActorData->flags & 0x2000)) {
            if (!(pActor->pActorData->flags & 0x80000)) {
                SpriteSetDirection(pSprite, a);
            } else {
                scale = (0x40000 / pActor->pActorData->moveSpeed >> 8) << 5;
                a = angle & 0xFFF;
                pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
                pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
                pSprite->moveSpeed = 0x4000000 / pActor->pActorData->moveSpeed;
            }
        } else if (!(pActor->pActorData->flags & 0x20000)) {
            scale = (0x80000 / pActor->pActorData->moveSpeed >> 8) << 5;
            a = angle & 0xFFF;
            pSprite->step.x = (rsin(a) * scale >> 12) * (s16)pActor->pActorData->scaleX;
            pSprite->step.z = (-(rcos(a) * scale) >> 12) * (s16)pActor->pActorData->scaleZ;
        } else {
            s32** ppModel = (s32**)&g_FieldActorModelPointers[pActor->pActorData->flags12C_0xE];

            pSprite->step.x = -(*ppModel)[0x128 / 4] << 16;
            pSprite->step.z = -(*ppModel)[0x130 / 4] << 16;
        }
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
extern s32 g_FieldActorModelPointerIndex;
extern u_char g_FieldPartyMoveInProgress;
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
   - g_FieldMaterialAngles is u_short[8] and g_FieldControl.angle is read unsigned (lhu
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
        func_80081F80(pSprite, angle, pFieldActor);
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
                pSprite->moveSpeed = 0;
            } else if (animationId == 2) {
                pSprite->moveSpeed = pSprite->field_0x82 * 0x60;
            } else {
                pSprite->moveSpeed = pSprite->field_0x82 * 0x30;
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
            func_80081F80(pSprite, pActorData->rotation.vx, pActor);
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
/* PARKED 74.01% (446 insns / 1784 bytes vs retail 451 / 1804). The whole shape is
   retail's: one switch over flags12C_0 with case 0 = the parent-matrix path, the
   RotMatrix/MulMatrix2/translation tail shared by cases 1-3 (needs the goto: three
   duplicated tails measure 66.81% / 502 insns because this cc1 does NOT cross-jump
   them), SetRotMatrix/SetTransMatrix, the command stream walk (0xC4/0xC8 skip,
   bit 8 = quad) and the min-height tail.
   Spelling that took 48.17 -> 62.02 and must stay: NO pFieldActor / pActor locals -
   every use spells g_FieldActors[actorIndex]... so the volatile table pointer is
   re-read, which is what retail does (it reloads the base + pActorData at each use
   and keeps pActor in $a1 only from the dispatch to case 1).
   Residuals measured this round:
   (1) the reference x/z: retail spills them with sw and reads them back with lw for
       the (x << 16) + z and with lhu only for the planePoint halfword stores; u16
       params give sh/lhu/lhu here. Tried u16 only so far.
   (2) actorIndex lands in s1 and pBounds in s5 here; retail has s5/s6.
   (3) retail's case bodies 2 and 3 reload the g_FieldActors chain (6 insns each)
       where case 1 reuses $a1 - a gcc CSE effect I could not reproduce by spelling.
   (4) the sxy[] stores and the first NormalClip's argument loads interleave in
       retail (store 0, load 0, store 1, load 1, store 2 in the jal slot); this body
       stores all three first. Same instruction count, different schedule.
   Not tried: int params, the (pCmd++)->indices[1] increment-form, permuter. */
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80083288);
/*
s32 func_80083288(int actorIndex, FieldActorModelBounds* pBounds, u16 refX, u16 refZ, s32* pHeight, VECTOR* pNormal) {
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
    switch (g_FieldActors[actorIndex].pActorData->flags12C_0) {
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
        goto rotate;
    default:
        pScratch->screenMatrix.t[2] = 0;
        pScratch->screenMatrix.t[1] = 0;
        pScratch->screenMatrix.t[0] = 0;
        pScratch->parentMatrix.t[2] = 0;
        pScratch->parentMatrix.t[1] = 0;
        pScratch->parentMatrix.t[0] = 0;
        CompMatrix(&g_Scene.worldRotationMatrix, &D_800AFC30, &pScratch->parentMatrix);
        if (g_FieldActors[actorIndex].pActorData->parentActorId != ACTOR_ID_INVALID) {
            CompMatrix(&pScratch->parentMatrix,
                &g_FieldActors[g_FieldActors[actorIndex].pActorData->parentActorId].childMatrix,
                &pScratch->workMatrix);
            pSourceMatrix = &pScratch->workMatrix;
        } else {
            pSourceMatrix = &pScratch->parentMatrix;
        }
        CompMatrix(pSourceMatrix, &g_FieldActors[actorIndex].transformMatrix, &pScratch->screenMatrix);
        break;
    }
    RotMatrix(&pScratch->rotation, &pScratch->workMatrix);
    MulMatrix2(&g_FieldActors[actorIndex].transformMatrix, &pScratch->workMatrix);
    pScratch->workMatrix.t[0] = g_FieldActors[actorIndex].transformMatrix.t[0];
    pScratch->workMatrix.t[1] = g_FieldActors[actorIndex].transformMatrix.t[1];
    pScratch->workMatrix.t[2] = g_FieldActors[actorIndex].transformMatrix.t[2];
    CompMatrix(&g_Scene.worldRotationMatrix, &pScratch->workMatrix, &pScratch->screenMatrix);
rotate:
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
    if (pScratch->minHeight != FIELD_POLY_HEIGHT_NONE) {
        *pHeight = pScratch->minHeight;
        FieldFreeScratchpad(sizeof(FieldPolyCheckScratch));
        return 0;
    }
    FieldFreeScratchpad(sizeof(FieldPolyCheckScratch));
    return -1;
}
*/
void FieldNoOp(void) {
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008399C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084158);

extern s32 D_800ADB98;
extern s32 D_800ADC0C;

/* 96.01% (276 bytes; retail is 276 too - the old note said 280, that was wrong).
   Every check matches. `new_var = pActor;` before the flags&2 test is the lever
   (it forces the copy into $a1 and takes the body 91.16 -> 96.01); moving the
   same assignment to the top of the function is 87.25/284, and dropping it is
   91.16. Residual, three spots: retail puts nop in the `beqz flags&2` delay slot
   where reorg steals our `move a1,a0`; retail keeps its `addiu v0,zero,-1` as a
   separate block (.L80084A34, reached by the `beq v1,v0` whose delay slot is a
   nop) where we thread `li v0,-1` into that delay slot; and retail threads
   `move v0,zero` into the `beqz flags&4` delay slot where we keep a zero block.
   The else-return-0 form and a nested if for the flags&2 test measure the same.
   Permuter run from 91.16 found the new_var copy (base 461 -> 407). */
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
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_8008492C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc8", func_80084A40);

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
    streamIndex = streamId;
    if (streamId == 0xFF) {
        return;
    }
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
