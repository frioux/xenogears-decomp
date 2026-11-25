#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/script_vm.h"
#include "field/text_box.h"

extern s32 g_FieldEntryRoutineFinished; // Is current actor a 2D actor (0) maybe?
extern s32 g_FieldCurActorIndex;
extern s8 g_FieldPartyMoveStepRan;
extern s8 D_800B21CF[];
extern s32 g_FieldCameraSnapFrames[];
extern s32 g_FieldSpriteUpdateIndex;
extern s32 D_800B2364;
extern s32 D_800B2368;
extern u16 g_FieldCameraRelativeAngleLUT[];
extern u16 g_FieldFacingAngleLUT[];
extern s16 g_FieldCameraDirectionDelta[];
extern s8 g_FieldPartyMoveInProgress[];
extern s32 g_PlayerActorIndex;
extern s32 g_FieldPartyActorSlots[];

#define FIELD_WALK_STUCK_FRAME_LIMIT 65

// g_FieldActors is volatile, which stops gcc from filling the delay slot of a call
// that follows an index through it; read the pointer through a plain lvalue instead.
#define FIELD_ACTORS (*((FieldActor**)(void*)&g_FieldActors))

/* 97.96% (1076 bytes, retail 1080 - ONE instruction short). Parked attempt.
   The switch dispatch is not the residual: gcc already emits retail's three-insn
   form `lui at,%hi(tbl); addu at,at,v0; lw v0,%lo(tbl)(at)`, differing only in the
   table symbol (the objdiff jump-table exception). The whole residual is the
   load-delay `nop` after `lw s0,0x4C(v1)` in the case 2 arm, and it is a
   SCHEDULING placement, not a source order. Retail's arm is: index scaling, then
   `lui a0,%hi(g_FieldActors); lw a0; lui a1,%hi(g_FieldScriptVMCurActor); lw a1;
   addu v1,v1,a0; lw s0,0x4C(v1); nop; lhu v0,0x1E(s0); lhu a0,0x1E(a1)` - the two
   pointer loads ADJACENT and the base load deferred below the index's last `sll`.
   Every build measured here loads the base BEFORE that `sll` and the cur-actor
   pointer AFTER `lw s0`, so gas has a free slot and no nop is emitted (4 bytes
   short, so the jump-table exception cannot carry it either).
   ROUND 408 measurements: cur-first body (below) 97.96; target-first
   `pTargetActor->solidRange + g_FieldScriptVMCurActor->solidRange` 97.72 (its two
   `lhu` order and the addu operand order DO match retail, the two pointer loads
   are still displaced); FIELD_ACTORS non-volatile lvalue + target-first 97.72, so
   the volatile read is not what pins the schedule. The unit's own cc1 on the
   built .i with -fno-schedule-insns AND -fno-schedule-insns2 emits the SAME order
   (base load first, cur load right after `lw s0`), so no sched1/sched2 flag
   reaches it either. Presets: 2.6.0-psx 95.27, 2.7.2-cdk-psx 96.74 (it too emits
   the cur load after `lw s0`), -O3 does not compile this unit. Unit stays Default:
   under cdk 8 matched neighbours fall (FieldSceneStartDIPInterpolation 49.82,
   FieldScriptSetSceneScaleInterpolation 84.06, FieldScriptVMWaitWhileSceneFlagSet
   90.24, FieldScriptSetCurActorRotationTowardPartySlot 90.41, FieldGetCameraDirection
   98.75), under 2.6.0 five do (FieldScriptVMWriteCurCharacterID 82.00,
   FieldScriptPlayAnimationAndClearFlag16 84.29, FieldScriptVMHandlerGetActorDirection
   85.00, FieldScriptSetSceneDIPInterpolation 89.09, FieldScriptSetCurActorRotationTowardActor
   94.63). Earlier rounds tried operand order, u16 range locals, block-scoped
   pointer locals, an element-pointer local, a sum temp, x/z assigned first,
   do/while(0) around the arm, volatile reads, and THREE decomp-permuter runs.
   What got it this far: a local pointer for the shared tail (the original keeps it
   in call-clobbered a2 and re-reads the global after FieldGetVec2Angle and in the
   arrival arm, so those statements must name g_FieldScriptVMCurActor directly) and
   separate block-scoped rotation variables per arm.
   ROUND 429 re-measured from the un-parked body: 97.96% (1076 bytes) stands. Two
   spellings the above did not cover. Target-first written as
   `(FIELD_ACTORS + FieldScriptVMGetActorIndex(1))->pActorData` with
   `pTargetActor->solidRange + g_FieldScriptVMCurActor->solidRange` gives 97.72 at
   the same 1076 bytes: its lhu pair and addu operand order DO reach retail's, the
   two pointer loads still do not, so the base load stays above the index's last
   `sll`. And making the cur-actor read volatile
   through a cast pointer does NOT pin that load ahead of `lw s0` - it is still
   sunk into the load-delay slot - so the placement belongs to the scheduler and
   is not reachable from source order.
   REDO LANE re-measure: 97.96% / 1076 bytes stands, and the two spellings this round
   re-tried change nothing structural. Operands reversed to
   `pTargetActor->solidRange + g_FieldScriptVMCurActor->solidRange` gives 97.72: its
   lhu pair and `addu a0` operands then DO match retail's, but the cur-pointer load
   still sinks below `lw s0,0x4C(v1)` and fills that load-delay slot, so the arm is
   still one instruction short. A pointer local for the cur actor assigned before the
   sum (`pCurActorData = g_FieldScriptVMCurActor;`) compiles byte-identically to the
   cur-first body (97.96): gcc CSEs the global and schedules it to its single use.
   The whole 4-byte gap is retail's nop after `lw s0`, which exists only because
   retail's cur-pointer load sits above the `addu`; no C statement order emits it. */
