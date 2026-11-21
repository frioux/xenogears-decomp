#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/script_vm.h"
#include "field/text_box.h"

extern FieldActor* g_FieldScriptVMCurFieldActor;
extern s32 g_FieldCurActorIndex;
extern s32 g_PlayerActorIndex;
// .rodata
// Debug string for the variable print in FieldScriptVMHandlerDebugPrintVariable. Sized to the 0x14 bytes
// splat assigned this unit's rodata subsegment, so the next unit's rodata still
// starts at 0x240.
const char g_StrFieldDebugVariableFormat[0x14] = "DEB=%xh %d \n";

extern SVECTOR D_800B21AE;
extern u8 g_FieldScreenShakeMode;


void FieldScriptVMHandlerStopVM(void) {
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptFadeOut(void) {
    int duration;
    
    FieldFadeInitializePrimitives(0);
    duration = FieldScriptVMGetArgument(1);
    FieldFadeToBlack(duration);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptFadeIn(void) {
    int duration = FieldScriptVMGetArgument(1);
    FieldFadeToWhite(duration);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerWaitForFileLoad(void) {
    if (ArchiveLoadStagedFileIfChanged(FieldScriptVMGetInstructionArgument(2) & 0xFFFF, SCRIPT_READ_U8_REL(1)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    }
}

void FieldScriptVMHandlerSkipInstructions(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer += FieldScriptVMGetArgument(1) * 3 + 3;
}

int FieldGetPlayerActorDirection(void) {
    int halfDirection = PSX_DEGREES(22.5);
    return (PSX_ANGLE_TO_DIRECTION_8(g_FieldActors[g_PlayerActorIndex].pActorData->rotation.vy + halfDirection) + 2) 
        & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS;
}

void FieldScriptVMHandlerMoveToActor(void) {
    ActorData* pActorData;

    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 2;
    pActorData = g_FieldActors[SCRIPT_READ_U8_REL(1)].pActorData;
    g_FieldScriptVMCurActor->moveStep.vx = CONV_TO_GTE(pActorData->position.vx);
    g_FieldScriptVMCurActor->moveStep.vz = CONV_TO_GTE(pActorData->position.vz);
    g_FieldScriptVMCurActor->moveStep.vy = CONV_TO_GTE(pActorData->position.vy);
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(5);
    }
    if (FieldScriptVMStepActorMove(FieldScriptVMGetArgument(5)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
    }
}

void FieldScriptVMHandlerMoveToActorUnbounded(void) {
    ActorData* pActorData;

    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 2;
    pActorData = g_FieldActors[SCRIPT_READ_U8_REL(1)].pActorData;
    g_FieldScriptVMCurActor->moveStep.vx = CONV_TO_GTE(pActorData->position.vx);
    g_FieldScriptVMCurActor->moveStep.vz = CONV_TO_GTE(pActorData->position.vz);
    g_FieldScriptVMCurActor->moveStep.vy = CONV_TO_GTE(pActorData->position.vy);
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
    if (FieldScriptVMStepActorMove(0xFFFF) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    }
}

void FieldScriptVMHandlerMoveByOffset(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 == 0) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 1;
        g_FieldScriptVMCurActor->moveStep.vx = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vx);
        g_FieldScriptVMCurActor->moveStep.vy = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
        g_FieldScriptVMCurActor->moveStep.vz = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vz);
    }
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(8);
    }
    if (FieldScriptVMStepActorMove(FieldScriptVMGetArgument(8)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
    }
}

void FieldScriptVMHandlerMoveByOffsetUnbounded(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 == 0) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 1;
        g_FieldScriptVMCurActor->moveStep.vx = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vx);
        g_FieldScriptVMCurActor->moveStep.vy = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
        g_FieldScriptVMCurActor->moveStep.vz = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vz);
    }
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
    if (FieldScriptVMStepActorMove(0xFFFF) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    }
}

void FieldScriptVMHandlerMoveInDirection(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 == 0) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 3;
        g_FieldScriptVMCurActor->moveStep.vx = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vx);
        g_FieldScriptVMCurActor->moveStep.vy = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
        g_FieldScriptVMCurActor->moveStep.vz = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vz);
    }
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(5);
    }
    if (FieldScriptVMStepActorMove(FieldScriptVMGetArgument(5)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    }
}

