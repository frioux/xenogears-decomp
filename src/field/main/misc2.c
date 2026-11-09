#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "system/math.h"
#include "system/controller.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

extern s16 D_800B218E;
extern u_char D_800ADC1C[]; // Bit mask of each of the eight movement directions
extern u16 D_800AFE9C; // Buttons pressed this frame (player 1)

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

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80072254);
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
/*
void func_80072254(int actorIndex) {
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
*/
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
void func_80072254(int actorIndex) {
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

int func_8007234C(u_int mask, int dir) {
    int i;
    int offset;

    for (i = 0, offset = 0; i < 8; i++, offset++) {
        if (!(mask & D_800ADC1C[dir++ & 7])) {
            return offset;
        }
    }
    return 0;
}

int func_80072398(u_int mask, int dir) {
    int i;
    int offset;

    for (i = 0, offset = 0; i < 8; i++, offset++) {
        if (!(mask & D_800ADC1C[dir-- & 7])) {
            return offset;
        }
    }
    return 0;
}

void func_800723E4(SVec4* pA, SVec4* pB, SVec4* pOut) {
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

// 62.01% (520 bytes vs the original's 524): the store set and order are right, and
// every g_Scene/g_CameraEye-vy-vz store already comes out as the original's
// lui-at/%lo pair. The original additionally keeps $s0 = &g_CamInterpolation live
// across the func_80070594 call and does the whole post-call reset block with
// negative displacements off it (-0x104 = g_CameraEye.vx, -0xF4..-0xEC = g_CameraAt,
// -0xD4..-0xCC = g_CameraEye2, +0x74/+0x78 = g_Scene.sceneScrZ/sceneDIP,
// +0x7A = sceneScale, -0x50 = g_FieldCameraMode, ...). Spelling those as offsets
// from a u_char* base gets the pre-call part exactly (register difference only) but
// gcc rematerialises the constant address after the call instead of saving it, so
// the post-call block is one lui-at/%lo pair per store. Defeating that
// rematerialisation is the whole remaining problem (same wall as func_80072254).
/*
extern VECTOR g_CameraEye;
extern void func_80070594(MATRIX* pMatrix);

// The original keeps &g_CamInterpolation in $s0 for the whole function and reaches
// the camera block that sits below it (g_CameraEye..g_CameraUp at 0x800AF880) with
// negative displacements, so those stores are spelled as offsets from that base. The
// g_Scene fields it also resets are outside that block and keep their own %hi/%lo.
// Both spellings are load-bearing: g_CameraEye.vx comes out of the base register,
// g_CameraEye.vy/vz out of the symbol.
void func_8007254C(void) {
    u_char* pCam;

    pCam = (u_char*)&g_CamInterpolation;
    *(s32*)pCam = 8;
    g_CamInterpolation.eyeStepDistance = 8;
    g_Scene.unk5C = 0x400000;
    g_Scene.unk60 = 0x8000000;
    g_Scene.sceneAngle.vy = 0x800;
    *(s32*)((u_char*)&g_Scene + 0xA8) = 0;
    *(s32*)((u_char*)&g_Scene + 0xA4) = 0;
    *(s32*)((u_char*)&g_Scene + 0xA0) = 0;
    *(s32*)((u_char*)&g_Scene + 0xB4) = 0;
    *(s32*)((u_char*)&g_Scene + 0xB0) = 0;
    *(s32*)((u_char*)&g_Scene + 0xAC) = 0;
    *(s16*)((u_char*)&g_Scene + 0x98) = 0;
    *(s16*)((u_char*)&g_Scene + 0x9C) = 0;
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
    func_80070594((MATRIX*)(pCam + 0x2C));

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
// 93.36% (856 bytes vs the original's 844): field camera dolly. The structure,
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
/*
extern void func_80284EA4(void);

void func_800726E8(void) {
    int offset;

    if (g_Scene.dollySet != 0xFF && g_Scene.dollyStop != 0xFF) {
        if (g_Scene.unk66 == 0) {
            if (D_800ADC1C[((u_int)g_Scene.sceneAngle.vy & 0xFFF) >> 9] & g_Scene.dollySet) {
                if (g_Scene.unk5C != -0x400000 && g_Scene.unk5C != 0x400000) {
                    g_Scene.unk5C = 0x400000;
                    g_Scene.unk7C += 0x200;
                }
                g_Scene.unk66 = 8;
            }
        }

        if (D_800ADC1C[((u_int)g_Scene.sceneAngle.vy & 0xFFF) >> 9] & g_Scene.dollyStop) {
            offset = func_8007234C(g_Scene.dollyStop,
                ((u_int)g_Scene.sceneAngle.vy & 0xFFF) >> 9);
            if (func_80072398(g_Scene.dollyStop,
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

    if (D_800AFE9C & 0x4) {
        if (!(g_Scene.unk48 & 0x8000) && g_Scene.unk66 == 0) {
            if (!(D_800ADC1C[((g_Scene.sceneAngle.vy - 0x200) & 0xFFF) >> 9] & g_Scene.dollyStop)) {
                g_Scene.unk5C = -0x400000;
                g_Scene.unk66 = 8;
                g_Scene.unk7C -= 0x200;
            }
        }
    }

    if (D_800AFE9C & 0x8) {
        if (!(g_Scene.unk48 & 0x8000) && g_Scene.unk66 == 0) {
            if (!(D_800ADC1C[((g_Scene.sceneAngle.vy + 0x200) & 0xFFF) >> 9] & g_Scene.dollyStop)) {
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

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80072A38);

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80072D74);

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80073230);

void func_80073684(VECTOR* pEye, VECTOR* pAt) {
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
void func_80073734(long* pValues) {
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
extern s32 D_800ADC18;

int FieldMathUpdateAngle(int curAngle, int targetAngle, int delta) {
    int result;
    if (D_800ADC18 == 0) {
        result = FieldMathInterpolateAngle(curAngle, targetAngle, delta);
    } else {
        result = PSX_ANGLE(targetAngle);
    }
    return result;
}

// Blocked in C without inline asm: this function switches the stack into the
// scratchpad three times (`lui/ori $t1,0x1F8003FC; sw $sp,0($t1); addiu $t1,-4;
// addu $sp,$t1` before the jal, `addiu $sp,4; lw $sp,0($sp)` after). No C
// construct makes gcc store $sp into memory, so this and func_8007554C (which has
// the same four-insn pair around its FieldRenderModels call) need an `asm`
// statement or stay INCLUDE_ASM. It is also the FIRST owner of the unit's .rodata
// (D_8006FB04 "MATRIX    "), so the five stubs below it cannot be converted
// before it: cc1 emits C .rdata at the top of the object, ahead of every
// asm-included .section .rodata, so only a prefix of D_8006FB04..D_8006FB70 can
// move into C.
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800739C0);

void func_80073E38(void) {
    int i;
    FieldActor* pActor;
    FieldActorModelData* pModel;

    for (i = 0; i < g_FieldNumActors; i++) {
        pActor = &g_FieldActors[i];
        if (pActor->status & 0x40) {
            continue;
        }

        pModel = pActor->pModelData;
        func_800AA9DC(pModel);

        if (D_800B218E != 0) {
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
int func_8007469C(void) {
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

extern u16 D_800AFEA0; // Buttons pressed this frame (player 2)
extern u16 D_800C2694; // Buttons released this frame (player 1)
extern u16 D_800C38F8; // Buttons released this frame (player 2)
extern u16 D_800C3900; // Buttons pressed this frame, first press only (player 1)
extern u16 D_800C3908; // Buttons pressed this frame, first press only (player 2)
extern u16 D_800ADB00; // Mask of buttons polled by the field
extern s32 D_800ADBDC;
extern int D_80065848[];
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern void func_8007AE78(int controllerIndex, int* pOut);

void FieldPollControllers(void) {
    D_800AFE9C = 0;
    D_800AFEA0 = 0;
    D_800C2694 = 0;
    D_800C38F8 = 0;
    D_800C3900 = 0;
    D_800C3908 = 0;

    while (ControllerPopState()) {
        D_800AFE9C |= g_C1ButtonState & g_FieldControl.controllerBtnMask;
        D_800AFEA0 |= g_C2ButtonState;
        D_800C2694 |= g_C1ButtonStateReleased & g_FieldControl.controllerBtnMask;
        D_800C38F8 |= g_C2ButtonStateReleased;
        D_800C3900 |= g_C1ButtonStatePressedOnce & g_FieldControl.controllerBtnMask;
        D_800C3908 |= g_C2ButtonStatePressedOnce;
    }

    D_800AFE9C &= D_800ADB00;
    D_800C2694 &= D_800ADB00;
    D_800C3900 &= D_800ADB00;

    ControllerResetState();
    func_8007AE78(1, D_80065848);

    if (D_800ADC18 != 0) {
        D_800AFE9C = 0;
        D_800AFEA0 = 0;
        D_800C2694 = 0;
        D_800C38F8 = 0;
        D_800C3900 = 0;
        D_800C3908 = 0;
    }

    if (D_800ADBDC == 0) {
        D_800C2694 &= 0xFF7F;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", FieldRenderModels);

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_8007520C);
// 91.00%: the original materialises &D_800B2264 into $s0, loads 0($s0) and passes
// $s0 - 0x48 as the second argument. Every C form tried (pointer local, s32 local,
// negative array index) makes gcc rematerialise a second %hi/%lo pair instead.
/*
extern s32 D_8004F380;
extern s32 D_800B2264;
extern u8 D_800B225C;
extern u8 D_800B225D;
extern u8 D_800B225E;
const char D_8006FB1C[0xC] = "GEAR      ";

void func_8007520C(void) {
    s32* pGear;

    if (D_8004F380 == 0) {
        pGear = &D_800B2264;
        if (*pGear != 0) {
            func_80086BA8();
            SetBackColor(D_800B225C, D_800B225D, D_800B225E);
            func_801E7D14(&g_Scene.worldToScreenMatrix, (u8*)(pGear - 18),
                &g_FieldCurRenderContext->ot1, g_FieldCurRenderContextIndex, 1);
        }
        if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
            func_80281B00(D_8006FB1C);
        }
    }
}
*/

extern u_char D_800ADB05;
extern s32 D_800ADBFC;

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800752C8);
// CARRIED FORWARD: 99.60%, code matches except $a1/$a2 (original: $a1 = FieldActor,
// $a2 = ActorData; gcc gives ActorData $a1). Converting this function is blocked by
// the unit's .rodata: D_8006FB28 ("CHAR      ") is the 4th of the nine strings the
// six stubs below own, and C .rodata is linked before asm .rodata, so a stub can only
// become C as part of a PREFIX of that data region, starting at D_8006FB04:
//   D_8006FB04 func_800739C0 (285 insns)  D_8006FB10 FieldRenderModels (575 insns)
//   D_8006FB1C func_8007520C (48 insns)   D_8006FB28 func_800752C8 (101 insns)
//   D_8006FB34..FB64 func_8007554C (293)  D_8006FB70 func_800759E4 (77 insns)
// So this function needs the three above it in C first, and func_800759E4 needs all
// five. The constant below is the one the object needs.
/*
// .rodata
const char D_8006FB28[0xC] = "CHAR      ";

void func_800752C8(void) {
    FieldActor* pActor;
    int i;

    if (D_800ADB05 == 1) {
        return;
    }

    func_800250E0(g_FieldCurRenderContextIndex);
    GfxSetCurrentOT(g_FieldCurRenderContext->ot1);
    func_80024FF4(&g_Scene.worldToScreenMatrix);
    func_8001D468();
    WorkListUpdate();
    TimerWorkListUpdate();
    func_80075B44(g_FieldCurRenderContext->ot1, g_FieldCurRenderContextIndex);

    for (i = 0; i < D_800ADBFC; i++) {
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

void FieldAddPrimitives(u_long* ot, u_long* pPrimList, int size) {
    AddPrims(ot, pPrimList + size, pPrimList);
}

extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern s16 D_800B00B2;
extern s32 D_800ADB50;
extern s32 D_800B007C;
extern s16 D_800B21D4;

void func_80075484(void) {
    SVec4 eye;
    SVec4 at;

    if (D_800B00B2 == 0) {
        return;
    }
    if (D_800ADB50 != 0) {
        return;
    }

    eye.x = CONV_TO_GTE(g_CameraEye.vx);
    eye.y = CONV_TO_GTE(g_CameraEye.vy);
    eye.z = CONV_TO_GTE(g_CameraEye.vz);
    at.x = CONV_TO_GTE(g_CameraAt.vx);
    at.y = CONV_TO_GTE(g_CameraAt.vy);
    at.z = CONV_TO_GTE(g_CameraAt.vz);

    func_800273C4(D_800B007C, &eye, &at, (MATRIX*)((u_char*)&g_CameraEye + 0x1E4),
        (int*)(D_800B21D4 * 4 + (int)g_FieldCurRenderContext + 0x40CC),
        g_FieldCurRenderContextIndex);
}

// Also switches the stack into the scratchpad (0x1F8003FC) around its
// FieldRenderModels call; see the note above func_800739C0.
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_8007554C);

extern s32 D_800ADB78;

void func_80075910(void) {
    RECT rect;

    FieldPollControllers();
    func_800A2030();
    func_800805F4();
    func_8008004C(g_FieldCurRenderContext->ot3, g_FieldCurRenderContextIndex);
    DrawSync(0);
    Vsync(0);

    rect.x = SCREEN_WIDTH;
    rect.w = SCREEN_WIDTH;
    rect.h = SCREEN_HEIGHT;
    rect.y = ((D_800ADB78 + 1) & 1) << 8;
    MoveImage(&rect, 0, g_FieldCurRenderContextIndex << 8);

    PutDispEnv(&g_FieldCurRenderContext->dispEnv);
    PutDrawEnv(g_FieldCurRenderContext);
    DrawOTag(&g_FieldCurRenderContext->ot3[7]);
}


// This unit owns 0x8006FB04..0x8006FB80 of .rodata (0x7C bytes, per field.yaml):
// nine 0xC-byte strings plus D_8006FB70 = VECTOR {0, 0, 0x1000, 0}. The strings
// come from the five stubs above (each owns the dlabel at the top of its .s), and C
// .rodata is linked *before* asm .rodata, so defining only the VECTOR here shifts
// the whole overlay by 0x10. D_8006FB70 is the LAST piece of the region, so this
// function only becomes C after all five owners above are converted, in address
// order, each moving its own constant into C (see the note above func_800752C8).
INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800759E4);

void func_80075B08(SpriteData* pSpriteData, u_char* pColor) {
    if (D_800B218E == 0) {
        SpriteSetColor(pSpriteData, pColor[0], pColor[1], pColor[2]);
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80075B44);

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800764B4);

u_int func_80076A74(SpriteData* pSpriteData) {
    ActorData* pActorData;

    pActorData = g_FieldActors[((SpriteDataI3*)pSpriteData->field_0x7C)->entityId].pActorData;
    pActorData->flags |= 0x10000;
    return pActorData->flags;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80076AC0);

void func_800771B0(u_int* pPrim, int size) {
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

void func_80077544(void) {
    if (g_FieldSystemMode == 0) {
        func_8003747C((void*)0x80270000);
        FontLoadFont(0x10, 0x10, 0x130, 0xE0, 0x400, 4, 0x3C0, 0x100, 0x100, 0x1FF, 0);
    }
    SystemTransferPaletteToVRAM(0x100, 0xF0);
}
