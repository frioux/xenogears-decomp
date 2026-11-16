#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "system/math.h"
#include "system/controller.h"
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
// cdk on this function's account. Same wall as func_8007254C and func_8007520C
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
extern s32 D_800AF8E4;
extern s32 D_800AF8E8;
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

// PARK REPAIR (round 163): 96.11% (1220 bytes; the original is 1236, so this body is
// FOUR INSTRUCTIONS SHORT). The old note 89.60% came from a version that computed the
// step difference in a local, so its "the macro reproduces retail shape exactly" claim
// was wrong: retail computes the `subu` AFTER the `beq` that compares the two high
// halves, i.e. in its delay slot, and gcc only does that when the difference is
// spelled out in the second operand of the && instead of held in a declared local.
// That is why the macro above writes (target - value) three times.
// What is left is the shake tail, and it is a real statement short, not a register tie:
//  - three `if (D_800AF8Ex < 0)` guards come out as two bgez here, and
//  - the ((s16*)g_Scene.unk98)[n] casts come out as lui/addiu plus offset-0 loads where
//    retail folds the displacement into the load (`lh v0,152(v0)`), and retail stores
//    `sw $zero, g_FieldCameraShakeOffset` as a bare macro store through $at where this materialises
//    the address into a saved register.
// The tail wants a typed struct for the g_Scene +0x98 block and for the g_FieldCameraShakeOffset/E4/E8
// shake-offset vector (x at 0, y at 4, z at 8; three separate s32 symbols in the
// object, added into the g_Scene +0xA0 block, with g_Scene.unk98[0]/[1]/[2] the three
// s16 counters and +0x8/+0x14 their offset/delta pairs). Everything above the shake
// block is exact.
// screened: Default 96.11 (best) - not a preset question.
void func_80072D74(void) {
    s32* pSnapFrames;
    s32* pShakeOffset;
    s32 atStepSq;
    s32 eyeStepSq;

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
    FIELD_CAMERA_STEP(g_CameraEye.vz, g_CameraEye2.vz, g_CamInterpolation.eyeStepDistance, eyeStepSq);
    FIELD_CAMERA_STEP(g_CameraEye.vy, g_CameraEye2.vy, g_CamInterpolation.eyeStepDistance, eyeStepSq);
    FIELD_CAMERA_STEP(g_CameraAt.vx, g_CameraAt2.vx, g_CamInterpolation.atStepDistance, atStepSq);
    FIELD_CAMERA_STEP(g_CameraAt.vz, g_CameraAt2.vz, g_CamInterpolation.atStepDistance, atStepSq);
    FIELD_CAMERA_STEP(g_CameraAt.vy, g_CameraAt2.vy, g_CamInterpolation.atStepDistance, atStepSq);

    g_FieldCameraShakeOffset = 0;
    D_800AF8E4 = 0;
    D_800AF8E8 = 0;
    if (((s16*)g_Scene.unk98)[0] != 0) {
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
        D_800AF8E4 = rand() * ((s16*)g_Scene.unk98)[7];
        shakeZ = rand() * ((s16*)g_Scene.unk98)[9];
        D_800AF8E8 = shakeZ;
        if (*pShakeOffset < 0) {
            *pShakeOffset = 0;
            ((s32*)g_Scene.unk98)[2] = 0;
        }
        if (D_800AF8E4 < 0) {
            D_800AF8E4 = 0;
            ((s32*)g_Scene.unk98)[3] = 0;
        }
        if (shakeZ < 0) {
            D_800AF8E8 = 0;
            ((s32*)g_Scene.unk98)[4] = 0;
        }
        if (((s16*)g_Scene.unk98)[1] > 0) {
            ((s16*)g_Scene.unk98)[1]--;
        }
    }
}

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
extern s32 D_800AF8E4;
extern s32 D_800AF8E8;
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
//    FB04..FB80 region has to move to C with it (see the note above func_800752C8);
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
    eye.vy += D_800AF8E4;
    eye.vz = g_CameraEye.vz;
    eye.vz += D_800AF8E8;
    at.vx = g_CameraAt.vx;
    at.vx += g_FieldCameraShakeOffset;
    at.vy = g_CameraAt.vy;
    at.vy += D_800AF8E4;
    at.vz = *(s32*)pCam;
    at.vz += D_800AF8E8;

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
                                                              : pActorData->unk11E;
                    targetAngle = pActorData->rotation.vy;
                } else {
                    targetAngle = (((pActorData->curWalkmeshTriMaterial >> 11) - 2) & 7) << 9;
                }
            } else {
                updateAngle = (pActorData->flags & 0x2000) ? g_FieldActorUpdateAngle
                                                           : pActorData->unk11E;
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
void func_8007520C(void) {
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
extern const char D_8006FB28[];

// PARK REPAIR (round 163): 99.60% (400 bytes, the same as the original), re-measured
// after `GfxSetBaseMatrix` was named in config/symbol_addrs.field.txt. The body compiles
// and every instruction matches except one register pair in the actor loop: the original
// keeps pActor in $a1 and pActor->pActorData in $a2, gcc 2.7.2 puts pActorData in $a1.
// That is the entire diff - 7 lines, 6 of them the two register names swapping - so it
// is a GIV_TIE, a permuter job and not a hand job.
// The .rodata is NOT a blocker (that claim in the previous note was wrong): gcc 2.7.2
// emits a C definition at its own place in the source, so a
// `const char D_8006FB28[0xC] = "CHAR      ";` written here, between the
// FieldRenderModels wrapper and this function, lands exactly where the original has it.
// Proven by func_8007520C, which converted that way and passes check_bin.
// screened: Default 99.60 (best), cdk 93.87, 2.6.0 99.60 - not a preset question.
/*
void func_800752C8(void) {
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
        // Offset-first integer arithmetic: matches the original's addu operand order
        pActor = (FieldActor*)(i * 0x5C + (int)g_FieldActors);
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
        AnimScriptTick((SpriteData*)pActor->pSpriteData);
    }

    func_800764B4(g_FieldCurRenderContext->ot1, g_FieldCurRenderContextIndex);

    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_80281B00(D_8006FB28);
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800752C8);

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
const VECTOR D_8006FB70 = { 0, 0, 0x1000, 0 };

// Builds the rotation of pMat as a frame orthonormal to pVec: the normal of the
// cross product of the reference axis with pVec, pVec itself, then their cross
// product.
void func_800759E4(MATRIX* pMat, VECTOR* pVec) {
    VECTOR axis;
    VECTOR normal;
    VECTOR cross;

    axis = D_8006FB70;
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

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80076AC0);

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
