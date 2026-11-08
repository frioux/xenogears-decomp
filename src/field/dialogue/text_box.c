#include "common.h"
#include "main/game.h"
#include "field/main.h"
#include "field/actor.h"
#include "field/script_vm.h"
#include "field/text_box.h"

extern s32 D_8005A444;
extern s32 D_8005A448;
extern s32 D_8005A44C;
extern s32 D_800AFD1C;

void func_8009BB0C(void) {
    int nTextBoxIndex;
    u32 lowerFlags;
    u32 upperFlags;

    // Check if there's no visible textbox owned by current actor
    if (func_8009CD18(&nTextBoxIndex) == -1) {
        g_FieldScriptMaxInstructionCount += 8;
        FieldScriptMemoryWriteU16(0x14, g_FieldScriptVMCurActor->unk81);
        g_FieldScriptVMCurActor->scriptInstructionPointer += 1;
        return;
    }
    
    if (g_FieldActors[g_FieldTextBoxes[nTextBoxIndex].talkingActorID].pActorData->flags & 0x200) {
        upperFlags = g_FieldScriptVMCurActor->dialogFlags >> 0x10;
        if (g_FieldScriptVMCurActor->dialogFlags >> 0x10 == 0) {
            lowerFlags = g_FieldScriptVMCurActor->dialogFlags & 0xFFFF;
        } else {
            lowerFlags = upperFlags & 0xFFFF;
        }
        
        if (!(lowerFlags & 0x1)) {
            if (g_FieldScriptVMCurActor->scripts[g_FieldScriptVMCurActor->curScriptIndex].flags_0x12 != 7) {
                func_800A1B70();
            }
            g_FieldTextBoxes[nTextBoxIndex].status = 0;
        }
    }
    D_800B00C0 = 1;
}

INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box", func_8009BC98);

// Best attempt: 71.87%. Everything up to the func_80033CD0 result test matches, and all
// data references are right (the embedded SystemString at 0x18, the cursor fields at
// 0x37C/0x37E/0x380/0x382 and func_80034800(pString, 0xEF, 0x1E, 0xF0) -- 0x1E in $a2
// and 0xF0 in $a3 are that call's green/blue, loaded early and surviving to the call).
// What is left is block layout: the original keeps the "D_800B00C0 = 1 and return" exit
// at the end of the function, sharing the tail store with the normal path, while gcc
// puts it inline after the two tests and inverts the second branch.
/*
// The SystemString embedded at 0x18 in FieldTextBox (see SystemString in system.c).
// Only the two fields this unit reads are named.
typedef struct {
    /* 0x00 */ undefined8 pad0[0x6C];
    /* 0x6C */ u_char unk6C;
    /* 0x6D */ undefined8 pad6D[0x17];
    /* 0x84 */ s16 unk84;
} FieldTextBoxString; // Size: 0x86

extern int func_80033CD0(FieldTextBoxString* pString);
extern void func_80034800(FieldTextBoxString* pString, u_char red, u_char green, u_char blue);

