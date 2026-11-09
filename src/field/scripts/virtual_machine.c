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

extern s32 D_800AFFEC;
extern s32 D_800AFD1C; // Current actor index
extern s32 D_800B00C0;

extern void func_800379C8(char*, ...);
extern char D_8006FD44; // "STACKERR ACT=%d\n"

// Store instruction pointer + 5 on stack
void func_800A1730(void) {
    if (g_FieldScriptVMCurActor->flags12C_0x6 != SCRIPT_MAX_STACK_SIZE) {
        g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6] = g_FieldScriptVMCurActor->scriptInstructionPointer + 5;
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(1);
        g_FieldScriptVMCurActor->flags12C_0x6++;
        return;
    }

    // Error
    if (g_FieldSystemMode == 0) {
        func_800379C8(&D_8006FD44, D_800AFD1C);
    }
    D_800B00C0 = 1;
}

// Store instruction pointer + 3 on stack
void func_800A17F4(void) {
    if (g_FieldScriptVMCurActor->flags12C_0x6 != SCRIPT_MAX_STACK_SIZE) {
        g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6] = g_FieldScriptVMCurActor->scriptInstructionPointer + 3;
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(1);
        g_FieldScriptVMCurActor->flags12C_0x6++;
        return;
    }
    
    // Error
    if (g_FieldSystemMode == 0) {
        func_800379C8(&D_8006FD44, D_800AFD1C);
    }
    D_800B00C0 = 1;
}

// Restore instruction pointer from stack
void func_800A18B8(void) {
    // Error, invalid stack value
    if (!g_FieldScriptVMCurActor->flags12C_0x6) {
        if (g_FieldSystemMode == 0) {
            func_800379C8(&D_8006FD44, D_800AFD1C);
        }
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 = 0xFF;
        g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].scriptId = 0xFF;
        D_800AFFEC = 1;
        D_800B00C0 = 1;
        return;
    }
    
    g_FieldScriptVMCurActor->flags12C_0x6--;
    g_FieldScriptVMCurActor->scriptInstructionPointer = g_FieldScriptVMCurActor->scriptPointersStack[g_FieldScriptVMCurActor->flags12C_0x6];
}

void func_800A19B0(void) {
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
    g_FieldScriptVMCurActor->unkCF = 0;
    g_FieldScriptVMCurActor->dialogFlags = 0;
    D_800B00C0 = 1;
    g_FieldScriptVMCurActor->flags12C_0x6 = 0;
}

// Yield / Stop, but change IP conditionally
void func_800A1A8C(void) {
    int i;

    for (i = 0; i < ACTOR_MAX_NUM_SCRIPTS; i++) {
        if (g_FieldScriptVMCurActor->scripts[i].flags_0x12 == 7) {
            g_FieldScriptVMCurActor->scripts[i].currentIP = FieldScriptGetBytecodeOffset(D_800AFD1C, 1);
        }
    }

    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 = 0xFF;
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].scriptId = 0xFF;
    D_800B00C0 = 1;
}

// Yield / Stop Handler
void func_800A1B70(void) {
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 = 0xFF;
    g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].scriptId = 0xFF;
    D_800AFFEC = 1;
    D_800B00C0 = 1;
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
    nArgumentsType = ((u_char*)g_FieldScriptVMCurScriptData)[g_FieldScriptVMCurActor->scriptInstructionPointer + 5] & 0xF0;
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
    nConditionType = ((u_char*)g_FieldScriptVMCurScriptData)[g_FieldScriptVMCurActor->scriptInstructionPointer + 5] & 0xF;
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

void func_800A1E9C(void) {
    g_FieldScriptMaxInstructionCount += 0x20;
    g_FieldScriptVMCurActor->scriptInstructionPointer++;
}

