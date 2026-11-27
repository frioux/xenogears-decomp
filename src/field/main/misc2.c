#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "system/math.h"
#include "system/controller.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"

extern s16 g_FieldFogEnabled;
extern u_char g_FieldDirectionMasks[]; // Bit mask of each of the eight movement directions
extern u16 g_FieldP1ButtonsPressed; // Buttons pressed this frame (player 1)

void FieldMatrixResetTranslation(MATRIX *matrix) {
    matrix->t[2] = 0;
    matrix->t[1] = 0;
    matrix->t[0] = 0;
}

void FieldComputeSceneMatrices(void) {
    VECTOR scale;
    MATRIX _unused;
    MATRIX sp40;
    long flag;

    RotMatrix(&g_Scene.camRotation, &g_Scene.camRotationMatrix);
    FieldMatrixResetTranslation(&g_Scene.camRotationMatrix);
    CompMatrix(&g_Scene.camRotationMatrix, &g_Scene.viewMatrix, &sp40);
    FieldMatrixCopy(&g_Scene.viewMatrix, &sp40);
    RotMatrix(&g_Scene.worldRotation, &g_Scene.worldRotationMatrix);
    FieldMatrixResetTranslation(&g_Scene.worldRotationMatrix);
    RotMatrix(&g_Scene.worldRotation, &g_Scene.worldToScreenMatrix);
    MulMatrix2(&g_Scene.viewMatrix, &g_Scene.worldToScreenMatrix);
    SetRotMatrix(&g_Scene.viewMatrix);
    SetTransMatrix(&g_Scene.viewMatrix);
    RotTrans(&g_Scene.worldTranslation, &g_Scene.worldToScreenMatrix.t, &flag);
    scale.vx = g_WorldScale;
    scale.vy = g_WorldScale;
    scale.vz = g_WorldScale;
    ScaleMatrix(&g_Scene.worldToScreenMatrix, &scale);
    SetRotMatrix(&g_Scene.worldToScreenMatrix);
    SetTransMatrix(&g_Scene.worldToScreenMatrix);
}

// 85.50%: everything matches but one register. The original keeps the *address*
// &g_FieldActors in $s1 across the RotMatrix call and loads the pointer through it
// for the 2nd and 3rd actor base (`lw a1,0($s1)`, `lw a0,0($s1)`), which costs one
// callee-saved slot (frame 0x30; ours 0x28 and 156 bytes vs 164). gcc 2.7.2
// rematerialises the symbol address instead in every form tried: three block-scoped
// pointer locals off `&g_FieldActors[i]` (84.62%), the same with the first block
// written offset-first `(FieldActor*)(i * 0x5C + (int)g_FieldActors)` to get the
// original's `addu v1,s0,v1` operand order (84.38%), and a function-scope
// `FieldActor** ppActors = &g_FieldActors` read as `(*ppActors)[i]` (85.50%, the
// best, below): the pseudo is a constant symbol address, so it is rematerialised
// rather than saved. `long scale[3]` (not VECTOR) is required - the original
// stores the three scales with 32-bit `sw` at sp+0x10/0x14/0x18. The scale reads
// are signed (`lh`), so the u_short ActorData::scaleX/Y/Z are read through (s16).
// Also tried, all 85.50%: `FieldActor* volatile* ppActors`, offset-first
// `(FieldActor*)(actorIndex * 0x5C + (int)*ppActors)` in all three blocks, and
// `ppActors` assigned before the scale block. gcc folds the symbol address into a
// fresh %hi/%lo pair at each dereference in every form; retail's own `lui/addiu
// s1` says the base is a constant there, so this is not the runtime-base case.
// Not a preset question either (try_flags on the 85.50% form): -O3 85.25, -G4
// 85.25, gcc-2.6.0 84.62, gcc-2.7.2-cdk 85.25 - so the unit cannot be switched to
// cdk on this function's account. Same wall as func_8007254C and FieldRenderGearFrame
// below: every one of this unit's convertible stubs is parked on it.
// screened: Default 85.50 (best), cdk 85.50, 2.6.0 84.62 - not the preset; still the $s1 rematerialisation wall.

void FieldActorApplyRotationAndScale(int actorIndex) {
    long scale[3];
    FieldActor** ppActors;

    {
        FieldActor* pActor = (FieldActor*)(actorIndex * 0x5C + (int)g_FieldActors);
        scale[0] = (s16)pActor->pActorData->scaleX;
        scale[1] = (s16)pActor->pActorData->scaleY;
        scale[2] = (s16)pActor->pActorData->scaleZ;
    }
    ppActors = &g_FieldActors;
    {
        FieldActor* pActor = &(*ppActors)[actorIndex];
        RotMatrix(&pActor->rotation, &pActor->transformMatrix);
    }
    {
        FieldActor* pActor = &(*ppActors)[actorIndex];
        ScaleMatrix(&pActor->transformMatrix, (VECTOR*)scale);
    }
}
// 85.25%: two findings. (1) The three pointer loads the original does into
// $v1/$a1/$a0 come out of *block-scoped* pointer locals - one declaration
// assigned once in each of the three statements. A single function-scope local
// makes gcc reuse one base pseudo (71%). (2) The scale reads are signed: `lh`,
// not `lhu`, so the u_short ActorData::scaleX is read through a (s16) cast
// here; FieldParticleUpdateAndRender's `lhu` shows the field itself stays
// unsigned.
// What is missing is the original's $s1: it holds &g_FieldActors (lui/addiu
// then `lw 0($s1)`) for the second and third loads, costing one callee-saved
// slot (frame 0x30, ours 0x28); gcc rematerialises the constant address for
// each load instead; the only other difference is the operand order of the
// first block's `addu` (original `s0 + v1`, ours `v1 + s0`).
// A third form, `(FieldActor*)(actorIndex * 0x5C + (int)g_FieldActors)` in all three
// blocks, does fix the `addu` operand order but merges the 2nd and 3rd pointer loads
// (84.62%): two loads instead of the original's three.
// A fourth form - offset-first cast in block 1, `&g_FieldActors[actorIndex]` in blocks
// 2 and 3 (with the two `addu`s then coming out base-first, as the original has them) -
// is 84.38% and differs in nothing but $s1: `lui/lw g_FieldActors` twice where the
// original keeps &g_FieldActors in one callee-saved register across the RotMatrix call
// (frame 0x30 vs our 0x28). gcc rematerialises the symbol address instead of saving it,
// so the whole function hinges on defeating that rematerialisation.
/*
void FieldActorApplyRotationAndScale(int actorIndex) {
    VECTOR scale;

    {
        FieldActor* pActor = &g_FieldActors[actorIndex];
        scale.vx = (s16)pActor->pActorData->scaleX;
        scale.vy = (s16)pActor->pActorData->scaleY;
        scale.vz = (s16)pActor->pActorData->scaleZ;
    }
    {
        FieldActor* pActor = &g_FieldActors[actorIndex];
        RotMatrix(&pActor->rotation, &pActor->transformMatrix);
    }
    {
        FieldActor* pActor = &g_FieldActors[actorIndex];
        ScaleMatrix(&pActor->transformMatrix, &scale);
    }
}
*/

void FieldMatrixCreateWorldToScreen(void) {
    FieldComputeSceneMatrices();
    SetRotMatrix(&g_Scene.worldToScreenMatrix);
    SetTransMatrix(&g_Scene.worldToScreenMatrix);

    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_802815B0();
    }
}

int FieldFindClearDirectionForward(u_int mask, int dir) {
    int i;
    int offset;

    for (i = 0, offset = 0; i < 8; i++, offset++) {
        if (!(mask & g_FieldDirectionMasks[dir++ & 7])) {
            return offset;
        }
    }
    return 0;
}

int FieldFindClearDirectionBackward(u_int mask, int dir) {
    int i;
    int offset;

    for (i = 0, offset = 0; i < 8; i++, offset++) {
        if (!(mask & g_FieldDirectionMasks[dir-- & 7])) {
            return offset;
        }
    }
    return 0;
}

void FieldMathIntersectLines(SVec4* pA, SVec4* pB, SVec4* pOut) {
    VECTOR nA;
    VECTOR nB;
    VECTOR v;
    int denom;
    int t;

    v.vx = pA->z - pA->x;
    v.vy = 0;
    v.vz = pA->w - pA->y;
    VectorNormal(&v, &nA);

    v.vx = pB->z - pB->x;
    v.vy = 0;
    v.vz = pB->w - pB->y;
    VectorNormal(&v, &nB);

    denom = (nB.vx * nA.vz - nB.vz * nA.vx) >> 12;
    if (denom == 0) {
        t = 0;
    } else {
        t = ((pB->y - pA->y) * nA.vx - (pB->x - pA->x) * nA.vz) / denom;
    }
    pOut->x = pB->x + ((t * nB.vx) >> 12);
    pOut->y = pB->y + ((t * nB.vz) >> 12);
}

// PARKED 91.81% / 408 bytes - retail's exact size, so the shape is right and only one
// allocator decision is missing (see ROUND 146 at the end of this note). Two findings
// from the first round on this function:
// (1) THE PLAIN POINTER IS WORTHLESS: `u_char* pCam = (u_char*)&g_CamInterpolation`
//     scores 62.01% because gcc folds every post-call use back into a fresh
//     %hi/%lo pair (`sw $zero,%lo(g_CamInterpolation-0x104)($at)`), so the `$s0`
//     base retail keeps across the FieldMatrixSetIdentity call never forms.
// (2) TAKING THE POINTER'S ADDRESS (`pCamRef = &pCam`, then `*pCamRef`) defeats
//     that fold: 62.01 -> 91.81%, and the permuter's temp/type soup below reaches
//     95.08% (420 vs 408 bytes). What is left is structural, not an allocator tie:
//     the address-taken pointer costs retail's frame its spare slot (0x20 vs 0x18,
//     one `sw`/`lw` of pCam) and our base ends up in $v1 picked up from that slot
//     instead of retail's $s0 held across the call. Retail has NO spill slot, so
//     retail's source laundered nothing - it kept a plain constant address in a
//     callee-saved register, which 2.7.2 will not do for any plain-pointer shape
//     tried here. Next idea worth a shot: find a source form where the base is a
//     genuinely loaded value without a local's address being taken (a global
//     pointer datum would need new data, which this unit cannot add).
// Screened already: Default 62.01, cdk 65.16, 2.6.0 62.01 on the plain form.
// permuter runs: 62.01 -> 91.81 -> 94.64 -> 95.08, still falling slowly.
// ROUND 146: the body below is the clean spelling of that 91.81% shape and it now
// measures 91.81% / 408 bytes, i.e. EXACTLY retail's size - the older 95.08% note
// above came from an earlier tree (a permuter soup of the same shape compiles to
// byte-identical output today, so the difference was the headers, not the C). The
// address-taken-pointer launder is what makes the whole camera block address as
// `disp(base)` instead of `%lo(sym+K)($at)`; it costs one stack slot and one reload.
// Everything else is now instruction-for-instruction retail's: same 4 base insns
// (lui/addiu + sw/lw of the pointer), same offsets, same store order, same CSE of
// the 8 / 0x800 / 0x10000000 / 0x1000 constants, same $at-form for the three
// CameraInterpolation members at +0x4/+0x8/+0xA. What is left is ONE allocator
// decision: retail holds the base in a callee-saved $s0 (frame 0x18, saved in the
// prologue, restored in the epilogue) so that the pre-call `sw v0,0($s0)` and the
// post-call block share it; ours re-loads the spilled pointer into $v0 just after
// the jal (frame 0x20), which also swaps v0/v1 through the whole post-call block.
// cse cannot hoist that load above the FieldMatrixSetIdentity call (a call clobbers
// memory, and pCam's address escapes), and 2.7.2 will not put a *constant* symbol
// address in a callee-saved register - it rematerialises or spills instead. Every
// other launder (a second pointer fed from the first, an s32 holding the address, a
// one-field struct) costs a `lw` plus a save/restore, i.e. +8 bytes, so the size
// gate fails even when the shape is right.
// Preset: --sweep over gcc 2.6.0 / 2.7.2 / 2.7.2-cdk x -O1/-O2/-O3 x -G4/-G8 x
// -mcpu 3000/r8000 tops out at 66.27 (cdk -O1) on the plain form, so this is not a
// flag question. Permuter (round 146, 5 min, this body, base = the parked body)
// found nothing better: RESULT PERMUTER_NONE, best candidate 465 against our 408-byte
// base.
extern VECTOR g_CameraEye;

