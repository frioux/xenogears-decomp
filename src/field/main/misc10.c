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

/* PARKED 96.70%, 176 bytes / 44 insns (retail is 176 / 44 too). This body has
   every register retail has: nItemId $s1, nIndex $s0, pIds $s2, the constant
   -1 $s3, pQuantities $s4, the loaded byte $a0, the address $v1 and the value
   $v0. The `pQuantity` pointer local is what fixes the $s0/$s1 mirror the older
   attempts had - without it the same statements score 96.70 with $s0/$s1
   swapped (176 bytes, same tail residual).
   The whole residual is the tail store's placement. Retail has ONE store:
   `beqz` (>= 99 -> epilogue) with a nop in its slot, then `j` to the `sb` that
   ends the else arm, with `addiu v0,a0,1` in the `j` slot; ours emits the `sb`
   itself in the `j` delay slot and jumps past the shared block (three `sb`
   against retail's two, paid for by retail's nop), and the else arm's two
   address `addu`s come out in the opposite order. It is jump.c failing to
   cross-jump arm 1's store onto the else arm's: arm 1's block ends with a jump
   to the epilogue while the else arm falls into it, so the two threads are not
   identical.
   Shapes measured and rejected: both arms storing `pQuantity[0]` with a shared
   `nQuantity` (86.70, value and address roles flip, no merge); the same with
   `return;` at the end of both arms (byte-identical to 86.70, cc1
   canonicalises it); both arms storing `pQuantity[0]` of an expression (96.59,
   pIds/pQuantities mirror); the pointer assigned before vs after the ids store
   in the else arm (byte-identical); one store statement after the join
   (90.41 -> 92.16 with the pointer local, same tail residual); two
   `pQuantities[nIndex]` stores (96.70, same residual). Presets on this body:
   gcc-2.6.0 96.93, 2.7.2-cdk 96.70 (identical), -O3 does not compile.
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