void FieldScriptVMHandlerContinueMovement(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(8);
    }
    if (FieldScriptVMStepActorMove(FieldScriptVMGetArgument(8)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
    }
}

void FieldScriptVMHandlerContinueMovementUnbounded(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
    if (FieldScriptVMStepActorMove(0xFFFF) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    }
}

int FieldScriptVMStepActorMove(int arg0) {
    FieldActor* pFieldActor;
    ActorData* pActorData;
    ActorData* pTargetActor;
    SpriteData* pSpriteData;
    VECTOR unused; // never referenced: it is what makes the frame 0x88
    VECTOR delta;
    VECTOR normal;
    VECTOR move;
    s32 depth;
    s32 magnitude;
    s32 distance;
    s32 curX;
    s32 curY;
    s32 curZ;
    s32 angle;
    s32 rotationAngle;
    ActorData* pTailActor;
    s32 state = -1;
    s32 y = 0;
    s32 z = 0;
    s32 x = 0;
    s32 radius = 0;

    pFieldActor = g_FieldActors;
    pActorData = pFieldActor[g_FieldCurActorIndex].pActorData;
    pSpriteData = pFieldActor[g_FieldCurActorIndex].pSpriteData;
    if (pActorData->flags & 0x2000) {
        pSpriteData->moveSpeed = 0x8000000 / g_FieldScriptVMCurActor->moveSpeed;
    } else {
        pSpriteData->moveSpeed = 0x4000000 / g_FieldScriptVMCurActor->moveSpeed;
    }
    magnitude = FieldGetVec1Magnitude(pSpriteData->moveSpeed >> 15) + 1;
    radius = 0; // a declaration initialiser would put this zero in the prologue


    switch (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17) {
    case 0:
        x = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
        z = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
        y = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(5));
        break;

    case 1:
        x = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)) + g_FieldScriptVMCurActor->moveStep.vx;
        z = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5)) + g_FieldScriptVMCurActor->moveStep.vz;
        y = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(5)) + g_FieldScriptVMCurActor->moveStep.vy;
        break;

    case 2:
        if (FieldScriptVMGetActorIndex(1) == ACTOR_ID_INVALID) {
            return 0;
        }
        pTargetActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        radius = FieldGetVec1Magnitude(pTargetActor->solidRange + g_FieldScriptVMCurActor->solidRange);
        x = g_FieldScriptVMCurActor->moveStep.vx;
        z = g_FieldScriptVMCurActor->moveStep.vz;
        y = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(4));
        break;

    case 3:
        angle = FieldScriptVMGetArgument(1) & 0xFFF;
        x = (g_FieldScriptVMCurActor->moveStep.vx + rsin(angle) * 0x20) >> 12;
        z = g_FieldScriptVMCurActor->moveStep.vz + (-(rcos(angle) << 5) >> 12);
        y = g_FieldScriptVMCurActor->moveStep.vy + FieldScriptArgument1(3, SCRIPT_READ_U8_REL(7));
        break;
    }

    curX = ((s16*)&g_FieldScriptVMCurActor->position.vx)[1];
    curZ = ((s16*)&g_FieldScriptVMCurActor->position.vz)[1];
    curY = ((s16*)&g_FieldScriptVMCurActor->position.vy)[1];
    delta.vx = curX - x;
    delta.vy = curY - y;
    delta.vz = curZ - z;
    VectorNormal(&delta, &normal);
    depth = pSpriteData->moveSpeed >> 8;
    move.vx = -((normal.vx * depth) >> 4);
    move.vy = -((normal.vy * depth) >> 4);
    move.vz = -((normal.vz * depth) >> 4);
    g_FieldScriptVMCurActor->move.vx = move.vx;
    g_FieldScriptVMCurActor->move.vy = move.vy;
    g_FieldScriptVMCurActor->move.vz = move.vz;
    g_FieldScriptVMCurActor->move.vy = 0;
    distance = FieldGetVec3Magnitude(x - curX, y - curY, z - curZ);
    if (((s16*)&g_FieldScriptVMCurActor->move.vx)[1] == 0
        && ((s16*)&g_FieldScriptVMCurActor->move.vz)[1] == 0) {
        state = 0;
    }
    pTailActor = g_FieldScriptVMCurActor;
    pTailActor->scriptFlags.flags |= 0x400000;

    if (pTailActor->scripts[pTailActor->curScriptIndex].flags_0 == 0
        || magnitude + radius >= distance) {
        if (state == -1) {
            if (arg0 != 0) {
                u_short rotation;

                if (!(pTailActor->scriptFlags.flags & 0x8000)) {
                    rotation = (u_short)pTailActor->rotation.vy | 0x8000;
                } else {
                    rotation = (u_short)pTailActor->lockedRotationY | 0x8000;
                }
                pTailActor->rotation.vx = rotation;
                pTailActor->rotation.vy = rotation;
            } else {
                rotationAngle = FieldGetVec2Angle((IVEC3*)&move) | 0x8000;
                g_FieldScriptVMCurActor->rotation.vx = rotationAngle;
                g_FieldScriptVMCurActor->rotation.vy = rotationAngle;
            }
        }
        g_FieldScriptVMCurActor->scriptYPos = (g_FieldScriptVMCurActor->position.vy + move.vy) >> 16;
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 0;
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
        return 0;
    }
    if (state == -1) {
        rotationAngle = FieldGetVec2Angle((IVEC3*)&move) | 0x8000;
        g_FieldScriptVMCurActor->rotation.vx = rotationAngle;
        g_FieldScriptVMCurActor->rotation.vy = rotationAngle;
    }
    g_FieldScriptVMCurActor->scriptYPos = (g_FieldScriptVMCurActor->position.vy + move.vy) >> 16;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x40000;
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0--;
    g_FieldScriptVMStopFlag = 1;
    return -1;
}

