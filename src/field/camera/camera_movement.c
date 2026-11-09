#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/actor.h"
#include "field/script_vm.h"

extern s16 g_CamEyeMovementDuration;
extern s16 g_CamAtMovementDuration;
extern s16 g_FieldCameraMode;
extern s32 D_800AF930;
extern s16 D_800AF936;
extern s16 D_800AF938;
extern s32 D_800B21D8[];
extern VECTOR g_CamAtMovementFrom;
extern s16 D_800AF93A[]; // Part of a struct
extern VECTOR g_CamAtMovementTo;
extern VECTOR g_CamEyeMovementFrom;
extern VECTOR g_CamEyeMovementTo;
extern VECTOR g_CameraEye;
extern VECTOR g_CameraEye2;
extern VECTOR g_CameraAt;
extern VECTOR g_CameraAt2;


void FieldScriptWaitForCameraMovement(void) {
    int targetFlags;
    int camMovementFlags;
    
    targetFlags = FieldScriptVMGetArgument(1);
    camMovementFlags = FIELD_CAMERA_AT_MOVEMENT_ACTIVE | FIELD_CAMERA_EYE_MOVEMENT_ACTIVE;
    
    // Is camera target movement done?
    if (g_CamAtMovementDuration == 0) {
        camMovementFlags &= ~FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
    }
    
    // Is camera position movement done?
    if (g_CamEyeMovementDuration == 0) {
        camMovementFlags &= FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
    }
    
    D_800B00C0 = 1;

    // Advance instruction pointer only when certain camera movements are done
    if (!(camMovementFlags & targetFlags)) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }
}


