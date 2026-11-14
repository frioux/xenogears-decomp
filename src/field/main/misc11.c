#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/actor.h"
#include "field/script_vm.h"
#include "field/text_box.h"
#include "field/particles.h"
#include "system/graphics.h"
#include "system/memory.h"

extern s32 D_800AFD1C;
extern s32 g_PlayerActorIndex;

extern u8 D_800B2190;
extern u8 D_800B2191;
extern u8 D_800B2192;
extern u8 D_800B2194;
extern u8 D_800B2195;
extern u8 D_800B2196;
extern s16 D_800B2198;
extern s16 D_800B219A;
extern s16 D_800B218E;

void func_80073E38(void);

void FieldScriptVMHandlerSetFogColor(void) {
    D_800B2190 = FieldScriptVMGetArgument(1);
    D_800B2191 = FieldScriptVMGetArgument(3);
    D_800B2192 = FieldScriptVMGetArgument(5);
    D_800B2194 = FieldScriptVMGetArgument(7);
    D_800B2195 = FieldScriptVMGetArgument(9);
    D_800B2196 = FieldScriptVMGetArgument(0xB);
    D_800B2198 = FieldScriptVMGetArgument(0xD);
    D_800B219A = FieldScriptVMGetArgument(0xF);
    D_800B218E = 1;
    func_80073E38();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x11;
}

typedef struct { u8 value; } FieldScriptGlobalU8;

extern FieldScriptGlobalU8 D_800B219C;
extern FieldScriptGlobalU8 D_800B219D;
extern FieldScriptGlobalU8 D_800B219E;

void FieldScriptVMHandlerSetScrollLimits(void) {
    g_Scene.unk4C[0] = FieldScriptVMGetInstructionArgumentS16(1);
    g_Scene.unk4C[1] = FieldScriptVMGetInstructionArgumentS16(3);
    g_Scene.unk4C[2] = FieldScriptVMGetInstructionArgumentS16(5);
    g_Scene.unk4C[3] = -FieldScriptVMGetInstructionArgumentS16(7);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptVMHandlerSetClearColor(void) {
    D_800B219C.value = FieldScriptVMGetArgument(1);
    D_800B219D.value = FieldScriptVMGetArgument(3);
    D_800B219E.value = FieldScriptVMGetArgument(5);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void FieldScriptVMHandlerEmpty(void) {
}

extern RECT D_800AF5E8[32];
extern s16 D_800AF5EA[]; // Part of D_800AF5E8[]: the y of each RECT
extern s16 D_800AF5EC[]; // Part of D_800AF5E8[]: the w of each RECT
extern s16 D_800AF5EE[]; // Part of D_800AF5E8[]: the h of each RECT
extern s16 D_800AF6E8[32];
extern s16 D_800AF728[32];
extern u_short D_800AF768;

void FieldMoveOrClearImageRect(int x, int y, int w, int h, int texX, int texY, int clear) {
    int index = D_800AF768 & 0x1F;

    D_800AF5E8[index].x = x;
    D_800AF5EA[index * 4] = y;
    D_800AF5EC[index * 4] = w;
    D_800AF5EE[index * 4] = h;
    D_800AF6E8[index] = texX;
    D_800AF728[index] = texY;
    if (!clear) {
        MoveImage(&D_800AF5E8[index], D_800AF6E8[index], (s16)texY);
    } else {
        ClearImage(&D_800AF5E8[index], 0, 0, 0);
    }
    D_800AF768++;
}

void FieldScriptVMHandlerDrawImage(void) {
    RECT rect; // unused, but the original frame has this 8-byte slot
    int x;
    int y;

    x = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    y = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    if (x == 0 && y == 0) {
        FieldMoveOrClearImageRect(
            FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD)),
            FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD)),
            FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD)),
            FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD)),
            0, 0, 1);
    } else {
        FieldMoveOrClearImageRect(x, y,
            FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD)),
            FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD)),
            FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD)),
            FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD)),
            0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptVMHandlerSetActorFlags134(void) {
    int value;

    value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
    g_FieldScriptVMCurActor->flags134 = (g_FieldScriptVMCurActor->flags134 & 0xFFFFFF9F) | ((value & 3) << 5);
    g_FieldScriptVMCurActor->unkEE = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerSetActorFlags134ByIndex(void) {
    ActorData* pActor;
    int value;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        value = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(6));
        pActor->flags134 = (pActor->flags134 & 0xFFFFFF9F) | ((value & 3) << 5);
        pActor->unkEE = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(6));
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void FieldScriptVMHandlerSetActorDataWord(void) {
    int index;
    int value;

    index = FieldScriptVMGetArgument(1);
    value = FieldScriptVMGetArgument(3);
    if (value >= 0x1000) {
        value = 0xFFF;
    }

    if (g_FieldActors[D_800AFD1C].status & 0x2000) {
        ((u_int*)g_FieldScriptVMCurActor->unk118)[index] = value;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerSwapVariables(void) {
    int value1 = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF);
    int value2 = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(3) & 0xFFFF);

    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, value1);
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, value2);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void GfxLineScrollUpdate(LineScroll*);

typedef struct {
    /* 0x0   */ s16 numLineScrolls;
    /* 0x2   */ s16 unk2;
    /* 0x4   */ LineScroll* pLineScrolls[32];
    /* 0x84  */ u8* pDatas[32];
    /* 0x104 */ s16 lengths[32];
} LineScrollTable; // Size: 0x144

extern LineScrollTable D_800AFEA8;

void FieldUpdateLineScrolls(void) {
    int i;

    for (i = 0; i < D_800AFEA8.numLineScrolls; i++) {
        GfxLineScrollUpdate(D_800AFEA8.pLineScrolls[i]);
    }
}

// D_800AFFAC holds a byte length per line scroll; the 0x20 data pointers it
// describes start 0x80 bytes earlier, so index them off the same symbol.
extern s16 D_800AFFAC[];

