#include "common.h"
#include "main/game.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

extern s32 D_8009C610;
extern u16 D_8009B674[];
extern u_char* D_8009C7EC;
extern u_char* D_8009D150;
extern u16 D_8009D478[];
extern u_char* D_8009D7E8;
extern u_char* D_8009D7EC;
extern u_char* D_8009D7F8;
extern u_char* D_8009D7FC;
extern s16 D_8009AFA0[];
extern u16 D_8009B688[];
extern u16 D_8009B64C[];
extern u16 D_8009B64E[];
extern u16 D_8009B624[];
extern u16 D_8009B626[];

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} WorldMapGridOrigin; // Size: 0x8

extern MATRIX D_8009C808;
extern MATRIX D_8009A180;
extern s16 D_8009BD3C;
extern s16 D_8009BE04;
extern u32* D_8009BE3C;
extern u_char* D_8009CEB4;
extern s16 D_8009D618[];
extern s16 D_8009D570[];
extern WorldMapGridOrigin D_8009C838;
extern s32 D_8009D7F0;


void WorldMapSetSpriteSetParent(s32 arg0, s32 arg1);
s32 WorldMapSetSceneLayerTPages(s32 arg0);
void func_80099BFC(u8* pSrc, s32 count, u32 arg2, u8* pPrim);

// 0x80-byte slot in the D_8009BE24 table
typedef struct {
    /* 0x00 */ u8 pad0[0x20];
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u8 pad22[0x6];
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ u8 pad34[0x4];
    /* 0x38 */ s32 unk38;
    /* 0x3C */ s32 unk3C;
    /* 0x40 */ s32 unk40;
    /* 0x44 */ u8 pad44[0x6];
    /* 0x4A */ u16 unk4A;
    /* 0x4C */ u8 pad4C[0x4];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ u8 pad60[0x20];
} WorldMapPlayer; // Size: 0x80

// 0x54-byte scene-layer entry in the D_8009C620 table
typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u8 unk2[0x6];
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ s32 unk14;
    /* 0x18 */ SVECTOR rot;
    /* 0x20 */ MATRIX mat;
    /* 0x40 */ u8* unk40; // header whose +0x4 is the prim count
    /* 0x44 */ u8 unk44[0x4];
    /* 0x48 */ u8* unk48; // vertex buffer
    /* 0x4C */ u8* unk4C; // copy destination for unk48
    /* 0x50 */ u8 unk50[0x4];
} WorldMapSceneLayer; // Size: 0x54

extern WorldMapPlayer* D_8009BE24;
extern WorldMapSceneLayer* D_8009C620;

void WorldMapSetSlotTPages(WorldMapSceneLayer* pLayer, u8* pSrc, s32 count, s32 abr);

typedef struct {
    /* 0x0 */ s32 offset;
    /* 0x4 */ s32 unk4;
} WorldMapTableEntry; // Size: 0x8

// 0x10-byte tile position entry in the D_8009AF30 / D_8009AF38 tables
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
    /* 0xC */ s32 unkC;
} WorldMapTilePos; // Size: 0x10

// 0x10-byte vertex built in D_8009D150
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
    /* 0xC */ s32 unkC;
} WorldMapTileVertex; // Size: 0x10

// 0x8-byte path entry built in D_8009CEB4
typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 pad6;
} WorldMapPathEntry; // Size: 0x8

extern WorldMapTilePos D_8009AF30[];
extern WorldMapTilePos D_8009AF38[];
extern int rand(void);

void WorldMapRelocateMapEntryOffsets(void) {
    WorldMapTableEntry* pEntry;
    u_char* pBase;
    s32 i;

    pBase = D_8009C7EC;
    pEntry = (WorldMapTableEntry*)pBase;
    for (i = 0; i < 0x100; i++, pEntry++) {
        if (pEntry->offset != 0) {
            pEntry->offset += (s32)pBase;
        }
    }
    for (i = 0; i < 0x10; i++) {
        D_8009D478[i] = GetClut(0xF0, i + 0x1F0);
    }
}

