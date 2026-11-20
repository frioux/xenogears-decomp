#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "system/math.h"
#include "system/controller.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

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
/*
extern void FieldMatrixSetIdentity(MATRIX* pMatrix);
extern VECTOR g_CameraEye;
extern VECTOR g_CameraUp;

void func_8007254C(void) {
    u_char* pCam = (u_char*)&g_CamInterpolation;
    u_char** pCamRef = &pCam;

    (void)pCamRef;

    *(s32*)(pCam + 0x0) = 8;
    g_CamInterpolation.eyeStepDistance = 8;
    g_Scene.unk5C = 0x400000;
    g_Scene.unk60 = 0x8000000;
    g_Scene.sceneAngle.vy = 0x800;
    ((s32*)g_Scene.unk98)[4] = 0;
    ((s32*)g_Scene.unk98)[3] = 0;
    ((s32*)g_Scene.unk98)[2] = 0;
    ((s32*)g_Scene.unk98)[7] = 0;
    ((s32*)g_Scene.unk98)[6] = 0;
    ((s32*)g_Scene.unk98)[5] = 0;
    ((s16*)g_Scene.unk98)[0] = 0;
    ((s16*)g_Scene.unk98)[2] = 0;
    g_Scene.unk48 = 0;
    g_CamInterpolation.curAngleY = 0;
    g_CamInterpolation.targetAngleY = 0;
    g_Scene.dollySet = 0;
    g_Scene.dollyStop = 0;
    g_Scene.unk66 = 0;
    g_Scene.sceneAngle.vx = 0;
    g_Scene.sceneAngle.vz = 0;
    g_Scene.unk7C = 0x800;
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
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_8007254C);



INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800726E8);
// 93.36% (856 bytes vs the original's 844). Re-checked under the unit's
// XenoFieldNoGp (-G0) preset: still 93.36. Retail's frame is 0x18 with ONE saved
// register ($ra at 0x14, $s0 at 0x10), so exactly one of the two addresses the
// block touches twice can live in a register across the FieldFindClearDirectionForward call;
// retail keeps &g_Scene.dollyStop (+0x65) and re-forms %hi/%lo for
// g_Scene.sceneAngle.vy on each use, gcc keeps the vy address (materialised
// first, for the table index) and re-forms dollyStop. That is a CSE winner
// choice, not a source shape: the vy address is created first in every spelling
// tried, and forcing dollyStop (a u_char* local, 89.89%) loses the frame.
// field camera dolly. The structure,
// the calls and every store match; what is left is (1) block 2 (the dollyStop
// mask + the two func_800723* calls): the original holds &g_Scene.dollyStop in
// $s0 and re-forms %hi/%lo for sceneAngle.vy on each use, gcc here holds
// &g_Scene.sceneAngle.vy in $s0 instead (a `u_char* pDollyStop` local in the
// block is worse, 89.89%); (2) the shared `unk66 == 0` tail: the original lays
// it out at the end of the function (+0x2B0) where gcc puts it at +0x168, so the
// early `dollySet/dollyStop == 0xFF` exits branch forward 0x148 instead of 0x2B0.
// Note the two expression spellings are required: `((u_int)vy & 0xFFF) >> 9`
// gives the original's lhu/srl in the first two blocks, while the plain signed
// `(vy +/- 0x200) & 0xFFF) >> 9` gives lh/sra in the button blocks.
// screened: Default 93.36 (best), cdk 65.00, 2.6.0 92.51 - not the preset; the dollyStop base register and the branch layout.
/*
extern void func_80284EA4(void);

void func_800726E8(void) {
    int offset;

    if (g_Scene.dollySet != 0xFF && g_Scene.dollyStop != 0xFF) {
        if (g_Scene.unk66 == 0) {
            if (g_FieldDirectionMasks[((u_int)g_Scene.sceneAngle.vy & 0xFFF) >> 9] & g_Scene.dollySet) {
                if (g_Scene.unk5C != -0x400000 && g_Scene.unk5C != 0x400000) {
                    g_Scene.unk5C = 0x400000;
                    g_Scene.unk7C += 0x200;
                }
                g_Scene.unk66 = 8;
            }
        }

        if (g_FieldDirectionMasks[((u_int)g_Scene.sceneAngle.vy & 0xFFF) >> 9] & g_Scene.dollyStop) {
            offset = FieldFindClearDirectionForward(g_Scene.dollyStop,
                ((u_int)g_Scene.sceneAngle.vy & 0xFFF) >> 9);
            if (FieldFindClearDirectionBackward(g_Scene.dollyStop,
                    ((u_int)g_Scene.sceneAngle.vy & 0xFFF) >> 9) < offset) {
                g_Scene.unk5C = -0x400000;
                g_Scene.unk7C -= 0x200;
            } else {
                g_Scene.unk5C = 0x400000;
                g_Scene.unk7C += 0x200;
            }
            g_Scene.unk66 = 8;
        }
    }

    if (g_FieldP1ButtonsPressed & 0x4) {
        if (!(g_Scene.unk48 & 0x8000) && g_Scene.unk66 == 0) {
            if (!(g_FieldDirectionMasks[((g_Scene.sceneAngle.vy - 0x200) & 0xFFF) >> 9] & g_Scene.dollyStop)) {
                g_Scene.unk5C = -0x400000;
                g_Scene.unk66 = 8;
                g_Scene.unk7C -= 0x200;
            }
        }
    }

    if (g_FieldP1ButtonsPressed & 0x8) {
        if (!(g_Scene.unk48 & 0x8000) && g_Scene.unk66 == 0) {
            if (!(g_FieldDirectionMasks[((g_Scene.sceneAngle.vy + 0x200) & 0xFFF) >> 9] & g_Scene.dollyStop)) {
                g_Scene.unk5C = 0x400000;
                g_Scene.unk66 = 8;
                g_Scene.unk7C += 0x200;
            }
        }
    }

    if (g_Scene.unk66 != 0) {
        g_Scene.unk60 += g_Scene.unk5C;
        g_Scene.sceneAngle.vy = g_Scene.unk60 >> 16;
        g_Scene.unk66--;
    }
    if (g_Scene.unk66 == 0) {
        g_Scene.sceneAngle.vy = g_Scene.unk7C;
    }

    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_80284EA4();
    }
}
*/

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

    if (g_Scene.unk48 & 0x1) {
        if (g_Scene.unk8C != 0) {
            g_Scene.unk90 += g_Scene.unk94;
            g_Scene.sceneScale = g_Scene.unk90 >> 16;
        }
        if (--g_Scene.unk8C == 0) {
            g_Scene.unk48 &= 0xFFFE;
        }
    }

    if (g_Scene.unk48 & 0x8) {
        s32 flags = g_Scene.unk48;
        g_Scene.unk74 += g_Scene.unk78;
        g_Scene.sceneDIP = g_Scene.unk74 >> 16;
        if (--g_Scene.unk70 == 0) {
            g_Scene.unk48 = flags & 0xFFF7;
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
// same for &g_Scene.unk98, which it then keeps live into the else-if branch) where
// retail folds the displacement into the load/store (`lh $v0,152($v0)`, `lui $at / sw
// $v0,%lo(...)($at)`). It also shifts the mult temporaries one register ($t1/$t2 here
// against retail's $t0/$t1). Naming the g_CameraEye word and the g_Scene +0x98 block as
// fields is a struct/field job in the shared header, not a C shape.
// permuter: base=531 best=426 (PERMUTER_BETTER, chained-assignment debris only).
// screened: Default 98.25 (best) - not a preset question.
// round 170 re-measured the park at 98.25% (1212 bytes, retail's size) and tried two
// more shapes, both of which keep the 98.25% or lose the tail:
//  - a `s32 result` local for the store (`(cur) = result;`) - byte-identical output;
//  - the whole +0x98 block spelled with byte indices (`*(s16*)&g_Scene.unk98[0x0]`)
//    as the neighbouring misc.c does - that makes gcc materialise &g_Scene.unk98 in a
//    register for the guard too (`lh v0,0(v1)`), so it is worse; the array-cast form
//    above is the one to keep.
// round 185: 98.99 (1212 bytes, retail's size). The permuter closed the +0x98 half of
// the hoist: ONE `s16* pShake = (s16*)g_Scene.unk98;` local, assigned between the eye-vx
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
/*
void func_80072D74(void) {
    s32 atStepSq;
    s32 eyeStepSq;
    s32* pSnapFrames;
    s32* pShakeOffset;
    s16* pShake;

    if (g_Scene.unk48 & FIELD_SCENE_INTERPOLATING_SCREEN_Z) {
        if (g_Scene.unk80 != 0) {
            g_Scene.unk84 += g_Scene.unk88;
            g_Scene.sceneScrZ = g_Scene.unk84 >> 16;
        }
        if (--g_Scene.unk80 < 0) {
            g_Scene.unk48 &= 0xFFEF;
            g_Scene.unk80 = 0;
        }
    }

    pSnapFrames = &g_FieldCameraSnapFrames;
    if (*pSnapFrames != 0) {
        g_CamInterpolation.atStepDistance = 1;
        g_CamInterpolation.eyeStepDistance = 1;
        (*pSnapFrames)--;
    }

    atStepSq = g_CamInterpolation.atStepDistance * g_CamInterpolation.atStepDistance;
    eyeStepSq = g_CamInterpolation.eyeStepDistance * g_CamInterpolation.eyeStepDistance;

    FIELD_CAMERA_STEP(g_CameraEye.vx, g_CameraEye2.vx, g_CamInterpolation.eyeStepDistance, eyeStepSq);
    pShake = (s16*)g_Scene.unk98;
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

        if (((s16*)g_Scene.unk98)[1] != 0) {
            ((s32*)g_Scene.unk98)[2] += ((s32*)g_Scene.unk98)[5];
            ((s32*)g_Scene.unk98)[3] += ((s32*)g_Scene.unk98)[6];
            ((s32*)g_Scene.unk98)[4] += ((s32*)g_Scene.unk98)[7];
        } else if (((s16*)g_Scene.unk98)[2] != 0) {
            ((s32*)g_Scene.unk98)[4] = 0;
            ((s32*)g_Scene.unk98)[3] = 0;
            ((s32*)g_Scene.unk98)[2] = 0;
            ((s16*)g_Scene.unk98)[0] = 0;
            ((s16*)g_Scene.unk98)[2] = 0;
        }
        pShakeOffset = &g_FieldCameraShakeOffset;
        *pShakeOffset = rand() * ((s16*)g_Scene.unk98)[5];
        g_FieldCameraShakeOffsetY = rand() * ((s16*)g_Scene.unk98)[7];
        shakeZ = rand() * ((s16*)g_Scene.unk98)[9];
        g_FieldCameraShakeOffsetZ = shakeZ;
        if (*pShakeOffset < 0) {
            *pShakeOffset = 0;
            ((s32*)g_Scene.unk98)[2] = 0;
        }
        if (g_FieldCameraShakeOffsetY < 0) {
            g_FieldCameraShakeOffsetY = 0;
            ((s32*)g_Scene.unk98)[3] = 0;
        }
        if (shakeZ < 0) {
            g_FieldCameraShakeOffsetZ = 0;
            ((s32*)g_Scene.unk98)[4] = 0;
        }
        if (((s16*)g_Scene.unk98)[1] > 0) {
            ((s16*)g_Scene.unk98)[1]--;
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
extern u16 g_FieldPlayerActorIndex16;
extern s16 g_FieldWalkmeshTriCount;
extern s32 D_800ADBAC;
extern s32 D_800ADBB0;
extern int func_8007B1C4(int x, int z, int walkmeshId, SVECTOR* pOut, VECTOR* pNormal);
extern long FieldGetVec2Magnitude(long x, long z);

// 89.20% (1108 bytes = the original's size). Structure is settled and the switch is a
// THREE-case switch whose mode-0 and mode-2 arms both carry the follow-camera block: gcc
// cross-jumps the two identical tails into one, which is why retail has the middle block
// once (between the case-2/case-0 arms and the case-1 arm) and why case 1 alone reaches
// the tail by fallthrough. What is left is gcc's address-CSE choice: retail uses bare
// %hi/%lo symbol forms for a load+store pair at offset 0 (g_CamInterpolation.atStepDistance,
// g_CamAtMovementCurrent.vx, g_CamMovementFlags) and only materialises a base where the
// pair sits in one basic block (eyeStepDistance), while gcc here materialises the base for
// every pair; that also re-orders the delta loads in the at-movement block. Also tried:
// plain symbols everywhere (86.18, 1112 bytes: the top loses retail's a0 base), one
// pointer local for all three g_FieldCameraMode uses (88.22, s0 where retail has a0+s1),
// delta/step locals, nested ifs instead of &&, and a pointer for eyeStepDistance alone
// (87.91, frame +8). Screened: 2.6.0 89.20, cdk 62.35 - not a preset question. The
// permuter cannot run on this unit body ("conversion to non-scalar type requested" on the
// first base compile).
/*
void func_80073230(void) {
    VECTOR pos;
    VECTOR normal;
    SVECTOR point;
    s16 duration;
    u16 flags;
    FieldActor* pActor;
    s32 atDistance;
    s32 eyeDistance;
    s16* pMode;
    s16* pCameraMode;

    pMode = &g_FieldCameraMode;
    switch (*pMode) {
    case 2:
        D_800ADBAC = 0;
        if (++D_800ADBB0 >= 65) {
            *pMode = 0;
        }
        func_800726E8();
        pActor = (FieldActor*)(g_FieldPlayerActorIndex16 * sizeof(FieldActor) + (mem_addr)g_FieldActors);
        pos.vx = pActor->pActorData->position.vx;
        pos.vy = pActor->pActorData->position.vy;
        pos.vz = pActor->pActorData->position.vz;
        FieldCameraUpdateFromTarget(&pos, (s16)pActor->pActorData->curYPos);
        if (!(g_Scene.unk48 & 0x4000)) {
            func_8007B1C4((s16)(g_CameraEye2.vx >> 16), (s16)(g_CameraEye2.vz >> 16),
                g_FieldWalkmeshTriCount - 1, &point, &normal);
            if (point.vy < (s16)(g_CameraEye2.vy >> 16)) {
                g_CameraEye2.vy = point.vy << 16;
            }
        }
        pCameraMode = &g_FieldCameraMode;
        if (*pCameraMode == 2) {
            atDistance = FieldGetVec2Magnitude((s16)(g_CameraAt2.vx >> 16) - (s16)(g_CameraAt.vx >> 16),
                (s16)(g_CameraAt2.vz >> 16) - (s16)(g_CameraAt.vz >> 16));
            eyeDistance = FieldGetVec2Magnitude((s16)(g_CameraEye2.vx >> 16) - (s16)(g_CameraEye.vx >> 16),
                (s16)(g_CameraEye2.vz >> 16) - (s16)(g_CameraEye.vz >> 16));
            if (atDistance < 0x80) {
                if (eyeDistance < 0x80) {
                    *pCameraMode = 0;
                }
            }
        }
        break;
    case 0:
        D_800ADBB0 = 0;
        if ((D_800ADBAC & 3) == 0) {
            g_CamInterpolation.atStepDistance = g_CamInterpolation.atStepDistance < 9
                ? 8 : g_CamInterpolation.atStepDistance - 2;
            g_CamInterpolation.eyeStepDistance = g_CamInterpolation.eyeStepDistance < 9
                ? 8 : g_CamInterpolation.eyeStepDistance - 2;
        }
        D_800ADBAC++;
        func_800726E8();
        pActor = (FieldActor*)(g_FieldPlayerActorIndex16 * sizeof(FieldActor) + (mem_addr)g_FieldActors);
        pos.vx = pActor->pActorData->position.vx;
        pos.vy = pActor->pActorData->position.vy;
        pos.vz = pActor->pActorData->position.vz;
        FieldCameraUpdateFromTarget(&pos, (s16)pActor->pActorData->curYPos);
        if (!(g_Scene.unk48 & 0x4000)) {
            func_8007B1C4((s16)(g_CameraEye2.vx >> 16), (s16)(g_CameraEye2.vz >> 16),
                g_FieldWalkmeshTriCount - 1, &point, &normal);
            if (point.vy < (s16)(g_CameraEye2.vy >> 16)) {
                g_CameraEye2.vy = point.vy << 16;
            }
        }
        pCameraMode = &g_FieldCameraMode;
        if (*pCameraMode == 2) {
            atDistance = FieldGetVec2Magnitude((s16)(g_CameraAt2.vx >> 16) - (s16)(g_CameraAt.vx >> 16),
                (s16)(g_CameraAt2.vz >> 16) - (s16)(g_CameraAt.vz >> 16));
            eyeDistance = FieldGetVec2Magnitude((s16)(g_CameraEye2.vx >> 16) - (s16)(g_CameraEye.vx >> 16),
                (s16)(g_CameraEye2.vz >> 16) - (s16)(g_CameraEye.vz >> 16));
            if (atDistance < 0x80) {
                if (eyeDistance < 0x80) {
                    *pCameraMode = 0;
                }
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
                g_CamAtMovementCurrent.vx += g_CamAtMovementDelta.vx;
                g_CamAtMovementCurrent.vy += g_CamAtMovementDelta.vy;
                g_CamAtMovementCurrent.vz += g_CamAtMovementDelta.vz;
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
        flags = g_CamMovementFlags;
        if (flags & FIELD_CAMERA_EYE_MOVEMENT_ACTIVE) {
            duration = g_CamEyeMovementDuration;
            if (duration != 0) {
                g_CamEyeMovementCurrent.vx += g_CamEyeMovementDelta.vx;
                g_CamEyeMovementCurrent.vy += g_CamEyeMovementDelta.vy;
                g_CamEyeMovementCurrent.vz += g_CamEyeMovementDelta.vz;
            }
            duration--;
            g_CamEyeMovementDuration = duration;
            if (duration == 0) {
                g_CamMovementFlags = flags & ~FIELD_CAMERA_EYE_MOVEMENT_ACTIVE;
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
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80073230);

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
extern void func_80073230(void);
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
    func_80073230();
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

// 95.62% / 1432 bytes (retail is 1428: 0x80074108..0x8007469C, 357 insns). What it is: an
// 8-row sprite sheet fill from D_800AFC08 into D_800AFD24 (a row zeroed when
// g_Scene.dollyStop & D_800ADC24[i]), a RECT at 0x800B004C with w = 0x80 and
// LoadImage(&rect, D_800AFD24), SetGeomScreen(0x80), SetGeomOffset(0x10A, 0xA6), a
// FieldMatrixLookAt whose pEye is (0, g_CameraEye.vy - g_CameraAt.vy,
// -FieldGetVec2Magnitude(...) << 16) with a zero pAt, the identity/CompMatrix/MulMatrix*
// chain over the 0x10/0x30/0x50/0x70/0x90 stack matrices, and two
// g_FieldCompassDisabled / g_FieldSkipCounter / D_8004F378 guarded passes:
// FieldRenderQuad over the compass quad (slot 0x14) with matCoord, then
// FieldRenderQuadScreenRect plus 20 FieldRenderQuad over the D_800B06BC quads and 4 more
// from slot 0x15 with matProj. Tail patches a prim's top byte between
// D_800B1E00[g_FieldCurRenderContextIndex] and the context's ot3[0], then
// SetGeomOffset(0xA0, 0x70) and SetGeomScreen(g_Scene.sceneScrZ). Frame 0xF8.
//
// The structure is right - every call, branch, loop, stack slot and stack-argument store
// is in retail's place, and the whole first half (sheet fill, LoadImage, LookAt, the
// CompMatrix chain) is instruction-identical. Residuals, all measured:
// 1. ONE INSTRUCTION OVER, so this body can never link as it stands. The last
//    FieldRenderQuad loop (D_800B0FEC, i = 0x15..0x18) puts the loop-carried
//    `addu a1,s1,zero` at the top of the body only and gives the back-edge delay slot to
//    the 0xFFFFFF constant's `lui`; this build emits the body copy AND a second
//    `move a1,s1` in the back-edge slot. Tried: `pQuad` + a separate `pQuad++` statement
//    (95.13), `pQuad++` in the argument (95.10 with a non-const tail), and the best,
//    `for (i = 0x15, pQuad = D_800B0FEC; ...)` (95.62, which also fixes the preheader
//    order of `li s2,0x80D4`).
// 2. The tail's three ot3[0] accesses NEED a context pointer local, not
//    g_FieldCurRenderContext inline: the store through pOverlayTag otherwise makes gcc
//    reload the global (3 extra insns). With the local, retail's three
//    `lui at,0x1 / addu at,$ctx,at` address forms (0x80D4 does not fit a 16-bit
//    displacement) come out - but this build then orders the D_800B1E00 symbol load after
//    the index shifts where retail has it before, and names the 0xFFFFFF mask $t1 where
//    retail has $a3. Same instruction count.
// 3. The camera base: retail keeps ONE register (`lui/addiu` of 0x800AF884, i.e.
//    &g_CameraEye.vy) and reaches the up vector as `addiu a3,s0,0x1C` with NO relocation,
//    so the original source took both from one symbol. Written with the symbols the rest
//    of the unit uses (g_CameraEye / g_CameraUp) this build materialises each separately
//    (`lui %hi(g_CameraEye); lw 4(...)` and `lui %hi(g_CameraUp); addiu`) - equal
//    instruction count, but it is the same subobject-base wall the unit's other parks are
//    stuck on (see FieldActorApplyRotationAndScale). Not tried: a single-symbol view such
//    as `((VECTOR*)&g_CameraEye)[2]` for the up vector.
// 4. The trigger-zone screen offsets are D_800ADC24[8 + k*2] / [9 + k*2] (four x/y pairs
//    stored after the eight direction masks, at 0x800ADC34), but retail's relocation names
//    g_pFieldTriggerZones + 0x40, i.e. the source reached 0x800ADC34 from a symbol at
//    0x800ADBF4. Same linked address either way; objdiff cannot reach 100% on the address
//    form. The counter starts at 0x10 because 0x700 / 0x70 = 16, so the trigger quads are
//    D_800B06BC[0x10..0x13], and D_800B0F7C / D_800B0FEC are the quads at D_800B06BC + 20
//    and + 21 (Quad is 0x70 bytes), which is why their references carry no offset.
// 5. `li s2,0x80D4` and `addiu a2,sp,0x70` land one slot apart from retail inside both quad
//    loop preheaders (harmless reorder).
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80074108);

/*
extern long FieldGetVec2Magnitude(long x, long z);
extern void FieldRenderQuad(u_long* ot, Quad* pQuad, MATRIX* pMatTransform, int renderContext);
extern void FieldRenderQuadScreenRect(u_long* ot, Quad* pQuad, MATRIX* pMatTransform, int renderContext);
extern FieldActor* g_FieldActors;
extern int g_FieldCurRenderContextIndex;
extern RenderContext* g_FieldCurRenderContext;
extern u16 g_FieldPlayerActorIndex16; // Slot the player actor occupies in g_FieldActors
extern u_char g_FieldCompassDisabled;
extern s32 D_8004F378;
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

#define FIELD_SPRITE_SHEET_ROW_COUNT 8
#define FIELD_SPRITE_SHEET_ROW_WIDTH 16
#define FIELD_SPRITE_SHEET_WIDTH 0x80
#define FIELD_OVERLAY_PRIM_SIZE 0xC0
#define FIELD_PRIM_WORD_MASK 0xFFFFFF
#define FIELD_PRIM_TOP_BYTE 0xFF000000

void func_80074108(void) {
    MATRIX matProj; // 0x10: projection matrix, t[2] holds the screen distance
    MATRIX matView; // 0x30
    MATRIX matWork; // 0x50
    MATRIX matCoord; // 0x70
    MATRIX matRot; // 0x90
    SVECTOR vecRot; // 0xB0
    VECTOR vecEye; // 0xB8
    VECTOR vecAt; // 0xC8
    FieldActor* pActor;
    RenderContext* pRenderContext;
    Quad* pQuad;
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

    vecAt.vx = 0;
    vecAt.vy = 0;
    vecAt.vz = 0;
    vecEye.vx = 0;
    vecEye.vy = g_CameraEye.vy - g_CameraAt.vy;
    nMagnitude = FieldGetVec2Magnitude((g_CameraEye.vx - g_CameraAt.vx) >> 16,
        (g_CameraEye.vz - g_CameraAt.vz) >> 16);
    vecEye.vz = -nMagnitude << 16;
    FieldMatrixLookAt(&matView, &vecEye, &vecAt, &g_CameraUp);

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

    if (g_FieldCompassDisabled == 0 && g_FieldSkipCounter == 0 && D_8004F378 == 0) {
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
    MulMatrix0(&matProj, &matWork, &g_Scene.unkF4);
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

    if (g_FieldCompassDisabled == 0 && g_FieldSkipCounter == 0 && D_8004F378 == 0) {
        for (i = 0x10; i < 0x14; i++) {
            FieldMatrixSetIdentity(&matWork);
            matWork.t[0] = (s16)D_800ADC24[(i - 0x10) * 2 + 8];
            matWork.t[2] = (s16)D_800ADC24[(i - 0x10) * 2 + 9];
            CompMatrix(&matProj, &matWork, &matCoord);
            FieldMatrixCopyTransform(&matCoord, &matRot);
            FieldRenderQuadScreenRect(g_FieldCurRenderContext->ot3, &D_800B06BC[i], &matCoord,
                g_FieldCurRenderContextIndex);
        }

        for (i = 0; i < 0x10; i++) {
            FieldRenderQuad(g_FieldCurRenderContext->ot3, &D_800B06BC[i], &matProj,
                g_FieldCurRenderContextIndex);
        }

        for (i = 0x15, pQuad = D_800B0FEC; i < 0x19; i++) {
            FieldRenderQuad(g_FieldCurRenderContext->ot3, pQuad++, &matProj,
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
}
*/

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

    func_800273C4(g_FieldScreenEffectData, &eye, &at, (MATRIX*)((u_char*)&g_CameraEye + 0x1E4),
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

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800764B4);

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

extern FieldSpritePlacement D_800B1F78[];
extern s32 g_GamePartySkinsInitialized;
extern s32 g_FieldScriptSpritesBuilt;
extern SpriteData* SpriteAllocFromAnimFile(void* pAnimFile, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5);
extern SpriteData* SpriteAllocFromAnimFileWithClut(void* pAnimFile, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5,
                                                   s32 arg6);
extern void SpriteSetCoordShift(SpriteData* pSpriteData, s32 shift);
extern void SpriteSetCallback(SpriteData* pSpriteData, u_int (*pCallback)(SpriteData*));
extern void SpriteReallocFramesData(SpriteData* pSpriteData, s32 size);
extern void func_8001F5BC(SpriteData* pSpriteData, s32 unusedIndex, s32* pOutX, s32* pOutY, s32* pOutZ);

// 73.94% (1728 bytes against the original's 1776; frame 0x58 both). The whole shape is
// transcribed and every store, call and branch is in the original's order; what is left:
//  - 12 instructions short, and the original has 63 `nop`s against our 41: several load
//    delays are filled differently, which is the allocator scheduling, not a missing
//    statement (offsets, widths and call arguments were checked one by one).
//  - one callee-saved register: the original keeps s0..s8+fp live (s3=actorIndex,
//    s0=param6, s8=param7), ours stops at s7, so every register from the second
//    parameter on is one lower.
//  - the D_800B1F78 table loads come out swapped: the original reads `texY` (0x7A)
//    first through its own %hi(D_800B1F7A) macro, gcc CSEs the base and loads offset 0
//    first however the two statements are ordered (tried both).
//  Findings worth keeping: FieldActor+0x40..0x48 is childMatrix.t[0..2] (NOT a `move`
//  vector - ActorData has one at the same offset); ActorData::position is a VECTOR of
//  longs and the code reads its 16.16 integer part through `((s16*)&position)[1]` /
//  [3] / [5]; g_FieldScriptSpritesBuilt is its own global (zeroed by func_800A28D4), not part of
//  g_FieldStoredImageBuffer; the double `step.y = 0` store at 0x10 IS in the original
//  (0x10, 0xC, 0x10, 0x14), duplicated here on purpose.
//  The sprite model path itself: modelKind 0 builds from the D_800B1F78 placement table,
//  1/2 from the fixed 0x280/0x2A0 layouts, and the tail copies the actor's transform
//  into both matrices and back into the sprite.
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80076AC0);
/*
// Loads a sprite model for an actor: replaces the actor's SpriteData with one built
// from pAnimFile, then points the actor's transform and position at the new sprite.
// modelKind 0 keeps the actor's own frame size, 1 and 2 are the two model layouts.
void func_80076AC0(s32 actorIndex, s32 spriteId, void* pAnimFile, s32 modelKind, s32 clutIndex, s32 spriteVariant,
                   s32 skipAnimTick) {
    FieldActor* pActor;
    ActorData* pActorData;
    SpriteData* pSpriteData;
    u16 texX;
    u16 texY;
    s32 frameWidth;
    s32 frameHeight;
    s32 frameDepth;

    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);

    g_FieldActors[actorIndex].pActorData->spriteId = spriteId;
    g_FieldActors[actorIndex].pActorData->spriteSource = spriteVariant;

    pActorData = g_FieldActors[actorIndex].pActorData;
    pActorData->flags134 = (pActorData->flags134 & ~0xF) | (clutIndex & 0xF);

    pActorData = g_FieldActors[actorIndex].pActorData;
    pActorData->flags130 = (pActorData->flags130 & 0xCFFFFFFF) | ((modelKind & 3) << 28);

    pActorData = g_FieldActors[actorIndex].pActorData;
    pActorData->flags134 = (pActorData->flags134 & ~0x10) | ((skipAnimTick & 1) << 4);

    if (modelKind == 0) {
        texY = D_800B1F78[spriteId].texY;
        texX = D_800B1F78[spriteId].texX;
        if (clutIndex == 0) {
            if (g_FieldActors[actorIndex].spriteFlags & 1) {
                SpriteDestroy(g_FieldActors[actorIndex].pSpriteData);
            }
            pSpriteData = SpriteAllocFromAnimFile(pAnimFile, 0x100, (s16)(spriteId + 0x1E0), texX, texY, 0x40);
        } else {
            if (g_FieldActors[actorIndex].spriteFlags & 1) {
                SpriteDestroy(g_FieldActors[actorIndex].pSpriteData);
            }
            pSpriteData = SpriteAllocFromAnimFileWithClut(pAnimFile, clutIndex * 0x10 + 0x100, (s16)(spriteId + 0x1E0),
                                                          texX, texY, 0x40, clutIndex);
        }
        g_FieldActors[actorIndex].pSpriteData = pSpriteData;
    } else {
        if (g_FieldActors[actorIndex].spriteFlags & 1) {
            SpriteDestroy(g_FieldActors[actorIndex].pSpriteData);
        }
        if (modelKind == 1) {
            pSpriteData = SpriteAllocFromAnimFile(pAnimFile, 0x100, (s16)(spriteId + 0xE0), 0x280,
                                                  (s16)(spriteId * 0x40 + 0x100), 8);
        } else {
            pSpriteData = SpriteAllocFromAnimFile(pAnimFile, 0x100, (s16)(spriteId + 0xE3), 0x2A0,
                                                  (s16)(spriteId * 0x40 + 0x100), 8);
        }
        SpriteReallocFramesData(pSpriteData, 0x20);
        g_FieldActors[actorIndex].pSpriteData = pSpriteData;
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


INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80077268);
// 68.96% (544 bytes vs the original's 548). Structure is right: both loops, both
// calls and all sixteen stores land in the original's order and the tail matches
// instruction for instruction. What is left is three allocator/scheduler
// tie-breaks, not a source shape:
// (1) head - the original loads g_PlayerActorIndex a second time for the address
// of the ActorData it reads position 0x26 from (`lui/lw v0`, then its own
// `sll/addu/sll/subu/sll` chain into v1 and a separate `addu v1,v1,v0`), and
// keeps that chain apart from the add of the pointer; gcc CSEs the second read of
// the global across the volatile `lw g_FieldActors` and merges the add into
// `addu v1,v0,v1`. Ordering the pointer read before the ActorData read (a
// `pPlayerActor` local for arg2) is required and gets the two `lw g_FieldActors`
// into the original's order; two separate expression spellings of the index do
// not produce the reload.
// (2) loop - identical instruction multiset, but gcc swaps $v0/$v1 on
// `li 0x200` / `lhu` / `andi` and permutes the callee-saved set (original
// s0=pActorData, s1=counter, s2=sprite, s3=actor base, s4=byte offset; ours
// s0=counter, s1=sprite, s2=actor base, s3=pActorData). Reordering the local
// declarations and a `u16 status` temp both leave it unchanged.
// (3) the sprite copy - gcc sinks each `sw` past the next statement's address
// setup and parks the loaded value in $a0; the original stores straight out of
// $v0 (`lw v0,0(v0); nop; sw v0,0(s2)`).
// Findings worth keeping: g_FieldActors IS volatile, so every `g_FieldActors[x]`
// statement re-reads the pointer and the value at ActorData+0x26 is the s16 view
// pPosition[3] (misc6's FieldActorApplyPositionTransform idiom); FieldActor+0x4 is pSpriteData and
// +0x20/+0x24/+0x28 is transformMatrix.t[0..2].
// screened: Default 68.96 (best), cdk 68.85, 2.6.0 68.90 - not the preset; the allocator and CSE differences noted above.
/*
extern void func_80084A40(int actorIndex, int y, FieldActor* pActor, ActorData* pData);
extern int FieldCharacterIdToPartyId(int characterId);
extern void func_80081C54(int actorIndex);
extern s32 g_PlayerActorIndex;
extern s32 D_800B2360;
extern s32 D_800B2364;
extern s32 D_800B2368;

void func_80077268(void) {
    ActorData* pData;
    int i;
    SpriteData* pSpriteData;
    FieldActor* pActor;
    FieldActor* pCurActor;
    int partyId;
    int actorIndex;

    actorIndex = g_PlayerActorIndex;
    {
        FieldActor* pPlayerActor = &g_FieldActors[actorIndex];
        ActorData* pPlayerData = g_FieldActors[g_PlayerActorIndex].pActorData;
        func_80084A40(actorIndex, ((s16*)&pPlayerData->position)[3], pPlayerActor, pPlayerData);
    }

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        u16 status;

        pActor = &g_FieldActors[i];
        status = pActor->status;
        pData = pActor->pActorData;
        if ((status & 0xF80) != 0x200) {
            continue;
        }
        partyId = FieldCharacterIdToPartyId(pData->characterId);
        if (partyId == -1) {
            continue;
        }
        pCurActor = &g_FieldActors[i];
        pSpriteData = (SpriteData*)pCurActor->pSpriteData;
        if (partyId == 0) {
            continue;
        }
        func_80084A40(i, ((s16*)&pCurActor->pActorData->position)[3], pActor, pData);
        pSpriteData->position.x = ((SpriteData*)g_FieldActors[g_PlayerActorIndex].pSpriteData)->position.x;
        pSpriteData->position.y = ((SpriteData*)g_FieldActors[g_PlayerActorIndex].pSpriteData)->position.y;
        pSpriteData->position.z = ((SpriteData*)g_FieldActors[g_PlayerActorIndex].pSpriteData)->position.z;
        pActor->transformMatrix.t[0] = g_FieldActors[g_PlayerActorIndex].transformMatrix.t[0];
        pActor->transformMatrix.t[1] = g_FieldActors[g_PlayerActorIndex].transformMatrix.t[1];
        pActor->transformMatrix.t[2] = g_FieldActors[g_PlayerActorIndex].transformMatrix.t[2];
    }

    D_800B2368 = 0;
    D_800B2364 = 0;
    D_800B2360 = 0;
    for (i = 0; i < 0x20; i++) {
        func_80081C54(g_PlayerActorIndex);
    }
}
*/

// 67.56% (548 bytes, same as the original's 548): every load, store, call and the
// two loops are in place. What is left: (1) the head - the original loads the
// g_PlayerActorIndex chain into $v0/$v1 the other way round and puts the
// `addu a2,a2,v0` in the jal delay slot, where gcc emits it before the `lh` and
// burns the slot on `move a3,s0`; (2) the copy blocks - gcc sinks each
// `sw` past the next block's address setup and parks the value in $a0, the
// original stores straight out of $v0 (3 more instructions after each lw in the
// original are `lw v0,0(v0)` then `nop` then `sw v0,0(s2)`).
// Findings: (1) g_FieldActors IS volatile here - the original re-loads it once per
// statement and re-forms `&g_FieldActors[i]` after the FieldCharacterIdToPartyId
// call, so the post-call block needs its own `pCurActor = &g_FieldActors[i]`
// local (that base is used for both pSpriteData and pActorData) while a2 still
// gets the loop-top pointer; (2) `FieldActor`'s copy target at 0x20/0x24/0x28 is
// transformMatrix.t[0..2], not a position member (misc6's FieldActorApplyPositionTransform has the
// same idiom); (3) the s16 argument at ActorData+0x26 is the high half of
// position.vy, read as a short lvalue like misc6's `pPosition[1]`.
/*
extern void func_80084A40(int actorIndex, int y, FieldActor* pActor, ActorData* pData);
extern int FieldCharacterIdToPartyId(int characterId);
extern void func_80081C54(int actorIndex);
extern s32 g_PlayerActorIndex;
extern s32 D_800B2360;
extern s32 D_800B2364;
extern s32 D_800B2368;

void func_80077268(void) {
    FieldActor* pActor;
    FieldActor* pCurActor;
    ActorData* pData;
    SpriteData* pSpriteData;
    int partyId;
    int i;

    pData = g_FieldActors[g_PlayerActorIndex].pActorData;
    func_80084A40(g_PlayerActorIndex, ((s16*)&pData->position.vy)[1],
        &g_FieldActors[g_PlayerActorIndex], pData);

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
        pCurActor = &g_FieldActors[i];
        pSpriteData = (SpriteData*)pCurActor->pSpriteData;
        if (partyId == 0) {
            continue;
        }
        func_80084A40(i, ((s16*)&pCurActor->pActorData->position.vy)[1], pActor, pData);
        pSpriteData->position.x = ((SpriteData*)g_FieldActors[g_PlayerActorIndex].pSpriteData)->position.x;
        pSpriteData->position.y = ((SpriteData*)g_FieldActors[g_PlayerActorIndex].pSpriteData)->position.y;
        pSpriteData->position.z = ((SpriteData*)g_FieldActors[g_PlayerActorIndex].pSpriteData)->position.z;
        pActor->transformMatrix.t[0] = g_FieldActors[g_PlayerActorIndex].transformMatrix.t[0];
        pActor->transformMatrix.t[1] = g_FieldActors[g_PlayerActorIndex].transformMatrix.t[1];
        pActor->transformMatrix.t[2] = g_FieldActors[g_PlayerActorIndex].transformMatrix.t[2];
    }

    D_800B2368 = 0;
    D_800B2364 = 0;
    D_800B2360 = 0;
    for (i = 0; i < 0x20; i++) {
        func_80081C54(g_PlayerActorIndex);
    }
}
*/

void FieldLoadFont(void) {
    if (g_FieldSystemMode == 0) {
        FontSetExternalBuffer((void*)0x80270000);
        FontLoadFont(0x10, 0x10, 0x130, 0xE0, 0x400, 4, 0x3C0, 0x100, 0x100, 0x1FF, 0);
    }
    SystemTransferPaletteToVRAM(0x100, 0xF0);
}