// round 396 re-measure: 91.81% / 408 bytes (retail's size, 102 vs retail's 103 insns). Nothing
// new: the address-taken launder is the reason the base never reaches $s0 - an address-taken
// local is a MEMORY variable to gcc, so it stays in the frame by construction (that is the
// extra frame slot and the lw after the jal), while the plain pointer (62.01%) is a constant
// pseudo gcc rematerialises per use. NEW this round: screened the LAUNDER body under other
// presets (the note above only screened the plain form) - gcc-2.6.0 -O2 -G0 gives 93.88, cdk
// compile-fails, 2.6.0 -O1 62.95, -O3 compile-fails. Not switchable: under 2.6.0 the unit's
// already-matched functions drop (FieldActorApplyRotationAndScale 99.12, FieldMathIntersectLines
// 98.11, FieldCameraUpdateFromTarget 99.81, FieldMatrixLookAt 95.50, FieldUpdateActorRenderModes
// 99.86), so there is no clean 93.88 -> 100 to chase here.
// ROUND 439, the diagnosis made mechanical (both forms re-measured on this tree):
// the parked body's residual is "MEM pseudo vs REG pseudo", visible as the prologue
// alone. Ours: frame 0x20, `lui a0/addiu a0` then `sw a0,16(sp)`, and after the jal
// `lw v0,16(sp)` - i.e. the base is the address-taken LOCAL, a stack slot, reloaded
// into a caller-saved register. Retail: frame 0x18, `sw s0,16(sp)` then `lui
// s0/addiu s0`, and every post-call store is `disp(s0)` - a REG pseudo global-alloc
// put in a callee-saved register. The plain pointer (62.01%) has retail's FRAME
// (0x18, no s0 slot) but re-forms `lui $at / sw %lo(g_CamInterpolation+K)($at)` for
// all 47 post-call stores (130 insns vs retail's 103), so 2.7.2's cse substitutes
// the constant address instead of keeping it in a register. Every launder tried so
// far moves the base from one bad place to the other; the untried class is a base
// whose VALUE the compiler cannot know (a pointer loaded from memory), which this
// unit cannot supply without new data - do not re-spend a lane on local pointer
// shapes, and note that gcc-2.6.0 -O2 -G0 gives 93.88 but breaks six matched
// neighbours (see above).
// ROUND 452 re-measured on this tree: 91.81% / 408 bytes (retail's exact size, 102 vs
// retail's 103 insns) - the note above is current, nothing to add on the pointer shapes.
// TWO NEW SPELLINGS MEASURED DEAD (the 'dead parameter carries the constant address'
// lever from world_map/misc5 does NOT transfer to a SYMBOL base): (1) declaring this
// function as 'void f(u_char* pCam)' and assigning 'pCam = (u_char*)&g_CamInterpolation;'
// and (2) the same with 'mem_addr' + a u_char* local - both fold the parameter back to a
// constant pseudo exactly like the plain local and score 62.01% / 520 bytes / 130 insns,
// i.e. the parameter pseudo is constant-propagated when the assigned value is a
// symbol_ref (that lever's win there was a literal const_int address, 0x1F8000A0).
// func_8007254C is called with no argument from field/main/misc3 func_..., so a
// one-parameter definition would also make this unit's signature disagree with misc3's
// extern; not worth it for nothing.
// round 465 re-measure: 91.81 percent / 408 bytes / 102 vs retail 103 insns - unchanged.
// ROUND 465 (redo lane) - five more spellings, all measured on this tree:
// (1) the array-alias lever from rounds 397/463 (extern u_char g_CamInterpolationTable[]
//     __asm__("g_CamInterpolation") then pCam = that) scores 62.01 / 520 bytes, i.e.
//     identical to the plain pointer: it does NOT fire here, the same context as the
//     round-411 negative - the address has to be live again after the call.
// (2) a bare literal address also folds: pCam = (u_char*)0x800AF984 gives lui $at,0x800b
//     plus disp($at) stores and no register base at all, so the residual is not
//     symbol-vs-const_int - 2.7.2 will not hold ANY constant address in a register for
//     a store.
// (3) preset screens on the parked body: -O1 -G0 58.09, -O3 -G0 91.81, -mcpu=r8000
//     -O2 -G0 91.81; the full sweep (G4/G8 x O1/O2/O3 x 2.6.0 / cdk / 2.7.2) tops out
//     at 64.72. Not a flags question.
// (4) THE PRECEDENT THAT DOES PRODUCE THE SHAPE, in this unit and already matched:
//     FieldCameraUpdateFromTarget holds pSceneDIP = &g_Scene.sceneDIP in $s0 across
//     THREE calls (rsin, rcos, FieldRotateEyeAboutAt) with the same negative disp
//     stores (-0x14C, -0x13C) off it. What makes it stick there is that the pointer's
//     VALUE is used, twice, for the derived pEye/pAt addius that are then passed as
//     arguments; a pointer whose only uses are memory operands is constant-propagated
//     into disp($at) per store. func_8007254C has exactly one value use of its base
//     (the pre-call jal argument addiu a0,s0,0x2C) and gcc materialises that in a
//     caller-saved register, so no callee-saved base ever forms. Retail's bytes contain
//     no second addiu off $s0, so the extra value use cannot be source-visible: this
//     reads as a frontend constant-folding blocker, not a spelling to find.
/*
void func_8007254C(void) {
    u_char* pCam = (u_char*)&g_CamInterpolation;
    u_char** pCamRef = &pCam;

    (void)pCamRef;

    *(s32*)(pCam + 0x0) = 8;
    g_CamInterpolation.eyeStepDistance = 8;
    g_Scene.sceneAngleStep = 0x400000;
    g_Scene.sceneAngleCurrent = 0x8000000;
    g_Scene.sceneAngle.vy = 0x800;
    g_Scene.shakeAmplitudeZ = 0;
    g_Scene.shakeAmplitudeY = 0;
    g_Scene.shakeAmplitudeX = 0;
    g_Scene.shakeStepZ = 0;
    g_Scene.shakeStepY = 0;
    g_Scene.shakeStepX = 0;
    g_Scene.shakeActive = 0;
    g_Scene.shakeStopWhenFinished = 0;
    g_Scene.flags = 0;
    g_CamInterpolation.curAngleY = 0;
    g_CamInterpolation.targetAngleY = 0;
    g_Scene.dollySet = 0;
    g_Scene.dollyStop = 0;
    g_Scene.sceneAngleFrames = 0;
    g_Scene.sceneAngle.vx = 0;
    g_Scene.sceneAngle.vz = 0;
    g_Scene.sceneAngleYTarget = 0x800;
    g_Scene.camRotation.vx = 0;
    g_Scene.camRotation.vy = 0;
    g_Scene.camRotation.vz = 0;
    FieldMatrixSetIdentity((MATRIX*)(pCam + 0x2C));

    *(s32*)(pCam - 0x104) = 0;
    g_CameraEye.vy = 0;
    g_CameraEye.vz = 0;
    *(s32*)(pCam - 0xE0) = 0x10000000;
    *(s32*)(pCam - 0xB0) = 0x10000000;
    *(s16*)(pCam + 0x78) = 0x1E;
    *(s32*)(pCam + 0x74) = 0x200;
    *(s32*)(pCam - 0xF4) = 0;
    *(s32*)(pCam - 0xF0) = 0;
    *(s32*)(pCam - 0xEC) = 0;
    *(s32*)(pCam - 0xE4) = 0;
    *(s32*)(pCam - 0xDC) = 0;
    *(s32*)(pCam - 0xA4) = 0;
    *(s32*)(pCam - 0xA0) = 0;
    *(s32*)(pCam - 0x9C) = 0;
    *(s32*)(pCam - 0xD4) = 0;
    *(s32*)(pCam - 0xD0) = 0;
    *(s32*)(pCam - 0xCC) = 0;
    *(s32*)(pCam - 0xC4) = 0;
    *(s32*)(pCam - 0xC0) = 0;
    *(s32*)(pCam - 0xBC) = 0;
    *(s32*)(pCam - 0xB4) = 0;
    *(s32*)(pCam - 0xAC) = 0;
    *(s16*)(pCam + 0x7C) = 0;
    *(s16*)(pCam + 0x8C) = 0;
    *(s16*)(pCam + 0x98) = 0;
    *(s16*)(pCam + 0x7A) = 0x1000;
    *(s16*)(pCam - 0x50) = 0;
    *(s16*)(pCam - 0x48) = 0;
    *(s16*)(pCam - 0x46) = 0;
    *(s16*)(pCam - 0x24) = 0;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_8007254C);





// The masks test the dolly byte as the LEFT operand and the two button blocks live
// inside the dolly if: that is what puts the shared `frames == 0` tail last and
// fills the `lh sceneAngle.vy` load delay with the dolly load (99.93 -> 100.00).
void FieldUpdateSceneDollyAngle(void) {
    int offset;

    if (g_Scene.dollySet != 0xFF && g_Scene.dollyStop != 0xFF) {
        if (g_Scene.sceneAngleFrames == 0) {
            if (g_Scene.dollySet & g_FieldDirectionMasks[((u_int)g_Scene.sceneAngle.vy & FIELD_ANGLE_MASK) >> 9]) {
                if (g_Scene.sceneAngleStep != -0x400000 && g_Scene.sceneAngleStep != 0x400000) {
                    g_Scene.sceneAngleStep = 0x400000;
                    g_Scene.sceneAngleYTarget += 0x200;
                }
                g_Scene.sceneAngleFrames = 8;
            }
            if (g_Scene.dollyStop & g_FieldDirectionMasks[((u_int)g_Scene.sceneAngle.vy & FIELD_ANGLE_MASK) >> 9]) {
                offset = FieldFindClearDirectionForward(g_Scene.dollyStop,
                    ((u_int)g_Scene.sceneAngle.vy & FIELD_ANGLE_MASK) >> 9);
                if (FieldFindClearDirectionBackward(g_Scene.dollyStop,
                        ((u_int)g_Scene.sceneAngle.vy & FIELD_ANGLE_MASK) >> 9) < offset) {
                    g_Scene.sceneAngleStep = -0x400000;
                    g_Scene.sceneAngleYTarget -= 0x200;
                } else {
                    g_Scene.sceneAngleStep = 0x400000;
                    g_Scene.sceneAngleYTarget += 0x200;
                }
                g_Scene.sceneAngleFrames = 8;
            }
        }

        if (g_FieldP1ButtonsPressed & 0x4) {
            if (!(g_Scene.flags & 0x8000) && g_Scene.sceneAngleFrames == 0) {
                if (!(g_Scene.dollyStop & g_FieldDirectionMasks[((g_Scene.sceneAngle.vy - 0x200) & FIELD_ANGLE_MASK) >> 9])) {
                    g_Scene.sceneAngleStep = -0x400000;
                    g_Scene.sceneAngleFrames = 8;
                    g_Scene.sceneAngleYTarget -= 0x200;
                }
            }
        }

        if (g_FieldP1ButtonsPressed & 0x8) {
            if (!(g_Scene.flags & 0x8000) && g_Scene.sceneAngleFrames == 0) {
                if (!(g_Scene.dollyStop & g_FieldDirectionMasks[((g_Scene.sceneAngle.vy + 0x200) & FIELD_ANGLE_MASK) >> 9])) {
                    g_Scene.sceneAngleStep = 0x400000;
                    g_Scene.sceneAngleFrames = 8;
                    g_Scene.sceneAngleYTarget += 0x200;
                }
            }
        }
    }

    if (g_Scene.sceneAngleFrames != 0) {
        g_Scene.sceneAngleCurrent += g_Scene.sceneAngleStep;
        g_Scene.sceneAngle.vy = g_Scene.sceneAngleCurrent >> 16;
        g_Scene.sceneAngleFrames--;
    }
    if (g_Scene.sceneAngleFrames == 0) {
        g_Scene.sceneAngle.vy = g_Scene.sceneAngleYTarget;
    }

    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_80284EA4();
    }
}
extern int func_8007CD80(VECTOR* pPos, SVECTOR* pPoints, SVec4* pOut);
extern void FieldRotateEyeAboutAt(VECTOR* pEye, VECTOR* pAt);
extern VECTOR g_CameraEye2;
extern VECTOR g_CameraAt2;
extern int g_FieldCameraAtYLatched;
extern u_char g_FieldCameraAtFollowTargetY;

// The non-void return type is load-bearing: with `void` gcc reorg steals the flag clear's
// `andi` into the final `bnez` delay slot (99.52%).
int FieldCameraUpdateFromTarget(VECTOR* pPos, int param) {
    SVECTOR points[2];
    SVec4 line;
    SVec4 cross;
    SVec4 result;
    VECTOR pos;
    // The original allocates 0x20 of stack the code never references (frame 0x80).
    MATRIX unusedMatrix;

    pos.vx = pPos->vx;
    pos.vy = 0;
    pos.vz = pPos->vz;

    if (func_8007CD80(&pos, points, &cross) == -1) {
        line.x = points[0].vx;
        line.y = points[0].vz;
        line.z = points[1].vx;
        line.w = points[1].vz;
        FieldMathIntersectLines(&line, &cross, &result);

        g_CameraAt2.vx = result.x << 16;
        g_CameraAt2.vz = result.y << 16;
        if (g_FieldCameraAtFollowTargetY == 0) {
            if (g_FieldCameraAtYLatched == 0) {
                g_CameraAt2.vy = param << 16;
                g_FieldCameraAtYLatched = 1;
            }
        } else {
            g_CameraAt2.vy = pPos->vy - 0x200000;
        }
    } else {
        g_CameraAt2.vx = pPos->vx;
        g_FieldCameraAtYLatched = 0;
        g_CameraAt2.vy = pPos->vy;
        g_CameraAt2.vz = pPos->vz;
        g_CameraAt2.vy = g_CameraAt2.vy - 0x200000;
    }

    {
        s16* pSceneDIP;
        VECTOR* pEye;
        VECTOR* pAt;
        s32 offset;

        pSceneDIP = &g_Scene.sceneDIP;
        pEye = (VECTOR*)((u_char*)pSceneDIP - 0x14C);
        pAt = (VECTOR*)((u_char*)pSceneDIP - 0x13C);
        offset = (-((rsin(((*pSceneDIP * 91) >> 3) + 0xC00) * (s32)g_Scene.sceneScrZ) << 5) >> 0x10) * g_Scene.sceneScale * 0x10;
        g_CameraEye2.vy = offset + g_CameraAt2.vy;
        offset = (((rcos(((*pSceneDIP * 91) >> 3) + 0xC00) * (s32)g_Scene.sceneScrZ) << 5) >> 0x10) * g_Scene.sceneScale * 0x10;
        g_CameraEye2.vz = offset + g_CameraAt2.vz;
        pEye->vx = pAt->vx;
        FieldRotateEyeAboutAt(pEye, pAt);
    }

    if (g_Scene.flags & 0x1) {
        if (g_Scene.sceneScaleFrames != 0) {
            g_Scene.sceneScaleCurrent += g_Scene.sceneScaleStep;
            g_Scene.sceneScale = g_Scene.sceneScaleCurrent >> 16;
        }
        if (--g_Scene.sceneScaleFrames == 0) {
            g_Scene.flags &= 0xFFFE;
        }
    }

    if (g_Scene.flags & 0x8) {
        s32 flags = g_Scene.flags;
        g_Scene.sceneDIPCurrent += g_Scene.sceneDIPStep;
        g_Scene.sceneDIP = g_Scene.sceneDIPCurrent >> 16;
        if (--g_Scene.sceneDIPFrames == 0) {
            g_Scene.flags = flags & 0xFFF7;
        }
    }
}


extern VECTOR g_CameraEye;
extern VECTOR g_CameraEye2;
extern VECTOR g_CameraAt;
extern VECTOR g_CameraAt2;
extern s32 g_FieldCameraShakeOffset;
extern s32 g_FieldCameraShakeOffsetY;
extern s32 g_FieldCameraShakeOffsetZ;
extern s32 g_FieldCameraSnapFrames;
extern int rand(void);

// A camera coordinate only takes a step when the step cannot overshoot it.
// The difference is spelled out instead of held in a local: retail computes the
// `subu` after the `beq` that compares the two high halves, i.e. in its delay slot.
#define FIELD_CAMERA_STEP(cur, tgt, step, stepSq)                       \
    {                                                                   \
        s32 value = (cur);                                              \
        s32 target = (tgt);                                             \
        if ((value >> 16) != (target >> 16) &&                          \
            (((target - value) >> 16) * ((target - value) >> 16)) >=     \
                (stepSq)) {                                             \
            (cur) = value + (target - value) / (step);                   \
        }                                                               \
    }

// PARK (round 166): 98.25%, and the compiled size now EQUALS the original's
// (1212 bytes = 303 instructions). Two things in the round-163 note were wrong:
//  - the body was not short of statements, it was short because the third clamp guard
//    reloads g_FieldCameraShakeOffsetZ where retail tests the product still sitting in $v1. Giving
//    ONLY that product a local (shakeZ) reproduces retail exactly, and the whole tail
//    from the +0xA2 amplitude load down is now line-for-line identical;
//  - the initial zeroing has to go through the GLOBALS (g_FieldCameraShakeOffset = 0;),
//    while the pointer local pShakeOffset must be assigned just before the three rand
//    products - that is what makes retail keep &g_FieldCameraShakeOffset in $s0 for the
//    later load/store through it while the earlier zeroing stays a macro store.
// What is left is one class of difference in three places: gcc keeps a ZERO-offset
// symbol address in a register (`lui/addiu $t0,&g_CameraEye` then lw+sw through it; the
// same for &g_Scene.shakeActive, which it then keeps live into the else-if branch) where
// retail folds the displacement into the load/store (`lh $v0,152($v0)`, `lui $at / sw
// $v0,%lo(...)($at)`). It also shifts the mult temporaries one register ($t1/$t2 here
// against retail's $t0/$t1). Naming the g_CameraEye word and the g_Scene +0x98 block as
// fields is a struct/field job in the shared header, not a C shape.
// permuter: base=531 best=426 (PERMUTER_BETTER, chained-assignment debris only).
// screened: Default 98.25 (best) - not a preset question.
// round 170 re-measured the park at 98.25% (1212 bytes, retail's size) and tried two
// more shapes, both of which keep the 98.25% or lose the tail:
//  - a `s32 result` local for the store (`(cur) = result;`) - byte-identical output;
//  - the whole +0x98 block spelled with byte indices (`*(s16*)&g_Scene.shakeActive[0x0]`)
//    as the neighbouring misc.c does - that makes gcc materialise &g_Scene.shakeActive in a
//    register for the guard too (`lh v0,0(v1)`), so it is worse; the array-cast form
//    above is the one to keep.
// round 185: 98.99 (1212 bytes, retail's size). The permuter closed the +0x98 half of
// the hoist: ONE `s16* pShake = (s16*)g_Scene.shakeActive;` local, assigned between the eye-vx
// and eye-vz steps and used ONLY by the `pShake[0] != 0` guard, makes retail's guard
// (`lui $v0 / lh %lo(v0)` straight off the symbol) come out right - the array-cast
// spelling for the same guard materialises the address and is 98.25. Assigning it
// anywhere else, or using it for the other +0x98 accesses, loses the tail again.
// What is left is the same hoist at ONE site: the eye-vx step (g_CameraEye, offset 0)
// keeps the symbol address in $t0 and reuses it for the store where retail forms the
// two accesses separately (`lui $a1 / lw %lo($a1)`, then `lui $at / sw %lo($at)`); the
// extra live address pushes the mult temporaries one register ($t1/$t2 against $t0/$t1).
// The offsets 4 and 8 of the same vector already match, so only the 0-offset spelling
// is wrong; nothing tried so far separates the load and store RTL, and a second
// permuter run found only a semantically invalid move (pShake assigned inside the vx
// step's if-body, 285).
// round 186 re-measured 98.99 and settled what the remaining difference is with a
// diagnostic: pointing ONLY this step's store at g_CameraEye2 (a different symbol)
// makes the vx load symbolic (lui $a1 / lw %lo($a1)) and makes the eye2 pair
// materialise a base - so gcc 2.7.2 cse shares every field address that is used twice
// (load and store), and retail's one symbolic load plus one symbolic store at offset 0
// is a reload tie, not reachable from a different lvalue spelling. Tried, each verified
// byte-identical to this body: a pointer local for the store, declared before the step
// and inside the if-body; a volatile store through the symbol; volatile on both
// accesses; the s32-array and long casts; a do-while around the store (that one only
// renumbers the base to $a3 and measures 98.93). Stop here unless the register allocator
// is understood better than this note.
// round 396: un-parked and re-measured = 98.99% / 1212 bytes (retail's size, 303 insns),
// unchanged. All 36 differing lines have ONE cause: the eye-vx step's g_CameraEye address
// (offset 0) is used twice (load + store) so cse keeps it in $t0, and the register it takes
// is the one retail uses for atStepSq ($t0), shifting every mult temporary. Retail forms
// both accesses symbolically (lui $a1 / lw $a1,%lo; lui $at / sw %lo($at)) - see the note
// above, do not re-try the lvalue spellings. Not a preset question (screened).
// round 396 NEW BEST 99.06% / 1212 bytes (retail's size, 303 insns). One change landed: the
// `pShake = (s16*)&g_Scene.shakeActive;` assignment moves UP, before the two step-square
// multiplies, instead of sitting between the eye-vx and eye-vz steps (98.99 - the round 185
// note above measured "assigning it anywhere else loses the tail" on an earlier tree; with
// this body the earlier assignment is worth 0.07). Everything still differing is the ONE
// cause from round 186: the eye-vx step's g_CameraEye offset-0 address is used twice (load
// and store) so cse keeps it in a register ($t1 now, $t0 before) and pushes the per-step
// square from $t1 to $t2; retail forms the two accesses symbolically. Permuter from this
// body (2 min, 812 iterations) found nothing: RESULT PERMUTER_NONE, best 285 = base 285.
// round 418: re-measured live = 99.06% / 1212 bytes = retail's exact size (303 insns), unchanged.
// All 34 differing lines are the eye-vx step. NEW evidence for why no spelling closes it, from
// cc1_probe of the step in isolation: this cc1 ALWAYS emits one shared `la $r,&g_CameraEye` for an
// offset-0 symbol address used twice (the load before the if and the store inside it), and then the
// load and the store both go through that one register, which takes $t1 - so the step square's
// `mflo` reads $t2 where retail has $t1 (that shift is 30 of the 34 lines). Retail instead has BOTH
// accesses symbolic (`lui $a1 / lw $a1,%lo(g_CameraEye)($a1)`, then `lui $at / sw $v0,%lo($at)`),
// i.e. maspsx's fold of TWO single-use la pairs - so its cse never made the address a pseudo.
// Probed and all still one `la`: the macro as written, the compound assignment
// (`g_CameraEye.vx += (target - g_CameraEye.vx) / step`), the s32-cast spelling
// (`*(s32*)&g_CameraEye` both sides), and a volatile store. Offsets +4 and +8 of the same vector
// already match, so only the 0-offset site is left, and it is not a preset question: cdk scores
// 69.05 on this body. Permuter from this body: 1130 iterations, RESULT PERMUTER_NONE, best 285 = base 285.
// round 431: re-measured 99.06% / 1212 bytes = retail's exact size (303 insns). The residual is
// NAMED now, by cc1_probe (field/main/misc2, both 2.7.2 and 2.6.0), not by another spelling hunt:
// retail's eye-vx pair is a BARE symbolic load (lui a1,%hi / lw a1,%lo(a1)) AND a bare symbolic
// store (lui at / sw %lo(at)), and this cc1 emits that pair only for a SCALAR-typed global.
// Every aggregate spelling - gEye.vx, *(s32*)&gEye, ((s32*)&gEye)[0], a single-use pointer
// local, a pointer local for the store, the compound assignment, a volatile load or store -
// makes cse materialise the address (la $t1,sym + lw 0(t1) + sw 0(t1), i.e. one extra
// instruction the assembler cannot fold), and the freed/taken register is the whole 34-line
// residual (mult temps shift t1 -> t2). Proved: with the two accesses spelled through a scalar
// symbol at g_CameraEye's address the instruction stream becomes IDENTICAL (1212 bytes, only the
// reloc name differs, objdiff 99.93). It cannot ship from this TU because a scalar extern
// clashes with the file's own 'extern VECTOR g_CameraEye' (cc1 exit 33; a block-scope shadow
// errors too), and an aliased scalar symbol would be an invented name for the vector's first
// word - the orchestrator's call, see this round's report.
// Screened again this round: 2.6.0 98.18, -O3 99.06, -O1 76.14, -O0 9.36, cdk -O2 compile-fail,
// cdk -O3 69.05 - not a preset question. Third permuter run from this body: NONE, base = best = 285.
/*
void func_80072D74(void) {
    s32 atStepSq;
    s32 eyeStepSq;
    s32* pSnapFrames;
    s32* pShakeOffset;
    s16* pShake;

    if (g_Scene.flags & FIELD_SCENE_INTERPOLATING_SCREEN_Z) {
        if (g_Scene.sceneScrZFrames != 0) {
            g_Scene.sceneScrZCurrent += g_Scene.sceneScrZStep;
            g_Scene.sceneScrZ = g_Scene.sceneScrZCurrent >> 16;
        }
        if (--g_Scene.sceneScrZFrames < 0) {
            g_Scene.flags &= 0xFFEF;
            g_Scene.sceneScrZFrames = 0;
        }
    }

    pSnapFrames = &g_FieldCameraSnapFrames;
    if (*pSnapFrames != 0) {
        g_CamInterpolation.atStepDistance = 1;
        g_CamInterpolation.eyeStepDistance = 1;
        (*pSnapFrames)--;
    }

    pShake = (s16*)&g_Scene.shakeActive;
    atStepSq = g_CamInterpolation.atStepDistance * g_CamInterpolation.atStepDistance;
    eyeStepSq = g_CamInterpolation.eyeStepDistance * g_CamInterpolation.eyeStepDistance;

    FIELD_CAMERA_STEP(g_CameraEye.vx, g_CameraEye2.vx, g_CamInterpolation.eyeStepDistance, eyeStepSq);
    FIELD_CAMERA_STEP(g_CameraEye.vz, g_CameraEye2.vz, g_CamInterpolation.eyeStepDistance, eyeStepSq);
    FIELD_CAMERA_STEP(g_CameraEye.vy, g_CameraEye2.vy, g_CamInterpolation.eyeStepDistance, eyeStepSq);
    FIELD_CAMERA_STEP(g_CameraAt.vx, g_CameraAt2.vx, g_CamInterpolation.atStepDistance, atStepSq);
    FIELD_CAMERA_STEP(g_CameraAt.vz, g_CameraAt2.vz, g_CamInterpolation.atStepDistance, atStepSq);
    FIELD_CAMERA_STEP(g_CameraAt.vy, g_CameraAt2.vy, g_CamInterpolation.atStepDistance, atStepSq);

    g_FieldCameraShakeOffset = 0;
    g_FieldCameraShakeOffsetY = 0;
    g_FieldCameraShakeOffsetZ = 0;
    if (pShake[0] != 0) {
        s32 shakeZ;

        if (g_Scene.shakeFrames != 0) {
            g_Scene.shakeAmplitudeX += g_Scene.shakeStepX;
            g_Scene.shakeAmplitudeY += g_Scene.shakeStepY;
            g_Scene.shakeAmplitudeZ += g_Scene.shakeStepZ;
        } else if (g_Scene.shakeStopWhenFinished != 0) {
            g_Scene.shakeAmplitudeZ = 0;
            g_Scene.shakeAmplitudeY = 0;
            g_Scene.shakeAmplitudeX = 0;
            g_Scene.shakeActive = 0;
            g_Scene.shakeStopWhenFinished = 0;
        }
        pShakeOffset = &g_FieldCameraShakeOffset;
        // retail reads the integer part with a halfword load, not a shift
        *pShakeOffset = rand() * ((s16*)&g_Scene.shakeAmplitudeX)[1];
        g_FieldCameraShakeOffsetY = rand() * ((s16*)&g_Scene.shakeAmplitudeY)[1];
        shakeZ = rand() * ((s16*)&g_Scene.shakeAmplitudeZ)[1];
        g_FieldCameraShakeOffsetZ = shakeZ;
        if (*pShakeOffset < 0) {
            *pShakeOffset = 0;
            g_Scene.shakeAmplitudeX = 0;
        }
        if (g_FieldCameraShakeOffsetY < 0) {
            g_FieldCameraShakeOffsetY = 0;
            g_Scene.shakeAmplitudeY = 0;
        }
        if (shakeZ < 0) {
            g_FieldCameraShakeOffsetZ = 0;
            g_Scene.shakeAmplitudeZ = 0;
        }
        if (g_Scene.shakeFrames > 0) {
            g_Scene.shakeFrames--;
        }
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80072D74);


extern s16 g_FieldCameraMode;
extern u16 g_CamMovementFlags;
extern s16 g_CamAtMovementDuration;
extern s16 g_CamEyeMovementDuration;
extern VECTOR g_CamAtMovementCurrent;
extern VECTOR g_CamAtMovementDelta;
extern VECTOR g_CamEyeMovementCurrent;
extern VECTOR g_CamEyeMovementDelta;
// Scalar spellings of existing storage: the words of the four movement vectors and the first
// word of g_CamInterpolation, reached by FieldCameraUpdate below. cc1 emits retail's bare
// `lui %hi`/`lw %lo(reg)` + `lui at`/`sw %lo(at)` pair only for a SCALAR symbol; every
// aggregate spelling (`g_V.vx`, `*(s32*)&g_V`, `((s32*)&g_V)[0]`, a pointer local)
// materialises the address instead, and the body then loses the match (90.85%).
extern long g_CamAtMovementCurrentVx __asm__("g_CamAtMovementCurrent");
extern long g_CamAtMovementCurrentVy __asm__("g_CamAtMovementCurrent+4");
extern long g_CamAtMovementCurrentVz __asm__("g_CamAtMovementCurrent+8");
extern long g_CamEyeMovementCurrentVx __asm__("g_CamEyeMovementCurrent");
extern long g_CamEyeMovementCurrentVy __asm__("g_CamEyeMovementCurrent+4");
extern long g_CamEyeMovementCurrentVz __asm__("g_CamEyeMovementCurrent+8");
extern long g_CamInterpolationAtStep __asm__("g_CamInterpolation");
extern u16 g_FieldPlayerActorIndex16;
extern s16 g_FieldWalkmeshTriCount;
extern s32 D_800ADBAC;
extern s32 D_800ADBB0;
extern int func_8007B1C4(int x, int z, int walkmeshId, SVECTOR* pOut, VECTOR* pNormal);
extern long FieldGetVec2Magnitude(long x, long z);

// Frames the dolly camera (mode 2) runs for, and the at/eye distance under which it settles.
#define FIELD_CAMERA_DOLLY_FRAME_COUNT 65
#define FIELD_CAMERA_DOLLY_SETTLE_DISTANCE 128

// The case-2 arm re-enters the middle of case 0's arm (retail jumps into the camera update
// from case 2 and falls into it from case 0), so that block is written once under a label.
void FieldCameraUpdate(void) {
    VECTOR pos;
    VECTOR normal;
    SVECTOR point;
    s32 atDistance;
    s32 eyeDistance;
    s16 duration;
    u16 flags;
    FieldActor* pActor;
    s16* pMode;
    s16* pCameraMode;
    u16* pFlags;

    pMode = &g_FieldCameraMode;
    switch (*pMode) {
    case 2:
        D_800ADBAC = 0;
        if (++D_800ADBB0 >= FIELD_CAMERA_DOLLY_FRAME_COUNT) {
            *pMode = 0;
        }
        goto updateCamera;
    case 0:
        D_800ADBB0 = 0;
        if ((D_800ADBAC & 3) == 0) {
            if (g_CamInterpolationAtStep < 9) {
                g_CamInterpolationAtStep = 8;
            } else {
                g_CamInterpolationAtStep -= 2;
            }
            if (g_CamInterpolation.eyeStepDistance < 9) {
                g_CamInterpolation.eyeStepDistance = 8;
            } else {
                g_CamInterpolation.eyeStepDistance -= 2;
            }
        }
        D_800ADBAC++;
    updateCamera:
        FieldUpdateSceneDollyAngle();
        pActor = (FieldActor*)(g_FieldPlayerActorIndex16 * sizeof(FieldActor) + (mem_addr)g_FieldActors);
        pos.vx = pActor->pActorData->position.vx;
        pos.vy = pActor->pActorData->position.vy;
        pos.vz = pActor->pActorData->position.vz;
        FieldCameraUpdateFromTarget(&pos, (s16)pActor->pActorData->curYPos);
        if (!(g_Scene.flags & 0x4000)) {
            func_8007B1C4((s16)(g_CameraEye2.vx >> 16), (s16)(g_CameraEye2.vz >> 16),
                g_FieldWalkmeshTriCount - 1, &point, &normal);
            if ((s16)(g_CameraEye2.vy >> 16) > point.vy) {
                g_CameraEye2.vy = point.vy << 16;
            }
        }
        pCameraMode = &g_FieldCameraMode;
        if (*pCameraMode == 2) {
            atDistance = FieldGetVec2Magnitude((s16)(g_CameraAt2.vx >> 16) - (s16)(g_CameraAt.vx >> 16),
                (s16)(g_CameraAt2.vz >> 16) - (s16)(g_CameraAt.vz >> 16));
            eyeDistance = FieldGetVec2Magnitude((s16)(g_CameraEye2.vx >> 16) - (s16)(g_CameraEye.vx >> 16),
                (s16)(g_CameraEye2.vz >> 16) - (s16)(g_CameraEye.vz >> 16));
            if (atDistance < FIELD_CAMERA_DOLLY_SETTLE_DISTANCE &&
                eyeDistance < FIELD_CAMERA_DOLLY_SETTLE_DISTANCE) {
                *pCameraMode = 0;
            }
        }
        break;
    case 1:
        flags = g_CamMovementFlags;
        D_800ADBAC = 0;
        D_800ADBB0 = 0;
        if (flags & FIELD_CAMERA_AT_MOVEMENT_ACTIVE) {
            duration = g_CamAtMovementDuration;
            if (duration != 0) {
                g_CamAtMovementCurrentVx += g_CamAtMovementDelta.vx;
                g_CamAtMovementCurrentVy += g_CamAtMovementDelta.vy;
                g_CamAtMovementCurrentVz += g_CamAtMovementDelta.vz;
            }
            duration--;
            g_CamAtMovementDuration = duration;
            if (duration == 0) {
                g_CamMovementFlags = flags & ~FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
            }
            g_CameraAt2.vx = g_CamAtMovementCurrent.vx;
            g_CameraAt2.vy = g_CamAtMovementCurrent.vy;
            g_CameraAt2.vz = g_CamAtMovementCurrent.vz;
        }
        pFlags = &g_CamMovementFlags;
        flags = *pFlags;
        if (flags & FIELD_CAMERA_EYE_MOVEMENT_ACTIVE) {
            duration = g_CamEyeMovementDuration;
            if (duration != 0) {
                g_CamEyeMovementCurrentVx += g_CamEyeMovementDelta.vx;
                g_CamEyeMovementCurrentVy += g_CamEyeMovementDelta.vy;
                g_CamEyeMovementCurrentVz += g_CamEyeMovementDelta.vz;
            }
            duration--;
            g_CamEyeMovementDuration = duration;
            if (duration == 0) {
                *pFlags = flags & ~FIELD_CAMERA_EYE_MOVEMENT_ACTIVE;
            }
            g_CameraEye2.vx = g_CamEyeMovementCurrent.vx;
            g_CameraEye2.vy = g_CamEyeMovementCurrent.vy;
            g_CameraEye2.vz = g_CamEyeMovementCurrent.vz;
        }
        break;
    }

    func_80072D74();
    g_Scene.sceneAngle.vy &= FIELD_ANGLE_MASK;
}

void FieldRotateEyeAboutAt(VECTOR* pEye, VECTOR* pAt) {
    MATRIX matSceneRotation;
    VECTOR distance;
    VECTOR transformed;

    PushMatrix();
    RotMatrix(&g_Scene.sceneAngle, &matSceneRotation);
    distance.vx = pAt->vx - pEye->vx;
    distance.vy = pAt->vy - pEye->vy;
    distance.vz = pAt->vz - pEye->vz;
    ApplyMatrixLV(&matSceneRotation, &distance, &transformed);
    pEye->vx = transformed.vx + pAt->vx;
    pEye->vz = transformed.vz + pAt->vz;
    PopMatrix();
}

// Widens the high half of each of the three words into the word itself
void FieldSignExtendHighHalves(long* pValues) {
    int value0 = ((s16*)pValues)[1];
    int value1 = ((s16*)pValues)[3];
    int value2 = ((s16*)pValues)[5];

    pValues[0] = value0;
    pValues[1] = value1;
    pValues[2] = value2;
}

// Computes a LookAt / View Matrix
// Eye: The position of the camera
// At: The point we're looking at
void FieldMatrixLookAt(MATRIX* pMatLookAt, VECTOR* pEye, VECTOR* pAt, VECTOR* pUp) {
    VECTOR vecWork;
    VECTOR zAxis;
    VECTOR xAxis;
    VECTOR yAxis;
    SVECTOR translation;

    // Forward direction (X-Axis) is defined by the line going from the eye and
    // the point we're looking at
    vecWork.vx = CONV_TO_GTE(pAt->vx - pEye->vx);
    vecWork.vy = CONV_TO_GTE(pAt->vy - pEye->vy);
    vecWork.vz = CONV_TO_GTE(pAt->vz - pEye->vz);
    
    yAxis.vx = pUp->vx;
    yAxis.vy = pUp->vy;
    yAxis.vz = pUp->vz;
    yAxis.vx >>= 0x10;
    yAxis.vy >>= 0x10;
    yAxis.vz >>= 0x10;
    
    VectorNormal(&vecWork, &zAxis);

    // Given a vector pointing in the upwards direction, we use the cross product
    // to get our right direction (Z-Axis)
    OuterProduct12(&yAxis, &zAxis, &vecWork);
    VectorNormal(&vecWork, &xAxis);

    // Finally, knowing our two other basis vectors a cross product between them
    // gives the remaining basis vector for the Y-Axis
    OuterProduct12(&zAxis, &xAxis, &vecWork);
    VectorNormal(&vecWork, &yAxis);
    
    pMatLookAt->m[0][0] = xAxis.vx;
    pMatLookAt->m[0][1] = xAxis.vy;
    pMatLookAt->m[0][2] = xAxis.vz;
    pMatLookAt->m[1][0] = yAxis.vx;
    pMatLookAt->m[1][1] = yAxis.vy;
    pMatLookAt->m[1][2] = yAxis.vz;
    pMatLookAt->m[2][0] = zAxis.vx;
    pMatLookAt->m[2][1] = zAxis.vy;
    pMatLookAt->m[2][2] = zAxis.vz;
    
    translation.vx = CONV_TO_GTE(pEye->vx) * 3;
    translation.vy = CONV_TO_GTE(pEye->vy) * 3;
    translation.vz = CONV_TO_GTE(pEye->vz) * 3;

    // We need to translate the eye position back to the origin first,
    // and combine the translation and rotation operation into a single matrix.
    ApplyMatrix(pMatLookAt, &translation, &vecWork);
    pMatLookAt->t[0] = -vecWork.vx;
    pMatLookAt->t[1] = -vecWork.vy;
    pMatLookAt->t[2] = -vecWork.vz;
}

int FieldMathInterpolateAngle(int from, int to, int delta) {
    int difference;
    int nextAngle;

    difference = PSX_ANGLE(from - to);

    // If we have less than 180 degrees difference we subtract the delta
    if (difference < PSX_DEGREES(180)) {
        nextAngle = from - delta;
        difference = PSX_ANGLE(nextAngle - to);
        if (difference >= PSX_DEGREES(180)) {
            nextAngle = to;
        }
        return PSX_ANGLE(nextAngle);
    } 
    
    // If our difference is greater than 180 degrees we add our delta
    nextAngle = from + delta;
    difference = PSX_ANGLE(nextAngle - to);
    if (difference < PSX_DEGREES(180)) {
        nextAngle = to;
    }
    return PSX_ANGLE(nextAngle);
}

// If 0, the angle will be updated in steps rather than directly set to the target
extern s32 g_FieldSkipCounter;

int FieldMathUpdateAngle(int curAngle, int targetAngle, int delta) {
    int result;
    if (g_FieldSkipCounter == 0) {
        result = FieldMathInterpolateAngle(curAngle, targetAngle, delta);
    } else {
        result = PSX_ANGLE(targetAngle);
    }
    return result;
}


// .rodata (must move to C with the function):
// const char D_8006FB04[0xC] = "MATRIX    ";

extern MATRIX D_800AFC30;
extern s32 D_800AFC44;
extern s32 D_800AFC48;
extern s32 D_800AFC4C;
extern MATRIX D_800AF85C;
extern s32 g_FieldCameraShakeOffset;
extern s32 g_FieldCameraShakeOffsetY;
extern s32 g_FieldCameraShakeOffsetZ;
extern SVECTOR g_FieldRotationVector;
extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern VECTOR g_CameraUp;
extern VECTOR g_CameraAt2;
extern VECTOR g_CameraEye2;
extern s32 D_800B00B4;
extern s16 g_FieldActorUpdateAngle;
extern u_char g_FieldDisableActorRender;
extern s32 g_FieldActiveActorCount;

extern void func_8008110C(void);
extern void FieldCameraUpdate(void);
extern void func_80281B00(const char* pName);
extern void func_800223B0(SpriteData* pSpriteData, s16 direction);
extern void SpriteSetDirection(SpriteData* pSpriteData, s16 direction);
extern long FieldGetVec2Magnitude(long x, long z);
extern const char D_8006FB04[];

// The stack is switched into the scratchpad around three calls ($sp saved at
// 0x1F8003FC, restored afterwards). Nothing in C makes gcc write $sp to memory, so
// those three spots are `asm` - the only asm authorised in this unit.
// PARK REPAIR (round 163): compiles again (the externs above are outside the block) and
// re-measures at exactly the 76.67% its old note claimed - 1136 bytes, frame 0x50 vs
// 0x48. Unparking it needs no other edit.
// Beyond the register wall below, two things block it:
//  - it owns D_8006FB04, the FIRST string of the unit .rodata, so the whole
//    FB04..FB80 region has to move to C with it (see the note above FieldRenderActors);
//  - its body switches $sp into the scratchpad around three calls (STACK_SWITCH): the
//    three `asm volatile` sequences below are the only way to emit that, and inline
//    asm is not allowed in game code, so this stub cannot become C at all.
// The code itself is parked on the same $s0 camera-base wall as func_8007254C: the
// original keeps ONE register ($s0 = &g_CameraAt+8) and reaches g_CameraEye.vx with
// -0x18($s0), g_CameraUp with +8($s0) and g_Scene.viewMatrix with +0xF8($s0).
// Spelling the base as `u_char* pCam = (u_char*)&g_CameraAt + 8` gets the lui/addiu
// and the offset-0 loads right, but gcc folds every NONZERO offset back into a fresh
// %hi/%lo base, which costs the frame and two instructions. Also still different: the
// updateAngle/targetAngle block is laid out on the other side (the mat-based branch
// first) and gcc reads rotation.vy/unk11E with lhu + sll/sra where the original is lh.
// screened: Default 76.67 (best), cdk 59.23, 2.6.0 76.16 - not a preset question.
// round 185: re-measured, the body BUILDS and reads 76.67% (1136 bytes against retail 1176) -
// so the sweep ERROR row was stale, not a compile break. It is still not shippable: the whole
// function switches $sp into the scratchpad around three calls (STACK_SWITCH) and the parked
// body emulates that with asm volatile, which game code may not use. Not a C job at all.
/*
void func_800739C0(void) {
    VECTOR eye;
    VECTOR at;
    int i;
    FieldActor* pActor;
    ActorData* pActorData;
    MATRIX* pView;
    VECTOR* pUp;
    u_char* pCam;
    s16 targetAngle;
    s16 updateAngle;

    func_8008110C();
    RotMatrix(&g_FieldRotationVector, &D_800AFC30);
    // &g_CameraAt.vz: the original reaches g_CameraEye.vx, g_CameraUp and
    // g_Scene.viewMatrix off this one base register, as displacements.
    pCam = (u_char*)&g_CameraAt + 8;
    pView = (MATRIX*)(pCam + 0xF8);
    pUp = (VECTOR*)(pCam + 8);
    D_800AFC4C = 0;
    D_800AFC48 = 0;
    D_800AFC44 = 0;

    g_CamInterpolation.curAngleY = ratan2(*(s32*)pCam - g_CameraEye.vz,
        g_CameraAt.vx - *(s32*)(pCam - 0x18)) - 0x400;
    g_CamInterpolation.targetAngleY = ratan2(g_CameraAt2.vz - g_CameraEye2.vz,
        g_CameraAt2.vx - g_CameraEye2.vx) - 0x400;
    D_800B00B4 = ratan2(FieldGetVec2Magnitude((g_CameraAt.vx - g_CameraEye.vx) >> 16,
            (g_CameraAt.vz - g_CameraEye.vz) >> 16),
        (g_CameraAt.vy - g_CameraEye.vy) >> 16);

    asm volatile("lui $t1, 0x1F80\n\tori $t1, $t1, 0x3FC");
    asm volatile("addu $t0, $t1, $zero\n\tsw $sp, 0($t0)\n\t"
                 "addiu $t0, $t0, -4\n\taddu $sp, $t0, $zero");
    FieldCameraUpdate();
    asm volatile("addiu $sp, $sp, 4\n\tlw $sp, 0($sp)");

    eye.vx = *(s32*)(pCam - 0x18);
    eye.vx += g_FieldCameraShakeOffset;
    eye.vy = g_CameraEye.vy;
    eye.vy += g_FieldCameraShakeOffsetY;
    eye.vz = g_CameraEye.vz;
    eye.vz += g_FieldCameraShakeOffsetZ;
    at.vx = g_CameraAt.vx;
    at.vx += g_FieldCameraShakeOffset;
    at.vy = g_CameraAt.vy;
    at.vy += g_FieldCameraShakeOffsetY;
    at.vz = *(s32*)pCam;
    at.vz += g_FieldCameraShakeOffsetZ;

    if (g_FieldSkipCounter != 0) {
        FieldMatrixLookAt(pView, &eye, &at, pUp);
        D_800AF85C = *pView;
    } else {
        *pView = D_800AF85C;
        FieldMatrixLookAt(&D_800AF85C, &eye, &at, pUp);
    }

    asm volatile("lui $t1, 0x1F80\n\tori $t1, $t1, 0x3FC\n\t"
                 "addu $t0, $t1, $zero\n\tsw $sp, 0($t0)\n\t"
                 "addiu $t0, $t0, -4\n\taddu $sp, $t0, $zero");
    FieldMatrixCreateWorldToScreen();
    asm volatile("addiu $sp, $sp, 4\n\tlw $sp, 0($sp)");

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pActor = (FieldActor*)(i * 0x5C + (int)g_FieldActors);
        pActorData = pActor->pActorData;
        if (!(pActor->status & 0xF40))
            continue;
        if (pActor->status & 0x20)
            continue;
        if (pActorData->flags & 0x100000)
            continue;
        if ((pActorData->flags & 0x600) == 0x200)
            continue;

        if (!(pActorData->scriptFlags.flags & 0x8000)) {
            if (pActorData->curWalkmeshTriMaterial & 0x200000) {
                updateAngle = 0x200;
                if (pActorData->scriptFlags.flags & 0x1800) {
                    updateAngle = (pActorData->flags & 0x2000) ? g_FieldActorUpdateAngle
                                                              : pActorData->updateAngle;
                    targetAngle = pActorData->rotation.vy;
                } else {
                    targetAngle = (((pActorData->curWalkmeshTriMaterial >> 11) - 2) & 7) << 9;
                }
            } else {
                updateAngle = (pActorData->flags & 0x2000) ? g_FieldActorUpdateAngle
                                                           : pActorData->updateAngle;
                targetAngle = pActorData->rotation.vy;
            }
            pActorData->rotation.vz = FieldMathUpdateAngle(pActorData->rotation.vz,
                targetAngle, updateAngle);
        }

        if (g_FieldDisableActorRender == 0) {
            if (!(pActorData->flags & 0x1000000)) {
                func_800223B0((SpriteData*)pActor->pSpriteData,
                    (s16)((u_short)g_CamInterpolation.curAngleY
                        + (u_short)pActorData->rotation.vz));
            } else {
                SpriteSetDirection((SpriteData*)pActor->pSpriteData, pActorData->rotation.vz);
            }
        }
    }

    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_80281B00(D_8006FB04);
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800739C0);

void FieldUpdateActorRenderModes(void) {
    int i;
    FieldActor* pActor;
    FieldActorModelData* pModel;

    for (i = 0; i < g_FieldNumActors; i++) {
        pActor = &g_FieldActors[i];
        if (pActor->status & 0x40) {
            continue;
        }

        pModel = pActor->pModelData;
        FieldActorModelComputeCenterAndSize(pModel);

        if (g_FieldFogEnabled != 0) {
            if (pActor->status & 0x10) {
                pModel->renderMode = 5;
            } else {
                pModel->renderMode = 4;
            }
        } else if (pActor->status & 0xC) {
            pModel->renderMode = 1;
        } else if (pActor->status & 0x4000) {
            pModel->renderMode = 3;
        } else if (pActor->status & 0x10) {
            pModel->renderMode = 2;
        } else {
            pModel->renderMode = 0;
        }
    }
}

void FieldClearAndSwapOTagInternal(void) {
    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD)
        asm("break 0x400");

    g_FieldCurRenderContextIndex = (g_FieldCurRenderContextIndex + 1) % 2;
    g_FieldCurRenderContext = &g_FieldRenderContexts[g_FieldCurRenderContextIndex];
    ClearOTagR(g_FieldCurRenderContext->ot3, 0x8);
}

void FieldClearAndSwapOTag(void) {
    FieldClearAndSwapOTagInternal();
    ClearOTagR(g_FieldCurRenderContext->ot1, 0x1000);
    if (g_FieldRenderContextUseOT2) {
        ClearOTagR(g_FieldCurRenderContext->ot2, 0x1000);
    }
}

void FieldMatrixCopy(MATRIX* dest, MATRIX* source) {
    FieldMatrixCopyTransform(dest, source);
    FieldMatrixCopyTranslation(dest, source);
}

void FieldMatrixCopyTranslation(MATRIX* dest, MATRIX* source) {
    dest->t[0] = source->t[0];
    dest->t[1] = source->t[1];
    dest->t[2] = source->t[2];
}

void FieldMatrixCopyTransform(MATRIX* dest, MATRIX* source) {
    dest->m[0][0] = source->m[0][0];
    dest->m[0][1] = source->m[0][1];
    dest->m[0][2] = source->m[0][2];
    dest->m[1][0] = source->m[1][0];
    dest->m[1][1] = source->m[1][1];
    dest->m[1][2] = source->m[1][2];
    dest->m[2][0] = source->m[2][0];
    dest->m[2][1] = source->m[2][1];
    dest->m[2][2] = source->m[2][2];
}

// 97.44% / 1432 bytes (retail is 1428: 0x80074108..0x8007469C, 357 insns). What it is: an
// 8-row sprite sheet fill from D_800AFC08 into D_800AFD24 (a row zeroed when
// g_Scene.dollyStop & D_800ADC24[i]), a RECT at 0x800B004C with w = 0x80 and
// LoadImage(&rect, D_800AFD24), SetGeomScreen(0x80), SetGeomOffset(0x10A, 0xA6), a
// FieldMatrixLookAt whose pEye is (0, g_CameraEye.vy - g_CameraAt.vy,
// -FieldGetVec2Magnitude(...) << 16) with a zero pAt, the identity/CompMatrix/MulMatrix*
// chain over the 0x10/0x30/0x50/0x70/0x90 stack matrices, and two
// g_FieldCompassDisabled / g_FieldSkipCounter / g_FieldDisableCompassRender guarded passes:
// FieldRenderQuad over the compass quad (slot 0x14) with matCoord, then
// FieldRenderQuadScreenRect plus 20 FieldRenderQuad over the D_800B06BC quads and 4 more
// from slot 0x15 with matProj. Tail patches a prim's top byte between
// D_800B1E00[g_FieldCurRenderContextIndex] and the context's ot3[0], then
// SetGeomOffset(0xA0, 0x70) and SetGeomScreen(g_Scene.sceneScrZ). Frame 0xF8.
//
// The structure is right - every call, branch, loop, stack slot, stack-argument store and
// the WHOLE first half (sheet fill, LoadImage, the camera vectors, LookAt, the CompMatrix
// chain) is instruction-identical. Residuals, all measured this round (round 340):
// 1. ONE INSTRUCTION OVER, still the blocker, and it can never link as it stands. The last
//    FieldRenderQuad loop (D_800B0FEC, i = 0x15..0x18) needs the per-iteration copy of the
//    induction pointer at the TOP of the body (`addu a1,s1,zero` preceded only by the loop
//    label, increment `addiu s1,s1,0x70` right after it, empty latch), which is what frees
//    the back-edge delay slot for the 0xFFFFFF `lui`. This build always produces the OTHER
//    loop.c form: a preheader initialiser plus the copy in the latch, so the copy takes the
//    delay slot and the `lui` moves after the loop - one instruction more than retail.
//    Diagnosed as loop.c's move_movables turning the per-iteration copy of a biv into a
//    SECOND biv; every spelling tried keeps the latch copy: `pQuad++` in the argument
//    (95.62, and its preheader order is wrong too), `pQuad` + a separate `pQuad++`
//    (95.13), `pCurrentQuad = pQuad++` (95.62), do-while with the increments in the body
//    (95.18), `goto` loop (95.56), `for (;;)` + if/else + break (94.82, 1436 bytes). The
//    BEST is the strength-reduced index `&D_800B0FEC[i - 0x15]`, which is the only spelling
//    that also reproduces retail's preheader order (`ori s0,0x15`; `ori s2,0x80D4`;
//    `lui/addiu s1`) - 97.44.
//    Round 341 sharpened that and measured four more shapes. The delta is EXACTLY ONE insn,
//    and it is the preheader initialiser of the second biv (`addu a1,s1,zero` before the
//    loop, paid for by the latch copy `addu a1,s1,zero` in the branch delay slot). Retail
//    has ONE `a1 = s1` at the head of the body and no initialiser. The copy itself cannot
//    go away: the pointer biv is bumped before the `jal`, so the argument needs its own
//    register. The three shapes settle the body order, which is the tell: retail is [copy,
//    `addiu a2,sp,0x10`, s1 bump, s0 bump], the index spelling is [a2, s1 bump, s0 bump]
//    plus the latch copy, and every pointer spelling is [s1 bump, a2, s0 bump] plus the
//    latch copy (96.28). Measured this round and no better: `pCurrentQuad = &D_800B0FEC[i -
//    0x15];` passed as the argument (byte-identical to the index form - cse merges it),
//    read-and-bump `&D_800B0FEC[i++ - 0x15]` with an empty for-increment (97.34; it moves
//    both bumps to the very top of the body, ahead of the a2), the round-318
//    guard-plus-bottom-test (`i = 0x15; if (i < 0x19) { do { ...; i++; } while (i < 0x19);`
//    - byte-identical to the for form), and `pQuad++` in the argument plus
//    `pCurrentQuad = pQuad; pQuad++;` in source order (96.28). So unlike round 338 the loop
//    CONSTRUCT is not a lever here: for, while, do-while and guard+bottom-test all
//    normalise to one shape. Not a preset question either - try_flags measured 2.6.0 at
//    97.44 (identical) and 2.7.2-cdk at 78.20. permute.sh cannot run on this function at
//    all: its parser rejects the file (`conversion to non-scalar type requested`, base.c
//    fails to compile), so the register-order search is unavailable - which is why this is
//    parked with the number instead of permuted.
// 2. The tail's three ot3[0] accesses: retail keeps the context in $a2 and the prim word in
//    $a1, with the 0xFFFFFF mask in $a3 (its `lui` filling the three guard delay slots) and
//    the D_800B1E00 base materialised BEFORE the index shifts. This build puts the context
//    in $a3, the mask in $t1 and the D_800B1E00 base after the shifts - same instruction
//    count, one register rotation over. `pRenderContext`/`pOverlayTag` locals are required
//    (inlining the global reloads it after the tag store, 3 extra insns). Same count.
// 3. SOLVED this round - do not re-derive: the camera base. Retail keeps ONE register
//    (`lui/addiu` of g_CameraEye + 0x4) and reaches the up vector as `addiu a3,s0,0x1C`
//    with NO relocation. A partial view struct declared in this file and based at
//    `&g_CameraEye.vy` reproduces it exactly (`FieldCameraVectors` below: eyeY at 0x0 is
//    g_CameraEye.vy, `up` at 0x1C is g_CameraUp). Writing g_CameraEye.vy / g_CameraUp by
//    name instead gives two separate symbol materialisations (the subobject-base wall).
// 4. SOLVED this round: the trigger-zone screen offsets. Retail reaches 0x800ADC34 as
//    g_pFieldTriggerZones + 0x40 (the symbol is at 0x800ADBF4) and reads it with `lh`, so
//    the source took `((s16 *)((u8 *)&g_pFieldTriggerZones + 0x40))[k]`; spelling them as
//    D_800ADC24[8 + k*2] links to the same bytes but objdiff can never agree on the reloc.
//    The masks above them stay D_800ADC24 (retail names that symbol). The counter starts at
//    0x10 because 0x700 / 0x70 = 16, so the trigger quads are D_800B06BC[0x10..0x13], and
//    D_800B0F7C / D_800B0FEC are the quads at D_800B06BC + 20 and + 21 (Quad is 0x70 bytes),
//    which is why their references carry no offset.
// 5. `li s2,0x80D4` and `addiu a2,sp,0x70` land one slot apart from retail inside both quad
//    loop preheaders (harmless reorder; the index spelling above fixes loop 3's).
//
// Host-side tip that made this round cheap: `.venv/bin/elfObjDisasm --functions <Func> -a
// build/src/field/main/misc2.c.o /tmp/out` disassembles the compiled object (relocations
// resolved, labels printed) without docker, so a loop's exact instruction order can be read
// instead of inferred from diff_func's side-by-side.
//
// Round 415 re-measured the parked body: 97.44% / 1432 bytes / 358 insns against retail's
// 357, so it still cannot link. The residual is confirmed as ONE instruction: loop 3's
// back-edge delay slot holds `move a1,s1` (a dead second-biv increment) where retail has
// the tail's `lui a3,0xFF0000`. Re-measured loop 3 spelled `pQuad = D_800B0FEC;` with
// `pQuad++` in the argument: 96.28% / 1432, the SAME +1 latch copy, plus the wrong
// preheader order (`lui/addiu s1` before `li s0,0x15`; retail has it after) - so the index
// spelling stays the best. NEW fact that narrows it: the trip count is NOT the trigger. The
// compass loop (loop 1) is the same construct, and giving it loop 3's bound (0x18, four
// iterations) still produces retail's body-top copy with a `nop` back-edge slot and NO
// latch copy. Our loop 1 and loop 3 compiled bodies are the same sequence ([move a1,s1,
// addiu s1, addiu a2, addiu s0]) yet only loop 3 gains the latch copy, and loop 3 is the
// only one whose fall-through is a duplicate `lui` that reorg could have taken for the slot.
// So the trigger is loop 3's context (the third guard's register state, which is also why
// the tail keeps the mask in $t1 where retail has $a3 - residual 2), not the loop
// construct. Source-side spellings of this loop are exhausted; next lead is the tail's
// register rotation, which shares that state.
extern long FieldGetVec2Magnitude(long x, long z);
extern void FieldRenderQuad(u_long* ot, Quad* pQuad, MATRIX* pMatTransform, int renderContext);
extern void FieldRenderQuadScreenRect(u_long* ot, Quad* pQuad, MATRIX* pMatTransform, int renderContext);
extern int g_FieldCurRenderContextIndex;
extern RenderContext* g_FieldCurRenderContext;
extern u_char g_FieldCompassDisabled;
extern s32 g_FieldDisableCompassRender;
extern s16 D_800ADB48; // Compass angle, interpolated towards D_800ADB4A every frame
extern s16 D_800ADB4A; // Compass angle the player actor's facing asks for
extern u16 D_800ADC24[]; // Eight dollyStop masks, then four x/y screen offsets for the compass
extern u16 D_800AFC08[]; // Sprite sheet whose rows are copied into D_800AFD24
extern u16 D_800AFD24[]; // Sprite sheet uploaded to VRAM once every direction mask is resolved
extern RECT D_800B004C; // LoadImage rect of the sprite sheet
extern Quad D_800B06BC[]; // Trigger zone quads, then the compass and screen overlay quads
extern Quad D_800B0F7C[]; // Compass quad
extern Quad D_800B0FEC[]; // Screen overlay quads
extern u8 D_800B1E00[];

// The camera vector block seen from the eye's y component: retail keeps that address in a
// register and reaches the up vector as +0x1C with no relocation, so the original source
// took both from one symbol.
typedef struct {
    s32 eyeY; // 0x00: g_CameraEye.vy
    s32 eyeZ; // 0x04
    s32 eyePad; // 0x08
    VECTOR at; // 0x0C: g_CameraAt
    VECTOR up; // 0x1C: g_CameraUp
} FieldCameraVectors;

#define FIELD_SPRITE_SHEET_ROW_COUNT 8
#define FIELD_SPRITE_SHEET_ROW_WIDTH 16
#define FIELD_SPRITE_SHEET_WIDTH 0x80
#define FIELD_OVERLAY_PRIM_SIZE 0xC0
#define FIELD_PRIM_WORD_MASK 0xFFFFFF
#define FIELD_PRIM_TOP_BYTE 0xFF000000

// Round 428 (park lane) re-measured the index spelling: 97.44% / 1432 bytes / 358 insns against
// retail's 357, so it still cannot link. The old note holds. New measurements, do not redo:
// - Retail's loop 2 DOES use the init+latch second biv (preheader 'addu a1,s5,zero', latch
//   'addu a1,s1,zero'), which is exactly what this build emits for loop 3 - so the shape is
//   not wrong by itself. Retail's loops 1 and 3 have the body-top copy instead, and only
//   loop 3 of ours gets the second biv.
// - Loop 3 spelled 'pQuad++' in the argument measures 96.28 and STILL gets the init+latch
//   biv, so it is strictly worse than the index spelling (it also puts 'la s1' before
//   'li s0,0x15' in the preheader). A FRESH pointer local for loop 3 ('pCurrentQuad') is
//   byte-identical to that, so the copy's pseudo identity is not the trigger.
// - The tail's base spelled '&D_800B1E00[i * FIELD_OVERLAY_PRIM_SIZE]' is byte-identical to
//   'D_800B1E00 + i * FIELD_OVERLAY_PRIM_SIZE': it does NOT move 'la D_800B1E00' ahead of the
//   index shifts where retail has it, so that spelling is not residual 2's lever either.
// Named cause of the link blocker: loop 3's argument copy becomes a second biv whose
// preheader initialiser is the +1 insn; the trigger is loop 3's register state (residual 2),
// not the loop construct.
// NOTE to whoever un-parks this body: the declarations below are live on purpose (the parked
// body needs them in scope to compile) EXCEPT g_FieldActors, which must stay undeclared here:
// include/field/actor.h has 'extern FieldActor* volatile g_FieldActors', and re-declaring it
// without the volatile makes the already-matched FieldAnyActorHasFlag8000 100 -> 108 bytes
// (check_bin.sh field catches it; diff_func cannot).
// Round 428: 97.44 / 1432 bytes re-measured with the declarations below live and g_FieldActors
// taken from actor.h (the file's own), so the number is what an un-park sees today.
// Round 441 (park lane): re-measured 97.44 / 1432 bytes / 358 insns against retail's 357, and the
// extra instruction is loop.c's derived biv for the loop-3 call argument in BOTH directions -
// cc1's own output (build/src/field/main/misc2.c.s) has `move $5,$17` before the loop label AND
// in the bne delay slot, while retail's loop 3 has ONE copy inside the body and the tail's
// `lui a3,0xff` in the slot. Three more loop-3 shapes measured, all 358 insns:
// - `i = 0x15; pQuad = D_800B0FEC; do { call(...pQuad...); pQuad++; i++; } while (i < 0x19);`
//   96.31 (also swaps $s0/$s1 roles).
// - `pQuad = D_800B0FEC; for (i = 0x15; i < 0x19; i++) { call(...pQuad...); pQuad++; }` 96.34.
// - `pCurrentQuad = &D_800B0FEC[i - 0x15];` as the loop body's FIRST statement: byte-identical to
//   the index form (copy-propagated away), 97.44.
// The preheader copy and the latch copy come as a pair in every spelling, so the pointer pseudos'
// identity and the loop construct are both ruled out - this is loop.c's decision alone.
// Preset leg closed on this body: gcc-2.7.2-cdk 78.20, gcc-2.6.0 97.44 (same shape), 2.7.2 -O3
// compile-fails, so the derived biv is not a compiler-version fact.
// Open question for the next lane: retail's loops 2 and 3 differ under the SAME compiler (loop 2
// keeps init+latch, loop 3 keeps only a body copy), and the one source difference between them is
// the `i - 0x15` additive offset in loop 3's subscript. That subtract is where the unmeasured
// cause is; nothing about the loop's spelling moves it.
// Round 454 (redo lane): un-parked, re-measured 97.44% / 1432 bytes / 358 insns against retail's
// 357 at index spelling. The residual is ONE instruction: retail's loop 3 copies its argument in
// the BODY (single 'addu a1,s1,zero' right after the loop label), ours has the same copy hoisted
// out - 'move a1,s1' in the preheader plus a re-copy in the bne delay slot after 'addiu s1,s1,0x70'
// - i.e. loop.c replaced the body movable with the init+latch pair. Only loop 3 does this; loops 1
// and 2 in this same body match retail (loop 1 body copy, loop 2 init+latch, exactly as retail).
// New measurements this round, do not redo:
// - TRIP COUNT IS NOT THE TRIGGER. Loop 1 has the SAME body and the SAME 'pQuad++' argument
//   spelling as loop 3 and keeps its body copy when its bound is changed to 0x16 (2 iterations,
//   358 insns) and to 0x18 (4 iterations, the same count as loop 3, 358 insns). So the movable's
//   savings/iteration-count analysis is ruled out; the decision depends on the loop's surrounding
//   RTL, not on the loop body, its bound or its position as a spelled loop.
// - A DIFFERENT COUNTER PSEUDO for loop 3 ('k' instead of the function-wide 'i', same index
//   spelling) still hoists: 96.42% / 1432 bytes (worse tail registers), so the shared counter
//   variable is ruled out too.
// permute.sh CANNOT run on this body: its parser rejects the preprocessed base
// ('/tmp/permuter_xxx.c:2041: conversion to non-scalar type requested'), so this park has had no
// permuter pass; a future lane would have to simplify the body before it can.
// Untried and unmeasured: nothing in the loop-3 spelling; the remaining question is what in the
// surrounding RTL (register pressure and the addresses live across loops 2 and 3) flips loop.c's
// move_movables for an otherwise identical loop.
// Round 467 (park lane): un-parked, re-measured 97.44% / 1432 bytes / 358 insns against retail's
// 357, and the +1 is exactly the round-454 residual: our loop 3 is [move a1,s1 in the preheader]
// [addiu a2,sp,0x10][addiu s1,s1,0x70][addiu s0,s0,1] with the copy repeated in the back-edge
// delay slot, retail is [addu a1,s1,zero at the loop label][addiu a2][bump][bump] with the tail's
// lui a3,0xFF0000 taken for that slot. New measurement, do not redo: the ONE-ARRAY spelling
// &D_800B06BC[i] for the third loop (D_800B0FEC IS D_800B06BC[0x15], i.e. 0x800B0FEC + Quad
// stride arithmetic, so this is the realistic form and its preheader only needs the same folded
// 0x800B0FEC immediate) measures 97.41% / 1432 / 358 - the same latch copy plus one reloc-name
// row, so the index ADDEND is not what flips loop.c either. That closes the last untried item and
// matches tools/rounds/levers.md (g-movable-copy-hoist-is-context-not-spelling and
// g-a-plus-one-in-the-loop-test-is-what-hoists-the-argument-copy): with a bare test, no addend and
// every pointer form hoisting, the decision is loop.c's and no source spelling reaches the
// body-copy form. NOTE for the next lane: residual 2 (the tail's $a3/$t1 mask rotation) CANNOT be
// the cause of residual 1 - loop optimization runs before register allocation, so the two are
// independent, and BOTH would have to be closed to reach 100.
/*
void func_80074108(void) {
    MATRIX matProj; // 0x10: projection matrix, t[2] holds the screen distance
    MATRIX matView; // 0x30
    MATRIX matWork; // 0x50
    MATRIX matCoord; // 0x70
    MATRIX matRot; // 0x90
    SVECTOR vecRot; // 0xB0
    VECTOR vecEye; // 0xB8
    VECTOR vecAt; // 0xC8
    FieldCameraVectors* pCamera;
    FieldActor* pActor;
    RenderContext* pRenderContext;
    Quad* pQuad;
    Quad* pCurrentQuad;
    u32* pOverlayTag;
    s16 nTargetAngle;
    u8 nDollyStop;
    int nMagnitude;
    int nAngle;
    int i;
    int k;

    nDollyStop = g_Scene.dollyStop;
    for (i = 0; i < FIELD_SPRITE_SHEET_ROW_COUNT; i++) {
        if (nDollyStop & D_800ADC24[i]) {
            for (k = 0; k < FIELD_SPRITE_SHEET_ROW_WIDTH; k++) {
                D_800AFD24[i * FIELD_SPRITE_SHEET_ROW_WIDTH + k] = 0;
            }
        } else {
            for (k = 0; k < FIELD_SPRITE_SHEET_ROW_WIDTH; k++) {
                D_800AFD24[i * FIELD_SPRITE_SHEET_ROW_WIDTH + k] = D_800AFC08[k];
            }
        }
    }

    D_800B004C.w = FIELD_SPRITE_SHEET_WIDTH;
    LoadImage(&D_800B004C, D_800AFD24);
    SetGeomScreen(FIELD_SPRITE_SHEET_WIDTH);
    SetGeomOffset(0x10A, 0xA6);

    pCamera = (FieldCameraVectors*)&g_CameraEye.vy;
    vecAt.vx = 0;
    vecAt.vy = 0;
    vecAt.vz = 0;
    vecEye.vx = 0;
    vecEye.vy = pCamera->eyeY - g_CameraAt.vy;
    nMagnitude = FieldGetVec2Magnitude((g_CameraEye.vx - g_CameraAt.vx) >> 16,
        (g_CameraEye.vz - g_CameraAt.vz) >> 16);
    vecEye.vz = -nMagnitude << 16;
    FieldMatrixLookAt(&matView, &vecEye, &vecAt, &pCamera->up);

    FieldMatrixSetIdentity(&matProj);
    matProj.t[2] = FIELD_SPRITE_SHEET_WIDTH;
    SetRotMatrix(&matProj);
    SetTransMatrix(&matProj);

    pActor = (FieldActor*)(g_FieldPlayerActorIndex16 * sizeof(FieldActor) + (mem_addr)g_FieldActors);
    nTargetAngle = (u16)pActor->pActorData->rotation.vy + ((u16)g_CamInterpolation.curAngleY + 0x400);
    D_800ADB4A = nTargetAngle;
    nAngle = FieldMathUpdateAngle(D_800ADB48, nTargetAngle, 0x40);

    D_800ADB48 = nAngle;
    vecRot.vx = 0;
    vecRot.vy = nAngle;
    vecRot.vz = 0;
    FieldMatrixResetTranslation(&matWork);
    RotMatrix(&vecRot, &matWork);
    MulMatrix2(&matView, &matWork);
    matWork.t[2] = 0x1000;
    CompMatrix(&matProj, &matWork, &matCoord);

    if (g_FieldCompassDisabled == 0 && g_FieldSkipCounter == 0 && g_FieldDisableCompassRender == 0) {
        pQuad = D_800B0F7C;
        for (i = 0x14; i < 0x15; i++) {
            FieldRenderQuad(g_FieldCurRenderContext->ot3, pQuad++, &matCoord,
                g_FieldCurRenderContextIndex);
        }
    }

    FieldMatrixSetIdentity(&matWork);
    MulMatrix2(&matView, &matWork);
    matWork.t[2] = 0x1000;
    CompMatrix(&matProj, &matWork, &matCoord);
    MulMatrix0(&matProj, &matWork, &g_Scene.worldScaleMatrix);
    SetRotMatrix(&matProj);
    SetTransMatrix(&matProj);
    FieldMatrixSetIdentity(&matWork);
    MulMatrix2(&g_Scene.viewMatrix, &matWork);
    matWork.t[2] = 0x1000;
    CompMatrix(&matProj, &matWork, &matCoord);
    FieldMatrixCopy(&matProj, &matCoord);

    vecRot.vx = 0x400;
    vecRot.vy = 0;
    vecRot.vz = 0;
    RotMatrix(&vecRot, &matRot);

    if (g_FieldCompassDisabled == 0 && g_FieldSkipCounter == 0 && g_FieldDisableCompassRender == 0) {
        for (i = 0x10; i < 0x14; i++) {
            FieldMatrixSetIdentity(&matWork);
            matWork.t[0] = ((s16 *)((u8 *)&g_pFieldTriggerZones + 0x40))[(i - 0x10) * 2];
            matWork.t[2] = ((s16 *)((u8 *)&g_pFieldTriggerZones + 0x40))[(i - 0x10) * 2 + 1];
            CompMatrix(&matProj, &matWork, &matCoord);
            FieldMatrixCopyTransform(&matCoord, &matRot);
            FieldRenderQuadScreenRect(g_FieldCurRenderContext->ot3, &D_800B06BC[i], &matCoord,
                g_FieldCurRenderContextIndex);
        }

        for (i = 0; i < 0x10; i++) {
            FieldRenderQuad(g_FieldCurRenderContext->ot3, &D_800B06BC[i], &matProj,
                g_FieldCurRenderContextIndex);
        }

        for (i = 0x15; i < 0x19; i++) {
            FieldRenderQuad(g_FieldCurRenderContext->ot3, &D_800B0FEC[i - 0x15], &matProj,
                g_FieldCurRenderContextIndex);
        }
    }

    pRenderContext = g_FieldCurRenderContext;
    pOverlayTag = (u32*)(D_800B1E00 + g_FieldCurRenderContextIndex * FIELD_OVERLAY_PRIM_SIZE);
    *pOverlayTag = (*pOverlayTag & FIELD_PRIM_TOP_BYTE) |
        (pRenderContext->ot3[0] & FIELD_PRIM_WORD_MASK);
    pRenderContext->ot3[0] = (pRenderContext->ot3[0] & FIELD_PRIM_TOP_BYTE) |
        ((u32)pOverlayTag & FIELD_PRIM_WORD_MASK);

    SetGeomOffset(0xA0, 0x70);
    SetGeomScreen(g_Scene.sceneScrZ);
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80074108);



// Returns 1 if an actor wants its position reported to the script VM
int FieldAnyActorHasFlag8000(void) {
    int i;
    FieldActor* pActors;

    for (i = 0; i < g_FieldNumActors; i++) {
        // The actor array is read once, not per iteration
        pActors = *(FieldActor**)&g_FieldActors;
        if (!(pActors[i].status & 0x40) && (pActors[i].status & 0x8000)) {
            return 1;
        }
    }
    return 0;
}

extern u16 g_FieldP2ButtonsPressed; // Buttons pressed this frame (player 2)
extern u16 g_FieldP1ButtonsReleased; // Buttons released this frame (player 1)
extern u16 g_FieldP2ButtonsReleased; // Buttons released this frame (player 2)
extern u16 g_FieldButtonsPressedOnce; // Buttons pressed this frame, first press only (player 1)
extern u16 g_FieldP2ButtonsPressedOnce; // Buttons pressed this frame, first press only (player 2)
extern u16 g_FieldButtonPollMask; // Mask of buttons polled by the field
extern s32 g_FieldMapChangeGate;
extern int g_FieldMouseState[];
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern void FieldGetMouseState(int controllerIndex, int* pOut);

void FieldPollControllers(void) {
    g_FieldP1ButtonsPressed = 0;
    g_FieldP2ButtonsPressed = 0;
    g_FieldP1ButtonsReleased = 0;
    g_FieldP2ButtonsReleased = 0;
    g_FieldButtonsPressedOnce = 0;
    g_FieldP2ButtonsPressedOnce = 0;

    while (ControllerPopState()) {
        g_FieldP1ButtonsPressed |= g_C1ButtonState & g_FieldControl.controllerBtnMask;
        g_FieldP2ButtonsPressed |= g_C2ButtonState;
        g_FieldP1ButtonsReleased |= g_C1ButtonStateReleased & g_FieldControl.controllerBtnMask;
        g_FieldP2ButtonsReleased |= g_C2ButtonStateReleased;
        g_FieldButtonsPressedOnce |= g_C1ButtonStatePressedOnce & g_FieldControl.controllerBtnMask;
        g_FieldP2ButtonsPressedOnce |= g_C2ButtonStatePressedOnce;
    }

    g_FieldP1ButtonsPressed &= g_FieldButtonPollMask;
    g_FieldP1ButtonsReleased &= g_FieldButtonPollMask;
    g_FieldButtonsPressedOnce &= g_FieldButtonPollMask;

    ControllerResetState();
    FieldGetMouseState(1, g_FieldMouseState);

    if (g_FieldSkipCounter != 0) {
        g_FieldP1ButtonsPressed = 0;
        g_FieldP2ButtonsPressed = 0;
        g_FieldP1ButtonsReleased = 0;
        g_FieldP2ButtonsReleased = 0;
        g_FieldButtonsPressedOnce = 0;
        g_FieldP2ButtonsPressedOnce = 0;
    }

    if (g_FieldMapChangeGate == 0) {
        g_FieldP1ButtonsReleased &= 0xFF7F;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", FieldRenderModels);

extern s32 g_FieldDisableGearRender;
extern s32 g_FieldActorModelSlotCount;
extern u8 g_FieldBackgroundR;
extern u8 g_FieldBackgroundG;
extern u8 g_FieldBackgroundB;
// .rodata: this function's stub owns D_8006FB1C, the unit's HDD frame profiler tag.
const char D_8006FB1C[0xC] = "GEAR      ";

// The second argument is the s32* base of the global 0x48 bytes below
// g_FieldActorModelSlotCount, which has no symbol of its own yet.
void FieldRenderGearFrame(void) {
    s32* pGear;

    if (g_FieldDisableGearRender == 0) {
        pGear = &g_FieldActorModelSlotCount;
        if (*pGear != 0) {
            FieldUpdateMotionSlots();
            SetBackColor(g_FieldBackgroundR, g_FieldBackgroundG, g_FieldBackgroundB);
            LibGearRenderFrame(&g_Scene.worldToScreenMatrix, (u8*)(pGear - 18),
                &g_FieldCurRenderContext->ot1, g_FieldCurRenderContextIndex, 1);
        }
        if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
            func_80281B00(D_8006FB1C);
        }
    }
}

extern u_char g_FieldDisableActorRender;
extern s32 g_FieldActiveActorCount;
// .rodata: this stub owns D_8006FB28, the HDD frame profiler tag (see FieldRenderGearFrame).
const char D_8006FB28[0xC] = "CHAR      ";

// The per-actor animation tick is wrapped in a `do { } while (0);` on purpose: it
// makes cc1 schedule the call as its own block, which is what keeps pActor in $a1
// and pActor->pActorData in $a2 as the original does. Written as a plain statement
// the body ties at 99.60% on the same 400 bytes (GIV_TIE, $a1/$a2 swapped).
void FieldRenderActors(void) {
    FieldActor* pActor;
    int i;

    if (g_FieldDisableActorRender == 1) {
        return;
    }

    GfxSetRenderContext(g_FieldCurRenderContextIndex);
    GfxSetCurrentOT(g_FieldCurRenderContext->ot1);
    GfxSetBaseMatrix(&g_Scene.worldToScreenMatrix);
    SpriteAnimFrameListFlush();
    WorkListUpdate();
    TimerWorkListUpdate();
    func_80075B44(g_FieldCurRenderContext->ot1, g_FieldCurRenderContextIndex);

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pActor = (FieldActor*)(i * sizeof(FieldActor) + (mem_addr)g_FieldActors);
        // The original reads status as a 32-bit word
        if ((*(u_int*)&pActor->status & 0x60) == 0x40) {
            if ((pActor->pActorData->flags & 0x600) == 0x200) {
                continue;
            }
            if (pActor->pActorData->flags & 0x1000) {
                continue;
            }
            if (pActor->pActorData->scriptFlags.fields.scriptFlags_0x0) {
                continue;
            }
        } else if (!(pActor->pActorData->flags & 0x1000000)) {
            continue;
        }
        do {
            AnimScriptTick((SpriteData*)pActor->pSpriteData);
        } while (0);
    }

    func_800764B4(g_FieldCurRenderContext->ot1, g_FieldCurRenderContextIndex);

    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_80281B00(D_8006FB28);
    }
}


void FieldAddPrimitives(u_long* ot, u_long* pPrimList, int size) {
    AddPrims(ot, pPrimList + size, pPrimList);
}

extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern s16 g_FieldScreenEffectEnabled;
extern s32 g_FieldScreenEffectSuppressed;
extern s32 g_FieldScreenEffectData;
extern s16 g_FieldScriptRenderContextIndex;

void FieldRenderScreenEffect(void) {
    SVec4 eye;
    SVec4 at;

    if (g_FieldScreenEffectEnabled == 0) {
        return;
    }
    if (g_FieldScreenEffectSuppressed != 0) {
        return;
    }

    eye.x = CONV_TO_GTE(g_CameraEye.vx);
    eye.y = CONV_TO_GTE(g_CameraEye.vy);
    eye.z = CONV_TO_GTE(g_CameraEye.vz);
    at.x = CONV_TO_GTE(g_CameraAt.vx);
    at.y = CONV_TO_GTE(g_CameraAt.vy);
    at.z = CONV_TO_GTE(g_CameraAt.vz);

    GfxProjectQuadBanks(g_FieldScreenEffectData, &eye, &at, (MATRIX*)((u_char*)&g_CameraEye + 0x1E4),
        (int*)(g_FieldScriptRenderContextIndex * 4 + (int)g_FieldCurRenderContext + 0x40CC),
        g_FieldCurRenderContextIndex);
}

// Also switches the stack into the scratchpad (0x1F8003FC) around its
// FieldRenderModels call; see the note above func_800739C0.
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_8007554C);

extern s32 g_FieldFrameBufferIndex;

void FieldRenderFrame(void) {
    RECT rect;

    FieldPollControllers();
    func_800A2030();
    FieldTextBoxUpdate();
    func_8008004C(g_FieldCurRenderContext->ot3, g_FieldCurRenderContextIndex);
    DrawSync(0);
    Vsync(0);

    rect.x = SCREEN_WIDTH;
    rect.w = SCREEN_WIDTH;
    rect.h = SCREEN_HEIGHT;
    rect.y = ((g_FieldFrameBufferIndex + 1) & 1) << 8;
    MoveImage(&rect, 0, g_FieldCurRenderContextIndex << 8);

    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(g_FieldCurRenderContext);
    DrawOTag(&g_FieldCurRenderContext->ot3[7]);
}


// This unit owns 0x8006FB04..0x8006FB80 of .rodata (0x7C bytes, per field.yaml):
// nine 0xC-byte strings plus the VECTOR below. Each string is the dlabel at the top of
// the .s of the stub that follows it, and a C const takes that stub's place in the
// layout when it is defined between the same two functions, so the stubs convert
// independently - one at a time is fine, address order is not required.
const VECTOR g_FieldFrameReferenceAxis = { 0, 0, 0x1000, 0 };

// Builds the rotation of pMat as a frame orthonormal to pVec: the normal of the
// cross product of the reference axis with pVec, pVec itself, then their cross
// product.
void FieldMatrixBuildRotationFromVector(MATRIX* pMat, VECTOR* pVec) {
    VECTOR axis;
    VECTOR normal;
    VECTOR cross;

    axis = g_FieldFrameReferenceAxis;
    OuterProduct12(&axis, pVec, &cross);
    VectorNormal(&cross, &normal);
    OuterProduct12(&normal, pVec, &cross);
    VectorNormal(&cross, &axis);

    pMat->m[0][0] = normal.vx;
    pMat->m[0][1] = normal.vy;
    pMat->m[0][2] = normal.vz;
    pMat->m[1][0] = pVec->vx;
    pMat->m[1][1] = pVec->vy;
    pMat->m[1][2] = pVec->vz;
    pMat->m[2][0] = axis.vx;
    pMat->m[2][1] = axis.vy;
    pMat->m[2][2] = axis.vz;
}

void FieldSetSpriteColorRGB(SpriteData* pSpriteData, u_char* pColor) {
    if (g_FieldFogEnabled == 0) {
        SpriteSetColor(pSpriteData, pColor[0], pColor[1], pColor[2]);
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80075B44);

extern s32 g_FieldP2RenderToggle;
extern s32 g_FieldSceneReloadFlags;
extern s32 g_GfxOTDepthShift;

// 99.52%: retail exact 1472 bytes / 362 insns, whole stream instruction-identical,
// 64 differing lines and every one of them a callee-saved register NAME. The only
// wrong hand-out is &screenMatrix: retail has screenMatrix=$s3, crossResult=$s4,
// i=$s5, acc=$s6, mask=$s7 where this body gets crossResult=$s3, i=$s4, acc=$s5,
// mask=$s6, screenMatrix=$s7 - everything but screenMatrix in retail order, so one
// allocno (pseudo 143) has to overtake pseudo 116 and nothing else may move.
// MEASURED with -da -dl -dg on this body (gcc-2.7.2-psx -O2 -G0): "16 regs to
// allocate: 80 99 269 98 76 181 172 163 116 81 271 248 143 267 73 72", dispositions
// 116 in 19 (s3), 81 in 20, 271 in 21, 248 in 22, 143 in 23, and -dl gives
// crossResult 9 refs across 196 insns against screenMatrix 7 refs across 195, i.e.
// floor_log2(refs)*refs*10000*4/live = 5510 against 2871. screenMatrix needs 9 refs
// (3*9*40000/195 = 5538, a 0.5% win) or a live range under 101 insns; its live
// range is the whole loop body and its 7 refs are its def plus the offset-0 uses
// only (column 0 three halfword stores, &screenMatrix at ScaleMatrix and at
// SetRotTransMatrix - columns 1/2 fold to sp+0x7A/sp+0x7C and add none), so two
// more references are two more instructions and retail has none to spare. PARK on
// the arithmetic (see the allocno-quotient levers), do not re-permute.
// Measured dead here: permuter 4 min / 1567 iterations base=175 best=175 (the
// parser DOES accept this GTE body - the previous note was wrong about that),
// cdk 87.32 and 2.6.0 91.69 (so not a preset), `otz >>= ...` as one statement,
// a do/while around the scale block, otz/scaleX/scaleY/scaleZ declared elsewhere,
// `long` scale products, `int i` first, the two MATRIX declarations swapped (99.47).
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800764B4);
/*
void func_800764B4(u_long* pOtBase, s32 renderContextIndex) {
    VECTOR opVec;
    VECTOR crossX;
    VECTOR crossResult;
    MATRIX actorMatrix;
    MATRIX screenMatrix;
    MATRIX screenMatrixCopy;
    VECTOR scale;
    long perspective;
    long flag;
    FieldActor* pActor;
    s32 scaleX;
    s32 scaleY;
    s32 scaleZ;
    s32 otz;
    int i;

    screenMatrixCopy = g_Scene.worldToScreenMatrix;

    if (g_FieldP2RenderToggle != 0) {
        return;
    }

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pActor = (FieldActor*)((mem_addr)g_FieldActors + i * sizeof(FieldActor));
        if ((*(u_int*)&pActor->status & 0x60) != 0x40) {
            continue;
        }
        if (pActor->pActorData->flags & 0x102200) {
            continue;
        }
        if (pActor->pActorData->flags & 0x800) {
            continue;
        }
        if (pActor->pActorData->scriptFlags.flags & 0x10000) {
            continue;
        }
        if (pActor->pActorData->curWalkmeshTriMaterial & 0x200002) {
            continue;
        }

        opVec.vx = 0;
        opVec.vy = 0;
        opVec.vz = 0x1000;
        gte_ldopv1_gpr(&opVec);
        gte_ldopv2(&pActor->pActorData->curTriNormal);
        gte_nop();
        gte_nop();
        gte_op12_b();
        gte_stlvnl(&crossResult);
        VectorNormal(&crossResult, &crossX);

        gte_ldopv1_gpr(&crossX);
        gte_ldopv2(&pActor->pActorData->curTriNormal);
        gte_nop();
        gte_nop();
        gte_op12_b();
        gte_stlvnl(&crossResult);
        VectorNormal(&crossResult, &opVec);

        actorMatrix.m[0][0] = crossX.vx;
        actorMatrix.m[0][1] = crossX.vy;
        actorMatrix.m[0][2] = crossX.vz;
        actorMatrix.m[1][0] = pActor->pActorData->curTriNormal.vx;
        actorMatrix.m[1][1] = pActor->pActorData->curTriNormal.vy;
        actorMatrix.m[1][2] = pActor->pActorData->curTriNormal.vz;
        actorMatrix.m[2][0] = opVec.vx;
        actorMatrix.m[2][1] = opVec.vy;
        actorMatrix.m[2][2] = opVec.vz;
        actorMatrix.t[0] = pActor->transformMatrix.t[0];
        actorMatrix.t[1] = ((SpriteData*)pActor->pSpriteData)->field_0x84;
        actorMatrix.t[2] = pActor->transformMatrix.t[2];

        gte_SetRotMatrixRows_gpr(&g_Scene.worldToScreenMatrix);
        gte_ldclmv(&actorMatrix.m[0][0]);
        gte_mvmva(1, 0, 3, 3, 0);
        gte_stclmv(&screenMatrix.m[0][0]);
        gte_ldclmv(&actorMatrix.m[0][1]);
        gte_mvmva(1, 0, 3, 3, 0);
        gte_stclmv(&screenMatrix.m[0][1]);
        gte_ldclmv(&actorMatrix.m[0][2]);
        gte_mvmva(1, 0, 3, 3, 0);
        gte_stclmv(&screenMatrix.m[0][2]);
        gte_SetTransMatrix_gpr(&g_Scene.worldToScreenMatrix);
        gte_ldlv0(&actorMatrix.t[0]);
        gte_mvmva(1, 0, 0, 0, 0);
        gte_stlvnl(&screenMatrix.t[0]);

        scaleX = (s16)pActor->pActorData->scaleX * 0xC00;
        scale.vx = scaleX >> 12;
        scaleY = (s16)pActor->pActorData->scaleY * 0xC00;
        scale.vy = scaleY >> 12;
        scaleZ = (s16)pActor->pActorData->scaleZ * 0xC00;
        scale.vz = scaleZ >> 12;
        if (g_FieldSceneReloadFlags != 0) {
            if (pActor->pActorData->scriptFlags.flags & 0x400) {
                scale.vx = scaleX >> 14;
                scale.vy = scaleY >> 14;
                scale.vz = scaleZ >> 14;
            }
        }

        ScaleMatrix(&screenMatrix, &scale);
        gte_SetRotTransMatrix_gpr(&screenMatrix);

        otz = RotAverage4(&((Quad*)pActor->pShadow)->vertices[0],
            &((Quad*)pActor->pShadow)->vertices[1],
            &((Quad*)pActor->pShadow)->vertices[2],
            &((Quad*)pActor->pShadow)->vertices[3],
            (long*)&((Quad*)pActor->pShadow)->polys[renderContextIndex].x0,
            (long*)&((Quad*)pActor->pShadow)->polys[renderContextIndex].x1,
            (long*)&((Quad*)pActor->pShadow)->polys[renderContextIndex].x2,
            (long*)&((Quad*)pActor->pShadow)->polys[renderContextIndex].x3, &perspective,
            &flag);
        otz >>= g_GfxOTDepthShift;

        addPrim(&pOtBase[otz], &((Quad*)pActor->pShadow)->polys[renderContextIndex]);
    }
}
*/

