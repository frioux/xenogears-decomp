#include "common.h"
#include "main/game.h"
#include "system/archive.h"
#include "system/memory.h"

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void func_80029AFC(StreamDataQueueEntry* pQueue, int arg1, int arg2);
extern void func_8008DFF4(void* pVector);

extern void* D_8005945C;
extern void* D_8006259C;
extern void* D_8009BC38;
extern void* D_8009BCB0;
extern void* D_8009BD20;
extern void* D_8009C180;
extern void* D_8009C59C;
extern s32 D_8009C174;
extern s32 D_8009C17C;
extern s32 D_8009C584;
extern s32 D_8009C5AC;
extern s32 D_8009C5B0;
extern s32 D_8009C5B4;
extern s32 D_8009D3C4;
extern s32 D_8009D3C8;
extern u16 D_8009D3F8;
extern void* D_8009D3FC;
extern u16 D_8009D400;
extern void* D_8009D404;
extern u16 D_8009D408;
extern void* D_8009D40C;
extern u16 D_8009D410;
extern void* D_8009D414;
extern void* D_8009D528;
extern s32 D_8009BE10;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800712D0);

int func_80071A50(void) {
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071A58);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071B9C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071CDC);

void func_80071EF0(void) {
    void* pBuffer;

    D_8009C59C = HeapAlloc(ArchiveDecodeAlignedSize(D_8009C17C), 1);
    D_8009BD20 = HeapAlloc(ArchiveDecodeAlignedSize(D_8009C174), 1);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_8009D3C4), 1);
    D_8009C180 = pBuffer;
    D_8009D3FC = pBuffer;
    D_8009D410 = 0;
    D_8009D414 = 0;
    D_8009D3F8 = D_8009D3C4;
    D_8009D400 = D_8009C17C;
    D_8009D408 = D_8009C174;
    D_8009D404 = D_8009C59C;
    D_8009D40C = D_8009BD20;
    func_80029AFC((StreamDataQueueEntry*)&D_8009D3F8, 0, 0);
}

void func_80071FEC(void) {
    void* pBuffer;

    D_8005945C = HeapAlloc(ArchiveDecodeAlignedSize(0x26), 1);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(0x25), 1);
    D_8009D528 = pBuffer;
    D_8009D3FC = pBuffer;
    D_8009D3F8 = 0x25;
    D_8009D400 = 0x26;
    D_8009D404 = D_8005945C;
    D_8009D408 = 0;
    D_8009D40C = 0;
    func_80029AFC((StreamDataQueueEntry*)&D_8009D3F8, 0, 0);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80072090);

void func_800721E4(void) {
    s32 nSize;

    nSize = ArchiveDecodeAlignedSize(D_8009D3C8);
    D_8006259C = HeapAlloc(nSize, 0);
    ArchiveReadFileToBuffer(D_8009D3C8, D_8006259C, 0, 0);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80072238);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_8007299C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80072BB0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80072DB4);

void func_80073300(void) {
    s32 nMode;

    if (g_GameState.unk1834 & 0x4000) {
        switch (g_GameState.unk1834 & 0x1FFF) {
        case 1:
            nMode = 4;
            break;
        case 2:
            nMode = 5;
            break;
        case 3:
        case 4:
            nMode = 7;
            break;
        default:
            return;
        }
    } else if (g_GameState.gearRide[0] | g_GameState.gearRide[1] | g_GameState.gearRide[2]) {
        nMode = 2;
    } else {
        nMode = 1;
    }
    D_8009BE10 = nMode;
}

void func_80073398(void) {
    s32 nMode;

    nMode = D_8009BE10;
    g_GameState.unk1836 = 0;
    switch (nMode) {
    case 1:
    case 2:
        D_8009C5AC = g_GameState.unk1820 << 12;
        D_8009C584 = g_GameState.unk1824;
        D_8009C5B4 = g_GameState.unk1822 << 12;
        break;
    case 4:
    case 5:
    case 7:
        func_8008DFF4(&D_8009C5AC);
        D_8009C584 = g_GameState.unk1832;
        break;
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073448);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073530);

void func_8007369C(void) {
    D_8009BC38 = HeapAlloc(0x1000, 0);
    D_8009BCB0 = HeapAlloc(0x1000, 0);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800736DC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800737EC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800739B8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073B04);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073E30);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800740B8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80074594);
