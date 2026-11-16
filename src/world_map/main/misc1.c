#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"
#include "system/archive.h"
#include "system/memory.h"

extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void func_80029AFC(StreamDataQueueEntry* pQueue, int arg1, int arg2);
extern void WorldMapGetGameStatePosition(void* pVector);

// D_8009D3F8 .. D_8009D414 are the 3-entry StreamDataQueueEntry stream request
// table; D_8009C5AC/B0/B4 are the 12.4 fixed point position written by
// WorldMapGetGameStatePosition.
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

// The world map frame render context D_8009BE3C points at: the two ordering
// table heads it fills each frame.
typedef struct {
    /* 0x00 */ u8 unk0[0x70];
    /* 0x70 */ u_long* pOt[2];
} WorldMapRenderContext; // partial view

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800712D0);

int WorldMapModeInitNop(void) {
    return 1;
}

extern s32 D_8009D144;
extern s32 D_8009BD40;
extern s16 D_8009D558;
extern s32 D_8009BBB4;
extern s32 D_8009BE28;
extern WorldMapRenderContext* D_8009BE3C;
extern s32 D_8009C5BC;

extern void WorldMapBuildRotationMatrix(s32* p);
extern void func_80097244(s32* p);
extern void func_80089748(void);
extern void func_80089C78(void);
extern void func_80085CDC(void);
extern void func_8008615C(void);
extern void func_800747DC(void);
extern void func_800848F4(void);
extern void func_800980D4(s32* p);
extern void func_800981C8(s32* p);
extern void WorldMapWaitForLoadSlotFree(void);
extern void func_80098CC0(void);
extern void func_800983A0(s32* p);
extern void func_8009932C(u_long* pOt0, u_long* pOt1, s32* pState);
extern void func_80073B04(void);
extern void func_800737EC(void);
extern void func_80086798(void);
extern void func_800740B8(void);

s32 WorldMapUpdateFrameWithSlotProjection(void) {
    s32* pState;
    s32 pad[2];

    if (D_8009D144 == 0) {
        WorldMapBuildRotationMatrix(&D_8009BD40);
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
        WorldMapWaitForLoadSlotFree();
        func_80098CC0();
    }
    pState = &D_8009BE28;
    func_800983A0(pState);
    func_8009932C(D_8009BE3C->pOt[0], D_8009BE3C->pOt[1], pState);
    D_8009C5BC = D_8009C5BC + 0x40;
    func_80073B04();
    func_800737EC();
    func_80086798();
    if (((u16*)g_GameState.unk1838)[5] == 0) {
        func_800740B8();
    }
    return 1;
}
typedef struct {
    s16 unk0;
    s16 unk2;
    s16 unk4;
    s16 unk6;
} WorldMapTableEntry; // Size: 0x8

