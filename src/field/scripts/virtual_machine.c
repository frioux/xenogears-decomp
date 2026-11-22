#include "common.h"
#include "main/game.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/script_vm.h"
#include "field/effects.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/debug.h"
#include "psyq/libgpu.h"
#include "psyq/libcd.h"

extern s32 g_FieldScriptVMScriptEndFlag;
extern s32 g_FieldCurActorIndex; // Current actor index
extern s32 g_FieldScriptVMStopFlag;

extern void func_800379C8(char*, ...);
extern char g_FieldScriptVMStackErrorFormat[]; // "STACKERR ACT=%d\n"

// Script variable addresses this file writes, named for what the stored value is
#define SCRIPT_VAR_PLAYER_POS_X 0x1E
#define SCRIPT_VAR_PLAYER_POS_Z 0x20
#define SCRIPT_VAR_PLAYER_POS_Y 0x22
#define SCRIPT_VAR_MAP_NUM 0x3C
#define SCRIPT_VAR_PARTY_MEMBER_1 0x3E
#define SCRIPT_VAR_PARTY_MEMBER_2 0x40
#define SCRIPT_VAR_PARTY_MEMBER_3 0x42

// Store instruction pointer + 5 on stack
void FieldScriptVMHandlerCallScript(void) {
    if (g_FieldScriptVMCurActor->flags12C_0x6 != SCRIPT_MAX_STACK_SIZE) {
        g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6] = g_FieldScriptVMCurActor->scriptInstructionPointer + 5;
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(1);
        g_FieldScriptVMCurActor->flags12C_0x6++;
        return;
    }

    // Error
    if (g_FieldSystemMode == 0) {
        func_800379C8(g_FieldScriptVMStackErrorFormat, g_FieldCurActorIndex);
    }
    g_FieldScriptVMStopFlag = 1;
}

// Store instruction pointer + 3 on stack
void FieldScriptVMHandlerCallScriptShort(void) {
    if (g_FieldScriptVMCurActor->flags12C_0x6 != SCRIPT_MAX_STACK_SIZE) {
        g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6] = g_FieldScriptVMCurActor->scriptInstructionPointer + 3;
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(1);
        g_FieldScriptVMCurActor->flags12C_0x6++;
        return;
    }
    
    // Error
    if (g_FieldSystemMode == 0) {
        func_800379C8(g_FieldScriptVMStackErrorFormat, g_FieldCurActorIndex);
    }
    g_FieldScriptVMStopFlag = 1;
}

// Restore instruction pointer from stack
void FieldScriptVMHandlerReturn(void) {
    // Error, invalid stack value
    if (!g_FieldScriptVMCurActor->flags12C_0x6) {
        if (g_FieldSystemMode == 0) {
            func_800379C8(g_FieldScriptVMStackErrorFormat, g_FieldCurActorIndex);
        }
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 = 0xFF;
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].scriptId = 0xFF;
        g_FieldScriptVMScriptEndFlag = 1;
        g_FieldScriptVMStopFlag = 1;
        return;
    }
    
    g_FieldScriptVMCurActor->flags12C_0x6--;
    g_FieldScriptVMCurActor->scriptInstructionPointer = g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6];
}

void FieldScriptVMHandlerResetScriptSlots(void) {
    int i;
    
    for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        g_FieldScriptVMCurActor->scripts[i].waitTimer = 0;
        g_FieldScriptVMCurActor->scripts[i].state = SCRIPT_STATE_IDLE;
        g_FieldScriptVMCurActor->scripts[i].flags_0x12 = 0xF;
        g_FieldScriptVMCurActor->scripts[i].currentIP = 0xFFFF;
        g_FieldScriptVMCurActor->scripts[i].isInUse = 0;
        g_FieldScriptVMCurActor->scripts[i].scriptId = 0xFF;
        g_FieldScriptVMCurActor->scripts[i].flags_0 = 0xFFFF;
        g_FieldScriptVMCurActor->scripts[i].flags_0x17 = 0x0;
    }

    g_FieldScriptVMCurActor->curScriptIndex = 0;
    g_FieldScriptVMCurActor->waitScriptIndex = 0;
    g_FieldScriptVMCurActor->dialogFlags = 0;
    g_FieldScriptVMStopFlag = 1;
    g_FieldScriptVMCurActor->flags12C_0x6 = 0;
}

// Yield / Stop, but change IP conditionally
void FieldScriptVMHandlerStopAndRetargetQueuedScripts(void) {
    int i;

    for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        if (g_FieldScriptVMCurActor->scripts[i].flags_0x12 == 7) {
            g_FieldScriptVMCurActor->scripts[i].currentIP = FieldScriptGetBytecodeOffset(g_FieldCurActorIndex, 1);
        }
    }

    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 = 0xFF;
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].scriptId = 0xFF;
    g_FieldScriptVMStopFlag = 1;
}

// Yield / Stop Handler
void FieldScriptVMHandlerEndScript(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 = 0xFF;
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].scriptId = 0xFF;
    g_FieldScriptVMScriptEndFlag = 1;
    g_FieldScriptVMStopFlag = 1;
}