void FieldScriptVMHandlerWalkToActor(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 2;
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(2);
    }
    if (func_80099AC0(FieldScriptVMGetArgument(2)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    }
}

void FieldScriptVMHandlerWalkToActorUnbounded(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 2;
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
    if (func_80099AC0(0xFFFF) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
}

void FieldScriptVMHandlerWalkInDirection(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 == 0) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 3;
        g_FieldScriptVMCurActor->moveStep.vx = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vx);
        g_FieldScriptVMCurActor->moveStep.vy = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
        g_FieldScriptVMCurActor->moveStep.vz = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vz);
    }
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(3);
    }
    if (func_80099AC0(FieldScriptVMGetArgument(3)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    }
}

void FieldScriptVMHandlerWalkByOffset(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 == 0) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 1;
        g_FieldScriptVMCurActor->moveStep.vx = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vx);
        g_FieldScriptVMCurActor->moveStep.vy = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
        g_FieldScriptVMCurActor->moveStep.vz = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vz);
    }
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(6);
    }
    if (func_80099AC0(FieldScriptVMGetArgument(6)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    }
}

void FieldScriptVMHandlerWalkByOffsetUnbounded(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 == 0) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 1;
        g_FieldScriptVMCurActor->moveStep.vx = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vx);
        g_FieldScriptVMCurActor->moveStep.vy = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
        g_FieldScriptVMCurActor->moveStep.vz = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vz);
    }
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
    if (func_80099AC0(0xFFFF) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    }
}