// Best: 92.27%. Everything matches except how &D_8009D7E8 is materialised:
// retail computes it into s0 before the second HeapAlloc, stores HeapAlloc #1's
// result through it there (delay slot) and reloads it after the call (lw v1,0(s0));
// we store and reload through the symbolic address (lui $at / fresh lui), which is
// the same 4 instructions, so the diff is register/reloc shape only. Taking the
// address into a `u_char** ppHeap` local is folded straight back by fold_rtx.
// Shape below is the natural one and stays at 92.27%. The address-in-register
// idiom is the only thing missing; both neighbours share it.
// func_800865A0 shows the identical idiom with D_8009D7F8. The 0x200-iteration
// POLY_FT4 loop and the trailing copy are byte-exact (the copy must be typed:
// memcpy of `u_char*` globals emits lwl/lwr and scores 60%).
// Screen: cdk 92.21, 2.6.0 92.27, best 92.27 here - not a compiler miss.
/*
void func_80085FE0(void) {
    POLY_FT4* pPrim;
    POLY_FT4* pDst;
    POLY_FT4* pSrc;
    s32 i;

    D_8009D7E8 = HeapAlloc(0x5000, 1);
    D_8009D7EC = HeapAlloc(0x5000, 1);
    pPrim = (POLY_FT4*)D_8009D7E8;
    for (i = 0; i < 0x200; i++, pPrim++) {
        setPolyFT4(pPrim);
        setRGB0(pPrim, 0x80, 0x80, 0x80);
        pPrim->u0 = 0;
        pPrim->v0 = 0x40;
        pPrim->u1 = 0x1F;
        pPrim->v1 = 0x40;
        pPrim->u2 = 0;
        pPrim->v2 = 0x6F;
        pPrim->u3 = 0x1F;
        pPrim->v3 = 0x6F;
        setClut(pPrim, 0xF0, 0x1FF);
        setTPage(pPrim, 0, 0, 0x380, 0x100);
    }
    pSrc = (POLY_FT4*)D_8009D7E8;
    pDst = (POLY_FT4*)D_8009D7EC;
    memcpy(pDst, pSrc, 0x5000);
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80085FE0);
void WorldMapFreeSpritePrimBuffers(void) {
    HeapFree(D_8009D7EC);
    HeapFree(D_8009D7E8);
}
// Best: 95.40% (660 bytes vs retail's 640). The quad setup in the scratchpad, both
// matrix copies (D_8009C808 and D_8009A180 into 0x1F800028/0x1F800048), RotMatrixZ
// and the 0x10-halfword copy of D_8009D478 are instruction-for-instruction exact, and
// the 5x5 tile loop matches too. The remaining 20 bytes are one register:
// retail hoists D_8009C7EC (s3) and &D_8009C83C (s4) but re-forms D_8009D618's
// address with `lui $at / addu $at,$at,v0` at each use; gcc here hoists that address
// into a sixth callee-saved register (s5), which is the extra save/restore pair and
// the 0x30 frame instead of 0x28. gcc also fills retail's `beq` delay slot (a nop)
// with the row-origin load. Second delta, cosmetic only: retail's origin base is
// &D_8009C83C with displacements 0 and -4, a struct at D_8009C838 gives 4 and 0 -
// the same linked bytes, so check_bin cannot see it.
// What got the score here (in order): two separate s16 externs 88.99, then ONE loop
// counter shared by the copy loop and the inner column loop - that is what puts
// retail's counter in s0 and forces `sw s0` before the save block - then the
// struct-pointer origin, 95.40.
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_8008615C);
/*
void func_8008615C(void) {
    WorldMapTableEntry* pEntry;
    WorldMapGridOrigin* pOrigin;
    u8* pTable;
    u16* pSrc;
    u16* pDst;
    s32 row;
    s32 id;
    s32 i;

    ((SVECTOR*)0x1F800000)->vx = -0x18;
    ((SVECTOR*)0x1F800000)->vy = -0x48;
    ((SVECTOR*)0x1F800000)->vz = 0;
    ((SVECTOR*)0x1F800008)->vx = 0x18;
    ((SVECTOR*)0x1F800008)->vy = -0x48;
    ((SVECTOR*)0x1F800008)->vz = 0;
    ((SVECTOR*)0x1F800010)->vx = -0x18;
    ((SVECTOR*)0x1F800010)->vy = 0;
    ((SVECTOR*)0x1F800010)->vz = 0;
    ((SVECTOR*)0x1F800018)->vx = 0x18;
    ((SVECTOR*)0x1F800018)->vy = 0;
    ((SVECTOR*)0x1F800018)->vz = 0;
    *(MATRIX*)0x1F800028 = D_8009C808;
    *(MATRIX*)0x1F800048 = D_8009A180;
    RotMatrixZ(-D_8009BD3C, (MATRIX*)0x1F800048);
    pSrc = D_8009D478;
    pDst = (u16*)0x1F800068;
    for (i = 0; i < 0x10; i++) {
        *pDst++ = *pSrc++;
    }
    D_8009BE04 = 0;
    pTable = D_8009C7EC;
    pOrigin = &D_8009C838;
    for (row = 0; row < 5; row++) {
        for (i = 0; i < 5; i++) {
            if (D_8009D618[row * 5 + i] == -1) {
                continue;
            }
            id = D_8009D570[(row + pOrigin->unk4) * 9 + i + pOrigin->unk0];
            pEntry = (WorldMapTableEntry*)(pTable + id * 8);
            if (pEntry->unk4 != 0) {
                func_80099BFC((u8*)pEntry->offset, pEntry->unk4, D_8009BE3C[0x1C],
                              ((u8**)&D_8009D7E8)[D_8009D7F0] + D_8009BE04 * 0x28);
            }
        }
    }
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_800863E0);
// Best: 93.98% (392 bytes = retail's exact size; instruction multiset identical
// apart from one systematic register tie: retail keeps pVertex in s3 and the x
// counter in s2, this body takes the reverse, which is also what moves the
// `addiu s1,pVertex,4` giv past `move s0,zero`). Both Rand loops, the 0x54/0x50
// bounds, the (D_8009AF30[i]/D_8009AF38[i]) strength-reduced strides and the
// double HeapAlloc prologue are exact.
// What the permuter found (run 1, base=926 -> 721, two runs, no ZERO):
//   yoff = y << 11 had to move INSIDE the x loop, after xoff - that alone put
//   the entry-block `x = 0` before the y-loop preheader, where retail has it
//   (a redundant `x = 0;` statement is deleted by gcc, so this is the only way).
//   Run 2 printed the same 721; run 3 (score-only) confirmed no further gain.
// Moving pVertex's declaration to the end of the list does not move the tie.
/*
void func_800863E0(void) {
    WorldMapPathEntry* pPath;
    s32 xoff;
    s32 yoff;
    s32 x;
    s32 y;
    s32 i;
    s32 value;
    WorldMapTileVertex* pVertex;

    D_8009D150 = HeapAlloc(0x500, 0);
    D_8009CEB4 = HeapAlloc(0x280, 0);
    pVertex = (WorldMapTileVertex*)D_8009D150;
    x = 0;
    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            xoff = x << 11;
            yoff = y << 11;
            for (i = 0; i < 5; i++) {
                pVertex->unk0 = (D_8009AF30[i].unk0 + xoff) << 12;
                pVertex->unk8 = (D_8009AF38[i].unk0 + yoff) << 12;
                value = -(rand() >> 10);
                pVertex->unk4 = ((value << 3) - 0x200) << 12;
                pVertex++;
            }
        }
    }
    pPath = (WorldMapPathEntry*)D_8009CEB4;
    for (i = 0; i < 0x50; i++) {
        value = (rand() & 3) + 1;
        pPath->unk0 = value * 3547;
        pPath->unk4 = -(value << 11);
        pPath->unk2 = rand() & 1;
        pPath++;
    }
}
*/
void WorldMapFreeTileVertexBuffers(void) {
    HeapFree(D_8009CEB4);
    HeapFree(D_8009D150);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_800865A0);