u_int FieldSetActorFlag10000(SpriteData* pSpriteData) {
    ActorData* pActorData;

    pActorData = g_FieldActors[((SpriteDataI3*)pSpriteData->field_0x7C)->entityId].pActorData;
    pActorData->flags |= 0x10000;
    return pActorData->flags;
}

// Per-sprite-id placement table, stride 8; misc3's FieldLoad walks the same array
// to upload the clut of each entry (offset 0x4/0x6).
typedef struct {
    /* 0x0 */ u16 texX;
    /* 0x2 */ u16 texY;
    /* 0x4 */ u16 clut;
    /* 0x6 */ s16 clutLoaded;
} FieldSpritePlacement; // Size: 0x8

extern FieldSpritePlacement g_FieldSpritePlacements[];
extern s32 g_GamePartySkinsInitialized;
extern s32 g_FieldScriptSpritesBuilt;
extern SpriteData* SpriteAllocFromAnimFile(void* pAnimFile, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5);
extern SpriteData* SpriteAllocFromAnimFileWithClut(void* pAnimFile, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5,
                                                   s32 arg6);
extern void SpriteSetCoordShift(SpriteData* pSpriteData, s32 shift);
extern void SpriteSetCallback(SpriteData* pSpriteData, u_int (*pCallback)(SpriteData*));
extern void SpriteReallocFramesData(SpriteData* pSpriteData, s32 size);
extern void func_8001F5BC(SpriteData* pSpriteData, s32 unusedIndex, s32* pOutX, s32* pOutY, s32* pOutZ);

