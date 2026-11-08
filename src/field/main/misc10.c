#include "common.h"
#include "main/game.h"
#include "field/actor.h"
#include "field/script_vm.h"

void func_80096214(void) {
    int nItemId;
    int nIndex;
    u_char* pQuantities;

    nItemId = FieldScriptVMGetArgument(1);
    nIndex = func_80095124(nItemId);
    pQuantities = (u_char*)func_800950A0(nItemId);
    func_8009501C(nItemId);
    if (nIndex != -1) {
        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, pQuantities[nIndex]);
    } else {
        FieldScriptMemoryWriteU16(FieldScriptVMGetInstructionArgument(3) & 0xFFFF, 0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}

void func_800962C0(void) {
    if (func_80095124(FieldScriptVMGetArgument(1)) != -1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(3);
    }
}

void func_8009631C(void) {
    func_8009635C(FieldScriptVMGetArgument(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 3;
}

INCLUDE_ASM("asm/field/nonmatchings/main/misc10", func_8009635C);

/* Best attempt (96.70%, 176 bytes vs the original's 172): the original has a
   single `sb` at the end of both branches (path 1 jumps into the tail of the
   else body), sharing one `pQuantities + nIndex` address. Here gcc emits one
   store per branch plus an extra `nop` in the beqz delay slot.
void func_8009635C(int nItemId) {
    int nIndex;
    u_char* pQuantities;
    u_char* pItems;

    nIndex = func_80095124(nItemId);
    pQuantities = (u_char*)func_800950A0(nItemId);
    pItems = (u_char*)func_8009501C(nItemId);
    if (nIndex != -1) {
        if (pQuantities[nIndex] < 0x63) {
            pQuantities[nIndex] = pQuantities[nIndex] + 1;
        }
    } else {
        nIndex = func_800951B8(nItemId);
        if (nIndex != -1) {
            pItems[nIndex] = nItemId;
            pQuantities[nIndex] = 1;
        }
    }
}
*/

void func_8009640C(void) {
    int nItemId;
    int nIndex;
    u_char* pItems;
    u_char* pQuantities;

    nItemId = FieldScriptVMGetArgument(1);
    nIndex = func_80095124(nItemId);
    if (nIndex != -1) {
        pItems = (u_char*)func_8009501C(nItemId);
        pQuantities = (u_char*)func_800950A0(nItemId);
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

void func_80096534(void) {
    if ((g_pGameState->unk1D30 >> SCRIPT_READ_U8_REL(1)) & 1) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 4;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(2);
    }
}

void func_800965A8(void) {
    g_pGameState->unk1D30 |= 1 << SCRIPT_READ_U8_REL(1);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}

void func_800965F4(void) {
    g_pGameState->unk1D30 &= ~(1 << SCRIPT_READ_U8_REL(1));
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
}
