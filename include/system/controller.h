#ifndef _XENO_CONTROLLER_INPUT_H
#define _XENO_CONTROLLER_INPUT_H

// Internal controller type masks
#define CONTROLLER_INTERNAL_TYPE_MOUSE 0x10
#define CONTROLLER_INTERNAL_TYPE_DIGITAL_PAD 0x40
#define CONTROLLER_INTERNAL_TYPE_ANALOG_PAD 0x50
#define CONTROLLER_INTERNAL_TYPE_ANALOG_STICK 0x70

// Controller Status values
#define CONTROLLER_STATUS_SUCCESS 0x0
#define CONTROLLER_STATUS_ERROR 0xFF

// External Controller types
#define CONTROLLER_TYPE_ERROR -1
#define CONTROLLER_TYPE_NONE 0x0
#define CONTROLLER_TYPE_DIGITAL_PAD 0x1
#define CONTROLLER_TYPE_MOUSE 0x2
#define CONTROLLER_TYPE_ANALOG_PAD 0x3
#define CONTROLLER_TYPE_ANALOG_STICK 0x4

#define CONTROLLER_BUFFER_SIZE 0x22
#define CONTROLLER_MAX_NUM_STATES 0x10

// Controller buffer indices
#define CONTROLLER_STATUS 0x0
#define CONTROLLER_TYPE 0x1
#define CONTROLLER_BUTTONS_1 0x2
#define CONTROLLER_BUTTONS_2 0x3

// Special button bits for analog stick controllers
#define CTRL_ANALOG_STICK_BTN_L1 0x2
#define CTRL_ANALOG_STICK_BTN_SQUARE 0x4
#define CTRL_ANALOG_STICK_BTN_TRIANGLE 0x8
#define CTRL_ANALOG_STICK_BTN_R1 0x10
#define CTRL_ANALOG_STICK_BTN_R2 0x80

// Button State Values
#define CTRL_BTN_L2 0x1
#define CTRL_BTN_R2 0x2
#define CTRL_BTN_L1 0x4
#define CTRL_BTN_R1 0x8
#define CTRL_BTN_TRIANGLE 0x10
#define CTRL_BTN_CIRCLE 0x20
#define CTRL_BTN_CROSS 0x40
#define CTRL_BTN_SQUARE 0x80
#define CTRL_BTN_SELECT 0x100
#define CTRL_BTN_L3 0x200
#define CTRL_BTN_R3 0x400
#define CTRL_BTN_START 0x800
#define CTRL_BTN_UP 0x1000
#define CTRL_BTN_RIGHT 0x2000
#define CTRL_BTN_DOWN 0x4000
#define CTRL_BTN_LEFT 0x8000

// One PAD buffer record, as the PAD library fills it (CONTROLLER_BUFFER_SIZE bytes).
typedef struct {
    /* 0x0 */ u_char status;
    /* 0x1 */ u_char id;         // recvSize low nibble, controller type high nibble
    /* 0x2 */ u_char buttons1;
    /* 0x3 */ u_char buttons2;
    /* 0x4 */ u_char rightStickX;
    /* 0x5 */ u_char rightStickY;
    /* 0x6 */ u_char leftStickX;
    /* 0x7 */ u_char leftStickY;
    /* 0x8 */ u_char unk8[0x1A];
} ControllerBuffer; // Size: 0x22

extern u_short g_C1ButtonState;
extern u_short g_C1ButtonStatePressedOnce;
extern u_short g_C1ButtonStateReleased;
extern u_short g_C1ButtonStatesPressed[CONTROLLER_MAX_NUM_STATES];
extern u_short g_C1ButtonStatesPressedOnce[CONTROLLER_MAX_NUM_STATES];
extern u_short g_C1ButtonStatesReleased[CONTROLLER_MAX_NUM_STATES];
extern u_short g_C2ButtonState;
extern u_short g_C2ButtonStatePressedOnce;
extern u_short g_C2ButtonStateReleased;
extern u_short g_C2ButtonStatesPressed[CONTROLLER_MAX_NUM_STATES];
extern u_short g_C2ButtonStatesPressedOnce[CONTROLLER_MAX_NUM_STATES];
extern u_short g_C2ButtonStatesReleased[CONTROLLER_MAX_NUM_STATES];
extern int g_ControllerCurStateWriteIndex;
extern int g_ControllerCurStateReadIndex;
extern int g_ControllerIsStateStackFull;
extern u_int g_ControllerNumStates;

extern u_char g_C1LeftStickXAxis;
extern u_char g_C1LeftStickYAxis;
extern u_char g_C1RightStickXAxis;
extern u_char g_C1RightStickYAxis;
extern u_char g_C2LeftStickXAxis;
extern u_char g_C2LeftStickYAxis;
extern u_char g_C2RightStickXAxis;
extern u_char g_C2RightStickYAxis;

extern int g_C1PrevButtonState;
extern int g_C2PrevButtonState;

// Counts to keep track of how long no buttons has been released
extern int g_C1FramesSinceButtonRelease;
extern int g_C2FramesSinceButtonRelease;

// Timer interrupt count
extern int g_SpuChannelMask;

// Analog => Stick mappings
extern u_char g_ControllerStickToAnalogX[];
extern u_char g_ControllerStickToAnalogY[];

extern u_char g_ControllerType;

// Controller buffer start; the PAD library writes raw bytes here, so the record
// view is ControllerBuffer.
extern u_char g_C1Buffer[];

// Controller buffer 2
extern u_char g_C2Buffer[];

extern int D_80050200;
extern short D_800594EC;
extern short D_800594E8;
extern short D_800594E0;
extern short D_800594DC;
extern short D_800595CC;
extern short D_800595C8;

extern u_char g_ControllerStickToAnalogX[0x10];
/*
u_char g_ControllerStickToAnalogX[0x10] = {
    0x80, 0x80, 0xFF, 0xFF,
    0x80, 0x80, 0xFF, 0x80,
    0x0, 0x0, 0x80, 0x80,
    0x0, 0x80, 0x80, 0x80
}
*/

extern u_char g_ControllerStickToAnalogY[0x10];
/*
u_char g_ControllerStickToAnalogY[0x10] = {
    0x80, 0x0, 0x80, 0x0,
    0xFF, 0x80, 0xFF, 0x80,
    0x80, 0x0, 0x80, 0x80,
    0xFF, 0x80, 0x80, 0x80
}
*/

extern u_char g_ControllerButtonMappings[8];
/*
u_char g_ControllerButtonMappings[8] = {
  0x0, 0x1, 0x2, 0x3,
  0x4, 0x5, 0x6, 0x7
};
*/

extern u_short g_ControllerButtonMasks[8];
/*
u_short g_ControllerButtonMasks[8] = {
  CTRL_BTN_CIRCLE, CTRL_BTN_CROSS, CTRL_BTN_TRIANGLE, CTRL_BTN_SQUARE,
  CTRL_BTN_L1, CTRL_BTN_L2, CTRL_BTN_R1, CTRL_BTN_R2
};
*/

#endif