void func_8008FABC(void) {
    s16 angleY;

    angleY = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    g_Scene.sceneAngle.vy = angleY;
    g_Scene.unk7C = angleY;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

INCLUDE_ASM("asm/field/nonmatchings/camera/camera_movement", func_8008FB28);

/* Best: 66.43% (112 bytes, right size). gcc 2.7.2 always defers the multiply's
   mflo/sra/store to the last insn of the block, so the D_800AF930 store lands
   after the scriptInstructionPointer read-modify-write instead of before it (and
   the RMW's `sh` therefore misses the jr delay slot). Reordering the source
   statements, storing through a local, or computing the mult into a local first
   all give the same 66.43%.
void func_8008FB28(void) {
    D_800AF93A[0] = 0x1000;
    D_800AF936 = g_Scene.sceneDIP;
    D_800AF938 = g_Scene.sceneAngle.vy;
    D_800AF930 = ((s32)g_Scene.sceneScrZ * g_Scene.sceneScale) >> 12;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
*/

/* Best: 66.43% (112 bytes). The original expands the multiply first, then fills
   the mult/mflo gap with the scene loads and the three D_800AF stores, and only
   then loads the actor; gcc hoists the actor load and the scriptInstructionPointer
   read-modify-write above the stores and leaves a nop in the jr delay slot. Both
   the assignment order above and putting the multiply first give the same 66.43%.
void func_8008FB28(void) {
    D_800AF930 = ((s32)g_Scene.sceneScrZ * g_Scene.sceneScale) >> 12;
    D_800AF93A[0] = 0x1000;
    D_800AF936 = g_Scene.sceneDIP;
    D_800AF938 = g_Scene.sceneAngle.vy;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
*/

void func_8008FB98(void) {
    g_FieldCameraMode = 1;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    D_800AF93A[0] = 0x1000;
    g_CamInterpolation.atStepDistance = 0xC;
    g_CamInterpolation.eyeStepDistance = 0xC;
    D_800AF930 = ((s32)g_Scene.sceneScrZ * g_Scene.sceneScale) >> 12;
    g_Scene.unk48 |= 0x8000;
    D_800AF938 = g_Scene.sceneAngle.vy;
    D_800AF936 = g_Scene.sceneDIP;
}

INCLUDE_ASM("asm/field/nonmatchings/camera/camera_movement", func_8008FC4C);

/* Best: 63.33% (224 bytes, right size and block layout; gcc keeps the address of
   g_FieldCameraMode in %hi/%lo instead of s0 and builds a different case order).
   Also tried a `s16* pMode = &g_FieldCameraMode;` local with the == 0 branch
   first (42.34%: gcc folds the pointer back to the symbol and, with case 0
   listed first, emits a jump table instead of the beq/slti/bnez chain).
   New: a `s16* pMode = &g_FieldCameraMode;` local with the deref INSIDE the
   switch (`switch (*pMode)`, not an if chain) and case 0 listed first does emit
   the beq/slti/bnez chain - the jump table seen earlier came from the if/else
   form - but it is still 63.33% and the same 224 bytes as the attempt below.
   The missing 20 bytes are exactly the s0 save/restore plus one lui/addiu:
   here reload rematerialises the symbol address at each of its three uses, so
   the unblocking form probably makes the address a runtime value (a pointer
   from memory) rather than a bare constant. Case order is a red herring.
void func_8008FC4C(void) {
    s32 value;

    switch (g_FieldCameraMode) {
    case 0:
        g_Scene.unk48 &= 0x7FFF;
        break;
    case 1:
        value = FieldScriptVMGetArgument(1);
        if (value == 0) {
            g_FieldCameraMode = 0;
            g_Scene.unk48 &= 0x7FFF;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
            D_800B21D8[0] = 2;
        } else {
            g_FieldCameraMode = 2;
            g_CamInterpolation.atStepDistance = value;
            g_CamInterpolation.eyeStepDistance = value;
        }
        break;
    default:
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}
*/

void FieldScriptSetCameraInterpolationStep(void) {
    g_CamInterpolation.atStepDistance = FieldScriptVMGetArgument(1);
    g_CamInterpolation.eyeStepDistance = FieldScriptVMGetArgument(3);
    
    if (g_CamInterpolation.atStepDistance == 0) {
        g_CamInterpolation.atStepDistance = 1;
    }
    if (g_CamInterpolation.eyeStepDistance == 0) {
        g_CamInterpolation.eyeStepDistance = 1;
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

// Reset initial camera target movement to current camera target
void FieldScriptResetCameraTargetMovement(void) {
    g_CamAtMovementFrom.vx = g_CameraAt2.vx;
    g_CamAtMovementFrom.vy = g_CameraAt2.vy;
    g_CamAtMovementFrom.vz = g_CameraAt2.vz;
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptSetCameraTargetMovementFrom(void) {
    g_CamAtMovementFrom.vx = CONV_FROM_GTE(FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(7)));
    g_CamAtMovementFrom.vz = CONV_FROM_GTE(FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7))); 
    g_CamAtMovementFrom.vy = CONV_FROM_GTE(FieldScriptArgument3(ARG(3), SCRIPT_READ_U8_REL(7)));
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

// Set initial camera target movement to position of an actor
void FieldScriptSetCameraTargetMovementDestToActor(void) {
    ActorData* pActor = g_FieldActors[func_8009CD7C(1)].pActorData;
    g_CamAtMovementTo.vx = pActor->position.vx;
    g_CamAtMovementTo.vy = pActor->position.vy;
    g_CamAtMovementTo.vz = pActor->position.vz;  
    g_FieldScriptMaxInstructionCount += 1;    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetCameraTargetMovementDest(void) {
    g_CamAtMovementTo.vx = CONV_FROM_GTE(FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(7)));
    g_CamAtMovementTo.vz = CONV_FROM_GTE(FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7)));
    g_CamAtMovementTo.vy = CONV_FROM_GTE(FieldScriptArgument3(ARG(3), SCRIPT_READ_U8_REL(7)));
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

// Reset initial camera position movement to current camera position
void FieldScriptResetCameraPosMovement(void) {
    g_CamEyeMovementFrom.vx = g_CameraEye2.vx;
    g_CamEyeMovementFrom.vy = g_CameraEye2.vy;
    g_CamEyeMovementFrom.vz = g_CameraEye2.vz;
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptSetCameraPosMovementFrom(void) {
    g_CamEyeMovementFrom.vx = CONV_FROM_GTE(FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(7)));
    g_CamEyeMovementFrom.vz = CONV_FROM_GTE(FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7))); 
    g_CamEyeMovementFrom.vy = CONV_FROM_GTE(FieldScriptArgument3(ARG(3), SCRIPT_READ_U8_REL(7)));
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptSetCameraPosMovementDestToActor(void) {
    ActorData* pActor = g_FieldActors[func_8009CD7C(1)].pActorData;
    g_CamEyeMovementTo.vx = pActor->position.vx;
    g_CamEyeMovementTo.vy = pActor->position.vy;
    g_CamEyeMovementTo.vz = pActor->position.vz;  
    g_FieldScriptMaxInstructionCount += 1;    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetCameraPosMovementDest(void) {
    g_CamEyeMovementTo.vx = CONV_FROM_GTE(FieldScriptArgument1(ARG(1), SCRIPT_READ_U8_REL(7)));
    g_CamEyeMovementTo.vz = CONV_FROM_GTE(FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7)));
    g_CamEyeMovementTo.vy = CONV_FROM_GTE(FieldScriptArgument3(ARG(3), SCRIPT_READ_U8_REL(7)));
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptResetCameraMovements(void) {
    g_CamAtMovementFrom.vx = g_CameraAt2.vx;
    g_CamAtMovementFrom.vy = g_CameraAt2.vy;
    g_CamAtMovementFrom.vz = g_CameraAt2.vz;
    
    g_CamAtMovementTo.vx = g_CameraAt2.vx;
    g_CamAtMovementTo.vy = g_CameraAt2.vy;
    g_CamAtMovementTo.vz = g_CameraAt2.vz;

    g_CamEyeMovementFrom.vx = g_CameraEye2.vx;
    g_CamEyeMovementFrom.vy = g_CameraEye2.vy;
    g_CamEyeMovementFrom.vz = g_CameraEye2.vz;
    
    g_CamEyeMovementTo.vx = g_CameraEye2.vx;
    g_CamEyeMovementTo.vy = g_CameraEye2.vy;
    g_CamEyeMovementTo.vz = g_CameraEye2.vz;
    
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

INCLUDE_ASM("asm/field/nonmatchings/camera/camera_movement", FieldScriptStartCameraMovement);

// Write the current interpolated value
void FieldScriptWriteCurCameraTarget(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), CONV_TO_GTE(g_CameraAt.vx));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), CONV_TO_GTE(g_CameraAt.vz));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), CONV_TO_GTE(g_CameraAt.vy));
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

