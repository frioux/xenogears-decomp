#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/actor.h"
#include "field/script_vm.h"
#include "field/text_box.h"
#include "field/particles.h"
#include "field/effects.h"

void FieldScriptMemoryWriteU16(int, int);

extern int rcos(int);
extern int rsin(int);

extern s32 D_800AFD1C;
extern s32 D_8005A444[MAX_PARTY_MEMBERS];

void FieldSetScreenDimensions(void) {
    g_FieldRenderContexts[0].dispEnv.screen.x = 0;
    g_FieldRenderContexts[0].dispEnv.screen.y = 10;
    g_FieldRenderContexts[0].dispEnv.screen.w = 0x100;
    g_FieldRenderContexts[0].dispEnv.screen.h = 0xd8;
    g_FieldRenderContexts[1].dispEnv.screen.x = 0;
    g_FieldRenderContexts[1].dispEnv.screen.y = 10;
    g_FieldRenderContexts[1].dispEnv.screen.w = 0x100;
    g_FieldRenderContexts[1].dispEnv.screen.h = 0xd8;
}

extern u8 D_800B2358[]; // Is pause disabled?

void FieldScriptSetPauseDisabled(void) {
    D_800B2358[0] = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s16 D_800ADB54;

void FieldScriptSetScreenMode(void) {
    RECT rect;
    switch (FieldScriptVMGetArgument(1)) {
        case 0:
            rect.w = 0x500;
            rect.x = 0;
            rect.y = 0;
            rect.h = 0x200;
            ClearImage(&rect, 0x0, 0x0, 0x0);
            DrawSync(0);
            Vsync(0);
            SetDefDrawEnv(g_FieldRenderContexts[0].drawEnvs, 0, 0, 0x280, 0xE0);
            SetDefDrawEnv(g_FieldRenderContexts[1].drawEnvs, 0, 0x100, 0x280, 0xE0);
            SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x280, 0xE0);
            SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x280, 0xE0);
            FieldSetScreenDimensions();
            break;
        case 1:
            D_800ADB54 = 0x1;
            break;
        case 2:
            D_800ADB54 = 0x0;
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Set model animation for actor
void FieldScriptSetModelAnimation(void) {
    g_FieldScriptVMCurActor->modelAnimation = (FieldScriptVMGetArgument(1) << 0xC) | FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptHandleSpriteOperations(void) {
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            // Initialize Sprite List
            func_800AAC08();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            return;
        case 2:
            // Free Sprite List
            func_800AABD8();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            return;
        case 1:
            // Set X, Y of Sprite and add it to OT for drawing
            func_800AAE4C(
                FieldScriptVMGetArgument(2), // Sprite Index
                FieldScriptVMGetArgument(4), // X
                FieldScriptVMGetArgument(6), // Y
                FieldScriptVMGetArgument(8)  // Sprite type
            );
            g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
            return;
        case 3:
            // Set Sprite Color
            func_800AADC8(
                FieldScriptVMGetArgument(2), // Sprite Index
                FieldScriptVMGetArgument(4), // Red
                FieldScriptVMGetArgument(6), // Green
                FieldScriptVMGetArgument(8)  // Blue
            );
            g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
            return;
    }
}

void FieldScriptSetGearStateFlag(void) {
    int index = FieldScriptVMGetArgument(1);
    int value = FieldScriptVMGetArgument(3);

    g_pGameState->unk16BC[index].unk1E |= value;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

// 97.81% (532 bytes, same instruction multiset). Remaining differences: the four
// rect stores go through D_800AFC58..D_800AFC5E as four separate s16 globals in the
// original (a RECT-typed D_800AFC58 gives addends 0/2/4/6 against one symbol), the
// original reuses the StoreImage argument register for the offset-0 store
// (`sh zero,0(a0)`) where ours re-forms `lui at`, and case 1's shift is emitted
// after the second GetArgument call in the original but folded into its delay slot
// by ours. Splitting case 1's shift onto its own statement costs 4 bytes (95.55%).
// NOTE the command dispatch must be a switch: an if/else chain compiles to a linear
// compare chain (77.68%) and the original has gcc's comparison tree. It does not
// generate a jump table (no jtbl_ relocation, 532 bytes).
/*
extern s32 D_800C3A48;
extern s32 D_800AF87C;
extern s16 D_800AFC58;
extern s16 D_800AFC5A;
extern s16 D_800AFC5C;
extern s16 D_800AFC5E;
extern s32 D_800ADBB4;
extern void func_80026F44(s32 count, s32 level, u16* pDst, u16* pSrc);

// Captures or restores the screen region described by the script instruction: 0
// grabs it into two freshly allocated buffers, 1 copies one back with
// func_80026F44, 2 frees the buffers and 3 only advances the pointer.
void func_800871B0(void) {
    u8 command = SCRIPT_READ_U8_REL(1);

    switch (command) {
    case 0: {
        s32 h;
        s32 y;

        g_FieldScriptMaxInstructionCount += 0x20;
        y = FieldScriptVMGetArgument(2);
        h = FieldScriptVMGetArgument(4);
        D_800C3A48 = HeapAlloc(h << 9, 0);
        D_800AF87C = HeapAlloc(h << 9, 0);
        D_800AFC58 = 0;
        D_800AFC5A = y;
        D_800AFC5C = 0x100;
        D_800AFC5E = h;
        StoreImage((RECT*)&D_800AFC58, (void*)D_800C3A48);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
        D_800B00C0 = 1;
        break;
    }
    case 1: {
        s32 offset;

        g_FieldScriptMaxInstructionCount += 0x20;
        offset = FieldScriptVMGetArgument(2) << 9;
        func_80026F44(0x100, FieldScriptVMGetArgument(4), (u16*)(D_800AF87C + offset),
                      (u16*)(D_800C3A48 + offset));
        D_800ADBB4 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
        break;
    }
    case 2:
        HeapFree((void*)D_800C3A48);
        HeapFree((void*)D_800AF87C);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        D_800B00C0 = 1;
        break;
    case 3:
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        D_800B00C0 = 1;
        break;
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800871B0);

extern u8 D_800B225F[];

void FieldScriptSetCharacterByte(void) {
    int characterId = FieldScriptVMGetArgument(1);
    D_800B225F[characterId] = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptWriteScaledU16Pair(void) {
    int arg1, arg2, arg3, arg4, arg5, arg6;
    int value1, value2;

    arg1 = FieldScriptVMGetArgument(ARG(1));
    arg2 = FieldScriptVMGetArgument(ARG(2));
    arg3 = FieldScriptVMGetArgument(ARG(3));
    arg4 = FieldScriptVMGetArgument(ARG(4));
    arg5 = FieldScriptVMGetArgument(ARG(5));
    arg6 = FieldScriptVMGetArgument(ARG(6));

    value1 = CONV_TO_GTE(
        CONV_FROM_GTE(arg5) / arg3 * arg1
    );
    value2 = CONV_TO_GTE(
        CONV_FROM_GTE(arg6) / arg4 * arg2
    );
    
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(7), 
        value1
    );
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(8), 
        value2
    );
    
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x11;
}

void FieldScriptNoOp3(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetGameStateFlag4000(void) {
    g_pGameState->unk22B6 |= 0x4000;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptCopyGear(void) {
    int sourceGearIndex = FieldScriptVMGetArgument(1);
    g_pGameState->gears[FieldScriptVMGetArgument(3)] = g_pGameState->gears[sourceGearIndex];
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

// Copies a character record (0xA4, word aligned) and the 0x20 bytes that start 4
// bytes into unk16BC[] from the same slot of the first argument.
// 93.49%: same size (440 bytes), same instruction multiset. Only ordering differs -
// the original emits `sll v1,t1,0x5` before the `lui/lw` of g_pGameState and hoists
// that load above the `bne` for destIndex == 9. Pointer locals for the 0x20-byte copy
// add 8 bytes (91.95%), a plain `characters[i] = characters[j]` assignment loses the
// word alignment and compiles to an alignment test plus two loops (62.52%).
/*
typedef struct {
    u32 words[0x29];
} CharacterRecordCopy;

typedef struct {
    u8 bytes[0x20];
} UnkGearStateShiftCopy;

void func_8008764C(void) {
    int sourceIndex = FieldScriptVMGetArgument(1);
    int destIndex = FieldScriptVMGetArgument(3);

    *(CharacterRecordCopy*)&g_pGameState->characters[destIndex] =
        *(CharacterRecordCopy*)&g_pGameState->characters[sourceIndex];
    *(UnkGearStateShiftCopy*)((u8*)&g_pGameState->unk16BC[destIndex] + 4) =
        *(UnkGearStateShiftCopy*)((u8*)&g_pGameState->unk16BC[sourceIndex] + 4);
    if (destIndex == 9) {
        g_pGameState->unk22B6 |= 0x2000;
    }
    if (destIndex == 0xA) {
        g_pGameState->unk22B6 |= 0x1000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008764C);

extern u8 D_80050622;

void FieldScriptWriteTransitionState(void) {
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(1), 
        D_80050622
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Globals written directly by field script handlers. Declaring them as struct
// members (rather than bare scalars) keeps gcc from hoisting the actor's
// instruction pointer load above the store.
typedef struct { s16 value; } FieldScriptGlobalS16;
typedef struct { u16 value; } FieldScriptGlobalU16;
typedef struct { s32 value; } FieldScriptGlobalS32;
typedef struct { u8 value; } FieldScriptGlobalU8;

extern FieldScriptGlobalS32 D_800ADBDC;
extern int D_800ADB2C;
extern s32 D_8004F308;
extern s32 D_800ADB88;
extern s32 D_800ADBE8;
extern u8 D_8005061C;
extern u8 D_8005061D;
extern u8 D_8005061E;
extern u8 D_8005061F;
extern u8 D_80050620;
extern u8 D_80050621;
extern void func_800379B4(int);

void FieldScriptSetTransitionParams(void) {
    if (D_800ADBDC.value == 0 || D_800ADBE4 == 0 || D_800ADB2C != 0 || D_8004F308 == -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        D_800B00C0 = 1;
    } else {
        func_800379B4(0);
        D_8005061C = FieldScriptVMGetArgument(1);
        D_8005061D = FieldScriptVMGetArgument(3);
        D_8005061E = FieldScriptVMGetArgument(5);
        D_8005061F = FieldScriptVMGetArgument(7);
        D_80050620 = FieldScriptVMGetArgument(9);
        D_80050621 = FieldScriptVMGetArgument(0xB);
        D_800ADB88 = 1;
        D_800ADBE8 = 0;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0xD;
    }
}

void FieldScriptWriteGameState17FC48(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u_short*)&g_pGameState->unk17FC[0x48]);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), *(u_short*)&g_pGameState->unk17FC[0x4A]);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptWriteGameState184E(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_pGameState->unk184E);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), g_pGameState->unk1852);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern FieldScriptGlobalU8 D_800B2357;

void FieldScriptSetByte2357(void) {
    D_800B2357.value = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern FieldScriptGlobalU8 D_800B2354;

void FieldScriptSetByte2354(void) {
    D_800B2354.value = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetGameState184E(void) {
    g_pGameState->unk184E = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    g_pGameState->unk1852 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    g_pGameState->unk1854 = 0;
    g_pGameState->unk1850 = 0;
    g_pGameState->unk1856 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptWriteGameState17FC30(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u_short*)&g_pGameState->unk17FC[0x30]);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), *(u_short*)&g_pGameState->unk17FC[0x32]);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), *(u_short*)&g_pGameState->unk17FC[0x34]);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(4), *(u_short*)&g_pGameState->unk17FC[0x36]);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

extern s32 D_8004F300;

void FieldScriptSetEffectFlag4F300(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    D_8004F300 = 1;
}

void FieldScriptSetGameState17FC30(void) {
    ActorData* pActor;
    s16 value;

    value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    pActor = g_FieldScriptVMCurActor;
    *(u_short*)&g_pGameState->unk17FC[0x30] = value;
    value = FieldScriptArgument2(3, SCRIPT_READ_U8(pActor->scriptInstructionPointer + 9));
    pActor = g_FieldScriptVMCurActor;
    *(u_short*)&g_pGameState->unk17FC[0x32] = value;
    value = FieldScriptArgument3(5, SCRIPT_READ_U8(pActor->scriptInstructionPointer + 9));
    pActor = g_FieldScriptVMCurActor;
    *(u_short*)&g_pGameState->unk17FC[0x34] = value;
    value = FieldScriptArgument4(7, SCRIPT_READ_U8(pActor->scriptInstructionPointer + 9));
    pActor = g_FieldScriptVMCurActor;
    *(u_short*)&g_pGameState->unk17FC[0x36] = value;
    pActor->scriptInstructionPointer += 0xA;
}

void FieldScriptWriteGameState17FC38(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u_short*)&g_pGameState->unk17FC[0x38]);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetGameState17FC38(void) {
    int value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    ActorData* pActor = g_FieldScriptVMCurActor;

    *(u_short*)&g_pGameState->unk17FC[0x38] = value;
    pActor->scriptInstructionPointer += 4;
}

extern FieldScriptGlobalU8 D_800B2355;
extern FieldScriptGlobalU8 D_800B2356;

void FieldScriptSetByte2355Or2356(void) {
    int value = FieldScriptVMGetArgument(2);

    if (SCRIPT_READ_U8_REL(1) == 0) {
        D_800B2355.value = value;
    } else {
        D_800B2356.value = value;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

extern FieldScriptGlobalS16 D_800B234C;

void FieldScriptSetByte234C(void) {
    D_800B234C.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800ADBFC;
extern s32 g_PlayerActorIndex;
extern FieldScriptGlobalU16 D_800B234E;
extern FieldScriptGlobalS16 D_800B233E;

// Switch the actor controlled by the player
void FieldScriptSetPlayerActor(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);

    if (actorIndex != ACTOR_ID_INVALID) {
        int i;

        if (actorIndex == D_8005A444[0]) {
            D_800B234E.value = 0;
        } else {
            D_800B234E.value = 1;
        }
        g_PlayerActorIndex = actorIndex;
        D_800B233E.value = actorIndex;
        for (i = 0; i < D_800ADBFC; i++) {
            g_FieldActors[i].pActorData->scriptFlags.flags &= ~0x01004000;
        }
        g_FieldActors[actorIndex].pActorData->scriptFlags.flags |= 0x4000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern FieldScriptGlobalU16 D_800B2348;

// 79.83%: gcc folds the symbol into both accesses (`lui/lhu` then `lui at/sh`),
// the original materialises &D_800B2348 once in $v1 and uses 0($v1) twice. A
// local pointer to the global (`p->value++`, also `+= 1`, `volatile` variants)
// is folded back to the same immediate form. Also 19.92% with a cast pointer
// (`u16* p = (u16*)&D_800B2348; (*p)++;`): gcc still folds and allocates worse.
// The `lui+addiu` form appears where the C uses the symbol's address as a VALUE
// (cf. func_8008861C's `$a2 = &g_FieldDefaultParticleBanks`), and a deref of
// `&sym` never survives fold, so a pointer local is not enough.
// preset screen: 2.7.2 79.83%, cdk 79.83%, 2.6.0 79.83% - no compiler
// fixes it.
/*
void func_80087FA4(void) {
    D_800B2348.value++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80087FA4);

void FieldScriptRebuildTileBuffers(void) {
    func_800A8BA4();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern void LibGearGetJointMatrix(MATRIX*, MATRIX*, int, int);
extern void SetRotMatrix(MATRIX*);
extern void SetTransMatrix(MATRIX*);

// Rotates the actor's model by the script angles and writes the position back.
void FieldScriptTransformPointByGearModelMatrix(void) {
    MATRIX matrix;
    MATRIX transformMatrix;
    SVECTOR rotation;
    SVECTOR result;
    long flag;

    matrix.t[2] = 0;
    matrix.t[1] = 0;
    matrix.t[0] = 0;
    LibGearGetJointMatrix(
        &matrix,
        &transformMatrix,
        FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xB)),
        FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xB))
    );
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);
    rotation.vx = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xB));
    rotation.vy = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xB));
    rotation.vz = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xB));
    RotTransSV(&rotation, &result, &flag);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(6), result.vx);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(7), result.vy);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG_ALIGNED(8), result.vz);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}

void FieldRestoreAllGearHpAndFuel(void) {
    int i;

    for (i = 0; i < MAX_GAME_GEARS; i++) {
        g_pGameState->gears[i].hp = g_pGameState->gears[i].maxHp;
        g_pGameState->gears[i].fuel = g_pGameState->gears[i].maxFuel;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptStoreOrRestoreScreenImage(void) {
    if (SCRIPT_READ_U8_REL(1) == 0) {
        FieldStoreScreenImage();
    } else {
        FieldRestoreScreenImage();
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s32 D_8004F308;

void FieldScriptWaitForDataLoadPending(void) {
    if (D_8004F308 == -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
    D_800B00C0 = 1;
}

void FieldScriptWriteCharacterGearId(void) {
    int partyId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(1));

    if (partyId != CHARACTER_ID_NONE) {
        FieldScriptMemoryWriteU16(
            FieldScriptVMGetInstructionArgument(3) & 0xFFFF, g_pGameState->characters[partyId].gearId);
    } else {
        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, 0xFF);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptSetCharacterGear(void) {
    int characterId = FieldScriptVMGetArgument(1);
    g_pGameState->characters[characterId].gearId = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptSetCharacterBit2318(void) {
    int partyId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(2));

    if (partyId != CHARACTER_ID_NONE) {
        if (SCRIPT_READ_U8_REL(1) == 0) {
            ((u_short*)&g_pGameState->unk22B6)[0x31] |= 1 << partyId;
        } else {
            ((u_short*)&g_pGameState->unk22B6)[0x31] &= ~(1 << partyId);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

extern FieldScriptGlobalS16 D_800B236C;

// 100.0% under gcc 2.6.0-psx, 78.75% under this unit's 2.7.2: it needs a
// per-file preset, and this unit cannot be split yet - it declares several names
// twice with different types mid-file (PartyDataChunk, FieldRandomValues), so
// copying the typedefs into a new TU does not compile.
/*
void func_8008848C(void) {
    int value = SCRIPT_READ_U8_REL(1);

    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    D_800B236C.value = value ^ 1;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008848C);

extern FieldScriptGlobalS16 D_800B21AC;

void FieldScriptSetByte21AC(void) {
    D_800B21AC.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// 97.68%: instruction for instruction identical except that gcc's allocator puts
// the `li a0,1` for the first FieldScriptVMGetInstructionArgument call at the top
// of the then-block instead of just before the jal, so reorg leaves a nop in the
// delay slot. Inline vs. `address` local makes no difference. Same instruction
// count either way (280 bytes): only those two slots differ. Casting the
// pSpriteData load to `SpriteData* volatile*` does not stop the hoist.
// preset screen: 2.7.2 97.68%, cdk 84.20%, 2.6.0 97.68% - no compiler
// fixes it.
/*
void func_80088508(void) {
    SpriteData* pSpriteData;
    int partyMemberId;
    int address;

    partyMemberId = D_8005A444[FieldScriptVMGetArgument(5)];
    g_FieldScriptMaxInstructionCount += 4;
    if (partyMemberId != ACTOR_ID_INVALID) {
        pSpriteData = g_FieldActors[partyMemberId].pSpriteData;
        address = FieldScriptVMGetInstructionArgument(1);
        FieldScriptMemoryWriteU16(address & 0xFFFF, *(u_short*)&((SpriteDataI3*)pSpriteData->field_0x7C)->field_0xC);
        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, partyMemberId);
        if (*(u_short*)&((SpriteDataI3*)pSpriteData->field_0x7C)->field_0xC != 1) {
            ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0xC = 0;
        }
    } else {
        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, 0);
        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, 0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_80088508);

typedef struct {
    /* 0x0 */ int bankIndex;
    /* 0x4 */ u8 unk4[0xC];
} ParticleBankHandle;

extern ParticleBankHandle D_800B2384;

// 92.50%: the single bankIndex load, &g_FieldDefaultParticleBanks materialised
// in $a2 and the .x store through %hi/%lo(g_FieldDefaultParticleBanks + 0x30)
// all match. Ours spends one extra `addu`: gcc builds
// (&banks + 0x78*bankIndex) as an intermediate and then adds 4*i, where the
// original shares the 4*i + 0x78*bankIndex sum with the .x store. A u8* base
// with an explicit byte offset gets the sum right (and the loop test moves
// after both stores), a ParticleBank* base gets the store order right. Best:
// 92.50% with
//   ParticleBank* pBank = &g_FieldDefaultParticleBanks[bankIndex];
//   pBank->directions[i].z = 0;
//   g_FieldDefaultParticleBanks[bankIndex].directions[i].x = 0;
// Byte-offset forms (`u8* pBanks` + `int offset = i * 4 + bank * 0x78`)
// and a flat element index `bank * 30 + i` both give 87.05% (the bank
// term and the 4*i term stop being shared).
// `ParticleBank* pBanks = g_FieldDefaultParticleBanks;` (pointer to the array
// start, index shared) gives 82.27%: the association comes out (base + 0x78*bank)
// + 4*i, one addu more than the original's (4*i + 0x78*bank) + base.
// preset screen: 2.7.2 92.50%, cdk 66.55%, 2.6.0 92.50% - no compiler
// fixes it.
/*
void func_8008861C(void) {
    ParticleBank* pBank;
    int i;

    for (i = 0; i < NUM_PARTICLE_BANKS; i++) {
        int bankIndex = D_800B2384.bankIndex;

        pBank = &g_FieldDefaultParticleBanks[bankIndex];
        pBank->directions[i].z = 0;
        g_FieldDefaultParticleBanks[bankIndex].directions[i].x = 0;
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008861C);

extern s32 D_800B2374;
extern s32 D_800B2378;
extern s32 D_800B237C;
extern s32 D_800B2380;
extern void FieldInitializeDefaultParticleBanks(int);

void FieldScriptInitializeParticleSystem(void) {
    int actorIndex = FieldScriptVMGetArgument(1);

    if (actorIndex == ACTOR_ID_INVALID) {
        actorIndex = 0;
    }
    D_800B2374 = FieldScriptVMGetArgument(1);
    D_800B2378 = FieldScriptVMGetArgument(3);
    D_800B237C = FieldScriptVMGetArgument(5);
    ((FieldScriptGlobalS32*)&D_800B2380)->value = FieldScriptVMGetArgument(7);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
    FieldInitializeDefaultParticleBanks(actorIndex);
    switch (D_800B2378) {
        case 0:
            D_800B2378 = 0;
            break;
        case 1:
            D_800B2378 = 0x10;
            break;
        case 2:
            D_800B2378 = 0x20;
            break;
        case 3:
            D_800B2378 = 0x30;
            break;
    }
    g_FieldScriptMaxInstructionCount += 4;
}

void FieldScriptInitializeParticleSystemByActorIndex(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);

    if (actorIndex == ACTOR_ID_INVALID) {
        actorIndex = 0;
    }
    D_800B2374 = actorIndex;
    D_800B2378 = FieldScriptVMGetArgument(2);
    D_800B237C = FieldScriptVMGetArgument(4);
    // Store through FieldScriptGlobalS32: a bare s32 store lets gcc hoist the
    // instruction pointer load above it (see the note above FieldScriptGlobalS32).
    ((FieldScriptGlobalS32*)&D_800B2380)->value = FieldScriptVMGetArgument(6);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    FieldInitializeDefaultParticleBanks(actorIndex);
    switch (D_800B2378) {
        case 0:
            D_800B2378 = 0;
            break;
        case 1:
            D_800B2378 = 0x10;
            break;
        case 2:
            D_800B2378 = 0x20;
            break;
        case 3:
            D_800B2378 = 0x30;
            break;
    }
    g_FieldScriptMaxInstructionCount += 4;
}

// 81.13%: the struct fields are now right (ActorData flags12C_0x12 is the 10-bit
// field at bit 18, so the mask is the original's 0xF003FFFF), but two misses
// remain. (1) The original keeps `type` in $s1 and shifts it by 6 into a temp for
// the sprite store, then by 24 for the bitfield (`sll s1,s1,0x18`); gcc 2.7.2 here
// CSEs `type << 6` into $s1 first and emits `sll s1,s1,0x12` after it. The
// original's single `<< 24` looks like source-level `(flags & 0xF003FFFF) |
// (type << 24)` rather than a `type << 6` bitfield store. (2) `li a0,1` for the
// first FieldScriptVMGetArgument is hoisted above the frame setup instead of
// sitting in the jal delay slot.
// preset screen: 2.7.2 81.13%, cdk 81.13%, 2.6.0 81.13% - no compiler
// fixes it.
/*
extern void func_8002303C(SpriteData*, int, int);

void func_800888A4(void) {
    SpriteData* pSpriteData;
    int type;
    int value;

    pSpriteData = g_FieldActors[D_800AFD1C].pSpriteData;
    type = FieldScriptVMGetArgument(1) & 0xF;
    value = ((FieldScriptVMGetArgument(1) >> 4) << 8) + FieldScriptVMGetArgument(3);
    func_8002303C(pSpriteData, 2, 0);
    ((u_short*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)[2] = type << 6;
    g_FieldScriptVMCurActor->flags12C_0x12 = type << 6;
    ((u_short*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)[3] = value;
    g_FieldScriptVMCurActor->flags130 = (g_FieldScriptVMCurActor->flags130 & ~0x1FF) | (value & 0x1FF);
    g_FieldScriptVMCurActor->flags12C_0x10 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800888A4);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800889BC);

// Start of particle handlers
extern s32 D_800ADB40;
extern s32 D_800ADB8C;

extern s32 g_FieldScriptMaxInstructionCount;

void FieldScriptSetParticleBankFlag(void) {
    int flags = 0;

    switch (FieldScriptVMGetArgument(1)) {
        case 1:
            flags = 0x80;
            break;
        case 2:
            flags = 0x40;
            break;
    }
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].flags |= flags;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void FieldScriptSetParticleBankMotion(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].speedMultiplier = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].flags |= FieldScriptVMGetArgument(3) << 8;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].rotAngle = FieldScriptVMGetArgument(5);
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void FieldScriptSetParticleBankDirections0() {
    FieldScriptSetParticleBankDirections(0);
}

void FieldScriptSetParticleBankDirections4() {
    FieldScriptSetParticleBankDirections(4);
}

void FieldScriptSetParticleBankDirections(int index) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index].x = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index].z = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 1].x = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 1].z = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 2].x = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 2].z = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 3].x = FieldScriptArgument7(0xD, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].directions[index + 3].z = FieldScriptArgument8(0xF, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}

void FieldScriptInitializeParticleBank(void) {
    D_800B2384.bankIndex = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].speedMultiplier = 1;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].targetActorID = D_800B2374;
    D_800ADB40 = g_FieldDefaultParticleBanks[D_800B2384.bankIndex].targetActorID;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].unk0 = 0;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].rotAngle = 0;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].max = FieldScriptVMGetArgument(3);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].swait = FieldScriptVMGetArgument(5);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].ewait = FieldScriptVMGetArgument(7);
    func_8008861C();
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptSetParticleBankPosition(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pos.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pos.vy  = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pos.vz = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].epos.vx = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].epos.vy = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].epos.vz = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptSetParticleBankPhysics(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].speed = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].gravity.vx = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].gravity.vy = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].gravity.vz = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].srange = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].erange = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptSetParticleBankParameters(void) {
    int flags;
    int flags_2;

    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pswait = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].pewait = FieldScriptVMGetArgument(3);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].shape = FieldScriptVMGetArgument(5);
    flags = FieldScriptVMGetArgument(7);
    flags_2 = (FieldScriptVMGetArgument(9) * 2);
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].flags = flags | flags_2 | D_800B2378;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].unk72 = D_800B237C;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].unk74 = D_800B2380;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}

void FieldScriptSetParticleBankScale(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scale.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scale.vy = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scale.vz = 0;
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scaleDelta.vx = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scaleDelta.vy = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].scaleDelta.vz = 0;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

void FieldScriptSetParticleBankColor(void) {
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].color.r  = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].color.g = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].color.b = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].colorDelta.r = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].colorDelta.g = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[D_800B2384.bankIndex].colorDelta.b = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptParticlesInitialize(void) {
    g_FieldScriptMaxInstructionCount += 4;
    if (D_800ADB8C == 0) {
        FieldInitializeParticleBanks(D_800AFD1C);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptStopParticleActor(void) {
    g_FieldScriptMaxInstructionCount += 4;
    FieldParticleActorStop(D_800AFD1C, SCRIPT_READ_U8_REL(0x1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}
// End of particle handlers

void FieldScriptWritePartySlotOfActor(void) {
    int i;

    for (i = 0; i < 3; i++) {
        if (D_8005A444[i] == D_800AFD1C) {
            FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, i);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
            return;
        }
    }
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, 0xFF);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// One u16 field each of two 8-byte-stride records, so every reference is a
// separate symbol with no addend.
extern u16 D_800B22E8[];
extern u16 D_800B22EA[];
extern u16 D_800B22EC[];
extern u16 D_800B2300[];
extern u16 D_800B2302[];
extern u16 D_800B2304[];
extern s32 D_800B2318[];

void FieldScriptSetMotionSlotPath(void) {
    int index = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x11));

    D_800B22E8[index * 4] = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x11));
    D_800B22EA[index * 4] = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x11));
    D_800B22EC[index * 4] = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x11));
    D_800B2300[index * 4] = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0x11));
    D_800B2302[index * 4] = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0x11));
    D_800B2304[index * 4] = FieldScriptArgument7(0xD, SCRIPT_READ_U8_REL(0x11));
    D_800B2318[index] = FieldScriptArgument8(0xF, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}

extern s16 D_800B22E2[];
extern s16 D_800B2324[];
extern s16 D_800B2326[];
extern s16 D_800B2328[];

void FieldScriptSetMotionSlotTarget(void) {
    int index = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    int actorIndex;

    D_800B2324[index * 4] = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    D_800B2328[index * 4] = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(9));
    D_800B2326[index * 4] = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(9));
    actorIndex = FieldScriptVMGetActorIndex(0xA);
    if (actorIndex != 0xFF) {
        D_800B22E2[index] = actorIndex;
    } else {
        D_800B22E2[index] = -1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}

extern FieldScriptGlobalS16 D_800B22E0;

void FieldScriptSetByte22E0(void) {
    D_800B22E0.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern FieldScriptGlobalU8 D_800B21D2;

void FieldScriptSetByte21D2(void) {
    D_800B21D2.value = FieldScriptVMGetArgument(1) - 0x80;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800AFE84;

void FieldScriptSetByteAFE84(void) {
    D_800AFE84 = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern FieldScriptGlobalS16 D_800B0080;
extern FieldScriptGlobalS16 D_800B0082;
extern FieldScriptGlobalS16 D_800B0084;
extern FieldScriptGlobalS16 D_800B0086;
extern FieldScriptGlobalS16 D_800B0088;
extern FieldScriptGlobalS16 D_800B008A;
extern FieldScriptGlobalS16 D_800B008C;
extern FieldScriptGlobalS16 D_800B008E;

void FieldScriptSetParams80(void) {
    int value;

    D_800B0080.value = FieldScriptVMGetInstructionArgument(1);
    D_800B0082.value = FieldScriptVMGetInstructionArgument(3);
    value = FieldScriptVMGetInstructionArgument(5);
    D_800B0084.value = value;
    if ((short)value == 0) {
        D_800B0084.value = value + 1;
    }
    D_800B0086.value = FieldScriptVMGetInstructionArgument(7);
    D_800B0088.value = 0;
    D_800B008A.value = FieldScriptVMGetInstructionArgument(9);
    D_800B008C.value = FieldScriptVMGetInstructionArgument(0xB);
    D_800B008E.value = FieldScriptVMGetInstructionArgument(0xD);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xF;
}

extern FieldScriptGlobalS32 D_800B0090;
extern FieldScriptGlobalS32 D_800B0094;
extern FieldScriptGlobalS32 D_800B0098;

void FieldScriptSetParams90(void) {
    D_800B0090.value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    D_800B0098.value = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    D_800B0094.value = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

extern FieldScriptGlobalU8 D_800B00A0;
extern FieldScriptGlobalU8 D_800B00A1;
extern FieldScriptGlobalU8 D_800B00A2;
extern FieldScriptGlobalU8 D_800B00A4;
extern FieldScriptGlobalU8 D_800B00A5;
extern FieldScriptGlobalU8 D_800B00A6;
extern FieldScriptGlobalU8 D_800B00A8;
extern FieldScriptGlobalU8 D_800B00A9;
extern FieldScriptGlobalU8 D_800B00AA;
extern FieldScriptGlobalS16 D_800B00AC;
extern FieldScriptGlobalS16 D_800B00AE;
extern FieldScriptGlobalS16 D_800B00B0;
extern FieldScriptGlobalS16 D_800B00B2;

void FieldScriptSetParamsA0(void) {
    D_800B00A0.value = FieldScriptVMGetArgument(1);
    D_800B00A1.value = FieldScriptVMGetArgument(3);
    D_800B00A2.value = FieldScriptVMGetArgument(5);
    D_800B00A4.value = FieldScriptVMGetArgument(7);
    D_800B00A5.value = FieldScriptVMGetArgument(9);
    D_800B00A6.value = FieldScriptVMGetArgument(0xB);
    D_800B00A8.value = FieldScriptVMGetArgument(0xD);
    D_800B00A9.value = FieldScriptVMGetArgument(0xF);
    D_800B00AA.value = FieldScriptVMGetArgument(0x11);
    D_800B00AC.value = FieldScriptVMGetArgument(0x13);
    D_800B00AE.value = FieldScriptVMGetArgument(0x15);
    D_800B00B0.value = FieldScriptVMGetArgument(0x17);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x19;
    D_800B00B2.value = 1;
}

extern s32 D_800ADB88;

void FieldScriptWaitForTransitionBusyFlag(void) {
    if (D_800ADB88 == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern s32 D_800B06A0;

void FieldScriptWriteByteB06A0(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), D_800B06A0);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// 98.33% (504 bytes, same instruction multiset): the only difference is which
// instruction fills the ArchiveSetIndex jal delay slot - the original keeps the
// `move s0,v0` of the FieldScriptArgument1 result there and emits `move a1,zero`
// before the call; ours is the other way round. Splitting the result into a second
// local and computing `value + 0x7FB` after the call gives 508 bytes / 97.38%.
// FieldLoadTIMWithClut is declared with int parameters here: the shared prototype's
// shorts cost an sll/sra pair on each of x, y and clutY (92.09%).
/*
extern void* D_800B1F74;
extern void FieldStoreScreenImage(void);
extern void FieldLoadTIMWithClut(u_long* pTimData, int x, int y, int clutX, int clutY, int clutWidth,
                                 int clutHeight);

// Loads a TIM (or, for the non-zero command byte, re-blits the one already loaded)
// into the field's screen image buffer and yields the script for one frame.
void func_8008A2E8(void) {
    s32 clutY;
    s32 x;
    s32 y;
    s32 value;
    s32 archiveIndex;
    u8 command;
    void* pBuffer;

    if (FieldArchiveTryDataSync() == -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        D_800B00C0 = 1;
        return;
    }
    command = SCRIPT_READ_U8_REL(1);
    if (command == 0) {
        archiveIndex = FieldScriptArgument1(5, SCRIPT_READ_U8_REL(0xD));
        ArchiveSetIndex(4, 0);
        archiveIndex += 0x7FB;
        pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(archiveIndex), 0);
        D_800B1F74 = pBuffer;
        ArchiveReadFileToBuffer(archiveIndex, pBuffer, 0, 0x80);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    } else if (command == 1) {
        value = FieldScriptArgument4(8, SCRIPT_READ_U8_REL(0xA));
        clutY = value + 0xE8;
        if (value == 0xFF) {
            clutY = -1;
        }
        x = FieldScriptArgument2(4, SCRIPT_READ_U8_REL(0xA));
        y = FieldScriptArgument3(6, SCRIPT_READ_U8_REL(0xA));
        if (y >= 0x100 && x >= 0x2C0) {
            FieldStoreScreenImage();
        }
        FieldLoadTIMWithClut(D_800B1F74, x, y, 0, clutY, 0, 0);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
    } else {
        HeapFree(D_800B1F74);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
    D_800B00C0 = 1;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008A2E8);

void FieldScriptNoOpHandler1(void) {}
void FieldScriptNoOpHandler2(void) {}
void FieldScriptNoOpHandler3(void) {}
void FieldScriptNoOpHandler4(void) {}
void FieldScriptNoOpHandler5(void) {}
void FieldScriptNoOpHandler6(void) {}
void FieldScriptNoOpHandler7(void) {}
void FieldScriptNoOpHandler8(void) {}

void FieldWaitForArchiveSync(void) {
    while (FieldArchiveTryDataSync()) {
        Vsync(0);
    }
}

extern int ArchiveDataSync(void);
extern void ArchiveCdDataSync(int mode);

extern int D_800ADB2C;

int FieldArchiveTryDataSync(void) {
    if (D_800ADB2C != 0) {
        return -1;
    }
    if (ArchiveDataSync() != 0) {
        return -1;
    }
    ArchiveCdDataSync(0);
    return 0;
}

extern void func_8003633C(int);

void FieldScriptClearControllerFlag(void) {
    if (SCRIPT_READ_U8_REL(1) == 0) {
        func_8003633C(0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern FieldScriptGlobalS16 D_800B21D4;

void FieldScriptSetRenderContextIndex(void) {
    D_800B21D4.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetCharacterField78(void) {
    int partyId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(3));

    if (partyId != CHARACTER_ID_NONE) {
        int value = FieldScriptVMGetArgument(1) - g_pGameState->characters[partyId].unk77;

        if (value < 0) {
            value = 0;
        }
        g_pGameState->characters[partyId].unk78 = value;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptWriteCharacterFieldSum(void) {
    int partyId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(3));

    if (partyId != CHARACTER_ID_NONE) {
        int value = g_pGameState->characters[partyId].unk77 + g_pGameState->characters[partyId].unk78;

        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, value);
    } else {
        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, 0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

int FieldPartyAllocSlot(int characterId, int* pPartyIndex) {
    int i;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_GamePartyMembers[i] == characterId) {
            break;
        }
        if (g_GamePartyMembers[i] == CHARACTER_ID_NONE) {
            *pPartyIndex = i;
            return 0;
        }
    }
    return -1;
}

extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern int ArchiveSetIndex(int directoryIndex, int entryIndex);
extern void ArchiveCdDataSync(int mode);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern int GameCharacterGetGearID(int characterId);
extern s32 g_GameSceneMapNum;
extern int g_GamePartyMemberSkins[MAX_PARTY_MEMBERS];
extern void* D_800ADBC0;
extern s32 D_800ADBC4;
extern void* D_800ADBC8;
extern s32 D_800ADBCC;

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008A7DC);
/*
 * func_8008A7DC 98.73% - only difference is $s0/$s1 assignment
 * (archiveIndex vs characterId); instruction multiset identical.
void func_8008A7DC(int characterId, int partyIndex) {
    int archiveIndex;
    int size;
    void* pBuffer;
    int characterIndex;

    D_800ADBCC = partyIndex;
    D_800ADBC8 = characterId;
    ArchiveSetIndex(4, 0);
    if (D_800ADB1C == 0) {
        ArchiveCdDataSync(0);
    }
    if ((g_GameSceneMapNum & 0xC000) == 0) {
        archiveIndex = characterId + 5;
        size = ArchiveDecodeAlignedSize(archiveIndex);
        g_GamePartyMemberSkins[D_800ADBCC] = characterId;
        pBuffer = HeapAlloc(size, 0);
        D_800ADBC0 = pBuffer;
        ArchiveReadFileToBuffer(archiveIndex, pBuffer, 0, 0x80);
    } else {
        characterIndex = GameCharacterGetGearID(characterId);
        if (characterIndex == 0xFF) {
            characterIndex = 0;
        }
        characterIndex += 0x10;
        archiveIndex = characterIndex + 5;
        size = ArchiveDecodeAlignedSize(archiveIndex);
        pBuffer = HeapAlloc(size, 0);
        D_800ADBC0 = pBuffer;
        g_GamePartyMemberSkins[D_800ADBCC] = characterIndex;
        ArchiveReadFileToBuffer(archiveIndex, pBuffer, 0, 0x80);
    }
    if (D_800ADB1C == 0) {
        ArchiveCdDataSync(0);
    }
    D_800ADBC4 = 1;
}
 */

void FieldScriptSetInvertedAnimationId(void) {
    g_FieldScriptVMCurActor->unkAnimationId = ~SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetInvertedAnimationIdAndClearFlag(void) {
    FieldScriptSetInvertedAnimationId();
    g_FieldScriptVMCurActor->flags &= 0xFFFEFFFF;
}

extern s32 D_800ADB90;

void FieldScriptLoadSpecialAnimFile(void) {
    if (FieldArchiveTryDataSync() == 0) {
        D_800ADB90 = 0;
        SpriteSetSpecialAnimFile(g_FieldActors[D_800AFD1C].pSpriteData, g_FieldScriptVMCurActor->unk120);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

void FieldScriptFreeSpecialAnimFile(void) {
    if (g_FieldScriptVMCurActor->unk124 != -1) {
        HeapFree(g_FieldScriptVMCurActor->unk120);
        g_FieldScriptVMCurActor->unk124 = -1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    D_800B00C0 = 1;
}

extern void* D_800AFD08; // WDS file buffer

extern s32 D_800AFD0C;

extern s32 D_800AFD18;

extern void* D_80062518[];

extern s32 D_80062524;

extern s32 D_800595AC;

extern s32 D_8004F370;

extern void func_8003BDFC(int);

extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern int ArchiveSetIndex(int directoryIndex, int entryIndex);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);

void FieldScriptLoadWdsSoundFile(void) {
    int flags;
    int wdsIndex;
    void* pWdsEntry;
    void* pWdsFileBuffer;

    if (FieldArchiveTryDataSync() == 0) {
        if (SCRIPT_READ_U8_REL(1) == 1) {
            D_80062518[D_800AFD18] = SoundLoadWdsFile(D_800AFD08, 0);
            func_8003BDFC(0x10);
            HeapFree(D_800AFD08);
            if (D_800AFD18 == 3) {
                D_800595AC = D_80062524;
            }
            D_800B00C0 = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        } else {
            wdsIndex = FieldScriptVMGetArgument(2);
            pWdsEntry = D_80062518[wdsIndex];
            D_800AFD18 = wdsIndex;
            SoundFreeWdsEntry(pWdsEntry);
            flags = FieldScriptVMGetArgument(4);
            D_800AFD0C = flags;
            if (!(flags & 0x80)) {
setIndex1C:
                ArchiveSetIndex(0x1C, 0);
                D_800AFD0C += 2;
                goto readFile;
            }
            if (D_8004F370 != 1) {
                D_800AFD0C = (flags & 0x7F) + 0x1F;
                ArchiveSetIndex(0x2C, 1);
                goto readFile;
            }
            D_800AFD0C = 4;
            goto setIndex1C;
readFile:
            pWdsFileBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_800AFD0C), 0);
            D_800AFD08 = pWdsFileBuffer;
            ArchiveReadFileToBuffer(D_800AFD0C, pWdsFileBuffer, 0, 0x80);
            ArchiveSetIndex(4, 0);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
        }
    } else {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
}

extern void ArchiveCdDataSync(int mode);

void FieldScriptLoadSpecialAnimArchive(void) {
    int scriptArgument;
    int archiveIndex;
    int bufferSize;
    void* pBuffer;

    if (D_800ADB90 == 0 && D_800ADB2C == 0) {
        if (FieldArchiveTryDataSync() != 0) {
            D_800B00C0 = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
            return;
        }
        if (g_FieldScriptVMCurActor->unk124 != -1) {
            HeapFree(g_FieldScriptVMCurActor->unk120);
            g_FieldScriptVMCurActor->unk124 = -1;
        }
        scriptArgument = FieldScriptVMGetArgument(1);
        ArchiveSetIndex(4, 0);
        archiveIndex = scriptArgument + 0x77A;
        bufferSize = ArchiveDecodeAlignedSize(archiveIndex) + 8;
        g_FieldScriptVMCurActor->unk124 = archiveIndex;
        pBuffer = HeapAlloc(bufferSize, 0);
        g_FieldScriptVMCurActor->unk120 = pBuffer;
        ArchiveReadFileToBuffer(archiveIndex, pBuffer, 0, 0x80);
        if (D_800ADB1C == 0) {
            ArchiveCdDataSync(0);
        }
        D_800ADB90 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

void FieldScriptSetFlag20000(void) {
    if (SCRIPT_READ_U8_REL(1) == 0) {
        g_FieldScriptVMCurActor->flags |= 0x20000;
    } else {
        g_FieldScriptVMCurActor->flags &= ~0x20000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s16 D_800B221C[];
extern s16 D_800B221E[];
extern s16 D_800B2220[];

void FieldScriptSetVectorTable221C(void) {
    int index = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));

    D_800B221C[index * 3] = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    D_800B221E[index * 3] = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(9));
    D_800B2220[index * 3] = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(9));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

extern s16 D_800B223C[];

void FieldScriptSetVectorTable223C(void) {
    int index = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));

    D_800B223C[index] = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    D_800B223C[index + 3] = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(9));
    D_800B223C[index + 6] = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(9));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

extern FieldScriptGlobalU8 D_800B225C;
extern FieldScriptGlobalU8 D_800B225D;
extern FieldScriptGlobalU8 D_800B225E;

void FieldScriptSetBackgroundColor(void) {
    D_800B225C.value = FieldScriptVMGetArgument(1);
    D_800B225D.value = FieldScriptVMGetArgument(3);
    D_800B225E.value = FieldScriptVMGetArgument(5);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern FieldScriptGlobalS16 D_800B21B4;

void FieldScriptSetUpdateAngle(void) {
    D_800B21B4.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800ADB1C;
extern s16 D_800B21E4[];

void FieldScriptSetGearModelAnimation(void) {
    if (D_800ADB1C != 0) {
        LibGearSetModelAnimation(FieldScriptVMGetArgument(1) & 0xFFFF, 0, FieldScriptVMGetArgument(3));
        D_800B21E4[FieldScriptVMGetArgument(1)] = FieldScriptVMGetArgument(3);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptSetByte11E(void) {
    g_FieldScriptVMCurActor->unk11E = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void FieldFadeSetParameters(int, int, int, int, int, int);

void FieldScriptSetFadeParameters(void) {
    FieldFadeSetParameters(
        1,
        FieldScriptVMGetArgument(9),
        FieldScriptVMGetArgument(3),
        FieldScriptVMGetArgument(5),
        FieldScriptVMGetArgument(7),
        FieldScriptVMGetArgument(1)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}

void FieldScriptInitializeDistortion(void) {
    FieldDistortionInitialize(0);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xF;
}

extern void FieldDistortionSetTarget(int, int, int, int, int, int, int);
extern void FieldDistortionFree(void);

void FieldScriptHandleDistortion(void) {
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            FieldDistortionSetTarget(0, 0, 0, 0, 0, 0, FieldScriptVMGetArgument(2));
            g_FieldEffects.distortion.isFinished = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
            break;
        case 1:
            if (g_FieldEffects.distortion.isActive == 0) {
                g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            } else {
                g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
            }
            break;
        case 2:
            g_FieldEffects.distortion.isActive = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
        case 3:
            FieldDistortionFree();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            break;
    }
    D_800B00C0 = 1;
}


extern FieldScriptGlobalS16 D_800B2184;
extern FieldScriptGlobalS16 D_800B2186;
extern FieldScriptGlobalS16 D_800B2188;

void FieldScriptSetRotationVector(void) {
    D_800B2184.value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    D_800B2188.value = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    D_800B2186.value = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptSetCameraRotation(void) {
    g_Scene.camRotation.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    g_Scene.camRotation.vz = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    g_Scene.camRotation.vy = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008B5D4);

extern void* g_PartyDataBuffers[];
extern void* D_800ADBC0;
extern s32 D_800ADBC4;
extern void* D_800ADBC8;
extern s32 D_800ADBCC;
extern void LZSSDecompress(void*, void*);
extern void func_8008B978(void*);

void FieldScriptFinishPartyModelLoad(void) {
    if (D_800ADBC4 != 0xFF && ArchiveDataSync() == 0) {
        ArchiveCdDataSync(0);
        LZSSDecompress(D_800ADBC0, g_PartyDataBuffers[D_800ADBCC]);
        HeapFree(D_800ADBC0);
        func_8008B978(D_800ADBC8);
        D_800ADBC4 = 0xFF;
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008B978);

extern void func_8008A7DC(int characterId, int partyIndex);

void FieldScriptAddPartyMember(void) {
    int characterId;
    int partyIndex;

    if (D_800ADBC4 == CHARACTER_ID_NONE && D_800ADB2C == 0 && FieldArchiveTryDataSync() == 0) {
        ArchiveCdDataSync(0);
        characterId = FieldScriptVMGetArgument(1);
        if (characterId != CHARACTER_ID_NONE) {
            if (FieldPartyAllocSlot(characterId, &partyIndex) == 0) {
                g_pGameState->gearRide[partyIndex] = 0;
                g_GamePartyMembers[partyIndex] = characterId;
                func_8008A7DC(characterId, partyIndex);
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
            } else {
                g_pGameState->unk1D30 |= 1 << characterId;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
            }
        } else {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
        }
    } else {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
}

// 94.32%: structure and every store match; only the second SCRIPT_READ_U8_REL(1)
// expansion is ordered differently (the original loads both globals before the
// `lhu 0xCC` and the party index before the `addu`; gcc reorders ours the other
// way and picks $v0/$v1 the opposite way round).
// preset screen: 2.7.2 94.89%, cdk 84.77%, 2.6.0 94.89%, all with the call below
// fixed - no compiler fixes it. As parked the body does not compile:
// func_8008A7DC() is declared with two params, and passing
// SCRIPT_READ_U8_REL(1) and partyIndex gives the numbers above.
/*
void func_8008BDD8(void) {
    int partyIndex;

    if (D_800ADBC4 == CHARACTER_ID_NONE && D_800ADB2C == 0 && FieldArchiveTryDataSync() == 0) {
        ArchiveCdDataSync(0);
        if (FieldPartyAllocSlot(SCRIPT_READ_U8_REL(1), &partyIndex) == 0) {
            g_pGameState->gearRide[partyIndex] = 0;
            g_GamePartyMembers[partyIndex] = SCRIPT_READ_U8_REL(1);
            func_8008A7DC();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        } else {
            g_pGameState->unk1D30 |= 1 << SCRIPT_READ_U8_REL(1);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        }
    } else {
        D_800B00C0 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008BDD8);

extern void* D_8005A418[];
extern s32 D_8005A448[];
extern int g_GamePartyMemberSkins[MAX_PARTY_MEMBERS];
extern void func_80076AC0(s32, s32, void*, s32, s32, s32, s32);

// One 0x10 byte block of the 0x14000 byte party model buffers (a 320x128 image)
typedef struct {
    u32 w0, w1, w2, w3;
} PartyDataChunk; // Size: 0x10

// 97.81%: every instruction matches except the `li 0x14000` that holds the
// copy end offset. The original allocates it to $t0, and $t0 still holds
// &D_8005A448 (read by the if-body as `addiu $v0,$t0,-4`), so reorg refuses
// the branch-target->delay-slot fill and leaves a nop before the loop
// (584 bytes). Ours gets $v0 there, which is dead in the if-body, so the fill
// is legal and the function is 580 bytes. Forcing the constant into a local
// (u32/int/long, declared first or last) or moving it out of the else block
// does not change the colour; writing the address arithmetic with $t0 already
// live (e.g. pEnd = &((u8*)pSrc)[0x14000], (u32)pSrc + 0x14000, u8* src,
// flipped loop condition, while/for forms) only loses more instructions.
// The addu operand order *is* solved: &g_FieldActors[characterId] emits
// base-first, `(characterId + g_FieldActors)` emits index-first as retail does.
// preset screen: 2.7.2 97.81%, cdk 86.01%, 2.6.0 97.81% - no compiler
// fixes it.
/*
void func_8008BF38(int partyIndex) {
    int characterId;
    ActorData* pActor;
    void* pSprite;
    PartyDataChunk* pEnd;
    PartyDataChunk* pSrc;
    PartyDataChunk* pDst;
    u8 unused[8]; // original has unused stack slots; reproduces the frame

    characterId = D_8005A448[partyIndex];
    if (characterId == ACTOR_ID_INVALID) {
        g_GamePartyMemberSkins[partyIndex] = g_GamePartyMemberSkins[partyIndex + 1];
        g_GamePartyMembers[partyIndex] = g_GamePartyMembers[partyIndex + 1];
        D_8005A448[partyIndex - 1] = D_8005A448[partyIndex];
        g_GamePartyMembers[partyIndex + 1] = characterId;
        g_GamePartyMemberSkins[partyIndex + 1] = characterId;
        D_8005A448[partyIndex] = characterId;
        return;
    }

    pSrc = D_8005A418[partyIndex];
    pDst = g_PartyDataBuffers[partyIndex];
    pEnd = (PartyDataChunk*)((u8*)pSrc + 0x14000);
    do {
        *pDst = *pSrc;
        pSrc++;
        pDst++;
    } while (pSrc != pEnd);

    g_GamePartyMemberSkins[partyIndex] = g_GamePartyMemberSkins[partyIndex + 1];
    g_GamePartyMembers[partyIndex] = g_GamePartyMembers[partyIndex + 1];
    D_8005A444[partyIndex] = D_8005A444[partyIndex + 1];

    pActor = (characterId + g_FieldActors)->pActorData;
    if (!(pActor->unk126 & 0x80)) {
        func_80076AC0(characterId, partyIndex, g_PartyDataBuffers[partyIndex], 1, 0, partyIndex, 1);
    } else {
        pSprite = (void*)((*(s32*)(g_FieldSpriteData + (pActor->unk126 & 0x7F) * 4 + 4)) + (s32)g_FieldSpriteData);
        func_80076AC0(characterId, pActor->spriteId, pSprite,
            ((u32)pActor->flags130 >> 28) & 0x3, pActor->flags134 & 0xF,
            (characterId + g_FieldActors)->pActorData->unk126,
            ((u32)(characterId + g_FieldActors)->pActorData->flags134 >> 4) & 0x1);
    }

    g_GamePartyMembers[partyIndex + 1] = ACTOR_ID_INVALID;
    g_GamePartyMemberSkins[partyIndex + 1] = ACTOR_ID_INVALID;
    D_8005A448[partyIndex] = ACTOR_ID_INVALID;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008BF38);

extern FieldActor* D_800B06B8;
extern void* g_PartyDataBuffers[];
extern void func_80080A74(int actorIndex);

// 92.49%: prologue, body and epilogue match instruction for instruction, but ours
// is 428 bytes against the original's 440. Two things are left, both from one
// cause. The original keeps `&g_FieldActors` (the address of the global
// pointer, not its value) in $s0 for the two `lw %lo(g_FieldActors)` in this
// function, so it needs one more callee-saved register ($s7 = 0xFF) and a 0x48
// frame; ours re-forms the symbol address at each of the two loads (lui+lw, same
// instruction count), which costs the s7 save and 8 bytes of frame. The 0xFF
// compares/const is the visible symptom. Also solved here, do not redo: the second
// half must read D_8005A444[partyIndex] ONCE into a local before the `sh` that
// stores FieldActor::status, or gcc reloads it three times (two after the store)
// and the function comes out 12 bytes short. Tried for the address, all at
// 92.49%: `&g_FieldActors[i]` twice, a `FieldActor** pFieldActors =
// (FieldActor**)&g_FieldActors;` local (initialiser and assignment, the misc6
// idiom), and a `FieldActor* volatile*` one. gcc 2.7.2 rematerialises the
// address (`lui; lw %lo(sym)`) at both loads because the pseudo is used only
// twice; misc6's func_800A0524 gets the materialised form with the same idiom
// because it uses the local eight times.
// preset screen: 2.7.2 92.49%, cdk 83.59%, 2.6.0 92.30% - no compiler
// fixes it.
/*
void func_8008C180(int partyIndex) {
    FieldActor** pFieldActors;
    FieldActor* pFieldActor;
    FieldActor* pPrevFieldActor;
    ActorData* pActor;
    ActorData* pPrevActor;
    int characterId;
    int prevActorIndex;
    u_short prevScriptPointer;

    if (D_8005A444[partyIndex] != ACTOR_ID_INVALID) {
        pFieldActors = (FieldActor**)&g_FieldActors;
        pFieldActor = &(*pFieldActors)[D_8005A444[partyIndex]];
        pPrevFieldActor = D_800B06B8;
        pPrevActor = g_FieldScriptVMCurActor;
        prevActorIndex = D_800AFD1C;
        prevScriptPointer = g_FieldScriptVMCurActor->scriptInstructionPointer;
        D_800B06B8 = pFieldActor;
        g_FieldScriptVMCurActor = pFieldActor->pActorData;
        func_80080A74(D_8005A444[partyIndex]);

        characterId = D_8005A444[partyIndex];
        pFieldActor = &(*pFieldActors)[characterId];
        pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
        D_800AFD1C = characterId;
        func_80076AC0(characterId, 0, g_PartyDataBuffers[0], 1, 0, 0, 1);

        pActor = g_FieldScriptVMCurActor;
        pActor->scriptFlags.flags |= 1;
        pActor->flags |= 0x100000;
        D_800B00C0 = 0;
        g_FieldScriptVMCurActor = pPrevActor;
        D_800B06B8 = pPrevFieldActor;
        D_800AFD1C = prevActorIndex;
        pActor->scriptInstructionPointer = prevScriptPointer;
        pActor->scriptFlags.flags |= 0x20000;
        pActor->flags |= 0x400;
        D_8005A444[partyIndex] = ACTOR_ID_INVALID;
    }
    g_GamePartyMembers[partyIndex] = ACTOR_ID_INVALID;
    g_GamePartyMemberSkins[partyIndex] = ACTOR_ID_INVALID;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008C180);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008C334);

void FieldScriptFreeActorFile(void) {
    if (g_FieldScriptVMCurActor->flags12C_0xD) {
        HeapFree(g_FieldScriptVMCurActor->unk114);
        g_FieldScriptVMCurActor->flags12C_0xD = 0;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern void func_8003A89C(s32, int, int);
extern s32 D_8004F36C;
extern s32 D_8004F324;
extern s32 D_80062528;

void FieldScriptSetAudioInterpolator70(void) {
    if (D_8004F36C != 0) {
        func_8003A89C(D_80062528, FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(3));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (D_8004F324 == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern void func_8003A948(s32, int, int);

void FieldScriptSetAudioInterpolator7C(void) {
    if (D_8004F36C != 0) {
        func_8003A948(D_80062528, FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)),
                      FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5)));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (D_8004F324 == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern void func_8003A838(int, int, int);

void FieldScriptSetAudioInterpolator64(void) {
    if (D_8004F36C != 0) {
        func_8003A838(D_80062528, FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(3));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (D_8004F324 == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern void func_8003A9BC(int, int, int);

void FieldScriptSetAudioInterpolator88(void) {
    if (D_8004F36C != 0) {
        func_8003A9BC(D_80062528, FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)),
                      FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5)));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (D_8004F324 == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern void func_8003AAC4(int, int);

void FieldScriptSetAudioMuteFlags(void) {
    if (D_8004F36C != 0) {
        func_8003AAC4(D_80062528, FieldScriptVMGetArgument(1));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else if (D_8004F324 == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else if (D_800ADB1C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern void FieldReleaseActorSoundSlot(s32);

void FieldScriptSetActorSoundParams0(void) {
    g_FieldScriptVMCurActor->unk10A = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->unk10D = 0;
    g_FieldScriptVMCurActor->unk10C = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    FieldReleaseActorSoundSlot(D_800AFD1C);
    if (g_FieldScriptVMCurActor->unk10A == 0) {
        g_FieldScriptVMCurActor->unk10D = 0xFF;
    }
}

void FieldScriptSetActorSoundParams80(void) {
    g_FieldScriptVMCurActor->unk10A = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->unk10D = 0x80;
    g_FieldScriptVMCurActor->unk10C = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    FieldReleaseActorSoundSlot(D_800AFD1C);
    if (g_FieldScriptVMCurActor->unk10A == 0) {
        g_FieldScriptVMCurActor->unk10D = 0xFF;
    }
}

void FieldScriptClearPartyFrMaskBit(void) {
    int partyId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(1));

    if (partyId != CHARACTER_ID_NONE) {
        g_pGameState->FrMask &= ~(1 << partyId);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetPartyFrMaskBit(void) {
    int partyId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(1));

    if (partyId != CHARACTER_ID_NONE) {
        g_pGameState->FrMask |= 1 << partyId;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern int FieldPartySlotToCharacter(int);

// Maps a party slot id (0xFC-0xFF) to the character in that slot.
int FieldPartySlotToCharacter(int characterId) {
    if (characterId == CHARACTER_ID_NONE) {
        return g_GamePartyMembers[2];
    }
    if (characterId == 0xFE) {
        return g_GamePartyMembers[1];
    }
    if (characterId == 0xFD) {
        return g_GamePartyMembers[0];
    }
    if (characterId == 0xFC) {
        return CHARACTER_ID_NONE;
    }
    return characterId;
}

void FieldScriptSetActorFaceId(void) {
    g_FieldScriptVMCurActor->faceId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern FieldScriptGlobalS16 D_800B21A0;
extern FieldScriptGlobalS16 D_800B21A2;
extern FieldScriptGlobalS16 D_800B21A4;
extern FieldScriptGlobalS16 D_800B21A6;
extern FieldScriptGlobalS16 D_800B21A8;
extern FieldScriptGlobalS16 D_800B21AA;

void FieldScriptSetRotationVectorPair(void) {
    D_800B21A0.value = FieldScriptVMGetInstructionArgument(1);
    D_800B21A4.value = FieldScriptVMGetInstructionArgument(3);
    D_800B21A2.value = FieldScriptVMGetInstructionArgument(5);
    D_800B21A6.value = FieldScriptVMGetInstructionArgument(7);
    D_800B21AA.value = FieldScriptVMGetInstructionArgument(9);
    D_800B21A8.value = FieldScriptVMGetInstructionArgument(0xB);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xD;
}

void FieldScriptSetFlag800(void) {
    if (FieldScriptVMGetArgument(1) == 0) {
        g_FieldScriptVMCurActor->flags &= ~0x800;
    } else {
        g_FieldScriptVMCurActor->flags |= 0x800;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void func_80072254(int);

void FieldScriptSetActorScaleUniform(void) {
    unsigned int scale = FieldScriptVMGetArgument(1);

    ((SpriteData*)g_FieldActors[D_800AFD1C].pSpriteData)->scale = (scale * 3) >> 2;
    g_FieldScriptVMCurActor->scaleX = scale;
    g_FieldScriptVMCurActor->scaleY = scale;
    g_FieldScriptVMCurActor->scaleZ = scale;
    func_80072254(D_800AFD1C);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetActorScale(void) {
    int scaleX = FieldScriptVMGetArgument(1);
    int scaleY = FieldScriptVMGetArgument(3);
    int scaleZ = FieldScriptVMGetArgument(5);

    ((SpriteData*)g_FieldActors[D_800AFD1C].pSpriteData)->scale = 0xC00;
    g_FieldScriptVMCurActor->scaleX = scaleX;
    g_FieldScriptVMCurActor->scaleY = scaleY;
    g_FieldScriptVMCurActor->scaleZ = scaleZ;
    func_80072254(D_800AFD1C);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern FieldScriptGlobalS16 D_800B218C;

void FieldScriptSetScale218C(void) {
    D_800B218C.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetSpriteGravity(void) {
    ((SpriteData*)g_FieldActors[D_800AFD1C].pSpriteData)->field_0x82 = FieldScriptVMGetArgument(1) * 2;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptNoOpHandler(void) {
}

void FieldScriptWriteScriptU16(int value, int offset) {
    ((u8*)g_FieldScriptVMCurScriptData)[offset + 1] = value >> 8;
    ((u8*)g_FieldScriptVMCurScriptData)[offset] = value;
}

long FieldGetVec2Magnitude(long, long);

int FieldActorsWithinDistance(int dstActorId, int srcActorId) {
    FieldActor* actors = g_FieldActors;
    long distance;

    distance = FieldGetVec2Magnitude(
        actors[dstActorId].transformMatrix.t[0] - actors[srcActorId].transformMatrix.t[0],
        actors[dstActorId].transformMatrix.t[2] - actors[srcActorId].transformMatrix.t[2]
    );
    return -(distance >= 0x10);
}

static inline void CopySpriteFieldS32(FieldActor* actors, int dstId, int srcId, int offset) {
    void* dst = actors[dstId].pSpriteData;
    void* src = actors[srcId].pSpriteData;
    *(s32*)((u8*)dst + offset) = *(s32*)((u8*)src + offset);
}

static inline void CopySpriteFieldU16(FieldActor* actors, int dstId, int srcId, int offset) {
    void* dst = actors[dstId].pSpriteData;
    void* src = actors[srcId].pSpriteData;
    *(u16*)((u8*)dst + offset) = *(u16*)((u8*)src + offset);
}

static inline void CopyActorFieldS32(FieldActor* base, int dstId, int srcId, int offset) {
    *(s32*)((u8*)&base[dstId] + offset) = *(s32*)((u8*)&base[srcId] + offset);
}

void FieldActorCopyPlacement(int dstActorId, int srcActorId) {
    FieldActor* actors = g_FieldActors;
    ActorData* dstActor;
    ActorData* srcActor;
    int i;

    srcActor = actors[srcActorId].pActorData;
    dstActor = actors[dstActorId].pActorData;

    for (i = 0; i < 4; i++) {
        dstActor->walkmeshTriIds[i] = srcActor->walkmeshTriIds[i];
    }
    {
      s16 walkmeshId = srcActor->walkmeshId;
      dstActor->walkmeshId = walkmeshId;
    }
    dstActor->curTriNormal.vx = srcActor->curTriNormal.vx;
    dstActor->curTriNormal.vy = srcActor->curTriNormal.vy;
    dstActor->curTriNormal.vz = srcActor->curTriNormal.vz;
    dstActor->position.vx = srcActor->position.vx;
    dstActor->position.vy = srcActor->position.vy;
    dstActor->position.vz = srcActor->position.vz;
    dstActor->unkEC = srcActor->unkEC;
    dstActor->curYPos = srcActor->curYPos;
    dstActor->curWalkmeshTriMaterial = srcActor->curWalkmeshTriMaterial;

    CopySpriteFieldU16(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, field_0x84));
    CopySpriteFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, position.x));
    CopySpriteFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, position.y));
    CopySpriteFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(SpriteData, position.z));

    CopyActorFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[0]));
    CopyActorFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[1]));
    CopyActorFieldS32(g_FieldActors, dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[2]));
}

extern u8 D_800B219F;

void FieldClearPartyMaskForCurrentActor(void) {
    int i = 0;
    s32 actorIndex = D_800AFD1C;
    u8* pMask = &D_800B219F;

    for (; i < MAX_PARTY_MEMBERS; i++) {
        if (D_8005A444[i] == actorIndex) {
            *pMask &= ~(1 << i);
        }
    }
}

extern FieldScriptGlobalU8 D_800B21CD;

void FieldScriptSetByte21CD(void) {
    D_800B21CD.value = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetFlag400(void) {
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            g_FieldScriptVMCurActor->flags &= ~0x400;
            break;
        case 1:
            g_FieldScriptVMCurActor->flags |= 0x400;
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetVariableBit(void) {
    int mask;
    unsigned int slotIndex;

    slotIndex = (unsigned int) (SCRIPT_IMM_ARG(1)) >> 4;
    mask = 1 << (FieldScriptVMGetInstructionArgument(1) & 0xF);
    FieldScriptMemoryWriteU16(slotIndex, FieldScriptVMGetVariableValue(slotIndex) | mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptClearVariableBit(void) {
    int mask;
    unsigned int slotIndex;

    slotIndex = (unsigned int) (SCRIPT_IMM_ARG(1)) >> 4;
    mask = 1 << (FieldScriptVMGetInstructionArgument(1) & 0xF);
    FieldScriptMemoryWriteU16(slotIndex, FieldScriptVMGetVariableValue(slotIndex) & ~mask);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptJumpUnlessVariableBitSet(void) {
    short destination;
    int mask;
    unsigned int slotIndex;

    slotIndex = (unsigned int) (SCRIPT_IMM_ARG(1)) >> 4;
    mask = 1 << (FieldScriptVMGetInstructionArgument(1) & 0xF);
    if (FieldScriptVMGetVariableValue(slotIndex) & mask) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        destination = FieldScriptVMGetInstructionArgument(3);
        g_FieldScriptVMCurActor->scriptInstructionPointer = destination;
    }
}

// Writes the 0x12-byte script instruction at the current instruction pointer: a
// verb byte, the three u16 operands it was handed, and a fixed tail.
void func_8008D808(int value0, int value1, int value2) {
    SCRIPT_READ_U8_REL(0) = 0x57;
    SCRIPT_READ_U8_REL(1) = 0x81;
    FieldScriptWriteScriptU16(value0, g_FieldScriptVMCurActor->scriptInstructionPointer + 2);
    FieldScriptWriteScriptU16(value1, g_FieldScriptVMCurActor->scriptInstructionPointer + 4);
    FieldScriptWriteScriptU16(value2, g_FieldScriptVMCurActor->scriptInstructionPointer + 6);
    FieldScriptWriteScriptU16(0xC, g_FieldScriptVMCurActor->scriptInstructionPointer + 8);
    SCRIPT_READ_U8_REL(0xA) = 0xFF;
    SCRIPT_READ_U8_REL(0xB) = 0x57;
    SCRIPT_READ_U8_REL(0xC) = 0x8F;
    SCRIPT_READ_U8_REL(0xD) = 0x26;
    SCRIPT_READ_U8_REL(0xE) = 0x1;
    SCRIPT_READ_U8_REL(0xF) = 0x80;
    SCRIPT_READ_U8_REL(0x10) = 0x57;
    SCRIPT_READ_U8_REL(0x11) = 0xF;
}

void FieldScriptPatchJumpInstruction(int value1, int value2) {
    SCRIPT_READ_U8_REL(0xC) = 0x4B;
    FieldScriptWriteScriptU16(value1, g_FieldScriptVMCurActor->scriptInstructionPointer + 0xD);
    FieldScriptWriteScriptU16(value2, g_FieldScriptVMCurActor->scriptInstructionPointer + 0xF);
    SCRIPT_READ_U8_REL(0x11) = 0xFF;
    SCRIPT_READ_U8_REL(0x12) = 0xFF;
    SCRIPT_READ_U8_REL(0x13) = 0x80;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xC;
}

void FieldScriptClearParentActor(void) {
    g_FieldScriptVMCurActor->parentActorId = ACTOR_ID_INVALID;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern FieldScriptGlobalS16 D_800B233E;

void FieldScriptSetPlayerActorIndexMirror(void) {
    D_800B233E.value = FieldScriptVMGetActorIndex(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}


int FieldPartyMemberIncreaseGearHp(int partyMemberIndex, unsigned int amount) {
    int gearId;
    unsigned int maxHp;
    unsigned int newHp;

    gearId = GameCharacterGetGearID(g_GamePartyMembers[partyMemberIndex]);
    if (gearId != CHARACTER_ID_NONE) {
        maxHp = g_pGameState->gears[gearId].maxHp;
        newHp = g_pGameState->gears[gearId].hp + amount;
        g_pGameState->gears[gearId].hp = newHp;
        if (maxHp < newHp) {
            g_pGameState->gears[gearId].hp = maxHp;
        }
    }
}

int FieldPartyMemberDecreaseGearHp(int partyMemberIndex, unsigned int amount) {
    int gearId;
    int newHp;

    gearId = GameCharacterGetGearID(g_GamePartyMembers[partyMemberIndex]);
    if (gearId != CHARACTER_ID_NONE) {
        newHp = g_pGameState->gears[gearId].hp - amount;
        if (newHp <= 0) {
            newHp = 1;
        }
        g_pGameState->gears[gearId].hp = newHp;
    }
}

extern s16 g_FieldNumPartyMembersMasks[4];

void FieldScriptVMHandlerIncreasePartyGearHp(void) {
    int mask;
    int amount;
    int i;

    amount = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    mask = g_FieldNumPartyMembersMasks[SCRIPT_READ_U8_REL(3) & 0x3];

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if ((g_GamePartyMembers[i] != CHARACTER_ID_NONE) && (mask & 1)) {
            FieldPartyMemberIncreaseGearHp(i, amount);
        }
        mask >>= 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptVMHandlerDecreasePartyGearHp(void) {
    int mask;
    int amount;
    int i;

    amount = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    mask = g_FieldNumPartyMembersMasks[SCRIPT_READ_U8_REL(3) & 0x3];

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if ((g_GamePartyMembers[i] != CHARACTER_ID_NONE) && (mask & 1)) {
            FieldPartyMemberDecreaseGearHp(i, amount);
        }
        mask >>= 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

void FieldScriptSetParentActor(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        g_FieldScriptVMCurActor->parentActorId = actorIndex;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

// g_FieldActors is volatile, which stops gcc from filling the delay slot of the
// call below; read the pointer through a non-volatile lvalue to get it back.
#define FIELD_ACTORS (*((FieldActor**)(void*)&g_FieldActors))

void FieldScriptWriteActorFlags1(void) {
    ActorData* pActorData;
    int actorIndex;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pActorData = FIELD_ACTORS[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), pActorData->scriptFlags.flags);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptWriteActorFlags2(void) {
    ActorData* pActorData;
    int actorIndex;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pActorData = FIELD_ACTORS[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), ((u_short*)&pActorData->scriptFlags)[1]);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptWriteActorFlags3(void) {
    ActorData* pActorData;
    int actorIndex;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pActorData = FIELD_ACTORS[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), pActorData->flags);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptWriteActorFlags4(void) {
    ActorData* pActorData;
    int actorIndex;

    actorIndex = FieldScriptVMGetActorIndex(1);
    if (actorIndex != ACTOR_ID_INVALID) {
        pActorData = FIELD_ACTORS[actorIndex].pActorData;
        FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), ((u_short*)&pActorData->flags)[1]);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// TODO: These two handlers could use more semantic names since they're likely used
// in certain situations.
void FieldScriptVMConditionalJump6(unsigned short flag) {
    unsigned short argument = SCRIPT_IMM_ARG(1);
    if (argument & flag) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(4);
    }
}

void FieldScriptVMConditionalJump5(unsigned short flag) {
    unsigned short argument = SCRIPT_IMM_ARG(1);
    if (argument & flag) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = SCRIPT_IMM_ARG(2);
    }
}

void FieldScriptWriteActorDistance(void) {
    int actorIndexA;
    int actorIndexB;
    ActorData* pActorA;
    ActorData* pActorB;
    int distance;

    distance = 0;
    actorIndexA = FieldScriptVMGetActorIndex(3);
    actorIndexB = FieldScriptVMGetActorIndex(4);
    if ((actorIndexA != ACTOR_ID_INVALID) && (actorIndexB != ACTOR_ID_INVALID)) {
        FieldActor* pActors = g_FieldActors;

        pActorA = pActors[actorIndexA].pActorData;
        pActorB = pActors[actorIndexB].pActorData;
        distance = FieldGetVec2Magnitude(
            CONV_TO_GTE(pActorA->position.vx) - CONV_TO_GTE(pActorB->position.vx),
            CONV_TO_GTE(pActorA->position.vz) - CONV_TO_GTE(pActorB->position.vz));
    }
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), distance);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

// The four halves of an actor's scriptFlags and flags words are tested by the script VM.
void FieldScriptJumpIfActorScriptFlags1(void) {
    FieldScriptVMConditionalJump6(((u_short*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[0]);
}

void FieldScriptJumpIfActorScriptFlags2(void) {
    FieldScriptVMConditionalJump6(((u_short*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[1]);
}

void FieldScriptJumpIfActorFlags1(void) {
    FieldScriptVMConditionalJump6(((u_short*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[2]);
}

void FieldScriptJumpIfActorFlags2(void) {
    FieldScriptVMConditionalJump6(((u_short*)g_FieldActors[FieldScriptVMGetActorIndex(3)].pActorData)[3]);
}

void FieldScriptJumpIfCurrentActorScriptFlags1(void) {
    FieldScriptVMConditionalJump5(((u_short*)g_FieldScriptVMCurActor)[0]);
}

void FieldScriptJumpIfCurrentActorScriptFlags2(void) {
    FieldScriptVMConditionalJump5(((u_short*)g_FieldScriptVMCurActor)[1]);
}

void FieldScriptJumpIfCurrentActorFlags1(void) {
    FieldScriptVMConditionalJump5(((u_short*)g_FieldScriptVMCurActor)[2]);
}

void FieldScriptJumpIfCurrentActorFlags2(void) {
    FieldScriptVMConditionalJump5(((u_short*)g_FieldScriptVMCurActor)[3]);
}

void FieldScriptWriteFlagsWord(unsigned int flag) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), (unsigned short)flag);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptWriteCurrentActorScriptFlags1(void) {
    FieldScriptWriteFlagsWord(((u_short*)g_FieldScriptVMCurActor)[0]);
}

void FieldScriptWriteCurrentActorScriptFlags2(void) {
    FieldScriptWriteFlagsWord(((u_short*)g_FieldScriptVMCurActor)[1]);
}

void FieldScriptWriteCurrentActorFlags1(void) {
    FieldScriptWriteFlagsWord(((u_short*)g_FieldScriptVMCurActor)[2]);
}

void FieldScriptWriteCurrentActorFlags2(void) {
    FieldScriptWriteFlagsWord(((u_short*)g_FieldScriptVMCurActor)[3]);
}

// Sets or clears a 16-bit half of the current actor's flags from a script argument.
void FieldScriptModifyActorFlags(void) {
    int argument = SCRIPT_IMM_ARG_ALIGNED(1);

    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            g_FieldScriptVMCurActor->scriptFlags.flags |= argument;
            break;
        case 1:
            g_FieldScriptVMCurActor->scriptFlags.flags |= argument << 16;
            break;
        case 2:
            g_FieldScriptVMCurActor->flags |= argument;
            break;
        case 3:
            g_FieldScriptVMCurActor->flags |= argument << 16;
            break;
        case 4:
            g_FieldScriptVMCurActor->scriptFlags.flags &= ~argument;
            break;
        case 5:
            g_FieldScriptVMCurActor->scriptFlags.flags &= ~(argument << 16);
            break;
        case 6:
            g_FieldScriptVMCurActor->flags &= ~argument;
            break;
        case 7:
            g_FieldScriptVMCurActor->flags &= ~(argument << 16);
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

// Fills values[0..count-1] with distinct random numbers in [0, unk4] and bumps
// them by one, so 0xFFFF becomes 0. D_800B2298/D_800B229C are splat's split of
// the same storage (see FieldScriptSetRandomSkinParams, the script handler that drives this).
// The original reads unk4 and count through those two symbols with offset 0
// while the array is based at D_800B2294 + 0xC, so this C is one symbol name
// apart from a 100% match (objdiff 99.26%, same instructions and bytes; the
// relocations resolve to the same addresses). Declaring D_800B2298/D_800B229C
// as structs to fix the names instead makes gcc hoist D_800B229C's address out
// of the branch in FieldScriptSetRandomSkinParams, because the two symbols no longer share an
// alias set, and that function's bytes change.
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 count;
    /* 0xC */ u16 values[0x20];
} FieldRandomValues; // Size: 0x4C

extern FieldRandomValues D_800B2294;
extern int rand(void);

void FieldRandomizeSkinSlots(void) {
    int i;
    int j;
    int value;
    u16 emptyValue;

    D_800B2294.unk0 = D_800B2294.unk4;
    if (D_800B2294.count == 0) {
        D_800B2294.unk4 = 0;
        return;
    }
    emptyValue = 0xFFFF;
    for (i = 0x1F; i >= 0; i--) {
        D_800B2294.values[i] = emptyValue;
    }
    for (i = 0; i < D_800B2294.count; i++) {
retry:
        value = ((rand() * (D_800B2294.unk4 + 1)) >> 15) & 0xFFFF;
        for (j = 0; j < 0x20; j++) {
            if (D_800B2294.values[j] == value) {
                goto retry;
            }
        }
        D_800B2294.values[i] = value;
    }
    for (i = 0; i < D_800B2294.count; i++) {
        D_800B2294.values[i]++;
    }
}

extern FieldScriptGlobalS32 D_800B2298;
extern FieldScriptGlobalS32 D_800B229C;

void FieldScriptSetRandomSkinParams(void) {
    int count;

    D_800B2298.value = FieldScriptVMGetArgument(1);
    count = FieldScriptVMGetArgument(3);
    D_800B229C.value = count;
    if (count >= 0x21) {
        D_800B229C.value = 0x20;
    }
    FieldRandomizeSkinSlots();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptSetActorMovementFlag(void) {
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            if (g_FieldScriptVMCurActor->scriptFlags.flags & 0x8000) {
                g_FieldScriptVMCurActor->scriptFlags.flags &= ~0x8000;
            }
            if (g_FieldScriptVMCurActor->flags & 0x80000) {
                SpriteData* pSpriteData = g_FieldActors[D_800AFD1C].pSpriteData;

                pSpriteData->moveSpeed = 0;
                pSpriteData->step.z = 0;
                pSpriteData->step.x = 0;
                g_FieldScriptVMCurActor->flags &= ~0x80000;
            }
            break;
        case 1:
            g_FieldScriptVMCurActor->scriptFlags.flags |= 0x8000;
            g_FieldScriptVMCurActor->unk11C = *(u_short*)&g_FieldScriptVMCurActor->rotation.vy;
            break;
        case 2:
            g_FieldScriptVMCurActor->flags |= 0x80000;
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s32 D_800ADB7C;

void FieldScriptWaitForFlagADB7C(void) {
    if (D_800ADB7C == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    } else {
        D_800ADB7C = 0;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
    D_800B00C0 = 1;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008EA58);

INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008EC30);

extern s16 D_800C3A20;
extern s16 D_800C3A22;
extern s16 D_800C3A24;
extern s16 D_800C3A26;
extern s16 D_800C3A28;
extern s16 D_800C3A2A;
extern s16 D_800C3A2C;
extern s16 D_800C3A2E;
extern u16 D_800C3A30;
extern u16 D_800C3A32;
extern u16 D_800C3A34;
extern u16 D_800C3A36;
extern u16 D_800C3A38;
extern u16 D_800C3A3A;
extern s32 D_800ADB70;
extern s32 D_800ADB74;
extern s32 D_800ADB80;

// 79.22%: everything up to the last four stores matches. The original's
// post-reload scheduler emits them as [lhu D_800C3A30][lw g_FieldScriptVMCurActor]
// [sh D_800C3A36][andi/sh D_800C3A30][lhu ip][li 1][sw D_800ADB70][addiu/sh ip];
// ours puts the D_800ADB70 store before the actor load and the ip load after it.
// preset screen: 2.7.2 79.22%, cdk 79.22%, 2.6.0 79.22% - no compiler
// fixes it.
/*
void func_8008EE14(void) {
    u16 value = FieldScriptVMGetArgument(9);
    s16 pos;

    D_800C3A20 = FieldScriptVMGetArgument(1);
    D_800C3A2A = FieldScriptVMGetArgument(3);
    D_800C3A2C = FieldScriptVMGetArgument(5);
    D_800C3A2E = FieldScriptVMGetArgument(7);
    D_800C3A30 = value;
    if (value == 0xFE) {
        D_800C3A3A = 1;
    } else {
        D_800C3A3A = 0;
    }
    pos = FieldScriptVMGetArgument(0xB);
    D_800C3A22 = pos;
    D_800C3A26 = pos;
    pos = FieldScriptVMGetArgument(0xD);
    D_800C3A24 = pos;
    D_800C3A28 = pos;
    D_800C3A32 = FieldScriptVMGetArgument(0xF);
    D_800C3A34 = FieldScriptVMGetArgument(0x11);
    D_800ADB80 = value & 0x40;
    D_800C3A38 = 0xFF;
    D_800ADB74 = 2;
    D_800C3A30 &= 0xF;
    D_800C3A36 = 0;
    D_800ADB70 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x13;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008EE14);

extern s32 D_800ADB3C;
extern s32 D_800ADB38;

void FieldScriptStartFade1(void) {
    D_800ADB3C = FieldScriptVMGetArgument(1);
    D_800ADB38 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptStartFade2(void) {
    D_800ADB3C = FieldScriptVMGetArgument(1);
    D_800ADB38 = 2;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetClipDimensions(void) {
    FieldSetClipDimensions(
        FieldScriptVMGetArgument(1),
        FieldScriptVMGetArgument(3),
        FieldScriptVMGetArgument(5),
        FieldScriptVMGetArgument(7)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptStartFade3(void) {
    D_800ADB3C = FieldScriptVMGetArgument(1);
    D_800ADB38 = 3;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetActorColors(void) {
    ActorData* pActorData;
    int actorIndex = FieldScriptVMGetActorIndex(2);

    if (actorIndex != ACTOR_ID_INVALID) {
        pActorData = g_FieldActors[actorIndex].pActorData;
        if (SCRIPT_READ_U8_REL(1) & 1) {
            pActorData->unkFC = FieldScriptVMGetArgument(3);
            pActorData->unkFD = FieldScriptVMGetArgument(5);
            pActorData->unkFE = FieldScriptVMGetArgument(7);
        }
        if (SCRIPT_READ_U8_REL(1) & 2) {
            pActorData->unkFF = FieldScriptVMGetArgument(3);
            pActorData->unk100 = FieldScriptVMGetArgument(5);
            pActorData->unk101 = FieldScriptVMGetArgument(7);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptSetCurrentActorColors(void) {
    if (SCRIPT_READ_U8_REL(1) & 1) {
        g_FieldScriptVMCurActor->unkFC = FieldScriptVMGetArgument(2);
        g_FieldScriptVMCurActor->unkFD = FieldScriptVMGetArgument(4);
        g_FieldScriptVMCurActor->unkFE = FieldScriptVMGetArgument(6);
    }
    if (SCRIPT_READ_U8_REL(1) & 2) {
        g_FieldScriptVMCurActor->unkFF = FieldScriptVMGetArgument(2);
        g_FieldScriptVMCurActor->unk100 = FieldScriptVMGetArgument(4);
        g_FieldScriptVMCurActor->unk101 = FieldScriptVMGetArgument(6);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

extern void func_80023290(void*, int);

void FieldScriptSetSpriteDirection(void) {
    func_80023290(g_FieldActors[D_800AFD1C].pSpriteData, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 D_800C3A5C;
extern s32 D_800C3A60;

void FieldScriptSetScreenScroll(void) {
    D_800C3A5C = FieldScriptVMGetArgument(1);
    D_800C3A60 = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern FieldScriptGlobalS16 D_800B233C;

void FieldScriptSetSoundSlotFlags(void) {
    D_800B233C.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void func_8003A450(int, int, int);

void FieldScriptSetSoundElementVolumeFade(void) {
    func_8003A450(FieldScriptVMGetArgument(3) * 2, FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(5));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern void func_8003A344(int, int);

void FieldScriptSetSoundElementLevel(void) {
    func_8003A344(FieldScriptVMGetArgument(3) * 2, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern void func_8003A55C(int, int);

void FieldScriptSetSoundElementVolume(void) {
    func_8003A55C(FieldScriptVMGetArgument(3) * 2, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptSetSoundSlotVolume(void) {
    FieldSetSoundSlotElement(FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(3));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptSetSoundSlotVolumeParams(void) {
    func_800855C8(
        FieldScriptVMGetArgument(1),
        FieldScriptVMGetArgument(5),
        FieldScriptVMGetArgument(3),
        FieldScriptVMGetArgument(7)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

extern int func_8003A5D0(int);

void FieldScriptWaitSoundElementStopped(void) {
    int inputMask = func_8003A5D0(-1);

    if ((inputMask & (FieldScriptVMGetArgument(1) << 8)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    D_800B00C0 = 1;
}

extern void FieldSetSoundSlotElement(int, int);

void FieldScriptSetSoundSlotVolume3(void) {
    FieldSetSoundSlotElement(FieldScriptVMGetArgument(1), 3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void func_800855C8(int, int, int, int);

void FieldScriptSetSoundSlotVolumeParams3(void) {
    func_800855C8(FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(5), FieldScriptVMGetArgument(3), 3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern FieldScriptGlobalS32 D_800ADBDC;
extern FieldScriptGlobalS32 D_8004F340;

void FieldScriptLoadCharacterData0(void) {
    if (D_800ADBDC.value == 0) {
        D_800B00C0 = 1;
    } else {
        D_8004F340.value = 0;
        func_8008F7B8();
    }
}

void FieldScriptLoadCharacterDataMinus1(void) {
    if (D_800ADBDC.value == 0) {
        D_800B00C0 = 1;
    } else {
        D_8004F340.value = -1;
        func_8008F7B8();
    }
}

extern s32 D_8004F354;
extern void FieldFreePendingAudioManager(void);
extern void func_8001B66C(void);
extern void FieldStartWdsBankLoad(int, int);

// 92.24%: the two guards match as `!=` conjunctions (both `beq` jump forward to
// the shared `D_800B00C0 = 1`), but then gcc merges that store with the one for
// the `FieldArchiveTryDataSync() != 0 || D_800ADBDC.value == 0` arm, which the original
// keeps separate and inline. Spelling that arm as `== 1 || == -1` keeps it
// inline (it shares the `li v1,1`) but inverts the second `beq` (90.82%).
// preset screen: 2.7.2 92.24%, cdk 92.24%, 2.6.0 92.24% - no compiler
// fixes it.
/*
void func_8008F7B8(void) {
    int characterId = FieldScriptVMGetArgument(1);

    if (D_800ADB1C == 0) {
        FieldFreePendingAudioManager();
        if (characterId != D_8004F324) {
            func_8001B66C();
            D_8004F308 = -1;
        }
        D_8004F324 = characterId;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else if (FieldArchiveTryDataSync() != 0 || D_800ADBDC.value == 0) {
        D_800B00C0 = 1;
    } else if (D_8004F354 != 1 && D_8004F308 != -1) {
        if (characterId != D_8004F324) {
            func_8001B66C();
            D_8004F324 = characterId;
            D_8004F308 = -1;
            FieldStartWdsBankLoad(characterId, 0);
        }
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        D_800B00C0 = 1;
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008F7B8);

// 90.60%: everything matches except that gcc's post-reload scheduler moves the
// `scriptInstructionPointer += 9` load/addiu/store below the three divisions and
// the two g_Scene s16 stores (the original has it right after the frames clamp).
// Writing the clamp first in the source makes it worse: the divs and the s16
// stores are then all emitted before the pointer store (48.80%).
// preset screen: 2.7.2 90.60%, cdk 72.87%, 2.6.0 90.60% - no compiler
// fixes it.
/*
// Starts an interpolation of the camera position from the current g_Scene
// position to the one passed in the script arguments.
void func_8008F90C(void) {
    int x = FieldScriptVMGetArgument(1);
    int y = FieldScriptVMGetArgument(3);
    int z = FieldScriptVMGetArgument(5);
    int frames = FieldScriptVMGetArgument(7);

    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
    if (frames == 0) {
        frames = 1;
    }
    *(s32*)&g_Scene.unk98[0x14] = ((x << 16) - *(s32*)&g_Scene.unk98[0x8]) / frames;
    *(s32*)&g_Scene.unk98[0x18] = ((z << 16) - *(s32*)&g_Scene.unk98[0xC]) / frames;
    *(s32*)&g_Scene.unk98[0x1C] = ((y << 16) - *(s32*)&g_Scene.unk98[0xC]) / frames;
    *(s16*)&g_Scene.unk98[0x2] = frames;
    *(s16*)&g_Scene.unk98[0x0] = 1;
    if (x == 0 && z == 0 && y == 0) {
        *(s16*)&g_Scene.unk98[0x2] = frames + 2;
        *(s16*)&g_Scene.unk98[0x4] = 1;
    } else {
        *(s16*)&g_Scene.unk98[0x4] = 0;
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008F90C);
