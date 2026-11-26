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
// Round 480: re-measured 90.55% / 636 bytes / 159 insns, retail's own size. The register
// swap is a CONSEQUENCE of one missing instruction, not an allocno tie: retail loads
// pActorData twice - a temp in $v0 for the flags guard at 0x20B8, then a fresh
// `lw a0,0x4C(a1)` in the join block at 0x2124 - so its pActorData pseudo is born late and
// short-lived and out-prioritises pFieldActor in global.c. gcc CSEs the two 0x4C loads here
// (the gate checks between them are plain loads), which makes pActorData long-lived, hands
// it $a1 and pushes pFieldActor to $a0 - the whole 68-line diff follows from that one
// eliminated load. Nothing else is missing. Presets this round: gcc-2.7.2-cdk 85.14,
// gcc-2.6.0 89.86, -O3 39.52.
extern s32 g_FieldMode;
extern s32 g_FieldActiveActorCount;
extern s32 D_800ADB68;
extern s32 D_800C4268;
extern u_char g_FieldPartyMoveInProgress;
extern s32 g_FieldPartyActorSlots[MAX_PARTY_MEMBERS];
extern FieldActor* g_FieldScriptVMCurFieldActor;

/*
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

// PARKED 90.44% (604 bytes against retail's 592, 151 vs 148 insns). Was 85.58 / 592 / size-exact.
// THE LEVER WAS THE EMPTY do/while BARRIER: every one of the six shake updates must be its own
// statement boundary, `do { <update> } while (0);`. Without barriers gcc CSEs the element
// pointer across the three updates and the group shrinks from retail's 3x8=24 insns to 18
// (76.59%, 143 insns); with one barrier per update 5 of the 6 blocks come out VERBATIM as
// retail's 8-insn block
//     lui a0,%hi(g_FieldActors) / lw a0 / lw v1,DELTA(a2) / addu a0,a1,a0 /
//     lw v0,FIELD(a0) / nop / addu v0,v0,v1 / sw v0,FIELD(a0)
// (all three delta reads are then plain `*(volatile s32*)(pShake - 0x16/-0x12/-0xE)`: routing
// t[1] through a static-inline helper, which the old 85.58 body needed, is now WORSE - 88.06).
// What is left:
//  1. the t[1] and t[2] blocks (in BOTH groups) sink the delta load one instruction late:
//     `lw a0,g_FieldActors / nop / addu / lw v0,36(a0) / lw v1,-18(a2) / nop / addu / sw`
//     = 9 insns, retail 8. The addu is scheduled above the delta load in ours and below it in
//     retail; that is the whole 604 vs 592. Tried: the helper on t[1] only (88.06), the
//     barrier written after each update instead of around it (identical code), plain reads
//     with no barriers (76.59).
//  2. frame 0x30 vs retail's 0x40: retail carries vars=32 with NO sp reference anywhere, ours
//     16 - an unreachable 16-byte local in the original, not a spelling we can force.
//  3. anim loop preheader matches except the blez delay slot: ours takes `addiu s2,v0,-128`,
//     retail `andi a0,s1,0xFFFF`. Player block: retail keeps canInteract in $s1 and pActorData
//     in $v1, ours $v1/$a0 (allocator ties, same instruction count).
// Presets on this body: gcc-2.6.0 76.86, gcc-2.7.2-cdk compile-fails. Earlier rounds: permuter
// 3 runs on the 85.58 body, PERMUTER_BETTER each (3056->1867, 2606->2054, 2054->1926).
/*
void func_800A24C4(void) {
    int i;
    ActorData* pActorData;
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
            if ((pShake[0] & 0x7F) == 0) {
                do { g_FieldActors[i].transformMatrix.t[0] += *(volatile s32*)(pShake - 0x16); } while (0);
                do { g_FieldActors[i].transformMatrix.t[1] += *(volatile s32*)(pShake - 0x12); } while (0);
                do { g_FieldActors[i].transformMatrix.t[2] += *(volatile s32*)(pShake - 0xE); } while (0);
            }
        }

        if ((pShake[0] & 0x7F) == 1) {
            do { g_FieldActors[i].transformMatrix.t[0] += *(volatile s32*)(pShake - 0x16); } while (0);
            do { g_FieldActors[i].transformMatrix.t[1] += *(volatile s32*)(pShake - 0x12); } while (0);
            do { g_FieldActors[i].transformMatrix.t[2] += *(volatile s32*)(pShake - 0xE); } while (0);
        }
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A24C4);

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

extern void func_800A3474(void);
extern void func_80076AC0(int actorIndex, int spriteId, void* pAnimFile, int modelKind, int clutIndex, int spriteVariant,
                          int skipAnimTick);
extern void SpriteAllocClutRect(SpriteData* pSpriteData, s32 count, u_int allocFlags);
extern void* g_PartyDataBuffers[];
extern s32 g_GamePartySkinsInitialized;
extern s32 g_FieldSceneReloadFlags;
extern s32 g_FieldPartyActorIndices[MAX_PARTY_MEMBERS];
extern s32 g_FieldPartyActorSlots[MAX_PARTY_MEMBERS];
extern s32 g_FieldScriptSpritesBuilt;

// The sprite blob g_FieldSpriteData points at: one s32 offset per sprite, measured from the
// blob's own start, so the first sprite sits at offsets[0] (0x4).
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 offsets[1];
} FieldSpriteDataBlob;

// Builds every actor's sprite from the source its script data selected, swaps the party's
// riders onto their gear's sprites, then runs each actor's script VM.
// PARKED at 86.63% (1772 bytes = retail's exactly, 443 insns = retail's exactly, 272 differing lines).
// The structure is right and the size is exact; what is left is register allocation.
//   THE STRUCTURE (a semantics fix, not a spelling, round 499): retail calls func_80076AC0 inside BOTH
//   arms of the spriteSource test with a different 3rd argument, and the CLUT switch lives in the
//   `spriteSource & 0x80` arm ONLY - the party-buffer arm ends with `j .L800A2BAC` (the loop tail) at
//   0x29B4, skipping the switch. The earlier park called once and ran the switch for every actor.
//   Splitting the call took the body from 1744 to 1836 bytes (83.19% -> 88.14%); giving each of the six
//   CLUT stores its own element pointer (`pFieldActor = &g_FieldActors[i];`) then made the instruction
//   count exactly retail's 443 at 86.63%. objdiff's fuzzy percentage is NOT monotone (the 459-insn body
//   read 88.14, this exact-size one 86.63), so judge with the count and the differing-line count.
//   KEPT FROM THE EARLIER PARK: the else branch (both VM loops and the FieldScriptVMRun fallback) is
//   exact, and it is where retail's $s2 comes from - it keeps &g_FieldActors in $s2 across the loops
//   (one `lw v0,0(s2)` per use) exactly as `ppFieldActors = (FieldActor**)&g_FieldActors;` does, the
//   misc.c idiom. ActorData::flags130 / flags134 are UNSIGNED in retail (`srl`, while the header
//   declares both as int), so every shift needs its (u_int) cast. The first VM loop's bytecode offset
//   must be a u16 local (retail's `sh 0xCC` then `andi 0xFFFF` around the SCRIPT_READ_U8 index); the
//   second loop stores the callee's return value directly, i.e. no local there.
// WHAT IS LEFT, none of it structure:
//   1. $s0/$s1 are mirrored through the whole first loop: retail keeps i in $s1 and the element-offset
//      IV in $s0, this body the opposite. Five declaration orders (ints first, j/i swapped, offset last,
//      pFieldActor last, u_int i) measure byte-identical, so it is a callee-saved allocation tie. It also
//      costs j: retail's party loop counter reuses $s1 (`move s1,zero` at 0x14A0), this body's gets $a3.
//   2. The gear-swap arm is 142 insns against retail's 135. Retail reads g_FieldActors ONCE for the three
//      statements that share a block and once more after the store (2 reads for 4 mentions), which is
//      non-volatile CSE killed by the aliasing store; with the header's `volatile` every mention reloads.
//      MEASURED both ways: a non-volatile g_FieldActors (one-line header edit, reverted) drops the whole
//      body to 402 insns because gcc then hoists the load out of the actor loop into $t1/$a1, and the
//      local view (*ppFieldActors)[i] (the misc.c idiom used by the else branch) gives 434. So neither
//      qualifier reproduces retail's per-block reload; the per-statement locals above do.
//   3. The else branch is 111 insns against 113: retail reaches g_FieldSpriteData as 0xC(s2), off the
//      &g_FieldActors base it keeps in $s2, where this body forms the address with lui/lw.
// Tried and reverted besides the above: cdk preset 83.51, gcc-2.6.0 will not compile the unit, a separate
// pCurActor variable for the arms (the loop's load count drops to 167 insns), six declaration orders.
/*
void func_800A28D4(void) {
    ActorData* pActor;
    FieldSpriteDataBlob* pSpriteDataBlob;
    FieldActor* pFieldActor;
    FieldActor** ppFieldActors;
    void* pAnimFile;
    void* pSwap;
    void* pSpriteData;
    u16 offset;
    int i;
    int j;

    if (g_GamePartySkinsInitialized) {
        func_800A3474();

        for (i = 0; i < g_FieldActiveActorCount; i++) {
            pActor = g_FieldActors[i].pActorData;
            if (!(pActor->spriteSource & 0x80)) {
                func_80076AC0(i, pActor->spriteId, g_PartyDataBuffers[pActor->spriteSource],
                              (u_int)pActor->flags130 >> 28, pActor->flags134 & 0xF,
                              g_FieldActors[i].pActorData->spriteSource,
                              (u_int)g_FieldActors[i].pActorData->flags134 >> 4 & 1);
            } else {
                pSpriteDataBlob = g_FieldSpriteData;
                func_80076AC0(i, pActor->spriteId,
                              (void*)(pSpriteDataBlob->offsets[pActor->spriteSource & 0x7F] + (s32)pSpriteDataBlob),
                              (u_int)pActor->flags130 >> 28, pActor->flags134 & 0xF,
                              g_FieldActors[i].pActorData->spriteSource,
                              (u_int)g_FieldActors[i].pActorData->flags134 >> 4 & 1);

            pFieldActor = &g_FieldActors[i];
            switch (pFieldActor->pActorData->flags12C_0x10) {
            case 1:
                SpriteAllocClutRect(pFieldActor->pSpriteData, 2, 0);
                pFieldActor = &g_FieldActors[i];
                ((u_short*)((SpriteDataI3*)((SpriteData*)pFieldActor->pSpriteData)->field_0x7C)->field_0x18)[2] =
                    pFieldActor->pActorData->flags12C_0x12;
                pFieldActor = &g_FieldActors[i];
                ((u_short*)((SpriteDataI3*)((SpriteData*)pFieldActor->pSpriteData)->field_0x7C)->field_0x18)[3] =
                    pFieldActor->pActorData->flags130 & 0x1FF;
                break;
            case 2:
                SpriteAllocClutRect(pFieldActor->pSpriteData, 3, 0);
                pFieldActor = &g_FieldActors[i];
                ((u_short*)((SpriteDataI3*)((SpriteData*)pFieldActor->pSpriteData)->field_0x7C)->field_0x18)[2] =
                    pFieldActor->pActorData->flags12C_0x12;
                pFieldActor = &g_FieldActors[i];
                ((u_short*)((SpriteDataI3*)((SpriteData*)pFieldActor->pSpriteData)->field_0x7C)->field_0x18)[3] =
                    pFieldActor->pActorData->flags130 & 0x1FF;
                pFieldActor = &g_FieldActors[i];
                ((u_short*)((SpriteDataI3*)((SpriteData*)pFieldActor->pSpriteData)->field_0x7C)->field_0x18)[4] =
                    (u_int)pFieldActor->pActorData->flags130 >> 9 & 0x3FF;
                pFieldActor = &g_FieldActors[i];
                ((u_short*)((SpriteDataI3*)((SpriteData*)pFieldActor->pSpriteData)->field_0x7C)->field_0x18)[5] =
                    (u_int)pFieldActor->pActorData->flags130 >> 19 & 0x1FF;
                break;
            }
            }
        }

        // A party member riding a gear shows the gear's sprite, so the two actors' sprites are
        // handed over; a rider on foot gets its own model flagged instead.
        if (g_FieldSceneReloadFlags) {
            for (j = 0; j < MAX_PARTY_MEMBERS; j++) {
                if (g_FieldPartyActorSlots[j] != ACTOR_ID_INVALID) {
                    if (g_pGameState->gearRide[j]) {
                        pSpriteData = g_FieldActors[g_FieldPartyActorIndices[j]].pSpriteData;
                        pSwap = g_FieldActors[g_FieldPartyActorSlots[j]].pSpriteData;
                        g_FieldActors[g_FieldPartyActorSlots[j]].pSpriteData = pSpriteData;
                        g_FieldActors[g_FieldPartyActorIndices[j]].pSpriteData = pSwap;

                        g_FieldActors[g_FieldPartyActorIndices[j]].pActorData->scriptFlags.flags |= 0x200;
                        g_FieldActors[g_FieldPartyActorIndices[j]].pActorData->scriptFlags.flags &= ~0x500;
                        g_FieldActors[g_FieldPartyActorIndices[j]].status |= ACTOR_STATUS_INVISIBLE;
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
        ppFieldActors = (FieldActor**)&g_FieldActors;

        for (i = 0; i < g_FieldActiveActorCount; i++) {
            pFieldActor = &(*ppFieldActors)[i];
            g_FieldCurActorIndex = i;
            g_FieldScriptVMCurFieldActor = pFieldActor;
            g_FieldScriptVMCurActor = pFieldActor->pActorData;
            offset = FieldScriptGetBytecodeOffset(i, 2);
            g_FieldScriptVMCurActor->scriptInstructionPointer = offset;
            if (SCRIPT_READ_U8(offset) == 0) {
                g_FieldScriptVMCurActor->flags |= 0x4000000;
            }

            pFieldActor = &(*ppFieldActors)[i];
            g_FieldCurActorIndex = i;
            g_FieldScriptVMCurFieldActor = pFieldActor;
            g_FieldScriptVMCurActor = pFieldActor->pActorData;
            g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptGetBytecodeOffset(i, 0);
        }

        for (i = 0; i < g_FieldActiveActorCount; i++) {
            pFieldActor = &(*ppFieldActors)[i];
            g_FieldCurActorIndex = i;
            g_FieldScriptSpritesBuilt = 0;
            g_FieldScriptVMScriptEndFlag = 0;
            g_FieldScriptVMCurFieldActor = pFieldActor;
            g_FieldScriptVMCurActor = pFieldActor->pActorData;
            FieldScriptVMRun(0xFFFF);

            // The script chose no sprite of its own, so the actor takes the blob's first one.
            if (g_FieldScriptSpritesBuilt == 0) {
                pSpriteDataBlob = g_FieldSpriteData;
                func_80076AC0(i, 0, (void*)(pSpriteDataBlob->offsets[0] + (s32)pSpriteDataBlob), 0, 0, 0x80, 0);
                g_FieldScriptVMCurActor->flags |= 0x800;
            }
        }
    }
}*/
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

