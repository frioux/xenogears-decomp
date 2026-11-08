#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/script_vm.h"
#include "field/text_box.h"

extern s32 D_800ADB1C; // Is current actor a 2D actor (0) maybe?
extern s32 D_800AFD1C;
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
/* 96.22% (1088 bytes, original is 1080). Below reproduces the original
   instruction for instruction except the arrival arm's two rotation stores: gcc
   keeps the temp in a1 and reloads g_FieldScriptVMCurActor for the `sh`s, where
   the original stores straight from v0 and reuses the pointer it already has in
   a2. Spelling those two stores out as duplicated `... | 0x8000` expressions
   reaches 96.43% (1084 bytes, pointer reused) but then gcc stops CSE-ing the
   field load. Block layout notes, all of which are load bearing: cases are
   written 1, 2, 3, then 0/4; the arrival test is the inverted
   `flags_0 == 0 || magnitude + radius >= distance`; the unk11C test is the
   inverted `!(flags & 0x8000)`; and both field reads need the (u_short) cast to
   get `lhu` + `ori` rather than `lh`.
int func_80099AC0(int arg0) {
    FieldActor* pFieldActor;
    ActorData* pActorData;
    SpriteData* pSpriteData;
    ActorData* pTargetActor;
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
    pActorData = pFieldActor[D_800AFD1C].pActorData;
    pSpriteData = pFieldActor[D_800AFD1C].pSpriteData;
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
        x = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)) + g_FieldScriptVMCurActor->unkD0.vx;
        z = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5)) + g_FieldScriptVMCurActor->unkD0.vz;
        break;

    case 2:
        if (FieldScriptVMGetActorIndex(1) == ACTOR_ID_INVALID) {
            return 0;
        }
        pTargetActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        radius = FieldGetVec1Magnitude(pTargetActor->solidRange + g_FieldScriptVMCurActor->solidRange);
        x = CONV_TO_GTE(pTargetActor->position.vx);
        z = CONV_TO_GTE(pTargetActor->position.vz);
        if (SCRIPT_READ_U8_REL(1) == g_PlayerActorIndex) {
            g_FieldScriptVMCurActor->scriptFlags.flags |= 0x200000;
        }
        break;

    case 3:
        angle = FieldScriptVMGetArgument(1) & 0xFFF;
        x = g_FieldScriptVMCurActor->unkD0.vx + ((rsin(angle) * 0x1000) >> 12);
        z = g_FieldScriptVMCurActor->unkD0.vz + (-(rcos(angle) * 0x1000) >> 12);
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
    g_FieldScriptVMCurActor->scriptFlags.flags |= 0x400000;

    if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 == 0
        || magnitude + radius >= distance) {
        if (arg0 != 0) {
            if (!(g_FieldScriptVMCurActor->scriptFlags.flags & 0x8000)) {
                rotation = (u_short)g_FieldScriptVMCurActor->rotation.vy | 0x8000;
            } else {
                rotation = (u_short)g_FieldScriptVMCurActor->unk11C | 0x8000;
            }
            g_FieldScriptVMCurActor->rotation.vx = rotation;
            g_FieldScriptVMCurActor->rotation.vy = rotation;
        } else {
            rotation = func_8007B694(&delta);
            g_FieldScriptVMCurActor->rotation.vx = rotation;
            g_FieldScriptVMCurActor->rotation.vy = rotation;
        }

        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0 = 0xFFFF;
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x17 = 0;
        g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x02200800;
        return 0;
    }

    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0--;
    rotation = func_8007B694(&delta);
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->rotation.vx = rotation;
    g_FieldScriptVMCurActor->rotation.vy = rotation;
    return -1;
} */