void FieldScriptVMHandlerWriteLineScrollByte(void) {
    int index;
    int offset;
    int value;

    index = FieldScriptVMGetArgument(1);
    offset = FieldScriptVMGetArgument(3);
    value = FieldScriptVMGetInstructionArgument(5) & 0xFFFF;

    if (offset < D_800AFFAC[index]) {
        ((u_char**)D_800AFFAC)[index - 0x20][offset] = value;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern s32 D_800ADB8C;

void GfxLineScrollInitialize(LineScroll*, s16, s16, s16, s16, s16, s16, s16, u8*);

void FieldScriptVMHandlerCreateLineScroll(void) {
    int count;
    int value;
    int arg1;
    int arg3;
    int arg5;
    int arg7;
    int argB;
    int i;

    if (D_800ADB8C == 0 && D_800AFEA8.numLineScrolls < 0x20) {
        count = FieldScriptVMGetInstructionArgument(9) & 0xFFFF;
        D_800AFEA8.lengths[D_800AFEA8.numLineScrolls] = count;
        D_800AFEA8.pDatas[D_800AFEA8.numLineScrolls] = HeapAlloc(count + 1, 0);
        D_800AFEA8.pLineScrolls[D_800AFEA8.numLineScrolls] = HeapAlloc(sizeof(LineScroll), 0);
        value = FieldScriptVMGetInstructionArgument(0xF) & 0xFFFF;
        for (i = 0; i < count; i++) {
            D_800AFEA8.pDatas[D_800AFEA8.numLineScrolls][i] = value;
        }
        arg1 = (s16)FieldScriptVMGetInstructionArgument(1);
        arg3 = (s16)FieldScriptVMGetInstructionArgument(3);
        arg5 = (s16)FieldScriptVMGetInstructionArgument(5);
        arg7 = (s16)FieldScriptVMGetInstructionArgument(7);
        argB = (s16)FieldScriptVMGetInstructionArgument(0xB);
        GfxLineScrollInitialize(D_800AFEA8.pLineScrolls[D_800AFEA8.numLineScrolls], arg1, arg3, arg5, arg7, count, argB, (s16)FieldScriptVMGetInstructionArgument(0xD),
                                D_800AFEA8.pDatas[D_800AFEA8.numLineScrolls]);
        D_800AFEA8.numLineScrolls++;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x11;
}

int FieldScriptVMAdvanceIpReturn(void) {
    int ip = g_FieldScriptVMCurActor->scriptInstructionPointer + 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer = ip;
    return ip;
}

int FieldScriptVMAdvanceIpReturnAlt(void) {
    int ip = g_FieldScriptVMCurActor->scriptInstructionPointer + 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer = ip;
    return ip;
}

extern u_int* D_800AFB20;

void func_800924D4(int, int, int);
void FieldResetCurrentActorMovement(void);

// Read one byte of the 4-byte record at the given index of D_800AFB20
int FieldScriptGetRecordByte(int index, int byteIndex) {
    switch (byteIndex) {
        case 0:
            return ((u_char*)&D_800AFB20[index])[0];
        case 1:
            return ((u_char*)&D_800AFB20[index])[1];
        case 2:
            return ((u_char*)&D_800AFB20[index])[2];
        case 3:
            return ((u_char*)&D_800AFB20[index])[3];
    }
    return 0;
}

// Write one byte of the 4-byte record at the given index of D_800AFB20
INCLUDE_ASM("asm/field/nonmatchings/main/misc11", func_800924D4);

/* Best attempt: 69.47%. Same instruction count and the correct masks
   (case 1's mask really is 0xFFFF00FF; splat's comment is right), but gcc keeps
   the record offset in a0 instead of the original's v1 and merges the tail of
   case 0 with cases 1-3, which the original does not.
void func_800924D4(int index, int byteIndex, int value) {
    switch (byteIndex) {
        case 0:
            D_800AFB20[index] = (D_800AFB20[index] & ~0xFF) | value;
            break;
        case 1:
            D_800AFB20[index] = (value << 8) | (D_800AFB20[index] & ~0xFF00);
            break;
        case 2:
            D_800AFB20[index] = (value << 16) | (D_800AFB20[index] & ~0xFF0000);
            break;
        case 3:
            D_800AFB20[index] = (value << 24) | (D_800AFB20[index] & ~0xFF000000);
            break;
    }
}
*/

extern s32 D_800B217C;
extern s16 D_800B21D6;

void func_800925A0(void) {
    int arg = FieldScriptVMGetArgument(1);

    D_800B217C = arg;
    switch (arg) {
        case 0:
            D_800B21D6 = 8;
            break;
        case 1:
            D_800B21D6 = 6;
            break;
        case 2:
            D_800B21D6 = 4;
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerSetControllerBtnMask(void) {
    g_FieldControl.controllerBtnMask = FieldScriptVMGetInstructionArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerSetRecordByte(void) {
    func_800924D4(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2), FieldScriptVMGetArgument(3));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerOrRecordByte(void) {
    int value;

    value = FieldScriptGetRecordByte(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2));
    value |= FieldScriptVMGetArgument(3);
    func_800924D4(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2), value);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerAndRecordByte(void) {
    int value;

    value = FieldScriptGetRecordByte(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2));
    value &= FieldScriptVMGetArgument(3);
    func_800924D4(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2), value);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern FieldActor* D_800B06B8;
extern s16 D_800C3A6A;
extern int rcos(int);
extern int rsin(int);

void FieldScriptVMHandlerSetDirectionFromRotation(void) {
    g_FieldScriptVMCurActor->unk60.vx = ((u_int)rsin(D_800B06B8->rotation.y) * 9) >> 10;
    g_FieldScriptVMCurActor->unk60.vz = -(rcos(D_800B06B8->rotation.y) * 36) >> 12;
    g_FieldScriptVMCurActor->flags |= 0x800;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc11", func_80092894);

extern u_int D_800B2350[];

extern s32 D_800ADBDC;
extern s32 D_800ADBE4;
extern s32 D_800ADB2C;
extern s32 D_800ADB90;
extern s32 D_8004F308;

int func_80092894(int, int, int, int);
void FieldPlayerSetScriptFlag80(void);

void FieldScriptVMHandlerMapTransitionWithArgs(void) {
    int arg1;
    int arg3;

    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        return;
    }

    arg1 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
    arg3 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
    if (D_800B2350[0] == 0) {
        D_800B2350[0] = g_FieldActors[g_PlayerActorIndex].pActorData->scriptFlags.flags;
    }
    FieldPlayerSetScriptFlag80();
    if (func_80092894(0, 1, arg1, arg3)) {
        return;
    }
    if (!(D_800B2350[0] & 0x80)) {
        g_FieldActors[g_PlayerActorIndex].pActorData->scriptFlags.flags &= ~0x80;
    }
    D_800B2350[0] = 0;
}

void FieldScriptVMHandlerMapTransition(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
    } else {
        FieldPlayerSetScriptFlag80();
        func_80092894(0, 0, 0, 0);
    }
}

void FieldScriptVMHandlerMapTransitionOffset(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
    } else {
        FieldPlayerSetScriptFlag80();
        func_80092894(0x3E0, 0, 0, 0);
    }
}

extern int g_GameSceneMapNum;
extern s32 D_800ADBD8;
extern s32 D_800B0064;

int FieldGetPlayerActorDirection(void);
int FieldGetCameraDirection(void);

void FieldScriptVMSaveSceneState(void) {
    FieldScriptMemoryWriteU16(4, g_GameSceneMapNum & 0x3FFF);
    FieldScriptMemoryWriteU16(6, FieldGetPlayerActorDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(8, FieldGetCameraDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(0x12, (short)(FieldScriptVMGetVariableValue(0x12) + 1));
}

void FieldScriptVMHandlerRestoreRandomEncounters(void) {
    if (D_800ADBD8 != 0) {
        g_FieldControl.isRandomEncountersEnabled = -1;
        D_800ADBD8 = 0;
        D_800B0064 = FieldScriptVMGetArgument(1);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern u8 D_800B02C8;

void FieldScriptVMHandlerSetFieldEntryState(void) {
    int value;

    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
        return;
    }

    func_800A31E8();
    g_FieldControl.isRandomEncountersEnabled = -1;
    D_800ADBE4 = 0;
    g_pGameState->unk231A = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    g_pGameState->unk231E = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    value = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(9));
    if (((u_short)value == 0xFFFF) | (value == -1)) {
        g_pGameState->unk231C = (g_Scene.sceneAngle.vy + 0x800) & 0xFFF;
    } else {
        g_pGameState->unk231C = (value + 0x800) & 0xFFF;
    }
    g_pGameState->unk2320 = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(9));
    FieldScriptVMEmptyHook();
    D_800B02C8 = 1;
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

void FieldScriptVMEmptyHook(void) {
}

extern s32 D_800B0048;
extern s32 D_800AFD14;

void FieldScriptVMHandlerSetFadeParams(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
        return;
    }

    FieldScriptVMHandlerRestoreMap();
    FieldScriptVMEmptyHook();
    D_800B0048 = FieldScriptVMGetArgument(0);
    D_800AFD14 = FieldScriptVMGetArgument(2);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

extern s32 D_800ADB70;
extern s32 D_800ADBEC;

void FieldScriptVMHandlerRestoreMap(void) {
    int a;
    int b;

    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0 || D_800ADB70 != 0) {
        D_800B00C0 = 1;
        return;
    }

    g_FieldControl.isRandomEncountersEnabled = -1;
    if (D_800ADBEC != 0) {
        a = FieldScriptVMGetArgument(3);
        b = FieldScriptVMGetArgument(1);
        FieldScriptVMSaveSceneState();
        D_800ADBEC = 0;
        FieldScriptMemoryWriteU16(2, a);
        g_GameSceneMapNum = b;
        FieldScriptVMEmptyHook();
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern u8 D_800B2356;
extern u8 D_8005954C;
extern u8 D_80059508;
extern u8 D_800594F8;
extern s32 D_800ADBE0;
extern s32 D_800ADB88;
extern s32 D_800ADB18;

void FieldScriptVMHandlerChangeMap(void) {
    int mapNum;

    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADBEC == 0 || D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        return;
    }

    D_8005954C = D_800B2356;
    D_80059508 = FieldScriptVMGetArgument(1);
    D_800594F8 = 0;
    D_800ADBDC = 0;
    D_800ADBE0 = 0;
    D_800ADB88 = 1;
    mapNum = FieldScriptVMGetArgument(5);
    if (mapNum != 0x7FFF) {
        int value = FieldScriptVMGetArgument(7);

        FieldScriptVMSaveSceneState();
        FieldScriptMemoryWriteU16(2, value);
        g_GameSceneMapNum = mapNum;
        D_800ADB18 = 1;
    }
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptVMHandlerReloadMap(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0 || D_800ADBEC == 0 || D_800ADB2C != 0 || D_8004F308 == -1 || D_800ADB90 != 0) {
        D_800B00C0 = 1;
        return;
    }

    D_8005954C = D_800B2356;
    D_80059508 = FieldScriptVMGetArgument(1);
    D_800594F8 = 0;
    D_800ADBDC = 0;
    D_800ADBE0 = 0;
    D_800ADB88 = 1;
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_8004F350;
extern s8 D_80059171;
extern s32 D_800ADB64;
extern u_short D_800B236C;

void FieldScriptVMHandlerCopyRecordByte(void) {
    int value = FieldScriptGetRecordByte(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2));

    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, value);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerWaitForFieldRequest(void) {
    if (D_8004F350 == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

void func_80093740(void) {
    D_800B00C0 = 1;
    D_800ADB64 = 0;
    D_80059171 = D_800B236C;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptVMHandlerRequestFieldState6(void) {
    D_80059171 = 1;
    D_800ADB64 = 6;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptVMHandlerRequestFieldState2(void) {
    D_800ADB64 = 2;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptVMHandlerRequestFieldState3(void) {
    D_80059171 = FieldScriptVMGetArgument(1);
    D_800ADB64 = 0x3;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerRequestFieldState1WithMap(void) {
    int a;
    int b;

    g_FieldControl.isRandomEncountersEnabled = -1;
    a = FieldScriptVMGetArgument(3);
    b = FieldScriptVMGetArgument(1);
    FieldScriptVMSaveSceneState();
    FieldScriptMemoryWriteU16(2, a);
    g_GameSceneMapNum = b;
    FieldScriptVMEmptyHook();
    D_800ADB64 = 1;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerRequestFieldState1WithValue(void) {
    ActorData* pActor;
    int value;

    value = FieldScriptVMGetArgument(1);
    D_800ADB64 = 1;
    D_800B00C0 = 1;
    pActor = g_FieldScriptVMCurActor;
    *(s16*)&g_pGameState->unk1928[0xA] = value;
    *(s16*)((u8*)g_pGameState + 0x2320) = value;
    D_800C3A6A = value;
    D_8004F350++;
    pActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerRequestFieldState4(void) {
    D_80059171 = FieldScriptVMGetArgument(1);
    D_800ADB64 = 0x4;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerRequestFieldState5(void) {
    D_80059171 = FieldScriptVMGetArgument(1);
    D_800ADB64 = 0x5;
    D_800B00C0 = 1;
    D_8004F350++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}


// Random encounter stuff

// These are likely part of a struct
extern s8 D_800B21D0[];
extern s8 D_800B21D1[];

extern s32 D_800ADBDC;
extern s32 D_800ADBE4;
extern s32 D_800B00C0;

void FieldScriptVMClearSceneEncounterFlag(void) {
    g_Scene.unk48 &= 0x7FFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMSetSceneEncounterFlag(void) {
    g_Scene.unk48 |= 0x8000;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMClearRandomEncounterFlags(void) {
    g_FieldControl.isRandomEncountersEnabled = 0;
    D_800B21D0[0] = 0;
    D_800B21D1[0] = 0;
    g_Scene.unk48 &= 0x3FFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMSetRandomEncounterFlags(void) {
    g_FieldControl.isRandomEncountersEnabled = -1;
    D_800B21D0[0] = 1;
    D_800B21D1[0] = 1;
    g_Scene.unk48 |= 0xC000;
    if ((D_800ADBDC == 0) || (D_800ADBE4 == 0)) {
        D_800B00C0 = 1; // Stop script VM execution?
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMClearFlag21D0(void) {
    D_800B21D0[0] = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMSetFlag21D0(void) {
    D_800B21D0[0] = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptEnableCompass(void) {
    D_800B21D1[0] = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptDisableCompass(void) {
    D_800B21D1[0] = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerDisableRandomEncounters(void) {
    g_FieldControl.isRandomEncountersEnabled = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerEnableRandomEncounters(void) {
    if (D_800ADBDC == 0 || D_800ADBE4 == 0) {
        D_800B00C0 = 1;
        return;
    }
    g_FieldControl.isRandomEncountersEnabled = -1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
// End of random encounter stuff


// Write U8 Handler
// Arg1 + Arg3 = Offset to U8 value in script
// Arg2 = Write address
void FieldScriptVMHandlerCopyScriptByte(void) {
    unsigned short offset;
    unsigned short arg1;

    arg1 = FieldScriptVMGetInstructionArgument(1);
    offset = arg1 + FieldScriptVMGetArgument(5);
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(2), 
        SCRIPT_READ_U8(offset)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

// Write U16/S16 handler
// Arg1 + Arg3 = Offset in script to short to write
// Arg3: Write address
void FieldScriptVMHandlerCopyScriptShort(void) {
    int arg1;
    int nAddress;
    unsigned short nOffset;
    int nValue;
    int arg3;

    arg1 = FieldScriptVMGetInstructionArgument(1);
    nOffset = arg1 + FieldScriptVMGetArgument(5);
    if (SCRIPT_READ_U8_REL(7) == 0) {
        // W/O carry
        FieldScriptMemoryWriteU16(
            SCRIPT_IMM_ARG(2),
            SCRIPT_READ_U8(nOffset) | (SCRIPT_READ_U8(nOffset + 1) << 8)
        );
    } else {
        // With carry
        FieldScriptMemoryWriteU16(
            SCRIPT_IMM_ARG(2),
            (short)(SCRIPT_READ_U8(nOffset) + (SCRIPT_READ_U8(nOffset + 1) << 8))
        );
    }
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

// The two functions seems to be related to handling room transitions
void FieldScriptVMHandlerStepActorRotationOn(void) {
    if (!g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13) {
        if (!(g_FieldScriptVMCurActor->flags12C_0x5)) {
            g_FieldScriptVMCurActor->flags12C_0x5 = 1;
            g_FieldScriptVMCurActor->curDoorStep = 0;
            FieldSetSoundSlotElement(8, 3);
        } else {
            g_FieldScriptVMCurActor->curDoorStep++;
            if (g_FieldScriptVMCurActor->curDoorStep < 0x1F) {
                if (SCRIPT_READ_U8_REL(1) == 0) {
                    g_FieldActors[D_800AFD1C].rotation.y += 0x20;
                } else {
                    g_FieldActors[D_800AFD1C].rotation.y -= 0x20;
                }
            } else {
                g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13 = 0x1;
                g_FieldScriptVMCurActor->flags12C_0x5 = 0;
                g_FieldScriptVMCurActor->curDoorStep = 0;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            }
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
    
    func_80072254(D_800AFD1C);
}

void FieldScriptVMHandlerStepActorRotationOff(void) {
    if (g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13) {
        if (!(g_FieldScriptVMCurActor->flags12C_0x5)) {
            g_FieldScriptVMCurActor->flags12C_0x5 = 1;
            g_FieldScriptVMCurActor->curDoorStep = 0;
            FieldSetSoundSlotElement(8, 3);
        } else {
            g_FieldScriptVMCurActor->curDoorStep++;
            if (g_FieldScriptVMCurActor->curDoorStep < 0x1F) {
                if (SCRIPT_READ_U8_REL(1) == 0) {
                    g_FieldActors[D_800AFD1C].rotation.y -= 0x20;
                } else {
                    g_FieldActors[D_800AFD1C].rotation.y += 0x20;
                }
            } else {
                g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13 = 0;
                g_FieldScriptVMCurActor->flags12C_0x5 = 0;
                g_FieldScriptVMCurActor->curDoorStep = 0;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            }
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
    func_80072254(D_800AFD1C);
}

extern void func_80072254(int);
extern void FieldSetSoundSlotElement(int,int);
extern s32 D_800AFD1C;
extern FieldActor* D_800B06B8;

void FieldScriptVMHandlerStepActorMoveOn() {
    ActorData* pActor;
    int angle;
    int delta;

    if (!g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13) {
        if (!g_FieldScriptVMCurActor->flags12C_0x5) {
            g_FieldScriptVMCurActor->flags12C_0x5 = 0x1;
            g_FieldScriptVMCurActor->curDoorStep = 0;
            FieldSetSoundSlotElement(8, 3);
            g_FieldScriptVMCurActor->unkD0.vx = g_FieldScriptVMCurActor->position.vx;
            g_FieldScriptVMCurActor->unkD0.vy = g_FieldScriptVMCurActor->position.vy;
            g_FieldScriptVMCurActor->unkD0.vz = g_FieldScriptVMCurActor->position.vz;
        } else {
            g_FieldScriptVMCurActor->curDoorStep++;
            if (g_FieldScriptVMCurActor->curDoorStep < FieldScriptVMGetArgument(3)) {
                switch (FieldScriptVMGetArgument(5)) { 
                    case 0x1000:
                        g_FieldScriptVMCurActor->unkD0.vy -= FieldScriptVMGetArgument(1) * 0x10;
                        D_800B06B8->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                    case 0x1001:
                        g_FieldScriptVMCurActor->unkD0.vy += FieldScriptVMGetArgument(1) * 0x10;
                        D_800B06B8->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                    default:
                        delta = PSX_DEGREES(90);
                        angle = D_800B06B8->rotation.y + FieldScriptVMGetArgument(5) - delta;
                        g_FieldScriptVMCurActor->unkD0.vx += rsin(angle) * FieldScriptVMGetArgument(1);
                        g_FieldScriptVMCurActor->unkD0.vz -= rcos(angle) * FieldScriptVMGetArgument(1);
                        D_800B06B8->transformMatrix.t[0] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vx);
                        D_800B06B8->transformMatrix.t[2] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                }
            } else {
                g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13 = 1;
                g_FieldScriptVMCurActor->flags12C_0x5 = 0;
                g_FieldScriptVMCurActor->curDoorStep = 0;   
                g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
            }
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
    }
    func_80072254(D_800AFD1C);
}

void FieldScriptVMHandlerStepActorMoveOff(void) {
    ActorData* temp_a1;
    int angle;
    int delta;
    
    if (g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13) {
        if (!g_FieldScriptVMCurActor->flags12C_0x5) {
            g_FieldScriptVMCurActor->flags12C_0x5 = 0x1;
            g_FieldScriptVMCurActor->curDoorStep = 0x0;
            FieldSetSoundSlotElement(8, 3);
        } else {
            g_FieldScriptVMCurActor->curDoorStep++;
            if (g_FieldScriptVMCurActor->curDoorStep < FieldScriptVMGetArgument(3)) {
                switch (FieldScriptVMGetArgument(5)) {
                    case 0x1000:
                        g_FieldScriptVMCurActor->unkD0.vy -= FieldScriptVMGetArgument(1) * 0x10;
                        D_800B06B8->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                    case 0x1001:
                        g_FieldScriptVMCurActor->unkD0.vy += FieldScriptVMGetArgument(1) * 0x10;
                        D_800B06B8->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                    default:
                        delta = PSX_DEGREES(90);
                        angle = D_800B06B8->rotation.y + FieldScriptVMGetArgument(5) - delta;
                        g_FieldScriptVMCurActor->unkD0.vx -= rsin(angle) * FieldScriptVMGetArgument(1);
                        g_FieldScriptVMCurActor->unkD0.vz += rcos(angle) * FieldScriptVMGetArgument(1);
                        D_800B06B8->transformMatrix.t[0] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vx);
                        D_800B06B8->transformMatrix.t[2] = CONV_TO_GTE(g_FieldScriptVMCurActor->unkD0.vz);
                        break;
                }
            } else {
                g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13 = 0x0;
                g_FieldScriptVMCurActor->flags12C_0x5 = 0x0;
                g_FieldScriptVMCurActor->curDoorStep = 0;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
            }
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
    }
    func_80072254(D_800AFD1C);
}

extern s32 D_8004F318;
extern s32 D_8004F328;

void FieldScriptVMHandlerSetWalkPosition(void) {
    D_8004F318 = 0;
    D_8004F328 = 0xFF;
    FieldScriptMemoryWriteU16(
        0xA, 
        ((FieldScriptVMGetArgument(1) << 8) & 0xFF00) | (FieldScriptVMGetArgument(3) & 0xFF)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerSetWalkFlags(void) {
    D_8004F328 = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerStopWalking(void) {
    D_8004F318 = 0;
    D_8004F328 = 0xFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerSetActorHideMode1(void) {
    g_FieldScriptVMCurActor->flags12C_0 = 0x1;
    g_FieldScriptVMCurActor->unk70 = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerSetActorHideMode2(void) {
    g_FieldScriptVMCurActor->flags12C_0 = 0x2;
    g_FieldScriptVMCurActor->unk70 = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerSetActorHideMode3(void) {
    g_FieldScriptVMCurActor->flags12C_0 = 0x3;
    g_FieldScriptVMCurActor->unk70 = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void func_80072254(int);

// Set/Increase/Decrease the X, Y or Z rotation of the actor named by the instruction
void FieldScriptVMHandlerRotateActor(void) {
    if (FieldScriptVMGetActorIndex(2) != ACTOR_ID_INVALID) {
        FieldActor* pActor = &g_FieldActors[FieldScriptVMGetActorIndex(2)];

        switch (SCRIPT_READ_U8_REL(1)) {
            case 0:
                pActor->rotation.x += FieldScriptVMGetArgument(3);
                break;
            case 1:
                pActor->rotation.x -= FieldScriptVMGetArgument(3);
                break;
            case 2:
                pActor->rotation.y += FieldScriptVMGetArgument(3);
                break;
            case 3:
                pActor->rotation.y -= FieldScriptVMGetArgument(3);
                break;
            case 4:
                pActor->rotation.z += FieldScriptVMGetArgument(3);
                break;
            case 5:
                pActor->rotation.z -= FieldScriptVMGetArgument(3);
                break;
        }
        func_80072254(FieldScriptVMGetActorIndex(2));
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

// Set X, Y or Z rotaiton of Actor D_800AFD1C
void FieldScriptVMHandlerSetCurrentActorRotation(void) {
    switch (SCRIPT_READ_U8_REL(3)) { 
        case 0:
            g_FieldActors[D_800AFD1C].rotation.x = FieldScriptVMGetArgument(1);
            break;
        case 1:
            g_FieldActors[D_800AFD1C].rotation.y = FieldScriptVMGetArgument(1);
            break;
        case 2:
            g_FieldActors[D_800AFD1C].rotation.z = FieldScriptVMGetArgument(1);
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    func_80072254(D_800AFD1C);
}

// Increase X Rotation of Actor D_800AFD1C
void func_80094A5C(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.x += FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Decrease X Rotation of Actor D_800AFD1C
void FieldScriptVMHandlerSubCurrentActorRotX(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.x -= FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Increase Y Rotation of Actor D_800AFD1C
void FieldScriptVMHandlerAddCurrentActorRotY(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.y += FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Decrease Y Rotation of Actor D_800AFD1C
void FieldScriptVMHandlerSubCurrentActorRotY(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.y -= FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Increase Z Rotation of Actor D_800AFD1C
void FieldScriptVMHandlerAddCurrentActorRotZ(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.z += FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Decrease Z Rotation of Actor D_800AFD1C
void FieldScriptVMHandlerSubCurrentActorRotZ(void) {
    (&g_FieldActors[D_800AFD1C])->rotation.z -= FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    func_80072254(D_800AFD1C);
}

// Find the first inventory item slot that is unused
s32 FieldInventoryFindEmptyItemSlot(void) {
    int i;

    for (i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        if (g_pGameState->itemQuantities[i] == 0 || g_pGameState->itemIDs[i] == 0) {
            return i;
        }
    }
    return -1;
}

// Find the first weapon slot that is unused
s32 FieldInventoryFindEmptyWeaponSlot(void) {
    int i;

    for (i = 0; i < MAX_INVENTORY_WEAPONS; i++) {
        if (g_pGameState->weaponQuantities[i] == 0 || g_pGameState->weaponIDs[i] == 0) {
            return i;
        }
    }
    return -1;
}

// Find the first accessory slot that is unused
s32 FieldInventoryFindEmptyAccessorySlot(void) {
    int i;

    for (i = 0; i < MAX_INVENTORY_ACCESSORIES; i++) {
        if (g_pGameState->accessoryQuantities[i] == 0 || g_pGameState->accessoryIDs[i] == 0) {
            return i;
        }
    }
    return -1;
}

// Find the first gear weapon slot that is unused
s32 FieldInventoryFindEmptyGearWeaponSlot(void) {
    int i;

    for (i = 0; i < MAX_INVENTORY_WEAPONS; i++) {
        if (g_pGameState->gearWeaponsQty[i] == 0 || g_pGameState->gearWeaponsIds[i] == 0) {
            return i;
        }
    }
    return -1;
}

// Find the first gear parts slot that is unused
s32 FieldInventoryFindEmptyGearPartsSlot(void) {
    int i;

    for (i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        if (g_pGameState->gearPartsQty[i] == 0 || g_pGameState->gearPartsIds[i] == 0) {
            return i;
        }
    }
    return -1;
}

// Find the inventory item slot holding the given item
s32 FieldInventoryFindItemSlot(int itemId) {
    int i;

    for (i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        if (g_pGameState->itemIDs[i] == itemId && g_pGameState->itemQuantities[i] != 0) {
            return i;
        }
    }
    return -1;
}

// Find the accessory slot holding the given accessory
s32 FieldInventoryFindAccessorySlot(int accessoryId) {
    int i;

    for (i = 0; i < MAX_INVENTORY_ACCESSORIES; i++) {
        if (g_pGameState->accessoryIDs[i] == accessoryId && g_pGameState->accessoryQuantities[i] != 0) {
            return i;
        }
    }
    return -1;
}

// Find the weapon slot holding the given weapon
s32 FieldInventoryFindWeaponSlot(int weaponId) {
    int i;

    for (i = 0; i < MAX_INVENTORY_WEAPONS; i++) {
        if (g_pGameState->weaponIDs[i] == weaponId && g_pGameState->weaponQuantities[i] != 0) {
            return i;
        }
    }
    return -1;
}

// Find the gear weapon slot holding the given gear weapon
s32 FieldInventoryFindGearWeaponSlot(int gearWeaponId) {
    int i;

    for (i = 0; i < MAX_INVENTORY_WEAPONS; i++) {
        if (g_pGameState->gearWeaponsIds[i] == gearWeaponId && g_pGameState->gearWeaponsQty[i] != 0) {
            return i;
        }
    }
    return -1;
}

// Find the gear parts slot holding the given gear part
s32 FieldInventoryFindGearPartsSlot(int gearPartId) {
    int i;

    for (i = 0; i < MAX_INVENTORY_ITEMS; i++) {
        if (g_pGameState->gearPartsIds[i] == gearPartId && g_pGameState->gearPartsQty[i] != 0) {
            return i;
        }
    }
    return -1;
}

// Inventory slot kind is encoded in the high byte of the script argument
// Get the array of inventory entry IDs for the given inventory kind
u_char* FieldInventoryGetIdsArray(int itemId) {
    switch (itemId >> 8) {
        case 0:
            return g_pGameState->itemIDs;
        case 1:
            return g_pGameState->weaponIDs;
        case 2:
            return g_pGameState->accessoryIDs;
        case 3:
            return g_pGameState->gearWeaponsIds;
        case 4:
            return g_pGameState->gearPartsIds;
    }
    return 0;
}

// Get the array of inventory entry quantities for the given inventory kind
u_char* FieldInventoryGetQuantitiesArray(int itemId) {
    switch (itemId >> 8) {
        case 0:
            return g_pGameState->itemQuantities;
        case 1:
            return g_pGameState->weaponQuantities;
        case 2:
            return g_pGameState->accessoryQuantities;
        case 3:
            return g_pGameState->gearWeaponsQty;
        case 4:
            return g_pGameState->gearPartsQty;
    }
    return 0;
}

// Find the inventory slot of the given kind holding the given ID
s32 func_80095124(s32 itemId) {
    switch (itemId >> 8) {
        case 0:
            return FieldInventoryFindItemSlot(itemId);
        case 1:
            return FieldInventoryFindWeaponSlot(itemId - 0x100);
        case 2:
            return FieldInventoryFindAccessorySlot(itemId - 0x200);
        case 3:
            return FieldInventoryFindGearWeaponSlot(itemId - 0x300);
        case 4:
            return FieldInventoryFindGearPartsSlot(itemId - 0x400);
    }
    return 0;
}

// Find the first unused inventory slot of the given kind
s32 FieldInventoryFindEmptySlotById(s32 itemId) {
    switch (itemId >> 8) {
        case 0:
            return FieldInventoryFindEmptyItemSlot();
        case 1:
            return FieldInventoryFindEmptyWeaponSlot();
        case 2:
            return FieldInventoryFindEmptyAccessorySlot();
        case 3:
            return FieldInventoryFindEmptyGearWeaponSlot();
        case 4:
            return FieldInventoryFindEmptyGearPartsSlot();
    }
    return 0;
}

void FieldScriptVMHandlerResetActorMovement(void) {
    FieldResetCurrentActorMovement();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

// Reset the movement of the actor the script VM is running on
void FieldResetCurrentActorMovement(void) {
    SpriteData* pSpriteData = g_FieldActors[D_800AFD1C].pSpriteData;
    ActorData* pActor = g_FieldScriptVMCurActor;
    u_short rotation = pActor->rotation.vx | 0x8000;

    D_800B00C0 = 1;
    pActor->moveModified.vx = 0;
    pActor->moveModified.vy = 0;
    pActor->moveModified.vz = 0;
    pActor->move.vx = 0;
    pActor->move.vy = 0;
    pActor->move.vz = 0;
    pActor->rotation.vy = rotation;
    pActor->rotation.vx = rotation;
    pSpriteData->step.x = 0;
    pSpriteData->step.z = 0;
    pSpriteData->moveSpeed = 0;
}

void FieldScriptVMHandlerSetControlAngle(void) {
    g_FieldControl.unkAngle = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Check if an actor is within a trigger zone when considering the entire scene projected to
// 2D, top-down. If so, call a script function.
void FieldScriptHandleTriggerZone2D(void) {
    ActorData* pActor;
    long actorPosition2D;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;
    long triggerPos1;
    int triggerIndex;
    FieldTriggerZone* pTrigger;

    triggerIndex = SCRIPT_READ_U8_REL(1);
    pActor = g_FieldActors[g_PlayerActorIndex].pActorData;

    // Pack Z and X position into a long for use w/ NormalClip
    actorPosition2D = (CONV_TO_GTE(pActor->position.vz) << 0x10) + CONV_TO_GTE(pActor->position.vx);
    
    // Consider trigger zone in 2D from a top-down
    triggerPos1 = (g_pFieldTriggerZones[triggerIndex].z0 << 0x10) + g_pFieldTriggerZones[triggerIndex].x0;
    triggerPos2 = (g_pFieldTriggerZones[triggerIndex].z1 << 0x10) + g_pFieldTriggerZones[triggerIndex].x1;
    triggerPos3 = (g_pFieldTriggerZones[triggerIndex].z2 << 0x10) + g_pFieldTriggerZones[triggerIndex].x2;
    triggerPos4 = (g_pFieldTriggerZones[triggerIndex].z3 << 0x10) + g_pFieldTriggerZones[triggerIndex].x3;
    
    if (NormalClip(triggerPos1, triggerPos2, actorPosition2D) >= 0 && 
        NormalClip(triggerPos2, triggerPos3, actorPosition2D) >= 0 && 
        NormalClip(triggerPos3, triggerPos4, actorPosition2D) >= 0 && 
        NormalClip(triggerPos4, triggerPos1, actorPosition2D) >= 0
    ) {
        // If we're inside the trigger zone, call script function based on argument
        if (g_FieldScriptVMCurActor->flags12C_0x6 != 0x4) {
            g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6] = g_FieldScriptVMCurActor->scriptInstructionPointer + 4;
            g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
            g_FieldScriptVMCurActor->flags12C_0x6++;;
            return;
        }
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

// Check if an actor is within a trigger zone, considering height of the trigger zone as well.
// If so, call a script function.
void FieldScriptHandleTriggerZone(void) {
    ActorData* pActor;
    int actorPositionY;
    long actorPosition2D;
    long triggerPos1;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;
    int triggerIndex;
    FieldTriggerZone* pTrigger;

    triggerIndex = SCRIPT_READ_U8_REL(1);
    pActor = g_FieldActors[g_PlayerActorIndex].pActorData;

    actorPositionY = CONV_TO_GTE(pActor->position.vy);
    if (g_pFieldTriggerZones[triggerIndex].y0 < actorPositionY && 
        (actorPositionY - pActor->height) < g_pFieldTriggerZones[triggerIndex].y0
    ) {
        triggerPos1 = (g_pFieldTriggerZones[triggerIndex].z0 << 0x10) + g_pFieldTriggerZones[triggerIndex].x0;
        triggerPos2 = (g_pFieldTriggerZones[triggerIndex].z1 << 0x10) + g_pFieldTriggerZones[triggerIndex].x1;
        triggerPos3 = (g_pFieldTriggerZones[triggerIndex].z2 << 0x10) + g_pFieldTriggerZones[triggerIndex].x2;
        triggerPos4 = (g_pFieldTriggerZones[triggerIndex].z3 << 0x10) + g_pFieldTriggerZones[triggerIndex].x3;
        actorPosition2D = (CONV_TO_GTE(pActor->position.vz) << 0x10) + CONV_TO_GTE(pActor->position.vx);

        if (
            NormalClip(triggerPos1, triggerPos2, actorPosition2D) >= 0 && 
            NormalClip(triggerPos2, triggerPos3, actorPosition2D) >= 0 && 
            NormalClip(triggerPos3, triggerPos4, actorPosition2D) >= 0 && 
            NormalClip(triggerPos4, triggerPos1, actorPosition2D) >= 0
        ) {
            // If we're inside the trigger zone, call script function based on argument
            if (g_FieldScriptVMCurActor->flags12C_0x6 != 0x4) {
                g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6] = g_FieldScriptVMCurActor->scriptInstructionPointer + 4;
                g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
                g_FieldScriptVMCurActor->flags12C_0x6++;;
                return;
            }
        }
    }
    g_FieldScriptMaxInstructionCount += 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptCheckTriggerZone2D(void) {
    ActorData* pActor;
    long actorPosition2D;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;
    long triggerPos1;
    int triggerIndex;
    FieldTriggerZone* pTrigger;

    triggerIndex = SCRIPT_READ_U8_REL(1);
    pActor = g_FieldActors[g_PlayerActorIndex].pActorData;

    // Pack Z and X position into a long for use w/ NormalClip
    actorPosition2D = (CONV_TO_GTE(pActor->position.vz) << 0x10) + CONV_TO_GTE(pActor->position.vx);
    
    // Consider trigger zone in 2D from a top-down
    triggerPos1 = (g_pFieldTriggerZones[triggerIndex].z0 << 0x10) + g_pFieldTriggerZones[triggerIndex].x0;
    triggerPos2 = (g_pFieldTriggerZones[triggerIndex].z1 << 0x10) + g_pFieldTriggerZones[triggerIndex].x1;
    triggerPos3 = (g_pFieldTriggerZones[triggerIndex].z2 << 0x10) + g_pFieldTriggerZones[triggerIndex].x2;
    triggerPos4 = (g_pFieldTriggerZones[triggerIndex].z3 << 0x10) + g_pFieldTriggerZones[triggerIndex].x3;
    
    if (NormalClip(triggerPos1, triggerPos2, actorPosition2D) >= 0 && 
        NormalClip(triggerPos2, triggerPos3, actorPosition2D) >= 0 && 
        NormalClip(triggerPos3, triggerPos4, actorPosition2D) >= 0 && 
        NormalClip(triggerPos4, triggerPos1, actorPosition2D) >= 0
    ) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    g_FieldScriptMaxInstructionCount += 1;
}

void FieldScriptCheckTriggerZone(void) {
    ActorData* pActor;
    int actorPositionY;
    long actorPosition2D;
    long triggerPos1;
    long triggerPos2;
    long triggerPos3;
    long triggerPos4;
    int triggerIndex;
    FieldTriggerZone* pTrigger;

    triggerIndex = SCRIPT_READ_U8_REL(1);
    pActor = g_FieldActors[g_PlayerActorIndex].pActorData;

    actorPositionY = CONV_TO_GTE(pActor->position.vy);
    if (g_pFieldTriggerZones[triggerIndex].y0 < actorPositionY && 
        (actorPositionY - pActor->height) < g_pFieldTriggerZones[triggerIndex].y0
    ) {
        triggerPos1 = (g_pFieldTriggerZones[triggerIndex].z0 << 0x10) + g_pFieldTriggerZones[triggerIndex].x0;
        triggerPos2 = (g_pFieldTriggerZones[triggerIndex].z1 << 0x10) + g_pFieldTriggerZones[triggerIndex].x1;
        triggerPos3 = (g_pFieldTriggerZones[triggerIndex].z2 << 0x10) + g_pFieldTriggerZones[triggerIndex].x2;
        triggerPos4 = (g_pFieldTriggerZones[triggerIndex].z3 << 0x10) + g_pFieldTriggerZones[triggerIndex].x3;
        actorPosition2D = (CONV_TO_GTE(pActor->position.vz) << 0x10) + CONV_TO_GTE(pActor->position.vx);

        if (
            NormalClip(triggerPos1, triggerPos2, actorPosition2D) >= 0 && 
            NormalClip(triggerPos2, triggerPos3, actorPosition2D) >= 0 && 
            NormalClip(triggerPos3, triggerPos4, actorPosition2D) >= 0 && 
            NormalClip(triggerPos4, triggerPos1, actorPosition2D) >= 0
        ) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
            return;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    g_FieldScriptMaxInstructionCount += 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc11", FieldProjectActorOriginToScreen);
/* 95.06%: everything but the first CompMatrix argument matches. The original
   builds the world-to-screen matrix address from the actor list:
   `lui/addiu %hi/%lo(g_FieldActors)` followed by `addiu a0, a1, -0xAC`, reusing
   a1 for the volatile actor pointer load. Writing the real symbol
   (`&g_Scene.worldToScreenMatrix`) gives two independent %hi/%lo pairs instead,
   three instructions longer. Same blocker as FieldActorWorldToScreenPosition in
   misc8.c: the matrix and the actor list were members of one global struct in
   the original source, so only that shared parent struct makes this match.
void FieldProjectActorOriginToScreen(int* pScreenX, int* pScreenY) {
    SVECTOR origin;
    MATRIX transformMatrix;
    long sxy;
    long p;
    long flag;

    CompMatrix(&g_Scene.worldToScreenMatrix, &g_FieldActors[func_8009CD7C(1)].childMatrix, &transformMatrix);
    origin.vx = 0;
    origin.vy = 0;
    origin.vz = 0;
    SetRotMatrix(&transformMatrix);
    SetTransMatrix(&transformMatrix);
    RotTransPers(&origin, &sxy, &p, &flag);
    *pScreenY = ((short*)&sxy)[1];
    *pScreenX = ((short*)&sxy)[0];
}
*/

extern s32 D_800ADC18;

void FieldScriptCheckActorOnScreenWithMargin(void) {
    int screenX;
    int screenY;

    FieldProjectActorOriginToScreen(&screenX, &screenY);
    if (D_800ADC18 != 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        return;
    }
    
    if ((screenY - 33) < 159U && (screenX - 33) < 255U) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    }
    
    D_800B00C0 = 1;
}

void FieldScriptCheckActorOnScreen(void) {
    int screenX;
    int screenY;

    FieldProjectActorOriginToScreen(&screenX, &screenY);
    if (D_800ADC18 != 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        return;
    }
    
    if ((screenY - 1) < (SCREEN_HEIGHT - 1) && (screenX - 1) < (SCREEN_WIDTH - 1)) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    }
    
    D_800B00C0 = 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc11", func_80095CC4);
/* best 96.19%: everything matches except where gcc puts `li a0,2` for the
   second call. The original has it in the jal delay slot (cc1 emitted it
   directly before the call, so reorg stole it); here gcc emits the call's
   constant argument setup before the actor-pointer address computation, so
   the delay slot gets a nop. Declaring the field as its own local moves the
   `li` into the slot but then the `lh` of walkmeshId comes before the call
   instead of after it (95%). Structure below is otherwise exact.
void func_80095CC4(void) {
    ActorData* pActor;
    int arg;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        arg = FieldScriptVMGetArgument(2);
        if (arg == pActor->walkmeshId) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
        } else {
            g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
    }
}
*/

typedef struct {
    /* 0x0 */ undefined8 unk0[0xC];
    /* 0xC */ u8 material;
    /* 0xD */ undefined8 padD;
} FieldWalkmeshTriangle; // Size: 0xE

extern FieldWalkmeshTriangle* D_800AFB24[];

void FieldScriptCheckActorWalkmeshMaterial(void) {
    ActorData* pActor;
    u8 material;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        material = D_800AFB24[pActor->walkmeshId][pActor->walkmeshTriIds[pActor->walkmeshId]].material;
        if (FieldScriptVMGetArgument(2) == material) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
            return;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
}

extern FieldActor* D_800B06B8;

void FieldScriptCheckActorDistance(void) {
    ActorData* pActor;
    ActorData* pActorOther;
    FieldActor* pFieldActor;
    int distance;
    int actorIndex;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != 0xFF) {
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        
        pActor = D_800B06B8->pActorData;
        pActorOther = pFieldActor->pActorData;
        distance = FieldGetVec3Magnitude(
            CONV_TO_GTE(pActor->position.vx) - CONV_TO_GTE(pActorOther->position.vx), 
            CONV_TO_GTE(pActor->position.vy) - CONV_TO_GTE(pActorOther->position.vy), 
            CONV_TO_GTE(pActor->position.vz) - CONV_TO_GTE(pActorOther->position.vz)
        );
        
        if (distance < FieldScriptVMGetArgument(2)) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
            return;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
}