extern u16 g_BattleClock;
extern u_char g_GameDiscChangeRequest;
extern s32 g_GameRequestedWdsBankIndex;
extern s32 g_GameSceneMapNum;
extern int FieldGetPlayerActorDirection(void);
extern int FieldGetCameraDirection(void);
void FieldScriptSyncMemoryWithGameState(void) {
    int i;

    g_pGameState->mapNum = g_GameSceneMapNum;
    g_pGameState->wdsBankIndex = g_GameRequestedWdsBankIndex;
    g_pGameState->nextMapFlags = g_pGameState->fieldScriptMemory[1];
    g_pGameState->entryAngle = g_pGameState->fieldScriptMemory[4] << 9;
    FieldScriptMemoryWriteU16(0x44, g_BattleClock);
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

extern u8 g_WorldMapSaveBlock[];
extern u8* g_WorldMapSaveCursor;
extern s32 g_FieldActiveActorCount;
extern s32 g_SavedGearRide[3];
extern u_int* g_FieldWalkmeshMaterialFlags;
extern VECTOR g_CameraEye;
extern u8 g_FieldScreenEffectData[];
extern char D_8006FD9C[]; // "SAVESIZE=%d %x\n"
extern void SpriteSaveAnimState(SpriteData* pSpriteData, void* pState);

// Pulls the field state that func_800A3F4C pushed into the save block back out, one memcpy per block,
// then each actor's rotation/status/ActorData and the script memory.
//
// Three scheduler artifacts are load-bearing and cannot be read as intent: the pScriptWorkWords
// restore is a volatile store (without it sched2 hoists the next statement's volatile
// `lw g_FieldActors` above it, which costs the function one instruction and the link), the two
// HeapAlloc results keep their `do {} while (0)` blocks, and taking `pFieldActor` once per iteration
// is what makes the rotation copy's destination address base-first, as retail has it.
void func_800A3474(void) {
    int i;
    void* pScriptWorkWords;
    ActorData* pActorData;
    void** pScriptMemory;
    FieldActor* pFieldActor;
    int nStatus;

    g_WorldMapSaveCursor = g_WorldMapSaveBlock;
    g_FieldNumActors = g_WorldMapSaveBlock[0];
    g_WorldMapSaveCursor += 4;
    memcpy(g_FieldScreenEffectData, g_WorldMapSaveCursor, 0x38);

    g_WorldMapSaveCursor += 0x38;
    memcpy((u8*)&g_Scene + 0xC4, g_WorldMapSaveCursor, 0x74);

    g_WorldMapSaveCursor += 0x74;
    memcpy(g_FieldWalkmeshMaterialFlags, g_WorldMapSaveCursor, 0x400);

    g_WorldMapSaveCursor += 0x400;
    memcpy(&g_FieldEffects, g_WorldMapSaveCursor, 0x2E4);

    g_WorldMapSaveCursor += 0x2E4;
    memcpy(&g_CameraEye, g_WorldMapSaveCursor, 0x1C8);

    g_WorldMapSaveCursor += 0x1C8;

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pFieldActor = &g_FieldActors[i];
        memcpy(&pFieldActor->rotation, g_WorldMapSaveCursor, 8);
        g_WorldMapSaveCursor += 8;
        memcpy(&nStatus, g_WorldMapSaveCursor, 4);
        g_FieldActors[i].status = nStatus;

        pActorData = g_FieldActors[i].pActorData;
        pScriptWorkWords = pActorData->pScriptWorkWords;
        g_WorldMapSaveCursor += 0x34;
        memcpy(pActorData, g_WorldMapSaveCursor, sizeof(ActorData));
        ((ActorData volatile*)g_FieldActors[i].pActorData)->pScriptWorkWords = pScriptWorkWords;
        g_WorldMapSaveCursor += 0x138;

        if (g_FieldActors[i].pActorData->flags134 & 0x80) {
            do {
                g_FieldActors[i].pActorData->pTargetTrack = HeapAlloc(0xC, 0);
            } while (0);
            memcpy(g_FieldActors[i].pActorData->pTargetTrack, g_WorldMapSaveCursor, 0xC);
            g_WorldMapSaveCursor += 0xC;
        }
        if (g_FieldActors[i].pActorData->flags12C_0xD) {
            do {
                g_FieldActors[i].pActorData->pScriptArgs = HeapAlloc(0x10, 0);
            } while (0);
            memcpy(g_FieldActors[i].pActorData->pScriptArgs, g_WorldMapSaveCursor, 0x10);
            g_WorldMapSaveCursor += 0x10;
        }
    }

    pScriptMemory = &g_FieldScriptMemory;
    memcpy(pScriptMemory, g_WorldMapSaveCursor, 0x800);
    g_WorldMapSaveCursor += 0x800;
}