/*
int func_80099AC0(int arg0) {
    FieldActor* pFieldActor;
    ActorData* pActorData;
    SpriteData* pSpriteData;
    ActorData* pTargetActor;
    ActorData* pTailActor;
    IVEC3 delta;
    int magnitude;
    int distance;
    int rotation;
    int angle;
    int curX;
    int curZ;
    int radius = 0;
    int z = 0;
    int x = 0;

    pFieldActor = g_FieldActors;
    pActorData = pFieldActor[g_FieldCurActorIndex].pActorData;
    pSpriteData = pFieldActor[g_FieldCurActorIndex].pSpriteData;
    if (pActorData->flags & 0x2000) {
        pSpriteData->moveSpeed = 0x8000000 / g_FieldScriptVMCurActor->moveSpeed;
    } else if (pSpriteData->moveSpeed == 0) {
        pSpriteData->moveSpeed = 0x4000000 / g_FieldScriptVMCurActor->moveSpeed;
    }
    magnitude = FieldGetVec1Magnitude(pSpriteData->moveSpeed >> 15) + 1;
    curX = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vx);
    curZ = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vz);

    switch (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17) {
    case 1:
        x = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)) + g_FieldScriptVMCurActor->moveStep.vx;
        z = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5)) + g_FieldScriptVMCurActor->moveStep.vz;
        break;

    case 2:
        if (FieldScriptVMGetActorIndex(1) == ACTOR_ID_INVALID) {
            return 0;
        }
        pTargetActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        radius = FieldGetVec1Magnitude(g_FieldScriptVMCurActor->solidRange + pTargetActor->solidRange);
        x = CONV_TO_GTE(pTargetActor->position.vx);
        z = CONV_TO_GTE(pTargetActor->position.vz);
        if (SCRIPT_READ_U8_REL(1) == g_PlayerActorIndex) {
            g_FieldScriptVMCurActor->scriptFlags.flags |= 0x200000;
        }
        break;

    case 3:
        angle = FieldScriptVMGetArgument(1) & 0xFFF;
        x = g_FieldScriptVMCurActor->moveStep.vx + ((rsin(angle) * 0x1000) >> 12);
        z = g_FieldScriptVMCurActor->moveStep.vz + (-(rcos(angle) * 0x1000) >> 12);
        break;

    case 0:
    case 4:
        x = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
        z = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
        break;
    }

    delta.x = x - curX;
    delta.y = 0;
    delta.z = z - curZ;
    distance = FieldGetVec2Magnitude(delta.x, delta.z);
    pTailActor = g_FieldScriptVMCurActor;
    pTailActor->scriptFlags.flags |= 0x400000;

    if (pTailActor->scripts[pTailActor->curScriptIndex].flags_0 == 0
        || magnitude + radius >= distance) {
        if (arg0 != 0) {
            u_short angle;

            if (!(pTailActor->scriptFlags.flags & 0x8000)) {
                angle = (u_short)pTailActor->rotation.vy | 0x8000;
            } else {
                angle = (u_short)pTailActor->lockedRotationY | 0x8000;
            }
            pTailActor->rotation.vx = angle;
            pTailActor->rotation.vy = angle;
        } else {
            int angle;

            angle = FieldGetVec2Angle(&delta);
            g_FieldScriptVMCurActor->rotation.vx = angle;
            g_FieldScriptVMCurActor->rotation.vy = angle;
        }

        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 0;
        g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x02200800;
        return 0;
    }

    pTailActor->scripts[pTailActor->curScriptIndex].flags_0--;
    rotation = FieldGetVec2Angle(&delta);
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->rotation.vx = rotation;
    g_FieldScriptVMCurActor->rotation.vy = rotation;
    return -1;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc12", func_80099AC0);

void FieldScriptVMWriteCurCharacterID(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_FieldScriptVMCurActor->characterId);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMWritePartyLeaderCharacterID(void) {
    ActorData* pActor = FIELD_ACTORS[g_PlayerActorIndex].pActorData;

    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), pActor->characterId);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerGetActorDirection(void) {
    int delta = PSX_DEGREES(22.5);

    // The direction is likely offset by two due to how g_FieldAngleToDirectionLUT is set up.
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(1),
        (PSX_ANGLE_TO_DIRECTION_8(g_FieldScriptVMCurActor->rotation.vy + delta) + 2) & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerGetActorPosition(void) {
    int nActorIndex = FieldScriptVMGetActorIndex(1);
    if (nActorIndex != ACTOR_ID_INVALID) {
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(1), g_FieldActors[nActorIndex].childMatrix.t[0]);
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(2), g_FieldActors[nActorIndex].childMatrix.t[2]);
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(3), g_FieldActors[nActorIndex].childMatrix.t[1]);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptSetDefaultAnimationId(void) {
    u_char animationID = SCRIPT_READ_U8_REL(1);

    g_FieldScriptVMCurActor->defaultAnimationId = animationID;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerPlayAnimation(void) {
    u8 animationID;

    g_FieldScriptVMCurActor->flags &= 0xFEFFFFFF;
    animationID = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->animationOverride = animationID;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptPlayAnimationAndClearFlag16(void) {
    FieldScriptVMHandlerPlayAnimation();
    g_FieldScriptVMCurActor->flags &= 0xFFFEFFFF;
}

void FieldScriptClearAnimationOverrideIfFlag16(void) {
    if (g_FieldScriptVMCurActor->flags & 0x10000) {
        g_FieldScriptVMCurActor->animationOverride = 0xFF;
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
    }
}

void FieldScriptSetCurActorRotationTowardPartySlot(void) {
    int actorIndex = g_FieldPartyActorSlots[SCRIPT_READ_U8_REL(1)];

    if (actorIndex != ACTOR_ID_INVALID) {
        ActorData* pActor = g_FieldActors[actorIndex].pActorData;
        short rotation = -ratan2(
            pActor->position.vz - g_FieldScriptVMCurActor->position.vz,
            pActor->position.vx - g_FieldScriptVMCurActor->position.vx
        ) | 0x8000;

        g_FieldScriptVMCurActor->rotation.vx = rotation;
        g_FieldScriptVMCurActor->rotation.vy = rotation;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetCurActorRotationTowardActor(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);

    if (actorIndex != ACTOR_ID_INVALID) {
        ActorData* pActor = g_FieldActors[actorIndex].pActorData;
        short rotation = -ratan2(
            pActor->position.vz - g_FieldScriptVMCurActor->position.vz,
            pActor->position.vx - g_FieldScriptVMCurActor->position.vx
        ) | 0x8000;

        g_FieldScriptVMCurActor->rotation.vx = rotation;
        g_FieldScriptVMCurActor->rotation.vy = rotation;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetSceneScaleInterpolation(void) {
    g_Scene.sceneScaleFrames = SCRIPT_READ_U8_REL(3);
    if (g_Scene.sceneScaleFrames == 0) {
        g_FieldCameraSnapFrames[0] += 2;
        g_Scene.sceneScaleFrames++;
    }
    g_Scene.sceneScaleStep = -((g_Scene.sceneScale - FieldScriptVMGetArgument(1)) << 16) / g_Scene.sceneScaleFrames;
    g_Scene.sceneScaleCurrent = g_Scene.sceneScale << 16;
    g_Scene.flags |= 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldSceneStartDIPInterpolation(int start, int duration) {
    if (duration == 0) {
        g_FieldCameraSnapFrames[0] = 2;
        duration = 1;
    }
    g_Scene.sceneDIPStep = -((g_Scene.sceneDIP - start) << 16) / duration;
    g_Scene.sceneDIPFrames = duration;
    g_Scene.sceneDIPCurrent = g_Scene.sceneDIP << 16;
    g_Scene.flags |= 8;
}

void FieldScriptSetSceneDIPInterpolation(void) {
    int start = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));

    FieldSceneStartDIPInterpolation(start, SCRIPT_READ_U8_REL(3) & 0x7F);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

int FieldGetCameraDirection(void) {
    int halfDirection = PSX_DEGREES(22.5);
    return (7 - PSX_ANGLE_TO_DIRECTION_8(g_CamInterpolation.targetAngleY - halfDirection))
        & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS;
}

void FieldScriptWriteCameraDirection(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), FieldGetCameraDirection() & 0xFFFF);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}


void FieldScriptVMWaitWhileSceneFlagSet(void) {
    if (!(g_Scene.flags & SCRIPT_READ_U8_REL(1))) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    } else {
        g_FieldScriptVMStopFlag = 1;
    }
}

void FieldScriptVMWaitWhileSceneFlagSet2(void) {
    if (!(g_Scene.flags & SCRIPT_READ_U8_REL(1))) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    } else {
        g_FieldScriptVMStopFlag = 1;
    }
}

void FieldScriptSetDollySet(void) {
    g_Scene.dollySet = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetDollyStop(void) {
    g_Scene.dollyStop = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptCos(void) {
    int nValue;
    int angle;
    int nFactor;
    int address;

    address = SCRIPT_IMM_ARG(1);
    angle = FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7));
    nFactor = FieldScriptArgument3(ARG(3),  SCRIPT_READ_U8_REL(7)); // ?
    nValue = rcos(angle) * nFactor;
    FieldScriptMemoryWriteU16(address, nValue >> 12);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptSin(void) {
    int nValue;
    int angle;
    int nFactor;
    int address;

    address = SCRIPT_IMM_ARG(1);
    angle = FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7));
    nFactor = FieldScriptArgument3(ARG(3),  SCRIPT_READ_U8_REL(7));
    nValue = rsin(angle) * nFactor;
    FieldScriptMemoryWriteU16(address, nValue >> 12);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptAtan2(void) {
    int x;
    int y;
    int address;

    address = SCRIPT_IMM_ARG(1);
    y = FieldScriptArgument2(ARG(2), SCRIPT_READ_U8_REL(7));
    x = FieldScriptArgument3(ARG(3), SCRIPT_READ_U8_REL(7));

    FieldScriptMemoryWriteU16(
        address & 0xFFFF,
        (s16) ratan2(y, x)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

int FieldScriptGetCurActorDirection(void) {
    // The direction is likely offset by two due to how g_FieldAngleToDirectionLUT is set up.
    int halfDirection = PSX_DEGREES(22.5);
    return (PSX_ANGLE_TO_DIRECTION_8(g_FieldScriptVMCurActor->rotation.vy + halfDirection) + 2) & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS;
}

void FieldSetCurrentActorRotation(int rotation) {
    short rotationValue2D;
    short rotationValue3D;

    if (g_FieldEntryRoutineFinished == 0) {
        rotationValue3D = rotation | 0x8000;
        g_FieldScriptVMCurActor->rotation.vx = rotationValue3D;
        g_FieldScriptVMCurActor->rotation.vy = rotationValue3D;
        g_FieldScriptVMCurActor->rotation.vz = rotationValue3D;
    }
    rotationValue2D = rotation | 0x8000;
    g_FieldScriptVMCurActor->rotation.vx = rotationValue2D;
    g_FieldScriptVMCurActor->rotation.vy = rotationValue2D;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetActorRotation(int angle) {
    ActorData* pActor;
    int actorIndex;
    int rotAngle3D;
    int rotAngle2D;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        if (g_FieldEntryRoutineFinished == 0) {
            rotAngle3D = angle | 0x8000;
            pActor->rotation.vx = rotAngle3D;
            pActor->rotation.vy = rotAngle3D;
            pActor->rotation.vz = rotAngle3D;

        }
        rotAngle2D = angle | 0x8000;
        pActor->rotation.vx = rotAngle2D;
        pActor->rotation.vy = rotAngle2D;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}


void FieldScriptSetActorRotationTowardActor(void) {
    ActorData* pTargetActor;
    ActorData* pActor;
    short rotation;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        if (FieldScriptVMGetActorIndex(2) != ACTOR_ID_INVALID) {
            pTargetActor = FIELD_ACTORS[FieldScriptVMGetActorIndex(2)].pActorData;
            pActor = FIELD_ACTORS[FieldScriptVMGetActorIndex(1)].pActorData;
            rotation = -ratan2(
                pTargetActor->position.vz - pActor->position.vz,
                pTargetActor->position.vx - pActor->position.vx
            ) | 0x8000;

            if (g_FieldEntryRoutineFinished == 0) {
                pActor->rotation.vx = rotation;
                pActor->rotation.vy = rotation;
                pActor->rotation.vz = rotation;
            }
            pActor->rotation.vx = rotation;
            pActor->rotation.vy = rotation;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldSetCurrentActorRotationFromCameraRelativeAngle(int rotation) {
    short rotationValue;

    rotationValue = PSX_ANGLE(rotation - g_CamInterpolation.targetAngleY) | 0x8000;
    g_FieldScriptVMCurActor->rotation.vx = rotationValue;
    g_FieldScriptVMCurActor->rotation.vy = rotationValue;

    // 3D Actor?
    if (g_FieldEntryRoutineFinished == 0) {
        g_FieldScriptVMCurActor->rotation.vz = rotationValue;
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}


extern u16 g_FieldAngleToDirectionLUT[
    // PSX_DEGREES(270) | 0x8000,
    // PSX_DEGREES(315) | 0x8000,
    // PSX_DEGREES(0) | 0x8000,
    // PSX_DEGREES(45) | 0x8000,
    // PSX_DEGREES(90) | 0x8000,
    // PSX_DEGREES(135) | 0x8000,
    // PSX_DEGREES(180) | 0x8000,
    // PSX_DEGREES(225) | 0x8000,
];

// Rotate current actor N turns clockwise, where 1 turn is 45 degrees
void FieldScriptRotateActorClockwise(void) {
    int numTurns = FieldScriptVMGetArgument(1);
    FieldSetCurrentActorRotation(g_FieldAngleToDirectionLUT[
        numTurns + FieldScriptGetCurActorDirection() & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS
    ]);
}

// Rotate current actor N turns counter-clockwise, where 1 turn is 45 degrees
void FieldScriptRotateActorCounterClockwise(void) {
    int numTurns = FieldScriptVMGetArgument(1);
    FieldSetCurrentActorRotation(g_FieldAngleToDirectionLUT[
        FieldScriptGetCurActorDirection() - numTurns & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS
    ]);
}

void FieldScriptSetActorDirection(void) {
    int directionIndex = FieldScriptVMGetArgument(2);
    FieldScriptSetActorRotation(g_FieldAngleToDirectionLUT[directionIndex]);
}

void FieldScriptSetActorDirectionRelativeToCamera(void) {
    FieldScriptSetActorRotation(
        (g_FieldAngleToDirectionLUT[FieldScriptVMGetArgument(2)] - (u_short)g_CamInterpolation.targetAngleY) & 0xFFF
    );
}

void FieldScriptVMHandlerSetCurActorRotation(void) {
    int directionIndex = FieldScriptVMGetArgument(1);
    FieldSetCurrentActorRotation(g_FieldAngleToDirectionLUT[directionIndex]);
}

void FieldScriptSetCurActorDirectionRelativeToCamera(void) {
    int directionIndex = FieldScriptVMGetArgument(1);
    FieldSetCurrentActorRotationFromCameraRelativeAngle(g_FieldAngleToDirectionLUT[directionIndex]);
}

void FieldScriptSetCurActorRotationFromTableRelativeToCamera(void) {
    short rotationValue = PSX_ANGLE(g_FieldCameraRelativeAngleLUT[SCRIPT_READ_U8_REL(1)] - g_CamInterpolation.targetAngleY) | 0x8000;

    g_FieldScriptVMCurActor->rotation.vx = rotationValue;
    g_FieldScriptVMCurActor->rotation.vy = rotationValue;

    // 3D Actor?
    if (g_FieldEntryRoutineFinished == 0) {
        g_FieldScriptVMCurActor->rotation.vz = rotationValue;
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetCurActorRotationFromTable(void) {
    short rotationValue = g_FieldFacingAngleLUT[SCRIPT_READ_U8_REL(1)] | 0x8000;

    g_FieldScriptVMCurActor->rotation.vx = rotationValue;
    g_FieldScriptVMCurActor->rotation.vy = rotationValue;

    // 3D Actor?
    if (g_FieldEntryRoutineFinished == 0) {
        g_FieldScriptVMCurActor->rotation.vz = rotationValue;
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMSetSceneFlag4000(void) {
    g_Scene.flags |= 0x4000;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMClearSceneFlag4000(void) {
    g_Scene.flags &= 0xBFFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldSceneStartScreenZInterpolation(s32 start, s32 duration) {
    if (duration != 0) {
        g_Scene.flags |= FIELD_SCENE_INTERPOLATING_SCREEN_Z;
        g_Scene.sceneScrZStep = -(s32)((g_Scene.sceneScrZ - start) << 16) / duration;
        g_Scene.sceneScrZFrames = duration;
        g_Scene.sceneScrZCurrent = g_Scene.sceneScrZ << 16;
    } else {
        g_FieldCameraSnapFrames[0] += 2;
        g_Scene.sceneScrZ = start;
        g_Scene.sceneScrZFrames = 0;
    }
    g_Scene.flags &= 0xDFFF;
}

typedef struct { s16 value; } FieldScriptGlobalS16;
extern FieldScriptGlobalS16 g_FieldWalkAbortCount;

long FieldGetVec1Magnitude(long);
long FieldGetVec2Magnitude(long, long);
int FieldGetVec2Angle(IVEC3*);
void func_8009E574(int, int);
extern s32 g_FieldCurActorIndex;

int func_8009AEE0(int actorSlotId, int x, int z, int direction) {
    FieldActor* pFieldActor;
    ActorData* pActor;
    SpriteData* pSpriteData;
    ActorData* pCurActor;
    IVEC3 delta;
    int prevActorIndex;
    int actorIndex;
    int rotation;
    int walkRotation;
    int distance;
    int magnitude;
    int positionX;
    int positionZ;
    int flagsMask;

    actorIndex = g_FieldPartyActorSlots[actorSlotId];
    if (actorIndex == ACTOR_ID_INVALID) {
        return 0;
    }
    pFieldActor = (FieldActor*)(actorIndex * sizeof(FieldActor) + (u32)g_FieldActors);
    if (pFieldActor->status & ACTOR_STATUS_INVISIBLE) {
        return 0;
    }
    pSpriteData = pFieldActor->pSpriteData;
    pActor = pFieldActor->pActorData;
    if (pSpriteData->moveSpeed == 0) {
        pSpriteData->moveSpeed = 0x4000000 / pActor->moveSpeed;
    }
    magnitude = FieldGetVec1Magnitude(pSpriteData->moveSpeed >> 15) + 1;
    positionX = CONV_TO_GTE(pActor->position.vx);
    positionZ = CONV_TO_GTE(pActor->position.vz);
    delta.x = x - positionX;
    delta.y = 0;
    delta.z = z - positionZ;
    distance = FieldGetVec2Magnitude(delta.x, delta.z);
    pActor->scriptFlags.flags |= 0x400000;
    // The goto keeps retail's block layout: the arrival tail is the fall-through and
    // the walking body sits out of line, jumping back to it.
    if (magnitude < distance) {
        goto walking;
    }

tail:
    if (!(pActor->scriptFlags.flags & 0x8000)) {
        if (direction == 0xFF) {
            rotation = (u_short)pActor->rotation.vy | 0x8000;
        } else {
            rotation = g_FieldAngleToDirectionLUT[direction] | 0x8000;
        }
    } else {
        rotation = (u_short)pActor->lockedRotationY | 0x8000;
    }
    pActor->rotation.vx = rotation;
    pActor->rotation.vy = rotation;
    do {
    // A local, not the literal: sched1 would otherwise float the first position
    // shift above the mask's lui/ori and sink the return's move below the flags load.
    flagsMask = ~0x02200800;
    pActor->position.vx = x << 16;
    pActor->position.vz = z << 16;
    pActor->stuckFrameCount = 0;
    pActor->scriptFlags.flags &= flagsMask;
    return 0;

walking:
    if (pActor->prevPosition.x == CONV_TO_GTE(pActor->position.vx)
        && pActor->prevPosition.y == CONV_TO_GTE(pActor->position.vy)
        && pActor->prevPosition.z == CONV_TO_GTE(pActor->position.vz)) {
        pActor->stuckFrameCount++;
    } else {
        pActor->stuckFrameCount = 0;
    }
    walkRotation = FieldGetVec2Angle(&delta);
    pActor->rotation.vx = walkRotation;
    pActor->rotation.vy = walkRotation;
    if ((short)pActor->stuckFrameCount < FIELD_WALK_STUCK_FRAME_LIMIT
        && g_FieldWalkAbortCount.value == 0) {
        return -1;
        do { } while (0);
    }
    pCurActor = g_FieldScriptVMCurActor;
    prevActorIndex = g_FieldCurActorIndex;
    g_FieldScriptVMCurActor = pActor;
    g_FieldCurActorIndex = g_FieldPartyActorSlots[actorSlotId];
    func_8009E574(x, z);
    g_FieldScriptVMCurActor = pCurActor;
    g_FieldCurActorIndex = prevActorIndex;
    } while (0);
    goto tail;
}

void FieldScriptVMSetFlag21CF(void) {
    D_800B21CF[0] = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMResetPartyMovement(void) {
    int i;

    D_800B21CF[0] = 0;
    g_FieldPartyMoveInProgress[0] = 0;
    D_800B2368 = 0;
    D_800B2364 = 0;
    g_FieldSpriteUpdateIndex = 0;
    g_FieldPartyMoveStepRan = 0;

    for (i = 0; i < 0x20; i++) {
        func_80081C54(g_PlayerActorIndex);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMMovePartyToLeader(void) {
    ActorData* pActor;
    int x;
    int z;
    int mask;

    pActor = g_FieldActors[g_FieldPartyActorSlots[0]].pActorData;
    x = CONV_TO_GTE(pActor->position.vx);
    z = CONV_TO_GTE(pActor->position.vz);
    mask = func_8009AEE0(0, x, z, 0xFF) == 0;
    if (func_8009AEE0(1, x, z, 0xFF) == 0) {
        mask |= 2;
    }
    if (func_8009AEE0(2, x, z, 0xFF) == 0) {
        mask |= 4;
    }
    g_FieldScriptVMStopFlag = 1;
    if (mask == 7) {
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
        g_FieldPartyMoveInProgress[0] = 0;
        g_FieldWalkAbortCount.value = 0;
        FieldScriptVMResetActorHistory();
    } else {
        g_FieldPartyMoveInProgress[0] = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
    }
}

void FieldScriptVMResetActorHistory(void) {
    int i;

    D_800B2368 = 0;
    D_800B2364 = 0;
    g_FieldSpriteUpdateIndex = 0;
    g_FieldPartyMoveStepRan = 0;

    for (i = 0; i < 0x20; i++) {
        func_80081C54(g_PlayerActorIndex);
    }
}

/* 95.05%, same instruction count (179). Everything from the else arm onwards
   matches instruction for instruction; only the 2D-actor arm differs, and it
   differs by one register: our compile materialises the `g_FieldPartyMoveStepRan = 1`
   constant into a0 and schedules its `sb` before the `sh` of
   scriptInstructionPointer, which pushes the g_FieldPartyActorSlots pointer, the loop
   counter, the 0xFF and the -0x8000 constants from a0-a3 out into a1/a2/a3/t0
   and shifts the whole arm by one register. The original materialises the 1
   into v0 *after* `sh 204(v0)` (the pointer register it just freed) and keeps
   a0-a3 for the loop. Attempts made: reversing the two source statements,
   writing the += 0x14 out in full, and a block-scoped pCurActor pointer local -
   all identical, because global.c's choice of a0 for the short-lived constant
   (and sched2's hoist of the sb) does not depend on source order. Bit idiom
   `| ~0x7FFF` is the house spelling (see FieldScriptVMHandlerRandomizeActorXRotation2 in misc6.c): `| 0x8000`
   on the short field gives lh/ori instead of lhu/or.
   Round 331 addendum - WHY source order cannot move the sb: this unit's Default
   preset compiles with -fschedule-insns ON (-O2's default for this target),
   verified with a cc1_probe snippet built --opt "-O2 -fno-schedule-insns", which
   restores retail's `lhu; addu; sh` order and leaves the store last. With the
   scheduler on, every shape tried (statements swapped, a do/while(0) around the
   +=, around the store, around both, the store made volatile, an s32
   slot-pointer local before the arm) puts the `li 1` + `sb` pair before the
   `sh`; the do-block forms that DO put the `li` after the `sh` (95.98%, 724
   bytes) push the loop's invariants after it instead and cost two instructions.
   Retail has the invariants BEFORE the lhu and the sb AFTER the sh, an order no
   source shape reached here. */
