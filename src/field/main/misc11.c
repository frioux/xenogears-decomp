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

// An actor script data word holds 12 bits; SetActorDataWord clamps to the maximum
#define FIELD_ACTOR_DATA_WORD_MAX 0xFFF

// g_FieldLineScrolls.pLineScrolls, .pDatas and .lengths hold this many entries
#define FIELD_LINE_SCROLL_MAX 32

// Number of steps a stepping actor rotation takes; curDoorStep counts them
#define FIELD_DOOR_STEP_COUNT 31

// Angle each of those steps moves the actor's Y rotation by
#define FIELD_DOOR_STEP_ANGLE 0x20

// g_FieldImageRects/g_FieldImageDestX/g_FieldImageDestY hold this many entries
#define FIELD_IMAGE_SLOT_COUNT 32

// FieldScene.unk48 bits: SetSceneEncounterFlag sets the first,
// SetRandomEncounterFlags sets both

// StepActorMoveOn/Off read a direction angle (0..0xFFF) here; these two step
// along Y instead, decreasing / increasing it
#define FIELD_MOVE_DIRECTION_Y_DECREASE 0x1000
#define FIELD_MOVE_DIRECTION_Y_INCREASE 0x1001

// Half of the field angle turn, so adding it points the opposite way

// HandlerChangeMap's map argument when the script does not change the map
#define FIELD_MAP_NUM_NONE 0x7FFF

// HandlerSetFieldEntryState's facing argument when the script gives none, so the
// entry angle follows the scene angle
#define FIELD_ENTRY_ANGLE_NONE 0xFFFF

// An inventory itemId carries the kind of table it indexes in its high byte
#define FIELD_INVENTORY_KIND_ITEM 0
#define FIELD_INVENTORY_KIND_WEAPON 1
#define FIELD_INVENTORY_KIND_ACCESSORY 2
#define FIELD_INVENTORY_KIND_GEAR_WEAPON 3
#define FIELD_INVENTORY_KIND_GEAR_PARTS 4

extern s32 g_FieldCurActorIndex;
extern s32 g_PlayerActorIndex;

extern u8 g_FieldFogNearR;
extern u8 g_FieldFogNearG;
extern u8 g_FieldFogNearB;
extern u8 g_FieldFogFarR;
extern u8 g_FieldFogFarG;
extern u8 g_FieldFogFarB;
extern s16 g_FieldFogNearDistance;
extern s16 g_FieldFogFarDistance;
extern s16 g_FieldFogEnabled;

void FieldUpdateActorRenderModes(void);

void FieldScriptVMHandlerSetFogColor(void) {
    g_FieldFogNearR = FieldScriptVMGetArgument(1);
    g_FieldFogNearG = FieldScriptVMGetArgument(3);
    g_FieldFogNearB = FieldScriptVMGetArgument(5);
    g_FieldFogFarR = FieldScriptVMGetArgument(7);
    g_FieldFogFarG = FieldScriptVMGetArgument(9);
    g_FieldFogFarB = FieldScriptVMGetArgument(0xB);
    g_FieldFogNearDistance = FieldScriptVMGetArgument(0xD);
    g_FieldFogFarDistance = FieldScriptVMGetArgument(0xF);
    g_FieldFogEnabled = 1;
    FieldUpdateActorRenderModes();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x11;
}

typedef struct { u8 value; } FieldScriptGlobalU8;

extern FieldScriptGlobalU8 g_FieldClearColorR;
extern FieldScriptGlobalU8 g_FieldClearColorG;
extern FieldScriptGlobalU8 g_FieldClearColorB;