// Write the current interpolated value
void FieldScriptWriteCurCameraPosition(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), CONV_TO_GTE(g_CameraEye.vx));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), CONV_TO_GTE(g_CameraEye.vz));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), CONV_TO_GTE(g_CameraEye.vy));
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

// Write the target / desired value
void FieldScriptWriteCameraTweenTarget(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), CONV_TO_GTE(g_CameraAt2.vx));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), CONV_TO_GTE(g_CameraAt2.vz));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), CONV_TO_GTE(g_CameraAt2.vy));
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

// Write the target / desired value
void FieldScriptWriteCameraTweenPosition(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), CONV_TO_GTE(g_CameraEye2.vx));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), CONV_TO_GTE(g_CameraEye2.vz));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), CONV_TO_GTE(g_CameraEye2.vy));
    g_FieldScriptMaxInstructionCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_80090C20(void) {
    if (SCRIPT_READ_U8_REL(3) == 0) {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800AF938);
    } else {
        D_800AF938 = FieldScriptVMGetInstructionArgument(1);
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void func_80090CB8(void) {
    if (SCRIPT_READ_U8_REL(3) == 0) {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800AF936);
    } else {
        D_800AF936 = FieldScriptVMGetInstructionArgument(1);
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void func_80090D50(void) {
    if (SCRIPT_READ_U8_REL(3) == 0) {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800AF930);
    } else {
        D_800AF930 = SCRIPT_IMM_ARG(1);
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void func_80090DEC(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800AF938);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), D_800AF936);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), D_800AF930);
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_80090E70(void) {
    VECTOR camAtDest;
    VECTOR camEyeDest;
    s32 yaw;
    s32 pitch;
    s32 halfDistance;
    long distance;
    u16 yawAngle;
    int pitchAngle;

    camAtDest.vx = g_CamAtMovementTo.vx;
    camAtDest.vy = g_CamAtMovementTo.vy;
    camAtDest.vz = g_CamAtMovementTo.vz;
    camEyeDest.vx = g_CamEyeMovementTo.vx;
    camEyeDest.vy = g_CamEyeMovementTo.vy;
    camEyeDest.vz = g_CamEyeMovementTo.vz;
    
    // Compute the half distance between the destination camera position and destination
    // camera target of the camera movement
    distance = FieldGetVec3Magnitude(
        CONV_TO_GTE(g_CamEyeMovementTo.vx - g_CamAtMovementTo.vx), 
        CONV_TO_GTE(g_CamEyeMovementTo.vy - g_CamAtMovementTo.vy), 
        CONV_TO_GTE(g_CamEyeMovementTo.vz - g_CamAtMovementTo.vz)
    );
    halfDistance = distance / 2;
    
    D_800AF93A[0] = 0x1000;

    // Cursed angle math
    yawAngle = ratan2(camAtDest.vz - camEyeDest.vz, camAtDest.vx - camEyeDest.vx);
    yaw = PSX_ANGLE((-yawAngle & 0xFFFF) - PSX_DEGREES(90));

    pitchAngle = -ratan2(
        FieldGetVec2Magnitude(
            CONV_TO_GTE(camEyeDest.vx - camAtDest.vx), 
            CONV_TO_GTE(camEyeDest.vz - camAtDest.vz)
        ), 
        CONV_TO_GTE(camAtDest.vy - camEyeDest.vy)
    );
    pitch = ((pitchAngle * 360) >> 12) + 91;
    
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), yaw);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), pitch);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), halfDistance);
    
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void func_80091008(VECTOR* pTarget, VECTOR* pParameter, s32 angle) {
    MATRIX matrix;
    VECTOR difference;
    VECTOR vecResult;
    SVECTOR rot;

    rot.vx = 0;
    rot.vy = angle;
    rot.vz = 0;
    PushMatrix();
    RotMatrix(&rot, &matrix);
    difference.vx = pParameter->vx - pTarget->vx;
    difference.vy = pParameter->vy - pTarget->vy;
    difference.vz = pParameter->vz - pTarget->vz;
    ApplyMatrixLV(&matrix, &difference, &vecResult);
    pTarget->vx = vecResult.vx + pParameter->vx;
    pTarget->vz = vecResult.vz + pParameter->vz;
    PopMatrix();
}

