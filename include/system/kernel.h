#ifndef _XENO_KERNEL_H
#define _XENO_KERNEL_H

#include "psyq/libgpu.h"

#define TRUE 1
#define FALSE 0

typedef struct {
    DRAWENV drawEnv;
    DISPENV dispEnv;
    void* ot;
    POLY_F3 cursor;
} KernelMenuRenderEnvironment;

extern int g_KernelMenuCurChoice;
extern int g_KernelMenuIsRunning;
extern KernelMenuRenderEnvironment* g_KernelMenuCurRenderEnvironment;
extern KernelMenuRenderEnvironment g_KernelMenuRenderEnvironments[2];

// Version?
extern char g_PlayTimeHours;
extern char g_PlayTimeMinutes;
extern char g_PlayTimeSeconds;

// Is Debug?
extern int g_DebugTable;

extern int D_800592C4;
extern int D_800592C8; // current menu render context

#endif