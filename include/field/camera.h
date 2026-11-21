#ifndef _XENO_FIELD_CAMERA_H
#define _XENO_FIELD_CAMERA_H

#include "psyq/libgte.h"

#define FIELD_CAMERA_AT_MOVEMENT_ACTIVE 0x1
#define FIELD_CAMERA_EYE_MOVEMENT_ACTIVE 0x2

typedef struct {
    /* 0x0  */ undefined16 duration;
    /* 0x2  */ undefined16 _pad2;
    /* 0x4  */ VECTOR current;
    /* 0x14 */ VECTOR delta;
} FieldCameraMovement; // Size: 0x28

typedef struct {
    /* 0x0 */ int atStepDistance;
    /* 0x4 */ int eyeStepDistance;
    /* 0x8 */ short targetAngleY;
    /* 0xA */ short curAngleY;
} CameraInterpolation;

// Scripted camera state: the F* camera ops copy the scene's values into it.
// The movement flags at 0xC and the durations that follow are named on their own
// (g_CamMovementFlags, g_CamAtMovementDuration).
typedef struct {
    /* 0x0 */ s32 distance; // Scaled scene depth: (sceneScrZ * sceneScale) >> 12
    /* 0x4 */ s16 mode; // Used on its own elsewhere as g_FieldCameraMode
    /* 0x6 */ s16 dip; // Copy of g_Scene.sceneDIP
    /* 0x8 */ s16 angleY; // Copy of g_Scene.sceneAngle.vy
    /* 0xA */ s16 circleScale; // 0x1000 is 1.0 in 4.12; scales the scripted circle radius
} FieldCameraState; // Size: 0xC

extern CameraInterpolation g_CamInterpolation;


#endif