void FieldScriptVMWriteCurCharacterID(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_FieldScriptVMCurActor->characterId);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc12", FieldScriptVMWritePartyLeaderCharacterID);
/* 94.84%: identical instructions except that gcc emits `li a0,1` for
   FieldScriptVMGetInstructionArgument early (after the prologue / before the
   g_FieldActors loads) instead of leaving it in that call's jal delay slot.
   Tried: `int nArgument = 1;` local, separating the pActor statement (same
   hoist, pct unchanged), the whole call as one expression (4.19%), and
   `int`/`short characterId` locals - those do put the li in the delay slot but
   turn the `lh a1,228(s0)` after the call into a `move a1,s0` (74%).
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
/* 97.58%: two instructions differ - gcc emits an extra `li a0,1` for the
   pActor GetActorIndex call right after the pTargetActor call instead of
   leaving it in that call's jal delay slot. Same hoist as
   FieldScriptVMWritePartyLeaderCharacterID. Tried: naming the pTargetActor
   index in a local (no change); swapping the two assignments changes the
   s0/s1 allocation so it is not the same code.
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
}
 */
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

typedef struct { s16 value; } FieldScriptGlobalS16;
extern FieldScriptGlobalS16 D_800B2348;

long FieldGetVec1Magnitude(long);
long FieldGetVec2Magnitude(long, long);
int func_8007B694(IVEC3*);
void func_8009E574(int, int);
extern s32 D_800AFD1C;
INCLUDE_ASM("asm/field/nonmatchings/main/misc12", func_8009AEE0);
/* 19.71% (636 vs 632 bytes): the C below is semantically right and matches
   instruction for instruction except for block layout. The original emits the
   shared tail (directional rotation + position snap + flag clear) *before* the
   `magnitude < distance` body, i.e. on the close-enough fall-through path, and
   the far body jumps back to it (`j .L8009AFDC`). gcc only lays blocks out in
   source order, so the source must have the tail written in both arms (an
   `if (magnitude >= distance) { tail } else { far + tail }`); both forms tried
   here moved `magnitude`/`distance` out of s0/v0 into s6/s7 and grew the frame,
   so the register allocation of the original must be reproduced some other way.
   Also still open: `pActor->rotation.vx = rotation` emits an extra `move a0,v0`.
int func_8009AEE0(int actorSlotId, int x, int z, int direction) {
    FieldActor* pFieldActor;
    ActorData* pActor;
    SpriteData* pSpriteData;
    ActorData* pCurActor;
    IVEC3 delta;
    int prevActorIndex;
    int actorIndex;
    int rotation;
    int newRotation;
    int distance;
    int magnitude;

    actorIndex = D_8005A444[actorSlotId];
    if (actorIndex == ACTOR_ID_INVALID) {
        return 0;
    }
    pFieldActor = &g_FieldActors[actorIndex];
    if (pFieldActor->status & ACTOR_STATUS_INVISIBLE) {
        return 0;
    }
    pSpriteData = pFieldActor->pSpriteData;
    pActor = pFieldActor->pActorData;
    if (pSpriteData->moveSpeed == 0) {
        pSpriteData->moveSpeed = 0x4000000 / pActor->moveSpeed;
    }
    magnitude = FieldGetVec1Magnitude(pSpriteData->moveSpeed >> 15) + 1;
    delta.x = x - CONV_TO_GTE(pActor->position.vx);
    delta.y = 0;
    delta.z = z - CONV_TO_GTE(pActor->position.vz);
    distance = FieldGetVec2Magnitude(delta.x, delta.z);
    pActor->scriptFlags.flags |= 0x400000;
    if (magnitude < distance) {
        if (pActor->prevPosition.x == CONV_TO_GTE(pActor->position.vx)
            && pActor->prevPosition.y == CONV_TO_GTE(pActor->position.vy)
            && pActor->prevPosition.z == CONV_TO_GTE(pActor->position.vz)) {
            pActor->unk6E++;
        } else {
            pActor->unk6E = 0;
        }
        rotation = func_8007B694(&delta);
        pActor->rotation.vx = rotation;
        pActor->rotation.vy = rotation;
        if ((short)pActor->unk6E < 0x41 && D_800B2348.value == 0) {
            return -1;
        }
        pCurActor = g_FieldScriptVMCurActor;
        prevActorIndex = D_800AFD1C;
        g_FieldScriptVMCurActor = pActor;
        D_800AFD1C = D_8005A444[actorSlotId];
        func_8009E574(x, z);
        g_FieldScriptVMCurActor = pCurActor;
        D_800AFD1C = prevActorIndex;
    }
    if (pActor->scriptFlags.flags & 0x8000) {
        rotation = pActor->unk11C | 0x8000;
    } else if (direction == 0xFF) {
        rotation = pActor->rotation.vy | 0x8000;
    } else {
        rotation = g_FieldAngleToDirectionLUT[direction] | 0x8000;
    }
    pActor->rotation.vx = rotation;
    pActor->rotation.vy = rotation;
    pActor->position.vx = x << 16;
    pActor->position.vz = z << 16;
    pActor->unk6E = 0;
    pActor->scriptFlags.flags &= ~0x02200800;
    return 0;
} */

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

