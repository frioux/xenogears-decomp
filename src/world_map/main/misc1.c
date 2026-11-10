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

extern s32 D_8009D144;
extern s32 D_8009BD40;
extern s16 D_8009D558;
extern s32 D_8009BBB4;
extern s32 D_8009BE28;
extern u8* D_8009BE3C;
extern s32 D_8009C5BC;

extern void func_80097440(s32* p);
extern void func_80097244(s32* p);
extern void func_80089748(void);
extern void func_80089C78(void);
extern void func_80085CDC(void);
extern void func_8008615C(void);
extern void func_800747DC(void);
extern void func_800848F4(void);
extern void func_800980D4(s32* p);
extern void func_800981C8(s32* p);
extern void func_80096130(void);
extern void func_80098CC0(void);
extern void func_800983A0(s32* p);
extern void func_8009932C(s32 arg0, s32 arg1, s32* p);
extern void func_80073B04(void);
extern void func_800737EC(void);
extern void func_80086798(void);
extern void func_800740B8(void);

s32 func_80071A58(void) {
    u8* p;
    s32* pState;
    s32 pad[2];

    if (D_8009D144 == 0) {
        func_80097440(&D_8009BD40);
    } else {
        func_80097244(&D_8009BD40);
    }
    func_80089748();
    func_80089C78();
    func_80085CDC();
    func_8008615C();
    func_800747DC();
    func_800848F4();
    func_800980D4(&D_8009BBB4);
    if (D_8009D558 != 0) {
        func_800981C8(&D_8009BE28);
        func_80096130();
        func_80098CC0();
    }
    pState = &D_8009BE28;
    func_800983A0(pState);
    p = D_8009BE3C;
    func_8009932C(*(s32*)(p + 0x70), *(s32*)(p + 0x74), pState);
    D_8009C5BC = D_8009C5BC + 0x40;
    func_80073B04();
    func_800737EC();
    func_80086798();
    if (((u16*)g_GameState.unk1838)[5] == 0) {
        func_800740B8();
    }
    return 1;
}
/* best: 89.50% (308 bytes, same size). Decoded: if arg0 < 8 an 8-entry u16
   level table at D_8009B566 is scanned while arg1 >= entry, and nLevel-1 (1 plus
   the count) goes to D_8009C610; then pEntry = (WorldMapTableEntry*)(arg0 * 8 +
   &D_8009B57C) for arg0 < 8 and &D_8009B58C otherwise, and 13 globals are set
   from pEntry. The whole 13-store tail already matches byte for byte (the source
   order is +1,+2,+3...,+0xA with unk2/unk4/unk6 stored last; the scheduler is
   what swaps each pair into +1,+3,+2,+5,+4...), so only the level loop and the
   pEntry address formation are wrong: retail materialises D_8009B564 first and
   then adds 2 (so the pointer came from an address with a runtime base, i.e.
   &D_8009B564 + 1 or a struct member), which also costs it one extra addiu,
   and it forms pEntry before the D_8009C610 store instead of after.

typedef struct {
    s16 unk0;
    s16 unk2;
    s16 unk4;
    s16 unk6;
} WorldMapTableEntry; // Size: 0x8

extern u16 D_8009B566;
extern WorldMapTableEntry D_8009B57C;
extern WorldMapTableEntry D_8009B58C;
extern s32 D_8009C610;
extern s32 D_8009CC98;
extern s32 D_8009D3D0;
extern s32 D_8009D800;
extern s32 D_8009BCD8;
extern s32 D_8009BCC8;
extern s32 D_8009D2B4;
extern s32 D_8009D160;
extern s32 D_8009BD08;
extern s32 D_8009D7CC;

void func_80071B9C(s32 arg0, s32 arg1) {
    WorldMapTableEntry* pEntry;
    u16* pTable;
    s32 nLevel;

    if (arg0 < 8) {
        nLevel = 1;
        pTable = &D_8009B566;
        while (arg1 >= *pTable) {
            pTable++;
            nLevel++;
        }
        D_8009C610 = nLevel - 1;
        pEntry = (WorldMapTableEntry*)(arg0 * 8 + (s32)&D_8009B57C);
    } else {
        pEntry = (WorldMapTableEntry*)(arg0 * 8 + (s32)&D_8009B58C);
    }
    D_8009D3C4 = pEntry->unk0 + 1;
    D_8009C17C = pEntry->unk0 + 2;
    D_8009C174 = pEntry->unk0 + 3;
    D_8009CC98 = pEntry->unk0 + 4;
    D_8009D3D0 = pEntry->unk0 + 5;
    D_8009D3C8 = pEntry->unk0 + 6;
    D_8009D800 = pEntry->unk0 + 7;
    D_8009BCC8 = pEntry->unk0 + 8;
    D_8009BCD8 = pEntry->unk0 + 9;
    D_8009BD08 = pEntry->unk0 + 0xA;
    D_8009D160 = pEntry->unk2;
    D_8009D2B4 = pEntry->unk4;
    D_8009D7CC = pEntry->unk6;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071B9C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071CDC);

/* best: 98.00%. Only the D_8009D3F8 store differs: the original reuses the
   $a0 that already holds the address for the func_80029AFC call and emits a
   bare "sh $v1,0($a0)"; gcc re-forms the symbol address and needs one extra
   lui $at. A local StreamDataQueueEntry* does not stop that folding, and neither
   does routing the address through an s32 local: cse replaces the store's
   (mem (symbol_ref)) with %hi/%lo regardless, because the address pseudo is
   created after the store. Dead end unless the address comes from a runtime base.

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
   each constant at its store. Tried and no help: an s32 local for 0x25 folds
   straight back; s32 locals for BOTH constants (fed to the two
   ArchiveDecodeAlignedSize calls and to the two archiveIndex stores) still emit
   each li at its store. sched2 will not hoist them, so this is a ready-list tie,
   not a source-order one. Worth keeping: hoisting the queue pointer to the top
   of the function DOES reproduce the original's early a0/a1 call setup, which
   is what leaves only these two li insns differing.

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

/* BLOCKED, do not convert: the stub reads jtbl_8006FB24, so it needs a C switch,
   and any C switch makes cc1 emit a table into this unit's .rodata. The overlay
   linker places every misc unit's .rodata after the asm rodata but BEFORE .text
   (linker/world_map.ld), so the table shifts all world_map code even at objdiff
   100%. Same for func_80073300 above. */
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