void FieldScriptVMHandlerWalkToPosition(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 0;
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(6);
    }
    if (func_80099AC0(FieldScriptVMGetArgument(6)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_800984EC);

/* Best attempt (91.25%, 208 bytes = retail's, 52/52 instructions). Blocks 1-3
   match exactly once the three halfwords are typed as one SVECTOR (0x800B21AE,
   vx/vy/vz); the two s16 externs the earlier park used (78.46%) gave gcc a
   different allocation for blocks 2-3 and lost 12 bytes. Residual is the tail
   only: retail loads g_FieldScriptVMCurActor into v1 right after the
   g_FieldScreenShakeMode lbu and takes the ip `lhu` AFTER the `sb`, so its
   addiu needs a load-delay nop; this body takes the actor pointer in a0 after
   the sb and the `lhu` first. Tried: `do { } while (0);` after the third store
   (same 91.25%, that barrier is what moves the load off the store), a second
   barrier (90.77%: right tail order but 54 insns, duplicate actor load), a
   local s16 for the third result, an explicit ActorData* local for the
   increment, an explicit ip temp, and the `u8 shakeMode` two-step kept here.
void func_800984EC(void) {
    D_800B21AE.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    D_800B21AE.vy = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    D_800B21AE.vz = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));
    {
        u8 shakeMode;
        shakeMode = g_FieldScreenShakeMode | 0x80;
        g_FieldScreenShakeMode = shakeMode;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}
*/

void FieldScriptVMHandlerDebugPrintVariable(void) {
    if (g_FieldSystemMode == 0) {
        int nValue = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF);

        func_800379C8(g_StrFieldDebugVariableFormat, nValue, nValue);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerWriteDistance2D(void) {
    s32 x0;
    s32 z0;
    s32 x1;
    s32 z1;
    s32 address;

    x0 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xB));
    z0 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xB));
    x1 = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xB));
    z1 = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xB));
    address = SCRIPT_IMM_ARG(1);
    FieldScriptMemoryWriteU16(address, FieldGetVec2Magnitude(x1 - x0, z1 - z0));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xC;
}

void FieldScriptVMHandlerWriteDistance3D(void) {
    s32 x0;
    s32 z0;
    s32 y0;
    s32 x1;
    s32 z1;
    s32 y1;
    s32 address;

    x0 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xF));
    z0 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xF));
    y0 = FieldScriptArgument3(7, SCRIPT_READ_U8_REL(0xF));
    x1 = FieldScriptArgument4(9, SCRIPT_READ_U8_REL(0xF));
    z1 = FieldScriptArgument5(0xB, SCRIPT_READ_U8_REL(0xF));
    y1 = FieldScriptArgument5(0xD, SCRIPT_READ_U8_REL(0xF));
    address = SCRIPT_IMM_ARG(1);
    FieldScriptMemoryWriteU16(address, FieldGetVec3Magnitude(x1 - x0, y1 - y0, z1 - z0));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x10;
}

void FieldScriptVMHandlerWriteInterpolatedAngle(void) {
    s32 angle0;
    s32 angle1;
    s32 angle2;
    s32 value;

    angle0 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    angle1 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(9));
    angle2 = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(9));
    value = FieldMathInterpolateAngle(angle0, angle1, angle2);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, value);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

void FieldScriptVMHandlerWriteActorRotation(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_FieldScriptVMCurActor->rotation.vy & 0xFFF);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerWriteActorRotationByIndex(void) {
    ActorData* pActorData;
    int actorIndex;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        // gcc otherwise hoists the `li a0,2` above the branch, emptying the jal delay slot
        do {
            pActorData = g_FieldActors[actorIndex].pActorData;
        } while (0);
        FieldScriptMemoryWriteU16(
            FieldScriptVMGetInstructionArgument(2) & 0xFFFF,
            pActorData->rotation.vy & 0xFFF
        );
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptVMHandlerTeleportActor(void) {
    SpriteData* pSpriteData;

    pSpriteData = g_FieldActors[g_FieldCurActorIndex].pSpriteData;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x10000;
    g_FieldScriptVMCurActor->flags |= 0x200000;
    g_FieldScriptVMCurActor->position.vx = CONV_FROM_GTE(FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7)));
    g_FieldScriptVMCurActor->position.vz = CONV_FROM_GTE(FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7)));
    g_FieldScriptVMCurActor->position.vy = CONV_FROM_GTE(FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7)));
    g_FieldActors[g_FieldCurActorIndex].transformMatrix.t[0] = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vx);
    g_FieldActors[g_FieldCurActorIndex].transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
    g_FieldActors[g_FieldCurActorIndex].transformMatrix.t[2] = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vz);
    pSpriteData->position.x = g_FieldScriptVMCurActor->position.vx;
    pSpriteData->position.y = g_FieldScriptVMCurActor->position.vy;
    pSpriteData->position.z = g_FieldScriptVMCurActor->position.vz;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptVMHandlerMoveActorAtSpeedUnbounded(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
    FieldScriptVMHandlerMoveActorAtSpeed(0);
}

void FieldScriptVMHandlerMoveActorAtSpeedWithFrameCount(void) {
    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0xFFFF) {
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = FieldScriptVMGetArgument(0xB);
    }
    FieldScriptVMHandlerMoveActorAtSpeed(1);
}