void func_8009B210(void) {
    ActorData* pActor;
    int x;
    int z;
    int mask;

    pActor = g_FieldActors[D_8005A444[0]].pActorData;
    x = CONV_TO_GTE(pActor->position.vx);
    z = CONV_TO_GTE(pActor->position.vz);
    mask = func_8009AEE0(0, x, z, 0xFF) == 0;
    if (func_8009AEE0(1, x, z, 0xFF) == 0) {
        mask |= 2;
    }
    if (func_8009AEE0(2, x, z, 0xFF) == 0) {
        mask |= 4;
    }
    D_800B00C0 = 1;
    if (mask == 7) {
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
        D_800B21CC[0] = 0;
        D_800B2348.value = 0;
        func_8009B338();
    } else {
        D_800B21CC[0] = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
    }
}

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
/* 95.05%, same instruction count (179). Only the 2D-actor arm differs: gcc
   schedules the `D_800B21CE = 1` store before the scriptInstructionPointer
   update (taking a0, which pushes the D_8005A444 pointer and loop counter into
   a1/a2); the original emits `sh 204(v0)` first and materialises the 1 into v0
   after it. Everything from the else arm onwards matches instruction for
   instruction. Bit idiom `| ~0x7FFF` is the house spelling (see func_8009F4CC
   in misc6.c): `| 0x8000` on the short field gives lh/ori instead of lhu/or.
void func_8009B398(void) {
    ActorData* pActor;
    short rotation;
    int i;
    int mask;

    if (FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD)) == 0x7FFF) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0x14;
        D_800B21CE = 1;

        for (i = 0; i < 3; i++) {
            if (D_8005A444[i] != 0xFF) {
                pActor = g_FieldActors[D_8005A444[i]].pActorData;
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
        D_800B00C0 = 1;
        if (mask == 7) {
            D_800B2348.value = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 0x14;
            D_800B21CC[0] = 0;
        } else {
            D_800B21CC[0] = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer--;
        }
        D_800B21CE = 1;
    }
} */

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
/* 96.94%: store order matches (unk7C, then unk60, then sceneAngle.vy). The
   original computes `angle << 9` twice and does not fold the unk60 chain into a
   single `sll 25`; gcc 2.7.2 CSEs the two shifts and combine folds
   (x<<9)<<16 -> sll 25. Tried: both statement orders, `* 0x200` / `* 0x10000`,
   a u_long temp for unk7C - all fold. */
/*
void func_8009BA7C(void) {
    int direction;

    g_Scene.sceneDIP = FieldScriptVMGetArgument(3);
    direction = (FieldScriptVMGetArgument(1) + 4) & 7;
    g_Scene.unk7C = direction * 0x200;
    g_Scene.sceneAngle.vy = direction << 9;
    g_Scene.unk60 = g_Scene.unk7C * 0x10000;
    g_Scene.sceneScrZ = FieldScriptVMGetArgument(5);
    SetGeomScreen(g_Scene.sceneScrZ);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}
*/