/* best: 26.13% - every one of the 68 insns is present with the right store
   order and width; only the register allocation differs. Decoded: p =
   &D_8009D198 (kept in $v0), 0x24-stride sub-blocks at 0x00..0xFC, each with a
   word at +0,+8,+0x10,+0x18 and bytes at +0x23 and +0x27; colours 0xFF7A70,
   0xFFF5E0, 0xC03745; bytes 8 and 0x38; the first 0x40 bytes are reached
   through their own absolute symbols (D_8009D197, D_8009D198, D_8009D1A0,
   D_8009D1A8, D_8009D1B0, D_8009D1BC, D_8009D1C4, D_8009D1CC, D_8009D1D4).
   gcc rotates the constant registers one place (it gets v1,a0,v0,a1 where the
   original has a0,a1,v1,a2): the base pointer loses the top allocation
   priority, and u32 locals for the two colours do not change it.

extern u32 D_8009D1A0;
extern u32 D_8009D1A8;
extern u32 D_8009D1B0;
extern u32 D_8009D1BC;
extern u32 D_8009D1C4;
extern u32 D_8009D1CC;
extern u32 D_8009D1D4;
extern u8 D_8009D197;
extern u8 D_8009D19B;
extern u8 D_8009D198;

void func_800736DC(void) {
    u8* p;

    p = &D_8009D198;
    D_8009D1C4 = 0xFF7A70;
    D_8009D1BC = 0xFF7A70;
    D_8009D1A0 = 0xFF7A70;
    *(u32*)p = 0xFF7A70;
    D_8009D1D4 = 0xFFF5E0;
    D_8009D1CC = 0xFFF5E0;
    D_8009D1B0 = 0xFFF5E0;
    D_8009D1A8 = 0xFFF5E0;
    D_8009D197 = 8;
    D_8009D19B = 0x38;
    p[0x23] = 8;
    p[0x27] = 0x38;
    *(u32*)(p + 0x74) = 0xC03745;
    *(u32*)(p + 0x6C) = 0xC03745;
    *(u32*)(p + 0x50) = 0xC03745;
    *(u32*)(p + 0x48) = 0xC03745;
    *(u32*)(p + 0x84) = 0xFF7A70;
    *(u32*)(p + 0x7C) = 0xFF7A70;
    *(u32*)(p + 0x60) = 0xFF7A70;
    *(u32*)(p + 0x58) = 0xFF7A70;
    p[0x47] = 8;
    p[0x4B] = 0x38;
    p[0x6B] = 8;
    p[0x6F] = 0x38;
    *(u32*)(p + 0xCC) = 0xC03745;
    *(u32*)(p + 0xC4) = 0xC03745;
    *(u32*)(p + 0xBC) = 0xC03745;
    *(u32*)(p + 0xB4) = 0xC03745;
    *(u32*)(p + 0xA8) = 0xC03745;
    *(u32*)(p + 0xA0) = 0xC03745;
    *(u32*)(p + 0x98) = 0xC03745;
    *(u32*)(p + 0x90) = 0xC03745;
    p[0x8F] = 8;
    p[0x93] = 0x38;
    p[0xB3] = 8;
    p[0xB7] = 0x38;
    *(u32*)(p + 0x114) = 0xC03745;
    *(u32*)(p + 0x10C) = 0xC03745;
    *(u32*)(p + 0x104) = 0xC03745;
    *(u32*)(p + 0xFC) = 0xC03745;
    *(u32*)(p + 0xF0) = 0xC03745;
    *(u32*)(p + 0xE8) = 0xC03745;
    *(u32*)(p + 0xE0) = 0xC03745;
    *(u32*)(p + 0xD8) = 0xC03745;
    p[0xD7] = 8;
    p[0xDB] = 0x38;
    p[0xFB] = 8;
    p[0xFF] = 0x38;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800736DC);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800737EC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800739B8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073B04);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073E30);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800740B8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80074594);