void FieldScriptVMMovePartyToTargets(void) {
    ActorData* pActor;
    short rotation;
    int i;
    int mask;

    if (FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD)) == 0x7FFF) {
        // volatile: with this unit's -fschedule-insns the plain store lets sched1
        // hoist the g_FieldPartyMoveStepRan store above it, which steals its register and shifts
        // the loop's four constants; the volatile access is the only barrier.
        *(volatile u_short *)&g_FieldScriptVMCurActor->scriptInstructionPointer += 0x14;
        g_FieldPartyMoveStepRan = 1;

        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            if (g_FieldPartyActorSlots[i] != 0xFF) {
                pActor = g_FieldActors[g_FieldPartyActorSlots[i]].pActorData;
                rotation = pActor->rotation.vy | ~0x7FFF;
                pActor->rotation.vx = rotation;
                pActor->rotation.vy = rotation;
            }
        }
    } else {
        mask = func_8009AEE0(
                   0,
                   FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD)),
                   FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD)),
                   FieldScriptVMGetArgument(0xE)
               ) == 0;
        if (func_8009AEE0(
                1,
                FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD)),
                FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD)),
                FieldScriptVMGetArgument(0x10)
            ) == 0) {
            mask |= 2;
        }
        if (func_8009AEE0(
                2,
                FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD)),
                FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD)),
                FieldScriptVMGetArgument(0x12)
            ) == 0) {
            mask |= 4;
        }
        g_FieldScriptVMStopFlag = 1;
        if (mask == 7) {
            g_FieldWalkAbortCount.value = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 0x14;
            g_FieldPartyMoveInProgress[0] = 0;
        } else {
            g_FieldPartyMoveInProgress[0] = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer--;
        }
        g_FieldPartyMoveStepRan = 1;
    }
}