extern s32 g_FieldCurActorIndex;
extern s32 g_FieldScriptVMStopFlag;

void FieldScriptVMHandlerMoveActorAtSpeed(s32 arg0) {
    IVEC3 prevPosition;
    s16 rotation;
    s32 animationId;
    s32 scriptArg2;
    s32 scriptArg1;
    s32 scriptArg3;
    s32 speed;
    SpriteData* pSprite;
    FieldActor* pFieldActor;
    ActorData *pActorData;

    pFieldActor = g_FieldActors;
    pActorData = pFieldActor[g_FieldCurActorIndex].pActorData;
    pSprite = pFieldActor[g_FieldCurActorIndex].pSpriteData;
    if (pActorData->flags & 0x2000) {
        speed = CONV_TO_GTE(0x8000000 / g_FieldScriptVMCurActor->moveSpeed);
    } else {
        speed = CONV_TO_GTE(0x4000000 / g_FieldScriptVMCurActor->moveSpeed);
    }
    if (speed == 0) {
        speed = 1;
    }
    
    animationId = 1;
    g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0xX = 1;
    if (SCRIPT_READ_U8_REL(1) == 0) {
        scriptArg1 = CONV_FROM_GTE(FieldScriptArgument1(2, SCRIPT_READ_U8_REL(0x8)));
        scriptArg2 = CONV_FROM_GTE(FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0x8)));
        scriptArg3 = CONV_FROM_GTE(FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0x8)));
        g_FieldScriptVMCurActor->frameCounter = FieldGetVec3Magnitude(
            CONV_TO_GTE(scriptArg1 - g_FieldScriptVMCurActor->position.vx), 
            CONV_TO_GTE(scriptArg3 - g_FieldScriptVMCurActor->position.vy), 
            CONV_TO_GTE(scriptArg2 - g_FieldScriptVMCurActor->position.vz)
        ) / speed;

        if (CONV_FROM_GTE(g_FieldScriptVMCurActor->frameCounter) == 0) {
            g_FieldScriptVMCurActor->frameCounter++;
        }
        
        g_FieldScriptVMCurActor->moveStep.vx = (scriptArg1 - g_FieldScriptVMCurActor->position.vx) / g_FieldScriptVMCurActor->frameCounter;
        g_FieldScriptVMCurActor->moveStep.vy = (scriptArg3 - g_FieldScriptVMCurActor->position.vy) / g_FieldScriptVMCurActor->frameCounter;
        g_FieldScriptVMCurActor->moveStep.vz = (scriptArg2 - g_FieldScriptVMCurActor->position.vz) / g_FieldScriptVMCurActor->frameCounter;
        if (CONV_TO_GTE(scriptArg1) != CONV_TO_GTE(g_FieldScriptVMCurActor->position.vx) || 
            CONV_TO_GTE(scriptArg2) != CONV_TO_GTE(g_FieldScriptVMCurActor->position.vz)
        ) {
            rotation = -ratan2(
                CONV_TO_GTE(g_FieldScriptVMCurActor->moveStep.vz), 
                CONV_TO_GTE(g_FieldScriptVMCurActor->moveStep.vx)
            );
            g_FieldScriptVMCurActor->rotation.vx = rotation;
            g_FieldScriptVMCurActor->rotation.vy = rotation;
        }
        g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
    } else {
        if (g_FieldScriptVMCurActor->frameCounter <= 0 || g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0) {
            g_FieldScriptVMCurActor->scriptInstructionPointer -= 9;
            if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 != 0) {
                prevPosition.x = g_FieldScriptVMCurActor->position.vx;
                prevPosition.y = g_FieldScriptVMCurActor->position.vy;
                prevPosition.z = g_FieldScriptVMCurActor->position.vz;
                g_FieldScriptVMCurActor->position.vx = CONV_FROM_GTE(FieldScriptArgument1(2, SCRIPT_READ_U8_REL(0x8)));
                g_FieldScriptVMCurActor->position.vz = CONV_FROM_GTE(FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0x8)));
                g_FieldScriptVMCurActor->position.vy = CONV_FROM_GTE(FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0x8)));
                g_FieldScriptVMCurActor->moveModified.vx = g_FieldScriptVMCurActor->position.vx - prevPosition.x;
                g_FieldScriptVMCurActor->moveModified.vy = g_FieldScriptVMCurActor->position.vy - prevPosition.y;
                g_FieldScriptVMCurActor->moveModified.vz = g_FieldScriptVMCurActor->position.vz - prevPosition.z;
            }
            animationId = g_FieldScriptVMCurActor->defaultAnimationId;
            if (arg0 == 0) {
                g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
            } else {
                g_FieldScriptVMCurActor->scriptInstructionPointer += 0xD;
            }
            g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
        } else {
            g_FieldScriptVMCurActor->position.vx += g_FieldScriptVMCurActor->moveStep.vx;
            g_FieldScriptVMCurActor->position.vz += g_FieldScriptVMCurActor->moveStep.vz;
            g_FieldScriptVMCurActor->position.vy +=  g_FieldScriptVMCurActor->moveStep.vy;
            g_FieldScriptVMCurActor->moveModified.vx = g_FieldScriptVMCurActor->moveStep.vx;
            g_FieldScriptVMCurActor->moveModified.vy = g_FieldScriptVMCurActor->moveStep.vy;
            g_FieldScriptVMCurActor->moveModified.vz = g_FieldScriptVMCurActor->moveStep.vz;
            g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0--;
            g_FieldScriptVMStopFlag = 1;
        }
        g_FieldScriptVMCurActor->frameCounter--;
        g_FieldActors[g_FieldCurActorIndex].transformMatrix.t[0] = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vx);
        g_FieldActors[g_FieldCurActorIndex].transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vy);
        g_FieldActors[g_FieldCurActorIndex].transformMatrix.t[2] = CONV_TO_GTE(g_FieldScriptVMCurActor->position.vz);
        pSprite->position.x = g_FieldScriptVMCurActor->position.vx;
        pSprite->position.y = g_FieldScriptVMCurActor->position.vy;
        pSprite->position.z = g_FieldScriptVMCurActor->position.vz;
    }
    
    if (g_FieldScriptVMCurActor->animationOverride != 0xFF) {
        animationId = g_FieldScriptVMCurActor->animationOverride;
    }
    
    if (g_FieldScriptVMCurActor->curAnimationId != animationId &&  
        !g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x18
    ) {
        g_FieldScriptVMCurActor->curAnimationId = animationId;
        FieldSetGearModelAnimationForActor(pSprite, animationId, g_FieldScriptVMCurFieldActor, g_FieldScriptVMCurActor);
    }
    func_80081F80(pSprite, g_FieldScriptVMCurActor->rotation.vx, g_FieldScriptVMCurFieldActor);
}

