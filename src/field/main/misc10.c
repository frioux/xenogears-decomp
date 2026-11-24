#include "common.h"
#include "main/game.h"
#include "field/actor.h"
#include "field/script_vm.h"

void FieldScriptWriteInventoryItemQuantity(void) {
    int nItemId;
    int nIndex;
    u_char* pQuantities;

    nItemId = FieldScriptVMGetArgument(1);
    nIndex = FieldInventoryFindSlotById(nItemId);
    pQuantities = (u_char*)FieldInventoryGetQuantitiesArray(nItemId);
    FieldInventoryGetIdsArray(nItemId);
    if (nIndex != -1) {
        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, pQuantities[nIndex]);
    } else {
        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, 0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void FieldScriptJumpIfInventoryHasItem(void) {
    if (FieldInventoryFindSlotById(FieldScriptVMGetArgument(1)) != -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(3);
    }
}

void FieldScriptAddInventoryItem(void) {
    func_8009635C(FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}


// PARKED 96.70%, 176 bytes / 44 insns (retail is 176 / 44 too). Registers are retail exactly: nItemId $s1, nIndex $s0, pIds $s2, pQuantities $s4, the constant -1 $s3, pointer $v1, loaded byte $a0, value $v0. The pQuantity pointer local is what holds the $s0/$s1 assignment; without it the same statements score 96.70 with $s0/$s1 mirrored.
// Residual (2 things, both in the tail): (a) retail has ONE quantity store in a block reached by arm 1 j and by the else arm fall-through, with addiu v0,a0,1 in the j slot and a nop in the beqz slot; ours duplicates the sb into arm 1 and jumps PAST the shared block to the epilogue. (b) the else arm computes the ids address before the quantities address, retail the reverse.
// This body is the per-arm spelling, which lever h-a-join-store-of-arms-that-store-different-values says is structurally WRONG: retail stores different values in the two arms, so the join store means the original had a value temp and ONE store below the if/else.
// Measured this round on the honest shape (u_char nQuantity set in each arm, one pQuantities[nIndex] = nQuantity below the chain, else return): 90.41 with the allocator mirroring $s0/$s1; adding the pQuantity pointer local to arm 1 (and pQuantity in the else arm) restores retail registers exactly and gives 92.16 with the same duplicated tail; computing pQuantity before the ids store in the else arm gives 95.91 but the allocator keeps the quantity address in $a0.
// Also measured earlier: both arms storing pQuantity[0] with a shared nQuantity (86.70); both arms pQuantity[0] of an expression (96.59); two pQuantities[nIndex] stores (96.70); pointer before/after the ids store (byte-identical). Presets: gcc-2.6.0 96.93, 2.7.2-cdk 96.70, -O3 does not compile.
// UNTRIED LEAD: a label spelling. Retail j target has two predecessors (j plus fall-through), which is a goto-label tail rather than a cross-jump merge; lever c-a-switch-shared-tail-sits-at-a-label says that shape is source-level. Spell it as nQuantity = *pQuantity + 1; goto store; ... store: *pQuantities = nQuantity; - one diff run, not yet measured.
/*
void func_8009635C(u_int nItemId) {
    u_int nIndex;
    u_char* pQuantities;
    u_char* pIds;
    u_char* pQuantity;

    nIndex = FieldInventoryFindSlotById(nItemId);
    pQuantities = (u_char*)FieldInventoryGetQuantitiesArray(nItemId);
    pIds = (u_char*)FieldInventoryGetIdsArray(nItemId);
    if (nIndex != -1) {
        pQuantity = &pQuantities[nIndex];
        if (*pQuantity < 99) {
            *pQuantity = *pQuantity + 1;
        }
    } else {
        nIndex = FieldInventoryFindEmptySlotById(nItemId);
        if (nIndex != -1) {
            pIds[nIndex] = nItemId;
            pQuantities[nIndex] = 1;
        }
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/main/misc10", func_8009635C);




void FieldScriptRemoveInventoryItem(void) {
    int nItemId;
    int nIndex;
    u_char* pItems;
    u_char* pQuantities;

    nItemId = FieldScriptVMGetArgument(1);
    nIndex = FieldInventoryFindSlotById(nItemId);
    if (nIndex != -1) {
        pItems = (u_char*)FieldInventoryGetIdsArray(nItemId);
        pQuantities = (u_char*)FieldInventoryGetQuantitiesArray(nItemId);
        if (--pQuantities[nIndex] == 0) {
            pItems[nIndex] = 0xFF;
        }
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

// Check if certain member is in current party
void FieldScriptCheckPartyMember(void) {
    int i;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (SCRIPT_READ_U8_REL(1) == g_GamePartyMembers[i]) {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
            return;
        }        
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
}

void FieldScriptJumpIfCharacterBit1D30(void) {
    if ((g_pGameState->characterFlags >> SCRIPT_READ_U8_REL(1)) & 1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    }
}

void FieldScriptSetCharacterBit1D30(void) {
    g_pGameState->characterFlags |= 1 << SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void FieldScriptClearCharacterBit1D30(void) {
    g_pGameState->characterFlags &= ~(1 << SCRIPT_READ_U8_REL(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}
