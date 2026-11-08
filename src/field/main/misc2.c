#include "common.h"
#include "field/main.h"
#include "field/actor.h"
#include "system/math.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

extern s16 D_800B218E;
extern u_char D_800ADC1C[]; // Bit mask of each of the eight movement directions

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
// 76.00%: everything matches except that the original caches &g_FieldActors in $s1
// across the RotMatrix call (`lui/addiu` once, then `lw $a1, 0($s1)` and, after the
// call, `lw $a0, 0($s1)`), so its frame is 0x30; gcc rematerialises the %hi/%lo pair
// for the second use and the frame is 0x28. Dropping `volatile` from g_FieldActors
// in actor.h gets 76% (the three scale reads then share one load, as in the
// original) but does not produce the $s1 caching, so it is not the whole story.
// Pointer locals (plain and volatile FieldActor** forms) make it worse.
/*
void func_80072254(int actorIndex) {
    VECTOR scale;

    scale.vx = g_FieldActors[actorIndex].pActorData->scaleX;
    scale.vy = g_FieldActors[actorIndex].pActorData->scaleY;
    scale.vz = g_FieldActors[actorIndex].pActorData->scaleZ;
    RotMatrix(&g_FieldActors[actorIndex].rotation,
        &g_FieldActors[actorIndex].transformMatrix);
    ScaleMatrix(&g_FieldActors[actorIndex].transformMatrix, &scale);
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

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_8007254C);

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_800726E8);

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

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", FieldPollControllers);

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
// 99.60%: only the $a1/$a2 choice is wrong. The original has $a1 = FieldActor and
// $a2 = ActorData; gcc always gives the ActorData pseudo $a1 however it is written
// (CSE'd, function-scope local, block-scope local, declaration order). The other
// difference, the `addu $a1, $s1, $v0` operand order, IS fixable: the address has to
// be written as integer arithmetic with the offset first (see below); a pointer
// expression `&g_FieldActors[i]` produces `addu $a1, $v0, $s1` in the wrong order.
// The status halfword at 0x58 is read as a 32-bit word, hence the cast. Re-add the
// rodata constant below with the function.
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

INCLUDE_ASM("asm/field/nonmatchings/main/misc2", func_80075484);
// 54.92%: the six reads are halfwords at g_CameraEye/g_CameraAt +2/+6/+A held in
// int temporaries (that is what makes gcc emit `lh` and keep all six live at once,
// so the SVec4 stores come out with `sh`), and the fourth argument is g_CameraEye +
// 0x1E4. The original materialises &g_CameraEye into $a3 with addend 0 and reads
// 2($a3); every C form tried folds the offset into the lui/%lo pair instead.
/*
extern VECTOR g_CameraEye;
extern VECTOR g_CameraAt;
extern s16 D_800B00B2;
extern s32 D_800ADB50;
extern s32 D_800B007C;
extern s16 D_800B21D4;

void func_80075484(void) {
    SVec4 eye;
    SVec4 at;
    short* pEye;
    short* pAt;
    int eyeX, eyeY, eyeZ, atX, atY, atZ;

    if (D_800B00B2 == 0) {
        return;
    }
    if (D_800ADB50 != 0) {
        return;
    }

    pEye = (short*)&g_CameraEye;
    pAt = (short*)&g_CameraAt;
    eyeX = pEye[1];
    eyeY = pEye[3];
    eyeZ = pEye[5];
    atX = pAt[1];
    atY = pAt[3];
    atZ = pAt[5];
    eye.x = eyeX;
    eye.y = eyeY;
    eye.z = eyeZ;
    at.x = atX;
    at.y = atY;
    at.z = atZ;

    func_800273C4(D_800B007C, &eye, &at, (u8*)pEye + 0x1E4,
        &g_FieldCurRenderContext->unk1 + D_800B21D4, g_FieldCurRenderContextIndex);
}
*/

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


// .rodata
// Owned by this unit's .rodata, which is not yet defined in C (see notes):
// 0x8006FB70 is the last 0x10 bytes of the 0x7C-byte block starting at 0x8006FB10.
extern const VECTOR D_8006FB70;

// Builds the sprite rotation matrix rows for a billboard from the up vector
void func_800759E4(s16* pOut, VECTOR* pUp) {
    VECTOR zAxis;
    VECTOR xAxis;
    VECTOR vecWork;

    zAxis = D_8006FB70;
    OuterProduct12(&zAxis, pUp, &vecWork);
    VectorNormal(&vecWork, &xAxis);
    OuterProduct12(&xAxis, pUp, &vecWork);
    VectorNormal(&vecWork, &zAxis);

    pOut[0] = xAxis.vx;
    pOut[1] = xAxis.vy;
    pOut[2] = xAxis.vz;
    pOut[3] = pUp->vx;
    pOut[4] = pUp->vy;
    pOut[5] = pUp->vz;
    pOut[6] = zAxis.vx;
    pOut[7] = zAxis.vy;
    pOut[8] = zAxis.vz;
}

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