extern u16 D_8009B564;
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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071B9C);
/* best: 96.74% (308 bytes, same size). Everything matches except the placement
   of the D_8009B564 base: retail materialises it in the block BEFORE the pre-check
   (lui v1 / addiu %lo(B564) with addend 0) and then adds 2 inside the if, while
   gcc folds the +2 into a rematerialised la inside the if and leaves a nop in the
   pre-block. Instruction count and every other insn are identical, so the only
   remaining lever is keeping that address in a register across the branch; gcc
   cse always rematerialises it. Shapes that got here:
   - pre-check tests the SYMBOL D_8009B566 (not *pTable), so gcc folds that load
     to a fresh lui v0 (matches retail);
   - the pointer is &D_8009B564 (NOT &D_8009B566) plus a separate increment, which
     is what produces retail's la-with-addend-0 + addiu 2 pattern;
   - the loop is a do/while with the increment first, which reproduces retail's
     rotated body exactly;
   - the table index after the loop is nLevel (retail does sll v0,a0,3 after
     addiu a0,a0,1, so the entry index and D_8009C610 both use the counter), and
     pEntry is formed BEFORE the D_8009C610 store.
   Tried with no change: pTable = &D_8009B564 + nLevel, (u16*)((u8*)pTable + 2),
   the increment statement before vs after the if, and pTable[1] as the pre-check
   (that one loads through the base register instead of folding).
   Round 50: gcc forms the la as %lo(D_8009B566)+2 (the symbol it already loaded
   for the pre-check), not %lo(D_8009B564); gcc-2.7.2-cdk drops this to 81.1% and
   -G0 does not compile this unit, so the preset is not the lever.
*/
/*
void func_80071B9C(s32 arg0, s32 arg1) {
    WorldMapTableEntry* pEntry;
    u16* pTable;
    s32 nLevel;

    if (arg0 < 8) {
        nLevel = 1;
        pTable = &D_8009B564;
        if (arg1 >= D_8009B566) {
            pTable++;
            do {
                pTable++;
                nLevel++;
            } while (arg1 >= *pTable);
        }
        pEntry = (WorldMapTableEntry*)(nLevel * 8 + (s32)&D_8009B57C);
        D_8009C610 = nLevel - 1;
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

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071CDC);
/* best: 84.51% (508 bytes vs 500). Every decoded fact matches and loop 1 is
   byte-identical; only loop 2's register shape differs. Retail keeps TWO
   running pointers advanced by 8 (a2 = index ptr = &D_8009D3F8, a1 = pData ptr =
   &D_8009D3FC with a2 = a1-4, both incremented right after their own store),
   nCount in $s1 and the loop index in $a0; gcc keeps ONE queue pointer (v1,
   +8) with a `sw v0,4(v1)` displacement, nCount in $a2 and the index in $s1.
   Retail also has no `andi 0xff` after the partyMembers lbu (gcc 2.7.2 masks it
   because the u8 feeds a short store). Tried with no change: s32 locals for the
   ids (breaks loop 1), pData stored before archiveIndex, nCount++ folded into
   `pQueue[nCount++].pData`, terminator stores through the absolute symbols
   D_8009D3F8/D_8009D3FC (grows the frame to 0x38).
*/
/*
extern void* D_8009CD34[3];
extern void* D_8009BDF8[3];
extern s32 D_8009C170;

void func_80071CDC(void) {
    StreamDataQueueEntry* pQueue;
    s32 i;
    s32 nCount;
    u8 nCharacterId;
    u8 nGearId;

    for (i = 0; i < 3; i++) {
        nCharacterId = g_GameState.partyMembers[i];
        if (nCharacterId != CHARACTER_ID_NONE) {
            D_8009CD34[i] = HeapAlloc(ArchiveDecodeAlignedSize(nCharacterId + 2), 0);
            nGearId = g_GameState.characters[nCharacterId].gearId;
            if (nGearId != GEAR_ID_NONE) {
                D_8009BDF8[i] = HeapAlloc(ArchiveDecodeAlignedSize(nGearId + 0x13), 0);
            } else {
                D_8009BDF8[i] = 0;
            }
        } else {
            D_8009BDF8[i] = 0;
            D_8009CD34[i] = 0;
        }
    }
    pQueue = (StreamDataQueueEntry*)&D_8009D3F8;
    D_8009C170 = 0;
    nCount = 0;
    for (i = 0; i < 3; i++) {
        nCharacterId = g_GameState.partyMembers[i];
        if (nCharacterId != CHARACTER_ID_NONE) {
            pQueue[nCount].archiveIndex = nCharacterId + 2;
            pQueue[nCount].pData = D_8009CD34[i];
            D_8009C170++;
            nCount++;
            nGearId = g_GameState.characters[nCharacterId].gearId;
            if (nGearId != GEAR_ID_NONE) {
                pQueue[nCount].archiveIndex = nGearId + 0x13;
                pQueue[nCount].pData = D_8009BDF8[i];
                nCount++;
            }
        }
    }
    pQueue[nCount].archiveIndex = 0;
    pQueue[nCount].pData = 0;
    func_80029AFC(pQueue, 0, 0);
}
*/


INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80071EF0);
/* best: 98.00% (256 bytes vs 252). Only the D_8009D3F8 store differs: the
   original reuses the $a0 that already holds the address for the func_80029AFC
   call and emits a bare "sh $v1,0($a0)"; gcc re-forms the symbol address and
   needs one extra lui $at. A local StreamDataQueueEntry* does not stop that
   folding, and neither does routing the address through an s32 local or a
   volatile pointer: cse replaces the store's (mem (symbol_ref)) with %hi/%lo
   regardless, because the address pseudo is created after the store. Dead end
   unless the address comes from a runtime base.
   Round 51: permuter 434 iterations, base=125 best=125 - no lever in the body.

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

void WorldMapQueueTwoFileLoads(void) {
    void* pBuffer;

    D_8005945C = HeapAlloc(ArchiveDecodeAlignedSize(0x26), 1);
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(0x25), 1);
    D_8009D3F8 = 0x25;
    D_8009D528 = pBuffer;
    D_8009D3FC = pBuffer;
    D_8009D400 = 0x26;
    D_8009D408 = 0;
    D_8009D40C = 0;
    D_8009D404 = D_8005945C;
    func_80029AFC((StreamDataQueueEntry*)&D_8009D3F8, 0, 0);
}

extern s32 D_8004F304;
extern s32 D_8009CC98;
extern s32 D_8009D3D0;
extern s32 D_8009D800;
extern s32 D_8009BCC8;
extern void* D_8009C88C;
extern void* D_8009C884;
extern void* D_8009C888;
extern void* D_8009C614;
extern u16 D_8009D418;
extern void* D_8009D41C;
extern u16 D_8009D420;
extern s32 D_8009D424;

void WorldMapQueueFileLoads(void) {
    void* pBuffer;

    D_8004F304++;
    D_8009D3F8 = D_8009CC98;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_8009CC98), 1);
    D_8009C88C = pBuffer;
    D_8009D3FC = pBuffer;
    D_8009D400 = D_8009D3D0;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_8009D3D0), 0);
    D_8009C884 = pBuffer;
    D_8009D404 = pBuffer;
    D_8009D408 = D_8009D3C8;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_8009D3C8), 0);
    D_8006259C = pBuffer;
    D_8009D40C = pBuffer;
    D_8009D410 = D_8009D800;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_8009D800), 0);
    D_8009C888 = pBuffer;
    D_8009D414 = pBuffer;
    D_8009D418 = D_8009BCC8;
    pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(D_8009BCC8), 0);
    D_8009C614 = pBuffer;
    D_8009D41C = pBuffer;
    D_8009D420 = 0;
    D_8009D424 = 0;
    func_80029AFC((StreamDataQueueEntry*)&D_8009D3F8, 0, 0);
}

void WorldMapLoadSedsBank(void) {
    s32 nSize;

    nSize = ArchiveDecodeAlignedSize(D_8009D3C8);
    D_8006259C = HeapAlloc(nSize, 0);
    ArchiveReadFileToBuffer(D_8009D3C8, D_8006259C, 0, 0);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80072238);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_8007299C);
/* best: 98.37%, 532 bytes (same size). Everything matches except the four
   insns of `D_8006F954 |= 0x8000`: retail materialises the address once
   (lui v1,%hi / addiu v1,v1,%lo) and loads and stores through v1, gcc folds the
   address into each access (lui v0,%hi + lhu %lo, then lui $at + sh %lo).
   Tried with no change: a u16* local, a two-statement pointer load/store, and
   `volatile u16`. Second note: the first loop's `addu` needed the index first,
   i.e. `(u8*)(i * 0x80 + (s32)D_8009BE24)`; the ptr-first spelling gives the
   reversed addu. Sibling teardown WorldMapAllocPrimBuffers has the same 98.55% class.
   Round 50: re-confirmed; `u16* pFlags = &D_8006F954; *pFlags |= 0x8000;` (and with a
   `volatile u16*`) still gives lui v0 / lhu %lo(v0) / lui $at + sh %lo: gcc folds the
   address into the load, so the loaded value occupies the only free register and the
   store needs $at. Retail needs the address in a THIRD register (v1) used twice.
*/
/*
extern void* D_80062528;
extern void* D_8009BE24;
extern void* D_8009BC3C;
extern void* D_8009BCB4;
extern u16 D_8006F954;
extern s32 D_8009D7CC;
extern void* D_8009CD34[3];
extern void* D_8009BDF8[3];

extern void SoundSetAudioManagerVolumeWithFade(void* p, s32 arg1, s32 arg2);
extern void SoundStopAllElements(void);
extern void SoundRemoveSedsEntry(void* p);
extern void SpriteDestroy(void* p);
extern void WorldMapSaveState(void);
extern void WorldMapFreeAreaNameString(void);
extern void WorldMapFreeMode4AreaNameString(void);
extern void WorldMapFreeSpriteSets(void);
extern void WorldMapFreeSpritePrimBuffers(void);
extern void GfxFreeWorkBuffers(void);
extern void WorldMapFreeTileVertexBuffers(void);
extern void WorldMapFreeSemiTransPrimBuffers(void);
extern void WorldMapFreePrimBuffers(void);
extern void WorldMapFreeTileAnimEntries(void);
extern void WorldMapFreeFrameAnimEntries(void);
extern void WorldMapFreeSpriteGroupTable(void);
extern void WorldMapFreePrimTemplateBuffers(void);
extern void WorldMapFreeGridImages(void);
extern void WorldMapFreeSlotPool(void);
extern void WorldMapFreeLoadQueues(void);

void func_8007299C(void) {
    u8* p;
    s32 i;

    if (D_8009D7CC == 0) {
        SoundSetAudioManagerVolumeWithFade(D_80062528, 0, 0xF0);
    }
    SoundStopAllElements();
    SoundRemoveSedsEntry(D_8006259C);
    HeapFree(D_8006259C);
    for (i = 0; i < 0x40; i++) {
        p = (u8*)(i * 0x80 + (s32)D_8009BE24);
        if (*(void**)(p + 0x4C) != NULL) {
            SpriteDestroy(*(void**)(p + 0x4C));
            *(void**)((u8*)D_8009BE24 + i * 0x80 + 0x4C) = NULL;
        }
    }
    if (D_8009D7CC == 1) {
        WorldMapSaveState();
        D_8006F954 |= 0x8000;
    }
    WorldMapFreeAreaNameString();
    WorldMapFreeMode4AreaNameString();
    WorldMapFreeSpriteSets();
    WorldMapFreeSpritePrimBuffers();
    GfxFreeWorkBuffers();
    WorldMapFreeTileVertexBuffers();
    WorldMapFreeSemiTransPrimBuffers();
    WorldMapFreePrimBuffers();
    WorldMapFreeTileAnimEntries();
    WorldMapFreeFrameAnimEntries();
    WorldMapFreeSpriteGroupTable();
    WorldMapFreePrimTemplateBuffers();
    WorldMapFreeGridImages();
    HeapFree(D_8009BC38);
    HeapFree(D_8009BCB0);
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    HeapFree(D_8009C180);
    for (i = 0; i < 3; i++) {
        if (D_8009CD34[i] != NULL) {
            HeapFree(D_8009CD34[i]);
        }
        if (D_8009BDF8[i] != NULL) {
            HeapFree(D_8009BDF8[i]);
        }
    }
    WorldMapFreeSlotPool();
    WorldMapFreeLoadQueues();
}
*/

