#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "psyq/libgpu.h"
#include "psyq/libetc.h"
#include "system/archive.h"
#include "system/memory.h"
#include "system/controller.h"

extern int g_FrameDeltaTime;

extern void ArchiveCdDataSync(int mode);
extern s32 D_800ADBD0;
extern s32 g_PlayerActorIndex;
extern s16 D_800B2344;

// g_FieldActors is volatile; reading it through a non-volatile lvalue reproduces
// the original's delay-slot filling.
#define FIELD_ACTORS (*((FieldActor**)(void*)&g_FieldActors))

extern s32 g_GameSceneMapNum;
extern int func_8001B484(int fileIndex, s32 arg1);
void FieldInitializeControllersAndMouse(void);
void func_80077884(void);
void func_80077AB4(void);

void FieldInitializeControllers(void) {
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    ArchiveSetIndex(ARCHIVE_DIR_FIELD, 0);
    FieldInitializeControllersAndMouse();
}

void FieldRenderSync(void) {
    DrawSync(0);
    Vsync(0);
}

INCLUDE_ASM("asm/field/nonmatchings/main/main", FieldLoadUITextures);

void func_800777DC(void) {
    ArchiveCdDataSync(0);
    while (func_8001B484((g_GameSceneMapNum & 0xFFF) * 2, 0) != 0) {
    }
}

void FieldUpdateDeltaTime(void) {
    g_FrameDeltaTime = Vsync(1);
}

void func_80077844(s16* pOut, int x0, int x1, int x2, int x3, int x4, int x5, int x6, int x7, int x8) {
    pOut[0] = x0;
    pOut[1] = x1;
    pOut[2] = x2;
    pOut[3] = x3;
    pOut[4] = x4;
    pOut[5] = x5;
    pOut[6] = x6;
    pOut[7] = x7;
    pOut[8] = x8;
}

INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077884);

INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077AB4);

void func_80077C60(void) {
    func_80077884();
    func_80077AB4();
}

extern void* g_PartyDataBuffers[];

void FieldPartyAllocateSkinDataBuffers(void) {
    HeapChangeCurrentUser(HEAP_USER_YOSI, NULL);
    g_PartyDataBuffers[0] = HeapAlloc(0x14000, 0);
    g_PartyDataBuffers[1] = HeapAlloc(0x14000, 0);
    g_PartyDataBuffers[2] = HeapAlloc(0x14000, 0);
    HeapPinBlock(g_PartyDataBuffers[0]);
    HeapPinBlock(g_PartyDataBuffers[1]);
    HeapPinBlock(g_PartyDataBuffers[2]);
}

void FieldPartyFreeSkinDataBuffers(void) {
    HeapUnpinBlock(g_PartyDataBuffers[0]);
    HeapUnpinBlock(g_PartyDataBuffers[1]);
    HeapUnpinBlock(g_PartyDataBuffers[2]);
    HeapFree(g_PartyDataBuffers[0]);
    HeapFree(g_PartyDataBuffers[1]);
    HeapFree(g_PartyDataBuffers[2]);
}

extern s32 D_800ADB9C;

// .rodata
// Sized to the 0xC bytes splat assigns this unit's rodata subsegment. The last byte
// (0x6B) sits past the string's terminator but is present in the original binary.
const char D_8006FB80[0xC] = { 'C', 'l', 'e', 'a', 'r', ' ', 'O', 'T', 'A', 'G', 0, 0x6B };

void func_80077DAC(void) {
    D_800ADB9C = Vsync(1);
    FieldClearAndSwapOTag();
    FieldPollControllers();
    if (g_FieldSystemMode == SYSTEM_MODE_PC_HDD) {
        func_80281B00(D_8006FB80);
    }
    func_800A31E8();
}

// FieldMain-related predicate used by callers in misc6; returns -1 while the
// player actor's script flags have 0x800 set and the field is idle.
INCLUDE_ASM("asm/field/nonmatchings/main/main", func_80077E10);
// 96.33%: the early-return shape below is right (the original's `move v0,zero`
// in the first bne delay slot and the `negu v0,v0` fall-through both come out),
// but gcc gives the two condition temporaries $v1/$v0 instead of the original's
// $a0/$v1, so v0 is clobbered by the second test and the bnez delay slot gets an
// extra `move v0,zero` (the original has a `nop` there). Same size, 120 bytes.
// The `s32 result = 0; ... return result;` version is 96.00% and needs a
// `move v0,a0`; `if (cond) return -(...); return 0;` is 91.50% (extra `j`).
/*
s32 func_80077E10(void) {
    if (D_800ADBD0 != 1 || D_800B2344 != 0) {
        return 0;
    }
    return -((FIELD_ACTORS[g_PlayerActorIndex].pActorData->scriptFlags.flags & 0x800) != 0);
}
*/

INCLUDE_ASM("asm/field/nonmatchings/main/main", FieldMain);