void func_800910C0(void) {
    VECTOR parameter;
    VECTOR vecResult;
    s32 angle;
    s32 temp;
    s32 factor;
    s32 yAngle;

    parameter.vx = CONV_FROM_GTE(FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD)));
    parameter.vz = CONV_FROM_GTE(FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD)));
    parameter.vy = CONV_FROM_GTE(FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD)));
    yAngle = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    temp = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    factor = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));

    // 0xC00 = 270 degrees
    angle = ((temp * 0xB60) >> 8) + 0xC00;

    // (number << 5) >> 0x10 is division by 0x800 = 180 degrees
    vecResult.vy = ((-((rsin(angle) * factor) << 5) >> 0x10) * D_800AF93A[0] * 0x10) + parameter.vy;
    vecResult.vz = ((((rcos(angle) * factor) << 5) >> 0x10) * D_800AF93A[0] * 0x10) + parameter.vz;
    vecResult.vx = parameter.vx;

    func_80091008(&vecResult, &parameter, yAngle);

    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(7), vecResult.vx >> 0x10);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(8), vecResult.vz >> 0x10);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(9), vecResult.vy >> 0x10);

    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x14;
}

void func_80091318(void) {
    VECTOR parameter;
    VECTOR vecResult;
    s32 angle;
    s32 temp_s1;
    s32 factor;
    s32 yAngle;
    u8 parameterType;

    parameterType = SCRIPT_READ_U8_REL(1);
    switch (parameterType) {
    case 0:
        parameter.vx = g_CamAtMovementFrom.vx;
        parameter.vy = g_CamAtMovementFrom.vy;
        parameter.vz = g_CamAtMovementFrom.vz;
        break;
    case 1:
        parameter.vx = g_CamAtMovementTo.vx;
        parameter.vy = g_CamAtMovementTo.vy;
        parameter.vz = g_CamAtMovementTo.vz;
        break;
    case 2:
        parameter.vx = g_CamEyeMovementFrom.vx;
        parameter.vy = g_CamEyeMovementFrom.vy;
        parameter.vz = g_CamEyeMovementFrom.vz;
        break;
    case 3:
        parameter.vx = g_CamEyeMovementTo.vx;
        parameter.vy = g_CamEyeMovementTo.vy;
        parameter.vz = g_CamEyeMovementTo.vz;
        break;
    }
    
    yAngle = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(8));
    temp_s1 = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(8));
    factor = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(8));
    
    // 0xC00 = 270 degrees
    angle = ((temp_s1 * 0xB60) >> 8) + 0xC00;

    // (number << 5) >> 0x10 is division by 0x800 = 180 degrees
    vecResult.vy = ((-((rsin(angle) * factor) << 5) >> 0x10) * D_800AF93A[0] * 0x10) + parameter.vy;
    vecResult.vz = ((((rcos(angle) * factor) << 5) >> 0x10) * D_800AF93A[0] * 0x10) + parameter.vz;
    vecResult.vx = parameter.vx;

    // Rotate difference between parameter value and the result vector by Y angle,
    // then we add it to parameter and set our result vector to it
    func_80091008(&vecResult, &parameter, yAngle);
    
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(5), vecResult.vx >> 0x10);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(6), vecResult.vz >> 0x10);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(7), vecResult.vy >> 0x10);
    
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 15;
}

