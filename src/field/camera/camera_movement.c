#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/actor.h"
#include "field/script_vm.h"

extern s16 g_CamEyeMovementDuration[];
extern s16 g_CamAtMovementDuration[];
extern FieldCameraState D_800AF930;
extern s16 g_FieldCameraMode;
extern s32 g_FieldCameraSnapFrames[];
extern VECTOR g_CamAtMovementFrom;
extern VECTOR g_CamAtMovementTo;
extern VECTOR g_CamEyeMovementFrom;
extern VECTOR g_CamEyeMovementTo;
extern u16 g_CamMovementFlags;
extern VECTOR g_CamAtMovementCurrent;
extern VECTOR g_CamAtMovementDelta;
extern VECTOR g_CamEyeMovementCurrent;
extern VECTOR g_CamEyeMovementDelta;
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
    if (g_CamAtMovementDuration[0] == 0) {
        camMovementFlags &= ~FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
    }
    
    // Is camera position movement done?
    if (g_CamEyeMovementDuration[0] == 0) {
        camMovementFlags &= FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
    }
    
    g_FieldScriptVMStopFlag = 1;

    // Advance instruction pointer only when certain camera movements are done
    if (!(camMovementFlags & targetFlags)) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }
}


void FieldScriptSetSceneAngleY(void) {
    s16 angleY;

    angleY = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    g_Scene.sceneAngle.vy = angleY;
    g_Scene.sceneAngleYTarget = angleY;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void func_8008FB28(void) {
    D_800AF930.circleScale = 0x1000;
    D_800AF930.angleY = g_Scene.sceneAngle.vy;
    D_800AF930.dip = g_Scene.sceneDIP;
    D_800AF930.distance = ((s32)g_Scene.sceneScrZ * g_Scene.sceneScale) >> 12;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptSetCameraInterpolationMode(void) {
    g_FieldCameraMode = 1;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    D_800AF930.circleScale = 0x1000;
    g_CamInterpolation.atStepDistance = 0xC;
    g_CamInterpolation.eyeStepDistance = 0xC;
    D_800AF930.distance = ((s32)g_Scene.sceneScrZ * g_Scene.sceneScale) >> 12;
    g_Scene.flags |= 0x8000;
    D_800AF930.angleY = g_Scene.sceneAngle.vy;
    D_800AF930.dip = g_Scene.sceneDIP;
}

extern s16 g_FieldCameraModeTable[] __asm__("g_FieldCameraMode");

// The scene flag word, declared volatile so retail's read-modify-write keeps the
// address in the instruction instead of cse-ing it into a base register.
typedef struct {
    /* 0x0  */ u8 unk0[0x48];
    /* 0x48 */ volatile s32 flags;
} FieldSceneFlagsView;
extern FieldSceneFlagsView g_SceneFlags __asm__("g_Scene");

/* Best: 97.30% (240 bytes against retail's 244 / 61 insns). Every line matches
   except ONE instruction in the case-0 arm.
   SOLVED and load-bearing:
   - THE SCENE FLAG WORD HAS TO BE A VOLATILE LVALUE. Retail's read-modify-write is
     the bare `lui v0,%hi(g_Scene + 0x48)` / `lw v0,%lo(...)(v0)` ... `lui at,%hi` /
     `sw %lo(at)` pair, i.e. cc1 left the symbolic address inside each mem instead of
     cse-ing it into one base register, and cse only does that for a volatile mem (it
     never enters the address in its hash table). Measured with cc1_probe on this
     unit: the plain `g_Scene.flags &= 0x7FFF` and every CAST spelling
     (`*(volatile s32 *)&g_Scene.flags`, `*(volatile s32 *)((u8 *)&g_Scene + 0x48)`)
     all give `la $3,g_Scene+72 / lw $2,0($3) / andi / sw $2,0($3)`; only a
     volatile-qualified MEMBER gives retail's pair. include/field/main.h cannot carry
     the qualifier (field/main/misc11's matched FieldScriptVMClearSceneEncounterFlag
     depends on the base form), so this unit declares its own FieldSceneFlagsView
     aliased onto g_Scene. 83.77 -> 97.30, and it also fixes the case-1 arm's
     registers.
   - the switch needs THREE case nodes: `case 2: return;` plus `default: return;`
     with the `+= 3` after the switch. With only cases 0/1 gcc roots the case tree
     at value 0 (`beqz v1,case0` first, 2 compares) and drops retail's `slti v1,2 /
     beqz END / nop / bnez END` chain; the third node roots it at value 1 and,
     because its label is the default's, it emits no instruction at all.
   - `pMode = g_FieldCameraModeTable;` where that is a second declaration of the
     symbol as an unbounded array with an asm label. That is the only spelling that
     keeps `&g_FieldCameraMode` in $s0 across the jal (retail: lui s0 / addiu s0 /
     lh / sh zero / sh v0); the scalar, `s16* pMode = &g_FieldCameraMode` and every
     other local rematerialise lui+addiu or the maspsx $at macro per use.
   - the zero path really does bump scriptInstructionPointer TWICE (+6): retail has
     the in-branch copy and the shared tail copy. Keep both.
   RESIDUAL (1 insn / 4 bytes): retail's case-0 block is the flags load, then a COPY
   of the tail's two-insn `lui v1,%hi(g_FieldScriptVMCurActor)` / `lw v1,%lo` (its
   lui fills the load-delay slot behind the flags load), andi, flags store, then
   `j` to the tail's lhu - while the tail keeps its own copy of the load for the two
   case-1 paths. Ours leaves the `nop` there and shares the tail's load, so case 0's
   `j` targets the load itself and the body is 4 bytes short. No statement on the
   case-0 path reads the actor pointer, so this is gcc duplicating the tail's load
   into the predecessor (a cse/reorg block split), not a source shape.
   Tried and measured for the residual: per-arm `+= 3` copies (94.10 / 256 bytes /
   64 insns; 2.7.2 has no cross-jumping, the two increments never merge), two
   permuter runs on the 97.30 body (1657 + 1049 iterations, PERMUTER_NONE
   base=304 best=304), gcc 2.6.0 86.31 (the volatile form is 2.7.2-specific),
   gcc 2.7.2-cdk compile-failed, -G0 97.30. Earlier, from the 83.77 body: the
   shared tail without case 2 (76.23), m2c's goto form (76.23), `case 2: break;`
   (82.87), a `s32 flags` temp for the read-modify-write, the increment moved before
   the flag clear, and a permuter do-while around the case-0 arm (84.51) that
   deletes the arm's j so case 0 falls into case 1 - wrong code, rejected.
*/
/*
void func_8008FC4C(void) {
    s32 value;
    s16* pMode;

    pMode = g_FieldCameraModeTable;
    switch (*pMode) {
    case 0:
        g_SceneFlags.flags &= 0x7FFF;
        break;
    case 1:
        value = FieldScriptVMGetArgument(1);
        if (value == 0) {
            *pMode = 0;
            g_SceneFlags.flags &= 0x7FFF;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
            g_FieldCameraSnapFrames[0] = 2;
        } else {
            *pMode = 2;
            g_CamInterpolation.atStepDistance = value;
            g_CamInterpolation.eyeStepDistance = value;
        }
        break;
    case 2:
        return;
    default:
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}*/
INCLUDE_ASM("asm/field/nonmatchings/camera/camera_movement", func_8008FC4C);


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
    ActorData* pActor = g_FieldActors[FieldScriptVMGetActorIndexOrDefault(1)].pActorData;
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
    ActorData* pActor = g_FieldActors[FieldScriptVMGetActorIndexOrDefault(1)].pActorData;
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

// Start a camera movement. The low nibble of the first argument byte selects
// which of the four movement vectors is started, the high bit snaps the current
// camera to the movement's starting point.
// Best: 82.87% (1604 bytes against retail 1620; 401 vs 405 insns). Re-measured
// this round; the body below is the measured one.
// - LEFT: EVERY ARM IS EXACTLY ONE INSTRUCTION SHORT (measured from the branch
//   targets: case 0 starts at +0x74 in both, case 2 at +0x198 retail / +0x194
//   ours, case 3 +0x350 / +0x348, case 1 +0x504 / +0x4f8, tail +0x620 / +0x610).
//   The missing insn is the load-delay nop behind the g_FieldScriptVMCurScriptData
//   load in the trailing if: retail puts the g_CamMovementFlags read-modify-write
//   between the Current and the Delta store groups, so its lw is followed by
//   nop / addu, while ours sinks the ori/sh past both store groups, the store
//   fills the delay slot and the nop is unnecessary. 4 arms x 4 bytes = the whole
//   16-byte deficit.
// - STATEMENT ORDER IS NOT A LEVER (measured this round): moving
//   "g_CamMovementFlags |= FIELD_CAMERA_AT_MOVEMENT_ACTIVE;" from before the
//   Current stores to after the Delta stores compiles to a byte-identical object
//   (same 82.87% / 1604 bytes). gcc places that store itself, so no reordering of
//   the C can move it.
// - The old note blamed the speed arms duration store for the 16 bytes: WRONG.
//   Cases 2 and 3 already emit retail form lui at,%hi / sh %lo(Duration)(at).
// - Confirmed again: the durations must be declared as arrays and read as
//   g_...Duration[0]; retail reaches all three divides of case 0/1 through one
//   address register (la a0,Duration; lh v0,0(a0) reused) which the scalar and the
//   s16* local both lose.
// - The speed arms hoist la $s1,g_CamAtMovementTo where retail re-forms the
//   address per use (lui/lw, twice). Instruction count is equal, so it is not the
//   deficit, but it is one more allocator divergence.
// - Every arm is a full permutation of retail register assignment (case 0: retail
//   X/Y/Z = a1/a0/v1 and the From words = t1/t0/a3, ours s1/a0/v1 and a3/a2/a1).
//   The prologue and the switch dispatch are instruction-identical up to the first
//   lw of case 0, so the residual is a global-alloc/sched lottery, not a source
//   shape. Needs a permuter job, not a hand job.
// - Measured this round: gcc-2.6.0 77.19 (worse), gcc-2.7.2-cdk compile-failed.
// - Earlier rounds on this body: two permuter runs PERMUTER_NONE base=304,
//   per-arm += 3 copies 94.10 (no cross-jumping in 2.7.2).
// - The switch needs the source case order 0, 2, 3, 1 to reproduce retail block
//   layout; do not sort it.
/*
void FieldScriptStartCameraMovement(void) {
    VECTOR direction;
    VECTOR normalized;
    s16 duration;
    s32 speed;
    s32 offsetX;
    s32 offsetY;
    s32 offsetZ;
    u8 movementType;

    movementType = SCRIPT_READ_U8_REL(1) & 0xF;
    switch (movementType) {
    case 0: {
        duration = FieldScriptVMGetArgument(2);
        g_CamAtMovementDuration[0] = duration;
        if (duration == 0) {
            g_CamAtMovementDuration[0] = duration + 1;
            g_CamInterpolation.atStepDistance = 1;
        }
        offsetX = (g_CamAtMovementTo.vx - g_CamAtMovementFrom.vx) / g_CamAtMovementDuration[0];
        offsetY = (g_CamAtMovementTo.vy - g_CamAtMovementFrom.vy) / g_CamAtMovementDuration[0];
        offsetZ = (g_CamAtMovementTo.vz - g_CamAtMovementFrom.vz) / g_CamAtMovementDuration[0];
        g_CamMovementFlags |= FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
        g_CamAtMovementCurrent.vx = g_CamAtMovementFrom.vx;
        g_CamAtMovementCurrent.vy = g_CamAtMovementFrom.vy;
        g_CamAtMovementCurrent.vz = g_CamAtMovementFrom.vz;
        g_CamAtMovementDelta.vx = offsetX;
        g_CamAtMovementDelta.vy = offsetY;
        g_CamAtMovementDelta.vz = offsetZ;
        if (SCRIPT_READ_U8_REL(1) & 0x80) {
            g_CameraAt.vx = g_CamAtMovementFrom.vx;
            g_CameraAt.vy = g_CamAtMovementFrom.vy;
            g_CameraAt.vz = g_CamAtMovementFrom.vz;
        }
        break;
    }
    case 2: {
        VECTOR* pFrom = &g_CamAtMovementFrom;

        direction.vx = CONV_TO_GTE(pFrom->vx - g_CamAtMovementTo.vx);
        direction.vy = CONV_TO_GTE(pFrom->vy - g_CamAtMovementTo.vy);
        direction.vz = CONV_TO_GTE(pFrom->vz - g_CamAtMovementTo.vz);
        VectorNormal(&direction, &normalized);
        offsetX = FieldGetVec3Magnitude(
            CONV_TO_GTE(pFrom->vx - g_CamAtMovementTo.vx),
            CONV_TO_GTE(pFrom->vy - g_CamAtMovementTo.vy),
            CONV_TO_GTE(pFrom->vz - g_CamAtMovementTo.vz));
        speed = FieldScriptVMGetArgument(2);
        g_CamAtMovementCurrent.vx = pFrom->vx;
        g_CamAtMovementCurrent.vy = pFrom->vy;
        g_CamAtMovementCurrent.vz = pFrom->vz;
        g_CamMovementFlags |= FIELD_CAMERA_AT_MOVEMENT_ACTIVE;
        g_CamAtMovementDelta.vx = -(normalized.vx * speed) << 4;
        g_CamAtMovementDelta.vy = -(normalized.vy * speed) << 4;
        g_CamAtMovementDelta.vz = -(normalized.vz * speed) << 4;
        g_CamAtMovementDuration[0] = offsetX / speed;
        if (SCRIPT_READ_U8_REL(1) & 0x80) {
            g_CameraAt.vx = pFrom->vx;
            g_CameraAt.vy = pFrom->vy;
            g_CameraAt.vz = pFrom->vz;
        }
        break;
    }
    case 3: {
        VECTOR* pFrom = &g_CamEyeMovementFrom;

        direction.vx = CONV_TO_GTE(pFrom->vx - g_CamEyeMovementTo.vx);
        direction.vy = CONV_TO_GTE(pFrom->vy - g_CamEyeMovementTo.vy);
        direction.vz = CONV_TO_GTE(pFrom->vz - g_CamEyeMovementTo.vz);
        VectorNormal(&direction, &normalized);
        offsetX = FieldGetVec3Magnitude(
            CONV_TO_GTE(pFrom->vx - g_CamEyeMovementTo.vx),
            CONV_TO_GTE(pFrom->vy - g_CamEyeMovementTo.vy),
            CONV_TO_GTE(pFrom->vz - g_CamEyeMovementTo.vz));
        speed = FieldScriptVMGetArgument(2);
        g_CamEyeMovementCurrent.vx = pFrom->vx;
        g_CamEyeMovementCurrent.vy = pFrom->vy;
        g_CamEyeMovementCurrent.vz = pFrom->vz;
        g_CamMovementFlags |= FIELD_CAMERA_EYE_MOVEMENT_ACTIVE;
        g_CamEyeMovementDelta.vx = -(normalized.vx * speed) << 4;
        g_CamEyeMovementDelta.vy = -(normalized.vy * speed) << 4;
        g_CamEyeMovementDelta.vz = -(normalized.vz * speed) << 4;
        g_CamEyeMovementDuration[0] = offsetX / speed;
        if (SCRIPT_READ_U8_REL(1) & 0x80) {
            g_CameraEye.vx = pFrom->vx;
            g_CameraEye.vy = pFrom->vy;
            g_CameraEye.vz = pFrom->vz;
        }
        break;
    }
    case 1: {
        duration = FieldScriptVMGetArgument(2);
        g_CamEyeMovementDuration[0] = duration;
        if (duration == 0) {
            g_CamEyeMovementDuration[0] = duration + 1;
            g_CamInterpolation.eyeStepDistance = 1;
        }
        offsetX = (g_CamEyeMovementTo.vx - g_CamEyeMovementFrom.vx) / g_CamEyeMovementDuration[0];
        offsetY = (g_CamEyeMovementTo.vy - g_CamEyeMovementFrom.vy) / g_CamEyeMovementDuration[0];
        offsetZ = (g_CamEyeMovementTo.vz - g_CamEyeMovementFrom.vz) / g_CamEyeMovementDuration[0];
        g_CamMovementFlags |= FIELD_CAMERA_EYE_MOVEMENT_ACTIVE;
        g_CamEyeMovementCurrent.vx = g_CamEyeMovementFrom.vx;
        g_CamEyeMovementCurrent.vy = g_CamEyeMovementFrom.vy;
        g_CamEyeMovementCurrent.vz = g_CamEyeMovementFrom.vz;
        g_CamEyeMovementDelta.vx = offsetX;
        g_CamEyeMovementDelta.vy = offsetY;
        g_CamEyeMovementDelta.vz = offsetZ;
        if (SCRIPT_READ_U8_REL(1) & 0x80) {
            g_CameraEye.vx = g_CamEyeMovementFrom.vx;
            g_CameraEye.vy = g_CamEyeMovementFrom.vy;
            g_CameraEye.vz = g_CamEyeMovementFrom.vz;
        }
        break;
    }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}*/
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

void FieldScriptWriteOrSetCameraAngleY(void) {
    if (SCRIPT_READ_U8_REL(3) == 0) {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800AF930.angleY);
    } else {
        D_800AF930.angleY = FieldScriptVMGetInstructionArgument(1);
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptWriteOrSetCameraDIP(void) {
    if (SCRIPT_READ_U8_REL(3) == 0) {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800AF930.dip);
    } else {
        D_800AF930.dip = FieldScriptVMGetInstructionArgument(1);
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptWriteOrSetCameraDistance(void) {
    if (SCRIPT_READ_U8_REL(3) == 0) {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800AF930.distance);
    } else {
        D_800AF930.distance = SCRIPT_IMM_ARG(1);
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptWriteCameraDistanceDipAndAngle(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800AF930.angleY);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), D_800AF930.dip);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), D_800AF930.distance);
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void FieldScriptWriteCameraMovementDestAnglesAndDistance(void) {
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
    
    D_800AF930.circleScale = 0x1000;

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

void FieldRotateVectorAroundPointY(VECTOR* pTarget, VECTOR* pParameter, s32 angle) {
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

void FieldScriptWriteRotatedCirclePointFromArgs(void) {
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
    vecResult.vy = ((-((rsin(angle) * factor) << 5) >> 0x10) * D_800AF930.circleScale * 0x10) + parameter.vy;
    vecResult.vz = ((((rcos(angle) * factor) << 5) >> 0x10) * D_800AF930.circleScale * 0x10) + parameter.vz;
    vecResult.vx = parameter.vx;

    FieldRotateVectorAroundPointY(&vecResult, &parameter, yAngle);

    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(7), vecResult.vx >> 0x10);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(8), vecResult.vz >> 0x10);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(9), vecResult.vy >> 0x10);

    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x14;
}

void FieldScriptWriteRotatedCirclePointFromMovement(void) {
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
    vecResult.vy = ((-((rsin(angle) * factor) << 5) >> 0x10) * D_800AF930.circleScale * 0x10) + parameter.vy;
    vecResult.vz = ((((rcos(angle) * factor) << 5) >> 0x10) * D_800AF930.circleScale * 0x10) + parameter.vz;
    vecResult.vx = parameter.vx;

    // Rotate difference between parameter value and the result vector by Y angle,
    // then we add it to parameter and set our result vector to it
    FieldRotateVectorAroundPointY(&vecResult, &parameter, yAngle);
    
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