// Best: 53.39%. Shape recovered: 0x120 POLY_FT4 primitives in D_8009D7F8, each
// getting setPolyFT4 + setRGB0(0x26,0x26,0x26) + tpage GetTPage(0,1,0x3C0,0x100)
// + clut GetClut(0x130,0x1FE) + SetSemiTrans(p,1), then a 0x2D00-byte memcpy
// into D_8009D7FC (gcc inlines it as the 16-bytes-per-iteration loop). Retail
// hoists both 9 and 0x2C into callee-saved s5/s4 and re-reads D_8009D7F8 through
// the CSE'd symbol address in s0 (frame 0x30, six saved regs); ours rematerialises
// 0x2C per iteration with li and reloads the symbol with a fresh lui (frame 0x28).
/*
void func_800865A0(void) {
    POLY_FT4* pPrim;
    POLY_FT4* pSrc;
    s32 i;

    D_8009D7F8 = HeapAlloc(0x2D00, 1);
    D_8009D7FC = HeapAlloc(0x2D00, 1);
    pPrim = (POLY_FT4*)D_8009D7F8;
    for (i = 0; i < 0x120; i++, pPrim++) {
        setPolyFT4(pPrim);
        setRGB0(pPrim, 0x26, 0x26, 0x26);
        pPrim->tpage = GetTPage(0, 1, 0x3C0, 0x100);
        pPrim->clut = GetClut(0x130, 0x1FE);
        SetSemiTrans(pPrim, 1);
    }
    pSrc = (POLY_FT4*)D_8009D7F8;
    memcpy(D_8009D7FC, pSrc, 0x2D00);
}*/

