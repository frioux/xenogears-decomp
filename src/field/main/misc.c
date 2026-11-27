#include "common.h"
#include "main/game.h"
#include "system/math.h"
#include "system/archive.h"
#include "system/sound.h"
#include "field/main.h"
#include "field/camera.h"
#include "field/actor.h"
#include "field/script_vm.h"
#include "field/text_box.h"
#include "field/particles.h"
#include "field/effects.h"

// The field's display window: 10 lines in from the top of the 320x224
// framebuffer, and narrower than it.
#define FIELD_SCREEN_DISPLAY_Y 10
#define FIELD_SCREEN_DISPLAY_WIDTH 256
#define FIELD_SCREEN_DISPLAY_HEIGHT 216

// Number of skin ids g_FieldRandomSkinSlots.values holds, and the largest count
// a script may ask FieldRandomizeSkinSlots for.
#define FIELD_RANDOM_SKIN_SLOT_COUNT 32

// What every slot of g_FieldRandomSkinSlots.values is reset to before the distinct skin
// ids are drawn, so a slot still holding it was never filled.
#define FIELD_RANDOM_SKIN_SLOT_EMPTY 0xFFFF

// g_GameSceneMapNum's top two bits: the map runs with the party in its gears, so a
// party member's gear skin is loaded instead of its character skin.
#define FIELD_MAP_GEAR_SKINS_MASK 0xC000

// Field archive entry a party member's skin file sits at, above its skin id.
#define FIELD_PARTY_SKIN_ARCHIVE_OFFSET 5

// Field archive entry an actor's special animation file sits at, above its animation id.
#define FIELD_SPECIAL_ANIM_ARCHIVE_OFFSET 0x77A

// The archive directories a script's WDS sound bank is read from: the field's own, and
// the second one a bank argument's high bit selects (battle reads its sound archive
// through that same directory).

// That high bit of a WDS bank argument; the bits below it are the bank index.
#define FIELD_WDS_BANK_ALT_ARCHIVE_MASK 0x80
#define FIELD_WDS_BANK_INDEX_MASK 0x7F

// Gear skins sit this far above the character skin ids in the skin id space
// g_GamePartyMemberSkins holds.
#define FIELD_GEAR_SKIN_ID_OFFSET 0x10

// Written to the slot a script asked for when the actor is in no party slot.
#define FIELD_PARTY_SLOT_NONE 0xFF

// One party member's saved 320x128 8bpp framebuffer, the block a slot change copies.
#define FIELD_PARTY_FRAMEBUFFER_SIZE 0x14000

// FieldActorsWithinDistance reports false once its two actors' 2D distance reaches this.
#define FIELD_ACTOR_DISTANCE_THRESHOLD 0x10

// The particle bank's range mode, in ParticleBank::flags bits 6-7 (see
// PARTICLE_FLAG_RANGE_MODE): a script argument of 1 sprays in a circle, 2 along a line.
#define FIELD_PARTICLE_BANK_RANGE_MODE_CIRCLE 0x80
#define FIELD_PARTICLE_BANK_RANGE_MODE_LINE 0x40

// The bits of the script byte that pick which of an actor's two color triples a
// color handler writes: bit 1 red/green/blue, bit 2 red2/green2/blue2.
#define FIELD_ACTOR_COLOR_SET_PRIMARY 0x1
#define FIELD_ACTOR_COLOR_SET_SECONDARY 0x2

// scriptFlags bits a script handler sets by hand: the actor the player drives, and
// the facing latched into ActorData::lockedRotationY.

// Field archive entry a WDS bank sits at, above its bank index: the field WDS
// directory's own, and the battle sound directory's.
#define FIELD_WDS_BANK_ARCHIVE_OFFSET 2
#define FIELD_WDS_BATTLE_BANK_ARCHIVE_OFFSET 0x1F

void FieldScriptMemoryWriteU16(int, int);

extern int rcos(int);
extern int rsin(int);

extern s32 g_FieldCurActorIndex;
extern s32 g_FieldPartyActorSlots[MAX_PARTY_MEMBERS];

void FieldSetScreenDimensions(void) {
    g_FieldRenderContexts[0].dispEnv.screen.x = 0;
    g_FieldRenderContexts[0].dispEnv.screen.y = FIELD_SCREEN_DISPLAY_Y;
    g_FieldRenderContexts[0].dispEnv.screen.w = FIELD_SCREEN_DISPLAY_WIDTH;
    g_FieldRenderContexts[0].dispEnv.screen.h = FIELD_SCREEN_DISPLAY_HEIGHT;
    g_FieldRenderContexts[1].dispEnv.screen.x = 0;
    g_FieldRenderContexts[1].dispEnv.screen.y = FIELD_SCREEN_DISPLAY_Y;
    g_FieldRenderContexts[1].dispEnv.screen.w = FIELD_SCREEN_DISPLAY_WIDTH;
    g_FieldRenderContexts[1].dispEnv.screen.h = FIELD_SCREEN_DISPLAY_HEIGHT;
}

extern u8 g_FieldPauseDisabled[]; // Is pause disabled?