typedef struct {
    /* 0x00 */ DRAWENV drawEnv; // Size: 0x5C
    /* 0x5C */ DISPENV dispEnv; // Size: 0x14
    /* 0x70 */ u8 pad70[8];
} WorldMapGfxEnv; // Size: 0x78

extern WorldMapGfxEnv D_8009BBC8[2];
extern u8 D_8009BB48;
extern u8 D_8009BB49;
extern u8 D_8009BB4A;
extern u8 D_8009BBDE;
extern u8 D_8009BBE0;
extern u8 D_8009BBE1;
extern u8 D_8009BBE2;
extern u8 D_8009BBE3;
extern u8 D_8009BC56;
extern u8 D_8009BC58;
extern u8 D_8009BC59;
extern u8 D_8009BC5A;
extern u8 D_8009BC5B;
extern u16 D_8009BC2C;
extern u16 D_8009BC2E;
extern u16 D_8009BC30;
extern u16 D_8009BC32;
extern u16 D_8009BCA4;
extern u16 D_8009BCA6;
extern u16 D_8009BCA8;
extern u16 D_8009BCAA;
extern s32 D_8009BCDC;
extern s32 D_8009D7CC;

extern void SetNearColor(long rfc, long gfc, long bfc);

void WorldMapInitGfxEnv(void) {
    WorldMapGfxEnv* pEnv;
    s32 nNear;

    ResetGraph(1);
    D_8009BCDC = 0x100;
    SetGeomScreen(0x100);
    pEnv = D_8009BBC8;
    SetDefDrawEnv(&pEnv[0].drawEnv, 0, 0, 0x140, 0xD8);
    SetDefDrawEnv(&pEnv[1].drawEnv, 0, 0xD8, 0x140, 0xD8);
    SetDefDispEnv(&pEnv[0].dispEnv, 0, 0xD8, 0x140, 0xD8);
    SetDefDispEnv(&pEnv[1].dispEnv, 0, 0, 0x140, 0xD8);
    D_8009BC58 = 1;
    D_8009BBE0 = 1;
    D_8009BC56 = 1;
    D_8009BBDE = 1;
    if (D_8009D7CC == 2) {
        D_8009BBE1 = 0;
        D_8009BBE2 = 0;
        D_8009BBE3 = 0;
        D_8009BC59 = 0;
        D_8009BC5A = 0;
        D_8009BC5B = 0;
    } else {
        D_8009BBE1 = 0;
        D_8009BBE2 = 0;
        D_8009BBE3 = 0x70;
        D_8009BC59 = 0;
        D_8009BC5A = 0;
        D_8009BC5B = 0x70;
    }
    D_8009BCA6 = 0xA;
    D_8009BC2E = 0xA;
    D_8009BCA8 = 0x100;
    D_8009BC30 = 0x100;
    D_8009BCA4 = 0;
    D_8009BC2C = 0;
    D_8009BCAA = 0xD8;
    D_8009BC32 = 0xD8;
    SetNearColor(0x80, 0x80, 0x80);
    SetBackColor(0x80, 0x80, 0x80);
    SetFarColor(D_8009BB48, D_8009BB49, D_8009BB4A);
    nNear = 0x800;
    if (D_8009D7CC == 2) {
        nNear = 0xB00;
    }
    SetFogNearFar(nNear, 0xE80, D_8009BCDC);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80072DB4);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073300);