extern s16 g_FieldWalkmeshTriCount;

// Fills walkmeshTriIds[0..g_FieldWalkmeshTriCount-2] with the walkmesh triangle the actor stands on
extern int func_8007B1C4(int x, int z, int walkmeshId, SVECTOR* pOut, VECTOR* pNormal);
extern int FieldGetVec2Angle(IVEC3* pVec);
extern long FieldGetVec2Magnitude(long, long);

INCLUDE_ASM("asm/field/nonmatchings/main/misc7", func_80099214);
/* Best attempt: 96.45%, 1896 bytes against retail's 1900 (one instruction short,
   `scriptInstructionPointer += 0xB` in the walk tail: retail emits the
   walkmeshTriIds[i] store before the trip-count load and puts `move a2,i` in the
   bnez delay slot, gcc sinks the store into that slot). Everything else is
   instruction-identical apart from the register pair: retail keeps frameCount in
   $s2 and arg3 in $s3, gcc 2.7.2 gives this body the opposite pair and every
   knock-on follows. Six declaration-order permutations, gcc-2.6.0 (92.89%) and
   two decomp-permuter runs (1578 iterations) left both untouched.
   What settled the rest:
   - the dispatch is switch (SCRIPT_READ_U8_REL(1) & 3); case 0 takes the frame
     count straight from FieldScriptArgument4, case 1 divides the distance by it,
     case 2 is the jump (step.y from SquareRoot0 of the height field at
     SpriteData+0x1E) and case 3 either rebuilds the walkmesh triangle list
     (byte1 == 0xF), advances along the step, or restarts the instruction.
   - the shared walk tail hangs off case 0 and cases 1/2 goto it, which is what
     puts it between case 0 and case 1 in the object.
   - SpriteData+0x10 is step.y and +0x1C gravity; +0x1E is read as a signed
     halfword, hence the ((s16*)&gravity)[1] view.
   - moveFrameTotal (0xE0) must be a signed short: retail reads it with lh in the
     frameCounter < moveFrameTotal test and lhu only for its own increment.
   - VECTOR normal/point are never referenced; they only reproduce retail's
     0xC0 frame.
   - the two pass-through calls whose result is discarded (FieldScriptArgument1
     and 2 at the top of case 2, SquareRoot0(value)) are what retail does. */