void FieldScriptVMHandlerConditionalJmp(void) {
    int nValue2;
    int nValue1;
    int nArgumentsType;
    int nCondCheck;
    int nCondResult;
    int nConditionType;

    nValue2 = 0;
    nValue1 = 0;
    nArgumentsType = SCRIPT_READ_U8_REL(5) & 0xF0;
    switch (nArgumentsType) {
    case 0x0:
        nValue1 = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF);
        nValue2 = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(3) & 0xFFFF);
        if (FieldScriptVMGetVariableSign(FieldScriptVMGetInstructionArgument(1) & 0xFFFF) != FIELD_SCRIPT_VM_VAR_SIGNED) {
            nValue2 = (u_short)nValue2;
        } else {
            nValue2 = (short)nValue2;
        }
        break;
    case 0x40:
        nValue1 = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(1) & 0xFFFF);
        nValue2 = FieldScriptVMGetInstructionArgumentS16(3);
        if (FieldScriptVMGetVariableSign(FieldScriptVMGetInstructionArgument(1) & 0xFFFF) != FIELD_SCRIPT_VM_VAR_SIGNED) {
            nValue2 = (u_short)nValue2;
        }
        break;
    case 0x80:
        nValue1 = FieldScriptVMGetInstructionArgumentS16(1);
        nValue2 = FieldScriptVMGetVariableValue(FieldScriptVMGetInstructionArgument(3) & 0xFFFF);
        if (FieldScriptVMGetVariableSign(FieldScriptVMGetInstructionArgument(3) & 0xFFFF) != FIELD_SCRIPT_VM_VAR_SIGNED) {
            nValue1 = (u_short)nValue1;
        }
        break;
    case 0xC0:
        nValue1 = FieldScriptVMGetInstructionArgumentS16(1);
        nValue2 = FieldScriptVMGetInstructionArgumentS16(3);
        break;
    }
    
    nCondResult = 0;
    nConditionType = SCRIPT_READ_U8_REL(5) & 0xF;
    switch (nConditionType) {
        case FIELD_SCRIPT_VM_COND_EQUAL:
            if (nValue1 == nValue2) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_NOT_EQUAL2:
            if (nValue1 != nValue2) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_LT:
            nCondCheck = nValue2 < nValue1;
            if (nCondCheck) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_LT2:
            nCondCheck = nValue1 < nValue2;
            if (nCondCheck) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_GTE:
            if (nValue1 >= nValue2) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_GTE2:
            if (nValue2 >= nValue1) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_AND:
            nCondCheck = nValue1 & nValue2;
            if (nCondCheck) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_NOT_EQUAL:
            if (nValue1 != nValue2) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_OR:
            nCondCheck = nValue1 | nValue2;
            if (nCondCheck) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_AND2:
            nCondCheck = nValue1 & nValue2;
            if (nCondCheck) nCondResult++;
            break;
        case FIELD_SCRIPT_VM_COND_NAND:
            nCondCheck = ~nValue1 & nValue2;
            if (nCondCheck) nCondResult++;
            break;
    }
    
    if (nCondResult == 1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer = g_FieldScriptVMCurActor->scriptInstructionPointer + 8;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(6);
    }
}

void FieldScriptVMHandlerJmp(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(1);
}