/* best: 92.05%, 144 bytes vs retail's 152. The JTBL layout is NOT a blocker (a
   previous note was wrong): retail's table starts 8-aligned at .rodata+0x18, so
   the trailing 0x00000000 word in jtbl_8006FB0C is the .align 3 pad before the
   next table, and a 5-entry C table at that offset reproduces the bytes exactly.
   The single blocker is that gcc folds `case 3:` and `case 4:` (both `nMode = 7`)
   into ONE block (144 bytes instead of 152). Tried with no change: both arms
   spelled separately, the arm bodies in reverse order, one arm wrapped in
   do{...}while(0), one arm in a bare { } block. Secondary: the value pseudo lands
   in $a1 where retail uses $v0, so the else chain emits `beq $3,$0` + `li a1,1`
   in the delay slot instead of retail's `bnez $3` + `addiu v0,2`.
*/
/*
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

void func_80073398(void) {
    s32 nMode;
    s32 nPos;

    nMode = D_8009BE10;
    g_GameState.unk1836 = 0;
    switch (nMode) {
    case 1:
    case 2:
        nPos = g_GameState.unk1820 << 12;
        D_8009C5AC = nPos;
        D_8009C584 = g_GameState.unk1824;
        nPos = g_GameState.unk1822 << 12;
        D_8009C5B4 = nPos;
        break;
    case 4:
    case 5:
    case 7:
        WorldMapGetGameStatePosition(&D_8009C5AC);
        D_8009C584 = g_GameState.unk1832;
        break;
    }
}

/* best: 94.74%. A single s32 nPos reused for both coordinates (below) is
   better than two separate temporaries (91.29): with separate pseudos gcc picks
   a second register for z, with one pseudo it at least reuses $v0. What still
   differs: retail loads p->z AFTER the D_8009C5B0/D_8009C5AC stores and needs
   the load-delay nop there; gcc's sched hoists the p->z load to the top of the
   block (into $v1) whatever the source order, so our block is 4 bytes shorter.
   Tried, no help: two separate temps, x-first / B0-first statement orders,
   direct stores through the globals, s32* pointer stores.
   Round 39: a volatile cast on the p->z read DOES hold it in place and keeps
   the load-delay nop (97.16%) but turns the lh into lhu + sll 16 / sra 4. */