/*
void func_80099214(void) {
    VECTOR normals[4];
    SVECTOR points[4];
    VECTOR normal;
    VECTOR point;
    SpriteData* pSprite;
    s32 speed;
    s32 frameCount;
    s32 arg1;
    s32 arg2;
    s32 arg3;
    s32 walkmesh;
    s32 value;
    s32 angle;
    s32 xDelta;
    s32 zDelta;
    int i;


    pSprite = g_FieldActors[g_FieldCurActorIndex].pSpriteData;
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x10000;
    switch (SCRIPT_READ_U8_REL(1) & 3) {
    case 0:
        frameCount = FieldScriptArgument4(8, SCRIPT_READ_U8_REL(0xA));
walk:
        if (frameCount == 0) {
            frameCount = 1;
        }
        arg1 = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(0xA));
        arg2 = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0xA));
        if ((SCRIPT_READ_U8_REL(1) & 0x80) == 0) {
            arg3 = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
        } else {
            walkmesh = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
            func_8007B1C4(arg1, arg2, walkmesh, &points[walkmesh], &normals[walkmesh]);
            arg3 = points[walkmesh].vy;
            g_FieldScriptVMCurActor->walkmeshId = walkmesh;
        }
        pSprite->step.y = -((s32)pSprite->gravity * frameCount / 2);
        pSprite->step.y += ((arg3 << 16) - g_FieldScriptVMCurActor->position.vy) / frameCount;
        xDelta = ((arg1 << 16) - g_FieldScriptVMCurActor->position.vx) / (frameCount + 1);
        zDelta = ((arg2 << 16) - g_FieldScriptVMCurActor->position.vz) / (frameCount + 1);
        g_FieldScriptVMCurActor->moveStep.vy = 0;
        g_FieldScriptVMCurActor->moveFrameTotal = frameCount;
        g_FieldScriptVMCurActor->frameCounter = 0;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
        g_FieldScriptVMCurActor->moveStep.vx = xDelta;
        g_FieldScriptVMCurActor->moveStep.vz = zDelta;
        break;
    case 1:
        arg1 = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(0xA));
        arg2 = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0xA));
        xDelta = (arg1 << 16) - g_FieldScriptVMCurActor->position.vx;
        zDelta = (arg2 << 16) - g_FieldScriptVMCurActor->position.vz;
        speed = FieldScriptArgument4(8, SCRIPT_READ_U8_REL(0xA));
        frameCount = FieldGetVec2Magnitude(xDelta >> 16, zDelta >> 16) / speed;
        goto walk;
    case 2:
        FieldScriptArgument1(2, SCRIPT_READ_U8_REL(0xA));
        FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0xA));
        arg3 = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
        value = -FieldScriptArgument4(8, SCRIPT_READ_U8_REL(0xA));
        speed = value * 2;
        xDelta = (arg3 << 16) - g_FieldScriptVMCurActor->position.vy;
        pSprite->step.y = -CONV_FROM_GTE(SquareRoot0(((s16*)&pSprite->gravity)[1] * speed));
        SquareRoot0(value);
        frameCount = SquareRoot0(abs(value - (xDelta >> 16)));
        if (frameCount < 0) {
            frameCount = -frameCount;
        }
        goto walk;
    case 3:
        if (SCRIPT_READ_U8_REL(1) == 0xF) {
            for (i = 0; i < g_FieldWalkmeshTriCount - 1; i++) {
                g_FieldScriptVMCurActor->walkmeshTriIds[i] = func_8007B1C4(
                    ((s16*)&g_FieldScriptVMCurActor->position)[1],
                    ((s16*)&g_FieldScriptVMCurActor->position)[5],
                    i, &points[i], &normals[i]);
            }
            g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x10000;
            g_FieldScriptVMCurActor->flags &= ~0x200000;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
        }
        if (g_FieldScriptVMCurActor->frameCounter < g_FieldScriptVMCurActor->moveFrameTotal) {
            g_FieldScriptVMCurActor->position.vx += g_FieldScriptVMCurActor->moveStep.vx;
            g_FieldScriptVMCurActor->position.vz += g_FieldScriptVMCurActor->moveStep.vz;
            g_FieldScriptVMCurActor->position.vy += pSprite->step.y;
            pSprite->step.y += pSprite->gravity;
            if (g_FieldScriptVMCurActor->moveStep.vx != 0 || g_FieldScriptVMCurActor->moveStep.vz != 0) {
                if ((g_FieldScriptVMCurActor->scriptFlags.flags & 0x8000) == 0) {
                    angle = FieldGetVec2Angle((IVEC3*)&g_FieldScriptVMCurActor->moveStep) | 0x8000;
                    g_FieldScriptVMCurActor->rotation.vx = angle;
                    g_FieldScriptVMCurActor->rotation.vy = angle;
                }
            }
        } else {
            g_FieldScriptVMCurActor->scriptInstructionPointer -= 0xB;
            arg1 = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(0xA));
            arg2 = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0xA));
            if ((SCRIPT_READ_U8_REL(1) & 0x80) != 0) {
                walkmesh = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
                g_FieldScriptVMCurActor->walkmeshTriIds[walkmesh] =
                    func_8007B1C4(arg1, arg2, walkmesh, &points[walkmesh], &normals[walkmesh]);
                arg3 = points[walkmesh].vy;
            } else {
                arg3 = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
            }
            pSprite->step.y = 0;
            g_FieldScriptVMCurActor->position.vx = arg1 << 16;
            g_FieldScriptVMCurActor->position.vy = arg3 << 16;
            g_FieldScriptVMCurActor->position.vz = arg2 << 16;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 0xD;
            g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x10000;
            g_FieldScriptVMCurActor->flags &= ~0x200000;
        }
        g_FieldActors[g_FieldCurActorIndex].transformMatrix.t[0] = ((s16*)&g_FieldScriptVMCurActor->position)[1];
        g_FieldActors[g_FieldCurActorIndex].transformMatrix.t[1] = ((s16*)&g_FieldScriptVMCurActor->position)[3];
        g_FieldActors[g_FieldCurActorIndex].transformMatrix.t[2] = ((s16*)&g_FieldScriptVMCurActor->position)[5];
        pSprite->position.x = g_FieldScriptVMCurActor->position.vx;
        pSprite->position.y = g_FieldScriptVMCurActor->position.vy;
        pSprite->position.z = g_FieldScriptVMCurActor->position.vz;
        g_FieldScriptVMCurActor->frameCounter++;
        break;
    }
    g_FieldScriptVMStopFlag = 1;
}
*/


void FieldScriptVMHandlerWalkToPositionUnbounded(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 0;
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
    if (func_80099AC0(0xFFFF) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0x6;
    }
}

long FieldGetVec3Magnitude(long x, long y, long z) {
    VECTOR vec;
    VECTOR vecSquared;

    vec.vx = x;
    vec.vy = y;
    vec.vz = z;

    Square0(&vec, &vecSquared);
    return SquareRoot0(vecSquared.vx + vecSquared.vy + vecSquared.vz);
}

long FieldGetVec2Magnitude(long x, long y) {
    VECTOR vec;
    VECTOR vecSquared;

    vec.vx = x;
    vec.vy = y;
    vec.vz = 0;
    Square0(&vec, &vecSquared);
    return SquareRoot0(vecSquared.vx + vecSquared.vy);
}

long FieldGetVec1Magnitude(long x) {
    VECTOR vec;
    VECTOR vecSquared;

    vec.vx = x;
    Square0(&vec, &vecSquared);
    return SquareRoot0(vecSquared.vx);
}