void WorldMapFreeSemiTransPrimBuffers(void) {
    HeapFree(D_8009D7FC);
    HeapFree(D_8009D7F8);
}

void WorldMapWrapTileVertexPositions(void) {
    WorldMapTileVertex* pDst = (WorldMapTileVertex*)D_8009D150;
    WorldMapPathEntry* pSrc = (WorldMapPathEntry*)D_8009CEB4;
    s32 i;

    for (i = 0; i < 0x50; i++) {
        s32 x = pDst[i].unk0 + pSrc[i].unk0;
        s32 y = pDst[i].unk8 + pSrc[i].unk4;

        if (x > 0x1FFFFFF) {
            x -= 0x2000000;
        }
        if (x < 0) {
            x += 0x2000000;
        }
        if (y > 0x1FFFFFF) {
            y -= 0x2000000;
        }
        if (y < 0) {
            y += 0x2000000;
        }
        pDst[i].unk0 = x;
        pDst[i].unk8 = y;
    }
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80086798);
s32 WorldMapInitSlotAngleStep8(s32 index) {
    WorldMapPlayer* pSlot = &D_8009BE24[index];

    pSlot->unk50 = 0;
    pSlot->unk54 = 8;
    return 1;
}
s32 WorldMapUpdateSlotRotationMatrices(s32 index) {
    WorldMapPlayer* pSlot = &D_8009BE24[index];
    SVECTOR* pVec0 = (SVECTOR*)0x1F8000A0;
    SVECTOR* pVec1 = (SVECTOR*)0x1F8000A8;
    WorldMapSceneLayer* pLayers;
    u16 angle;

    pSlot->unk50 = (pSlot->unk50 + pSlot->unk54) & 0xFFF;
    pLayers = D_8009C620;
    pVec1->vx = 0;
    pVec0->vx = 0;
    pVec0->vy = pLayers[12].rot.vy;
    pVec1->vy = pLayers[14].rot.vy;
    angle = pSlot->unk50;
    pVec1->vz = angle;
    pVec0->vz = angle;
    RotMatrix(pVec0, &pLayers[12].mat);
    RotMatrix(pVec1, &pLayers[14].mat);
    return 1;
}
s32 func_800877E0(s32 index) {
    WorldMapPlayer* pSlot = &D_8009BE24[index];

    pSlot->unk50 = 0;
    pSlot->unk54 = 8;
    return 1;
}
s32 WorldMapUpdateSceneLayerRotationMatrices(s32 index) {
    WorldMapPlayer* pSlot = &D_8009BE24[index];
    WorldMapSceneLayer* pLayers = D_8009C620;
    s32 entryIndex = D_8009C610 * 2;
    s32 index1 = D_8009B624[entryIndex];
    s32 index2 = D_8009B626[entryIndex];
    SVECTOR* pVec0 = (SVECTOR*)0x1F8000A0;
    SVECTOR* pVec1 = (SVECTOR*)0x1F8000A8;
    WorldMapSceneLayer* pLayer1;
    WorldMapSceneLayer* pLayer2;
    u16 angle;
    long tmp[2]; // unused: reproduces retail's 0x20-byte stack frame

    pSlot->unk50 = (pSlot->unk50 + pSlot->unk54) & 0xFFF;
    pLayer1 = &pLayers[index1];
    pVec1->vx = 0;
    pVec0->vx = 0;
    pVec0->vy = pLayer1->rot.vy;
    pLayer2 = &pLayers[index2];
    pVec1->vy = pLayer2->rot.vy;
    angle = pSlot->unk50;
    pVec1->vz = angle;
    pVec0->vz = angle;
    RotMatrix(pVec0, &pLayer1->mat);
    RotMatrix(pVec1, &pLayer2->mat);
    return 1;
}
void WorldMapSetSlotTPages(WorldMapSceneLayer* pLayer, u8* pSrc, s32 count, s32 abr) {
    long tmp[2]; // unused: reproduces retail's 0x30-byte stack frame
    POLY_FT4* pPrim;
    s32 rows = count;
    s32 tpageAbr = abr;
    s32 i = 0;

    if (rows > 0) {
        pPrim = (POLY_FT4*)pSrc;
        do {
            pPrim->tpage = GetTPage(0, tpageAbr, 0x1A0, 0xA0);
            pPrim->code |= 2;
            pPrim++;
        } while (++i < rows);
    }
    memcpy(pLayer->unk4C, pLayer->unk48, rows * 0x28);
}
s32 BattleInitSlotRowsForActor(s32 index) {
    WorldMapPlayer* pSlot = &D_8009BE24[index];

    pSlot->unk50 = 0;
    pSlot->unk54 = 0x10;
    WorldMapSetSceneLayerTPages(index);
    return 1;
}
s32 WorldMapSetSceneLayerTPages(s32 index) {
    WorldMapSceneLayer* pLayers = D_8009C620;
    WorldMapSceneLayer* pLayer1;
    WorldMapSceneLayer* pLayer2;
    s32 entryIndex;
    long tmp[2]; // unused: reproduces retail's 0x10-byte stack slot

    entryIndex = D_8009C610 * 2;
    pLayer1 = &pLayers[D_8009B64C[entryIndex]];
    pLayer2 = &pLayers[D_8009B64E[entryIndex]];
    WorldMapSetSlotTPages(pLayer1, pLayer1->unk48, ((u16*)pLayer1->unk40)[2], 3);
    WorldMapSetSlotTPages(pLayer2, pLayer2->unk48, ((u16*)pLayer2->unk40)[2], 3);
    return 1;
}
s32 WorldMapSetSceneLayerRotMatrices(s32 index) {
    WorldMapPlayer* pSlot = &D_8009BE24[index];
    WorldMapSceneLayer* pLayers = D_8009C620;
    s32 entryIndex = D_8009C610 * 2;
    s32 index1 = D_8009B64C[entryIndex];
    s32 index2 = D_8009B64E[entryIndex];
    SVECTOR* pVec0 = (SVECTOR*)0x1F8000A0;
    SVECTOR* pVec1 = (SVECTOR*)0x1F8000A8;
    WorldMapSceneLayer* pLayer1 = &pLayers[index1];
    WorldMapSceneLayer* pLayer2;
    u16 angle;
    long tmp[2]; // unused: reproduces retail's 0x20-byte stack frame

    pSlot->unk50 = (pSlot->unk50 + pSlot->unk54) & 0xFFF;
    pVec1->vz = 0;
    pVec0->vz = 0;
    pVec1->vx = 0;
    pVec0->vx = 0;
    angle = pSlot->unk50;
    pVec1->vy = angle;
    pVec0->vy = angle;
    pLayer2 = &pLayers[index2];
    RotMatrix(pVec0, &pLayer1->mat);
    RotMatrix(pVec1, &pLayer2->mat);
    return 1;
}

