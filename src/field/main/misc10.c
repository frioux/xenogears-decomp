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

INCLUDE_ASM("asm/field/nonmatchings/main/misc10", func_8009635C);

/* Attempt A, 96.70%, 176 bytes / 44 insns (the original is 176 bytes / 44
   insns too). Two store statements, one per arm: gcc does not cross-jump them
   and instead fills the `j` delay slot with path 1's `sb`, so the shared tail
   store the original has is lost.
void func_8009635C(int nItemId) {
    int nIndex;
    u_char* pQuantities;
    u_char* pItems;

    nIndex = FieldInventoryFindSlotById(nItemId);
    pQuantities = (u_char*)FieldInventoryGetQuantitiesArray(nItemId);
    pItems = (u_char*)FieldInventoryGetIdsArray(nItemId);
    if (nIndex != -1) {
        if (pQuantities[nIndex] < 0x63) {
            pQuantities[nIndex] = pQuantities[nIndex] + 1;
        }
    } else {
        nIndex = FieldInventoryFindEmptySlotById(nItemId);
        if (nIndex != -1) {
            pItems[nIndex] = nItemId;
            pQuantities[nIndex] = 1;
        }
    }
}
*/

/* Attempt B, 90.41%, 176 bytes / 44 insns: ONE store statement at the end, so
   the block layout and instruction count are the original's exactly (path 1
   jumps to the shared `sb`, path 2 falls into it). The whole residual is
   register allocation: the original keeps nItemId in $s1 and nIndex in $s0,
   the loaded byte in $a0, the stored value in $v0 and the `pQuantities +
   nIndex` address in $v1 reused across the branch; this body mirrors $s0/$s1,
   keeps byte and value in $v1 with an in-place `addiu` and re-forms the
   address in the `j` delay slot. Measured and rejected on top of it: int and
   u_int parameter (identical code), u_int nIndex (identical), u_char and int
   nQuantity (identical), `nQuantity = pQuantities[nIndex]` then
   `if (nQuantity < 99) nQuantity++` (gives slti not sltiu, 89.16%), and a
   4-minute permuter run (2729 iterations, base 580 best 280) whose only winner
   dropped `nQuantity = 1` and stored a literal 1 in the shared block, i.e.
   wrong code.
void func_8009635C(u_int nItemId) {
    u_int nIndex;
    int nQuantity;
    u_char* pQuantities;
    u_char* pIds;

    nIndex = FieldInventoryFindSlotById(nItemId);
    pQuantities = (u_char*)FieldInventoryGetQuantitiesArray(nItemId);
    pIds = (u_char*)FieldInventoryGetIdsArray(nItemId);
    if (nIndex != -1) {
        if (pQuantities[nIndex] < 99) {
            nQuantity = pQuantities[nIndex] + 1;
        } else {
            return;
        }
    } else {
        nIndex = FieldInventoryFindEmptySlotById(nItemId);
        if (nIndex == -1) {
            return;
        }
        pIds[nIndex] = nItemId;
        nQuantity = 1;
    }
    pQuantities[nIndex] = nQuantity;
}
*/

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