/*
void func_80073448(s32 arg0) {
    WorldMapPoint* p;
    s32 nPos;

    if (g_GameState.unk1834 & 0x2000) {
        g_GameState.unk1834 &= ~0x2000;
        WorldMapGetGameStatePosition(&D_8009C5AC);
        D_8009C584 = g_GameState.unk1832;
    } else {
        p = D_8009D3F4;
        while (*(s16*)&p->id != -1) {
            if ((s16)p->id == arg0) {
                nPos = p->x << 12;
                D_8009C5B0 = 0;
                D_8009C5AC = nPos;
                nPos = p->z << 12;
                D_8009C5B4 = nPos;
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


extern void* LZSSHeapDecompress(void* pCompressedData, int flags);
extern s32 D_8009CD48;
extern s32 D_8009D308;
extern s32 D_8009BD30;
extern s32 D_8009C7EC;
extern s32 D_8009BCC0;
extern s32 D_8009D784;
extern s32 D_8009D77C;
extern s32 D_8009D7C8;
extern s32* D_8009BD00;
extern s32 D_8009D73C[];

/* best: 97.42%. Everything matches except one register: retail keeps pEntry
   (pBase + pHeader[1], live across the 16-entry table loop) in $s0 and gives the
   loop $a1/$a2/$a3, while gcc 2.7.2 puts pEntry in $a1 and shifts the walker to
   $a2/$a3/$t0. The 8 header stores, the loop body and the four D_8009BD00
   fixups (loads through the local, stores through the reloaded global) all
   match. Tried: pEntry computed before vs after the header stores, indexed vs
   pointer-walk loop, explicit walker copy. Allocator tie, not source order.
   Sibling WorldMapDecompressMapData in misc2 is the same function without the loop.

void func_80073530(void) {
    s32* pHeader;
    u8* pBase;
    s32* pEntry;
    s32* p;
    s32 i;

    pHeader = (s32*)D_8009C180;
    D_8009C180 = LZSSHeapDecompress(pHeader, 0);
    HeapFree(pHeader);
    pBase = (u8*)D_8009C180;
    pHeader = (s32*)pBase;
    pEntry = (s32*)(pBase + pHeader[1]);
    D_8009CD48 = (s32)(pBase + pHeader[2]);
    D_8009D308 = (s32)(pBase + pHeader[3]);
    D_8009BD30 = (s32)(pBase + pHeader[4]);
    D_8009C7EC = (s32)(pBase + pHeader[5]);
    D_8009BCC0 = (s32)(pBase + pHeader[6]);
    D_8009D784 = (s32)(pBase + pHeader[7]);
    D_8009D77C = (s32)(pBase + pHeader[8]);
    D_8009D7C8 = (s32)(pBase + pHeader[9]);
    for (i = 0; i < 0x10; i++) {
        D_8009D73C[i] = (s32)(pBase + pHeader[0xB + i]);
    }
    D_8009D3F4 = (WorldMapPoint*)((u8*)pEntry + pEntry[0]);
    p = (s32*)((u8*)pEntry + pEntry[1]);
    D_8009BD00 = p;
    p[0] = (s32)((u8*)pEntry + p[0]);
    D_8009BD00[1] = (s32)((u8*)pEntry + p[1]);
    D_8009BD00[2] = (s32)((u8*)pEntry + p[2]);
    D_8009BD00[3] = (s32)((u8*)pEntry + p[3]);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073530);

void WorldMapAllocWorkBuffers(void) {
    D_8009BC38 = HeapAlloc(0x1000, 0);
    D_8009BCB0 = HeapAlloc(0x1000, 0);
}


/* best: 26.13% under this unit's preset (gcc 2.7.2). The blocker is NOT the
   allocator: 2.7.2 folds the `u8* p = &D_8009D198` base into absolute
   `D_8009D198+NN` symbol addresses, so the 40-odd `NN($v0)` offset stores
   become `lui $at` + store pairs and the function is 106 insns instead of 68.
   Retail keeps the base in $v0. Under gcc-2.7.2-cdk, whose `la` is two RTL
   insns (`lui %hi` + `addiu %lo`) so the address is a runtime value, the same
   source scores 90.34% and reproduces the shape (la $v0 + offsets); it only
   rotates the second colour into $v0 instead of $a1 and forms the la at its
   first use. Retail: a0=0xFF7A70, a1=0xFFF5E0, v1=0xC03745, v0=base (formed
   BEFORE the first store), a2=8, a1 reused for 0x38.
   Decoded layout: 0x24-stride sub-blocks at 0x00..0xFC, each with a word at
   +0,+8,+0x10,+0x18 and bytes at +0x23 and +0x27; colours 0xFF7A70, 0xFFF5E0,
   0xC03745; bytes 8 and 0x38; the first 0x40 bytes are reached through their
   own absolute symbols (D_8009D197, D_8009D198, D_8009D1A0, D_8009D1A8,
   D_8009D1B0, D_8009D1BC, D_8009D1C4, D_8009D1CC, D_8009D1D4).
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
}*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800736DC);


extern u16 D_8009BD3A;
extern MATRIX D_8009C808;
extern SVECTOR D_8009A280[16];

// The four quad primitives the world map projects each frame, 0x48 bytes each
// at D_8009D194; D_8009D7F0 picks the half of the buffer this frame draws from.
typedef struct {
    /* 0x00 */ u32 tag; // OT link word, rewritten by addPrim
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u32 xy0; // RotTransPers4 output, one packed (x,y) per vertex
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u32 xy1;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ u32 xy2;
    /* 0x1C */ u32 unk1C;
    /* 0x20 */ u32 xy3;
    /* 0x24 */ u8 unk24[0x24];
} WorldMapQuadPrim; // Size: 0x48

extern u8 D_8009D194[];
extern s32 D_8009D7F0;
extern s32 D_80050100;

extern long RotTransPers4(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2, SVECTOR* v3, long* p0, long* p1,
                          long* p2, long* p3, long* xy0, long* xy1);
extern MATRIX* RotMatrixYXZ(SVECTOR* r, MATRIX* m);
extern MATRIX* CompMatrix(MATRIX* m0, MATRIX* m1, MATRIX* m2);
extern void SetRotMatrix(MATRIX* m);
extern void SetTransMatrix(MATRIX* m);