void FieldScriptVMHandlerSetScrollLimits(void) {
    g_Scene.scrollLimits[0] = FieldScriptVMGetInstructionArgumentS16(1);
    g_Scene.scrollLimits[1] = FieldScriptVMGetInstructionArgumentS16(3);
    g_Scene.scrollLimits[2] = FieldScriptVMGetInstructionArgumentS16(5);
    g_Scene.scrollLimits[3] = -FieldScriptVMGetInstructionArgumentS16(7);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptVMHandlerSetClearColor(void) {
    g_FieldClearColorR.value = FieldScriptVMGetArgument(1);
    g_FieldClearColorG.value = FieldScriptVMGetArgument(3);
    g_FieldClearColorB.value = FieldScriptVMGetArgument(5);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void FieldScriptVMHandlerEmpty(void) {
}

// g_FieldImageDrawIndex wraps within this many queued image rects
extern RECT g_FieldImageRects[FIELD_IMAGE_SLOT_COUNT];
extern s16 g_FieldImageDestX[FIELD_IMAGE_SLOT_COUNT];
extern s16 g_FieldImageDestY[FIELD_IMAGE_SLOT_COUNT];
extern u_short g_FieldImageDrawIndex;

// objdiff reads 99.46% because splat's expected asm names the interior addresses
// (D_800AF5EA/EC/EE): the .y/.w/.h element offsets become displacements off
// g_FieldImageRects, which resolve to those same addresses. check_bin: OK.

void FieldMoveOrClearImageRect(int x, int y, int w, int h, int texX, int texY, int clear) {
    int index = g_FieldImageDrawIndex & (FIELD_IMAGE_SLOT_COUNT - 1);

    g_FieldImageRects[index].x = x;
    g_FieldImageRects[index].y = y;
    g_FieldImageRects[index].w = w;
    g_FieldImageRects[index].h = h;
    g_FieldImageDestX[index] = texX;
    g_FieldImageDestY[index] = texY;
    if (!clear) {
        MoveImage(&g_FieldImageRects[index], g_FieldImageDestX[index], (s16)texY);
    } else {
        ClearImage(&g_FieldImageRects[index], 0, 0, 0);
    }
    g_FieldImageDrawIndex++;
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
    g_FieldScriptVMCurActor->spriteClipY = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptVMHandlerSetActorFlags134ByIndex(void) {
    ActorData* pActor;
    int value;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        value = FieldScriptArgument1(2, SCRIPT_READ_U8_REL(6));
        pActor->flags134 = (pActor->flags134 & 0xFFFFFF9F) | ((value & 3) << 5);
        pActor->spriteClipY = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(6));
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void FieldScriptVMHandlerSetActorDataWord(void) {
    int index;
    int value;

    index = FieldScriptVMGetArgument(1);
    value = FieldScriptVMGetArgument(3);
    if (value >= FIELD_ACTOR_DATA_WORD_MAX + 1) {
        value = FIELD_ACTOR_DATA_WORD_MAX;
    }

    if (g_FieldActors[g_FieldCurActorIndex].status & 0x2000) {
        ((u_int*)g_FieldScriptVMCurActor->pScriptWorkWords)[index] = value;
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
    /* 0x4   */ LineScroll* pLineScrolls[FIELD_LINE_SCROLL_MAX];
    /* 0x84  */ u8* pDatas[FIELD_LINE_SCROLL_MAX];
    /* 0x104 */ s16 lengths[FIELD_LINE_SCROLL_MAX];
} LineScrollTable; // Size: 0x144

extern LineScrollTable g_FieldLineScrolls;

void FieldUpdateLineScrolls(void) {
    int i;

    for (i = 0; i < g_FieldLineScrolls.numLineScrolls; i++) {
        GfxLineScrollUpdate(g_FieldLineScrolls.pLineScrolls[i]);
    }
}

// objdiff reads 99.50% for the same reason as FieldMoveOrClearImageRect: splat names the
// interior address D_800AFFAC, this indexes g_FieldLineScrolls.lengths/.pDatas; check_bin: OK.
void FieldScriptVMHandlerWriteLineScrollByte(void) {
    int index;
    int offset;
    int value;

    index = FieldScriptVMGetArgument(1);
    offset = FieldScriptVMGetArgument(3);
    value = FieldScriptVMGetInstructionArgument(5) & 0xFFFF;

    if (offset < g_FieldLineScrolls.lengths[index]) {
        g_FieldLineScrolls.pDatas[index][offset] = value;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern s32 g_FieldScriptRoutineRunning;

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

    if (g_FieldScriptRoutineRunning == 0 && g_FieldLineScrolls.numLineScrolls < FIELD_LINE_SCROLL_MAX) {
        count = FieldScriptVMGetInstructionArgument(9) & 0xFFFF;
        g_FieldLineScrolls.lengths[g_FieldLineScrolls.numLineScrolls] = count;
        g_FieldLineScrolls.pDatas[g_FieldLineScrolls.numLineScrolls] = HeapAlloc(count + 1, 0);
        g_FieldLineScrolls.pLineScrolls[g_FieldLineScrolls.numLineScrolls] = HeapAlloc(sizeof(LineScroll), 0);
        value = FieldScriptVMGetInstructionArgument(0xF) & 0xFFFF;
        for (i = 0; i < count; i++) {
            g_FieldLineScrolls.pDatas[g_FieldLineScrolls.numLineScrolls][i] = value;
        }
        arg1 = (s16)FieldScriptVMGetInstructionArgument(1);
        arg3 = (s16)FieldScriptVMGetInstructionArgument(3);
        arg5 = (s16)FieldScriptVMGetInstructionArgument(5);
        arg7 = (s16)FieldScriptVMGetInstructionArgument(7);
        argB = (s16)FieldScriptVMGetInstructionArgument(0xB);
        GfxLineScrollInitialize(g_FieldLineScrolls.pLineScrolls[g_FieldLineScrolls.numLineScrolls], arg1, arg3, arg5, arg7, count, argB, (s16)FieldScriptVMGetInstructionArgument(0xD),
                                g_FieldLineScrolls.pDatas[g_FieldLineScrolls.numLineScrolls]);
        g_FieldLineScrolls.numLineScrolls++;
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

extern u_int* g_FieldWalkmeshMaterialFlags;

void FieldScriptSetRecordByte(int index, int byteIndex, u_int value);
void FieldResetCurrentActorMovement(void);

// Read one byte of the 4-byte record at the given index of g_FieldWalkmeshMaterialFlags
int FieldScriptGetRecordByte(int index, int byteIndex) {
    switch (byteIndex) {
        case 0:
            return ((u_char*)&g_FieldWalkmeshMaterialFlags[index])[0];
        case 1:
            return ((u_char*)&g_FieldWalkmeshMaterialFlags[index])[1];
        case 2:
            return ((u_char*)&g_FieldWalkmeshMaterialFlags[index])[2];
        case 3:
            return ((u_char*)&g_FieldWalkmeshMaterialFlags[index])[3];
    }
    return 0;
}

// Write one byte of the 4-byte record at the given index of g_FieldWalkmeshMaterialFlags
void FieldScriptSetRecordByte(int index, int byteIndex, u_int value) {
    switch (byteIndex) {
        case 0:
            g_FieldWalkmeshMaterialFlags[index] = (g_FieldWalkmeshMaterialFlags[index] & ~0xFF) | value;
            break;
        case 1:
            value <<= 8;
            g_FieldWalkmeshMaterialFlags[index] = (g_FieldWalkmeshMaterialFlags[index] & ~(0xFF << 8)) | value;
            break;
        case 2:
            value <<= 16;
            g_FieldWalkmeshMaterialFlags[index] = (g_FieldWalkmeshMaterialFlags[index] & ~(0xFF << 16)) | value;
            break;
        case 3:
            value <<= 24;
            g_FieldWalkmeshMaterialFlags[index] = (g_FieldWalkmeshMaterialFlags[index] & ~(0xFF << 24)) | value;
            break;
    }
}

extern s32 g_FieldFrameDelay;
extern s16 g_FieldTextScaleDivisor;

void func_800925A0(void) {
    int arg = FieldScriptVMGetArgument(1);

    g_FieldFrameDelay = arg;
    switch (arg) {
        case 0:
            g_FieldTextScaleDivisor = 8;
            break;
        case 1:
            g_FieldTextScaleDivisor = 6;
            break;
        case 2:
            g_FieldTextScaleDivisor = 4;
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerSetControllerBtnMask(void) {
    g_FieldControl.controllerBtnMask = FieldScriptVMGetInstructionArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerSetRecordByte(void) {
    FieldScriptSetRecordByte(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2), FieldScriptVMGetArgument(3));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerOrRecordByte(void) {
    int value;

    value = FieldScriptGetRecordByte(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2));
    value |= FieldScriptVMGetArgument(3);
    FieldScriptSetRecordByte(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2), value);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerAndRecordByte(void) {
    int value;

    value = FieldScriptGetRecordByte(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2));
    value &= FieldScriptVMGetArgument(3);
    FieldScriptSetRecordByte(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2), value);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern FieldActor* g_FieldScriptVMCurFieldActor;
extern s16 D_800C3A6A;
extern int rcos(int);
extern int rsin(int);

void FieldScriptVMHandlerSetDirectionFromRotation(void) {
    g_FieldScriptVMCurActor->positionOffset.vx = ((u_int)rsin(g_FieldScriptVMCurFieldActor->rotation.y) * 9) >> 10;
    g_FieldScriptVMCurActor->positionOffset.vz = -(rcos(g_FieldScriptVMCurFieldActor->rotation.y) * 36) >> 12;
    g_FieldScriptVMCurActor->flags |= 0x800;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

extern u_int g_FieldSavedPlayerScriptFlags[];

extern s32 g_FieldMapChangeGate;
extern s32 g_FieldEntryStateGate;
extern s32 g_FieldArchiveStreamActive;
extern s32 g_FieldSpecialAnimLoadPending;
extern s32 g_GameWdsBankLoadState;
extern s32 g_FieldMapTransitionGate;
extern int g_GameSceneMapNum;

long FieldGetVec1Magnitude(long);
long FieldGetVec2Magnitude(long, long);
s32 FieldGetVec2Angle(IVEC3*);
void FieldSetGearModelAnimationForActor(int, int, FieldActor*);

// A field angle is a 4096-step turn, so a quarter of one points sideways
#define FIELD_ANGLE_QUARTER_TURN 0x400

// An actor that has not moved for this many frames gives up walking and is
// snapped onto its target
#define FIELD_ACTOR_STUCK_FRAME_MAX 65

/* 91.82% (904 bytes, retail 908). The block-order wall is broken: retail inlines the
   ARRIVAL block right after the `slt radius,distance` test and its walking arm's
   give-up test branches BACK into it, and a single shared arrival reaches that layout
   where three if/else spellings and a duplicated arrival (cross-jumped onto the later
   copy) all put the arrival block last at 48.90%. It takes a label:
       if (radius < distance) goto walk;
   arrived: <arrival>; return 0;
   walk:    <stuck counter>; if (stuck < MAX) { ...; return -1; } goto arrived;
   Residual, measured, in order of size:
   (1) the entry copies of arg2/arg3 (x/z) take the wrong callee-saved registers: mine
       s1/s0, retail s0/s1, and the copy of x lands at the END of the entry block
       instead of in the prologue. s0 goes to whichever of the two gets the LATE copy:
       `x = arg2` before the calls gives x=s1 + a late copy of z (91.82), `x = arg2`
       after curZ gives x=s0 + a late copy of x (91.18) - retail has BOTH in the
       prologue and x in s0, which no spelling tried produces;
   (2) the arrival block's argument setup is hoisted: `addu a0,s7,zero` and
       `addu a1,zero,zero` float above `lhu 0x106(s2)` (retail fills that branch's
       delay slot with `nop` and emits the a0 copy second), and the
       g_PlayerActorIndex load is hoisted above the `ori 0x8000`;
   (3) `li v0,-1` and `li a0,8` swap places in the prologue.
   Also measured: dropping the rotationAngle local and reusing arg3 gives retail's
   exact 908 bytes but 91.01%, so the missing instruction is in one of the above.
   Round 470 re-measured and added three spellings, all worse or equal: `z = arg3;`
   made unconditional at the top (retail's both-copies-in-the-prologue shape) gives
   x=s1 / z=s0 at ENTRY - the pair is mirrored, not the positions - and the else arm
   then loses its join copy, 91.42% / 896 bytes / 224 insns (so the `else z = arg3;`
   is load-bearing for the `move a3,s1` join copy); naming the two parameters x/z
   instead of arg2/arg3 (no local copies at all) measures the same prologue as the
   plain body, so the tie is not local-vs-parameter. The hand-out is a global-allocno
   priority contest (z wins $s0 whichever way the statements are ordered), not a
   statement-order one - see levers.md a-global-allocno-priority-is. */
// Round 483 re-measured the park: 91.82% / 904 bytes / 226 insns, one instruction short
// (retail's nop in the bnez delay slot is lost because our arrival block emits its
// 'move a0,s7' first and reorg fills the slot with it; retail's first arrival insn is
// the lhu that writes the branch's v0 and cannot fill). Preset screens, both dead:
// gcc-2.7.2-cdk 91.36, gcc-2.6.0 92.88 - and 2.6.0 drops 13 already-matched functions
// in this unit under 100 (SetControllerBtnMask 85.33, SetRecordByte 85.60, SetFogColor
// 95.51, SetActorFlags134 94.21, SetDirectionFromRotation 93.71, RestoreRandom-
// Encounters 90.83, RequestFieldState0 97.00, RequestFieldState3/4/5 85.60,
// RequestFieldState1WithValue 92.14, func_800925A0 93.53, SetScrollLimits 92.14,
// SetClearColor 84.35), so the unit is Default and cannot be switched.
// NEW this round, and the useful half of the result: the prologue shape IS reachable.
// Naming the two coordinate parameters x and z and assigning to them in the if arm only
// (s32 f(s32 arg0, s32 arg1, s32 x, s32 z)) gives retail's FOUR assign_parms prologue
// copies (sw sN / move sN,argN for a0,a1,a2,a3) at 91.42% / 896 bytes / 224 insns. What
// it does not give is retail's register map or the else-arm join copy: x takes $s1 and
// arg3's home $s0 (mirrored), and z's if-arm definition lands straight in its home
// instead of in the join register $a3, so retail's 'move a3,s1' in the else arm
// disappears. So retail's z is a MERGE value in caller-saved $a3 (its if-arm value
// computed into the rcos jal delay slot, its else-arm value copied from arg3's
// callee-saved home) while x is a plain callee-saved local - and the pair that mirrors is
// x against arg3's home. arg3's home copy is emitted at the END of the entry block in
// every spelling measured (retail has it in the prologue), which shortens its live range
// and hands it the LOWER register; that is the whole residual, and it is the allocno
// contest of levers.md a-global-allocno-priority-is, not a statement order.
// Best 96.68% / 912 bytes / 228 insns (retail 908 / 227), round 496. Two round-496 wins, both measured:
// (1) `x = arg2;` MOVED INTO THE ELSE ARM (with `s32 x, z;` uninitialised) gives retail's
//     prologue exactly: s4=arg0, s3=arg1, s0=arg2, s1=arg3 and the s0 save pair in place
//     (the if-arm x lands in s0, the else-arm z is copied `move a3,s1`), 92.56 from 91.82.
//     So the x/arg3-home mirror of the old note was a live-range question after all: the
//     entry copy made the x pseudo live from insn 2 and it lost the contest.
// (2) THE ARRIVAL BLOCK'S ORDER IS RETAIL'S, for free, from a one-iteration do-while of the
//     two dead stores before the call (levers h-an-empty-do-while / a-dead-if-else-block):
//         do { pSpriteData->moveSpeed = 0; pActorData->curAnimationId = 0; } while (0);
//         FieldSetGearModelAnimationForActor(...);        <- label between them: 96.42
//     The label stops reorg stealing the call's arg copy into the bnez delay slot, so the
//     block starts with the `lhu` and the slot is retail's nop, and `sh zero,232`,
//     `move a1,zero` and the g_PlayerActorIndex chain land in retail's order. Keeping the
//     call OUTSIDE the do-while (a second label, so the arg copies start a block) is 96.68.
// Residual, two allocator facts, both visible in the last 20 lines of the diff:
// (a) retail has a PER-BLOCK copy of pSpriteData in $a0 filling the lhu load-delay slot and
//     its moveSpeed store goes through it (`move a0,s7` / `sw zero,24(a0)`); we coalesced
//     pSpriteData into s3, so we emit `sw zero,24(s3)` plus a late `move a0,s3` and the
//     load-delay slot takes a nop - that nop IS the 4 extra bytes, so nothing here can link
//     until it goes.
// (b) the callee-saved map is a one-step rotation of retail's: ours pSpriteData=s3, arg1=s4,
//     arg0=s5, curX=s6, curZ=s7; retail arg1=s3, arg0=s4, curX=s5, curZ=s6, pSpriteData=s7.
//     Same save SET, so retail's pSpriteData qty is the LOWEST-priority allocno of the five
//     and ours is the highest - levers a-global-allocno-priority-is; read -dl before spending
//     a lane, the pSpriteData qty is live entry-to-arrival in both.
// Measured and worse: `rotation |= 0x8000` inside the do-while so the label sits after the
// lhu (92.43), swapping the two stores (95.82), spelling the entry actor pointer
// `g_FieldActors + g_PlayerActorIndex` (byte-identical to `&g_FieldActors[...]` at 92.56).
// Untouched residual from the old note: `li v0,-1` / `li a0,8` swap places in the prologue
// and retail's entry loads pSpriteData before the flags word where we sink it after.
/*
s32 func_80092894(s32 arg0, s32 arg1, s32 arg2, s32 arg3) {
    FieldActor* pFieldActor;
    ActorData* pActorData;
    SpriteData* pSpriteData;
    IVEC3 delta;
    u_short rotation;
    s32 angle;
    s32 rotationAngle;
    s32 radius;
    s32 distance;
    s32 x;
    s32 z;
    s32 curX;
    s32 curZ;

    g_FieldControl.isRandomEncountersEnabled = -1;
    pFieldActor = &g_FieldActors[g_PlayerActorIndex];
    pActorData = pFieldActor->pActorData;
    pSpriteData = pFieldActor->pSpriteData;
    pActorData->flags |= 0x38;
    pSpriteData->moveSpeed = 0x80000;
    radius = FieldGetVec1Magnitude(pSpriteData->moveSpeed >> 16) * 2;
    curX = ((s16*)&pActorData->position.vx)[1];
    curZ = ((s16*)&pActorData->position.vz)[1];

    if (arg1 == 0) {
        if (g_FieldMapChangeGate == 0 || g_FieldEntryStateGate == 0) {
            g_FieldScriptVMStopFlag = 1;
        }
        if (g_FieldMapTransitionGate != 0) {
            s32 mapArg = FieldScriptVMGetArgument(4);
            s32 mapValue = FieldScriptVMGetArgument(2);

            FieldScriptVMSaveSceneState();
            g_FieldMapTransitionGate = 0;
            FieldScriptMemoryWriteU16(2, mapArg);
            g_GameSceneMapNum = mapValue;
        }
        angle = g_FieldScriptVMCurFieldActor->rotation.y + arg0 - FIELD_ANGLE_QUARTER_TURN;
        x = g_FieldScriptVMCurActor->positionOffset.vx + ((s16*)&g_FieldScriptVMCurActor->position.vx)[1] + ((rsin(angle) * 40) >> 12);
        z = g_FieldScriptVMCurActor->positionOffset.vz + ((s16*)&g_FieldScriptVMCurActor->position.vz)[1] + (-(rcos(angle) * 40) >> 12);
    } else {
        x = arg2;
        z = arg3;
    }

    delta.x = x - curX;
    delta.y = 0;
    delta.z = z - curZ;
    distance = FieldGetVec2Magnitude(delta.x, delta.z);

    if (radius < distance) {
        goto walk;
    }

arrived:
    rotation = (u_short)pActorData->rotation.vy | 0x8000;
    pActorData->rotation.vx = rotation;
    pActorData->rotation.vy = rotation;
    do {
        pSpriteData->moveSpeed = 0;
        pActorData->curAnimationId = 0;
    } while (0);
    FieldSetGearModelAnimationForActor(pSpriteData, 0, &g_FieldActors[g_PlayerActorIndex]);
    g_FieldScriptVMStopFlag = 1;
    pActorData->scripts[pActorData->curScriptIndex].flags_0 = 0xFFFF;
    pActorData->scripts[pActorData->curScriptIndex].flags_0x17 = 0;
    pActorData->scriptFlags.flags &= ~0x200000;
    if (arg1 == 1) {
        pActorData->flags &= ~0x38;
    }
    pActorData->stuckFrameCount = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    return 0;

walk:
    if (pActorData->prevPosition.x == ((s16*)&pActorData->position.vx)[1]
        && pActorData->prevPosition.y == ((s16*)&pActorData->position.vy)[1]
        && pActorData->prevPosition.z == ((s16*)&pActorData->position.vz)[1]) {
        pActorData->stuckFrameCount++;
    } else {
        pActorData->stuckFrameCount = 0;
    }

    if ((s16)pActorData->stuckFrameCount < FIELD_ACTOR_STUCK_FRAME_MAX) {
        rotationAngle = FieldGetVec2Angle(&delta);
        pActorData->rotation.vx = rotationAngle;
        pActorData->rotation.vy = rotationAngle;
        g_FieldScriptVMStopFlag = 1;
        if (arg1 != 0) {
            g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        }
        return -1;
    }
    goto arrived;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc11", func_80092894);




void FieldPlayerSetScriptFlag80(void);

void FieldScriptVMHandlerMapTransitionWithArgs(void) {
    int arg1;
    int arg3;

    if (g_FieldMapChangeGate == 0 || g_FieldEntryStateGate == 0 || g_FieldArchiveStreamActive != 0 || g_GameWdsBankLoadState == -1 || g_FieldSpecialAnimLoadPending != 0) {
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        return;
    }

    arg1 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5));
    arg3 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5));
    if (g_FieldSavedPlayerScriptFlags[0] == 0) {
        g_FieldSavedPlayerScriptFlags[0] = g_FieldActors[g_PlayerActorIndex].pActorData->scriptFlags.flags;
    }
    FieldPlayerSetScriptFlag80();
    if (func_80092894(0, 1, arg1, arg3)) {
        return;
    }
    if (!(g_FieldSavedPlayerScriptFlags[0] & 0x80)) {
        g_FieldActors[g_PlayerActorIndex].pActorData->scriptFlags.flags &= ~0x80;
    }
    g_FieldSavedPlayerScriptFlags[0] = 0;
}

void FieldScriptVMHandlerMapTransition(void) {
    if (g_FieldMapChangeGate == 0 || g_FieldEntryStateGate == 0 || g_FieldArchiveStreamActive != 0 || g_GameWdsBankLoadState == -1 || g_FieldSpecialAnimLoadPending != 0) {
        g_FieldScriptVMStopFlag = 1;
    } else {
        FieldPlayerSetScriptFlag80();
        func_80092894(0, 0, 0, 0);
    }
}

void FieldScriptVMHandlerMapTransitionOffset(void) {
    if (g_FieldMapChangeGate == 0 || g_FieldEntryStateGate == 0 || g_FieldArchiveStreamActive != 0 || g_GameWdsBankLoadState == -1 || g_FieldSpecialAnimLoadPending != 0) {
        g_FieldScriptVMStopFlag = 1;
    } else {
        FieldPlayerSetScriptFlag80();
        func_80092894(0x3E0, 0, 0, 0);
    }
}

extern s32 g_FieldGameStateChangeRequest;
extern s32 g_FieldNextGameState;

int FieldGetPlayerActorDirection(void);
int FieldGetCameraDirection(void);

void FieldScriptVMSaveSceneState(void) {
    FieldScriptMemoryWriteU16(4, g_GameSceneMapNum & 0x3FFF);
    FieldScriptMemoryWriteU16(6, FieldGetPlayerActorDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(8, FieldGetCameraDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(0x12, (short)(FieldScriptVMGetVariableValue(0x12) + 1));
}

void FieldScriptVMHandlerRestoreRandomEncounters(void) {
    if (g_FieldGameStateChangeRequest != 0) {
        g_FieldControl.isRandomEncountersEnabled = -1;
        g_FieldGameStateChangeRequest = 0;
        g_FieldNextGameState = FieldScriptVMGetArgument(1);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern u8 g_FieldScriptMemoryUpdateLocked;

void FieldScriptVMHandlerSetFieldEntryState(void) {
    int value;

    if (g_FieldMapChangeGate == 0 || g_FieldEntryStateGate == 0 || g_FieldArchiveStreamActive != 0 || g_GameWdsBankLoadState == -1 || g_FieldSpecialAnimLoadPending != 0) {
        g_FieldScriptVMStopFlag = 1;
        return;
    }

    FieldScriptUpdateMemoryEachFrame();
    g_FieldControl.isRandomEncountersEnabled = -1;
    g_FieldEntryStateGate = 0;
    g_pGameState->mapNum = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    g_pGameState->unk231E = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    value = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(9));
    if (((u_short)value == FIELD_ENTRY_ANGLE_NONE) | (value == -1)) {
        g_pGameState->entryAngle = (g_Scene.sceneAngle.vy + FIELD_ANGLE_HALF_TURN) & FIELD_ANGLE_MASK;
    } else {
        g_pGameState->entryAngle = (value + FIELD_ANGLE_HALF_TURN) & FIELD_ANGLE_MASK;
    }
    g_pGameState->nextMapFlags = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(9));
    FieldScriptVMEmptyHook();
    g_FieldScriptMemoryUpdateLocked = 1;
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

void FieldScriptVMEmptyHook(void) {
}

extern s32 g_FieldFadeMode;
extern s32 g_FieldFadeDuration;

void FieldScriptVMHandlerSetFadeParams(void) {
    if (g_FieldMapChangeGate == 0 || g_FieldEntryStateGate == 0 || g_FieldArchiveStreamActive != 0 || g_GameWdsBankLoadState == -1 || g_FieldSpecialAnimLoadPending != 0) {
        g_FieldScriptVMStopFlag = 1;
        return;
    }

    FieldScriptVMHandlerRestoreMap();
    FieldScriptVMEmptyHook();
    g_FieldFadeMode = FieldScriptVMGetArgument(0);
    g_FieldFadeDuration = FieldScriptVMGetArgument(2);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

extern s32 g_FieldStreamSceneActive;
void FieldScriptVMHandlerRestoreMap(void) {
    int a;
    int b;

    if (g_FieldMapChangeGate == 0 || g_FieldEntryStateGate == 0 || g_FieldArchiveStreamActive != 0 || g_GameWdsBankLoadState == -1 || g_FieldSpecialAnimLoadPending != 0 || g_FieldStreamSceneActive != 0) {
        g_FieldScriptVMStopFlag = 1;
        return;
    }

    g_FieldControl.isRandomEncountersEnabled = -1;
    if (g_FieldMapTransitionGate != 0) {
        a = FieldScriptVMGetArgument(3);
        b = FieldScriptVMGetArgument(1);
        FieldScriptVMSaveSceneState();
        g_FieldMapTransitionGate = 0;
        FieldScriptMemoryWriteU16(2, a);
        g_GameSceneMapNum = b;
        FieldScriptVMEmptyHook();
    }
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern u8 g_FieldScriptSoundEffectBank;
extern u8 g_SoundEffectBankIndex;
extern u8 g_GameSelectedSkinIndex;
extern u8 g_GameSelectedSkinLoadPending;
extern s32 g_FieldMapChangeRequestGate;
extern s32 g_FieldTransitionBusyFlag;
extern s32 g_FieldMapNumChanged;

void FieldScriptVMHandlerChangeMap(void) {
    int mapNum;

    if (g_FieldMapChangeGate == 0 || g_FieldEntryStateGate == 0 || g_FieldMapTransitionGate == 0 || g_FieldArchiveStreamActive != 0 || g_GameWdsBankLoadState == -1 || g_FieldSpecialAnimLoadPending != 0) {
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        return;
    }

    g_SoundEffectBankIndex = g_FieldScriptSoundEffectBank;
    g_GameSelectedSkinIndex = FieldScriptVMGetArgument(1);
    g_GameSelectedSkinLoadPending = 0;
    g_FieldMapChangeGate = 0;
    g_FieldMapChangeRequestGate = 0;
    g_FieldTransitionBusyFlag = 1;
    mapNum = FieldScriptVMGetArgument(5);
    if (mapNum != FIELD_MAP_NUM_NONE) {
        int value = FieldScriptVMGetArgument(7);

        FieldScriptVMSaveSceneState();
        FieldScriptMemoryWriteU16(2, value);
        g_GameSceneMapNum = mapNum;
        g_FieldMapNumChanged = 1;
    }
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptVMHandlerReloadMap(void) {
    if (g_FieldMapChangeGate == 0 || g_FieldEntryStateGate == 0 || g_FieldMapTransitionGate == 0 || g_FieldArchiveStreamActive != 0 || g_GameWdsBankLoadState == -1 || g_FieldSpecialAnimLoadPending != 0) {
        g_FieldScriptVMStopFlag = 1;
        return;
    }

    g_SoundEffectBankIndex = g_FieldScriptSoundEffectBank;
    g_GameSelectedSkinIndex = FieldScriptVMGetArgument(1);
    g_GameSelectedSkinLoadPending = 0;
    g_FieldMapChangeGate = 0;
    g_FieldMapChangeRequestGate = 0;
    g_FieldTransitionBusyFlag = 1;
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 g_FieldPendingRequestCount;
extern s8 g_ShopIndex;
extern s32 g_FieldRequestCode;
extern u_short g_FieldScriptShopIndex;

void FieldScriptVMHandlerCopyRecordByte(void) {
    int value = FieldScriptGetRecordByte(SCRIPT_READ_U8_REL(1), SCRIPT_READ_U8_REL(2));

    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, value);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerWaitForFieldRequest(void) {
    if (g_FieldPendingRequestCount == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

void FieldScriptVMHandlerRequestFieldState0(void) {
    g_FieldScriptVMStopFlag = 1;
    g_FieldRequestCode = 0;
    g_ShopIndex = g_FieldScriptShopIndex;
    g_FieldPendingRequestCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptVMHandlerRequestFieldState6(void) {
    g_ShopIndex = 1;
    g_FieldRequestCode = 6;
    g_FieldScriptVMStopFlag = 1;
    g_FieldPendingRequestCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptVMHandlerRequestFieldState2(void) {
    g_FieldRequestCode = 2;
    g_FieldScriptVMStopFlag = 1;
    g_FieldPendingRequestCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptVMHandlerRequestFieldState3(void) {
    g_ShopIndex = FieldScriptVMGetArgument(1);
    g_FieldRequestCode = 3;
    g_FieldScriptVMStopFlag = 1;
    g_FieldPendingRequestCount++;
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
    g_FieldRequestCode = 1;
    g_FieldScriptVMStopFlag = 1;
    g_FieldPendingRequestCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerRequestFieldState1WithValue(void) {
    ActorData* pActor;
    int value;

    value = FieldScriptVMGetArgument(1);
    g_FieldRequestCode = 1;
    g_FieldScriptVMStopFlag = 1;
    pActor = g_FieldScriptVMCurActor;
    *(s16*)&g_pGameState->unk1928[0xA] = value;
    g_pGameState->nextMapFlags = value;
    D_800C3A6A = value;
    g_FieldPendingRequestCount++;
    pActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerRequestFieldState4(void) {
    g_ShopIndex = FieldScriptVMGetArgument(1);
    g_FieldRequestCode = 4;
    g_FieldScriptVMStopFlag = 1;
    g_FieldPendingRequestCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerRequestFieldState5(void) {
    g_ShopIndex = FieldScriptVMGetArgument(1);
    g_FieldRequestCode = 5;
    g_FieldScriptVMStopFlag = 1;
    g_FieldPendingRequestCount++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}


// Random encounter stuff

// These are likely part of a struct
extern s8 g_FieldRandomEncounterFlag[];
extern s8 g_FieldCompassDisabled[];

extern s32 g_FieldMapChangeGate;
extern s32 g_FieldEntryStateGate;
extern s32 g_FieldScriptVMStopFlag;

void FieldScriptVMClearSceneEncounterFlag(void) {
    g_Scene.flags &= 0x7FFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMSetSceneEncounterFlag(void) {
    g_Scene.flags |= FIELD_SCENE_ENCOUNTER_FLAG;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMClearRandomEncounterFlags(void) {
    g_FieldControl.isRandomEncountersEnabled = 0;
    g_FieldRandomEncounterFlag[0] = 0;
    g_FieldCompassDisabled[0] = 0;
    g_Scene.flags &= 0x3FFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMSetRandomEncounterFlags(void) {
    g_FieldControl.isRandomEncountersEnabled = -1;
    g_FieldRandomEncounterFlag[0] = 1;
    g_FieldCompassDisabled[0] = 1;
    g_Scene.flags |= FIELD_SCENE_RANDOM_ENCOUNTER_FLAGS;
    if ((g_FieldMapChangeGate == 0) || (g_FieldEntryStateGate == 0)) {
        g_FieldScriptVMStopFlag = 1; // Stop script VM execution?
        g_FieldScriptVMCurActor->scriptInstructionPointer--;
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMClearFlag21D0(void) {
    g_FieldRandomEncounterFlag[0] = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMSetFlag21D0(void) {
    g_FieldRandomEncounterFlag[0] = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptEnableCompass(void) {
    g_FieldCompassDisabled[0] = 0x0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptDisableCompass(void) {
    g_FieldCompassDisabled[0] = 0x1;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerDisableRandomEncounters(void) {
    g_FieldControl.isRandomEncountersEnabled = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerEnableRandomEncounters(void) {
    if (g_FieldMapChangeGate == 0 || g_FieldEntryStateGate == 0) {
        g_FieldScriptVMStopFlag = 1;
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
            if (g_FieldScriptVMCurActor->curDoorStep < FIELD_DOOR_STEP_COUNT) {
                if (SCRIPT_READ_U8_REL(1) == 0) {
                    g_FieldActors[g_FieldCurActorIndex].rotation.y += FIELD_DOOR_STEP_ANGLE;
                } else {
                    g_FieldActors[g_FieldCurActorIndex].rotation.y -= FIELD_DOOR_STEP_ANGLE;
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
    
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
}

void FieldScriptVMHandlerStepActorRotationOff(void) {
    if (g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13) {
        if (!(g_FieldScriptVMCurActor->flags12C_0x5)) {
            g_FieldScriptVMCurActor->flags12C_0x5 = 1;
            g_FieldScriptVMCurActor->curDoorStep = 0;
            FieldSetSoundSlotElement(8, 3);
        } else {
            g_FieldScriptVMCurActor->curDoorStep++;
            if (g_FieldScriptVMCurActor->curDoorStep < FIELD_DOOR_STEP_COUNT) {
                if (SCRIPT_READ_U8_REL(1) == 0) {
                    g_FieldActors[g_FieldCurActorIndex].rotation.y -= FIELD_DOOR_STEP_ANGLE;
                } else {
                    g_FieldActors[g_FieldCurActorIndex].rotation.y += FIELD_DOOR_STEP_ANGLE;
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
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
}

extern void FieldActorApplyRotationAndScale(int);
extern void FieldSetSoundSlotElement(int,int);
extern s32 g_FieldCurActorIndex;
extern FieldActor* g_FieldScriptVMCurFieldActor;

void FieldScriptVMHandlerStepActorMoveOn() {
    ActorData* pActor;
    int angle;
    int delta;

    if (!g_FieldScriptVMCurActor->scriptFlags.fields.scriptFlags_0x13) {
        if (!g_FieldScriptVMCurActor->flags12C_0x5) {
            g_FieldScriptVMCurActor->flags12C_0x5 = 0x1;
            g_FieldScriptVMCurActor->curDoorStep = 0;
            FieldSetSoundSlotElement(8, 3);
            g_FieldScriptVMCurActor->moveStep.vx = g_FieldScriptVMCurActor->position.vx;
            g_FieldScriptVMCurActor->moveStep.vy = g_FieldScriptVMCurActor->position.vy;
            g_FieldScriptVMCurActor->moveStep.vz = g_FieldScriptVMCurActor->position.vz;
        } else {
            g_FieldScriptVMCurActor->curDoorStep++;
            if (g_FieldScriptVMCurActor->curDoorStep < FieldScriptVMGetArgument(3)) {
                switch (FieldScriptVMGetArgument(5)) { 
                    case FIELD_MOVE_DIRECTION_Y_DECREASE:
                        g_FieldScriptVMCurActor->moveStep.vy -= FieldScriptVMGetArgument(1) * 0x10;
                        g_FieldScriptVMCurFieldActor->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->moveStep.vz);
                        break;
                    case FIELD_MOVE_DIRECTION_Y_INCREASE:
                        g_FieldScriptVMCurActor->moveStep.vy += FieldScriptVMGetArgument(1) * 0x10;
                        g_FieldScriptVMCurFieldActor->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->moveStep.vz);
                        break;
                    default:
                        delta = PSX_DEGREES(90);
                        angle = g_FieldScriptVMCurFieldActor->rotation.y + FieldScriptVMGetArgument(5) - delta;
                        g_FieldScriptVMCurActor->moveStep.vx += rsin(angle) * FieldScriptVMGetArgument(1);
                        g_FieldScriptVMCurActor->moveStep.vz -= rcos(angle) * FieldScriptVMGetArgument(1);
                        g_FieldScriptVMCurFieldActor->transformMatrix.t[0] = CONV_TO_GTE(g_FieldScriptVMCurActor->moveStep.vx);
                        g_FieldScriptVMCurFieldActor->transformMatrix.t[2] = CONV_TO_GTE(g_FieldScriptVMCurActor->moveStep.vz);
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
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
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
                    case FIELD_MOVE_DIRECTION_Y_DECREASE:
                        g_FieldScriptVMCurActor->moveStep.vy -= FieldScriptVMGetArgument(1) * 0x10;
                        g_FieldScriptVMCurFieldActor->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->moveStep.vz);
                        break;
                    case FIELD_MOVE_DIRECTION_Y_INCREASE:
                        g_FieldScriptVMCurActor->moveStep.vy += FieldScriptVMGetArgument(1) * 0x10;
                        g_FieldScriptVMCurFieldActor->transformMatrix.t[1] = CONV_TO_GTE(g_FieldScriptVMCurActor->moveStep.vz);
                        break;
                    default:
                        delta = PSX_DEGREES(90);
                        angle = g_FieldScriptVMCurFieldActor->rotation.y + FieldScriptVMGetArgument(5) - delta;
                        g_FieldScriptVMCurActor->moveStep.vx -= rsin(angle) * FieldScriptVMGetArgument(1);
                        g_FieldScriptVMCurActor->moveStep.vz += rcos(angle) * FieldScriptVMGetArgument(1);
                        g_FieldScriptVMCurFieldActor->transformMatrix.t[0] = CONV_TO_GTE(g_FieldScriptVMCurActor->moveStep.vx);
                        g_FieldScriptVMCurFieldActor->transformMatrix.t[2] = CONV_TO_GTE(g_FieldScriptVMCurActor->moveStep.vz);
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
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
}

extern s32 g_FieldScriptWalkFrameCounter;
extern s32 g_FieldScriptWalkFlags;

void FieldScriptVMHandlerSetWalkPosition(void) {
    g_FieldScriptWalkFrameCounter = 0;
    g_FieldScriptWalkFlags = 0xFF;
    FieldScriptMemoryWriteU16(
        0xA, 
        ((FieldScriptVMGetArgument(1) << 8) & 0xFF00) | (FieldScriptVMGetArgument(3) & 0xFF)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptVMHandlerSetWalkFlags(void) {
    g_FieldScriptWalkFlags = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptVMHandlerStopWalking(void) {
    g_FieldScriptWalkFrameCounter = 0;
    g_FieldScriptWalkFlags = 0xFF;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMHandlerSetActorHideMode1(void) {
    g_FieldScriptVMCurActor->flags12C_0 = 0x1;
    g_FieldScriptVMCurActor->hideModeParam = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerSetActorHideMode2(void) {
    g_FieldScriptVMCurActor->flags12C_0 = 0x2;
    g_FieldScriptVMCurActor->hideModeParam = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptVMHandlerSetActorHideMode3(void) {
    g_FieldScriptVMCurActor->flags12C_0 = 0x3;
    g_FieldScriptVMCurActor->hideModeParam = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void FieldActorApplyRotationAndScale(int);

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
        FieldActorApplyRotationAndScale(FieldScriptVMGetActorIndex(2));
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

// Set X, Y or Z rotaiton of Actor g_FieldCurActorIndex
void FieldScriptVMHandlerSetCurrentActorRotation(void) {
    switch (SCRIPT_READ_U8_REL(3)) { 
        case 0:
            g_FieldActors[g_FieldCurActorIndex].rotation.x = FieldScriptVMGetArgument(1);
            break;
        case 1:
            g_FieldActors[g_FieldCurActorIndex].rotation.y = FieldScriptVMGetArgument(1);
            break;
        case 2:
            g_FieldActors[g_FieldCurActorIndex].rotation.z = FieldScriptVMGetArgument(1);
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
}

// Increase X Rotation of Actor g_FieldCurActorIndex
void func_80094A5C(void) {
    (&g_FieldActors[g_FieldCurActorIndex])->rotation.x += FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
}

// Decrease X Rotation of Actor g_FieldCurActorIndex
void FieldScriptVMHandlerSubCurrentActorRotX(void) {
    (&g_FieldActors[g_FieldCurActorIndex])->rotation.x -= FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
}

// Increase Y Rotation of Actor g_FieldCurActorIndex
void FieldScriptVMHandlerAddCurrentActorRotY(void) {
    (&g_FieldActors[g_FieldCurActorIndex])->rotation.y += FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
}

// Decrease Y Rotation of Actor g_FieldCurActorIndex
void FieldScriptVMHandlerSubCurrentActorRotY(void) {
    (&g_FieldActors[g_FieldCurActorIndex])->rotation.y -= FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
}

// Increase Z Rotation of Actor g_FieldCurActorIndex
void FieldScriptVMHandlerAddCurrentActorRotZ(void) {
    (&g_FieldActors[g_FieldCurActorIndex])->rotation.z += FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
}

// Decrease Z Rotation of Actor g_FieldCurActorIndex
void FieldScriptVMHandlerSubCurrentActorRotZ(void) {
    (&g_FieldActors[g_FieldCurActorIndex])->rotation.z -= FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
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
        case FIELD_INVENTORY_KIND_ITEM:
            return g_pGameState->itemIDs;
        case FIELD_INVENTORY_KIND_WEAPON:
            return g_pGameState->weaponIDs;
        case FIELD_INVENTORY_KIND_ACCESSORY:
            return g_pGameState->accessoryIDs;
        case FIELD_INVENTORY_KIND_GEAR_WEAPON:
            return g_pGameState->gearWeaponsIds;
        case FIELD_INVENTORY_KIND_GEAR_PARTS:
            return g_pGameState->gearPartsIds;
    }
    return 0;
}

// Get the array of inventory entry quantities for the given inventory kind
u_char* FieldInventoryGetQuantitiesArray(int itemId) {
    switch (itemId >> 8) {
        case FIELD_INVENTORY_KIND_ITEM:
            return g_pGameState->itemQuantities;
        case FIELD_INVENTORY_KIND_WEAPON:
            return g_pGameState->weaponQuantities;
        case FIELD_INVENTORY_KIND_ACCESSORY:
            return g_pGameState->accessoryQuantities;
        case FIELD_INVENTORY_KIND_GEAR_WEAPON:
            return g_pGameState->gearWeaponsQty;
        case FIELD_INVENTORY_KIND_GEAR_PARTS:
            return g_pGameState->gearPartsQty;
    }
    return 0;
}

// Find the inventory slot of the given kind holding the given ID
s32 FieldInventoryFindSlotById(s32 itemId) {
    switch (itemId >> 8) {
        case FIELD_INVENTORY_KIND_ITEM:
            return FieldInventoryFindItemSlot(itemId);
        case FIELD_INVENTORY_KIND_WEAPON:
            return FieldInventoryFindWeaponSlot(itemId - (FIELD_INVENTORY_KIND_WEAPON << 8));
        case FIELD_INVENTORY_KIND_ACCESSORY:
            return FieldInventoryFindAccessorySlot(itemId - (FIELD_INVENTORY_KIND_ACCESSORY << 8));
        case FIELD_INVENTORY_KIND_GEAR_WEAPON:
            return FieldInventoryFindGearWeaponSlot(itemId - (FIELD_INVENTORY_KIND_GEAR_WEAPON << 8));
        case FIELD_INVENTORY_KIND_GEAR_PARTS:
            return FieldInventoryFindGearPartsSlot(itemId - (FIELD_INVENTORY_KIND_GEAR_PARTS << 8));
    }
    return 0;
}

// Find the first unused inventory slot of the given kind
s32 FieldInventoryFindEmptySlotById(s32 itemId) {
    switch (itemId >> 8) {
        case FIELD_INVENTORY_KIND_ITEM:
            return FieldInventoryFindEmptyItemSlot();
        case FIELD_INVENTORY_KIND_WEAPON:
            return FieldInventoryFindEmptyWeaponSlot();
        case FIELD_INVENTORY_KIND_ACCESSORY:
            return FieldInventoryFindEmptyAccessorySlot();
        case FIELD_INVENTORY_KIND_GEAR_WEAPON:
            return FieldInventoryFindEmptyGearWeaponSlot();
        case FIELD_INVENTORY_KIND_GEAR_PARTS:
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
    SpriteData* pSpriteData = g_FieldActors[g_FieldCurActorIndex].pSpriteData;
    ActorData* pActor = g_FieldScriptVMCurActor;
    u_short rotation = pActor->rotation.vx | 0x8000;

    g_FieldScriptVMStopFlag = 1;
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
    g_FieldControl.angle = FieldScriptVMGetArgument(1);
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
        if (g_FieldScriptVMCurActor->flags12C_0x6 != SCRIPT_MAX_STACK_SIZE) {
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
            if (g_FieldScriptVMCurActor->flags12C_0x6 != SCRIPT_MAX_STACK_SIZE) {
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

// MATCH (192 bytes / 48 insns, retail's exact size; check_bin OK). objdiff reads 99.79 because
// the one %hi/%lo pair is `g_Scene + 0x180` here and `g_FieldActors` in splat's expected asm:
// both name 0x800AFB10, so the linked bytes are identical. The base has to survive copy
// propagation for retail's shape (`la base` / `addiu a0,base,-0xAC` / `lw a1,0(base)`): a bare
// symbol base (`&g_FieldActors`) always folds the -0xAC into one %lo, and a struct-typed
// pointer variable folds into two independent pairs - only a struct-member address reached
// through a (u8*) cast stays in a register. misc8's FieldActorWorldToScreenPosition (parked
// 95.25) is the same function and should take this same spelling; untested there.
void FieldProjectActorOriginToScreen(int* pScreenX, int* pScreenY) {
    SVECTOR origin;
    MATRIX transformMatrix;
    long sxy;
    long p;
    long flag;
    FieldActor* volatile* pActorList;

    // The actor list pointer sits 0xAC bytes after g_Scene.worldToScreenMatrix
    // (0x800AFB10 = the g_FieldActors symbol); retail anchors both on that one base.
    pActorList = (FieldActor* volatile*)((u8*)&g_Scene.worldToScreenMatrix + 0xAC);
    CompMatrix((MATRIX*)((u8*)pActorList - 0xAC), &(*pActorList)[FieldScriptVMGetActorIndexOrDefault(1)].childMatrix, &transformMatrix);
    origin.vx = 0;
    origin.vy = 0;
    origin.vz = 0;
    SetRotMatrix(&transformMatrix);
    SetTransMatrix(&transformMatrix);
    RotTransPers(&origin, &sxy, &p, &flag);
    *pScreenY = ((short*)&sxy)[1];
    *pScreenX = ((short*)&sxy)[0];
}



extern s32 g_FieldSkipCounter;

void FieldScriptCheckActorOnScreenWithMargin(void) {
    int screenX;
    int screenY;

    FieldProjectActorOriginToScreen(&screenX, &screenY);
    if (g_FieldSkipCounter != 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        return;
    }
    
    if ((screenY - 33) < 159U && (screenX - 33) < 255U) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    }
    
    g_FieldScriptVMStopFlag = 1;
}

void FieldScriptCheckActorOnScreen(void) {
    int screenX;
    int screenY;

    FieldProjectActorOriginToScreen(&screenX, &screenY);
    if (g_FieldSkipCounter != 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        return;
    }
    
    if ((screenY - 1) < (SCREEN_HEIGHT - 1) && (screenX - 1) < (SCREEN_WIDTH - 1)) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    }
    
    g_FieldScriptVMStopFlag = 1;
}

// best 96.19% (172 bytes, retail 168 / 42 insns, mine 43); re-measured round 470: 96.19 again with the call inlined into the compare (no `arg` local), and 96.19 under gcc-2.6.0 too (try_flags --all --gcc gcc-2.6.0-psx, where the unit's matched neighbour func_800925A0 drops to 93.53, so the preset is not the answer); re-measured round 470: the same 96.19 twice more, with the call written inline in the compare (no `arg` local) as well: documented blocker, levers.md [c-a-call-argument-constant-folds-early-when-the-compare-load-sinks]. Instruction multiset is otherwise EXACT - the whole body, registers and branch layout match. Only residual: cc1 emits the second call's argument constant `li $4,2` NINE insns early (right after the second FieldScriptVMGetActorIndex call, before the sll chain), so reorg cannot steal it into the jal slot and the slot gets a nop = the one extra instruction. Retail has it adjacent to the jal, so its slot holds `ori a0,2`. Measured: (1) plain `arg = GetArgument(2); if (arg == pActor->walkmeshId)` -> 96.19; (2) hoisted `int walkmeshId = pActor->walkmeshId;` local -> compiles BYTE-IDENTICALLY (copy-propagated), 96.19 - so statement order of the load does not decide the li site; (3) `FieldActor* pFieldActor = &g_FieldActors[...]` pointer local -> addu operand order flips (95.71); (4) negated test with arms swapped -> byte-identical to (1), 96.19. gcc-2.7.2-cdk -O2 -G8 also 96.19 (try_flags). The matched neighbour FieldScriptCheckActorWalkmeshMaterial has THIS exact `jal; ori a0,2` slot, so the shape is reachable in this unit - the difference there is that the compared value (lbu through the triangle table) sits before the call rather than after it. Not permuted: below the 99% permuter gate.
/*
void func_80095CC4(void) {
    ActorData* pActor;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        if (FieldScriptVMGetArgument(2) != pActor->walkmeshId) {
            g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
        } else {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
        }
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc11", func_80095CC4);





typedef struct {
    /* 0x0 */ undefined8 unk0[0xC];
    /* 0xC */ u8 material;
    /* 0xD */ undefined8 padD;
} FieldWalkmeshTriangle; // Size: 0xE

extern FieldWalkmeshTriangle* g_FieldWalkmeshTriangles[];

void FieldScriptCheckActorWalkmeshMaterial(void) {
    ActorData* pActor;
    u8 material;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        pActor = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData;
        material = g_FieldWalkmeshTriangles[pActor->walkmeshId][pActor->walkmeshTriIds[pActor->walkmeshId]].material;
        if (FieldScriptVMGetArgument(2) == material) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
            return;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
}

extern FieldActor* g_FieldScriptVMCurFieldActor;

void FieldScriptCheckActorDistance(void) {
    ActorData* pActor;
    ActorData* pActorOther;
    FieldActor* pFieldActor;
    int distance;
    int actorIndex;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pFieldActor = &g_FieldActors[FieldScriptVMGetActorIndex(1)];
        
        pActor = g_FieldScriptVMCurFieldActor->pActorData;
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