void FieldScriptVMRun(int maxInstructionCount) {
    int nInstructionCount;
    u_short nInstructionPointer;
    u_char nHandlerIndex;

    D_800B00C0 = 0;
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
                func_800379C8(D_8006FD84, D_800AFD1C); // Error printing
            }
            return;
        }
        nInstructionPointer = g_FieldScriptVMCurActor->scriptInstructionPointer;
        nHandlerIndex = ((u_char *)g_FieldScriptVMCurScriptData)[nInstructionPointer];
        g_FieldScriptVMHandlers[nHandlerIndex]();

        if (D_800AFFEC == 0) {
            g_FieldScriptMaxInstructionCount = 0xFFFF;
        }

        // Various exit checks
        if ((D_800ADB1C != 0 && (D_800ADBE0 == 0 ||  D_800ADBE4 == 0 ||  D_800ADBEC == 0))) {
            return;
        }

        if (D_800B00C0 == 1 && D_800AFFEC == D_800B00C0) {
            return;
        }

    }

}

// Best attempt: 90.49% (636 bytes, same size as the original: only register assignment differs).
// - nMinPriority = 0xF must be assigned BEFORE the D_800B21CC check; the original keeps the
//   `li a2,0xF` in the delay slot of that beqz.
// - the "no free script slot" branch is taken when nMinPriority == 0xF, not !=.
// - the flags read-modify-write belongs BEFORE the D_800B06B8 / D_800AFD1C / g_FieldScriptVMCurActor
//   stores, and the guard clauses above it must spell pFieldActor->pActorData->flags inline.
// Left: the original reloads pActorData from 0x4C($a1) in the join block and keeps pFieldActor in
// $a1 / pActorData in $a0; gcc here CSEs the 0x4C load across the two-pred join and lands on
// `addu $a0,$v0,$s1` with the roles swapped, which is 100% of what is left. Tried without change:
// explicit (u_char*)g_FieldActors + i*sizeof(FieldActor) byte arithmetic; a local pActor assigned
// twice; -G0/-O3 and gcc-2.7.2-cdk presets. Round 36 also: all 7 locals in 12 different declaration
// orders (90.49% every time) and gcc-2.6.0-psx (89.86%), so the temp-register split is a
// global.c allocno tie, not a source shape.
/*
extern s32 D_800ADBFC;
extern s32 D_800ADB1C;
extern s32 D_800ADB74;
extern FieldActor* D_800B06B8;
extern s32 D_800ADB68;
extern s32 D_800C4268;
extern u_char D_800B21CC;
extern s32 D_8005A444[3];

// Runs the field script VM for every actor that has a script pending
void func_800A2030(void) {
    int i;
    int j;
    int nActorCount;
    int nMinPriority;
    u_short nInstructionPointer;
    FieldActor* pFieldActor;
    ActorData* pActor;

    if (D_800ADB74 == 1) {
        nActorCount = 1;
    } else {
        nActorCount = D_800ADBFC;
    }
    D_800ADB68 = 0;
    D_800C4268 = 0;

    for (i = 0; i < nActorCount; i++) {
        pFieldActor = &g_FieldActors[i];
        if (!(pFieldActor->status & 0xF00)) {
            continue;
        }

        if (pFieldActor->pActorData->flags & 0x100000) {
            continue;
        }

        if (D_800ADB1C != 0 && (D_800ADBE0 == 0 || D_800ADBE4 == 0 || D_800ADBEC == 0)) {
            return;
        }

        pActor = pFieldActor->pActorData;
        pActor->scriptFlags.flags &= ~0x1000000;

        D_800B06B8 = pFieldActor;
        D_800AFD1C = i;
        g_FieldScriptVMCurActor = pActor;

        // Pick the script slot with the lowest priority
        nMinPriority = 0xF;
        if (D_800B21CC != 0) {
            for (j = 0; j < 3; j++) {
                if (D_8005A444[j] != ACTOR_ID_INVALID && D_8005A444[j] == i) {
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
        D_800AFFEC = 1;
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
extern FieldActor* D_800B06B8;
extern s32 D_800ADB1C;
void func_800A22AC(int scriptRoutineIndex) {
    int i;
    ActorData* pNewActor;

    // Change current actor to the first actor in the list
    D_800B06B8 = g_FieldActors;
    g_FieldScriptVMCurActor = D_800B06B8->pActorData;
    pNewActor = HeapAlloc(0x138, 0x1);
    *pNewActor = *D_800B06B8->pActorData;

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
    
    D_800AFD1C = 0;
    D_800ADB1C = 0;
    D_800AFFEC = 0;

    // Run entry point routine
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptGetBytecodeOffset(0, scriptRoutineIndex);
    FieldScriptVMRun(0xFFFF);
    D_800ADB1C = 1;

    // Running VM bytecode will likely have changed some fields, so they are copied back into the top
    // of the actor array.
    *D_800B06B8->pActorData = *pNewActor;
    HeapFree(pNewActor);
}


extern s32 D_800ADB8C;
extern void func_800ACE24();
void func_800A2488(void) {
    D_800ADB8C = 1;
    func_800A22AC(3);
    func_800ACE24();
    D_800ADB8C = 0;
}

// Best attempt: 57.73% (468 bytes; frame 0x30 vs 0x40, one fewer saved register). The control
// flow is right - the count-guarded table loop, the player position.vy -= 8, and the
// "if (i < D_800ADBFC) { if (flags12C_0) continue; } else if (b == 0) deltas" /
// "if (b == 1) deltas" pair reproduce the original block layout exactly. Two walls remain, both
// about addresses:
//  1. the original materialises ONE base register (a2 = &D_800B21D2, hoisted before the loop) and
//     reads the three deltas with reloc-free negative offsets (lw v1,-0x16(a2), -0x12, -0xE), so
//     its source reached D_800B21BC/C0/C4 relative to the byte symbol; this C names the three
//     globals, so gcc hoists them into t1/t0/a3 with their own %hi/%lo pairs.
//  2. the original reloads g_FieldActors on every use inside the loop body
//     (lui a0,%hi(g_FieldActors); lw a0,0(a0); addu a0,a1,a0 - loop offset first), while gcc here
//     CSEs that load and keeps it in a0, which also loses the addu operand order.
// The D_800B2264 table/count reach-one-base idiom in the first loop is the same wall documented in
// main/main.c func_80077884 (90.25%): fold_rtx folds the -0x80/+0x80 pair back to the symbol.
/*
extern s32 D_800B2264;
extern s32 D_800ADBFC;
extern s32 g_PlayerActorIndex;
extern s32 g_GamePartySkinsInitialized;
extern s32 D_800B21BC;
extern s32 D_800B21C0;
extern s32 D_800B21C4;
extern u_char D_800B21D2;
extern void func_800AD898(void);

// Applies the camera shake delta from the script globals to every actor's translation
void func_800A24C4(void) {
    int i;
    ActorData* pActorData;
    FieldActor* pFieldActor;

    if (!g_GamePartySkinsInitialized) {
        return;
    }

    func_800A22AC(2);
    func_800AD898();

    for (i = 0; i < D_800B2264; i++) {
        func_801E8330(i, 0, ((s16*)((u8*)&D_800B2264 - 0x80))[i]);
    }

    pActorData = g_FieldActors[g_PlayerActorIndex].pActorData;
    if (pActorData->canInteract != 0xFF) {
        pActorData->position.vy -= 8;
    }

    for (i = 0; i < g_FieldNumActors; i++) {
        pFieldActor = &g_FieldActors[i];
        if (i < D_800ADBFC) {
            if (pFieldActor->pActorData->flags12C_0) {
                continue;
            }
        } else if ((D_800B21D2 & 0x7F) == 0) {
            pFieldActor->transformMatrix.t[0] += D_800B21BC;
            pFieldActor->transformMatrix.t[1] += D_800B21C0;
            pFieldActor->transformMatrix.t[2] += D_800B21C4;
        }

        if ((D_800B21D2 & 0x7F) == 1) {
            pFieldActor->transformMatrix.t[0] += D_800B21BC;
            pFieldActor->transformMatrix.t[1] += D_800B21C0;
            pFieldActor->transformMatrix.t[2] += D_800B21C4;
        }
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A24C4);

extern s32 g_GamePartySkinsInitialized;
extern s32 D_800ADBFC;
void func_800A2714(void) {
    ActorData* pActor;
    FieldActor* pFieldActors;
    int i;
    void* pData;

    if (g_GamePartySkinsInitialized) {
        // Read animation files
        for (i = 0; i < D_800ADBFC; i++) {
            ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0x0);
            pActor = g_FieldActors[i].pActorData;
            if (pActor->unk124 != -1) {
                g_FieldScriptVMCurActor = pActor;
                pData = HeapAlloc(ArchiveDecodeAlignedSize(pActor->unk124, pActor) + 8, 0x0);
                g_FieldScriptVMCurActor->unk120 = pData;
                ArchiveReadFileToBuffer(g_FieldScriptVMCurActor->unk124, pData, 0, CdlModeSpeed); // Read from disc into buffer
                ArchiveCdDataSync(0);
            }
        }

        // Set Special animation file
        for (i = 0; i < D_800ADBFC; i++) {
            pFieldActors = g_FieldActors;
            pActor = pFieldActors[i].pActorData;
            if (pActor->unk124 != -1) {
                SpriteSetSpecialAnimFile(pFieldActors[i].pSpriteData, pActor->unk120);
            }
        }
        
        func_800A3C8C();
        
        if (g_FieldEffects.distortion.isActive) {
            FieldDistortionInitialize(1);
        }
        
        FieldScriptMemoryWriteU16(0x10, 0x0);
        FieldScriptWritePartyMemberIDs();

        // Apply rotation and scale
        for (i = 0; i < D_800ADBFC; i++) {
            func_80072254(i);
        }
    }
}

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

// Best attempt: 97.14% - the original shifts the index into a second register ($a2) before scaling
// it ($v0); gcc 2.7.2 always reuses $a0 for both shifts with every form tried. Also tried, all
// 97.14%: a local pointer with p[i] / *(p + (i >> 1)), an int param with a (u_short) cast on the
// value, and &g_FieldScriptMemory + (index >> 1). index / 2 gives 69.29% (36 bytes, rounding
// fixup) and a volatile cast 52.86%. A two-step `nOffset = index >> 1` plus a byte-offset add
// `(u_char*)&g_FieldScriptMemory + (nOffset << 1)` is also 97.14%. gcc-2.6.0, gcc-2.7.2-cdk and
// -O3 all give the same 97.14%. Only the shift's destination register differs from the original.
// Best attempt: 97.14% - the original shifts the index into a second register ($a2) before scaling
// it ($v0); gcc 2.7.2 always reuses $a0 for both shifts with every form tried. Also tried, all
// 97.14%: a local pointer with p[i] / *(p + (i >> 1)), an int param with a (u_short) cast on the
// value, and &g_FieldScriptMemory + (index >> 1). index / 2 gives 69.29% (36 bytes, rounding
// fixup) and a volatile cast 52.86%. A two-step `nOffset = index >> 1` plus a byte-offset add
// `(u_char*)&g_FieldScriptMemory + (nOffset << 1)` is also 97.14%. gcc-2.6.0, gcc-2.7.2-cdk and
// -O3 all give the same 97.14%. Only the shift's destination register differs from the original.
/*
void FieldScriptMemoryWriteU16(int index, u_short value) {
    ((u_short*)&g_FieldScriptMemory)[index >> 1] = value;
}
*/
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", FieldScriptMemoryWriteU16);