void func_800737EC(void) {
    SVECTOR* pRot;
    SVECTOR* pVec;
    WorldMapQuadPrim* pBase;
    WorldMapQuadPrim* pPrim;
    s32 i;
    s32 nOtz;

    pRot = (SVECTOR*)0x1F800000;
    *(u16*)0x1F800004 = 0;
    pRot->vx = 0;
    *(u16*)0x1F800002 = D_8009BD3A;
    RotMatrixYXZ(pRot, (MATRIX*)0x1F800028);
    *(s32*)0x1F800044 = 0;
    *(s32*)0x1F800040 = 0;
    *(s32*)0x1F80003C = 0;
    CompMatrix(&D_8009C808, (MATRIX*)0x1F800028, (MATRIX*)0x1F800008);
    SetRotMatrix((MATRIX*)0x1F800008);
    SetTransMatrix((MATRIX*)0x1F800008);
    pVec = D_8009A280;
    for (i = 0; i < 4; i++, pVec += 4) {
        pBase = (WorldMapQuadPrim*)(D_8009D194 + D_8009D7F0 * 0x24);
        // the sum has to stay integer arithmetic: `&pBase[i]` flips the addu operands
        pPrim = (WorldMapQuadPrim*)(i * sizeof(WorldMapQuadPrim) + (s32)pBase);
        nOtz = RotTransPers4(pVec, pVec + 1, pVec + 2, pVec + 3,
                             (long*)&pPrim->xy0, (long*)&pPrim->xy1,
                             (long*)&pPrim->xy2, (long*)&pPrim->xy3,
                             (long*)((u8*)pRot + 0x48), (long*)((u8*)pRot + 0x4C));
        if (*(s32*)((u8*)pRot + 0x4C) >= 0) {
            addPrim((P_TAG*)((u32*)D_8009BE3C->pOt[0] + (nOtz >> D_80050100)), (P_TAG*)pPrim);
        }
    }
}

extern POLY_FT4 D_8009C744[];
extern DR_MODE D_8009D3D8[];

void WorldMapSetupMapPrims(void) {
    POLY_FT4* pPrim;
    RECT tw;
    u16 tpage;
    u16 clut;
    s32 i;

    tpage = GetTPage(0, 1, 0x380, 0x100);
    clut = GetClut(0x110, 0x1FE);
    pPrim = D_8009C744;
    for (i = 0; i < 4; i++, pPrim++) {
        setPolyFT4(pPrim);
        pPrim->r0 = 0x30;
        pPrim->g0 = 0x30;
        pPrim->b0 = 0x30;
        pPrim->u0 = 0;
        pPrim->v0 = 0;
        pPrim->u1 = 0xFF;
        pPrim->v1 = 0;
        pPrim->u2 = 0;
        pPrim->v2 = 0x3F;
        pPrim->u3 = 0xFF;
        pPrim->v3 = 0x3F;
        pPrim->tpage = tpage;
        pPrim->clut = clut;
        SetSemiTrans(pPrim, 1);
    }
    tw.x = 0;
    tw.y = 0;
    tw.w = 0x80;
    tw.h = 0;
    func_800453AC(&D_8009D3D8[0], &tw);
    tw.x = 0;
    tw.y = 0;
    tw.w = 0;
    tw.h = 0;
    func_800453AC(&D_8009D3D8[1], &tw);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073B04);

// The two POLY_FT4 primitives at D_8009C5C0; retail names every field store
// by its own address (D_8009C5C3 = tag.len, D_8009C5C7 = tag.code, ...), so the
// fields are declared individually and only the base is typed.
extern POLY_FT4 D_8009C5C0[2];
extern u8 D_8009C5C3;
extern u8 D_8009C5C4;
extern u8 D_8009C5C5;
extern u8 D_8009C5C6;
extern u8 D_8009C5C7;
extern s16 D_8009C5C8;
extern s16 D_8009C5CA;
extern u8 D_8009C5CC;
extern u8 D_8009C5CD;
extern u16 D_8009C5CE;
extern s16 D_8009C5D0;
extern s16 D_8009C5D2;
extern u8 D_8009C5D4;
extern u8 D_8009C5D5;
extern u16 D_8009C5D6;
extern s16 D_8009C5D8;
extern s16 D_8009C5DA;
extern u8 D_8009C5DC;
extern u8 D_8009C5DD;
extern s16 D_8009C5E0;
extern s16 D_8009C5E2;
extern u8 D_8009C5E4;
extern u8 D_8009C5E5;
extern DR_TPAGE D_8009C5A0;
extern POLY_G3 D_8009C664[8];
extern TILE D_8009C898[0x40];