void FieldScriptSetPauseDisabled(void) {
    g_FieldPauseDisabled[0] = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s16 g_FieldScreenStripSpritesEnabled;

void FieldScriptSetScreenMode(void) {
    RECT rect;
    switch (FieldScriptVMGetArgument(1)) {
        case 0:
            rect.w = 0x500;
            rect.x = 0;
            rect.y = 0;
            rect.h = 0x200;
            ClearImage(&rect, 0, 0, 0);
            DrawSync(0);
            Vsync(0);
            SetDefDrawEnv(g_FieldRenderContexts[0].drawEnvs, 0, 0, 0x280, SCREEN_HEIGHT);
            SetDefDrawEnv(g_FieldRenderContexts[1].drawEnvs, 0, 0x100, 0x280, SCREEN_HEIGHT);
            SetDefDispEnv(&g_FieldRenderContexts[0].dispEnv, 0, 0x100, 0x280, SCREEN_HEIGHT);
            SetDefDispEnv(&g_FieldRenderContexts[1].dispEnv, 0, 0, 0x280, SCREEN_HEIGHT);
            FieldSetScreenDimensions();
            break;
        case 1:
            g_FieldScreenStripSpritesEnabled = 1;
            break;
        case 2:
            g_FieldScreenStripSpritesEnabled = 0;
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
            FieldInitSpriteList();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            return;
        case 2:
            // Free Sprite List
            FieldFreeSpriteList();
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
            return;
        case 1:
            // Set X, Y of Sprite and add it to OT for drawing
            FieldDrawSprite(
                FieldScriptVMGetArgument(2), // Sprite Index
                FieldScriptVMGetArgument(4), // X
                FieldScriptVMGetArgument(6), // Y
                FieldScriptVMGetArgument(8)  // Sprite type
            );
            g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
            return;
        case 3:
            // Set Sprite Color
            FieldSetSpriteColor(
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

    g_pGameState->conditionRows[index].statusFlags |= value;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern s32 D_800C3A48;
extern s32 D_800AF87C;
extern RECT D_800AFC58[];
extern s32 D_800ADBB4;
extern void func_80026F44(s32 count, s32 level, u16* pDst, u16* pSrc);

// Captures or restores the screen region the script instruction describes: 0 grabs
// it into two freshly allocated buffers, 1 copies one back with func_80026F44, 2
// frees the buffers, 3 only advances the pointer.
//
// The dispatch must be a switch: an if/else chain compiles to a linear compare
// chain (77.68%) while retail has gcc's comparison tree, and there is no jtbl_
// relocation. Two shapes are load-bearing: case 1's shift has to stay a separate
// operand (`offset = FieldScriptVMGetArgument(2);` then `(offset << 9)` at both
// calls, or the copy folds into the `sll` and the body loses 4 bytes), and the rect
// stores need D_800AFC58 as an unbounded RECT array (a scalar RECT keeps the
// `lui at / sh %lo` form, and four s16 externs cannot produce retail's
// `sh zero,0(a0)`).
//
// PARKED 99.59% (532 bytes: retail's exact size and instruction multiset, one
// allocator tie). Case 0 keeps argument 2 (the y field) in $s1 and argument 4 (the
// height) in $s2 in retail; this build emits the mirror image at every use - the
// sll, the rect.y store and the rect.h store - with the instruction positions
// identical. h is referenced once more than y (the sll plus rect.h), so it wins
// global_alloc's priority contest (floor_log2(refs) * refs / live_length) here and
// loses it in retail. Unmoved by: either declaration order, a separate `size`
// local versus `h << 9` written twice, `u32 h`, `u16 y,h` (536 bytes), `s16
// offset` in case 1 (98.23% / 540 bytes), and two permuter runs (base 25 best 25;
// an earlier one reached 243 and proposed only the s16 shape).
// Re-measured on this body (round 414): 99.59% / 532 bytes exactly as above;
// gcc-2.6.0-psx 95.56 and gcc-2.7.2-cdk-psx 94.54, so the preset is not the
// cause; `s32 h; s32 y;` (declaration order swapped, calls untouched) is
// instruction-identical to `s32 y; s32 h;`, so the pseudo's declaration order
// does not decide the tie either; `s16 h` costs the sll/sra pair (536 bytes,
// 98.80); an empty `do { } while (0)` around the two HeapAlloc statements moves
// the D_800AF87C store out of its slot (98.53); and a third permuter run on this
// body is PERMUTER_NONE base=25 best=25 over 718 iterations.
// Round 428: re-measured 99.59% / 532 bytes, 133 insns, 8 differing lines (both
// moves, the sll, the two rect stores). The `size` local was measured again as a
// separate `s32 size = h << 9;` with both HeapAllocs taking it: still 99.59 and
// still the mirror, because h keeps TWO references either way (the sll and the
// rect.h store), so the refs term of the priority quotient cannot be moved from
// source - y has one reference and therefore a priority of 0 (floor_log2(1)), so
// the pair can only flip if h's term is 0 too, which needs h referenced once,
// which the rect.h store forbids. That is the levers.md quotient verdict
// (a-global-allocno-priority-is) applied to this body: stop permuting it.
/*
void func_800871B0(void) {
    u8 command = SCRIPT_READ_U8_REL(1);

    switch (command) {
    case 0: {
        s32 y;
        s32 h;

        g_FieldScriptMaxInstructionCount += 0x20;
        y = FieldScriptVMGetArgument(2);
        h = FieldScriptVMGetArgument(4);
        D_800C3A48 = HeapAlloc(h << 9, 0);
        D_800AF87C = HeapAlloc(h << 9, 0);
        D_800AFC58[0].x = 0;
        D_800AFC58[0].y = y;
        D_800AFC58[0].w = 0x100;
        D_800AFC58[0].h = h;
        StoreImage(&D_800AFC58[0], (void*)D_800C3A48);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
        g_FieldScriptVMStopFlag = 1;
        break;
    }
    case 1: {
        s32 offset;

        g_FieldScriptMaxInstructionCount += 0x20;
        offset = FieldScriptVMGetArgument(2);
        func_80026F44(0x100, FieldScriptVMGetArgument(4), (u16*)(D_800AF87C + (offset << 9)),
                      (u16*)(D_800C3A48 + (offset << 9)));
        D_800ADBB4 = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
        break;
    }
    case 2:
        HeapFree((void*)D_800C3A48);
        HeapFree((void*)D_800AF87C);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        g_FieldScriptVMStopFlag = 1;
        break;
    case 3:
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        g_FieldScriptVMStopFlag = 1;
        break;
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800871B0);

extern u8 g_FieldActorModelSpriteRow[];

void FieldScriptSetCharacterByte(void) {
    int characterId = FieldScriptVMGetArgument(1);
    g_FieldActorModelSpriteRow[characterId] = FieldScriptVMGetArgument(3);
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

void FieldScriptCopyCharacter(void) {
    GameState* pGameState;
    int sourceIndex = FieldScriptVMGetArgument(1);
    int destIndex = FieldScriptVMGetArgument(3);

    g_pGameState->characters[destIndex] = g_pGameState->characters[sourceIndex];
    *(UnkGearState*)&g_pGameState->conditionRows[destIndex].commandMask =
        *(UnkGearState*)&g_pGameState->conditionRows[sourceIndex].commandMask;
    pGameState = g_pGameState; // reused for the first flag write only; the second reloads it
    if (destIndex == 9) {
        pGameState->unk22B6 |= 0x2000;
    }
    if (destIndex == 10) {
        g_pGameState->unk22B6 |= 0x1000;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern u8 g_BattleResultCode;

void FieldScriptWriteTransitionState(void) {
    FieldScriptMemoryWriteU16(
        SCRIPT_IMM_ARG(1), 
        g_BattleResultCode
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

extern FieldScriptGlobalS32 g_FieldMapChangeGate;
extern int g_FieldArchiveStreamActive;
extern s32 g_GameWdsBankLoadState;
extern s32 g_FieldTransitionBusyFlag;
extern s32 g_FieldStagedFileSyncHold;
extern u8 g_BattleTransitionFlag;
extern u8 g_BattleRequestedMode;
extern u8 D_8005061E;
extern u8 D_8005061F;
extern u8 g_BattleRequestedMatchCount;
extern u8 g_BattleRequestedDifficultyLevel;
extern void func_800379B4(int);

void FieldScriptSetTransitionParams(void) {
    if (g_FieldMapChangeGate.value == 0 || g_FieldEntryStateGate == 0 || g_FieldArchiveStreamActive != 0 || g_GameWdsBankLoadState == -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        g_FieldScriptVMStopFlag = 1;
    } else {
        func_800379B4(0);
        g_BattleTransitionFlag = FieldScriptVMGetArgument(1);
        g_BattleRequestedMode = FieldScriptVMGetArgument(3);
        D_8005061E = FieldScriptVMGetArgument(5);
        D_8005061F = FieldScriptVMGetArgument(7);
        g_BattleRequestedMatchCount = FieldScriptVMGetArgument(9);
        g_BattleRequestedDifficultyLevel = FieldScriptVMGetArgument(0xB);
        g_FieldTransitionBusyFlag = 1;
        g_FieldStagedFileSyncHold = 0;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0xD;
    }
}

void FieldScriptWriteGameState17FC48(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_pGameState->playerMapPosX);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), g_pGameState->playerMapPosZ);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptWriteGameState184E(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_pGameState->mapPosX);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), g_pGameState->mapPosZ);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern FieldScriptGlobalU8 g_FieldFogDisabled;

void FieldScriptSetByte2357(void) {
    g_FieldFogDisabled.value = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern FieldScriptGlobalU8 g_FieldAlternatePadAngles;

void FieldScriptSetByte2354(void) {
    g_FieldAlternatePadAngles.value = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetGameState184E(void) {
    g_pGameState->mapPosX = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    g_pGameState->mapPosZ = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    g_pGameState->playerPlaced = 0;
    g_pGameState->mapPosZFraction = 0;
    g_pGameState->unk1856 = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void FieldScriptWriteGameState17FC30(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), *(u_short*)&g_pGameState->scriptPosX);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(2), *(u_short*)&g_pGameState->scriptPosY);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(3), *(u_short*)&g_pGameState->scriptPosZ);
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(4), g_pGameState->scriptAngle);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

extern s32 g_FieldTextRenderEnabled;

void FieldScriptSetEffectFlag4F300(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    g_FieldTextRenderEnabled = 1;
}

void FieldScriptSetGameState17FC30(void) {
    ActorData* pActor;
    s16 value;

    value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    pActor = g_FieldScriptVMCurActor;
    g_pGameState->scriptPosX = value;
    value = FieldScriptArgument2(3, SCRIPT_READ_U8(pActor->scriptInstructionPointer + 9));
    pActor = g_FieldScriptVMCurActor;
    g_pGameState->scriptPosY = value;
    value = FieldScriptArgument3(5, SCRIPT_READ_U8(pActor->scriptInstructionPointer + 9));
    pActor = g_FieldScriptVMCurActor;
    g_pGameState->scriptPosZ = value;
    value = FieldScriptArgument4(7, SCRIPT_READ_U8(pActor->scriptInstructionPointer + 9));
    pActor = g_FieldScriptVMCurActor;
    g_pGameState->scriptAngle = value;
    pActor->scriptInstructionPointer += 0xA;
}

void FieldScriptWriteGameState17FC38(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_pGameState->worldMapState);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetGameState17FC38(void) {
    int value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(3));
    ActorData* pActor = g_FieldScriptVMCurActor;

    g_pGameState->worldMapState = value;
    pActor->scriptInstructionPointer += 4;
}

extern FieldScriptGlobalU8 g_FieldWdsSoundEffectBankIndex;
extern FieldScriptGlobalU8 g_FieldScriptSoundEffectBank;

void FieldScriptSetByte2355Or2356(void) {
    int value = FieldScriptVMGetArgument(2);

    if (SCRIPT_READ_U8_REL(1) == 0) {
        g_FieldWdsSoundEffectBankIndex.value = value;
    } else {
        g_FieldScriptSoundEffectBank.value = value;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

extern FieldScriptGlobalS16 g_FieldPlayerWalkmeshMaterialOverride;

void FieldScriptSetByte234C(void) {
    g_FieldPlayerWalkmeshMaterialOverride.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 g_FieldActiveActorCount;
extern s32 g_PlayerActorIndex;
extern FieldScriptGlobalU16 g_FieldPlayerActorPartySlot;
extern FieldScriptGlobalS16 g_FieldPlayerActorIndex16;

// Switch the actor controlled by the player
void FieldScriptSetPlayerActor(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);

    if (actorIndex != ACTOR_ID_INVALID) {
        int i;

        if (actorIndex == g_FieldPartyActorSlots[0]) {
            g_FieldPlayerActorPartySlot.value = 0;
        } else {
            g_FieldPlayerActorPartySlot.value = 1;
        }
        g_PlayerActorIndex = actorIndex;
        g_FieldPlayerActorIndex16.value = actorIndex;
        for (i = 0; i < g_FieldActiveActorCount; i++) {
            g_FieldActors[i].pActorData->scriptFlags.flags &= ~0x01004000;
        }
        g_FieldActors[actorIndex].pActorData->scriptFlags.flags |= FIELD_SCRIPT_FLAG_PLAYER_ACTOR;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern FieldScriptGlobalU16 g_FieldWalkAbortCount[];

// 79.83%: gcc folds the symbol into both accesses (`lui/lhu` then `lui at/sh`),
// the original materialises &g_FieldWalkAbortCount once in $v1 and uses 0($v1) twice. A
// local pointer to the global (`p->value++`, also `+= 1`, `volatile` variants)
// is folded back to the same immediate form. Also 19.92% with a cast pointer
// (`u16* p = (u16*)&g_FieldWalkAbortCount; (*p)++;`): gcc still folds and allocates worse.
// The `lui+addiu` form appears where the C uses the symbol's address as a VALUE
// (cf. FieldClearParticleBankDirections's `$a2 = &g_FieldDefaultParticleBanks`), and a deref of
// `&sym` never survives fold, so a pointer local is not enough.
// preset screen: 2.7.2 79.83%, cdk 79.83%, 2.6.0 79.83% - no compiler
// fixes it.
void FieldScriptVMHandlerAbortWalk(void) {
    g_FieldWalkAbortCount[0].value++;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

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

extern s32 g_GameWdsBankLoadState;

void FieldScriptWaitForDataLoadPending(void) {
    if (g_GameWdsBankLoadState == -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

void FieldScriptWriteCharacterGearId(void) {
    int partyId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(1));

    if (partyId != CHARACTER_ID_NONE) {
        FieldScriptMemoryWriteU16(
            FieldScriptVMGetInstructionArgument(3) & 0xFFFF, g_pGameState->characters[partyId].gearId);
    } else {
        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, GEAR_ID_NONE);
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
            g_pGameState->partyMemberFlags |= 1 << partyId;
        } else {
            g_pGameState->partyMemberFlags &= ~(1 << partyId);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
}

extern FieldScriptGlobalS16 g_FieldScriptShopIndex;

// Sets the shop index a script step selects to the inverse of its argument's low
// bit (0/1), then steps over the instruction.
void FieldScriptVMAdvanceInstruction(void) {
    int value = SCRIPT_READ_U8_REL(1);

    // volatile: retail loads the instruction pointer a second time for the step,
    // and gcc 2.7.2 folds the two reads into one without it.
    *(volatile u16 *)&g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    g_FieldScriptShopIndex.value = value ^ 1;
}

extern FieldScriptGlobalS16 g_FieldSoundMaxDistance;

void FieldScriptSetByte21AC(void) {
    g_FieldSoundMaxDistance.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptWritePartySpriteFrame(void) {
    SpriteData* pSpriteData;
    int partyMemberId;
    int address;

    partyMemberId = g_FieldPartyActorSlots[FieldScriptVMGetArgument(5)];
    g_FieldScriptMaxInstructionCount += 4;
    if (partyMemberId != ACTOR_ID_INVALID) {
        pSpriteData = g_FieldActors[partyMemberId].pSpriteData;
        // empty statement: without it gcc hoists the `li a0,1` out of the call's delay slot
        do { } while (0);
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

typedef struct {
    /* 0x0 */ int bankIndex;
    /* 0x4 */ u8 unk4[0xC];
} ParticleBankHandle;

extern ParticleBankHandle g_FieldScriptParticleBank;

// Zeroes the .z (and .x) of every direction in the script's current bank.
// The .z store keeps an INDEX-FIRST sum added to a base held as a value: the
// typed `directions[i].z` spellings commute that sum and fold the 0x30 field
// offset into `%hi` instead of retail's `%lo` + `sh 0x32`, and measure 38.77%,
// 80.68% (pointer to the bank) and 82.27% (pointer to the array) against
// retail's 88 bytes. Retail's own .x store two instructions later is the folded
// `%hi/%lo(base + 0x30)` form, so the asymmetry is retail's and the .z store
// stays an integer-offset cast.
// Both pointer locals live inside the loop: hoisted, the address load comes
// before `i = 0` and the block is 8 bytes longer.
void FieldClearParticleBankDirections(void) {
    int i;

    for (i = 0; i < NUM_PARTICLE_BANKS; i++) {
        int bankIndex = g_FieldScriptParticleBank.bankIndex;
        ParticleBank* pBanks = g_FieldDefaultParticleBanks;

        *(s16*)((bankIndex * sizeof(ParticleBank) + i * sizeof(ParticleBankAngleOffset)) + (mem_addr)pBanks
                + offsetof(ParticleBank, directions[0].z)) = 0;
        g_FieldDefaultParticleBanks[bankIndex].directions[i].x = 0;
    }
}

extern s32 g_FieldParticleBankTargetActor;
extern s32 g_FieldParticleBankModeBits;
extern s32 g_FieldParticleBankParam72;
extern s32 g_FieldParticleBankParam74;
extern void FieldInitializeDefaultParticleBanks(int);

void FieldScriptInitializeParticleSystem(void) {
    int actorIndex = FieldScriptVMGetArgument(1);

    if (actorIndex == ACTOR_ID_INVALID) {
        actorIndex = 0;
    }
    g_FieldParticleBankTargetActor = FieldScriptVMGetArgument(1);
    g_FieldParticleBankModeBits = FieldScriptVMGetArgument(3);
    g_FieldParticleBankParam72 = FieldScriptVMGetArgument(5);
    ((FieldScriptGlobalS32*)&g_FieldParticleBankParam74)->value = FieldScriptVMGetArgument(7);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
    FieldInitializeDefaultParticleBanks(actorIndex);
    switch (g_FieldParticleBankModeBits) {
        case 0:
            g_FieldParticleBankModeBits = 0;
            break;
        case 1:
            g_FieldParticleBankModeBits = 0x10;
            break;
        case 2:
            g_FieldParticleBankModeBits = 0x20;
            break;
        case 3:
            g_FieldParticleBankModeBits = 0x30;
            break;
    }
    g_FieldScriptMaxInstructionCount += 4;
}

void FieldScriptInitializeParticleSystemByActorIndex(void) {
    int actorIndex = FieldScriptVMGetActorIndex(1);

    if (actorIndex == ACTOR_ID_INVALID) {
        actorIndex = 0;
    }
    g_FieldParticleBankTargetActor = actorIndex;
    g_FieldParticleBankModeBits = FieldScriptVMGetArgument(2);
    g_FieldParticleBankParam72 = FieldScriptVMGetArgument(4);
    // Store through FieldScriptGlobalS32: a bare s32 store lets gcc hoist the
    // instruction pointer load above it (see the note above FieldScriptGlobalS32).
    ((FieldScriptGlobalS32*)&g_FieldParticleBankParam74)->value = FieldScriptVMGetArgument(6);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    FieldInitializeDefaultParticleBanks(actorIndex);
    switch (g_FieldParticleBankModeBits) {
        case 0:
            g_FieldParticleBankModeBits = 0;
            break;
        case 1:
            g_FieldParticleBankModeBits = 0x10;
            break;
        case 2:
            g_FieldParticleBankModeBits = 0x20;
            break;
        case 3:
            g_FieldParticleBankModeBits = 0x30;
            break;
    }
    g_FieldScriptMaxInstructionCount += 4;
}

// PARKED 79.41% / 280 bytes / 70 insns = retail's EXACT size (round 532). Round 501 called the
// head's size gap unreachable (cc1 hoists the first FieldScriptVMGetArgument's li a0,1 above the
// whole preamble, so the reorder region's jal took a nop where retail has the li). It IS reachable:
// one do { } while (0) around BOTH argument reads puts a block boundary between the preamble and the
// reads, cc1 then keeps the li adjacent to the jal, maspsx fills the delay slot with it, and the body
// is 280 bytes / 70 insns. Barrier placement measured: both reads in one block 79.41 / 280 <- best,
// first read only 73.70 / 280, each read in its own block 73.70 / 280, value read only 72.27 / 284,
// no barrier 82.99 / 284 (unlinkable).
// THE RESIDUAL is allocation and scheduling, not statement order:
//  (1) retail keeps pCurActor in a2 and 0xF003FFFF in a0; this body takes a0 and a1, so ~20 rows read
//      a0/a1/v1 where retail reads a2/a0/v1 (0xFFFCFFFF: v1 here, a1 in retail);
//  (2) the tail's two read-modify-writes emit the 0x12C group first where retail does 0x130 first and
//      sinks both stores after both loads;
//  (3) addiu sp lands in the load-delay slot of the g_FieldCurActorIndex load here (artefact of the
//      barrier, same count).
// MEASURED on top of this head, all reverted: a do-while around the tail statements 69-77% at
// 284-288; pCurActor loaded before the rect writes 70.47 / 288; inline g_FieldScriptVMCurActor at every
// tail access 79.41 / 280 (identical to the local); a flags local for the 0x12C read-modify-write 79.13
// / 280. The 0x12C write must stay whole-word ((u_int*)pCurActor)[0x4B]: the header's flags12C_0x12
// bitfield makes gcc cse the <<6 with the rect store (sll 0x12 instead of retail's sll 0x18).
/*
void func_800888A4(void) {
    SpriteData* pSpriteData;
    ActorData* pCurActor;
    int type;
    int value;

    pSpriteData = g_FieldActors[g_FieldCurActorIndex].pSpriteData;
    do {
        type = FieldScriptVMGetArgument(1) & 0xF;
        value = ((FieldScriptVMGetArgument(1) >> 4) << 8) + FieldScriptVMGetArgument(3);
    } while (0);
    SpriteAllocClutRect(pSpriteData, 2, 0);
    ((u_short*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)[2] = type << 6;
    pCurActor = g_FieldScriptVMCurActor;
    ((u_int*)pCurActor)[0x4B] = (((u_int*)pCurActor)[0x4B] & 0xF003FFFF) | (type << 24);
    ((u_short*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)[3] = value;
    pCurActor->flags130 = (pCurActor->flags130 & ~0x1FF) | (value & 0x1FF);
    pCurActor->flags12C_0x10 = 1;
    pCurActor->scriptInstructionPointer += 5;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800888A4);



// 88.92% (432 bytes against retail's 428; frame 0x28 and the saved set s0-s4+ra
// are retail's exactly, every call and every store is in retail's own order).
//   SETTLED: the four script arguments are read in this order (arg1 masked to
//   0xF first, then arg1 >> 4 << 8 + arg3, then the same pair for arg5/arg7),
//   SpriteAllocClutRect(pSpriteData, 3, 0) is called between them and the stores,
//   and the rect words 2..5 plus the flags130 bit fields (bits 0-8, 9-18, 19-27,
//   masks ~0x1FF / ~0x7FE00 / ~0xFF80000) and flags12C (10-bit field at bit 18,
//   then flags12C_0x10 = 2) follow one statement each.
//   THE ONE SIZE MISS: the first FieldScriptVMGetArgument's `ori a0,zero,1` is
//   hoisted above the frame stores here, which leaves a nop in that jal's delay
//   slot - that instruction is the whole 432-vs-428 gap. The sibling
//   func_800888A4's note records the same miss; passing the argument through a
//   local (`arg = 1;`) measured the same.
//   MEASURED: `flags12C_0x12 = type << 6` makes gcc cse the << 6 with the sprite
//   store and chain `sll s2,s2,0x12` after it (82.07%); retail's single
//   `sll s2,s2,24` needs the shift written against the raw local, which needs
//   whole-word access to the 0x12C word (no scalar member in the header) - that
//   one change is 82.07 -> 88.63. Also measured dead: `type * 0x40` at either
//   site, a modelType local, a pCurActor local vs the inline global, and moving
//   the pCurActor assignment to its first use.
//   THE REST is register naming (retail keeps the actor pointer in a2 and the
//   0xFF80000 mask in a3, and leaves a1 free across the whole tail; here they are
//   a1 and a2) plus the last block's three store order (retail stores 0x130, then
//   scriptInstructionPointer, then 0x12C; two source orders were measured and
//   neither reproduces it).
// Round 463: re-measured 88.92% / 432 bytes (retail 428, ONE short) - both residuals
// unchanged, and the tail's mechanism is now named. Wrapping JUST the last statement
// in 'do { pCurActor->flags12C_0x10 = 2; } while (0);' splits the basic block and
// the three tail stores come out in retail's order ([sw 0x130][sh 0xCC][sw 0x12C],
// loads 0x130 then ip, instead of the 0x12C group first), which proves the reorder
// is sched2 inside one block. The split costs the constant sharing retail's
// single-block schedule has (retail materialises 0xFFFCFFFF during the type2 group
// and 0x20000 inside the last group, and reuses them there), so that body is 86.10%
// at the same 432 bytes. Writing the last 0x12C write whole-word as
// '((u_int*)pCurActor)[0x4B] = (((u_int*)pCurActor)[0x4B] & ~0x30000) | 0x20000;'
// measures 82.56% / 432. So the tail wants ONE block with the 12C load hoisted with
// the other two and all three stores sunk - not reproducible from statement order.
// The head's single size miss (li a0,1 hoisted above the frame saves, leaving a nop
// in the first jal's delay slot where retail has the li) is untouched by any of this.
// Round 501: re-measured 88.92% / 432 bytes / 108 insns (retail 428 / 107) with the body below, and
// the head's size miss is now PROVEN to be cc1's, not gas's: a cc1_probe.sh snippet of this exact body
// shows cc1 emitting the first FieldScriptVMGetArgument's 'li a0,1' at the TOP of the entry block (in the
// load delay slot of 'lw v1,%lo(g_FieldCurActorIndex)'), leaving the reorder region's jal a nop; retail's
// bytes are jal / li / li / jal. Same miss in the sibling func_800888A4. try_flags.sh: 2.6.0 88.92,
// 2.7.2-cdk 88.92, so no preset reaches it. The tail's store order (retail 0x130, 0xCC, 0x12C against this
// body's 0x12C, 0x130, 0xCC from the same statement order) is the sched2 wall recorded above.
/*
void func_800889BC(void) {
    SpriteData* pSpriteData;
    int type;
    int value;
    int type2;
    int value2;
    ActorData* pCurActor;

    pSpriteData = g_FieldActors[g_FieldCurActorIndex].pSpriteData;
    type = FieldScriptVMGetArgument(1) & 0xF;
    value = ((FieldScriptVMGetArgument(1) >> 4) << 8) + FieldScriptVMGetArgument(3);
    type2 = FieldScriptVMGetArgument(5) & 0xF;
    value2 = ((FieldScriptVMGetArgument(5) >> 4) << 8) + FieldScriptVMGetArgument(7);
    SpriteAllocClutRect(pSpriteData, 3, 0);
    ((u_short*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)[2] = type << 6;
    pCurActor = g_FieldScriptVMCurActor;
    ((u_int*)pCurActor)[0x4B] = (((u_int*)pCurActor)[0x4B] & 0xF003FFFF) | (type << 24);
    ((u_short*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)[3] = value;
    pCurActor->flags130 = (pCurActor->flags130 & ~0x1FF) | (value & 0x1FF);
    ((u_short*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)[4] = type2 << 6;
    pCurActor->flags130 = (pCurActor->flags130 & ~0x7FE00) | (type2 << 15);
    ((u_short*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)[5] = value2;
    pCurActor->flags130 = (pCurActor->flags130 & ~0xFF80000) | ((value2 & 0x1FF) << 19);
    pCurActor->scriptInstructionPointer += 9;
    pCurActor->flags12C_0x10 = 2;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_800889BC);


// Start of particle handlers
extern s32 g_FieldParticleTargetActor;
extern s32 g_FieldScriptRoutineRunning;

extern s32 g_FieldScriptMaxInstructionCount;

void FieldScriptSetParticleBankFlag(void) {
    int flags = 0;

    switch (FieldScriptVMGetArgument(1)) {
        case 1:
            flags = FIELD_PARTICLE_BANK_RANGE_MODE_CIRCLE;
            break;
        case 2:
            flags = FIELD_PARTICLE_BANK_RANGE_MODE_LINE;
            break;
    }
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].flags |= flags;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

void FieldScriptSetParticleBankMotion(void) {
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].speedMultiplier = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].flags |= FieldScriptVMGetArgument(3) << 8;
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].rotAngle = FieldScriptVMGetArgument(5);
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
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].directions[index].x = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].directions[index].z = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].directions[index + 1].x = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].directions[index + 1].z = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].directions[index + 2].x = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].directions[index + 2].z = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].directions[index + 3].x = FieldScriptArgument7(0xD, SCRIPT_READ_U8_REL(0x11));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].directions[index + 3].z = FieldScriptArgument8(0xF, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}

void FieldScriptInitializeParticleBank(void) {
    g_FieldScriptParticleBank.bankIndex = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].speedMultiplier = 1;
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].targetActorID = g_FieldParticleBankTargetActor;
    g_FieldParticleTargetActor = g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].targetActorID;
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].unk0 = 0;
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].rotAngle = 0;
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].max = FieldScriptVMGetArgument(3);
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].swait = FieldScriptVMGetArgument(5);
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].ewait = FieldScriptVMGetArgument(7);
    FieldClearParticleBankDirections();
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptSetParticleBankPosition(void) {
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].pos.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].pos.vy  = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].pos.vz = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].epos.vx = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].epos.vy = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].epos.vz = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptSetParticleBankPhysics(void) {
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].speed = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].gravity.vx = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].gravity.vy = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].gravity.vz = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].srange = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].erange = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptSetParticleBankParameters(void) {
    int flags;
    int flags_2;

    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].pswait = FieldScriptVMGetArgument(1);
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].pewait = FieldScriptVMGetArgument(3);
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].shape = FieldScriptVMGetArgument(5);
    flags = FieldScriptVMGetArgument(7);
    flags_2 = (FieldScriptVMGetArgument(9) * 2);
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].flags = flags | flags_2 | g_FieldParticleBankModeBits;
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].unk72 = g_FieldParticleBankParam72;
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].unk74 = g_FieldParticleBankParam74;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}

void FieldScriptSetParticleBankScale(void) {
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].scale.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].scale.vy = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].scale.vz = 0;
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].scaleDelta.vx = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].scaleDelta.vy = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x9));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].scaleDelta.vz = 0;
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

void FieldScriptSetParticleBankColor(void) {
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].color.r  = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].color.g = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].color.b = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].colorDelta.r = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].colorDelta.g = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0xD));
    g_FieldDefaultParticleBanks[g_FieldScriptParticleBank.bankIndex].colorDelta.b = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0xD));
    g_FieldScriptMaxInstructionCount += 4;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xE;
}

void FieldScriptParticlesInitialize(void) {
    g_FieldScriptMaxInstructionCount += 4;
    if (g_FieldScriptRoutineRunning == 0) {
        FieldInitializeParticleBanks(g_FieldCurActorIndex);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

void FieldScriptStopParticleActor(void) {
    g_FieldScriptMaxInstructionCount += 4;
    FieldParticleActorStop(g_FieldCurActorIndex, SCRIPT_READ_U8_REL(0x1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}
// End of particle handlers

void FieldScriptWritePartySlotOfActor(void) {
    int i;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_FieldPartyActorSlots[i] == g_FieldCurActorIndex) {
            FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, i);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
            return;
        }
    }
    FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(1) & 0xFFFF, FIELD_PARTY_SLOT_NONE);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// A record per motion slot: three s16 components written one script argument
// at a time (vx, vz, vy), then the pad word the following array starts on.
extern SVECTOR g_FieldMotionSlotPathStart[];
extern SVECTOR g_FieldMotionSlotPathEnd[];
extern s32 g_FieldMotionSlotFrameCount[];

void FieldScriptSetMotionSlotPath(void) {
    int index = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0x11));

    g_FieldMotionSlotPathStart[index].vx = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(0x11));
    g_FieldMotionSlotPathStart[index].vy = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(0x11));
    g_FieldMotionSlotPathStart[index].vz = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(0x11));
    g_FieldMotionSlotPathEnd[index].vx = FieldScriptArgument5(9, SCRIPT_READ_U8_REL(0x11));
    g_FieldMotionSlotPathEnd[index].vy = FieldScriptArgument6(0xB, SCRIPT_READ_U8_REL(0x11));
    g_FieldMotionSlotPathEnd[index].vz = FieldScriptArgument7(0xD, SCRIPT_READ_U8_REL(0x11));
    g_FieldMotionSlotFrameCount[index] = FieldScriptArgument8(0xF, SCRIPT_READ_U8_REL(0x11));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x12;
}

extern s16 g_FieldMotionSlotActorIds[];
extern SVECTOR g_FieldMotionSlotTargetPositions[];

void FieldScriptSetMotionSlotTarget(void) {
    int index = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));
    int actorIndex;

    g_FieldMotionSlotTargetPositions[index].vx = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    g_FieldMotionSlotTargetPositions[index].vz = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(9));
    g_FieldMotionSlotTargetPositions[index].vy = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(9));
    actorIndex = FieldScriptVMGetActorIndex(0xA);
    if (actorIndex != ACTOR_ID_INVALID) {
        g_FieldMotionSlotActorIds[index] = actorIndex;
    } else {
        g_FieldMotionSlotActorIds[index] = -1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}

extern FieldScriptGlobalS16 g_FieldSoundProximitySource;

void FieldScriptSetByte22E0(void) {
    g_FieldSoundProximitySource.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern FieldScriptGlobalU8 g_FieldScreenShakeMode;

void FieldScriptSetByte21D2(void) {
    g_FieldScreenShakeMode.value = FieldScriptVMGetArgument(1) - 0x80;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 g_FieldScriptSuppressScreenEffect;

void FieldScriptSetByteAFE84(void) {
    g_FieldScriptSuppressScreenEffect = SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

// The screen effect setup record at 0x800B0080 (0x1C bytes, padded to the next
// record at 0xA0): eight s16 then three s32, which FieldLoad hands to the effect
// setup and which the two parameter ops below fill a field at a time.
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ s32 unk18;
} FieldScreenEffectSetup; // Size: 0x1C

extern FieldScreenEffectSetup g_FieldScreenEffectConfig;

void FieldScriptSetParams80(void) {
    int value;

    g_FieldScreenEffectConfig.unk0 = FieldScriptVMGetInstructionArgument(1);
    g_FieldScreenEffectConfig.unk2 = FieldScriptVMGetInstructionArgument(3);
    value = FieldScriptVMGetInstructionArgument(5);
    g_FieldScreenEffectConfig.unk4 = value;
    if ((short)value == 0) {
        g_FieldScreenEffectConfig.unk4 = value + 1;
    }
    g_FieldScreenEffectConfig.unk6 = FieldScriptVMGetInstructionArgument(7);
    g_FieldScreenEffectConfig.unk8 = 0;
    g_FieldScreenEffectConfig.unkA = FieldScriptVMGetInstructionArgument(9);
    g_FieldScreenEffectConfig.unkC = FieldScriptVMGetInstructionArgument(0xB);
    g_FieldScreenEffectConfig.unkE = FieldScriptVMGetInstructionArgument(0xD);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xF;
}

void FieldScriptSetParams90(void) {
    g_FieldScreenEffectConfig.unk10 = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    g_FieldScreenEffectConfig.unk18 = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    g_FieldScreenEffectConfig.unk14 = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

// The argument record the batch handlers below fill at 0x800B00A0 (the 4 bytes
// before it are the alignment hole that ends the previous record at 0x9C): a run
// of byte parameters and an s16 tail, read back as a group by FieldLoad.
typedef struct {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u8 unk4;
    /* 0x05 */ u8 unk5;
    /* 0x06 */ u8 unk6;
    /* 0x07 */ u8 unk7;
    /* 0x08 */ u8 unk8;
    /* 0x09 */ u8 unk9;
    /* 0x0A */ u8 unkA;
    /* 0x0B */ u8 unkB;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
} FieldScreenEffectParams; // Size: 0x12

extern FieldScreenEffectParams g_FieldScreenEffectParams;
extern FieldScriptGlobalS16 g_FieldScreenEffectEnabled;

void FieldScriptSetParamsA0(void) {
    g_FieldScreenEffectParams.unk0 = FieldScriptVMGetArgument(1);
    g_FieldScreenEffectParams.unk1 = FieldScriptVMGetArgument(3);
    g_FieldScreenEffectParams.unk2 = FieldScriptVMGetArgument(5);
    g_FieldScreenEffectParams.unk4 = FieldScriptVMGetArgument(7);
    g_FieldScreenEffectParams.unk5 = FieldScriptVMGetArgument(9);
    g_FieldScreenEffectParams.unk6 = FieldScriptVMGetArgument(0xB);
    g_FieldScreenEffectParams.unk8 = FieldScriptVMGetArgument(0xD);
    g_FieldScreenEffectParams.unk9 = FieldScriptVMGetArgument(0xF);
    g_FieldScreenEffectParams.unkA = FieldScriptVMGetArgument(0x11);
    g_FieldScreenEffectParams.unkC = FieldScriptVMGetArgument(0x13);
    g_FieldScreenEffectParams.unkE = FieldScriptVMGetArgument(0x15);
    g_FieldScreenEffectParams.unk10 = FieldScriptVMGetArgument(0x17);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0x19;
    g_FieldScreenEffectEnabled.value = 1;
}

extern s32 g_FieldTransitionBusyFlag;

void FieldScriptWaitForTransitionBusyFlag(void) {
    if (g_FieldTransitionBusyFlag == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

extern s32 g_FieldStreamFramePosition;

void FieldScriptWriteByteB06A0(void) {
    FieldScriptMemoryWriteU16(SCRIPT_IMM_ARG(1), g_FieldStreamFramePosition);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void* g_FieldScreenImageBuffer;
extern void FieldStoreScreenImage(void);
extern void FieldLoadTIMWithClut(u_long* pTimData, int x, int y, int clutX, int clutY, int clutWidth,
                                 int clutHeight);

void FieldScriptScreenImageHandler(void) {
    s32 clutY;
    s32 x;
    s32 y;
    s32 value;
    s32 archiveIndex;
    s32 imageId;
    u8 command;
    void* pBuffer;

    if (FieldArchiveTryDataSync() == -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        g_FieldScriptVMStopFlag = 1;
        return;
    }
    command = SCRIPT_READ_U8_REL(1);
    if (command == 0) {
        imageId = FieldScriptArgument1(5, SCRIPT_READ_U8_REL(0xD));
        ArchiveSetIndex(4, 0);
        archiveIndex = imageId + 0x7FB;
        pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(archiveIndex), 0);
        g_FieldScreenImageBuffer = pBuffer;
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
        FieldLoadTIMWithClut(g_FieldScreenImageBuffer, x, y, 0, clutY, 0, 0);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
    } else {
        HeapFree(g_FieldScreenImageBuffer);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
    g_FieldScriptVMStopFlag = 1;
}

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

extern int g_FieldArchiveStreamActive;

int FieldArchiveTryDataSync(void) {
    if (g_FieldArchiveStreamActive != 0) {
        return -1;
    }
    if (ArchiveDataSync() != 0) {
        return -1;
    }
    ArchiveCdDataSync(0);
    return 0;
}

extern void ControllerSetControllerFlag(int);

void FieldScriptClearControllerFlag(void) {
    if (SCRIPT_READ_U8_REL(1) == 0) {
        ControllerSetControllerFlag(0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern FieldScriptGlobalS16 g_FieldScriptRenderContextIndex;

void FieldScriptSetRenderContextIndex(void) {
    g_FieldScriptRenderContextIndex.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetCharacterField78(void) {
    int partyId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(3));

    if (partyId != CHARACTER_ID_NONE) {
        int value = FieldScriptVMGetArgument(1) - g_pGameState->characters[partyId].statBase;

        if (value < 0) {
            value = 0;
        }
        g_pGameState->characters[partyId].statBonus = value;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptWriteCharacterFieldSum(void) {
    int partyId = FieldPartySlotToCharacter(FieldScriptVMGetArgument(3));

    if (partyId != CHARACTER_ID_NONE) {
        int value = g_pGameState->characters[partyId].statBase + g_pGameState->characters[partyId].statBonus;

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
extern void* g_FieldPartyModelLoadBuffer;
extern s32 g_FieldPartyModelLoadState;
extern s32 g_FieldPartyModelLoadCharacter;
extern s32 g_FieldPartyModelLoadPartyIndex;

void FieldPartyLoadMemberSkin(int characterId, int partyIndex) {
    int size;
    void* pBuffer;
    int characterIndex;

    g_FieldPartyModelLoadPartyIndex = partyIndex;
    g_FieldPartyModelLoadCharacter = characterId;
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    if (g_FieldEntryRoutineFinished == 0) {
        ArchiveCdDataSync(0);
    }
    if ((g_GameSceneMapNum & FIELD_MAP_GEAR_SKINS_MASK) == 0) {
        int archiveIndex = characterId + FIELD_PARTY_SKIN_ARCHIVE_OFFSET;

        size = ArchiveDecodeAlignedSize(archiveIndex);
        g_GamePartyMemberSkins[g_FieldPartyModelLoadPartyIndex] = characterId;
        pBuffer = HeapAlloc(size, 0);
        g_FieldPartyModelLoadBuffer = pBuffer;
        ArchiveReadFileToBuffer(archiveIndex, pBuffer, 0, CdlModeSpeed);
    } else {
        int archiveIndex;

        characterIndex = GameCharacterGetGearID(characterId);
        if (characterIndex == GEAR_ID_NONE) {
            characterIndex = 0;
        }
        characterIndex += FIELD_GEAR_SKIN_ID_OFFSET;
        archiveIndex = characterIndex + FIELD_PARTY_SKIN_ARCHIVE_OFFSET;
        size = ArchiveDecodeAlignedSize(archiveIndex);
        pBuffer = HeapAlloc(size, 0);
        g_FieldPartyModelLoadBuffer = pBuffer;
        g_GamePartyMemberSkins[g_FieldPartyModelLoadPartyIndex] = characterIndex;
        ArchiveReadFileToBuffer(archiveIndex, pBuffer, 0, CdlModeSpeed);
    }
    if (g_FieldEntryRoutineFinished == 0) {
        ArchiveCdDataSync(0);
    }
    g_FieldPartyModelLoadState = 1;
}

void FieldScriptSetInvertedAnimationId(void) {
    g_FieldScriptVMCurActor->animationOverride = ~SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptSetInvertedAnimationIdAndClearFlag(void) {
    FieldScriptSetInvertedAnimationId();
    g_FieldScriptVMCurActor->flags &= 0xFFFEFFFF;
}

extern s32 g_FieldSpecialAnimLoadPending;

void FieldScriptLoadSpecialAnimFile(void) {
    if (FieldArchiveTryDataSync() == 0) {
        g_FieldSpecialAnimLoadPending = 0;
        SpriteSetSpecialAnimFile(g_FieldActors[g_FieldCurActorIndex].pSpriteData, g_FieldScriptVMCurActor->pSpecialAnimFile);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

void FieldScriptFreeSpecialAnimFile(void) {
    if (g_FieldScriptVMCurActor->specialAnimArchiveIndex != -1) {
        HeapFree(g_FieldScriptVMCurActor->pSpecialAnimFile);
        g_FieldScriptVMCurActor->specialAnimArchiveIndex = -1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    g_FieldScriptVMStopFlag = 1;
}

extern void* g_FieldWdsLoadBuffer; // WDS file buffer

extern s32 g_FieldWdsLoadEntryIndex;

extern s32 g_FieldWdsLoadSlotIndex;

extern void* g_GameWdsEntries[];

extern s32 D_80062524;

extern s32 g_GameWdsEntrySlot3;

extern s32 g_FieldSuppressStateChange;

extern void SoundGetPendingSpuCommandType(int);

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
            g_GameWdsEntries[g_FieldWdsLoadSlotIndex] = SoundLoadWdsFile(g_FieldWdsLoadBuffer, 0);
            SoundGetPendingSpuCommandType(SOUND_CTL_FLAG_TRANSFER_BUSY);
            HeapFree(g_FieldWdsLoadBuffer);
            if (g_FieldWdsLoadSlotIndex == 3) {
                g_GameWdsEntrySlot3 = D_80062524;
            }
            g_FieldScriptVMStopFlag = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        } else {
            wdsIndex = FieldScriptVMGetArgument(2);
            pWdsEntry = g_GameWdsEntries[wdsIndex];
            g_FieldWdsLoadSlotIndex = wdsIndex;
            SoundFreeWdsEntry(pWdsEntry);
            flags = FieldScriptVMGetArgument(4);
            g_FieldWdsLoadEntryIndex = flags;
            if (!(flags & FIELD_WDS_BANK_ALT_ARCHIVE_MASK)) {
setIndex1C:
                ArchiveSetIndex(ARCHIVE_DIR_FIELD_WDS, 0);
                g_FieldWdsLoadEntryIndex += FIELD_WDS_BANK_ARCHIVE_OFFSET;
                goto readFile;
            }
            if (g_FieldSuppressStateChange != 1) {
                g_FieldWdsLoadEntryIndex = (flags & FIELD_WDS_BANK_INDEX_MASK) + FIELD_WDS_BATTLE_BANK_ARCHIVE_OFFSET;
                ArchiveSetIndex(ARCHIVE_DIR_BATTLE_SOUND, 1);
                goto readFile;
            }
            g_FieldWdsLoadEntryIndex = 4;
            goto setIndex1C;
readFile:
            pWdsFileBuffer = HeapAlloc(ArchiveDecodeAlignedSize(g_FieldWdsLoadEntryIndex), 0);
            g_FieldWdsLoadBuffer = pWdsFileBuffer;
            ArchiveReadFileToBuffer(g_FieldWdsLoadEntryIndex, pWdsFileBuffer, 0, CdlModeSpeed);
            ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
        }
    } else {
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
}

extern void ArchiveCdDataSync(int mode);

void FieldScriptLoadSpecialAnimArchive(void) {
    int scriptArgument;
    int archiveIndex;
    int bufferSize;
    void* pBuffer;

    if (g_FieldSpecialAnimLoadPending == 0 && g_FieldArchiveStreamActive == 0) {
        if (FieldArchiveTryDataSync() != 0) {
            g_FieldScriptVMStopFlag = 1;
            g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
            return;
        }
        if (g_FieldScriptVMCurActor->specialAnimArchiveIndex != -1) {
            HeapFree(g_FieldScriptVMCurActor->pSpecialAnimFile);
            g_FieldScriptVMCurActor->specialAnimArchiveIndex = -1;
        }
        scriptArgument = FieldScriptVMGetArgument(1);
        ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
        archiveIndex = scriptArgument + FIELD_SPECIAL_ANIM_ARCHIVE_OFFSET;
        bufferSize = ArchiveDecodeAlignedSize(archiveIndex) + 8;
        g_FieldScriptVMCurActor->specialAnimArchiveIndex = archiveIndex;
        pBuffer = HeapAlloc(bufferSize, 0);
        g_FieldScriptVMCurActor->pSpecialAnimFile = pBuffer;
        ArchiveReadFileToBuffer(archiveIndex, pBuffer, 0, CdlModeSpeed);
        if (g_FieldEntryRoutineFinished == 0) {
            ArchiveCdDataSync(0);
        }
        g_FieldSpecialAnimLoadPending = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    g_FieldScriptVMStopFlag = 1;
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

extern s16 g_FieldMotionSlotSteps[];

void FieldScriptSetVectorTable223C(void) {
    int index = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(9));

    g_FieldMotionSlotSteps[index] = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(9));
    g_FieldMotionSlotSteps[index + 3] = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(9));
    g_FieldMotionSlotSteps[index + 6] = FieldScriptArgument4(7, SCRIPT_READ_U8_REL(9));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xA;
}

extern FieldScriptGlobalU8 g_FieldBackgroundR;
extern FieldScriptGlobalU8 g_FieldBackgroundG;
extern FieldScriptGlobalU8 g_FieldBackgroundB;

void FieldScriptSetBackgroundColor(void) {
    g_FieldBackgroundR.value = FieldScriptVMGetArgument(1);
    g_FieldBackgroundG.value = FieldScriptVMGetArgument(3);
    g_FieldBackgroundB.value = FieldScriptVMGetArgument(5);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern FieldScriptGlobalS16 g_FieldActorUpdateAngle;

void FieldScriptSetUpdateAngle(void) {
    g_FieldActorUpdateAngle.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 g_FieldEntryRoutineFinished;
extern s16 g_FieldModelAnimationIds[];

void FieldScriptSetGearModelAnimation(void) {
    if (g_FieldEntryRoutineFinished != 0) {
        LibGearSetModelAnimation(FieldScriptVMGetArgument(1) & 0xFFFF, 0, FieldScriptVMGetArgument(3));
        g_FieldModelAnimationIds[FieldScriptVMGetArgument(1)] = FieldScriptVMGetArgument(3);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptSetByte11E(void) {
    g_FieldScriptVMCurActor->updateAngle = FieldScriptVMGetArgument(1);
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
    g_FieldScriptVMStopFlag = 1;
}


extern SVECTOR g_FieldRotationVector;

void FieldScriptSetRotationVector(void) {
    g_FieldRotationVector.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    g_FieldRotationVector.vz = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    g_FieldRotationVector.vy = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

void FieldScriptSetCameraRotation(void) {
    g_Scene.camRotation.vx = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(7));
    g_Scene.camRotation.vz = FieldScriptArgument2(3, SCRIPT_READ_U8_REL(7));
    g_Scene.camRotation.vy = FieldScriptArgument3(5, SCRIPT_READ_U8_REL(7));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}
// Round 487: re-measured 98.06%, 177 insns / 708 bytes against retail's 176 /
// 0x2C0. Round 462's -da proof stands. The THREE pointers' priority ranking is
// retail's exact reverse (retail s1=pData s2=pPrev s3=pCur, ours s1=pPrev
// s2=pCur s3=pData), and the reversed ranking is what also costs the first
// argument constant its jal slot (cc1 hoists 'li $4,1' to the entry block).
// New negatives this round, both from the block-barrier trick that DID flip a
// two-way hand-out in func_8008C180: a do-while around the second argument read
// 96.59 (worse map), a dead if/else around the tail ip increment is byte-
// identical at 98.06. So the barrier lever does not reach a THREE-way rotation.
// See also 'a call-argument constant folds early' and the -G8 rematerialise
// lever; do not re-hunt this one.
/*
void func_8008B5D4(void) {
    FieldActorModelData* pModel;
    FieldActorModelBounds* pBounds;
    u32* pData;
    u8* pCur;
    u8* pPrev;
    POLY_FT3* pPoly3;
    POLY_FT3* pPrevPoly3;
    POLY_FT4* pPoly4;
    POLY_FT4* pPrevPoly4;
    s32 ctx;
    s32 du;
    s32 dv;
    s32 n;
    s32 i;
    s32 j;
    u32 code;
    u32 w;

    pModel = g_FieldActors[g_FieldCurActorIndex].pModelData;
    pBounds = pModel->pBounds;
    ctx = g_FieldCurRenderContextIndex;
    pCur = pModel->pPrimitiveBuffers[ctx];
    pPrev = pModel->pPrimitiveBuffers[(ctx + 1) & 1];
    pData = pBounds->pCommands;
    du = FieldScriptVMGetInstructionArgumentS16(1);
    dv = FieldScriptVMGetInstructionArgumentS16(3);
    for (i = pBounds->numCommands; i > 0; i--) {
        w = *pData;
        code = w & 0xFF;
        n = w >> 16;
        if (code == 0xC4 || code == 0xC8) {
            pData++;
            continue;
        }
        pData++;
        if (!(w & 8)) {
            pPoly3 = (POLY_FT3*)pCur;
            pPrevPoly3 = (POLY_FT3*)pPrev;
            for (j = 0; j < n; j++, pData += 2) {
                pPoly3->u0 += du;
                pPoly3->u1 += du;
                pPoly3->u2 += du;
                pPoly3->v0 += dv;
                pPoly3->v1 += dv;
                pPoly3->v2 += dv;
                pPrevPoly3->u0 = pPoly3->u0;
                pPrevPoly3->u1 = pPoly3->u1;
                pPrevPoly3->u2 = pPoly3->u2;
                pPrevPoly3->v0 = pPoly3->v0;
                pPrevPoly3->v1 = pPoly3->v1;
                pPrevPoly3->v2 = pPoly3->v2;
                pPoly3++;
                pPrevPoly3++;
            }
            pCur = (u8*)pPoly3;
            pPrev = (u8*)pPrevPoly3;
        } else {
            pPoly4 = (POLY_FT4*)pCur;
            pPrevPoly4 = (POLY_FT4*)pPrev;
            for (j = 0; j < n; j++, pData += 2) {
                pPoly4->u0 += du;
                pPoly4->u1 += du;
                pPoly4->u2 += du;
                pPoly4->u3 += du;
                pPoly4->v0 += dv;
                pPoly4->v1 += dv;
                pPoly4->v2 += dv;
                pPoly4->v3 += dv;
                pPrevPoly4->u0 = pPoly4->u0;
                pPrevPoly4->u1 = pPoly4->u1;
                pPrevPoly4->u2 = pPoly4->u2;
                pPrevPoly4->u3 = pPoly4->u3;
                pPrevPoly4->v0 = pPoly4->v0;
                pPrevPoly4->v1 = pPoly4->v1;
                pPrevPoly4->v2 = pPoly4->v2;
                pPrevPoly4->v3 = pPoly4->v3;
                pPoly4++;
                pPrevPoly4++;
            }
            pCur = (u8*)pPoly4;
            pPrev = (u8*)pPrevPoly4;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008B5D4);




extern void* g_PartyDataBuffers[];
extern void* g_FieldPartyModelLoadBuffer;
extern s32 g_FieldPartyModelLoadState;
extern s32 g_FieldPartyModelLoadCharacter;
extern s32 g_FieldPartyModelLoadPartyIndex;
extern void LZSSDecompress(void*, void*);
extern void func_8008B978(int);

void FieldScriptFinishPartyModelLoad(void) {
    if (g_FieldPartyModelLoadState != CHARACTER_ID_NONE && ArchiveDataSync() == 0) {
        ArchiveCdDataSync(0);
        LZSSDecompress(g_FieldPartyModelLoadBuffer, g_PartyDataBuffers[g_FieldPartyModelLoadPartyIndex]);
        HeapFree(g_FieldPartyModelLoadBuffer);
        func_8008B978(g_FieldPartyModelLoadCharacter);
        g_FieldPartyModelLoadState = CHARACTER_ID_NONE;
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    } else {
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
}

extern FieldActor* g_FieldScriptVMCurFieldActor;
extern s32 g_FieldPartyActorIndices[MAX_PARTY_MEMBERS];
extern s32 g_FieldScriptVMScriptEndFlag;
extern s32 FieldScriptGetBytecodeOffset(int scriptIndex, int routineIndex);
extern void FieldScriptVMRun(int maxInstructionCount);
extern void FieldSnapPartyToPlayer(void);

// Bytecode opcode a party member's actor runs once its model file has loaded; its
// argument is the character id that was just loaded.
#define FIELD_SCRIPT_OPCODE_PARTY_MODEL 0x16

// 97.50%: 776 bytes (retail's size) and every instruction matches except one
// four-instruction group in the found arm, where the compiler keeps the actor
// address in $v1 and pActorData in $v0 and retail has them the other way round.
// Retail's own gear-branch site has the opposite pairing again, so that one group
// is an allocation coin flip: tried pActor vs g_FieldScriptVMCurFieldActor
// store-forwarding, both declaration orders, (*ppFieldActors)[i].pActorData
// (95.94%), and the permuter (its parser rejects this body).
// Round 322: 97.58% (776 bytes), ONE instruction left - the found arm's addu
// operand order (retail `addu v0,v0,s2`, this body `addu v0,s2,v0`: same
// registers, same destination, printed the other way round). The gear site's
// register swap closed by giving its write-back its OWN pointer local (the
// pWriteActor declaration below: 97.53 -> 97.58, retail's `lw v0,76(v0)` /
// `sh s1,204(v0)`). The addu order is the assignment-vs-subexpression lever
// exactly, and it will not do both: with the sum ASSIGNED (`pFieldActor =
// &(*ppFieldActors)[i]; pActor = pFieldActor->pActorData;`, an embedded
// assignment, or `pFieldActor = *ppFieldActors; pFieldActor += i;`) the operand
// order is retail's base-first but the sum lands in $v1 and the load and both
// stores swap with it (97.45-97.50); as a sub-expression it lands in $v0 with
// the wrong order (this body, 97.58). The permuter cannot parse the unit
// ("conversion to non-scalar type requested" in the preprocessed file).
// The pieces that are load-bearing, all measured: ppFieldActors =
// (FieldActor**)&g_FieldActors before the loop (without it the compiler
// re-materialises the address at both actor sites and the frame loses that
// register); (FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors) at the two
// write-back sites, where retail re-forms the address instead of using the
// hoisted one; ONE s32 offset local reused for the second
// FieldScriptGetBytecodeOffset result (a second local costs 3 instructions);
// prevMaxInstructionCount assigned BEFORE prevActorIndex (that decides which of
// the two gets a register and which is spilled); and a s32 rather than u_short
// return for FieldScriptGetBytecodeOffset, since retail has no andi 0xffff.
// Round 323: the round-315 two-local base spelling was tried too. A base local
// assigned from *ppFieldActors plus `pFieldActor = (FieldActor*)((u8*)pBase +
// i * sizeof(FieldActor));` gives `addu v1,v0,s2` (base-first, sum in $v1) at
// 97.50; the in-place form (`pFieldActor = *ppFieldActors; pFieldActor =
// (FieldActor*)((u8*)pFieldActor + i * sizeof(FieldActor));`) puts the sum in
// the base's own register (`addu v1,v1,s2`) at 97.45 - so the sum CAN take the
// base register, but then the base's load and the pActorData load swap $v0/$v1
// against retail. The whole group is one two-register coin flip; the
// subexpression form below keeps $v0 (retail's destination) at 97.58.
// Round 397: 99.92% (776 bytes = retail's size, 194 insns, 14 differing
// lines). THE RESTRUCTURE THAT FIXED THE GUARD REGION: retail's shape is an
// explicit entry guard, not a for - `prevIP; prevStopFlag; actorCount =
// g_FieldActiveActorCount; prevMaxInstructionCount; prevActorIndex; i = 0;
// if (actorCount > 0) { ppFieldActors = ...; do {...} while (i <
// g_FieldActiveActorCount); }` reproduces retail's [maxinstr load][i=0][blez]
// [la s5][la s6][iv init] exactly (97.58 -> 99.84 with the frame still 0x50);
// the cached actorCount explains retail's EARLY count load while the bottom
// test re-reads the global each iteration, as retail does.
// THE FRAME: retail (and the old for-form!) has a 4th, never-accessed 8-byte
// slot at 0x28 (vars=32) - a reload-era slot the for machinery creates. The
// `u8 unused[8]` dummy below reproduces the frame size but its RTL-gen slot
// lands FIRST (0x10), pushing the three real slots +8 (99.92); any aggregate
// declared anywhere in the function lands first, so the dummy cannot close
// this - the phantom must be reproduced by a reload-era pseudo (mechanism
// unidentified; -dg of the for-form would name it).
// THE addu COIN FLIP, re-measured on this body: assigned form (pFieldActor
// assigned first) gives retail's base-first `addu v1,v0,s2` but the sum lands
// in v1 and the load and both stores swap with it (99.84); the subexpression
// form keeps the sum in retail's v0 but emits index-first `addu v0,s2,v0`
// (99.92, this body). Same wall as rounds 322-323.
// Measured dead ends this round: `if (actorCount > 0) { setup; for (; i <
// count; i++) }` does not merge the guards (+3 insns, 788 bytes, 98.29);
// ppFieldActors assigned INSIDE the loop is deleted, not hoisted (both actor
// sites re-form lui/lw inline, 95.75). Permuter still cannot parse the unit
// ("conversion to non-scalar type requested").
// Round 414 (re-measured): WITH the dummy 99.92, WITHOUT it 99.84 - and without
// it the three spills land on retail's own 0x10/0x18/0x20, so the whole residual
// is the frame alone (sp -0x50 against retail's -0x58, all ten saves and
// restores 8 low). Frame and slot offsets are therefore locked together: the
// frame needs an allocation that ENDS above 0x28, and every expand-time object
// takes 0x10 (the dummy proves it), so only a reload-era slot can sit at 0x28 -
// most plausibly a pseudo that alter_reg gave a slot and reload then
// rematerialised (an address constant). gcc-2.6.0-psx scores this body 97.84,
// so the preset is not the answer either.
// Round 428, re-measured on this body: 99.92% / 776 bytes, 194 insns, and the
// compiled .frame reads vars=32 - FOUR 8-byte stack slots: the dummy at 0x10 and
// the three spills at 0x18/0x20/0x28. Retail's hole is the TOP slot (0x28) and its
// three spills are the low three. Moving the dummy from the last declaration to
// the first changes nothing (still 99.92), so an expand-time aggregate always
// takes the LOWEST var slot and the hole cannot be moved from source that way; it
// must be a fourth SPILL-era pseudo declared after prevIP whose accesses reload
// dropped. No local in this body spills into it, so the frame and the three slot
// offsets stay locked together. Also re-measured: assigning pFieldActor before
// pActor gives retail's base-first addu but swaps the actor-data load and both
// stores ($v0/$v1) - 99.84, so the subexpression form below stands.
/*
void func_8008B978(int characterId) {
    FieldActor* pPrevFieldActor;
    ActorData* pActor;
    FieldActor* pFieldActor;
    ActorData* pPrevActor;
    s32 prevStopFlag;
    s32 actorCount;
    s32 prevMaxInstructionCount;
    s32 prevActorIndex;
    s32 i;
    s32 prevIP;
    FieldActor** ppFieldActors;
    ActorData* pWriteActor;
    s32 offset;
    u8* pScript;
    u8 unused[8]; // original has an unused stack slot; reproduces the frame

    g_pGameState->characterFlags |= 1 << g_FieldPartyModelLoadCharacter;
    pPrevFieldActor = g_FieldScriptVMCurFieldActor;
    pPrevActor = g_FieldScriptVMCurActor;
    if (g_FieldEntryRoutineFinished != 0) {
        prevIP = pPrevActor->scriptInstructionPointer;
        prevStopFlag = g_FieldScriptVMStopFlag;
        actorCount = g_FieldActiveActorCount;
        prevMaxInstructionCount = g_FieldScriptMaxInstructionCount;
        prevActorIndex = g_FieldCurActorIndex;
        i = 0;
        if (actorCount > 0) {
            ppFieldActors = (FieldActor**)&g_FieldActors;
            do {
                offset = FieldScriptGetBytecodeOffset(i, 0);
                pScript = (u8*)g_FieldScriptVMCurScriptData + offset;
                if (pScript[0] == FIELD_SCRIPT_OPCODE_PARTY_MODEL && pScript[1] == characterId) {
                    pActor = (&(*ppFieldActors)[i])->pActorData;
                    pFieldActor = &(*ppFieldActors)[i];
                    g_FieldScriptVMCurFieldActor = pFieldActor;
                    g_FieldScriptVMCurActor = pActor;
                    func_80080A74(i);
                    pWriteActor = ((FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors))->pActorData;
                    g_FieldCurActorIndex = i;
                    pWriteActor->scriptInstructionPointer = offset;
                    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptGetBytecodeOffset(i, 0);
                    g_FieldScriptVMScriptEndFlag = 0;
                    offset = FieldScriptGetBytecodeOffset(i, 0);
                    FieldActorCopyPlacement(i, g_PlayerActorIndex);
                    FieldScriptVMRun(0xFFFF);
                    FieldSnapPartyToPlayer();
                    g_pGameState->characterFlags |= 1 << g_FieldPartyModelLoadCharacter;
                    if ((g_GameSceneMapNum & FIELD_MAP_GEAR_SKINS_MASK) != 0) {
                        pFieldActor = &(*ppFieldActors)[g_FieldPartyActorIndices[g_FieldPartyModelLoadPartyIndex]];
                        g_FieldScriptVMCurFieldActor = pFieldActor;
                        g_FieldScriptVMCurActor = pFieldActor->pActorData;
                        func_80080A74(g_FieldPartyActorIndices[g_FieldPartyModelLoadPartyIndex]);
                        ((FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors))->pActorData->scriptInstructionPointer = offset;
                        g_FieldCurActorIndex = g_FieldPartyActorIndices[g_FieldPartyModelLoadPartyIndex];
                        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptGetBytecodeOffset(g_FieldPartyActorIndices[g_FieldPartyModelLoadPartyIndex], 0);
                        g_FieldScriptVMScriptEndFlag = 0;
                        FieldScriptVMRun(0xFFFF);
                    }
                    break;
                }
                i++;
            } while (i < g_FieldActiveActorCount);
        }
        g_FieldScriptVMCurFieldActor = pPrevFieldActor;
        g_FieldScriptVMCurActor = pPrevActor;
        g_FieldScriptVMStopFlag = prevStopFlag;
        g_FieldCurActorIndex = prevActorIndex;
        g_FieldScriptMaxInstructionCount = prevMaxInstructionCount;
        pPrevActor->scriptInstructionPointer = prevIP;
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008B978);

extern void FieldPartyLoadMemberSkin(int characterId, int partyIndex);

void FieldScriptAddPartyMember(void) {
    int characterId;
    int partyIndex;

    if (g_FieldPartyModelLoadState == CHARACTER_ID_NONE && g_FieldArchiveStreamActive == 0 && FieldArchiveTryDataSync() == 0) {
        ArchiveCdDataSync(0);
        characterId = FieldScriptVMGetArgument(1);
        if (characterId != CHARACTER_ID_NONE) {
            if (FieldPartyAllocSlot(characterId, &partyIndex) == 0) {
                g_pGameState->gearRide[partyIndex] = 0;
                g_GamePartyMembers[partyIndex] = characterId;
                FieldPartyLoadMemberSkin(characterId, partyIndex);
                g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
            } else {
                g_pGameState->characterFlags |= 1 << characterId;
                g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
            }
        } else {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
        }
    } else {
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
}

// 94.32%: structure and every store match; only the second SCRIPT_READ_U8_REL(1)
// expansion is ordered differently (the original loads both globals before the
// `lhu 0xCC` and the party index before the `addu`; gcc reorders ours the other
// way and picks $v0/$v1 the opposite way round).
// preset screen: 2.7.2 94.89%, cdk 84.77%, 2.6.0 94.89%, all with the call below
// fixed - no compiler fixes it. As parked the body does not compile:
// FieldPartyLoadMemberSkin() is declared with two params, and passing
// SCRIPT_READ_U8_REL(1) and partyIndex gives the numbers above.
void FieldScriptAddPartyMemberShort(void) {
    int partyIndex;
    u8 characterId;

    if (g_FieldPartyModelLoadState == CHARACTER_ID_NONE && g_FieldArchiveStreamActive == 0 && FieldArchiveTryDataSync() == 0) {
        ArchiveCdDataSync(0);
        if (FieldPartyAllocSlot(SCRIPT_READ_U8_REL(1), &partyIndex) == 0) {
            g_pGameState->gearRide[partyIndex] = 0;
            characterId = SCRIPT_READ_U8_REL(1);
            g_GamePartyMembers[partyIndex] = characterId;
            FieldPartyLoadMemberSkin(characterId, partyIndex);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        } else {
            g_pGameState->characterFlags |= 1 << SCRIPT_READ_U8_REL(1);
            g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
        }
    } else {
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
}

extern void* g_FieldSavedFramebuffers[];
extern void* g_FieldSavedFramebuffer1;
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
// 97.81% (580 bytes; retail 584): rows' 0x14000 constant gets $v0 and reorg
// steals its lui into the bne delay slot, where retail has nop and $t0.
// Preset screen: 2.7.2 97.81%, cdk 86.01%, 2.6.0 97.81%; a 4-minute permuter
// run found nothing (best candidate 206 vs base 317).
// Round 397: still 97.81 (580 vs retail 584), three more spellings measured
// identical (empty do{}while(0) around the copy loop; two-statement in-place
// pEnd = pSrc; pEnd = (u8*)pEnd + 0x14000). -dg proves the wall: the 0x14000 is
// emitted as a RELOAD SCRATCH with no pseudo/allocno ((set (reg:SI 2 v0)
// (const_int 65536)) post-allocation), so which register it lands in is reload's
// first-free-reg pick, not a source-controlled allocno; retail's $t0 held
// &D_8005A448 live through the if, so its nop + lui/ori after the branch cannot
// be reorg'd from C.
// Round 420: re-measured 97.81% (580 bytes, retail 584, one instruction short) and
// confirmed this is lever a-li-constant-is-a-reload-scratch: the 0x14000 gets
// $v0 (dead at the bne) so reorg fills the delay slot; retail has `li 0x14000`
// in $t0 and a nop. No new spelling attempted - the lever records ten.
// Round 435: re-measured 97.81% (580 bytes, retail 584 - one instruction short, so it cannot link). Two more spellings dead: pEnd formed from the global load BEFORE pSrc (pEnd = (u8*)g_FieldSavedFramebuffers[i] + 0x14000) shifts the whole set - pSrc takes $v0, the index $a2, the constant $v1 - and reaches retail's 146 insns / 584 bytes at 96.58%, i.e. the size is reachable and the colour is not; and pEnd between the two global loads is byte-identical to this park. Same wall as recorded: the 0x14000 constant's register is reload/allocator-chosen.
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

    pSrc = g_FieldSavedFramebuffers[partyIndex];
    pDst = g_PartyDataBuffers[partyIndex];
    pEnd = (PartyDataChunk*)((u8*)pSrc + 0x14000);
    do {
        *pDst = *pSrc;
        pSrc++;
        pDst++;
    } while (pSrc != pEnd);

    g_GamePartyMemberSkins[partyIndex] = g_GamePartyMemberSkins[partyIndex + 1];
    g_GamePartyMembers[partyIndex] = g_GamePartyMembers[partyIndex + 1];
    g_FieldPartyActorSlots[partyIndex] = g_FieldPartyActorSlots[partyIndex + 1];

    pActor = (characterId + g_FieldActors)->pActorData;
    if (!(pActor->spriteSource & 0x80)) {
        func_80076AC0(characterId, partyIndex, g_PartyDataBuffers[partyIndex], 1, 0, partyIndex, 1);
    } else {
        pSprite = (void*)((*(s32*)(g_FieldSpriteData + (pActor->spriteSource & 0x7F) * 4 + 4)) + (s32)g_FieldSpriteData);
        func_80076AC0(characterId, pActor->spriteId, pSprite,
            ((u32)pActor->flags130 >> 28) & 0x3, pActor->flags134 & 0xF,
            (characterId + g_FieldActors)->pActorData->spriteSource,
            ((u32)(characterId + g_FieldActors)->pActorData->flags134 >> 4) & 0x1);
    }

    g_GamePartyMembers[partyIndex + 1] = ACTOR_ID_INVALID;
    g_GamePartyMemberSkins[partyIndex + 1] = ACTOR_ID_INVALID;
    D_8005A448[partyIndex] = ACTOR_ID_INVALID;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008BF38);


extern FieldActor* g_FieldScriptVMCurFieldActor;
extern void* g_PartyDataBuffers[];
extern void func_80080A74(int actorIndex);
// Second declaration of g_FieldActors as an array: its address then stays a
// value in a callee-saved register instead of folding into each load (retail
// holds $s0). Removing it costs the frame slot and rotates every other value.
extern FieldActor* g_FieldActorsTable[] __asm__("g_FieldActors");

void func_8008C180(int partyIndex) {
    FieldActor** pFieldActors;
    FieldActor* pFieldActor;
    FieldActor* pPrevFieldActor;
    ActorData* pActor;
    ActorData* pPrevActor;
    int characterId;
    int prevActorIndex;
    u_short prevScriptPointer;

    if (g_FieldPartyActorSlots[partyIndex] == ACTOR_ID_INVALID) {
        g_GamePartyMembers[partyIndex] = ACTOR_ID_INVALID;
        g_GamePartyMemberSkins[partyIndex] = ACTOR_ID_INVALID;
        return;
    }
    pFieldActors = g_FieldActorsTable;
    pFieldActor = &(*pFieldActors)[g_FieldPartyActorSlots[partyIndex]];
    pPrevFieldActor = g_FieldScriptVMCurFieldActor;
    pPrevActor = g_FieldScriptVMCurActor;
    prevActorIndex = g_FieldCurActorIndex;
    prevScriptPointer = g_FieldScriptVMCurActor->scriptInstructionPointer;
    g_FieldScriptVMCurFieldActor = pFieldActor;
    g_FieldScriptVMCurActor = pFieldActor->pActorData;
    func_80080A74(g_FieldPartyActorSlots[partyIndex]);

    characterId = g_FieldPartyActorSlots[partyIndex];
    pFieldActor = &(*pFieldActors)[characterId];
    pFieldActor->status = (pFieldActor->status & 0xF07F) | 0x200;
    g_FieldCurActorIndex = characterId;
    func_80076AC0(characterId, 0, g_PartyDataBuffers[0], 1, 0, 0, 1);

    pActor = g_FieldScriptVMCurActor;
    pActor->scriptFlags.flags |= 1;
    pActor->flags |= 0x100000;
    g_FieldScriptVMStopFlag = 0;
    g_FieldScriptVMCurActor = pPrevActor;
    g_FieldScriptVMCurFieldActor = pPrevFieldActor;
    g_FieldCurActorIndex = prevActorIndex;
    pActor->scriptInstructionPointer = prevScriptPointer;
    pActor->scriptFlags.flags |= 0x20000;
    pActor->flags |= 0x400;
    g_FieldPartyActorSlots[partyIndex] = ACTOR_ID_INVALID;
    g_GamePartyMembers[partyIndex] = ACTOR_ID_INVALID;
    g_GamePartyMemberSkins[partyIndex] = ACTOR_ID_INVALID;
}


extern int FieldCharacterIdToPartyId(int characterId);
extern void func_8008C180(int partyIndex);
extern void func_8008BF38(int partyIndex);

// PARKED 98.25% (1188 bytes / 297 insns = retail's exact size and instruction
// count; 70 differing lines, all of them register names). Round 515 closed the
// 92.66% -> 98.25% gap with two discoveries:
//
// 1. The party-slot element must be spelled through a struct-pointer CAST, not as
//    an array subscript and not as a member of a *declared* extern struct. All
//    three were measured in a cc1_probe.sh snippet on this unit: `g_P1[1]` and
//    `g_P2.slot1` both emit `la $3,g_P+4` (lui+addiu, then lw/sw 0(reg)) because
//    the element is read and written in the same block, while
//    `((PartySlots*)g_P3)->slot1` emits retail's direct symbol access - `lui
//    $v0,%hi(sym)` + `lw $v0,%lo(sym+4)($v0)` for the load and the bare
//    `sw $reg,sym+4` macro (gas expands it through $at) for the store. That single
//    respelling took 92.66 -> 98.18; the old note's conclusion ("the only route
//    is one symbol per slot in symbol_addrs") was wrong.
// 2. g_FieldSavedFramebuffers[1] is the BARE symbol g_FieldSavedFramebuffer1 in retail's object
//    (misc5.c already spells it that way), so the two slot-2 copies read
//    `extern void* g_FieldSavedFramebuffer1;` and not the array at +4: 98.18 -> 98.25.
//
// What remains is two register artifacts, both downstream of the 0x14000 copy-end
// constant:
//   - Retail builds the constant in $t0 and adds into it (`lui $t0,0x1` in the
//     bne delay slot / `ori` / `addu $t0,$a2,$t0`); this cc1 always materialises it
//     in $v0 with pEnd in $t0. That is reload's scratch, not an allocno: forming
//     pEnd between the two global loads is byte-identical, forming it in chunk
//     units (`pSrc + 0x1400`) is byte-identical, and hoisting the member/skin
//     loads above the copy loop (so $v0 is live across it) STILL gives $v0 - see
//     lever a-li-constant-is-a-reload-scratch, which this unit's own func_8008BF38
//     measured on the same constant.
//   - Because $v0 is taken, the member/skin loads land in $a1/$a2 where retail has
//     $v0/$a1 (18 of the 70 lines; size-neutral).
//   - The last 4 lines: retail stores g_PlayerActorIndex before the `sll` chain,
//     this body 3 insns later (in the `lw a0,76($v0)` delay slot). A 4-minute
//     permuter run (1480 iterations, base 520 / best 180) only reached it by
//     declaring `extern volatile unsigned int g_PlayerActorIndex` - a declaration
//     lie, rejected.
//
// Load-bearing layout kept from the earlier parks: the member/skin reads are taken
// into locals BEFORE the block clears the slots (that is retail's store order
// members[1]=NONE, skins[1]=NONE, members[0]=temp, skins[0]=temp), and each arm's
// `gearRide[N] = 0` is the LAST statement so its sb lands in the exit jump's delay
// slot. The finishEntry case-0 arm calls func_8008C180(0), func_8008BF38(0) and
// func_8008BF38(1) (round 489; only one a0=1 call exists, in the shared tail).

// The three party slots held by g_GamePartyMembers / g_GamePartyMemberSkins.
// Spelled through a struct pointer because the array form makes cc1 materialise the
// element address (lui+addiu / lw 0(reg)); retail keeps the slot offset in the
// symbol's %lo half (lui / lw %lo(sym+4)($reg)) for every slot but the first.
typedef struct {
    /* 0x0 */ s32 slot0;
    /* 0x4 */ s32 slot1;
    /* 0x8 */ s32 slot2;
} PartySlots; // Size: 0xC

#define g_PartySlots ((PartySlots*)g_GamePartyMembers)
#define g_PartySkins ((PartySlots*)g_GamePartyMemberSkins)
#define PARTY_FB_CHUNKS (FIELD_PARTY_FRAMEBUFFER_SIZE / sizeof(PartyDataChunk))

// Round 528 (redo lane): un-parked, re-measured 98.25% (1188 bytes / 297 insns = retail's
// exact size and count, 70 differing lines, all register names). The body below is
// kept as-is; the residual is the one lever
// the-0x14000-copy-end-constant-is-reloads-scratch-in-this-unit-too already records
// as blocked (reload scratch, no allocno), plus the tail sw that sched2 sinks.
// Tried this round: `do { } while (0)` around case 0's member/skin epilogue store
// group (the block-barrier knob that closed func_8008C180, above) - byte-identical,
// 70 differing lines before and after, so the barrier does not reach a caller-saved
// hand-out; reverted.
/*
void func_8008C334(void) {
    s32 loadState;
    s32 partyIndex;
    s32 member;
    s32 skin;
    PartyDataChunk* pSrc;
    PartyDataChunk* pDst;
    PartyDataChunk* pEnd;
    ActorData* pActor;

    loadState = g_FieldPartyModelLoadState;
    if (loadState != CHARACTER_ID_NONE) {
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
        return;
    }
    DrawSync(0);
    partyIndex = FieldCharacterIdToPartyId(
        FieldPartySlotToCharacter(((u8*)g_FieldScriptVMCurScriptData)[g_FieldScriptVMCurActor->scriptInstructionPointer + 1]));
    if (partyIndex != -1) {
        if (g_FieldEntryRoutineFinished == 0) {
            switch (partyIndex) {
            case 0:
                if (g_PartySlots->slot1 == CHARACTER_ID_NONE) {
                    g_PartySlots->slot0 = CHARACTER_ID_NONE;
                    g_PartySkins->slot0 = CHARACTER_ID_NONE;
                    g_pGameState->gearRide[0] = 0;
                    break;
                }
                pSrc = g_FieldSavedFramebuffers[0];
                pDst = g_PartyDataBuffers[0];
                pEnd = pSrc + PARTY_FB_CHUNKS;
                do {
                    *pDst = *pSrc;
                    pSrc++;
                    pDst++;
                } while (pSrc != pEnd);
                member = g_PartySlots->slot1;
                skin = g_PartySkins->slot1;
                g_PartySlots->slot1 = FIELD_PARTY_SLOT_NONE;
                g_PartySkins->slot1 = FIELD_PARTY_SLOT_NONE;
                g_PartySlots->slot0 = member;
                g_PartySkins->slot0 = skin;
                g_pGameState->gearRide[0] = g_pGameState->gearRide[1];
                if (g_PartySlots->slot2 != FIELD_PARTY_SLOT_NONE) {
                    pSrc = g_FieldSavedFramebuffer1;
                    pDst = g_FieldSavedFramebuffers[0];
                    pEnd = pSrc + PARTY_FB_CHUNKS;
                    do {
                        *pDst = *pSrc;
                        pSrc++;
                        pDst++;
                    } while (pSrc != pEnd);
                    member = g_PartySlots->slot2;
                    skin = g_PartySkins->slot2;
                    g_PartySlots->slot2 = FIELD_PARTY_SLOT_NONE;
                    g_PartySkins->slot2 = FIELD_PARTY_SLOT_NONE;
                    g_PartySlots->slot1 = member;
                    g_PartySkins->slot1 = skin;
                    g_pGameState->gearRide[1] = g_pGameState->gearRide[2];
                }
                break;
            case 1:
                if (g_PartySlots->slot2 == CHARACTER_ID_NONE) {
                    g_PartySlots->slot1 = CHARACTER_ID_NONE;
                    g_PartySkins->slot1 = CHARACTER_ID_NONE;
                    g_pGameState->gearRide[1] = 0;
                    break;
                }
                pSrc = g_FieldSavedFramebuffer1;
                pDst = g_FieldSavedFramebuffers[0];
                pEnd = pSrc + PARTY_FB_CHUNKS;
                do {
                    *pDst = *pSrc;
                    pSrc++;
                    pDst++;
                } while (pSrc != pEnd);
                member = g_PartySlots->slot2;
                skin = g_PartySkins->slot2;
                g_PartySlots->slot2 = FIELD_PARTY_SLOT_NONE;
                g_PartySkins->slot2 = FIELD_PARTY_SLOT_NONE;
                g_PartySlots->slot1 = member;
                g_PartySkins->slot1 = skin;
                g_pGameState->gearRide[1] = g_pGameState->gearRide[2];
                g_pGameState->gearRide[2] = 0;
                break;
            case 2:
                g_PartySlots->slot2 = CHARACTER_ID_NONE;
                g_PartySkins->slot2 = CHARACTER_ID_NONE;
                g_pGameState->gearRide[2] = 0;
                break;
            }
        } else {
            switch (partyIndex) {
            case 0:
                g_pGameState->gearRide[0] = g_pGameState->gearRide[1];
                g_pGameState->gearRide[1] = g_pGameState->gearRide[2];
                g_pGameState->gearRide[2] = 0;
                func_8008C180(0);
                func_8008BF38(0);
                func_8008BF38(1);
                break;
            case 1:
                g_pGameState->gearRide[1] = g_pGameState->gearRide[2];
                g_pGameState->gearRide[2] = 0;
                func_8008C180(1);
                func_8008BF38(1);
                break;
            case 2:
                g_pGameState->gearRide[2] = 0;
                func_8008C180(2);
                break;
            }
            if (g_PartySlots->slot0 != FIELD_PARTY_SLOT_NONE && g_FieldPartyActorSlots[0] != FIELD_PARTY_SLOT_NONE) {
                g_PlayerActorIndex = g_FieldPartyActorSlots[0];
                pActor = g_FieldActors[g_FieldPartyActorSlots[0]].pActorData;
                pActor->scriptFlags.flags = (pActor->scriptFlags.flags | 0x4400) & ~0x80;
            } else {
                g_PlayerActorIndex = 0;
            }
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008C334);




void FieldScriptFreeActorFile(void) {
    if (g_FieldScriptVMCurActor->flags12C_0xD) {
        HeapFree(g_FieldScriptVMCurActor->pScriptArgs);
        g_FieldScriptVMCurActor->flags12C_0xD = 0;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
}

extern void SoundSetAudioManagerVolumeWithFade(s32, int, int);
extern s32 g_GameAudioAndWdsLoaded;
extern s32 g_GameRequestedWdsBankIndex;
extern s32 g_SoundAudioManager;

void FieldScriptSetAudioInterpolator70(void) {
    if (g_GameAudioAndWdsLoaded != 0) {
        SoundSetAudioManagerVolumeWithFade(g_SoundAudioManager, FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(3));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (g_GameRequestedWdsBankIndex == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (g_FieldEntryRoutineFinished == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

extern void SoundFadeAudioManagerInterpolator7c(s32, int, int);

void FieldScriptSetAudioInterpolator7C(void) {
    if (g_GameAudioAndWdsLoaded != 0) {
        SoundFadeAudioManagerInterpolator7c(g_SoundAudioManager, FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)),
                      FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5)));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (g_GameRequestedWdsBankIndex == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (g_FieldEntryRoutineFinished == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

extern void SoundSetAudioManagerMasterVolumeWithFade(int, int, int);

void FieldScriptSetAudioInterpolator64(void) {
    if (g_GameAudioAndWdsLoaded != 0) {
        SoundSetAudioManagerMasterVolumeWithFade(g_SoundAudioManager, FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(3));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (g_GameRequestedWdsBankIndex == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else if (g_FieldEntryRoutineFinished == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

extern void SoundFadeAudioManagerInterpolator88(int, int, int);

void FieldScriptSetAudioInterpolator88(void) {
    if (g_GameAudioAndWdsLoaded != 0) {
        SoundFadeAudioManagerInterpolator88(g_SoundAudioManager, FieldScriptArgument1(1, SCRIPT_READ_U8_REL(5)),
                      FieldScriptArgument2(3, SCRIPT_READ_U8_REL(5)));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (g_GameRequestedWdsBankIndex == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else if (g_FieldEntryRoutineFinished == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

extern void SoundMuteElementsByFlags(int, int);

void FieldScriptSetAudioMuteFlags(void) {
    if (g_GameAudioAndWdsLoaded != 0) {
        SoundMuteElementsByFlags(g_SoundAudioManager, FieldScriptVMGetArgument(1));
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else if (g_GameRequestedWdsBankIndex == CHARACTER_ID_NONE) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else if (g_FieldEntryRoutineFinished == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

extern void FieldReleaseActorSoundSlot(s32);

void FieldScriptSetActorSoundParams0(void) {
    g_FieldScriptVMCurActor->soundId = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->soundState = 0;
    g_FieldScriptVMCurActor->soundVolume = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    FieldReleaseActorSoundSlot(g_FieldCurActorIndex);
    if (g_FieldScriptVMCurActor->soundId == 0) {
        g_FieldScriptVMCurActor->soundState = FIELD_ACTOR_SOUND_STATE_NONE;
    }
}

void FieldScriptSetActorSoundParams80(void) {
    g_FieldScriptVMCurActor->soundId = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->soundState = 0x80;
    g_FieldScriptVMCurActor->soundVolume = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    FieldReleaseActorSoundSlot(g_FieldCurActorIndex);
    if (g_FieldScriptVMCurActor->soundId == 0) {
        g_FieldScriptVMCurActor->soundState = FIELD_ACTOR_SOUND_STATE_NONE;
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

// The paired rotation op writes the same three components, in the same
// vx/vz/vy order, to two vectors of 0x6 bytes each (an SVECTOR without its pad
// word); 0x800B21A0..0x800B21AC.
typedef struct {
    /* 0x0 */ s16 vx;
    /* 0x2 */ s16 vy;
    /* 0x4 */ s16 vz;
} FieldRotationVector; // Size: 0x6

typedef struct {
    /* 0x0 */ FieldRotationVector first;
    /* 0x6 */ FieldRotationVector second;
} FieldRotationVectorPair; // Size: 0xC

extern FieldRotationVectorPair g_FieldRotationVectorPair;

void FieldScriptSetRotationVectorPair(void) {
    g_FieldRotationVectorPair.first.vx = FieldScriptVMGetInstructionArgument(1);
    g_FieldRotationVectorPair.first.vz = FieldScriptVMGetInstructionArgument(3);
    g_FieldRotationVectorPair.first.vy = FieldScriptVMGetInstructionArgument(5);
    g_FieldRotationVectorPair.second.vx = FieldScriptVMGetInstructionArgument(7);
    g_FieldRotationVectorPair.second.vz = FieldScriptVMGetInstructionArgument(9);
    g_FieldRotationVectorPair.second.vy = FieldScriptVMGetInstructionArgument(0xB);
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

extern void FieldActorApplyRotationAndScale(int);

void FieldScriptSetActorScaleUniform(void) {
    unsigned int scale = FieldScriptVMGetArgument(1);

    ((SpriteData*)g_FieldActors[g_FieldCurActorIndex].pSpriteData)->scale = (scale * 3) >> 2;
    g_FieldScriptVMCurActor->scaleX = scale;
    g_FieldScriptVMCurActor->scaleY = scale;
    g_FieldScriptVMCurActor->scaleZ = scale;
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetActorScale(void) {
    int scaleX = FieldScriptVMGetArgument(1);
    int scaleY = FieldScriptVMGetArgument(3);
    int scaleZ = FieldScriptVMGetArgument(5);

    ((SpriteData*)g_FieldActors[g_FieldCurActorIndex].pSpriteData)->scale = 0xC00;
    g_FieldScriptVMCurActor->scaleX = scaleX;
    g_FieldScriptVMCurActor->scaleY = scaleY;
    g_FieldScriptVMCurActor->scaleZ = scaleZ;
    FieldActorApplyRotationAndScale(g_FieldCurActorIndex);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern FieldScriptGlobalS16 g_FieldPolarVectorScale;

void FieldScriptSetScale218C(void) {
    g_FieldPolarVectorScale.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetSpriteGravity(void) {
    ((SpriteData*)g_FieldActors[g_FieldCurActorIndex].pSpriteData)->field_0x82 = FieldScriptVMGetArgument(1) * 2;
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
    return -(distance >= FIELD_ACTOR_DISTANCE_THRESHOLD);
}

// Each of these re-reads the volatile g_FieldActors itself rather than taking
// the array: retail re-reads that base once per copied field, and the one read
// has to serve both operands. The field selector is a byte offset because the
// typed spellings let gcc hoist the `index * sizeof(FieldActor)` multiplies:
// position.x/y/z as `((SpriteData*)pSpriteData)->position.x` is 93.47%/500,
// transformMatrix.t[n] typed is 78.56%/492, all seven typed 80.43%/524,
// against 100.00%/496. The sprite cursor has to be a `u8*` local (99.44% if
// CopyActorFieldS32 gets the same treatment). The selector is a parameter, so
// no field type can name it: both stay byte-offset cursors.
static inline void CopySpriteFieldS32(int dstId, int srcId, u32 byteOffset) {
    FieldActor* actors = g_FieldActors;
    u8* dst = (u8*)actors[dstId].pSpriteData;
    u8* src = (u8*)actors[srcId].pSpriteData;

    *(s32*)(dst + byteOffset) = *(s32*)(src + byteOffset);
}

// Retail copies this field with `lhu`/`sh`, so it reads unsigned even though
// SpriteData::field_0x84 is declared s16.
static inline void CopySpriteField0x84(int dstId, int srcId) {
    FieldActor* actors = g_FieldActors;
    SpriteData* dst = actors[dstId].pSpriteData;
    SpriteData* src = actors[srcId].pSpriteData;

    *(u16*)&dst->field_0x84 = *(u16*)&src->field_0x84;
}

// Same runtime-offset cursor as CopySpriteFieldS32 (the offset names a word of
// the 0xA4-byte FieldActor).
static inline void CopyActorFieldS32(int dstId, int srcId, u32 byteOffset) {
    FieldActor* actors = g_FieldActors;
    *(s32*)((u8*)&actors[dstId] + byteOffset) = *(s32*)((u8*)&actors[srcId] + byteOffset);
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
    dstActor->scriptYPos = srcActor->scriptYPos;
    dstActor->curYPos = srcActor->curYPos;
    dstActor->curWalkmeshTriMaterial = srcActor->curWalkmeshTriMaterial;

    CopySpriteField0x84(dstActorId, srcActorId);
    CopySpriteFieldS32(dstActorId, srcActorId, offsetof(SpriteData, position.x));
    CopySpriteFieldS32(dstActorId, srcActorId, offsetof(SpriteData, position.y));
    CopySpriteFieldS32(dstActorId, srcActorId, offsetof(SpriteData, position.z));

    CopyActorFieldS32(dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[0]));
    CopyActorFieldS32(dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[1]));
    CopyActorFieldS32(dstActorId, srcActorId, offsetof(FieldActor, transformMatrix.t[2]));
}

extern u8 g_FieldPartySlotMask;

void FieldClearPartyMaskForCurrentActor(void) {
    int i = 0;
    s32 actorIndex = g_FieldCurActorIndex;
    u8* pMask = &g_FieldPartySlotMask;

    for (; i < MAX_PARTY_MEMBERS; i++) {
        if (g_FieldPartyActorSlots[i] == actorIndex) {
            *pMask &= ~(1 << i);
        }
    }
}

extern FieldScriptGlobalU8 g_FieldCameraAtFollowTargetY;

void FieldScriptSetByte21CD(void) {
    g_FieldCameraAtFollowTargetY.value = SCRIPT_READ_U8_REL(1);
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
void FieldScriptWriteScriptInstruction3U16(int value0, int value1, int value2) {
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

extern FieldScriptGlobalS16 g_FieldPlayerActorIndex16;

void FieldScriptSetPlayerActorIndexMirror(void) {
    g_FieldPlayerActorIndex16.value = FieldScriptVMGetActorIndex(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}


int FieldPartyMemberIncreaseGearHp(int partyMemberIndex, unsigned int amount) {
    int gearId;
    unsigned int maxHp;
    unsigned int newHp;

    gearId = GameCharacterGetGearID(g_GamePartyMembers[partyMemberIndex]);
    if (gearId != GEAR_ID_NONE) {
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
    if (gearId != GEAR_ID_NONE) {
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
// while the array is based at g_FieldRandomSkinSlots + 0xC, so this C is one symbol name
// apart from a 100% match (objdiff 99.26%, same instructions and bytes; the
// relocations resolve to the same addresses). Declaring D_800B2298/D_800B229C
// as structs to fix the names instead makes gcc hoist D_800B229C's address out
// of the branch in FieldScriptSetRandomSkinParams, because the two symbols no longer share an
// alias set, and that function's bytes change.
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 count;
    /* 0xC */ u16 values[FIELD_RANDOM_SKIN_SLOT_COUNT];
} FieldRandomValues; // Size: 0x4C

extern FieldRandomValues g_FieldRandomSkinSlots;
extern int rand(void);

void FieldRandomizeSkinSlots(void) {
    int i;
    int j;
    int value;
    u16 emptyValue;

    g_FieldRandomSkinSlots.unk0 = g_FieldRandomSkinSlots.unk4;
    if (g_FieldRandomSkinSlots.count == 0) {
        g_FieldRandomSkinSlots.unk4 = 0;
        return;
    }
    emptyValue = FIELD_RANDOM_SKIN_SLOT_EMPTY;
    for (i = FIELD_RANDOM_SKIN_SLOT_COUNT - 1; i >= 0; i--) {
        g_FieldRandomSkinSlots.values[i] = emptyValue;
    }
    for (i = 0; i < g_FieldRandomSkinSlots.count; i++) {
retry:
        value = ((rand() * (g_FieldRandomSkinSlots.unk4 + 1)) >> 15) & 0xFFFF;
        for (j = 0; j < FIELD_RANDOM_SKIN_SLOT_COUNT; j++) {
            if (g_FieldRandomSkinSlots.values[j] == value) {
                goto retry;
            }
        }
        g_FieldRandomSkinSlots.values[i] = value;
    }
    for (i = 0; i < g_FieldRandomSkinSlots.count; i++) {
        g_FieldRandomSkinSlots.values[i]++;
    }
}

extern FieldScriptGlobalS32 D_800B2298;
extern FieldScriptGlobalS32 D_800B229C;

void FieldScriptSetRandomSkinParams(void) {
    int count;

    D_800B2298.value = FieldScriptVMGetArgument(1);
    count = FieldScriptVMGetArgument(3);
    D_800B229C.value = count;
    if (count > FIELD_RANDOM_SKIN_SLOT_COUNT) {
        D_800B229C.value = FIELD_RANDOM_SKIN_SLOT_COUNT;
    }
    FieldRandomizeSkinSlots();
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptSetActorMovementFlag(void) {
    switch (SCRIPT_READ_U8_REL(1)) {
        case 0:
            if (g_FieldScriptVMCurActor->scriptFlags.flags & FIELD_SCRIPT_FLAG_ROTATION_LOCKED) {
                g_FieldScriptVMCurActor->scriptFlags.flags &= ~FIELD_SCRIPT_FLAG_ROTATION_LOCKED;
            }
            if (g_FieldScriptVMCurActor->flags & 0x80000) {
                SpriteData* pSpriteData = g_FieldActors[g_FieldCurActorIndex].pSpriteData;

                pSpriteData->moveSpeed = 0;
                pSpriteData->step.z = 0;
                pSpriteData->step.x = 0;
                g_FieldScriptVMCurActor->flags &= ~0x80000;
            }
            break;
        case 1:
            g_FieldScriptVMCurActor->scriptFlags.flags |= FIELD_SCRIPT_FLAG_ROTATION_LOCKED;
            g_FieldScriptVMCurActor->lockedRotationY = *(u_short*)&g_FieldScriptVMCurActor->rotation.vy;
            break;
        case 2:
            g_FieldScriptVMCurActor->flags |= 0x80000;
            break;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

extern s32 g_FieldStreamReadyFlag;

void FieldScriptWaitForFlagADB7C(void) {
    if (g_FieldStreamReadyFlag == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    } else {
        g_FieldStreamReadyFlag = 0;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

extern s16 g_FieldStreamFileIndex;
extern s16 g_FieldStreamImageX1;
extern s16 g_FieldStreamImageY1;
extern s16 g_FieldStreamImageX2;
extern s16 g_FieldStreamImageY2;
extern s16 D_800C3A2A;
extern s16 g_FieldStreamFrameBase;
extern s16 g_FieldStreamTotalFrames;
extern u16 g_FieldStreamType;
extern u16 D_800C3A32;
extern u16 D_800C3A34;
extern u16 g_FieldStreamRgb24;
extern u16 g_FieldStreamId;
extern u16 g_FieldStreamLoopPlayback;
extern s32 g_FieldStreamSceneActive;
extern s32 g_FieldMode;
extern s32 g_FieldStreamFlags;

void FieldStreamStartImage(void) {
    ActorData* pActor;
    s16 value;
    u16 ip;
    u16 streamType;
    u16* pIp;

    if (g_FieldMapChangeGate.value == 0) {
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    } else {
        value = FieldScriptArgument1(1, SCRIPT_READ_U8_REL(0xB));
        pActor = g_FieldScriptVMCurActor;
        g_FieldStreamFileIndex = value;
        ip = pActor->scriptInstructionPointer;
        do {
            value = FieldScriptArgument2(3, SCRIPT_READ_U8(ip + 0xB));
        } while (0);
        pActor = g_FieldScriptVMCurActor;
        D_800C3A2A = value;
        do {
            value = FieldScriptArgument3(5, SCRIPT_READ_U8(pActor->scriptInstructionPointer + 0xB));
        } while (0);
        pActor = g_FieldScriptVMCurActor;
        g_FieldStreamFrameBase = value;
        do {
            value = FieldScriptArgument4(7, SCRIPT_READ_U8(pActor->scriptInstructionPointer + 0xB));
        } while (0);
        pActor = g_FieldScriptVMCurActor;
        g_FieldStreamTotalFrames = value;
        do {
            value = FieldScriptArgument5(9, SCRIPT_READ_U8(pActor->scriptInstructionPointer + 0xB));
        } while (0);
        g_FieldStreamId = value;
        D_800C3A32 = 0x140;
        g_FieldStreamFlags = 0x40;
        g_FieldStreamRgb24 = 1;
        streamType = g_FieldStreamType;
        pIp = &g_FieldScriptVMCurActor->scriptInstructionPointer;
        D_800C3A34 = 0x100;
        do {
            g_FieldStreamImageX2 = 0;
            g_FieldStreamImageY1 = 0;
            g_FieldStreamImageX1 = 0;
        } while (0);
        g_FieldStreamImageY2 = 0x100;
        g_FieldStreamLoopPlayback = 0;
        do {
            g_FieldStreamType = streamType & 0xF;
        } while (0);
        ip = *pIp;
        g_FieldMode = 0;
        g_FieldStreamSceneActive = 1;
        g_FieldScriptVMStopFlag = 1;
        *pIp = ip + 0xC;
    }
}

// 94.74% (480 bytes against retail's 484 - ONE instruction short, so it cannot
// link yet). Everything matches: the g_FieldMapChangeGate guard, the four
// FieldScriptVMGetArgument calls, the switch tree (cases 0/1/2, no default), the
// 0x140/0x100 image rect constants and the trailing globals.
//   THE SIZE MISS is two register copies around `value` (arg7): retail keeps
//   `addu v1,v0,zero` for the raw g_FieldStreamType store and the `& 0xC0` flags, computes
//   `andi v0,v0,0xf` early, and copies again (`addu v1,v0,zero`) for the switch
//   operand; here gcc masks in place and zero-extends the operand
//   (`andi v1,v0,0xffff`), which nets one instruction less. Measured identical:
//   `switch (value & 0xF)` vs an `int type = value & 0xF;` local, and moving that
//   assignment either side of the g_FieldStreamFlags store.
//   THE OTHER GAP is the tail: retail reads the instruction pointer
//   (`lhu v0,0xcc(a0); ori v1,1`) between the g_FieldStreamLoopPlayback and
//   g_FieldStreamSceneActive stores; here the load is hoisted above LoopPlayback.
//   The matched-neighbour note on FieldStreamStartImageFromScript records the same hoist.
// 94.74% (480 bytes, same size): the post-reload scheduler puts our lhu ip
// + li 1 above the g_FieldStreamLoopPlayback store where retail has them after
// it; a volatile store on that playbacks field does not move it, and neither does
// one on the FIRST store of the group (g_FieldStreamId) (round 322: identical
// 480 bytes; volatile can only pin the load lower by pinning the whole group).
//   The size miss is the raw-value copy retail keeps (`move v1,v0` before
//   `andi v0,v0,0xf` / `sh v1` / `andi v1,v1,0xc0`). A volatile store on the raw
//   g_FieldStreamType store DOES produce the copy, but pins the type mask after it
//   (89.25% 492 bytes, and 480/91.93% once the masked store is written
//   `g_FieldStreamType = type;` instead of `&= 0xF`), so the copy and retail's AND order
//   cannot be had together; a separate `u16 raw` local folds away (476). Statement
//   order of `type = value & 0xF` against the raw store changes nothing in either
//   form.
// Round 323: the loopPlayback volatile was pinning the tail group the wrong way -
// with a plain `g_FieldStreamLoopPlayback = 0;` retail's `lhu 0xCC(a0)` / `li
// v1,1` sit after that `sh` and the tail matches. `int value` (476 bytes) and a
// volatile raw store with `g_FieldStreamType = type` (476) both lose the 4 bytes; the
// head's three-instruction residual is the copy-and-AND-order trade recorded
// above and nothing reorders it.
// Round 420: re-measured 94.74% (480 bytes, retail 484 - still ONE short). Three
// more spellings tried: (a) flags store before the type mask with `g_FieldStreamType =
// type;` -> 476/90.16; (b) `u16 type` (assigned either side of the raw store)
// -> 480/86.97, gcc masks into a fresh register and stores the raw value from v0,
// so the wasteful `move v1,v0` / `sh v1` / `andi v1,v1,0xc0` triple retail keeps
// is unreachable: 2.7.2 folds copy+AND whenever the copy has one use; (c) -O3 by
// try_flags -> 94.74. The 3-line `lhu 0xCC`/`li 1` hoist at the tail is probably
// downstream of the same missing copy.
// Round 462: re-measured 94.74% (480 bytes, retail 484). Two more spellings dead:
// (a) `switch (value & 0xF)` inline with no `type` local and the masked store as
// `g_FieldStreamType = value & 0xF;` -> identical 480 (gcc CSEs it into the same body);
// (b) `int value` -> 476/90.16 and it names the second residual's cause: the
// `andi v1,v0,0xffff` for the switch operand IS the HImode->SImode promotion of
// `value & 0xF` (with an int `value` it disappears), so retail's `move v1,v0`
// there means retail's switch operand was SImode while the raw store still forced
// an in-place `andi v0,v0,0xf`. Net: our build either masks the type late from v0
// (u16: 480, raw store needs no copy) or masks it into a fresh register (int: 476,
// no copies at all); retail's "destructive mask BEFORE the raw store" ordering is
// what no statement order produces. cdk 89.74, 2.6.0 86.97 (preset is right).
// Round 435: re-measured 94.74% (480 bytes, retail 484 - still one instruction short). One more spelling dead: s32 value with the type mask written BEFORE the raw store (retail's own instruction order) measures 476 bytes, 31 differing lines - gcc folds the copy and puts the store back under the flags mask. cc1_probe.sh on the isolated sequence (call, raw sh, flags mask/sw, type andi, switch chain) gives our $2/$3 shape under three statement orders, so the missing move v1,v0 needs the call-result pseudo to be allocated away from $v0, which no statement order in this block does.
/*
void func_8008EC30(void) {
    u16 value;
    int type;

    if (g_FieldMapChangeGate.value == 0) {
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    } else {
        g_FieldStreamFileIndex = FieldScriptVMGetArgument(1);
        D_800C3A2A = FieldScriptVMGetArgument(3);
        g_FieldStreamTotalFrames = FieldScriptVMGetArgument(5);
        value = FieldScriptVMGetArgument(7);
        g_FieldStreamType = value;
        type = value & 0xF;
        g_FieldStreamFlags = value & 0xC0;
        D_800C3A32 = 0x140;
        D_800C3A34 = 0x100;
        g_FieldStreamType &= 0xF;
        g_FieldStreamFrameBase = 1;
        switch (type) {
            case 0:
                g_FieldStreamImageX1 = 0x140;
                g_FieldStreamImageY1 = 0;
                g_FieldStreamImageX2 = 0x140;
                g_FieldStreamImageY2 = 0x100;
                g_FieldMode = 1;
                g_FieldStreamRgb24 = 0;
                break;
            case 1:
                g_FieldStreamImageX2 = 0;
                g_FieldStreamImageY1 = 0;
                g_FieldStreamImageX1 = 0;
                g_FieldStreamImageY2 = 0x100;
                g_FieldMode = 0;
                g_FieldStreamRgb24 = 0;
                break;
            case 2:
                g_FieldStreamImageX2 = 0;
                g_FieldStreamImageY1 = 0;
                g_FieldStreamImageX1 = 0;
                g_FieldStreamImageY2 = 0x100;
                g_FieldMode = 0;
                g_FieldStreamRgb24 = 1;
                break;
        }
        g_FieldStreamId = 0xFF;
        g_FieldStreamLoopPlayback = 0;
        g_FieldStreamSceneActive = 1;
        g_FieldScriptVMStopFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008EC30);


void FieldStreamStartImageFromScript(void) {
    u16 value;
    int type;
    s16 pos;
    u16 ip;
    u16 *pIp;

    g_FieldStreamFileIndex = FieldScriptVMGetArgument(1);
    D_800C3A2A = FieldScriptVMGetArgument(3);
    g_FieldStreamFrameBase = FieldScriptVMGetArgument(5);
    g_FieldStreamTotalFrames = FieldScriptVMGetArgument(7);
    value = FieldScriptVMGetArgument(9);
    g_FieldStreamType = value;
    type = value;
    if (type == 0xFE) {
        g_FieldStreamLoopPlayback = 1;
    } else {
        g_FieldStreamLoopPlayback = 0;
    }
    pos = FieldScriptVMGetArgument(0xB);
    g_FieldStreamImageX1 = pos;
    g_FieldStreamImageX2 = pos;
    pos = FieldScriptVMGetArgument(0xD);
    g_FieldStreamImageY1 = pos;
    g_FieldStreamImageY2 = pos;
    D_800C3A32 = FieldScriptVMGetArgument(0xF);
    D_800C3A34 = FieldScriptVMGetArgument(0x11);
    g_FieldStreamFlags = type & 0x40;
    g_FieldStreamId = 0xFF;
    g_FieldMode = 2;
    g_FieldStreamRgb24 = 0;
    g_FieldStreamType &= 0xF;
    pIp = &g_FieldScriptVMCurActor->scriptInstructionPointer;
    ip = *pIp;
    g_FieldStreamSceneActive = 1;
    *pIp = ip + 0x13;
}


extern s32 g_FieldScriptFadeDuration;
extern s32 g_FieldScriptFadeMode;

void FieldScriptStartFade1(void) {
    g_FieldScriptFadeDuration = FieldScriptVMGetArgument(1);
    g_FieldScriptFadeMode = 1;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptStartFade2(void) {
    g_FieldScriptFadeDuration = FieldScriptVMGetArgument(1);
    g_FieldScriptFadeMode = 2;
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
    g_FieldScriptFadeDuration = FieldScriptVMGetArgument(1);
    g_FieldScriptFadeMode = 3;
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

void FieldScriptSetActorColors(void) {
    ActorData* pActorData;
    int actorIndex = FieldScriptVMGetActorIndex(2);

    if (actorIndex != ACTOR_ID_INVALID) {
        pActorData = g_FieldActors[actorIndex].pActorData;
        if (SCRIPT_READ_U8_REL(1) & FIELD_ACTOR_COLOR_SET_PRIMARY) {
            pActorData->red = FieldScriptVMGetArgument(3);
            pActorData->green = FieldScriptVMGetArgument(5);
            pActorData->blue = FieldScriptVMGetArgument(7);
        }
        if (SCRIPT_READ_U8_REL(1) & FIELD_ACTOR_COLOR_SET_SECONDARY) {
            pActorData->red2 = FieldScriptVMGetArgument(3);
            pActorData->green2 = FieldScriptVMGetArgument(5);
            pActorData->blue2 = FieldScriptVMGetArgument(7);
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

void FieldScriptSetCurrentActorColors(void) {
    if (SCRIPT_READ_U8_REL(1) & FIELD_ACTOR_COLOR_SET_PRIMARY) {
        g_FieldScriptVMCurActor->red = FieldScriptVMGetArgument(2);
        g_FieldScriptVMCurActor->green = FieldScriptVMGetArgument(4);
        g_FieldScriptVMCurActor->blue = FieldScriptVMGetArgument(6);
    }
    if (SCRIPT_READ_U8_REL(1) & FIELD_ACTOR_COLOR_SET_SECONDARY) {
        g_FieldScriptVMCurActor->red2 = FieldScriptVMGetArgument(2);
        g_FieldScriptVMCurActor->green2 = FieldScriptVMGetArgument(4);
        g_FieldScriptVMCurActor->blue2 = FieldScriptVMGetArgument(6);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
}

extern void SpriteSetDirectionAndRefreshFrames(void*, int);

void FieldScriptSetSpriteDirection(void) {
    SpriteSetDirectionAndRefreshFrames(g_FieldActors[g_FieldCurActorIndex].pSpriteData, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern s32 g_FieldScreenScrollX;
extern s32 g_FieldScreenScrollY;

void FieldScriptSetScreenScroll(void) {
    g_FieldScreenScrollX = FieldScriptVMGetArgument(1);
    g_FieldScreenScrollY = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern FieldScriptGlobalS16 g_FieldSoundElementMask;

void FieldScriptSetSoundSlotFlags(void) {
    g_FieldSoundElementMask.value = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void SoundFadeElementVolumeByIndex(int, int, int);

void FieldScriptSetSoundElementVolumeFade(void) {
    SoundFadeElementVolumeByIndex(FieldScriptVMGetArgument(3) * 2, FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(5));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern void SoundSetElementVolumeByIndex(int, int);

void FieldScriptSetSoundElementLevel(void) {
    SoundSetElementVolumeByIndex(FieldScriptVMGetArgument(3) * 2, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

extern void SoundSetElementPanByIndex(int, int);

void FieldScriptSetSoundElementVolume(void) {
    SoundSetElementPanByIndex(FieldScriptVMGetArgument(3) * 2, FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptSetSoundSlotVolume(void) {
    FieldSetSoundSlotElement(FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(3));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptSetSoundSlotVolumeParams(void) {
    FieldPlaySoundElementPair(
        FieldScriptVMGetArgument(1),
        FieldScriptVMGetArgument(5),
        FieldScriptVMGetArgument(3),
        FieldScriptVMGetArgument(7)
    );
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
}

extern int SoundGetActiveElementMask(int);

void FieldScriptWaitSoundElementStopped(void) {
    int inputMask = SoundGetActiveElementMask(-1);

    if ((inputMask & (FieldScriptVMGetArgument(1) << 8)) == 0) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer -= 1;
    }
    g_FieldScriptVMStopFlag = 1;
}

extern void FieldSetSoundSlotElement(int, int);

void FieldScriptSetSoundSlotVolume3(void) {
    FieldSetSoundSlotElement(FieldScriptVMGetArgument(1), 3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

extern void FieldPlaySoundElementPair(int, int, int, int);

void FieldScriptSetSoundSlotVolumeParams3(void) {
    FieldPlaySoundElementPair(FieldScriptVMGetArgument(1), FieldScriptVMGetArgument(5), FieldScriptVMGetArgument(3), 3);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 7;
}

extern FieldScriptGlobalS32 g_FieldMapChangeGate;
extern FieldScriptGlobalS32 g_GameAudioFadeInPending;

void FieldScriptLoadCharacterData0(void) {
    if (g_FieldMapChangeGate.value == 0) {
        g_FieldScriptVMStopFlag = 1;
    } else {
        g_GameAudioFadeInPending.value = 0;
        func_8008F7B8();
    }
}

void FieldScriptLoadCharacterDataMinus1(void) {
    if (g_FieldMapChangeGate.value == 0) {
        g_FieldScriptVMStopFlag = 1;
    } else {
        g_GameAudioFadeInPending.value = -1;
        func_8008F7B8();
    }
}

extern s32 g_GameWdsStreamInProgress;
extern void FieldFreePendingAudioManager(void);
extern void SoundFreeLoadedWdsAndAudioManager(void);
extern void FieldStartWdsBankLoad(int, int);

// PARKED 92.24% (320 bytes against retail's 340, 80 insns against 85). Every
// guard, call, load and branch matches. Retail stores g_FieldScriptVMStopFlag
// TWICE: one block at .L8008F85C shared by the FieldArchiveTryDataSync/gate arms
// (source register $v0) and its own store at .L8008F8EC for the final arm
// (source register $v1, which still holds the 1 built for the
// g_GameWdsStreamInProgress == 1 compare). This build cross-jumps all three into
// ONE store at the function tail, so the whole 20-byte gap (5 insns) is that
// second lui+sw pair plus the j/nop that retail's early block needs, minus one
// li v0,1 that the merged block hoists into the loadState branch delay slot.
// The mechanism is the source register: at post-regalloc jump time the two
// stores are only mergeable when both read the same constant-1 pseudo, and the
// merged block's live range across the else code is what makes cse reuse it there.
// Measured, all 320 bytes / 92.24%: the two conditions split into separate
// else-ifs; a flat early-return form; an empty do-while around each arm; a
// volatile store on the else body (cc1 folds it back to the plain sw); the
// (s32)1 cast. Preset screen: 2.7.2 92.24, cdk 92.24, 2.6.0 92.24.
// Decisive A/B: writing the else arm as `= 0` instead of `= 1` keeps retail's
// exact block shape and scores 98.41% / 336 bytes (84 insns) - the only delta
// left is retail's extra `li v1,1` - so the shared body is right and nothing but
// the store's source register is in the way.
// Round 489 added the missing quadrant: an if/else-if/else chain whose store is
// the THEN block (`else if (stream == 1 || loadState == -1) { stop; } else
// { body; }`) DOES separate the two stores - retail's `sw $v1` register comes
// out exactly, and the first block keeps `sw $v0` + its own `j` - but gcc emits
// the then block before the else body, so the third store lands mid-function and
// pays a j+nop: 344 bytes / 86 insns / 90.82%. The store-as-`else` form merges
// again (92.24), with or without a `return;` in the then block. So both halves
// of retail's shape are reachable, never in one body: the block split needs the
// store to be a then-block, the position needs it to be the else/exit tail.
// Round 502 addendum: two more shapes measured dead on this body. Storing the
// condition itself (g_FieldScriptVMStopFlag = stream == 1 || loadState == -1)
// re-tests both globals and lands at 87.00% / 372 bytes / 93 insns. The
// preset screen was already run (2.7.2 92.24, cdk 92.24, 2.6.0 92.24).
/*
void func_8008F7B8(void) {
    int characterId = FieldScriptVMGetArgument(1);

    if (g_FieldEntryRoutineFinished == 0) {
        FieldFreePendingAudioManager();
        if (characterId != g_GameRequestedWdsBankIndex) {
            SoundFreeLoadedWdsAndAudioManager();
            g_GameWdsBankLoadState = -1;
        }
        g_GameRequestedWdsBankIndex = characterId;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
        return;
    }
    if (FieldArchiveTryDataSync() != 0 || g_FieldMapChangeGate.value == 0) {
        g_FieldScriptVMStopFlag = 1;
        return;
    }
    if (g_GameWdsStreamInProgress != 1 && g_GameWdsBankLoadState != -1) {
        if (characterId != g_GameRequestedWdsBankIndex) {
            SoundFreeLoadedWdsAndAudioManager();
            g_GameRequestedWdsBankIndex = characterId;
            g_GameWdsBankLoadState = -1;
            FieldStartWdsBankLoad(characterId, 0);
        }
        g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
        return;
    }
    g_FieldScriptVMStopFlag = (s32)1;
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc", func_8008F7B8);



void FieldScriptStartCameraShake(void) {
    int x = FieldScriptVMGetArgument(1);
    int y = FieldScriptVMGetArgument(3);
    int z = FieldScriptVMGetArgument(5);
    int frames = FieldScriptVMGetArgument(7);

    if (frames == 0) {
        frames = 1;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 9;
    g_Scene.shakeStepX = ((x << 16) - g_Scene.shakeAmplitudeX) / frames;
    g_Scene.shakeStepY = ((z << 16) - g_Scene.shakeAmplitudeY) / frames;
    g_Scene.shakeStepZ = ((y << 16) - g_Scene.shakeAmplitudeY) / frames;
    g_Scene.shakeFrames = frames;
    g_Scene.shakeActive = 1;
    if (x == 0 && z == 0 && y == 0) {
        g_Scene.shakeFrames = frames + 2;
        g_Scene.shakeStopWhenFinished = 1;
    } else {
        g_Scene.shakeStopWhenFinished = 0;
    }
}