#ifndef _XENO_BATTLE_WORK_H
#define _XENO_BATTLE_WORK_H

#include "psyq/libgpu.h"

// Per-frame battle display state (g_BattleDisplayState): the flags and atlas indices the
// battle draw units read while building the ordering table. Fields are named from the
// unit that shows what they hold (misc1, misc2, misc3, misc12, misc16); other units
// reach past 0x104, so 0x108 is a lower bound on the real size.
typedef struct {
    /* 0x000 */ RECT texWindowRects[6]; // texture windows the tile draw modes are set with: the whole page, then each tile's texel rect
    /* 0x030 */ u32 sourceWordOffset; // word offset into the sub-image source BattleUploadImageFrame loads, 4 words per upload
    /* 0x034 */ s32 x; // screen x of the sprite the move steps carry
    /* 0x038 */ s32 unk38;
    /* 0x03C */ s32 unk3C;
    /* 0x040 */ s32 unk40;
    /* 0x044 */ s32 y; // screen y of the sprite the move steps carry
    /* 0x048 */ s32 unk48;
    /* 0x04C */ s32 unk4C;
    /* 0x050 */ s32 unk50;
    /* 0x054 */ s32 xStep; // 8.8 increment xOffset takes per moveStep
    /* 0x058 */ s32 unk58;
    /* 0x05C */ s32 yStep; // 8.8 increment yOffset takes per moveStep
    /* 0x060 */ s32 unk60;
    /* 0x064 */ s32 xOffset; // 8.8 offset added to x, stepped once per moveSteps
    /* 0x068 */ s32 unk68;
    /* 0x06C */ s32 yOffset; // 8.8 offset added to y, stepped once per moveSteps
    /* 0x070 */ u8 unk70[0xB];
    /* 0x07B */ u8 actorSpritePackageCount; // packages built at gfx+0x9C8, one 0x50-byte row each
    /* 0x07C */ u8 slotMarks[3]; // per slot: raised when an action is queued for that slot
    /* 0x07F */ u8 unk7F[0x17];
    /* 0x096 */ u8 stringTableLoaded; // set while D_800C3DE8 holds the archive entry 3 string table
    /* 0x097 */ u8 primRowCount; // rows of 0x10-byte prims built at gfx+0x908
    /* 0x098 */ u8 primRowStart; // row of that table the prims start at
    /* 0x099 */ u8 valueSpritePrimStart[3]; // prim index each slot's value digits were built at
    /* 0x09C */ u8 spriteColumnPending; // set when the 0xBA8 list has prims to add
    /* 0x09D */ u8 menuColumnPending; // set when the 0x1E68 list has prims to add
    /* 0x09E */ u8 cursorPending; // set when the 0x27C8 list has prims to add
    /* 0x09F */ u8 unk9F;
    /* 0x0A0 */ u8 unkA0;
    /* 0x0A1 */ u8 unkA1;
    /* 0x0A2 */ u8 unkA2;
    /* 0x0A3 */ u8 primStart; // prim index the 0x641C lists were built at
    /* 0x0A4 */ u8 actorSpritePrimStart; // prim index the 0x9C8 packages were built at
    /* 0x0A5 */ u8 spriteColumnPrimStart;
    /* 0x0A6 */ u8 menuColumnPrimStart;
    /* 0x0A7 */ u8 cursorPrimStart;
    /* 0x0A8 */ u8 spriteWorkListsPending; // set when g_BattleSpriteColumnWork's lists have prims
    /* 0x0A9 */ u8 moveSteps; // position interpolation steps accumulated, 6 per frame
    /* 0x0AA */ u8 frameCount; // incremented every update; the sub-image upload runs on odd values
    /* 0x0AB */ u8 sizeSteps; // size increment steps accumulated, 1 per frame
    /* 0x0AC */ u8 unkAC;
    /* 0x0AD */ u8 actorDrawActive; // cleared when the acting slot's draw state is reset
    /* 0x0AE */ u8 slotStringTableLoaded; // set while D_800D367C holds the slot's archive string table
    /* 0x0AF */ u8 actorSpriteActive; // set while the actor's sprite group at gfx+0x9C8 is live
    /* 0x0B0 */ u8 primFlags[4]; // per slot: prim flags the *PrimFlags*Slots pairs raise
    /* 0x0B4 */ u8 unkB4[0x3];
    /* 0x0B7 */ u8 primState; // which prim set is up: the *PrimFlags*Slots setters store 0-3, 0 for none
    /* 0x0B8 */ u8 rectExpandActive[7]; // per slot: while set, the slot's rect grows toward its target size
    /* 0x0BF */ u8 rectExpandDone[7]; // per slot: set once w and h reached the target size
    /* 0x0C6 */ u8 pulseTriangleActive; // BattleUpdateRotationBuffer returns unless set
    /* 0x0C7 */ u8 actorPrimListsPending; // set while the actor's prim lists in g_BattleSpriteColumnWork are to be added
    /* 0x0C8 */ u8 packagePending; // set when a package of BattlePrimPool is to be added
    /* 0x0C9 */ u8 objectRenderPending;
    /* 0x0CA */ u8 slotTimersActive; // set to tick the g_BattlePrimPool timers down
    /* 0x0CB */ u8 primListsPending; // set when the 0x641C lists have prims to add
    /* 0x0CC */ u8 unkCC[0x4];
    /* 0x0D0 */ s32 packageCounts[2]; // packages appended to each 0x641C list
    /* 0x0D8 */ u8 unkD8[0x8];
    /* 0x0E0 */ s32 statSpriteCount[3]; // packages BattleRenderActorStatDigits appended for the slot
    /* 0x0EC */ s32 valueSpriteCount[3]; // packages BattleRenderActorValueDigits appended for the slot
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
    /* 0x14 */ s32 direntCount; // resource directory entry count ResourceHelperGetTexCoords returns for this tile
} BattleTileTexture; // Size: 0x18