// 73.82% (1764 bytes against retail's 1776; 441 instructions against 444; frame 0x58
// both). The body below is realistic per-statement `g_FieldActors[actorIndex]`, which
// is what the reload pattern in the .s shows (retail reloads the volatile base and
// pActorData for every statement, because every `sb`/`sw` kills those loads).
// WHAT IS LEFT, and it is one register: retail keeps the `actorIndex * 0x5C` byte
// offset in a CALLEE-SAVED register ($s2) and re-adds it to the freshly loaded base
// per statement; gcc keeps ours in $a1 and re-forms the sll/addu/sll/subu/sll chain
// per block. That is the whole of the 423-line diff: retail saves s0..s7+fp (10 words),
// ours s0..s7 (9), so every value above $s0 is one register low. Screened dead:
// gcc-2.7.2-cdk 66.64, gcc-2.6.0 73.61, -O3/-G0 no change.
// MEASURED THIS ROUND (do not re-derive, all on this same body):
//  - a source-level `s32 actorOffset = actorIndex * sizeof(FieldActor)` used by the
//    whole body (raw `*(FieldActor*)((u8*)g_FieldActors + actorOffset)`): 75.06%,
//    1692 bytes / 423 insns, and gcc then DOES use the 9th callee-saved register
//    ($s1) plus $fp for the 7th parameter: the saved count and the frame become
//    retail's exactly. So the wall is breakable; the spelling is not the answer.
//  - the same offset local only from the top through SpriteReallocFramesData (the
//    tail left per-statement): 76.98%, 1736 bytes / 434 insns, saved set and frame
//    still retail's. What is left there is (a) a two-register swap - retail has
//    spriteId=$s1 / offset=$s2, we get the mirror (global.c's refs/live-length
//    quotient, see levers) - and (b) retail's reuse of the tested base in the two
//    `if (spriteFlags & 1)` then-blocks (`lw a0,4(v1)`) where a volatile read must
//    load again, worth 5 instructions per arm.
//  - `g_FieldActors` declared non-volatile (include/field/actor.h) on the plain body:
//    72.15%, 1468 bytes / 367 insns - gcc then CSEs the base across the statements and
//    the reload pattern retail has disappears. The header's `volatile` is load-bearing
//    (it IS slightly wrong: retail does reuse one load inside a block, `lw a0,4(v1)`
//    off the base the `spriteFlags & 1` test just used, which no volatile body can
//    emit - but the non-volatile body loses 77 instructions, so leave the header).
//  Remaining tail residual (3 instructions, same in both spellings): retail loads
//  pActorData and the s16 position half ONCE per component and stores that register
//  into both transformMatrix.t[i] (0x20/0x24/0x28) and childMatrix.t[i]
//  (0x40/0x44/0x48); ours re-loads the 0x4C pointer and redoes the `lh` for the second
//  store, because the 32-bit store through pActor in between is alias-ambiguous with
//  the pointer load.
// NOTES ON THE DATA, checked one by one against the assembly: FieldActor+0x20..0x28
// and +0x40..0x48 are transformMatrix.t[0..2] and childMatrix.t[0..2] (MATRIX::t is
// at 0x14 of a MATRIX at 0xC / 0x2C, so NOT a `move` vector); ActorData::position
// (0x20, VECTOR of longs) is read in 16.16 through `((s16*)&position)[1]/[3]/[5]`;
// g_FieldScriptSpritesBuilt is its own global; the double `step.y = 0` store at
// 0x10 IS in the original (0x10, 0xC, 0x10, 0x14), duplicated here on purpose.
// 1. The `pActorData->pSpriteData = pSpriteData` store in the modelKind != 0 path
//    comes BEFORE SpriteReallocFramesData: retail's `sw s0,4(v0)` sits in the jal
//    delay slot, so the store executes first. Writing it after the call costs the
//    whole 5-instruction index chain, re-formed after the call (441 -> 432 insns).
// 2. The tail re-assigns the actor pointer per matrix component (`pActor =
//    &g_FieldActors[actorIndex]`), which is why retail reloads the volatile there.
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80076AC0);
// 76.88% (1700 bytes against retail's 1776; 425 instructions against 444; frame 0x58 and the
// saved set s0..s7+fp+ra are retail's exactly). Retail holds actorIndex * sizeof(FieldActor) in
// a callee-saved register ($s2) for the whole first region and re-does only the one `addu` per
// statement; gcc keeps the same value in a caller-saved scratch (a3, copied to s6 before the
// calls) and re-forms the 5-instruction index chain after every call, and it fills retail's
// load-delay nops with the re-formed address - that is the whole 19-instruction gap. The
// source-level offset local is what makes gcc spend the 9th callee-saved register and $fp for
// the 7th parameter (see the round notes above); without it the saved set is 9 words and the
// score is 73.82%. Tried this round: per-statement `pActor = ...` locals (76.83), the inline
// cast spelling (76.88), gcc-2.6.0 (75.05), gcc-2.7.2-cdk (68.11). No twin exists.
/*
void func_80076AC0(s32 actorIndex, s32 spriteId, void* pAnimFile, s32 modelKind, s32 clutIndex, s32 spriteVariant,
                   s32 skipAnimTick) {
    FieldActor* pActor;
    ActorData* pActorData;
    SpriteData* pSpriteData;
    s32 actorOffset;
    u16 texX;
    u16 texY;
    s32 frameWidth;
    s32 frameHeight;
    s32 frameDepth;

    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);

    actorOffset = actorIndex * (s32)sizeof(FieldActor);

    ((FieldActor*)((u8*)g_FieldActors + actorOffset))->pActorData->spriteId = spriteId;

    ((FieldActor*)((u8*)g_FieldActors + actorOffset))->pActorData->spriteSource = spriteVariant;

    pActor = (FieldActor*)((u8*)g_FieldActors + actorOffset);
    pActorData = pActor->pActorData;
    pActorData->flags134 = (pActorData->flags134 & ~0xF) | (clutIndex & 0xF);

    pActor = (FieldActor*)((u8*)g_FieldActors + actorOffset);
    pActorData = pActor->pActorData;
    pActorData->flags130 = (pActorData->flags130 & 0xCFFFFFFF) | ((modelKind & 3) << 28);

    pActor = (FieldActor*)((u8*)g_FieldActors + actorOffset);
    pActorData = pActor->pActorData;
    pActorData->flags134 = (pActorData->flags134 & ~0x10) | ((skipAnimTick & 1) << 4);

    if (modelKind == 0) {
        texY = g_FieldSpritePlacements[spriteId].texY;
        texX = g_FieldSpritePlacements[spriteId].texX;
        if (clutIndex == 0) {
            pActor = (FieldActor*)((u8*)g_FieldActors + actorOffset);
            if (pActor->spriteFlags & 1) {
                SpriteDestroy(pActor->pSpriteData);
            }
            pSpriteData = SpriteAllocFromAnimFile(pAnimFile, 0x100, (s16)(spriteId + 0x1E0), texX, texY, 0x40);
        } else {
            pActor = (FieldActor*)((u8*)g_FieldActors + actorOffset);
            if (pActor->spriteFlags & 1) {
                SpriteDestroy(pActor->pSpriteData);
            }
            pSpriteData = SpriteAllocFromAnimFileWithClut(pAnimFile, clutIndex * 0x10 + 0x100, (s16)(spriteId + 0x1E0),
                                                          texX, texY, 0x40, clutIndex);
        }
        pActor = (FieldActor*)((u8*)g_FieldActors + actorOffset);
        pActor->pSpriteData = pSpriteData;
    } else {
        pActor = (FieldActor*)((u8*)g_FieldActors + actorOffset);
        if (pActor->spriteFlags & 1) {
            SpriteDestroy(pActor->pSpriteData);
        }
        if (modelKind == 1) {
            pSpriteData = SpriteAllocFromAnimFile(pAnimFile, 0x100, (s16)(spriteId + 0xE0), 0x280,
                                                  (s16)(spriteId * 0x40 + 0x100), 8);
        } else {
            pSpriteData = SpriteAllocFromAnimFile(pAnimFile, 0x100, (s16)(spriteId + 0xE3), 0x2A0,
                                                  (s16)(spriteId * 0x40 + 0x100), 8);
        }
        g_FieldActors[actorIndex].pSpriteData = pSpriteData;
        SpriteReallocFramesData(pSpriteData, 0x20);
    }

    g_FieldActors[actorIndex].spriteFlags |= 1;
    func_8001F5BC(pSpriteData, 0, &frameWidth, &frameHeight, &frameDepth);
    SpriteSetCoordShift(pSpriteData, 3);

    pSpriteData->scale = 0xC00;
    pSpriteData->field_0x82 = 0x2000;

    if (g_GamePartySkinsInitialized == 0) {
        pSpriteData->position.x = g_FieldActors[actorIndex].pActorData->position.vx;
        pSpriteData->position.y = g_FieldActors[actorIndex].pActorData->position.vy;
        pSpriteData->position.z = g_FieldActors[actorIndex].pActorData->position.vz;
        pSpriteData->step.y = 0;
        pSpriteData->step.x = 0;
        pSpriteData->step.y = 0;
        pSpriteData->step.z = 0;
        pSpriteData->gravity = 0x10000;
        pSpriteData->field_0x84 = g_FieldActors[actorIndex].transformMatrix.t[1];
        if (modelKind == 0) {
            pActorData = g_FieldActors[actorIndex].pActorData;
            pActorData->height = frameHeight * 2;
        } else {
            pActorData = g_FieldActors[actorIndex].pActorData;
            pActorData->height = 0x40;
        }
    }

    if (g_FieldFogEnabled != 0) {
        pSpriteData->field_0x40 |= 0x40000;
    }

    SpritePlayAnimation(pSpriteData, 0);
    SpriteSetDirection(pSpriteData, 0);
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);

    ((SpriteDataI3*)pSpriteData->field_0x7C)->entityId = actorIndex;
    SpriteSetCallback(pSpriteData, FieldSetActorFlag10000);

    if (skipAnimTick == 0) {
        AnimScriptTick(pSpriteData);
        TimerWorkListUpdate();
        if (((SpriteDataI3*)pSpriteData->field_0x7C)->field_0xC == 0xFF) {
            pActorData = g_FieldActors[actorIndex].pActorData;
            pActorData->animationOverride = 0xFF;
            pActorData->flags |= 0x1000000;
            pSpriteData->position.x = g_FieldActors[actorIndex].pActorData->position.vx;
            pSpriteData->position.y = g_FieldActors[actorIndex].pActorData->position.vy;
            pSpriteData->position.z = g_FieldActors[actorIndex].pActorData->position.vz;
        }
    }

    pActor = &g_FieldActors[actorIndex];
    pActor->transformMatrix.t[0] = ((s16*)&pActor->pActorData->position)[1];
    pActor->childMatrix.t[0] = ((s16*)&pActor->pActorData->position)[1];
    pActor = &g_FieldActors[actorIndex];
    pActor->transformMatrix.t[1] = ((s16*)&pActor->pActorData->position)[3];
    pActor->childMatrix.t[1] = ((s16*)&pActor->pActorData->position)[3];
    pActor = &g_FieldActors[actorIndex];
    pActor->transformMatrix.t[2] = ((s16*)&pActor->pActorData->position)[5];
    pActor->childMatrix.t[2] = ((s16*)&pActor->pActorData->position)[5];
    pSpriteData->field_0x84 = g_FieldActors[actorIndex].transformMatrix.t[1];
    pSpriteData->position.x = g_FieldActors[actorIndex].pActorData->position.vx;
    pSpriteData->position.y = g_FieldActors[actorIndex].pActorData->position.vy;
    pSpriteData->position.z = g_FieldActors[actorIndex].pActorData->position.vz;

    g_FieldScriptSpritesBuilt++;
}
*/




