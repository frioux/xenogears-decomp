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

INCLUDE_ASM("asm/field/nonmatchings/main/misc10", func_8009640C);

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