// Pulls the field state func_800A3F4C pushed into the save block back out.
// PARKED 97.01% (700 bytes, 175 insns against retail's 176 / 704 bytes).
// The head's STATEMENT ORDER is what the store elimination needs:
//     g_WorldMapSaveCursor = pSrc;
//     g_FieldNumActors     = pSrc[0];   /* or *pNumActors, same code */
//     g_WorldMapSaveCursor = pSrc + 0x3C;
//     memcpy((u8*)&g_FieldNumActors - 0xB8, pSrc + 0x3C, 0x74);
//     g_WorldMapSaveCursor += 0x920;    /* AFTER the memcpy, BEFORE the gear loop */
// The g_FieldNumActors store between the two cursor stores is an alias barrier, so
// both cursor stores survive (the old head, where the second store directly followed
// the first, lost the first one: 91.47%, 688 bytes). The 0x920 cursor update must come
// after the inlined memcpy for the same reason - its stores kill the known cursor
// value, so retail's load+addiu+store reappears instead of a fold into pSrc+0x95C -
// and it must come BEFORE the 3-entry gear loop, which is where retail emits it.
// 86.75 -> 88.71 -> 91.47 -> 97.01.
// Load-bearing spellings measured earlier:
//  - the actor loop must go through an element-pointer local that is re-assigned at the
//    top of each of its three blocks: retail re-reads the volatile g_FieldActors once per
//    block and reuses that one element pointer for every field in the block. Spelling
//    g_FieldActors[i].pSpriteData in the SpriteRestoreAnimState call instead costs 4 insns.
//  - the 0x12C flag is bit 12, i.e. the bitfield flags12C_0xD (retail's andi 0x1000): it
//    gates the same 0x10-byte pScriptArgs chunk that func_800A3F4C writes.
//  - `g_WorldMapSaveCursor += 0x920;` must sit after a memory store (here *pNumActors).
//    Placed directly after the `= pSrc + 0x3C;` store, gcc folds the two into one store
//    of pSrc+0x95C and the second store and its reload both disappear.
// What is left (all three are the same shape in the memcpy expansion):
//  1. retail emits `addiu t0,a1,172` (the block-move end pointer) once per alignment
//     path, 176 insns total; this body emits it once, shared, in the beqz delay slot,
//     hence 175 insns / 700 bytes. THE BODY IS ONE INSTRUCTION SHORT AND CANNOT LINK.
//     Tried without change: nothing in C reaches it; the size, the source expression
//     and the register holding the base are identical on both sides.
//  2. retail materialises &g_FieldNumActors into $a0, stores through it (sw v1,0(a0))
//     and derives the copy destination with addiu a3,a0,-0xB8. gcc 2.7.2 folds both
//     uses into a %lo addend (lui a3,%hi; addiu a3,-184) plus the assembler's $at store
//     - 2 insns for the store where retail spends 1 (its store lands in the beqz delay
//     slot, which is where this body's shared addiu t0 goes). Tried without change: the
//     pNumActors pointer local, the inline (u8*)&g_FieldNumActors - 0xB8 form, and the
//     store placed before/after/between the cursor stores.
//  3. allocator only, same instruction count: block 1 keeps the element pointer in $v0
//     and the cursor in $a1 where this compile uses $a1/$a2, and block 2's pActorData
//     lands in $a1 here against $a0 there - a one-register shift.
// Presets measured on this body: gcc-2.6.0 94.70, gcc-2.7.2-cdk 85.70, cdk -G0 73.59
// (measured on the 91.47 body), -G0 compile-fails, -O3 compile-fails.
/*
void func_800A3C8C(void) {
    int i;
    int nDiff;
    ActorData* pActorData;
    FieldActor* pFieldActor;
    int* pNumActors;
    u8* pCursor;
    u8* pSrc;

    pSrc = g_WorldMapSaveBlock;
    pNumActors = &g_FieldNumActors;
    g_WorldMapSaveCursor = pSrc;
    *pNumActors = pSrc[0];
    g_WorldMapSaveCursor = pSrc + 0x3C;
    memcpy((u8*)pNumActors - 0xB8, pSrc + 0x3C, 0x74);
    g_WorldMapSaveCursor += 0x920;

    nDiff = 0;
    for (i = 0; i < 3; i++) {
        if (g_SavedGearRide[i] != g_pGameState->gearRide[i]) {
            nDiff++;
        }
    }

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pFieldActor = (FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors);
        pActorData = pFieldActor->pActorData;
        pCursor = g_WorldMapSaveCursor;
        g_WorldMapSaveCursor = pCursor + 0xC;
        if (pActorData->specialAnimArchiveIndex != -1 && pActorData->animationOverride != 0xFF) {
            *(u_short*)(pCursor + 0x20) = pActorData->animationOverride;
        }

        pFieldActor = (FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors);
        pActorData = pFieldActor->pActorData;
        if (!(pActorData->flags & 0x1000000) &&
            (g_FieldSceneReloadFlags == 0 || !(pActorData->scriptFlags.flags & 0x600) || nDiff == 0)) {
            SpriteRestoreAnimState(pFieldActor->pSpriteData, g_WorldMapSaveCursor);
        }

        pFieldActor = (FieldActor*)(i * sizeof(FieldActor) + (s32)g_FieldActors);
        pActorData = pFieldActor->pActorData;
        pCursor = g_WorldMapSaveCursor;
        g_WorldMapSaveCursor = pCursor + 0x168;
        if (pActorData->flags134 & 0x80) {
            g_WorldMapSaveCursor = pCursor + 0x174;
        }
        if (pFieldActor->pActorData->flags12C_0xD) {
            g_WorldMapSaveCursor += 0x10;
        }
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3C8C);



// Pushes the field state the world map is about to overwrite into the save block,
// one memcpy per block, then the actors' animation state and the script memory.
// func_800A3C8C pulls it back.
// PARKED 99.94% at retail's exact 2044 bytes (511 insns both sides, 8 differing lines).
// The cursor advances must be spelled as a COMPOUND ASSIGNMENT ON THE GLOBAL, with the
// global as the memcpy argument:
//     g_WorldMapSaveCursor += N;
//     memcpy(g_WorldMapSaveCursor, src, N);
// at the four state-chunk sites AND the actor loop's +0x30 site, and the head as
//     g_WorldMapSaveCursor = g_WorldMapSaveBlock;
//     g_WorldMapSaveBlock[0] = g_FieldNumActors;
//     g_WorldMapSaveCursor += 4;
//     memcpy(g_WorldMapSaveCursor, g_FieldScreenEffectData, 0x38);
// That is what makes the sum reuse the loaded pointer's $2 (retail's addiu v0,v0,N);
// the `pDst = g_WorldMapSaveCursor + N; g_WorldMapSaveCursor = pDst;` copy spelling
// always takes a fresh $3 and parks at 99.26 (the a-memcpy-dst-sum-tie lever calls that
// unreachable - the += is a different RTL and wins it). 99.26 -> 99.94%.
// Left, two ties only:
//  1. the script-memory copy at the tail prints `or v0,a2,a3` where retail has
//     `or v0,a3,a2` - same registers and same roles (a3 = cursor/dst, a2 = src), only
//     the commutative IOR operand order.
//  2. the tail's `&g_WorldMapSaveBlock` lands in $v1; retail keeps it in $s1
//     (lui s1 / addiu s1 / subu a1,a1,s1). Round 421 measured why this is a CLASS
//     difference rather than a spelling: $s1 is callee-saved, and local_alloc hands out
//     REG_ALLOC_ORDER's caller-saved registers first to a value that lives in one block,
//     so retail's address was a global allocno live across a call. The one spelling that
//     makes it one - pSaveBlock assigned at the head and used again at the tail -
//     measures 99.04% / 2044 bytes with the frame at 0x34: it buys $s2 and an extra
//     save/restore instead of reusing the $s1 the actor and gear index already saves,
//     i.e. the free reuse retail shows is not reachable by sharing one variable.
// Round 421 re-measured this body: 99.94% / 2044 bytes / 511 insns, unchanged, and the
// permuter from it is again base=30 best=30 (1375 iterations, 3 min). Also rejected this
// round: a pSaveBlock local for the tail subtraction alone (2044, identical code), and
// do { } while (0) around the tail (2044, identical code).
// Measured and rejected earlier: nSaveSize passed to the SAVESIZE printf twice
// (2048 bytes), &g_FieldScriptMemory inlined at the copy (2060), pScriptMemory moved
// down to its use (2060), u8* pScriptMemory (no change), pActorData hoisted above the
// 0x30 cursor update (2052), pDst defined before the base store (2040), embedded cursor
// store (2040), a pSave local for the head only, cursor store moved after the memcpy
// (2024), gcc-2.6.0 (98.88), gcc-2.7.2-cdk (compile fails). Seven earlier permuter runs
// (3-12 min) empty, six of them from the 99.26 body.
/*
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
    g_WorldMapSaveBlock[0] = g_FieldNumActors;
    g_WorldMapSaveCursor += 4;
    memcpy(g_WorldMapSaveCursor, g_FieldScreenEffectData, 0x38);

    pScriptMemory = &g_FieldScriptMemory;
    g_WorldMapSaveCursor += 0x38;
    memcpy(g_WorldMapSaveCursor, (u8*)&g_Scene + 0xC4, 0x74);

    g_WorldMapSaveCursor += 0x74;
    memcpy(g_WorldMapSaveCursor, g_FieldWalkmeshMaterialFlags, 0x400);

    g_WorldMapSaveCursor += 0x400;
    memcpy(g_WorldMapSaveCursor, &g_FieldEffects, 0x2E4);

    g_WorldMapSaveCursor += 0x2E4;
    memcpy(g_WorldMapSaveCursor, &g_CameraEye, 0x1C8);

    g_WorldMapSaveCursor += 0x1C8;

    for (i = 0; i < g_FieldActiveActorCount; i++) {
        pRot = &g_FieldActors[i].rotation;
        memcpy(g_WorldMapSaveCursor, pRot, 8);
        nStatus = (u_short)g_FieldActors[i].status;
        pDst = (g_WorldMapSaveCursor = g_WorldMapSaveCursor + 8);
        memcpy(pDst, &nStatus, 4);

        pDst = (g_WorldMapSaveCursor = g_WorldMapSaveCursor + 4);
        SpriteSaveAnimState((g_FieldActors + i)->pSpriteData, pDst);

        g_WorldMapSaveCursor += 0x30;
        memcpy(g_WorldMapSaveCursor, g_FieldActors[i].pActorData, sizeof(ActorData));

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
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3F4C);

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