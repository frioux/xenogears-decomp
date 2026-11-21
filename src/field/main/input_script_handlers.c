#include "common.h"
#include "field/actor.h"
#include "field/script_vm.h"

extern u16 g_FieldScriptButtonsHeld; // Held buttons state?
extern u16 g_FieldP1ButtonsPressed; // Buttons pressed this frame

void FieldScriptVMCheckControllerInput(u_short buttonState) {
    u_short nButtonMask;
    
    nButtonMask = FieldScriptVMGetInstructionArgument(1);
    if (nButtonMask & buttonState) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(3);
    }
}

void FieldScriptCheckControllerInputExact(u_short buttonState) {
    u_short buttonMask;

    buttonMask = FieldScriptVMGetInstructionArgument(1);
    if (buttonMask == buttonState) {
        g_FieldScriptVMCurActor->scriptInstructionPointer += 5;
    } else {
        g_FieldScriptVMCurActor->scriptInstructionPointer = FieldScriptVMGetInstructionArgument(3);
    }
}

void FieldScriptCheckControllerInputExactPressed(void) {
    FieldScriptCheckControllerInputExact(g_FieldP1ButtonsPressed);
}

void FieldScriptCheckControllerInputExactHeld(void) {
    FieldScriptCheckControllerInputExact(g_FieldScriptButtonsHeld);
}

// Is button pressed handler
void FieldScriptCheckControllerInputMaskPressed(void) {
    FieldScriptVMCheckControllerInput(g_FieldP1ButtonsPressed);
}

void FieldScriptCheckControllerInputMaskHeld(void) {
    FieldScriptVMCheckControllerInput(g_FieldScriptButtonsHeld);
}

void FieldScriptClearHeldButtons(void) {
    u_short scriptInstructionPointer = g_FieldScriptVMCurActor->scriptInstructionPointer;

    g_FieldScriptButtonsHeld = 0;
    g_FieldScriptVMCurActor->scriptInstructionPointer = scriptInstructionPointer + 1;
}