// scriptIndex here refers to the index of the script, which will (always?) correspond to an entity index
// routineIndex is an index into the offset table, which points to a bytecode routine in that script.
u_short FieldScriptGetBytecodeOffset(int scriptIndex, int routineIndex) {
    int nOffset;
    u_short* pScriptData;

    pScriptData = &g_FieldCurScriptFile->metadata;
    nOffset = (scriptIndex * (SCRIPT_OFFSET_TABLE_SIZE / sizeof(u_short)) + routineIndex);
    return *(pScriptData + nOffset);
}

void FieldScriptWritePartyMemberIDs(void) {
    FieldScriptMemoryWriteU16(0x3E, g_GamePartyMembers[0]);
    FieldScriptMemoryWriteU16(0x40, g_GamePartyMembers[1]);
    FieldScriptMemoryWriteU16(0x42, g_GamePartyMembers[2]);
}

extern u16 D_8005941C;
extern u_char D_800594D0;
extern s32 D_8004F324;
extern s32 g_GameSceneMapNum;
extern int FieldGetPlayerActorDirection(void);
extern int FieldGetCameraDirection(void);
void func_800A30FC(void) {
    int i;

    g_pGameState->unk231A = g_GameSceneMapNum;
    g_pGameState->unk2322 = D_8004F324;
    g_pGameState->unk2320 = g_pGameState->unk1930[1];
    g_pGameState->unk231C = g_pGameState->unk1930[4] << 9;
    FieldScriptMemoryWriteU16(0x44, D_8005941C);
    FieldScriptMemoryWriteU16(0x46, D_800594D0);
    FieldScriptMemoryWriteU16(6, FieldGetPlayerActorDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(8, FieldGetCameraDirection() & 0xFFFF);
    FieldScriptMemoryWriteU16(0x24, g_Scene.sceneDIP);
    FieldScriptMemoryWriteU16(0x3C, g_GameSceneMapNum);
    FieldScriptWritePartyMemberIDs();

    for (i = 0; i < 0x200; i++) {
        g_pGameState->unk1930[i] = ((u_short*)&g_FieldScriptMemory)[i];
    }
}

extern u16 D_800AFC6C;
extern u16 D_800AFE9C;
extern char D_80059418;
extern char D_80059420;
extern char D_80059484;
extern s32 g_PlayerActorIndex;
extern u8 D_800B02C8;
extern s32 D_8004F318;
extern s32 D_8004F328;
extern s32 D_8004F2F4;
extern void func_8009FEE4(int actorIndex);

// Refreshes the script memory from the field state, and every 0x1F calls walks the map position pair
void func_800A31E8(void) {
    int i;
    int nValue;
    int nLow;
    int nHigh;

    if (D_800B02C8 == 1) {
        return;
    }

    D_800AFC6C |= D_800AFE9C;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        g_pGameState->partyMembers[i] = g_GamePartyMembers[i];
    }

    func_800A30FC();
    D_8004F2F4 = 0;
    D_8004F318++;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_pGameState->gearRide[i] == 1) {
            func_8009FEE4(i);
        }
    }

    if (D_8004F318 >= 0x1F) {
        D_8004F318 = 0;

        // Step the map position pair in the direction the script asked for
        if (!(D_8004F328 & 0x80)) {
            nValue = FieldScriptVMGetVariableValue(0xA);
            nLow = nValue & 0xFF;
            nHigh = (nValue >> 8) & 0xFF;

            if (!(D_8004F328 & 4)) {
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

    FieldScriptMemoryWriteU16(0xC, D_80059418 | (D_80059420 << 8));
    FieldScriptMemoryWriteU16(0xE, D_80059484);

    FieldScriptMemoryWriteU16(0x1E, ((s16*)&g_FieldActors[g_PlayerActorIndex].pActorData->position)[1]);
    FieldScriptMemoryWriteU16(0x20, ((s16*)&g_FieldActors[g_PlayerActorIndex].pActorData->position)[5]);
    FieldScriptMemoryWriteU16(0x22, ((s16*)&g_FieldActors[g_PlayerActorIndex].pActorData->position)[3]);
}

INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3474);

// Best attempt: 86.75% (692 bytes vs 696). Everything structural already matches: the 0x74-byte
// memcpy through `(u8*)&g_FieldNumActors - 0xB8` reproduces the original's two-loop
// mips_expand_block_move verbatim (the (src|dst)&3 alignment test, the lwl/lwr loop, the lw loop and
// the 4-byte tail), and both loops plus the func_80021D50 guard chain match instruction for
// instruction. What is left is register roles and one missing reload:
//  1. the original keeps &g_FieldNumActors in $a0 and derives $a3 = $a0 - 0xB8, and keeps
//     &D_8005A4E4 in $a1 (addiu $v0,$a1,0x3C / move $a2,$v0); gcc here folds the -0xB8 into the
//     symbol (lui $a3 / addiu $a3,-184) and puts &D_8005A4E4 in a temp. Tried inline expressions and
//     an explicit pSrc local, no change.
//  2. the original reloads pActorData from the FieldActor in the last block (lw $v0,0x4C($a1)) and
//     tests flags12C with `andi $v0,$v0,0x1000`; gcc keeps pActorData in $a0 across the jal and
//     reports the flags12C read as a plain word test here (it needs the u_int cast to avoid a
//     srl/andi pair, that part is now right).
/*
extern u8 D_8005A4E4;
extern u8* D_800AFC50;
extern s32 D_8005A408[3];
extern s32 D_800ADBFC;
extern s32 D_800B2268;
extern void func_80021D50(void*, u8*);
void func_800A3C8C(void) {
    int i;
    int nDiff;
    ActorData* pActorData;
    u8* pCursor;
    u8* pSrc;

    pSrc = &D_8005A4E4;
    memcpy((u8*)&g_FieldNumActors - 0xB8, pSrc, 0x74);
    g_FieldNumActors = *pSrc;
    D_800AFC50 = pSrc;
    D_800AFC50 = &D_8005A4E4 + 0x3C;
    D_800AFC50 += 0x920;

    nDiff = 0;
    for (i = 0; i < 3; i++) {
        if (D_8005A408[i] != g_pGameState->gearRide[i]) {
            nDiff++;
        }
    }

    for (i = 0; i < D_800ADBFC; i++) {
        pActorData = g_FieldActors[i].pActorData;
        pCursor = D_800AFC50;
        D_800AFC50 = pCursor + 0xC;
        if (pActorData->unk124 != -1 && pActorData->unkAnimationId != 0xFF) {
            *(u_short*)(pCursor + 0x20) = pActorData->unkAnimationId;
        }

        pActorData = g_FieldActors[i].pActorData;
        if (!(pActorData->flags & 0x1000000) &&
            (D_800B2268 == 0 || !(pActorData->scriptFlags.flags & 0x600) || nDiff == 0)) {
            func_80021D50(g_FieldActors[i].pSpriteData, D_800AFC50);
        }

        pActorData = g_FieldActors[i].pActorData;
        pCursor = D_800AFC50;
        D_800AFC50 = pCursor + 0x168;
        if (pActorData->flags134 & 0x80) {
            D_800AFC50 = pCursor + 0x174;
        }
        if (*(u_int*)((u_char*)pActorData + 0x12C) & 0x1000) {
            D_800AFC50 += 0x10;
        }
    }
}
*/
INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3C8C);

INCLUDE_ASM("asm/field/nonmatchings/scripts/virtual_machine", func_800A3F4C);

void func_800A4748(void) {
    func_800A476C(0x2c0,0x100);
}

void func_800A476C(int x, int y) {
    RECT rect;

    rect.w = 0x140;
    rect.y = 0;
    rect.x = 0;
    rect.h = 0xe0;
    SetGeomScreen(0x200);
    MoveImage(&rect, x, y);
    FieldRenderSync();
}