void FieldScriptWriteSceneScreenZ(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_Scene.sceneScrZ);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetSceneScreenZInterpolation(void) {
    int arg1 = FieldScriptVMGetArgument(1);
    int arg3 = FieldScriptVMGetArgument(3);

    FieldSceneStartScreenZInterpolation(arg1, arg3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldSceneStartSceneAngleInterpolation(int direction, int duration) {
    int cameraDirection = FieldGetCameraDirection() & 0xFFFF;

    if (duration == 0) {
        g_FieldCameraSnapFrames[0] += 2;
        duration = 1;
    }
    g_Scene.sceneAngleStep = (g_FieldCameraDirectionDelta[cameraDirection * 8 + direction] * 0x2000000) / duration;
    g_Scene.sceneAngleFrames = duration;
    g_Scene.sceneAngleYTarget = ((direction + 4) & 7) << 9;
}

void FieldSceneStartSceneAngleRotation(int direction, int duration) {
    if (duration == 0) {
        g_FieldCameraSnapFrames[0] += 2;
        duration = 1;
    }
    if (direction == 0) {
        g_Scene.sceneAngleStep = 0x2000000 / duration;
        g_Scene.sceneAngleYTarget += 0x200;
    } else {
        g_Scene.sceneAngleStep = -0x2000000 / duration;
        g_Scene.sceneAngleYTarget -= 0x200;
    }
    g_Scene.sceneAngleFrames = duration;
}

void FieldScriptVMStartSceneAngleRotation(void) {
    if (g_Scene.sceneAngleFrames == 0) {
        FieldSceneStartSceneAngleRotation(0, FieldScriptVMGetArgument(1));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }
    g_FieldScriptVMStopFlag = 1;
}

void FieldScriptVMStartSceneAngleRotation2(void) {
    if (g_Scene.sceneAngleFrames == 0) {
        FieldSceneStartSceneAngleRotation(0, FieldScriptVMGetArgument(1));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }
    g_FieldScriptVMStopFlag = 1;
}

void FieldScriptSetSceneAngleInterpolation(void) {
    int direction = FieldScriptVMGetArgument(1);
    int duration = FieldScriptVMGetArgument(3);

    if (g_FieldEntryRoutineFinished == 0) {
        u_char angle = (direction + 4) & 7;

        g_Scene.sceneAngle.vy = angle << 9;
        g_Scene.sceneAngleYTarget = angle << 9;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (g_Scene.sceneAngleFrames == 0) {
        FieldSceneStartSceneAngleInterpolation(direction, duration);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    }
    g_FieldScriptVMStopFlag = 1;
}

void FieldScriptSaveCameraState(void) {
    if (g_Scene.sceneAngleFrames == 0) {
        g_Scene.savedCameraDirection = FieldGetCameraDirection();
        g_Scene.savedSceneScrZ = g_Scene.sceneScrZ;
        g_Scene.savedSceneDIP = g_Scene.sceneDIP;
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
    }
}

void FieldScriptRestoreCameraState(void) {
    if (g_Scene.sceneAngleFrames == 0) {
        FieldSceneStartSceneAngleInterpolation(g_Scene.savedCameraDirection, 0x20);
        FieldSceneStartScreenZInterpolation(g_Scene.savedSceneScrZ, 0x20);
        FieldSceneStartDIPInterpolation(g_Scene.savedSceneDIP, 0x20);
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
    }
}

void FieldScriptSetSceneProjection(void) {
    int direction;
    int angle;
    int screenZ;

    g_Scene.sceneDIP = FieldScriptVMGetArgument(3);
    direction = (FieldScriptVMGetArgument(1) + 4) & 7;
    angle = (g_Scene.sceneAngle.vy = direction << 9);
    g_Scene.sceneAngleYTarget = angle;
    angle = angle << 16;
    g_Scene.sceneAngleCurrent = angle;
    screenZ = FieldScriptVMGetArgument(5);
    g_Scene.sceneScrZ = screenZ;
    SetGeomScreen(screenZ);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}
