#ifndef __SYSTEM_ANIMATION_H
#define __SYSTEM_ANIMATION_H

#include "field/actor.h"
#include "psyq/libgpu.h"

#define NUM_RENDER_CONTEXTS 2

typedef void (*WorkListCallback_t)(void*);

typedef struct {
    RECT rect;
    u_long* addr;
    struct Image* pNext;
} Image;



typedef struct {
    struct WorkListEntry* unk0; //
    void* unk4; // pSpriteData, at least in some cases. Could be a more general pointer to data
    WorkListCallback_t onTriggerCallback;
    WorkListCallback_t onFreeCallback;

    // Flags, shrug
    u32 unk10: 29;
    u32 unk10_1: 1;
    u32 unk10_2: 1;
    u32 unk10_3 : 1;

    u32 unk14: 29;
    u32 unk14_1: 1;
    u32 unk14_2: 1;
    u32 unk14_3 : 1;

    struct WorkListEntry* pNext;
} WorkListEntry;

typedef struct {
    WorkListEntry task1;
    WorkListEntry task2;
    SpriteData spriteData;
} AnimTask;

extern u8 D_800591AF;
extern WorkListCallback_t func_80022DF4[];
extern WorkListCallback_t func_80022EB8[];
extern void* g_GfxWorkBuffer2;
extern MATRIX D_8004FBB8;
extern Image* g_GfxImageList[2];
extern u_long* g_GfxCurOT;


int g_GfxWorkBufferSize;
s32 D_80059300; // LinkedLists of SpriteTileData pointers
s32 D_80059304;
void* g_GfxWorkBuffers;
int g_GfxCurContext;
void* g_GfxCurWorkBufferEnd;
void* g_GfxCurWorkBuffer;


#endif