void func_8009BC98(void) {
    int nTextBoxIndex;
    int nStart;

    if (func_8009CD18(&nTextBoxIndex) == 0) {
        g_FieldScriptMaxInstructionCount += 8;

        if (func_80033CD0((FieldTextBoxString*)g_FieldTextBoxes[nTextBoxIndex]._pad) != 1 &&
            (((FieldTextBoxString*)g_FieldTextBoxes[nTextBoxIndex]._pad)->unk84 == 0 ||
             ((FieldTextBoxString*)g_FieldTextBoxes[nTextBoxIndex]._pad)->unk6C == 0)) {
            D_800B00C0 = 1;
            return;
        }

        g_FieldTextBoxes[nTextBoxIndex].cursor.visibility = 0;
        g_FieldScriptVMCurActor->unk81 = 0xFF;
        nStart = SCRIPT_READ_U8_REL(1) >> 4;
        g_FieldTextBoxes[nTextBoxIndex].cursor.startPosition = nStart;
        g_FieldTextBoxes[nTextBoxIndex].cursor.curPosition = 0;
        g_FieldTextBoxes[nTextBoxIndex].cursor.maxPosition = (SCRIPT_READ_U8_REL(1) & 0xF) - nStart + 1;
        func_80034800((FieldTextBoxString*)g_FieldTextBoxes[nTextBoxIndex]._pad, 0xEF, 0x1E, 0xF0);
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    D_800B00C0 = 1;
}

*/
s32 func_8009BE58(void) {
    return ((g_FieldScriptVMCurActor->direction - (FieldGetCameraDirection() & 0xffff)) & MASK_8DIR_MOVEMENT_NUM_DIRECTIONS) < 5;
}

// Reset text box?
void func_8009BE9C(void) {
    int index;
    
    if (SCRIPT_READ_U8_REL(1) == 0) {
        if (func_8009CD18(&index) == 0) {
            g_FieldTextBoxes[index].status = 0;
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        } else {
            g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
        }
    } else {
        g_FieldScriptVMCurActor->dialogPixelWidth = 0;
        g_FieldScriptVMCurActor->dialogPixelHeight = 0;
        g_FieldScriptVMCurActor->dialogWidth = 0;
        g_FieldScriptVMCurActor->dialogHeight = 0;
        g_FieldScriptVMCurActor->dialogFlags = 0;
        g_FieldScriptVMCurActor->scriptInstructionPointer += 2;
    }
    D_800B00C0 = 1;
}

// modifies the face id.. is this changing out the portrait?
void func_8009BF8C(void) {
    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        g_FieldScriptVMCurActor->faceId = g_FieldActors[FieldScriptVMGetActorIndex(1)].pActorData->faceId;
        func_8009C01C();
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void func_8009C01C(void) {
    u32 nActorIndex;

    if (FieldScriptVMGetActorIndex(1) != ACTOR_ID_INVALID) {
        nActorIndex = FieldScriptVMGetActorIndex(1);
        g_FieldScriptVMCurActor->scriptInstructionPointer++;
        if (func_8009C5A8(nActorIndex, 0) == -1) {
            g_FieldScriptVMCurActor->scriptInstructionPointer--;
        }
        return;
    }
    g_FieldScriptVMCurActor->scriptInstructionPointer += 6;
}

void func_8009C0B4(void) {
    func_8009C5A8(D_800AFD1C, 0);
}

void func_8009C0DC(void) {
    func_8009C5A8(D_800AFD1C, 1);
}

void func_8009C104(void) {
    func_8009C5A8(D_800AFD1C, 2);
}

void func_8009C12C(void) {
    func_8009C5A8(D_800AFD1C, 3);
}

// https://decomp.me/scratch/tL6mE
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box", func_8009C154);

// Check if there is a portrait w/ targetId that's free to use?
int func_8009C538(int targetId) {
    int i;
    
    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].visibility || g_FieldTextBoxes[i].portrait.shouldRenderPortrait != 1) {
            continue;
        }
        
        if (g_FieldTextBoxes[i].portrait.portraitID == targetId) {
            return -1;
        }
    }

    return 0;
}


INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box", func_8009C5A8);

extern u16 D_800B2174[];

void func_8009CCF8(int arg0) {
    D_800B2174[0] |= 1 << arg0;
}
// find_first_unused()?
int func_8009CD18(int* pTextBoxIndex) {
    int i;

    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].ownerActorID == D_800AFD1C && g_FieldTextBoxes[i].visibility == 0) {
            *pTextBoxIndex = i;
            return 0;
        }
    }

    return -1;
}

u32 func_8009CD7C(int bytecodeOffset) {
    u32 nActorIndex = FieldScriptVMGetActorIndex(bytecodeOffset);
    if (nActorIndex == ACTOR_ID_INVALID) {
        return D_8005A444;
    }
    return nActorIndex;
}

u32 FieldScriptVMGetActorIndex(int bytecodeOffset) {
    u32 actorID = SCRIPT_READ_U8_REL(bytecodeOffset);
    if (actorID == ACTOR_ID_INVALID) {
        actorID = D_8005A444;  
    } else if (actorID == 0xFE) {
        actorID = D_8005A448;
    } else if (actorID == 0xFD) {
        actorID = D_8005A44C;
    } else if (actorID == 0xFB) {
        actorID = D_800AFD1C;
    }
    return actorID;
}


// Set up dialog window / text box
void func_8009CE48(void) {
    g_FieldScriptVMCurActor->dialogPixelWidth = SCRIPT_READ_U8_REL(1) * 2;
    g_FieldScriptVMCurActor->dialogPixelHeight = SCRIPT_READ_U8_REL(2);
    g_FieldScriptVMCurActor->dialogWidth = SCRIPT_READ_U8_REL(3) * 3;
    g_FieldScriptVMCurActor->dialogHeight = SCRIPT_READ_U8_REL(4);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
}


// Set up dialog window / text box
void func_8009CEE0(void) {
    g_FieldScriptVMCurActor->dialogPixelWidth = FieldScriptVMGetArgument(1);
    g_FieldScriptVMCurActor->dialogPixelHeight = FieldScriptVMGetArgument(3);
    g_FieldScriptVMCurActor->dialogWidth = FieldScriptVMGetArgument(5) * 3;
    g_FieldScriptVMCurActor->dialogHeight = FieldScriptVMGetArgument(7);
    g_FieldScriptVMCurActor->dialogFlags = FieldScriptVMGetArgument(9);
    g_FieldScriptVMCurActor->scriptInstructionPointer += 0xB;
}
