#include "common.h"
#include "main/game.h"
#include "system/archive.h"
#include "system/memory.h"

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void func_80029AFC(StreamDataQueueEntry* pQueue, int arg1, int arg2);
extern void func_8008DFF4(void* pVector);

// D_8009D3F8 .. D_8009D414 are the 3-entry StreamDataQueueEntry stream request
// table; D_8009C5AC/B0/B4 are the 12.4 fixed point position written by
// func_8008DFF4.
extern void* D_8005945C;
extern void* D_8006259C;
extern void* D_8009BC38;
extern void* D_8009BCB0;
extern s32 D_8009BE10;
extern void* D_8009BD20;
extern void* D_8009C180;
extern s32 D_8009C174;
extern s32 D_8009C17C;
extern s32 D_8009C584;
extern void* D_8009C59C;
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

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ u16 id;
    /* 0x4 */ s16 z;
    /* 0x6 */ u16 pad6;
} WorldMapPoint; // Size: 0x8

extern WorldMapPoint* D_8009D3F4;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800712D0);

int func_80071A50(void) {
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071A58);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071B9C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071CDC);

/* best: 98.00%. Only the D_8009D3F8 store differs: the original reuses the
   $a0 that already holds the address for the func_80029AFC call and emits a
   bare "sh $v1,0($a0)"; gcc re-forms the symbol address and needs one extra
   lui $at. A local StreamDataQueueEntry* does not stop that folding.

void func_80071EF0(void) {
    void* pBuffer;
    StreamDataQueueEntry* pQueue;

    D_8009C59C = HeapAlloc(ArchiveDecodeAlignedSize(D_8009C17C), 1);
    D_8009BD20 = HeapAlloc(ArchiveDecodeAlignedSize(D_8009C174), 1);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_8009D3C4), 1);
    pQueue = (StreamDataQueueEntry*)&D_8009D3F8;
    D_8009C180 = pBuffer;
    D_8009D410 = 0;
    D_8009D3FC = pBuffer;
    D_8009D414 = 0;
    pQueue->archiveIndex = D_8009D3C4;
    D_8009D400 = D_8009C17C;
    D_8009D408 = D_8009C174;
    D_8009D404 = D_8009C59C;
    D_8009D40C = D_8009BD20;
    func_80029AFC(pQueue, 0, 0);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071EF0);
/* best: 94.98%. Store order now matches the original exactly; the only delta is
   that the original materialises the 0x25 constant into $v1 before the two
   pointer stores and keeps 0x26 in $v0 until its own store, while gcc emits
   each constant at its store. An s32 local for 0x25 folds straight back.

void func_80071FEC(void) {
    void* pBuffer;

    D_8005945C = HeapAlloc(ArchiveDecodeAlignedSize(0x26), 1);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(0x25), 1);
    D_8009D528 = pBuffer;
    D_8009D3FC = pBuffer;
    D_8009D3F8 = 0x25;
    D_8009D400 = 0x26;
    D_8009D408 = 0;
    D_8009D40C = 0;
    D_8009D404 = D_8005945C;
    func_80029AFC((StreamDataQueueEntry*)&D_8009D3F8, 0, 0);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071FEC);

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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073300);
/* best: 92.05%. Jump table shape matches (5 entries, .rodata vs jtbl_ is the
   linker-equivalent difference). Needs the value pseudo in $v0 instead of $a1
   and gcc kept case 3 and case 4 as one shared block where the original has
   two identical blocks. The explicit case 0 is required for a 5 entry table.

void func_80073300(void) {
    s32 nMode;

    if (g_GameState.unk1834 & 0x4000) {
        switch (g_GameState.unk1834 & 0x1FFF) {
        case 0:
            return;
        case 1:
            nMode = 4;
            break;
        case 2:
            nMode = 5;
            break;
        case 3:
            nMode = 7;
            break;
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
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073398);
/* best: 85.11%. Cases 1,2 share a block; 4,5,7 share a block; 3 and 6 fall
   through to the epilogue, which is why the table has 7 entries.

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
*/

/* best: 91.29%. Instruction multiset is identical; gcc hoists the p->z load
   above the D_8009C5B0 store and therefore keeps it in a second register,
   where the original reuses $v0. Tried: direct stores (91.29), s32* pointer
   stores (87.24, also changes the relocation base to D_8009C5AC+4), an s32
   temp for x, and separate x/z locals.
void func_80073448(s32 arg0) {
    WorldMapPoint* p;

    if (g_GameState.unk1834 & 0x2000) {
        g_GameState.unk1834 &= ~0x2000;
        func_8008DFF4(&D_8009C5AC);
        D_8009C584 = g_GameState.unk1832;
    } else {
        p = D_8009D3F4;
        while (*(s16*)&p->id != -1) {
            if ((s16)p->id == arg0) {
                D_8009C5B0 = 0;
                D_8009C5AC = p->x << 12;
                D_8009C5B4 = p->z << 12;
                return;
            }
            p++;
        }
        D_8009C5AC = 0;
        D_8009C5B0 = 0;
        D_8009C5B4 = 0;
    }
}
*/
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
