#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/script_vm.h"
#include "field/text_box.h"

extern s32 D_800ADB1C; // Is current actor a 2D actor (0) maybe?
extern s8 D_800B21CE;
extern s8 D_800B21CF[];
extern s32 D_800B21D8[];
extern s32 D_800B2360;
extern s32 D_800B2364;
extern s32 D_800B2368;
extern u16 D_800AEA44[];
extern u16 D_800AEA54[];
extern s16 D_800AEA64[];
extern s8 D_800B21CC[];
extern s32 g_PlayerActorIndex;
extern s32 D_8005A444[];

INCLUDE_ASM("asm/field/nonmatchings/main/misc12", func_80099AC0);

void FieldScriptVMWriteCurCharacterID(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_FieldScriptVMCurActor->characterId);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc12", FieldScriptVMWritePartyLeaderCharacterID);
/* 94.84%: identical instructions, but gcc hoists the `li a0,1` for
   FieldScriptVMGetInstructionArgument above the prologue instead of leaving it
   in the jal delay slot.
void FieldScriptVMWritePartyLeaderCharacterID(void) {
    ActorData* pActor = g_FieldActors[g_PlayerActorIndex].pActorData;

    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), pActor->characterId);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
} */

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

void func_8009A0FC(void) {
    u_char animationID = SCRIPT_READ_U8_REL(1);

    g_FieldScriptVMCurActor->defaultAnimationId = animationID;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerPlayAnimation(void) {
    unsigned char animationID;

    g_FieldScriptVMCurActor->flags &= 0xFEFFFFFF;
    animationID = *(u8*)&g_FieldScriptVMCurScriptData[g_FieldScriptVMCurActor->scriptInstructionPointer + 1];
    g_FieldScriptVMCurActor->unkAnimationId = animationID;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009A174(void) {
    FieldScriptVMHandlerPlayAnimation();
    g_FieldScriptVMCurActor->flags &= 0xFFFEFFFF;
}

void func_8009A1AC(void) {
    if (g_FieldScriptVMCurActor->flags & 0x10000) {
        g_FieldScriptVMCurActor->unkAnimationId = 0xFF;
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
    }
}

void func_8009A1E4(void) {
    int actorIndex = D_8005A444[SCRIPT_READ_U8_REL(1)];

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

void func_8009A2A8(void) {
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

void func_8009A34C(void) {
    g_Scene.unk8C = SCRIPT_READ_U8_REL(3);
    if (g_Scene.unk8C == 0) {
        D_800B21D8[0] += 2;
        g_Scene.unk8C++;
    }
    g_Scene.unk94 = -((g_Scene.sceneScale - FieldScriptVMGetArgument(1)) << 16) / g_Scene.unk8C;
    g_Scene.unk90 = g_Scene.sceneScale << 16;
    g_Scene.unk48 |= 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void func_8009A420(int start, int duration) {
    if (duration == 0) {
        D_800B21D8[0] = 2;
        duration = 1;
    }
    g_Scene.unk78 = -((g_Scene.sceneDIP - start) << 16) / duration;
    g_Scene.unk70 = duration;
    g_Scene.unk74 = g_Scene.sceneDIP << 16;
    g_Scene.unk48 |= 8;
}

void func_8009A490(void) {
    int start = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));

    func_8009A420(start, SCRIPT_READ_U8_REL(3) & 0x7F);
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


void func_8009A58C(void) {
    if (!(g_Scene.unk48 & SCRIPT_READ_U8_REL(1))) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    } else {
        D_800B00C0 = 1;
    }
}

void func_8009A5E0(void) {
    if (!(g_Scene.unk48 & SCRIPT_READ_U8_REL(1))) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    } else {
        D_800B00C0 = 1;
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

    if (D_800ADB1C == 0) {
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
        if (D_800ADB1C == 0) {
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


INCLUDE_ASM("asm/field/nonmatchings/main/misc12", func_8009AA00);
/* 97.58%: one extra `li a0,1`, same cause as FieldScriptVMWritePartyLeaderCharacterID.
void func_8009AA00(void) {
    ActorData* pTargetActor;
    ActorData* pActor;
    short rotation;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        if (FieldScriptVMGetActorIndex(2) != ACTOR_ID_INVALID) {
            pTargetActor = g_FieldActors[FieldScriptVMGetActorIndex(2)].pActorData;
            pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
            rotation = -ratan2(
                pTargetActor->position.vz - pActor->position.vz,
                pTargetActor->position.vx - pActor->position.vx
            ) | 0x8000;

            if (D_800ADB1C == 0) {
                pActor->rotation.vx = rotation;
                pActor->rotation.vy = rotation;
                pActor->rotation.vz = rotation;
            }
            pActor->rotation.vx = rotation;
            pActor->rotation.vy = rotation;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
} */

void func_8009AB08(int rotation) {
    short rotationValue;

    rotationValue = PSX_ANGLE(rotation - g_CamInterpolation.targetAngleY) | 0x8000;
    g_FieldScriptVMCurActor->rotation.vx = rotationValue;
    g_FieldScriptVMCurActor->rotation.vy = rotationValue;

    // 3D Actor?
    if (D_800ADB1C == 0) {
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

void func_8009AC34(void) {
    FieldScriptSetActorRotation(
        (g_FieldAngleToDirectionLUT[FieldScriptVMGetArgument(2)] - (u_short)g_CamInterpolation.targetAngleY) & 0xFFF
    );
}

void FieldScriptVMHandlerSetCurActorRotation(void) {
    int directionIndex = FieldScriptVMGetArgument(1);
    FieldSetCurrentActorRotation(g_FieldAngleToDirectionLUT[directionIndex]);
}

void func_8009ACB4(void) {
    int directionIndex = FieldScriptVMGetArgument(1);
    func_8009AB08(g_FieldAngleToDirectionLUT[directionIndex]);
}

void func_8009ACEC(void) {
    short rotationValue = PSX_ANGLE(D_800AEA44[SCRIPT_READ_U8_REL(1)] - g_CamInterpolation.targetAngleY) | 0x8000;

    g_FieldScriptVMCurActor->rotation.vx = rotationValue;
    g_FieldScriptVMCurActor->rotation.vy = rotationValue;

    // 3D Actor?
    if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->rotation.vz = rotationValue;
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009AD6C(void) {
    short rotationValue = D_800AEA54[SCRIPT_READ_U8_REL(1)] | 0x8000;

    g_FieldScriptVMCurActor->rotation.vx = rotationValue;
    g_FieldScriptVMCurActor->rotation.vy = rotationValue;

    // 3D Actor?
    if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->rotation.vz = rotationValue;
    }

    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_8009ADDC(void) {
    g_Scene.unk48 |= 0x4000;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009AE0C(void) {
    g_Scene.unk48 &= 0xBFFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009AE3C(s32 start, s32 duration) {
    if (duration != 0) {
        g_Scene.unk48 |= FIELD_SCENE_INTERPOLATING_SCREEN_Z;
        g_Scene.unk88 = -(s32)((g_Scene.sceneScrZ - start) << 16) / duration;
        g_Scene.unk80 = duration;
        g_Scene.unk84 = g_Scene.sceneScrZ << 16;
    } else {
        D_800B21D8[0] += 2;
        g_Scene.sceneScrZ = start;
        g_Scene.unk80 = 0;
    }
    g_Scene.unk48 &= 0xDFFF;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc12", func_8009AEE0);

void func_8009B15C(void) {
    D_800B21CF[0] = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void func_8009B184(void) {
    int i;

    D_800B21CF[0] = 0;
    D_800B21CC[0] = 0;
    D_800B2368 = 0;
    D_800B2364 = 0;
    D_800B2360 = 0;
    D_800B21CE = 0;

    for (i = 0; i < 0x20; i++) {
        func_80081C54(g_PlayerActorIndex);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc12", func_8009B210);

void func_8009B338(void) {
    int i;

    D_800B2368 = 0;
    D_800B2364 = 0;
    D_800B2360 = 0;
    D_800B21CE = 0;

    for (i = 0; i < 0x20; i++) {
        func_80081C54(g_PlayerActorIndex);
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc12", func_8009B398);

void func_8009B664(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_Scene.sceneScrZ);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void func_8009B6AC(void) {
    int arg1 = FieldScriptVMGetArgument(1);
    int arg3 = FieldScriptVMGetArgument(3);

    func_8009AE3C(arg1, arg3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_8009B708(int direction, int duration) {
    int cameraDirection = FieldGetCameraDirection() & 0xFFFF;

    if (duration == 0) {
        D_800B21D8[0] += 2;
        duration = 1;
    }
    g_Scene.unk5C = (D_800AEA64[cameraDirection * 8 + direction] * 0x2000000) / duration;
    g_Scene.unk66 = duration;
    g_Scene.unk7C = ((direction + 4) & 7) << 9;
}

void func_8009B7A8(int direction, int duration) {
    if (duration == 0) {
        D_800B21D8[0] += 2;
        duration = 1;
    }
    if (direction == 0) {
        g_Scene.unk5C = 0x2000000 / duration;
        g_Scene.unk7C += 0x200;
    } else {
        g_Scene.unk5C = -0x2000000 / duration;
        g_Scene.unk7C -= 0x200;
    }
    g_Scene.unk66 = duration;
}

void func_8009B824(void) {
    if (g_Scene.unk66 == 0) {
        func_8009B7A8(0, FieldScriptVMGetArgument(1));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }
    D_800B00C0 = 1;
}

void func_8009B884(void) {
    if (g_Scene.unk66 == 0) {
        func_8009B7A8(0, FieldScriptVMGetArgument(1));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    }
    D_800B00C0 = 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc12", func_8009B8E4);
/* 89.26%: the 2D branch computes `(direction + 4 & 7) << 9` twice in the
   original (the g_FieldScriptVMCurActor load clobbers v1 between the two
   stores); here gcc keeps the shifted value in one register and schedules the
   actor load above both stores.
void func_8009B8E4(void) {
    int direction = FieldScriptVMGetArgument(1);
    int duration = FieldScriptVMGetArgument(3);

    if (D_800ADB1C == 0) {
        s32 angle = (direction + 4 & 7) << 9;

        g_Scene.sceneAngle.vy = angle;
        g_Scene.unk7C = angle;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (g_Scene.unk66 == 0) {
        func_8009B708(direction, duration);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    }
    D_800B00C0 = 1;
} */

void func_8009B9A0(void) {
    if (g_Scene.unk66 == 0) {
        g_Scene.unkB8 = FieldGetCameraDirection();
        g_Scene.unkBC = g_Scene.sceneScrZ;
        g_Scene.unkC0 = g_Scene.sceneDIP;
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
    }
}

void func_8009BA0C(void) {
    if (g_Scene.unk66 == 0) {
        func_8009B708(g_Scene.unkB8, 0x20);
        func_8009AE3C(g_Scene.unkBC, 0x20);
        func_8009A420(g_Scene.unkC0, 0x20);
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc12", func_8009BA7C);
/* 96.94%: same statements and store order; the original reaches unk60 with
   `sll 9` then `sll 16` on the same register, gcc folds them into `sll 25`.
void func_8009BA7C(void) {
    int direction;

    g_Scene.sceneDIP = FieldScriptVMGetArgument(3);
    direction = (FieldScriptVMGetArgument(1) + 4) & 7;
    g_Scene.unk7C = direction << 9;
    g_Scene.sceneAngle.vy = direction << 9;
    g_Scene.unk60 = g_Scene.unk7C << 16;
    g_Scene.sceneScrZ = FieldScriptVMGetArgument(5);
    SetGeomScreen(g_Scene.sceneScrZ);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
} */