void WorldMapBuildBasisMatrix(VECTOR* pVec, VECTOR* pWork, MATRIX* pMat) {
    s16* pOut = (s16*)pMat;

    pWork->vz = 0;
    pWork->vx = 0;
    pWork->vy = 0x1000;
    OuterProduct12(pWork, pVec, pWork);
    VectorNormal(pWork, pWork);
    pOut[0] = pWork->vx;
    pOut[1] = pWork->vy;
    pOut[2] = pWork->vz;
    OuterProduct12(pVec, pWork, pWork);
    VectorNormal(pWork, pWork);
    pOut[3] = pWork->vx;
    pOut[4] = pWork->vy;
    pOut[5] = pWork->vz;
    pOut[6] = pVec->vx;
    pOut[7] = pVec->vy;
    pOut[8] = pVec->vz;
    TransposeMatrix(pMat, pMat);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087C6C);
s32 WorldMapSetSceneSpriteSetParents(void) {
    s32 value = D_8009B674[D_8009C610];

    WorldMapSetSpriteSetParent(value, value - 4);
    WorldMapSetSpriteSetParent(value, value - 3);
    WorldMapSetSpriteSetParent(value, value - 2);
    WorldMapSetSpriteSetParent(value, value - 1);
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087FD0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80088570);
// Best: 95.92% (284 bytes = retail's size, identical instruction multiset). The
// single tie: gcc gives the &g_GameState.unk1854 pointer a0 (retail: v1), which
// pushes angleY's lh into v1 in the else branch (retail has it in v0) and hoists
// the 0x4800000 lui one insn early in the if branch. The explicit u16* pCount
// local (parked below) does NOT move that pick; a0 is free at that point, so the
// allocator's order takes it before v1. Permuter run 1 (base=410) found the
// do-while wrapper + store order now in the body. Not a compiler miss (cdk/2.6.0
// were screened by an earlier round).
/*
s32 func_80088570(s32 index) {
    WorldMapPlayer* pSlot;
    u16* pCount;
    s32 v;

    func_8008868C();
    pSlot = &D_8009BE24[index];
    pSlot->unk54 = 4;
    pSlot->unk20 = 0;
    pSlot->unk50 = 0;
    pSlot->unk58 = 0;
    pSlot->unk5C = 0xC;
    pCount = &g_GameState.unk1854;
    if (*pCount == 0) {
        (*pCount)++;
        pSlot->unk2C = 0xFFD80000;
        pSlot->unk28 = 0;
        v = 0x4800000;
    } else {
        s32 angleY = *(s16*)&g_GameState.unk184E;
        u16 posX = ((u16*)g_GameState.unk1838)[0xA];

        pSlot->unk2C = 0xFFD80000;
        pSlot->unk28 = angleY * 0x1000 + posX;
        do {
            v = *(s16*)&g_GameState.unk1852 * 0x1000 + g_GameState.unk1850;
        } while (0);
    }
    pSlot->unk30 = v;
    pSlot->unk40 = 0xB50;
    pSlot->unk38 = 0xB50;
    pSlot->unk3C = 0;
    pSlot->unk4A = 1;
    ((u16*)g_GameState.unk1838)[0xA] = pSlot->unk28;
    g_GameState.unk184E = pSlot->unk28 >> 12;
    g_GameState.unk1850 = pSlot->unk30;
    g_GameState.unk1852 = pSlot->unk30 >> 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_8008868C);
// Best: 76.81%. Two-pointer walk over the interleaved s16 pairs of D_8009AFA0
// (pX[0], pX[1] with pX += 2) offset by base = D_8009B688[D_8009C610], ending at
// a -1 sentinel. Retail materialises the sentinel as `li v0,-1` inside the loop
// and as `li a0,-1` before it, i.e. loop.c did not hoist the constant; every
// decl order / do-while / pointer-vs-index shape tried so far gets it hoisted
// into a callee-saved s3 (frame 0x28 instead of 0x20).
// Screen: cdk 61.86, 2.6.0 76.81, best 76.81 here - not a compiler miss.
/*
s32 func_8008868C(void) {
    s32 base = D_8009B688[D_8009C610];
    s16* pX = D_8009AFA0;
    s16* pY = D_8009AFA0 + 1;

    if (*pX != -1) {
        do {
            s32 vx = *pX;
            s32 vy = *pY;

            WorldMapSetSpriteSetParent(base + vx, base + vy);
            pX += 2;
            pY += 2;
        } while (*pX != -1);
    }
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80088720);