void FieldScriptWriteCameraMovementParameter(void) {
    int mode;
    VECTOR movementParam;

    mode = SCRIPT_READ_U8_REL(1);
    switch (mode) { 
        case 0:
            movementParam.vx  = g_CamAtMovementFrom.vx;
            movementParam.vy = g_CamAtMovementFrom.vy;
            movementParam.vz = g_CamAtMovementFrom.vz;
            break;
        case 1:
            movementParam.vx  = g_CamAtMovementTo.vx;
            movementParam.vy = g_CamAtMovementTo.vy;
            movementParam.vz = g_CamAtMovementTo.vz;
            break;
        case 2:
            movementParam.vx  = g_CamEyeMovementFrom.vx;
            movementParam.vy = g_CamEyeMovementFrom.vy;
            movementParam.vz = g_CamEyeMovementFrom.vz;
            break;
        case 3:
            movementParam.vx  = g_CamEyeMovementTo.vx;
            movementParam.vy = g_CamEyeMovementTo.vy;
            movementParam.vz = g_CamEyeMovementTo.vz;
            break;
    }
    
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(1), CONV_TO_GTE(movementParam.vx));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(2), CONV_TO_GTE(movementParam.vz));
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(3), CONV_TO_GTE(movementParam.vy));

    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptSetCameraMovementParameter(void) {
    VECTOR parameter;
    u8 srcParameter;
    u8 destParameter;

    srcParameter = SCRIPT_READ_U8_REL(1);
    switch (srcParameter) {
    case 0:
        parameter.vx = g_CamAtMovementFrom.vx;
        parameter.vy = g_CamAtMovementFrom.vy;
        parameter.vz = g_CamAtMovementFrom.vz;
        break;
    case 1:
        parameter.vx = g_CamAtMovementTo.vx;
        parameter.vy = g_CamAtMovementTo.vy;
        parameter.vz = g_CamAtMovementTo.vz;
        break;
    case 2:
        parameter.vx = g_CamEyeMovementFrom.vx;
        parameter.vy = g_CamEyeMovementFrom.vy;
        parameter.vz = g_CamEyeMovementFrom.vz;
        break;
    case 3:
        parameter.vx = g_CamEyeMovementTo.vx;
        parameter.vy = g_CamEyeMovementTo.vy;
        parameter.vz = g_CamEyeMovementTo.vz;
        break;
    }
    
    destParameter = SCRIPT_READ_U8_REL(2);
    switch (destParameter) {
    case 0:
        g_CamAtMovementFrom.vx = parameter.vx;
        g_CamAtMovementFrom.vy = parameter.vy;
        g_CamAtMovementFrom.vz = parameter.vz;
        break;
    case 1:
        g_CamAtMovementTo.vx = parameter.vx;
        g_CamAtMovementTo.vy = parameter.vy;
        g_CamAtMovementTo.vz = parameter.vz;
        break;
    case 2:
        g_CamEyeMovementFrom.vx = parameter.vx;
        g_CamEyeMovementFrom.vy = parameter.vy;
        g_CamEyeMovementFrom.vz = parameter.vz;
        break;
    case 3:
        g_CamEyeMovementTo.vx = parameter.vx;
        g_CamEyeMovementTo.vy = parameter.vy;
        g_CamEyeMovementTo.vz = parameter.vz;
        break;
    }
    
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}