// The battle draw work block (g_BattleGraphicsWork). Only the fields
// src/battle/main/misc3 touches are named; other units reach past 0xA2C8, so
// that is a lower bound on the real size.
typedef struct {
    /* 0x0000 */ u8 unk0[0x6410];
    /* 0x6410 */ s32 scrollOffset; // sub-image scroll BattleUpdateImageUpload steps by 4 between 0x7C and 0x81
    /* 0x6414 */ u8 unk6414;
    /* 0x6415 */ u8 scrollActive; // while set, scrollOffset steps every update
    /* 0x6416 */ u8 scrollDir; // 0 counts scrollOffset up to 0x81, 1 counts down
    /* 0x6417 */ u8 unk6417[0x2539];
    /* 0x8950 */ RECT uploadRect[4]; // VRAM destinations of the four sub-image uploads
    /* 0x8970 */ u8 subImages[0x18C0]; // the four sub-images BattleUploadImageFrame streams to VRAM, 0x630 bytes each
    /* 0xA230 */ u8* pPacketPrims; // 0x670-byte pool of POLY_FT4 pairs
// Byte size of the packet prim pool pPacketPrims points at (0x670).
#define BATTLE_PACKET_PRIM_POOL_SIZE 0x670
    /* 0xA234 */ u8 unkA234[0x4];
    /* 0xA238 */ BattleTileTexture tiles[6];
} BattleGraphicsWork; // Size: 0xA2C8

// The per-slot draw block (g_BattleSlotGraphics[slot]): the tile primitives
// BattleInitTilePrims fills, the two greyscale quads and their draw modes.
typedef struct {
    /* 0x000 */ u8 primBuffer[0x140]; // raw POLY_FT4 pool the slot's 0x50-byte sprite packages are built in
    /* 0x140 */ POLY_FT4 tilePrims[4][4];
    /* 0x3C0 */ POLY_G4 prims[2];
    /* 0x408 */ DR_MODE drawModes[2];
    /* 0x420 */ u8 unk420[0x180];
    /* 0x5A0 */ s32 packageCount; // packages BattleDrawSpriteQuad appended to the pool at +0x0
    /* 0x5A4 */ u8 packagePrimStart; // prim index those packages were built at
    /* 0x5A5 */ u8 unk5A5[0x3];
} BattleSlotGraphics; // Size: 0x5A8

// Prim pool for the sprite package drawn by BattleAddPrimAndRenderObject
// (g_BattlePrimPool): prims holds the two POLY_FT4 primIndex selects between.
typedef struct {
    /* 0x000 */ u8 unk0[0x394];
    /* 0x394 */ u16 workWords[0x208]; // the battle VM's word array, indexed by the script's operand
    /* 0x7A4 */ POLY_FT4 prims[2];
    /* 0x7F4 */ u8 primIndex;
} BattlePrimPool; // Size: 0x7F5

#endif