void FieldSetHalfwordFlag8000(u_int* pPrim, int size) {
    int i = size / 2;

    while (i--) {
        *pPrim++ |= 0x80008000;
    }
}

void FieldLoadTIM(u_long* pTimData) {
    TIM_IMAGE timImage;

    OpenTIM(pTimData);
    while (ReadTIM(&timImage) ) {
        if (timImage.caddr != NULL) {
            LoadImage(timImage.crect, timImage.caddr);
        }
        if (timImage.paddr != NULL) {
            LoadImage(timImage.prect, timImage.paddr);
        }
    }
}


extern volatile s32 g_PlayerActorIndex;
extern s32 g_FieldSpriteUpdateIndex;
extern s32 D_800B2364;
extern s32 D_800B2368;
extern void func_80084A40(int actorIndex, int y, FieldActor* pActor, ActorData* pData);
extern int FieldCharacterIdToPartyId(int characterId);
extern void func_80081C54(int actorIndex);

void FieldSnapPartyToPlayer(void) {
    ActorData* pData;
    int i;
    SpriteData* pSpriteData;
    FieldActor* pActor;
    FieldActor* pCurActor;
    int partyId;
    ActorData* pPlayerData;
    FieldActor* pFieldActors;
    int actorIndex;
    int playerIndex;

    actorIndex = g_PlayerActorIndex;
    playerIndex = g_PlayerActorIndex;
    pFieldActors = g_FieldActors;
    pPlayerData = g_FieldActors[playerIndex].pActorData;
    func_80084A40(actorIndex, ((s16*)&pPlayerData->position.vy)[1], &pFieldActors[actorIndex],
        pPlayerData);

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pActor = &g_FieldActors[i];
        pData = pActor->pActorData;
        if ((pActor->status & 0xF80) != 0x200) {
            continue;
        }
        partyId = FieldCharacterIdToPartyId(pData->characterId);
        if (partyId == -1) {
            continue;
        }
        pCurActor = (FieldActor*)(i * sizeof(FieldActor) + (int)g_FieldActors);
        pSpriteData = pCurActor->pSpriteData;
        if (partyId == 0) {
            continue;
        }
        func_80084A40(i, ((s16*)&pCurActor->pActorData->position.vy)[1], pActor, pData);
        *(volatile int*)&pSpriteData->position.x = ((SpriteData*)g_FieldActors[g_PlayerActorIndex].pSpriteData)->position.x;
        *(volatile int*)&pSpriteData->position.y = ((SpriteData*)g_FieldActors[g_PlayerActorIndex].pSpriteData)->position.y;
        *(volatile int*)&pSpriteData->position.z = ((SpriteData*)g_FieldActors[g_PlayerActorIndex].pSpriteData)->position.z;
        *(volatile int*)&pActor->transformMatrix.t[0] = g_FieldActors[g_PlayerActorIndex].transformMatrix.t[0];
        *(volatile int*)&pActor->transformMatrix.t[1] = g_FieldActors[g_PlayerActorIndex].transformMatrix.t[1];
        *(volatile int*)&pActor->transformMatrix.t[2] = g_FieldActors[g_PlayerActorIndex].transformMatrix.t[2];
    }

    D_800B2368 = 0;
    D_800B2364 = 0;
    g_FieldSpriteUpdateIndex = 0;
    for (i = 0; i < 0x20; i++) {
        func_80081C54(g_PlayerActorIndex);
    }
}

void FieldLoadFont(void) {
    if (g_FieldSystemMode == 0) {
        FontSetExternalBuffer((void*)0x80270000);
        FontLoadFont(0x10, 0x10, 0x130, 0xE0, 0x400, 4, 0x3C0, 0x100, 0x100, 0x1FF, 0);
    }
    SystemTransferPaletteToVRAM(0x100, 0xF0);
}