/* best: 97.23%. The store order and every per-field symbol are right; only the
   constant register allocation differs. Retail keeps 0x80 in $v0 for the whole
   first block and therefore parks 0xD0 in $t0 and 0x137 in $v1 (then reuses $t0
   for 0x7F and $v1 for 0xFF), while gcc reuses $v0 for 0xD0/0x137 since their
   live ranges do not overlap retail's. Six source orders tried (address order,
   retail store order, colours first, columns first); retail store order is the
   best. The last mismatch is the TILE loop's base: retail forms
   lui/addiu %hi(D_8009C8A6) (the array seen through tile->h) and stores at
   -0xB..0, ours forms D_8009C898+0xE, which links identically but shows as an
   addend difference.
void func_80073E30(void) {
    POLY_FT4* pPrim;
    POLY_G3* pPoly;
    TILE* pTile;
    s32 i;

    pPrim = D_8009C5C0;
    D_8009C5C3 = 9;
    D_8009C5C7 = 0x2C;
    D_8009C5CA = 0x78;
    D_8009C5D2 = 0x78;
    D_8009C5DA = 0xD7;
    D_8009C5E2 = 0xD7;
    D_8009C5C8 = 0xD0;
    D_8009C5D8 = 0xD0;
    D_8009C5D0 = 0x137;
    D_8009C5E0 = 0x137;
    D_8009C5CC = 0;
    D_8009C5CD = 0x80;
    D_8009C5D4 = 0x7F;
    D_8009C5D5 = 0x80;
    D_8009C5DC = 0;
    D_8009C5DD = 0xFF;
    D_8009C5E4 = 0x7F;
    D_8009C5E5 = 0xFF;
    D_8009C5C4 = 0x80;
    D_8009C5C5 = 0x80;
    D_8009C5C6 = 0x80;
    D_8009C5D6 = GetTPage(0, 0, 0x380, 0x100);
    D_8009C5CE = GetClut(0x100, 0x1FE);
    SetSemiTrans(pPrim, 1);
    pPrim[1] = pPrim[0];
    SetDrawTPage(&D_8009C5A0, 1, 0, GetTPage(0, 1, 0x380, 0x100));
    pPoly = D_8009C664;
    for (i = 0; i < 8; i++, pPoly++) {
        setPolyG3(pPoly);
        pPoly->r0 = 0xFF;
        pPoly->g0 = 0x40;
        pPoly->b0 = 0x40;
        pPoly->r1 = 0;
        pPoly->g1 = 0;
        pPoly->b1 = 0;
        pPoly->r2 = 0;
        pPoly->g2 = 0;
        pPoly->b2 = 0;
        SetSemiTrans(pPoly, 1);
    }
    pTile = D_8009C898;
    for (i = 0; i < 0x40; i++, pTile++) {
        setTile(pTile);
        pTile->r0 = 0x80;
        pTile->g0 = 0x80;
        pTile->b0 = 0x10;
        pTile->w = 2;
        pTile->h = 2;
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_80073E30);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc1", func_800740B8);
extern void* D_8009D30C;
extern void* D_8009BE14;
extern void* D_8009BE18;
extern s32 D_8009BE38;

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} WorldMapSlot; // Size: 0x8

typedef struct {
    /* 0x0 */ u32 w0;
    /* 0x4 */ u32 w1;
    /* 0x8 */ u32 w2;
    /* 0xC */ u32 w3;
} WorldMapChunk; // Size: 0x10

void WorldMapAllocPrimBuffers(void) {
    WorldMapSlot* pSlot;
    POLY_FT4* pPrim;
    WorldMapChunk* pSrc;
    WorldMapChunk* pDst;
    WorldMapChunk* pEnd;
    s32 i;

    D_8009D30C = HeapAlloc(0x80, 0);
    D_8009BE14 = HeapAlloc(0x280, 0);
    D_8009BE18 = HeapAlloc(0x280, 0);
    pSlot = (WorldMapSlot*)D_8009D30C;
    for (i = 0xF; i != -1; i--) {
        pSlot->unk4 = 0;
        pSlot->unk2 = 0;
        pSlot->unk0 = 0;
        pSlot++;
    }
    pPrim = (POLY_FT4*)D_8009BE14;
    for (i = 0xF; i != -1; i--) {
        setPolyFT4(pPrim);
        pPrim->r0 = 0x40;
        pPrim->g0 = 0x40;
        pPrim->b0 = 0x48;
        pPrim->u0 = 0x80;
        pPrim->v0 = 0xF0;
        pPrim->u1 = 0x8F;
        pPrim->v1 = 0xF0;
        pPrim->u2 = 0x80;
        pPrim->v2 = 0xFF;
        pPrim->u3 = 0x8F;
        pPrim->v3 = 0xFF;
        pPrim->clut = GetClut(0x120, 0x1FE);
        pPrim->tpage = GetTPage(0, 0, 0x380, 0x100);
        SetSemiTrans(pPrim, 1);
        pPrim++;
    }
    pSrc = (WorldMapChunk*)D_8009BE14;
    pDst = (WorldMapChunk*)D_8009BE18;
    pEnd = pSrc + 0x28;
    do {
        *pDst++ = *pSrc++;
    } while (pSrc != pEnd);
    D_8009BE38 = 0;
}
