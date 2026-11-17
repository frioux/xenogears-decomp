#ifndef _XENO_BATTLE_WORK_H
#define _XENO_BATTLE_WORK_H

#include "psyq/libgpu.h"

// Per-frame battle display state (g_BattleDisplayState): the flags and atlas indices the
// battle draw units read while building the ordering table. Only the fields
// src/battle/main/misc2 touches are named; other units reach past 0x104, so
// 0x108 is a lower bound on the real size.
typedef struct {
    /* 0x000 */ u8 unk0[0x9C];
    /* 0x09C */ u8 spriteColumnPending; // set when the 0xBA8 list has prims to add
    /* 0x09D */ u8 menuColumnPending; // set when the 0x1E68 list has prims to add
    /* 0x09E */ u8 cursorPending; // set when the 0x27C8 list has prims to add
    /* 0x09F */ u8 unk9F;
    /* 0x0A0 */ u8 unkA0;
    /* 0x0A1 */ u8 unkA1;
    /* 0x0A2 */ u8 unkA2;
    /* 0x0A3 */ u8 primStart; // prim index the 0x641C lists were built at
    /* 0x0A4 */ u8 unkA4;
    /* 0x0A5 */ u8 spriteColumnPrimStart;
    /* 0x0A6 */ u8 menuColumnPrimStart;
    /* 0x0A7 */ u8 cursorPrimStart;
    /* 0x0A8 */ u8 spriteWorkListsPending; // set when g_BattleSpriteColumnWork's lists have prims
    /* 0x0A9 */ u8 unkA9[0x1D];
    /* 0x0C6 */ u8 pulseTriangleActive; // BattleUpdateRotationBuffer returns unless set
    /* 0x0C7 */ u8 unkC7;
    /* 0x0C8 */ u8 packagePending; // set when a package of BattlePrimPool is to be added
    /* 0x0C9 */ u8 objectRenderPending;
    /* 0x0CA */ u8 unkCA;
    /* 0x0CB */ u8 primListsPending; // set when the 0x641C lists have prims to add
    /* 0x0CC */ u8 unkCC[0x4];
    /* 0x0D0 */ s32 packageCounts[2]; // packages appended to each 0x641C list
    /* 0x0D8 */ u8 unkD8[0x20];
    /* 0x0F8 */ s32 spriteColumnPackageCount;
    /* 0x0FC */ s32 menuColumnPackageCount;
    /* 0x100 */ s32 cursorPackageCount;
    /* 0x104 */ u8 unk104[0x4];
} BattleWork; // Size: 0x108

// The per-tile texture parameters BattleInitTilePrims feeds to GetTPage and
// GetClut (g_BattleGraphicsWork + 0xA238, stride 0x18).
typedef struct {
    /* 0x00 */ s32 tp; // GetTPage depth mode
    /* 0x04 */ s32 clutX;
    /* 0x08 */ s32 clutY;
    /* 0x0C */ s32 texPageX;
    /* 0x10 */ s32 texPageY;
    /* 0x14 */ s32 unk14;
} BattleTileTexture; // Size: 0x18

// The battle draw work block (g_BattleGraphicsWork). Only the fields
// src/battle/main/misc3 touches are named; other units reach past 0xA2C8, so
// that is a lower bound on the real size.
typedef struct {
    /* 0x0000 */ u8 unk0[0xA230];
    /* 0xA230 */ u8* pPacketPrims; // 0x670-byte pool of POLY_FT4 pairs
// Byte size of the packet prim pool pPacketPrims points at (0x670).
#define BATTLE_PACKET_PRIM_POOL_SIZE 0x670
    /* 0xA234 */ u8 unkA234[0x4];
    /* 0xA238 */ BattleTileTexture tiles[6];
} BattleGraphicsWork; // Size: 0xA2C8

// The per-slot draw block (g_BattleSlotGraphics[slot]): the tile primitives
// BattleInitTilePrims fills, the two greyscale quads and their draw modes.
typedef struct {
    /* 0x000 */ u8 unk0[0x140];
    /* 0x140 */ POLY_FT4 tilePrims[4][4];
    /* 0x3C0 */ POLY_G4 prims[2];
    /* 0x408 */ DR_MODE drawModes[2];
    /* 0x420 */ u8 unk420[0x180];
    /* 0x5A0 */ s32 packageCount; // packages BattleDrawSpriteQuad appended to the pool at +0x0
    /* 0x5A4 */ u8 unk5A4[0x4];
} BattleSlotGraphics; // Size: 0x5A8

// Prim pool for the sprite package drawn by BattleAddPrimAndRenderObject
// (g_BattlePrimPool): prims holds the two POLY_FT4 primIndex selects between.
typedef struct {
    /* 0x000 */ u8 unk0[0x7A4];
    /* 0x7A4 */ POLY_FT4 prims[2];
    /* 0x7F4 */ u8 primIndex;
} BattlePrimPool; // Size: 0x7F5

#endif