void FieldScriptVMHandlerExtendInstructionLimit(void) {
    g_FieldScriptMaxInstructionCount += 0x20;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMRun(int maxInstructionCount) {
    int nInstructionCount;
    u_short nInstructionPointer;
    u_char nHandlerIndex;

    g_FieldScriptVMStopFlag = 0;
    g_FieldScriptMaxInstructionCount = maxInstructionCount;

    for (nInstructionCount = 0; nInstructionCount < g_FieldScriptMaxInstructionCount; nInstructionCount++) {
        /* TODO: delete this asm() when we hit 100% matching.
         *
         * The current working theory is that there was some debugging code injected here, most likely a break
         * instruction, and they just changed the define to an empty string for the release build. This caused
         * the loop optimization for this function to break, which is why you see a noop in the jump's delay
         * slot. From all testing done with code structure and compiler arguments, this appears to be the only
         * way to make the function match.
         */
        asm(ASM_BREAKPOINT);
        if (nInstructionCount > 0x400) {
            if (g_FieldSystemMode == SYSTEM_PC_HARDDRIVE) {
                func_800379C8(g_FieldScriptVMEventLoopErrorFormat, g_FieldCurActorIndex); // Error printing
            }
            return;
        }
        nInstructionPointer = g_FieldScriptVMCurActor->scriptInstructionPointer;
        nHandlerIndex = SCRIPT_READ_U8(nInstructionPointer);
        g_FieldScriptVMHandlers[nHandlerIndex]();

        if (g_FieldScriptVMScriptEndFlag == 0) {
            g_FieldScriptMaxInstructionCount = 0xFFFF;
        }

        // Various exit checks
        if ((g_FieldEntryRoutineFinished != 0 && (g_FieldMapChangeRequestGate == 0 ||  g_FieldEntryStateGate == 0 ||  g_FieldMapTransitionGate == 0))) {
            return;
        }

        if (g_FieldScriptVMStopFlag == 1 && g_FieldScriptVMScriptEndFlag == g_FieldScriptVMStopFlag) {
            return;
        }

    }

}

// Round 306: permute.sh cannot run on this unit - its C parser rejects the preprocessed body
// ('conversion to non-scalar type requested' while randomizing), so the 90.55% tie below has
// had no permuter pass.
// Best attempt: 90.49% (636 bytes, same size as the original: only register assignment differs).
// - nMinPriority = 0xF must be assigned BEFORE the g_FieldPartyMoveInProgress check; the original keeps the
//   `li a2,0xF` in the delay slot of that beqz.
// - the "no free script slot" branch is taken when nMinPriority == 0xF, not !=.
// - the flags read-modify-write belongs BEFORE the g_FieldScriptVMCurFieldActor / g_FieldCurActorIndex / g_FieldScriptVMCurActor
//   stores, and the guard clauses above it must spell pFieldActor->pActorData->flags inline.
// Left: the original reloads pActorData from 0x4C($a1) in the join block and keeps pFieldActor in
// $a1 / pActorData in $a0; gcc here CSEs the 0x4C load across the two-pred join and lands on
// `addu $a0,$v0,$s1` with the roles swapped, which is 100% of what is left. Tried without change:
// explicit (u_char*)g_FieldActors + i*sizeof(FieldActor) byte arithmetic; a local pActor assigned
// twice; -G0/-O3 and gcc-2.7.2-cdk presets. Round 36 also: all 7 locals in 12 different declaration
// orders (90.49% every time) and gcc-2.6.0-psx (89.86%), so the temp-register split is a
// global.c allocno tie, not a source shape.
// Round 40 read of the original: the 'lbu g_FieldPartyMoveInProgress' is at 0x213C, BEFORE the three
// g_FieldScriptVMCurFieldActor / g_FieldCurActorIndex / g_FieldScriptVMCurActor stores at 0x2144-0x2158, and only the
// 'beqz' on it comes after them - so the source read the byte into a local between the flag
// read-modify-write and those stores (e.g. 'u_char bSlotLocked = g_FieldPartyMoveInProgress;').  Writing
// 'if (g_FieldPartyMoveInProgress != 0)' there puts the load at the branch instead.
// Round 41: 'u_char bSlotLocked = g_FieldPartyMoveInProgress;' before the three stores does put the lbu
// there (its 'beqz' delay slot then takes the 0xF), and the loop header must spell
// 'pFieldActor = (FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors);' - that is
// what makes the original's index-first 'addu a1,s1,v0' come out.  90.49 -> 90.55%.  What
// is left is the register swap of the two loop temps: retail keeps pFieldActor in $a1 and
// pActor in $a0, gcc puts pFieldActor in $a0 and pActor in $a1 (and reloads pActorData
// from 0x4C($a1) in the join block).  Sizes are identical (636 bytes).
/*
extern s32 g_FieldActiveActorCount;
extern s32 g_FieldEntryRoutineFinished;
extern s32 g_FieldMode;
extern FieldActor* g_FieldScriptVMCurFieldActor;
extern s32 D_800ADB68;
extern s32 D_800C4268;
extern u_char g_FieldPartyMoveInProgress;
extern s32 g_FieldPartyActorSlots[3];

// Runs the field script VM for every actor that has a script pending
void func_800A2030(void) {
    int i;
    int j;
    int nActorCount;
    int nMinPriority;
    u_short nInstructionPointer;
    FieldActor* pFieldActor;
    ActorData* pActor;
    u_char bSlotLocked;

    if (g_FieldMode == 1) {
        nActorCount = 1;
    } else {
        nActorCount = g_FieldActiveActorCount;
    }
    D_800ADB68 = 0;
    D_800C4268 = 0;

    for (i = 0; i < nActorCount; i++) {
        pFieldActor = (FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors);
        if (!(pFieldActor->status & 0xF00)) {
            continue;
        }

        if (pFieldActor->pActorData->flags & 0x100000) {
            continue;
        }

        if (g_FieldEntryRoutineFinished != 0 && (g_FieldMapChangeRequestGate == 0 || g_FieldEntryStateGate == 0 || g_FieldMapTransitionGate == 0)) {
            return;
        }

        pActor = pFieldActor->pActorData;
        pActor->scriptFlags.flags &= ~0x1000000;
        bSlotLocked = g_FieldPartyMoveInProgress;

        g_FieldScriptVMCurFieldActor = pFieldActor;
        g_FieldCurActorIndex = i;
        g_FieldScriptVMCurActor = pActor;

        // Pick the script slot with the lowest priority
        nMinPriority = 0xF;
        if (bSlotLocked != 0) {
            for (j = 0; j < 3; j++) {
                if (g_FieldPartyActorSlots[j] != ACTOR_ID_INVALID && g_FieldPartyActorSlots[j] == i) {
                    goto next_actor;
                }
            }
        }

        for (j = 0; j < ACTOR_MAX_NUM_SCRIPTS; j++) {
            if (g_FieldScriptVMCurActor->scripts[j].flags_0x12 <= nMinPriority) {
                nMinPriority = g_FieldScriptVMCurActor->scripts[j].flags_0x12;
                g_FieldScriptVMCurActor->curScriptIndex = j;
            }
        }

        if (nMinPriority == 0xF) {
            g_FieldScriptVMCurActor->scripts[0].currentIP = FieldScriptGetBytecodeOffset(i, 1);
            g_FieldScriptVMCurActor->curScriptIndex = 0;
            g_FieldScriptVMCurActor->scripts[0].flags_0x12 = 7;
        }

        nInstructionPointer = g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].currentIP;
        g_FieldScriptVMScriptEndFlag = 1;
        g_FieldScriptVMCurActor->scriptInstructionPointer = nInstructionPointer;
        if (!(g_FieldScriptVMCurActor->scriptFlags.flags & 1)) {
            FieldScriptVMRun(8);
        }
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].currentIP = g_FieldScriptVMCurActor->scriptInstructionPointer;

    next_actor:
        ;
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A2030);

// Changes current actor to the top of the field actor array and runs a script routine on it
extern FieldActor* g_FieldScriptVMCurFieldActor;
extern s32 g_FieldEntryRoutineFinished;
void FieldScriptRunFirstActorRoutine(int scriptRoutineIndex) {
    int i;
    ActorData* pNewActor;

    // Change current actor to the first actor in the list
    g_FieldScriptVMCurFieldActor = g_FieldActors;
    g_FieldScriptVMCurActor = g_FieldScriptVMCurFieldActor->pActorData;
    pNewActor = HeapAlloc(sizeof(ActorData), 0x1);
    *pNewActor = *g_FieldScriptVMCurFieldActor->pActorData;

    // Reset all event slots
    for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        g_FieldScriptVMCurActor->scripts[i].waitTimer = 0;
        g_FieldScriptVMCurActor->scripts[i].state = SCRIPT_STATE_IDLE;
        g_FieldScriptVMCurActor->scripts[i].flags_0x12 = 0xF;
        g_FieldScriptVMCurActor->scripts[i].currentIP = 0xFFFF;
        g_FieldScriptVMCurActor->scripts[i].isInUse = 0x0;
        g_FieldScriptVMCurActor->scripts[i].scriptId = 0xFF;
        g_FieldScriptVMCurActor->scripts[i].flags_0 = 0xFFFF;
        g_FieldScriptVMCurActor->scripts[i].flags_0x17 = 0x0;
    }
    
    g_FieldCurActorIndex = 0;
    g_FieldEntryRoutineFinished = 0;
    g_FieldScriptVMScriptEndFlag = 0;

    // Run entry point routine
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptGetBytecodeOffset(0, scriptRoutineIndex);
    FieldScriptVMRun(0xFFFF);
    g_FieldEntryRoutineFinished = 1;

    // Running VM bytecode will likely have changed some fields, so they are copied back into the top
    // of the actor array.
    *g_FieldScriptVMCurFieldActor->pActorData = *pNewActor;
    HeapFree(pNewActor);
}


extern s32 g_FieldScriptRoutineRunning;
extern void func_800ACE24();
void FieldScriptRunFirstActorRoutine3(void) {
    g_FieldScriptRoutineRunning = 1;
    FieldScriptRunFirstActorRoutine(3);
    func_800ACE24();
    g_FieldScriptRoutineRunning = 0;
}

// Best attempt: 74.92% (572 bytes vs 592; frame 0x30 vs 0x40, both with s0-s2+ra).
// Structural match: the count-guarded table loop, the player position.vy -= 8, the
// "if (i < g_FieldActiveActorCount) { if (flags12C_0) continue; } else if (mode == 0) deltas" /
// "if (mode == 1) deltas" pair, and the delta reads through one hoisted pointer to
// g_FieldScreenShakeMode with -0x16/-0x12/-0xE offsets (volatile, so gcc does not hoist them out of
// the loop - that alone was worth 7 points).  What is left:
//  1. the original materialises &g_FieldActorModelSlotCount ONCE ('lui v1,%hi; addiu v1,v1,%lo'), loads
//     the loop count through it ('lw v0,0(v1)') and derives the anim table from the same
//     register ('addiu s2,v1,-128') in the loop preheader, reloading the count as
//     'lw v0,0x80(s2)'.  This C gets three separate address materialisations because
//     gcc 2.7.2 folds '(u8*)&g_FieldActorModelSlotCount - 0x80' into 'la g_FieldActorModelSlotCount-128' and folds the
//     count load to %lo.  Tried: pCount/pAnimTable pointer locals, the inline form,
//     gcc-2.7.2-cdk (74.89%), -G0, -O3 - same shape every time.
//  2. per delta the original loads the delta value BETWEEN the g_FieldActors pointer load
//     and the 'addu' that finishes the actor address, which costs it a load-delay nop
//     (8 insns per field vs 7 here).  Loading the delta into a temp first, or storing
//     through a pointer local, does not move it.
//  3. the 0x10-byte-larger frame means the original had 0x30 bytes of locals this body
//     does not need; cosmetic next to (1) and (2).
// Round 41: (1) is the same fold_rtx wall as field/main/main.c func_80077884 - gcc folds
// the count load back to %lo and re-forms a separate lui/%lo for the table, so the two
// accesses are never expressed off one register.  Also tried the base-relative form
// 'for (i = 0; i < g_FieldActorModelSlotCount; i++) LibGearSetModelAnimation(i, 0, ((s16*)&g_FieldActorModelSlotCount)[i - 0x40);'
// - same result, and it drops the +0x80 reload shape.  (3) is 0x30 bytes of UNUSED locals in
// retail (its saved regs sit at 0x30-0x3C), so a dummy 0x30-byte local would fix the frame
// if the rest ever closes.
// Round 306: the model-animation loop can be made instruction-identical except its preheader.
// Best now 75.68% (568 bytes vs 592):
//     pAnimTable = (s16*)((u8*)&g_FieldActorModelSlotCount - 0x80);
//     for (i = 0; i < ((s32*)((u8*)pAnimTable + 0x80))[0]; i++)
//         LibGearSetModelAnimation(i, 0, pAnimTable[i]);
// reproduces the loop body exactly (lh 0(s0) walk, 'lw v0,0x80(s2)' count reload, the andi 0xFFFF
// in both branch delay slots). Retail's preheader materialises &g_FieldActorModelSlotCount into a
// call-clobbered register ('lui v1,%hi / addiu v1,v1,%lo / lw v0,0(v1)') and then derives the table
// base from it ('addiu s2,v1,-0x80'); the nearest spelling above emits 'lw %lo' + its own
// 'la count-0x80'. 'extern s32 g_FieldActorModelSlotCount[]' + pCount = <array> produces retail's
// la/lw preheader but hands the callee-saved register to &count instead of the table base.
extern s32 g_GamePartySkinsInitialized;
extern s32 g_FieldActorModelSlotCount[]; // animation id table ends 0x80 below the count word
extern s32 g_PlayerActorIndex;
extern s32 g_FieldActiveActorCount;
extern u_char g_FieldScreenShakeMode;
extern void func_800AD898(void);
extern void LibGearSetModelAnimation(u16 modelIndex, u16 bits, s32 arg2);

// Read through a call boundary: gcc 2.7.2 otherwise hoists this volatile load above
// the actor address computation of the next statement (retail loads it in between).
inline volatile s32 FieldShakeReadDeltaY(volatile u_char* pShake) {
    return *(volatile s32*)(pShake - 0x12);
}

void func_800A24C4(void) {
    int i;
    ActorData* pActorData;
    int new_var;
    s32* pCount;
    s16* pAnimTable;
    volatile u_char* pShake;

    if (!g_GamePartySkinsInitialized) {
        return;
    }

    FieldScriptRunFirstActorRoutine(2);
    func_800AD898();

    pCount = g_FieldActorModelSlotCount;
    pAnimTable = (s16*)pCount - 0x40;
    for (i = 0; i < ((s32*)((u8*)pAnimTable + 0x80))[0]; i++) {
        LibGearSetModelAnimation(i, 0, pAnimTable[i]);
    }

    pActorData = g_FieldActors[g_PlayerActorIndex].pActorData;
    if (pActorData->canInteract != 0xFF) {
        pActorData->position.vy -= 8;
    }

    // The shake deltas sit at -0x16..-0xE from the mode byte
    pShake = &g_FieldScreenShakeMode;
    for (i = 0; i < g_FieldNumActors; i++) {
        if (i < g_FieldActiveActorCount) {
            if (g_FieldActors[i].pActorData->flags12C_0) {
                continue;
            }
        } else {
            new_var = 1;
            if ((pShake[0] & 0x7F) == 0) {
                g_FieldActors[i].transformMatrix.t[0] += *(volatile s32*)(pShake - 0x16);
                g_FieldActors[i].transformMatrix.t[new_var] += FieldShakeReadDeltaY(pShake);
                g_FieldActors[i].transformMatrix.t[2] += *(volatile s32*)(pShake - 0xE);
            }
        }

        if ((pShake[0] & 0x7F) == 1) {
            g_FieldActors[i].transformMatrix.t[0] += *(volatile s32*)(pShake - 0x16);
            g_FieldActors[i].transformMatrix.t[1] += FieldShakeReadDeltaY(pShake);
            do { g_FieldActors[i].transformMatrix.t[2] += *(volatile s32*)(pShake - 0xE); } while (0);
        }
    }
}
extern s32 g_GamePartySkinsInitialized;
extern s32 g_FieldActiveActorCount;
void FieldLoadActorAnimationFiles(void) {
    ActorData* pActor;
    FieldActor* pFieldActors;
    int i;
    void* pData;

    if (g_GamePartySkinsInitialized) {
        // Read animation files
        for (i = 0; i < g_FieldActiveActorCount; i++) {
            ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0x0);
            pActor = g_FieldActors[i].pActorData;
            if (pActor->specialAnimArchiveIndex != -1) {
                g_FieldScriptVMCurActor = pActor;
                pData = HeapAlloc(ArchiveDecodeAlignedSize(pActor->specialAnimArchiveIndex, pActor) + 8, 0x0);
                g_FieldScriptVMCurActor->pSpecialAnimFile = pData;
                ArchiveReadFileToBuffer(g_FieldScriptVMCurActor->specialAnimArchiveIndex, pData, 0, CdlModeSpeed); // Read from disc into buffer
                ArchiveCdDataSync(0);
            }
        }

        // Set Special animation file
        for (i = 0; i < g_FieldActiveActorCount; i++) {
            pFieldActors = g_FieldActors;
            pActor = pFieldActors[i].pActorData;
            if (pActor->specialAnimArchiveIndex != -1) {
                SpriteSetSpecialAnimFile(pFieldActors[i].pSpriteData, pActor->pSpecialAnimFile);
            }
        }
        
        func_800A3C8C();
        
        if (g_FieldEffects.distortion.isActive) {
            FieldDistortionInitialize(1);
        }
        
        FieldScriptMemoryWriteU16(0x10, 0x0);
        FieldScriptWritePartyMemberIDs();

        // Apply rotation and scale
        for (i = 0; i < g_FieldActiveActorCount; i++) {
            FieldActorApplyRotationAndScale(i);
        }
    }
}

// Best attempt: 79.98% (1768 bytes against the original's 1772). The structure is exact: the
// sprite/party split of the first loop, both func_80076AC0 call sites, the 2/3-entry CLUT-rect
// switch on flags12C_0x10, the party sprite swap keyed on g_pGameState->gearRide, and both VM loops
// (bytecode offsets 2 then 0, then FieldScriptVMRun(0xFFFF) with the g_FieldScriptSpritesBuilt fallback).
// What is left is one saved register (frame 0x44 here against 0x48: the original keeps
// &g_FieldActors in $s2 and does `lw v0,0(s2)` throughout the two VM loops, this body re-forms
// lui/lw per access and so never needs $s2) plus the load sharing inside each case-body statement:
// the original reads g_FieldActors ONCE per statement and takes both pSpriteData and pActorData
// from that one read, while the volatile declaration in include/field/actor.h makes this body read
// it twice. Measured alternatives: the plain view ((FieldActor**)&g_FieldActors, the misc6 idiom)
// makes gcc keep the element pointer in a callee-saved register across the SpriteAllocClutRect call
// so every reload disappears (81.92% at 1616 bytes); re-assigning a local pointer before each
// statement reproduces the read count but reads 77.88%; the sprite path as the `then` branch
// (the other if/else polarity) reads 77.59%. Also: asm/field/nonmatchings/scripts/virtual_machine/
// func_800A28D4.s owns no data, so converting this stub cannot shift the overlay.
/*
extern void func_800A3474(void);
extern void func_80076AC0(int actorIndex, int spriteId, void* pAnimFile, int modelKind, int clutIndex, int spriteVariant,
                          int skipAnimTick);
extern void SpriteAllocClutRect(SpriteData* pSpriteData, int count, int allocFlags);
extern void* g_PartyDataBuffers[];
extern s32 g_FieldSceneReloadFlags;
extern s32 g_FieldPartyActorIndices[3];
extern s32 g_FieldPartyActorSlots[3];
extern s32 g_FieldScriptSpritesBuilt;

// The sprite blob's offset table: one offset per sprite, relative to the blob itself.
typedef struct {
    // 0x0 s32 unk0;
    // 0x4 s32 offsets[1];
} FieldSpriteDataBlob;

// Builds every actor's sprite from the source its script data selected, swaps the party's
// riders onto their gear's sprites, then runs each actor's script VM. Every actor access
// goes through g_FieldActors (volatile), so the loads are in the original's order.
void func_800A28D4(void) {
    ActorData* pActor;
    FieldSpriteDataBlob* pSpriteDataBlob;
    FieldActor* pFieldActor;
    void* pAnimFile;
    void* pSwap;
    void* pSpriteData;
    int i;
    int j;

    if (g_GamePartySkinsInitialized) {
        func_800A3474();

        for (i = 0; i < g_FieldActiveActorCount; i++) {
            pActor = g_FieldActors[i].pActorData;
            if (!(pActor->spriteSource & 0x80)) {
                pAnimFile = g_PartyDataBuffers[pActor->spriteSource];
            } else {
                pSpriteDataBlob = g_FieldSpriteData;
                pAnimFile = (void*)(pSpriteDataBlob->offsets[pActor->spriteSource & 0x7F] + (s32)pSpriteDataBlob);
            }
            func_80076AC0(i, pActor->spriteId, pAnimFile, (pActor->flags130 >> 28) & 3, pActor->flags134 & 0xF,
                          g_FieldActors[i].pActorData->spriteSource, (g_FieldActors[i].pActorData->flags134 >> 4) & 1);

            pFieldActor = &g_FieldActors[i];
            switch (pFieldActor->pActorData->flags12C_0x10) {
            case 1:
                SpriteAllocClutRect((SpriteData*)pFieldActor->pSpriteData, 2, 0);
                ((u_short*)((SpriteDataI3*)((SpriteData*)g_FieldActors[i].pSpriteData)->field_0x7C)->field_0x18)[2] =
                    g_FieldActors[i].pActorData->flags12C_0x12;
                ((u_short*)((SpriteDataI3*)((SpriteData*)g_FieldActors[i].pSpriteData)->field_0x7C)->field_0x18)[3] =
                    g_FieldActors[i].pActorData->flags130 & 0x1FF;
                break;
            case 2:
                SpriteAllocClutRect((SpriteData*)pFieldActor->pSpriteData, 3, 0);
                ((u_short*)((SpriteDataI3*)((SpriteData*)g_FieldActors[i].pSpriteData)->field_0x7C)->field_0x18)[2] =
                    g_FieldActors[i].pActorData->flags12C_0x12;
                ((u_short*)((SpriteDataI3*)((SpriteData*)g_FieldActors[i].pSpriteData)->field_0x7C)->field_0x18)[3] =
                    g_FieldActors[i].pActorData->flags130 & 0x1FF;
                ((u_short*)((SpriteDataI3*)((SpriteData*)g_FieldActors[i].pSpriteData)->field_0x7C)->field_0x18)[4] =
                    (g_FieldActors[i].pActorData->flags130 >> 9) & 0x3FF;
                ((u_short*)((SpriteDataI3*)((SpriteData*)g_FieldActors[i].pSpriteData)->field_0x7C)->field_0x18)[5] =
                    (g_FieldActors[i].pActorData->flags130 >> 19) & 0x1FF;
                break;
            }
        }

        // A party member riding a gear uses the gear's sprite; hand the two actors' sprites
        // over, or flag the model on the rider when the pair is not in gear.
        if (g_FieldSceneReloadFlags) {
            for (j = 0; j < MAX_PARTY_MEMBERS; j++) {
                if (g_FieldPartyActorSlots[j] != ACTOR_ID_INVALID) {
                    if (g_pGameState->gearRide[j] != 0) {
                        pSpriteData = g_FieldActors[g_FieldPartyActorIndices[j]].pSpriteData;
                        pSwap = g_FieldActors[g_FieldPartyActorSlots[j]].pSpriteData;
                        g_FieldActors[g_FieldPartyActorSlots[j]].pSpriteData = pSpriteData;
                        g_FieldActors[g_FieldPartyActorIndices[j]].pSpriteData = pSwap;

                        g_FieldActors[g_FieldPartyActorIndices[j]].pActorData->scriptFlags.flags |= 0x200;
                        g_FieldActors[g_FieldPartyActorIndices[j]].pActorData->scriptFlags.flags &= ~0x500;
                        g_FieldActors[g_FieldPartyActorIndices[j]].status |= 0x20;
                    } else {
                        g_FieldActors[g_FieldPartyActorIndices[j]].pActorData->scriptFlags.flags |= 0x400;
                        g_FieldActors[g_FieldPartyActorIndices[j]].pActorData->scriptFlags.flags &= ~0x300;
                    }
                }
            }
        }
    } else {
        FieldScriptMemoryWriteU16(0x10, 0x0);
        FieldScriptWritePartyMemberIDs();

        for (i = 0; i < g_FieldActiveActorCount; i++) {
            g_FieldCurActorIndex = i;
            g_FieldScriptVMCurFieldActor = &g_FieldActors[i];
            g_FieldScriptVMCurActor = g_FieldActors[i].pActorData;
            g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptGetBytecodeOffset(i, 2);
            if (SCRIPT_READ_U8(g_FieldScriptVMCurActor->scriptInstructionPointer) == 0) {
                g_FieldScriptVMCurActor->flags |= 0x4000000;
            }

            g_FieldCurActorIndex = i;
            g_FieldScriptVMCurFieldActor = &g_FieldActors[i];
            g_FieldScriptVMCurActor = g_FieldActors[i].pActorData;
            g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptGetBytecodeOffset(i, 0);
        }

        for (i = 0; i < g_FieldActiveActorCount; i++) {
            pFieldActor = &g_FieldActors[i];
            g_FieldCurActorIndex = i;
            g_FieldScriptSpritesBuilt = 0;
            g_FieldScriptVMScriptEndFlag = 0;
            g_FieldScriptVMCurFieldActor = pFieldActor;
            g_FieldScriptVMCurActor = pFieldActor->pActorData;
            FieldScriptVMRun(0xFFFF);

            // The script did not choose a model, so give the actor the first sprite in the blob
            if (g_FieldScriptSpritesBuilt == 0) {
                pSpriteDataBlob = g_FieldSpriteData;
                func_80076AC0(i, 0, (void*)(pSpriteDataBlob->offsets[0] + (s32)pSpriteDataBlob), 0, 0, 0x80, 0);
                g_FieldScriptVMCurActor->flags |= 0x800;
            }
        }
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A28D4);

void FieldScriptVMHandlerNop(void) {
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

// Script files contains a sections of sign bits for variables. 
// This function is a bit of a convoluted way to check if a bit in section of data is set or not.
int FieldScriptVMGetVariableSign(int index) {
    return -((g_FieldCurScriptFile->signBits[index >> 6] & (1 << ((index >> 1) & 0x1F))) != 0);
}

int FieldScriptVMGetVariableValue(int index) {
    if (!(g_FieldCurScriptFile->signBits[index >> 6] & (1 << ((index >> 1) & 0x1F)))) {
        return ((short*)&g_FieldScriptMemory)[index >> 1];
    } else {
        return ((u_short*)&g_FieldScriptMemory)[index >> 1];
    }
}

// The dead second test is what keeps the shift in its own register: a plain store coalesces
// both shifts onto $a0 (a global.c allocno tie), while here the same store in two branches
// gcc cross-jumps away emits no branch and leaves the shift to $a2.
void FieldScriptMemoryWriteU16(int index, int value) {
    u_short uValue;
    int nOffset;
    int nStoreOffset;

    uValue = value;
    nOffset = index >> 1;
    if (uValue) {
        ((u_short*)&g_FieldScriptMemory)[index >> 1] = uValue;
    } else if (index || nStoreOffset) {
        nStoreOffset = nOffset;
        ((u_short*)&g_FieldScriptMemory)[nStoreOffset] = uValue;
    } else {
        nStoreOffset = nOffset;
        ((u_short*)&g_FieldScriptMemory)[nStoreOffset] = uValue;
    }
}

// scriptIndex here refers to the index of the script, which will (always?) correspond to an entity index
// routineIndex is an index into the offset table, which points to a bytecode routine in that script.
u_short FieldScriptGetBytecodeOffset(int scriptIndex, int routineIndex) {
    u_short* pOffsets;
    int nOffset;

    pOffsets = (u_short*)&g_FieldCurScriptFile->metadata;
    nOffset = (scriptIndex * (SCRIPT_OFFSET_TABLE_SIZE / sizeof(u_short)) + routineIndex);
    return pOffsets[nOffset];
}

void FieldScriptWritePartyMemberIDs(void) {
    FieldScriptMemoryWriteU16(SCRIPT_VAR_PARTY_MEMBER_1, g_GamePartyMembers[0]);
    FieldScriptMemoryWriteU16(SCRIPT_VAR_PARTY_MEMBER_2, g_GamePartyMembers[1]);
    FieldScriptMemoryWriteU16(SCRIPT_VAR_PARTY_MEMBER_3, g_GamePartyMembers[2]);
}

extern u16 D_8005941C;
extern u_char g_GameDiscChangeRequest;
extern s32 g_GameRequestedWdsBankIndex;
extern s32 g_GameSceneMapNum;
extern int FieldGetPlayerActorDirection(void);
extern int FieldGetCameraDirection(void);
void FieldScriptSyncMemoryWithGameState(void) {
    int i;

    g_pGameState->mapNum = g_GameSceneMapNum;
    g_pGameState->wdsBankIndex = g_GameRequestedWdsBankIndex;
    g_pGameState->unk2320 = g_pGameState->fieldScriptMemory[1];
    g_pGameState->entryAngle = g_pGameState->fieldScriptMemory[4] << 9;
    FieldScriptMemoryWriteU16(0x44, D_8005941C);
    FieldScriptMemoryWriteU16(0x46, g_GameDiscChangeRequest);
    FieldScriptMemoryWriteU16(6, FieldGetPlayerActorDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(8, FieldGetCameraDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(0x24, g_Scene.sceneDIP);
    FieldScriptMemoryWriteU16(SCRIPT_VAR_MAP_NUM, g_GameSceneMapNum);
    FieldScriptWritePartyMemberIDs();

    for (i = 0; i < 0x200; i++) {
        g_pGameState->fieldScriptMemory[i] = ((u_short*)&g_FieldScriptMemory)[i];
    }
}

extern u16 g_FieldScriptButtonsHeld;
extern u16 g_FieldP1ButtonsPressed;
extern char g_PlayTimeSeconds;
extern char g_PlayTimeMinutes;
extern char g_PlayTimeHours;
extern s32 g_PlayerActorIndex;
extern u8 g_FieldScriptMemoryUpdateLocked;
extern s32 g_FieldScriptWalkFrameCounter;
extern s32 g_FieldScriptWalkFlags;
extern s32 D_8004F2F4;
extern void FieldScriptWritePartyMemberPosition(int actorIndex);

// Refreshes the script memory from the field state, and every 0x1F calls walks the map position pair
void FieldScriptUpdateMemoryEachFrame(void) {
    int i;
    int nValue;
    int nLow;
    int nHigh;

    if (g_FieldScriptMemoryUpdateLocked == 1) {
        return;
    }

    g_FieldScriptButtonsHeld |= g_FieldP1ButtonsPressed;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        g_pGameState->partyMembers[i] = g_GamePartyMembers[i];
    }

    FieldScriptSyncMemoryWithGameState();
    D_8004F2F4 = 0;
    g_FieldScriptWalkFrameCounter++;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_pGameState->gearRide[i] == 1) {
            FieldScriptWritePartyMemberPosition(i);
        }
    }

    if (g_FieldScriptWalkFrameCounter >= 0x1F) {
        g_FieldScriptWalkFrameCounter = 0;

        // Step the map position pair in the direction the script asked for
        if (!(g_FieldScriptWalkFlags & 0x80)) {
            nValue = FieldScriptVMGetVariableValue(0xA);
            nLow = nValue & 0xFF;
            nHigh = (nValue >> 8) & 0xFF;

            if (!(g_FieldScriptWalkFlags & 4)) {
                if (nLow != 0xFF3B) {
                    nLow++;
                    if (nLow >= 0x3D) {
                        nLow = 0;
                        nHigh++;
                    }
                }
            } else {
                if (nLow == 0) {
                    if (nHigh != 0) {
                        nLow = 0x3B;
                        nHigh--;
                    }
                } else {
                    nLow--;
                }
            }

            FieldScriptMemoryWriteU16(0xA, (nHigh << 8) | (nLow & 0xFF));
        }
    }

    FieldScriptMemoryWriteU16(SCRIPT_VAR_SYSTEM_TIME_LOW, g_PlayTimeSeconds | (g_PlayTimeMinutes << 8));
    FieldScriptMemoryWriteU16(SCRIPT_VAR_SYSTEM_TIME_HIGH, g_PlayTimeHours);

    FieldScriptMemoryWriteU16(SCRIPT_VAR_PLAYER_POS_X, ((s16*)&g_FieldActors[g_PlayerActorIndex].pActorData->position)[1]);
    FieldScriptMemoryWriteU16(SCRIPT_VAR_PLAYER_POS_Z, ((s16*)&g_FieldActors[g_PlayerActorIndex].pActorData->position)[5]);
    FieldScriptMemoryWriteU16(SCRIPT_VAR_PLAYER_POS_Y, ((s16*)&g_FieldActors[g_PlayerActorIndex].pActorData->position)[3]);
}

INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3474);

// Best attempt: 86.75% (692 bytes vs 696). Everything structural already matches: the 0x74-byte
// memcpy through `(u8*)&g_FieldNumActors - 0xB8` reproduces the original's two-loop
// mips_expand_block_move verbatim (the (src|dst)&3 alignment test, the lwl/lwr loop, the lw loop and
// the 4-byte tail), and both loops plus the SpriteRestoreAnimState guard chain match instruction for
// instruction. What is left is register roles and one missing reload:
//  1. the original keeps &g_FieldNumActors in $a0 and derives $a3 = $a0 - 0xB8, and keeps
//     &g_WorldMapSaveBlock in $a1 (addiu $v0,$a1,0x3C / move $a2,$v0); gcc here folds the -0xB8 into the
//     symbol (lui $a3 / addiu $a3,-184) and puts &g_WorldMapSaveBlock in a temp. Tried inline expressions and
//     an explicit pSrc local, no change.
//  2. the original reloads pActorData from the FieldActor in the last block (lw $v0,0x4C($a1)) and
//     tests flags12C with `andi $v0,$v0,0x1000`; gcc keeps pActorData in $a0 across the jal and
//     reports the flags12C read as a plain word test here (it needs the u_int cast to avoid a
//     srl/andi pair, that part is now right).
// Round 306: the body below copies from 'pSrc' = &g_WorldMapSaveBlock, but retail copies from
// &g_WorldMapSaveBlock + 0x3C ('addiu v0,a1,0x3C / move a2,v0' then the lwl/lwr loop, and its loop
// end pointer is save+0xAC). Also measured: the two-statement 'pDst = (u8*)&g_FieldNumActors;
// pDst -= 0xB8;' form still folds the -0xB8 into the la addend ('addiu a3,a3,-184' with the %lo
// relocation) and scores 84.25%, so the materialise-then-adjust shape needs another lever.
/*
extern u8 g_WorldMapSaveBlock;
extern u8* g_WorldMapSaveCursor;
extern s32 g_SavedGearRide[3];
extern s32 g_FieldActiveActorCount;
extern s32 g_FieldSceneReloadFlags;
extern void SpriteRestoreAnimState(void*, u8*);
void func_800A3C8C(void) {
    int i;
    int nDiff;
    ActorData* pActorData;
    u8* pCursor;
    u8* pSrc;

    pSrc = &g_WorldMapSaveBlock;
    memcpy((u8*)&g_FieldNumActors - 0xB8, pSrc, 0x74);
    g_FieldNumActors = *pSrc;
    g_WorldMapSaveCursor = pSrc;
    g_WorldMapSaveCursor = &g_WorldMapSaveBlock + 0x3C;
    g_WorldMapSaveCursor += 0x920;

    nDiff = 0;
    for (i = 0; i < 3; i++) {
        if (g_SavedGearRide[i] != g_pGameState->gearRide[i]) {
            nDiff++;
        }
    }

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pActorData = g_FieldActors[i].pActorData;
        pCursor = g_WorldMapSaveCursor;
        g_WorldMapSaveCursor = pCursor + 0xC;
        if (pActorData->specialAnimArchiveIndex != -1 && pActorData->animationOverride != 0xFF) {
            *(u_short*)(pCursor + 0x20) = pActorData->animationOverride;
        }

        pActorData = g_FieldActors[i].pActorData;
        if (!(pActorData->flags & 0x1000000) &&
            (g_FieldSceneReloadFlags == 0 || !(pActorData->scriptFlags.flags & 0x600) || nDiff == 0)) {
            SpriteRestoreAnimState(g_FieldActors[i].pSpriteData, g_WorldMapSaveCursor);
        }

        pActorData = g_FieldActors[i].pActorData;
        pCursor = g_WorldMapSaveCursor;
        g_WorldMapSaveCursor = pCursor + 0x168;
        if (pActorData->flags134 & 0x80) {
            g_WorldMapSaveCursor = pCursor + 0x174;
        }
        if (*(u_int*)((u_char*)pActorData + 0x12C) & 0x1000) {
            g_WorldMapSaveCursor += 0x10;
        }
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3C8C);

INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3F4C);

// PARKED 99.26% at retail's exact 2044 bytes: 511 insns on both sides, 22 differing, every one of
// them a register preference. Retail merges each `cursor + N` sum into the register already holding
// the loaded cursor (`lw v0,cursor / addiu v0,v0,116 / move a3,v0`), so no extra pseudo exists and
// $3 stays free for the tail's &g_WorldMapSaveBlock (retail keeps it in s1, we use v1). Our build
// keeps the load in v0 and the sum in a fresh v1 at the four middle chunk sites, swaps base and
// cursor temp in the head block (retail base v1 / sum v0), loads g_FieldActors after the cursor
// read in the actor loop (retail loads it first), and puts src first in the script-memory copy's
// alignment `or` (retail: dst first). The actor loop's 0x30 site DOES tie in place on both sides,
// so the tie itself is reachable; the four chunk sites simply do not.
// Measured and rejected this round: `pDst = cursor; pDst += N;` 99.10 (68 lines), cursor store
// moved after the memcpy 93.23 (2024 bytes, 506 insns), nSaveSize passed to the SAVESIZE printf
// with the two tail statements swapped 98.94 (2048 bytes), gcc-2.6.0 98.88, gcc-2.7.2-cdk
// compile-failed, reordered local declarations no change. Permuter from this body, 3 min, 1273
// iterations: base=240 best=240, no lead; six earlier runs (3/6/7/8/12/12 min) also ended at 240.
// Rejected by an earlier round and kept here so it is not retried: pDst defined before the base
// store (2040), embedded cursor store `pDst = (g_WorldMapSaveCursor = 4 + block)` (2040), a pSave
// local for the head, hoisting pActorData above the cursor read (2052), every one-statement
// spelling of the pActorData copy.
/*
extern u8 g_WorldMapSaveBlock[];
extern u8* g_WorldMapSaveCursor;
extern s32 g_FieldActiveActorCount;
extern s32 g_SavedGearRide[3];
extern u_int* g_FieldWalkmeshMaterialFlags;
extern VECTOR g_CameraEye;
extern u8 g_FieldScreenEffectData[];
extern char D_8006FD9C[]; // "SAVESIZE=%d %x\n"
extern void SpriteSaveAnimState(SpriteData* pSpriteData, void* pState);
extern void *memcpy();

// Saves the field state the world map overwrites into the block after
// g_WorldMapSaveBlock, one memcpy per state block, and back into g_SavedGearRide.
// FieldScriptVMSaveAnimStates (func_800A3C8C) is the reverse.
void func_800A3F4C(void) {
    ActorData* pActorData;
    void** pScriptMemory;
    u8* pDst;
    SVECTOR* pRot;
    s32 nStatus;
    s32 nSaveSize;
    s32 nSystemMode;
    int i;

    g_WorldMapSaveCursor = g_WorldMapSaveBlock;
    pDst = 4 + g_WorldMapSaveBlock;
    g_WorldMapSaveBlock[0] = g_FieldNumActors;
    g_WorldMapSaveCursor = pDst;
    memcpy(pDst, g_FieldScreenEffectData, 0x38);

    pDst = g_WorldMapSaveCursor + 0x38;
    pScriptMemory = &g_FieldScriptMemory;
    g_WorldMapSaveCursor = pDst;
    memcpy(pDst, (u8*)&g_Scene + 0xC4, 0x74);

    pDst = g_WorldMapSaveCursor + 0x74;
    g_WorldMapSaveCursor = pDst;
    memcpy(pDst, g_FieldWalkmeshMaterialFlags, 0x400);

    pDst = g_WorldMapSaveCursor + 0x400;
    g_WorldMapSaveCursor = pDst;
    memcpy(pDst, &g_FieldEffects, 0x2E4);

    pDst = g_WorldMapSaveCursor + 0x2E4;
    g_WorldMapSaveCursor = pDst;
    memcpy(pDst, &g_CameraEye, 0x1C8);

    g_WorldMapSaveCursor += 0x1C8;

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pRot = &g_FieldActors[i].rotation;
        memcpy(g_WorldMapSaveCursor, pRot, 8);
        nStatus = (u_short)g_FieldActors[i].status;
        pDst = (g_WorldMapSaveCursor = g_WorldMapSaveCursor + 8);
        memcpy(pDst, &nStatus, 4);

        pDst = (g_WorldMapSaveCursor = g_WorldMapSaveCursor + 4);
        SpriteSaveAnimState((g_FieldActors + i)->pSpriteData, pDst);

        pDst = g_WorldMapSaveCursor + 0x30;
        g_WorldMapSaveCursor = pDst;
        memcpy(pDst, g_FieldActors[i].pActorData, sizeof(ActorData));

        pDst = (g_WorldMapSaveCursor = g_WorldMapSaveCursor + 0x138);
        pActorData = g_FieldActors[i].pActorData;
        if (pActorData->flags134 & 0x80) {
            memcpy(pDst, pActorData->pTargetTrack, 0xC);
            g_WorldMapSaveCursor += 0xC;
        }
        pActorData = g_FieldActors[i].pActorData;
        if (pActorData->flags12C_0xD) {
            memcpy(g_WorldMapSaveCursor, pActorData->pScriptArgs, 0x10);
            g_WorldMapSaveCursor += 0x10;
        }
    }

    memcpy(g_WorldMapSaveCursor, pScriptMemory, 0x800);
    g_WorldMapSaveCursor += 0x800;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        g_SavedGearRide[i] = g_pGameState->gearRide[i];
    }

    nSystemMode = g_FieldSystemMode;
    nSaveSize = g_WorldMapSaveCursor - g_WorldMapSaveBlock;
    if (nSystemMode == 0) {
        func_800379C8(D_8006FD9C, g_WorldMapSaveCursor - g_WorldMapSaveBlock, g_WorldMapSaveCursor - g_WorldMapSaveBlock);
    }
}
*/

void FieldCopyScreenToFadeTexture(void) {
    FieldCopyScreenToVram(0x2c0,0x100);
}

void FieldCopyScreenToVram(int x, int y) {
    RECT rect;

    rect.w = 0x140;
    rect.y = 0;
    rect.x = 0;
    rect.h = 0xe0;
    SetGeomScreen(0x200);
    MoveImage(&rect, x, y);
    FieldRenderSync();
}