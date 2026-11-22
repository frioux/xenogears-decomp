#include "common.h"
#include "main/game.h"
#include "battle/actor.h"
#include "battle/battle_work.h"
#include "psyq/libgpu.h"

// The sound the battle menu plays when it refuses an input: the branch that found
// nothing to act on. battle/main/misc12 names the same id for the same call.
#define BATTLE_SOUND_MENU_DENIED 0x4F

// The sprite packages BattleDrawSpriteQuad appends to a slot's draw block, one per
// corner of the quad, each drawn with its own code.
#define BATTLE_SPRITE_PACKAGE_COUNT 4

// The tile prims one tile-row builder fills in a slot's draw block: the two quads at
// tilePrims[row][g_BattlePrimStartIndex] and [g_BattlePrimStartIndex + 2].
#define BATTLE_TILE_QUAD_COUNT 2

// Where a tile-row builder takes its texels from: the low byte of the record for the
// row's texture (g_BattleGraphicsWork).tiles[row + 1] (stride 0x18 from 0xA238,
// battle_work.h), texPageX for u and texPageY 4 bytes later for v. Read as bytes, as
// the original does, because the u coordinate is the page byte's low six bits.
#define BATTLE_TILE_SOURCE_BASE 0xA25C
#define BATTLE_TILE_SOURCE_STRIDE 0x18
#define BATTLE_TILE_SOURCE_V 4

// The two tile-prim lists of the battle draw work block (g_BattleGraphicsWork + 0x641C,
// the second one 0xFA0 further on; battle/main/misc2 spells the same pair):
// packageCounts[j] packages of two POLY_FT4 prims each, built from primStart.
#define BATTLE_TILE_PRIM_LIST_BASE 0x641C
#define BATTLE_TILE_PRIM_LIST_STRIDE 0xFA0
#define BATTLE_TILE_PRIM_LIST_COUNT 2

// One actor's block of the per-actor action-row table g_BattleActionRows: 42 rows per
// actor (BattleRow, 0x28 bytes, actor.h), of which this unit reads flags (+0x0) and
// commandPointCost (+0x24) of the four rows at +0x25. The offset stays an integer:
// indexing a BattleRow array makes gcc form the actor term (arg0 * 0x690) before the
// row term and reach both fields off one base register (464 bytes against retail's
// 468, 87.56%), where retail forms the row term first and re-forms the symbol for
// each read.
#define BATTLE_ACTION_ROW_STRIDE 0x690
// The first of the four action rows this unit reads (rows 0x25..0x28 of the actor's
// 42-row block).
#define BATTLE_ACTION_ROW_FIRST 0x25
extern u8 g_BattleActionRows[];
extern u8 D_800CEF34[];

// One sprite package of the per-slot draw block: the two POLY_FT4
// BattleDrawSpritePackage writes, i.e. the 0x50-byte stride the block's package
// counter advances by.
typedef struct {
    /* 0x00 */ POLY_FT4 prims[2];
} BattleSpritePackage; // Size: 0x50

// One slot's rect record (g_BattleSlotRects[slot]): the coordinates the draw block
// expands from and the size the expand animation grows to (battle/misc16_r2 reads
// the same record through the same array).
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 targetW;
    /* 0x6 */ u16 targetH;
    /* 0x8 */ u16 w;
    /* 0xA */ u16 h;
    /* 0xC */ u8 slot;
    /* 0xD */ u8 padD;
} BattleSlotRect; // Size: 0xE

// One of the two tile-prim lists: the row of POLY_FT4 prims a tile-row builder fills.
typedef struct {
    /* 0x000 */ POLY_FT4 prims[BATTLE_TILE_PRIM_LIST_STRIDE / sizeof(POLY_FT4)];
} BattleTilePrimList; // Size: 0xFA0

extern u8 g_BattleCommandGridIds[];
extern u8 *D_800D329C;
extern u8 *g_BattleGraphicsWork;
extern u8 *g_BattleCommandQueue;
extern volatile s32 g_BattlePrimStartIndex;
extern BattleWork *g_BattleDisplayState;
extern BattleSlotGraphics *g_BattleSlotGraphics[];
extern BattleSlotRect *g_BattleSlotRects[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
extern u8 D_800CCD38[];
extern u8 D_800CCD3A[];
extern u8 D_800D2F7C;
extern u8 D_800D2F7D;
extern u8 D_800D2F7E;
extern u8 D_800D2F7F;
extern u8 D_800D2F80;
extern u8 D_800D2F81;
extern u8 D_800D2F82;
extern u8 D_800D2F83;

extern void BattleUpdateFrame(void);
extern void HeapFree(void *pMem);
extern void func_8008FC1C(s32, s32, s32, s32, u8);
extern s32 BattleDrawSpritePackage(s32, u8 *, s16, s16);
extern void BattleSetPolyFT4RectH13(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, s16 dx);
extern void BattleLoadImageAndSync(RECT *pRect, u_long *pData);
extern void *BattleAllocTextBuffer(s32);
extern void bzero(void *p, s32 n);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern u8 g_BattleCommandGridAltIds[];
extern void *BattleAllocHeapBlock(s32, s32);
extern void BattleInitSlotPrims(s32);
extern void func_8008F6E4(s32, u16, u16, u16, u16);
extern void BattleDrawSpriteQuad(u8, s32, s32, s32, u16);
extern void func_8008DE04(u8, s32, s32, u16);
extern void func_8008E430(u8, s32, s32, u16, s32);
extern void BattleBuildTileRow2Prims(u8, s32, s32, u16);
// The fifth argument of BattleBuildTileRow3Prims arrives as a four-byte aggregate, not a bare
// u16: gcc 2.6.0 loads a narrow stack parameter into a register in the function
// prologue, while an aggregate parameter stays in the caller's frame, and the
// original reads it with `lhu` from the y2 store (BattleBuildTileRow3Prims's note).
typedef struct {
    /* 0x0 */ u16 height; // the row's height: every y is inset by 8, then advanced by (height - 16) / 2
    /* 0x2 */ u16 unk2;   // unread by this unit
} BattleRowSize; // Size: 0x4
extern void BattleBuildTileRow3Prims(u8, s32, s32, s32, BattleRowSize);
extern void func_80090310(u8, u8);
extern void func_800904A0(s32, s32);
extern void BattleDrawMenuEntryString(u8, u8);
extern void *g_BattleSpriteArchive;
extern u8 g_BattleDisplayMode;
extern void BattleSetPrimGrey80BlendModeFromGlobal(POLY_FT4 *);
extern s32 func_800263E4(void *, s32, void *, s32, s16, s16, s32, s32, s32);
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
// g_BattleSlotStatusRows and D_800CCDC4 are two bases of the same per-slot table (stride
// 0x170, actor.h's BattleTableEntry): the status mask sits at +0 of the g_BattleSlotStatusRows
// base and the slot's counter at +0 of the D_800CCDC4 base, 0x62 bytes later.
extern BattleTableEntry g_BattleSlotStatusRows[];
extern BattleTableEntry D_800CCDC4[];
extern u8 g_BattleActorCharIds[];
extern s32 BattleMaskHasStatusBit(s32, u8);
extern u8 BattleRunTargetSlotSelection(u16, u8, s32);
extern void BattleClearPrimFlagsThreeSlots(u8);
extern void BattleSetPrimFlagsThreeSlots(u8);
extern void BattlePlaySoundIfEnabled(u8);
extern void BattleSetPolyFT4Rect(POLY_FT4 *, s16, s16, u8, u8, s16, s16);
extern u8 g_BattleSlotTextIndex[];
extern u8 g_BattleMenuEvent;
extern void BattleFreePacketPrimitives(void);
extern void BattleClearFlagC6(void);
extern void BattleFreeSlotBuffers(u8);
extern void BattleDrawCharStatusIfMaskUnset(u8, u8);
extern void BattleStepAnimatedCursorPrim(s32, s32, s32 *, u8 *);
extern void func_800930AC(u8, u8 *, u16 *);
extern void BattleInitPulseTrianglePrims(void);

u8 BattleTryUseSlotCommand(u8 arg0, u8 arg1) {
    u16 statusMasks[4];
    u8 flag;
    u8 ret;
    s32 idx;
    u16 trigger;
    u16 cur;
    u16 dmg;

    statusMasks[0] = D_800C324E;
    statusMasks[1] = D_800C3250;
    statusMasks[2] = D_800C3252;
    statusMasks[3] = D_800C323A;
    ret = 0;
    flag = 1;
    idx = (arg1 + 0x25) * sizeof(BattleRow) + arg0 * BATTLE_ACTION_ROW_STRIDE;
    trigger = *(u16 *)(g_BattleActionRows + idx + 0x24); // row's commandPointCost
    cur = D_800CCDC4[arg0].flags;
    dmg = *(u16 *)(g_BattleActionRows + idx); // row's flags
    if (cur >= trigger &&
        (u16)BattleMaskHasStatusBit((u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].statusFlags, arg1) != 0 &&
        (g_BattleSlotStatusRows[arg0].flags & statusMasks[arg1]) == 0) {
        BattleClearPrimFlagsThreeSlots(1);
        g_BattleDisplayState->pulseTriangleActive = 1;
        if (BattleRunTargetSlotSelection(dmg, arg0, 0) != 0) {
            D_800CCDC4[arg0].flags -= trigger;
            ret = 1;
        } else {
            g_BattleDisplayState->pulseTriangleActive = 0;
            BattleSetPrimFlagsThreeSlots(arg0);
        }
        flag = 0;
    }
    if (flag != 0) {
        BattlePlaySoundIfEnabled(BATTLE_SOUND_MENU_DENIED);
    }
    return ret;
}
// Runs the slot-command cursor menu: fills each action row's code/damage from the
// slot's status, builds the four status rows, then loops on g_BattleMenuEvent until
// the player confirms or cancels, moving the cursor and starting the chosen command.
// Load-bearing spellings (each measured): `firstRow` is a local so the row biv starts
// at retail's 0x5C8 and the read is based on D_800CEF34 (g_BattleActionRows + 0x24);
// the step call is a plain statement, not a do/while(0), which is what puts `cursor`
// in $s3 and the codes walk in $s2.
u8 BattleRunSlotCommandMenu(u8 arg0) {
    u8 codes[4];
    u16 damage[4];
    u16 statusMasks[4];
    s32 cursorFrame;
    u8 cursorTimer;
    s32 lastCursor;
    s32 firstRow;
    u8 limit;
    u8 state;
    s32 cursor;
    s32 i;

    cursor = 0;
    lastCursor = 0xFF;
    state = 2;
    statusMasks[0] = D_800C324E;
    statusMasks[1] = D_800C3250;
    statusMasks[2] = D_800C3252;
    statusMasks[3] = D_800C323A;
    cursorFrame = 4;
    cursorTimer = 0;
    for (i = 0; i < 4; i++) {
        codes[i] = 0xFF;
        damage[i] = 0;
    }
    for (i = 0; i < (limit = 4); i++) {
        if ((u16)BattleMaskHasStatusBit((u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].statusFlags, i) != 0 &&
            (g_BattleSlotStatusRows[arg0].flags & statusMasks[i]) == 0) {
            codes[i] = g_BattleSlotTextIndex[arg0 * 0x170] * 4 + i;
            firstRow = BATTLE_ACTION_ROW_FIRST;
            damage[i] = *(u16 *)((u8 *)D_800CEF34 + (firstRow + i) * sizeof(BattleRow) + arg0 * BATTLE_ACTION_ROW_STRIDE);
        }
    }
    g_BattleDisplayState->primListsPending = 0;
    BattleInitSlotGraphics(0, 0x74, 0xA0, 0xAC, 0x38, 0, 1);
    BattleInitSlotGraphics(1, 0x84, 0x4C, 0x9C, 0x50, 0, 1);
    BattleInitSlotGraphics(2, 0x1C, 0xA0, 0x50, 0x18, 0, 1);
    func_800930AC(arg0, codes, damage);
    BattleInitPulseTrianglePrims();
    while (state == 2) {
        if (cursor != lastCursor) {
            BattleDrawCharStatusIfMaskUnset(arg0, cursor);
            lastCursor = cursor;
        }
        BattleStepAnimatedCursorPrim(0x8C, cursor * 0x10 + 0x58, &cursorFrame, &cursorTimer);
        BattleUpdateFrame();
        switch (g_BattleMenuEvent) {
        case 5:
            state = 0;
            break;
        case 4:
            if (BattleTryUseSlotCommand(arg0, cursor) != 0) {
                state = 1;
                g_BattleCommandQueue[0x2E6] = cursor + 0x10;
            }
            break;
        case 1:
            cursor++;
            if (cursor >= limit) {
                cursor = 3;
            }
            break;
        case 3:
            cursor--;
            if (cursor < 0) {
                cursor = 0;
            }
            break;
        }
    }
    BattleClearPrimFlagsThreeSlots(state);
    BattleFreePacketPrimitives();
    BattleClearFlagC6();
    BattleFreeSlotBuffers(0);
    BattleFreeSlotBuffers(1);
    BattleFreeSlotBuffers(2);
    return state;
}


// Fades the prims BattleAddTwoPrimLists later combines: sets the transparency and
// shading bits on each prim of both tile-prim lists, then subtracts the packet's colour
// shift (0x2E0 of the command queue; battle/misc11 names it colorShift) from its
// r0/g0/b0. packageCounts[j] * 2 prims of a row are walked from primStart.
// Every statement re-derives its address from g_BattleGraphicsWork and the two sums are
// associated differently on purpose (row offset first for the tpage read/write, prim
// base first for the three colour stores): that asymmetry is what gives retail's two
// addu operand orders. A pointer local collapses the body to 400 bytes (51.21%), and
// the uniform `row->prims[i].field` spelling shares one base register across the store
// group for 628 bytes (88.97%).
void BattleFadeTilePrimRow(void) {
    u8 found;
    s32 j;
    s32 i;

    found = 0;
    for (j = 0; j < BATTLE_TILE_PRIM_LIST_COUNT; j++) {
        if (g_BattleDisplayState->packageCounts[j] != 0) {
            for (i = 0; i < g_BattleDisplayState->packageCounts[j] * 2; i += 2) {
                u8 c;

                SetSemiTrans(&((BattleTilePrimList *)(g_BattleGraphicsWork + BATTLE_TILE_PRIM_LIST_BASE +
                                                     j * BATTLE_TILE_PRIM_LIST_STRIDE))->prims[i + g_BattleDisplayState->primStart], 1);
                SetShadeTex(&((BattleTilePrimList *)(g_BattleGraphicsWork + BATTLE_TILE_PRIM_LIST_BASE +
                                                    j * BATTLE_TILE_PRIM_LIST_STRIDE))->prims[i + g_BattleDisplayState->primStart], 0);
                ((POLY_FT4 *)(j * BATTLE_TILE_PRIM_LIST_STRIDE + (i + g_BattleDisplayState->primStart) * sizeof(POLY_FT4) +
                              g_BattleGraphicsWork + BATTLE_TILE_PRIM_LIST_BASE))->tpage |= 0x20;
                c = ((POLY_FT4 *)(j * BATTLE_TILE_PRIM_LIST_STRIDE + (i + g_BattleDisplayState->primStart) * sizeof(POLY_FT4) +
                                  g_BattleGraphicsWork + BATTLE_TILE_PRIM_LIST_BASE))->r0;
                if (c != 0) {
                    found = 1;
                    ((POLY_FT4 *)(g_BattleGraphicsWork + j * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                  (i + g_BattleDisplayState->primStart) * sizeof(POLY_FT4)))->r0 = c - (*(u8 *)(g_BattleCommandQueue + 0x2E0) << 4);
                    ((POLY_FT4 *)(g_BattleGraphicsWork + j * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                  (i + g_BattleDisplayState->primStart) * sizeof(POLY_FT4)))->g0 = c - (*(u8 *)(g_BattleCommandQueue + 0x2E0) << 4);
                    ((POLY_FT4 *)(g_BattleGraphicsWork + j * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                  (i + g_BattleDisplayState->primStart) * sizeof(POLY_FT4)))->b0 = c - (*(u8 *)(g_BattleCommandQueue + 0x2E0) << 4);
                }
            }
        }
    }
    if (found == 0) {
        g_BattleDisplayState->primListsPending = 0;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D598);
/* 71.88% at 1652 bytes against retail's 1692 (413 vs 423 instructions) - the whole
   function, never matched. Structure that IS right and measured: the fade path, the
   two-entry table init (through a u8** to the D_800C3000 element, which is what keeps
   retail's two `lw 0(t0)` per iteration), the record walk (8-byte BattleTileRecord,
   cmd 0xFFFF terminator, the >= 0x4000 queue lookup with its 0xFF continue), the
   package append and the two grey-level loops. Residual, named: the frame. Retail keeps
   exactly six locals in memory (listIndex@0x10, codes@0x18, rowOffset@0x20,
   pListIndex@0x28, pCodesStart@0x30, pCodes@0x38, pLists@0x40) and re-forms
   `pCodesStart + 2` from a spilled pointer (`lw t1,0x30(sp); addiu v0,t1,2`) where gcc
   2.6.0 here folds the bound to the constant address `sp+0x1A` and spills a different
   set, so every register and every spill slot after the init loop differs.
   Tried: a plain `for (pCodes = codes; pCodes < pCodesStart + 2; pCodes++, pLists++,
   offset += 2)` outer walk (this body); the offset-first store addresses in the two
   grey-level loops (worth +9% by itself: 62.73 -> 71.88, retail's `v1 = offset` then
   `v1 = v1 + listbase` association, kept); an index-only outer loop. NOT tried: the
   permuter (no run - structure, not registers, is the gap) and a two-array
   stack-layout swap.
   To resume: make the outer bound a value gcc cannot constant-fold (e.g. keep the end
   pointer in a variable that is only read after an aliasing store) and re-measure.
// One entry of a command's tile list (D_800C2F4C[code]) that BattleBuildTileRows walks:
// the command to resolve, the sprite the package is drawn at, and the grey level its
// prims get. 0xFFFF in cmd ends the list.
typedef struct {
    // 0x0 u16 cmd; // 0xFFFF ends the list; 0x4000 and above selects one of the slot's four 0x2C0 entries
    // 0x2 s16 x;
    // 0x4 s16 y;
    // 0x6 u16 shade; // the level the prims are set to; 0x2000 and above gates it on the slot's command flag
} BattleTileRecord; // Size: 0x8

extern BattleTileRecord *D_800C2F4C[];
extern u8 *D_800C3000[];

// Draws one battle menu page: for each of the two tile-prim lists the page's table
// entry names, appends a sprite package per record of the command's tile list and
// sets the new prims' grey level, then returns the prim index the page started at.
// The no-rebuild path (colorShift set) only fades the prims already built.
s32 func_8008D598(u8 arg0, u8 arg1, u8 arg2) {
    u16 listIndex[2];
    u8 codes[2];
    BattleTileRecord *pRecords;
    u8 **pTableEntry;
    u8 *pCodes;
    u8 *pCodesStart;
    u16 *pLists;
    u16 *pListIndex;
    u16 cmd;
    u16 action;
    u16 shade;
    s32 count;
    s32 level;
    s32 rowOffset;
    s32 offset;
    s32 first;
    s32 i;
    s32 r;
    s32 j;

    if (arg2 != 0) {
        BattleFadeTilePrimRow();
        return g_BattleDisplayState->primStart;
    }
    pTableEntry = &D_800C3000[arg1];
    for (i = 0; i < 2; i++) {
        listIndex[i] = (*pTableEntry)[i];
        codes[i] = (*pTableEntry)[i + 2];
        g_BattleDisplayState->packageCounts[i] = 0;
    }
    rowOffset = arg0 << 6;
    offset = 0;
    pListIndex = listIndex;
    pLists = listIndex;
    pCodesStart = codes;
    for (pCodes = codes; pCodes < pCodesStart + 2; pCodes++, pLists++, offset += 2) {
        if (*pLists == 0xFF) {
            break;
        }
        pRecords = D_800C2F4C[*pCodes];
        for (r = 0, cmd = pRecords[0].cmd; cmd != 0xFFFF; r++, cmd = pRecords[r].cmd) {
            if (cmd >= 0x4000) {
                g_BattleCommandQueue[0x2C0 + (cmd & 3)] = g_BattleCommandQueue[rowOffset + (cmd & 0xFF)];
                action = g_BattleCommandQueue[rowOffset + (cmd & 0xFF)];
                if (action == 0xFF) {
                    continue;
                }
            } else {
                action = cmd;
            }
            count = g_BattleDisplayState->packageCounts[*pLists];
            g_BattleDisplayState->packageCounts[*pLists] += BattleDrawSpritePackage(
                action,
                (u8 *)(*pLists * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE + g_BattleGraphicsWork) +
                    count * sizeof(BattleSpritePackage),
                pRecords[r].x, pRecords[r].y);
            first = count * 2;
            shade = pRecords[r].shade;
            level = (shade & 0xFF) >> 1;
            if (shade >= 0x2000) {
                level = 0x10;
                if (((u16 *)(g_BattleCommandQueue + rowOffset + 0x1C))[shade & 0xF] == 0) {
                    level = (shade & 0xF0) >> 1;
                }
            }
            if (level != 0) {
                if (first < g_BattleDisplayState->packageCounts[*pLists] * 2) {
                    for (j = first; j < g_BattleDisplayState->packageCounts[listIndex[i]] * 2; j += 2) {
                        SetSemiTrans((POLY_FT4 *)(listIndex[i] * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                                  g_BattleGraphicsWork + (j + g_BattlePrimStartIndex) * sizeof(POLY_FT4)), 1);
                        SetShadeTex((POLY_FT4 *)(listIndex[i] * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                                 g_BattleGraphicsWork + (j + g_BattlePrimStartIndex) * sizeof(POLY_FT4)), 0);
                        ((POLY_FT4 *)((j + g_BattlePrimStartIndex) * sizeof(POLY_FT4) +
                                      (listIndex[i] * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                       g_BattleGraphicsWork)))->r0 = level;
                        ((POLY_FT4 *)((j + g_BattlePrimStartIndex) * sizeof(POLY_FT4) +
                                      (listIndex[i] * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                       g_BattleGraphicsWork)))->g0 = level;
                        ((POLY_FT4 *)((j + g_BattlePrimStartIndex) * sizeof(POLY_FT4) +
                                      (listIndex[i] * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                       g_BattleGraphicsWork)))->b0 = level;
                        ((POLY_FT4 *)((j + g_BattlePrimStartIndex) * sizeof(POLY_FT4) +
                                      (listIndex[i] * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                       g_BattleGraphicsWork)))->tpage |= 0x20;
                    }
                }
            } else {
                if (first < g_BattleDisplayState->packageCounts[*pLists] * 2) {
                    for (j = first; j < g_BattleDisplayState->packageCounts[listIndex[i]] * 2; j += 2) {
                        ((POLY_FT4 *)((j + g_BattlePrimStartIndex) * sizeof(POLY_FT4) +
                                      (listIndex[i] * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                       g_BattleGraphicsWork)))->r0 = 0x80;
                        ((POLY_FT4 *)((j + g_BattlePrimStartIndex) * sizeof(POLY_FT4) +
                                      (listIndex[i] * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                       g_BattleGraphicsWork)))->g0 = 0x80;
                        ((POLY_FT4 *)((j + g_BattlePrimStartIndex) * sizeof(POLY_FT4) +
                                      (listIndex[i] * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE +
                                       g_BattleGraphicsWork)))->b0 = 0x80;
                    }
                }
            }
        }
    }
    return g_BattlePrimStartIndex;
}
*/

// The per-slot draw block is a raw POLY_FT4 pool (0x28 stride) that
// BattleDrawSpriteQuad appends 0x50-byte sprite packages to; BattleSlotGraphics
// names the prims inside it, so the two views of the pool base are cast from it.
// The prim view is formed inside the loop on purpose: hoisting it above the four
// package calls gives it a second register to keep alive across them.
void BattleDrawSpriteQuad(u8 arg0, s32 arg1, s32 arg2, s32 arg3, u16 arg4) {
    u8 codes[BATTLE_SPRITE_PACKAGE_COUNT];
    BattleSlotGraphics *p;
    BattleSpritePackage *pPackage;
    POLY_FT4 *pPrims;
    s16 dx;
    s16 dy;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    if (g_BattleDisplayMode != 0) {
        codes[0] = 0x4A;
        codes[1] = 0x4C;
        codes[2] = 0x4F;
        codes[3] = 0x51;
    } else {
        codes[0] = 0xF0;
        codes[1] = 0xF2;
        codes[2] = 0xF5;
        codes[3] = 0xF7;
    }
    p->packageCount = 0;
    pPackage = (BattleSpritePackage *)p->primBuffer;
    p->packageCount = BattleDrawSpritePackage(codes[0], (u8 *)&pPackage[p->packageCount], (s16)arg1, (s16)arg2) + p->packageCount;
    dy = arg2; // the sum must go through dy: dy + arg4 keeps retail's addu operand order
    dx = arg1 + arg3 - 8;
    p->packageCount = BattleDrawSpritePackage(codes[1], (u8 *)&pPackage[p->packageCount], dx, (s16)dy) + p->packageCount;
    dy = (s16)(dy + arg4 - 8);
    p->packageCount = BattleDrawSpritePackage(codes[2], (u8 *)&pPackage[p->packageCount], (s16)arg1, dy) + p->packageCount;
    p->packageCount = BattleDrawSpritePackage(codes[3], (u8 *)&pPackage[p->packageCount], dx, dy) + p->packageCount;
    for (i = 0; i < BATTLE_SPRITE_PACKAGE_COUNT; i++) {
        pPrims = (POLY_FT4 *)p->primBuffer;
        BattleSetPrimGrey80BlendModeFromGlobal(&pPrims[i * 2 + g_BattlePrimStartIndex]);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008DE04);
/* 80.93% at retail's exact 1580 bytes (395 instructions; the 27 load-delay nops
   match). The tile-row builders (this and func_8008E430/EA70/F0A8) each write one
   row of BattleSlotGraphics' tilePrims[4][4]: quad 0 at [row][g_BattlePrimStartIndex]
   and quad 1 two prims later, i.e. a constant +0x50 displacement. That is why quad 1
   is spelled `(p->tilePrims[0] + 2)`: `p->tilePrims[0][idx + 2]` emits an addiu +2
   and loses the displacement. The index-first sum
   `(g_BattlePrimStartIndex + p->tilePrims[...])->field` is what makes gcc emit
   retail's `addu v0,v0,s1`; battle/misc3's BattleInitSlotPrims comments the same
   idiom. Every store being a volatile lvalue is the lever that took this body from
   65.30 to 80.93: with plain stores cc1 2.6.0 moves the next statement's volatile
   g_BattlePrimStartIndex load above the current store, which costs the whole
   interleaving and 14 load-delay nops (1528 against retail's 1580 bytes).
   Left: a pure three-register rotation - retail keeps left in a1, top in a0, mid in
   t0 and we get left a0, top t0, mid a1 - and retail's x1/x3 sum is
   (arg1 + 8) + half in one addu where ours reassociates to (half + 8) + arg1.
   Measured worse: an s16 `left` local (80.24), s16 locals for all five values
   (70.25 pre-volatile), the x1 store listed first (72.56), and the permuter from
   both bases (base 8640, nothing better in 4 minutes).
   ROUND 384: four more bodies, all worse - s32 locals for left/bottom/top/half/mid
   (75.99; cse2 hoists all five definitions above the first volatile store),
   `left = arg1 + 8` as a statement BEFORE the slot-graphics load (78.23 / 1576,
   and it does put left in retail's $a1 in the prologue), `left` early plus
   `arg2 + 0xFFF8` inline (80.01 / 1576 - the li/addu shape retail has, but bottom
   lands in $t0 and mid in $a0, the mirror of retail). None beats the body below,
   which stays the closest at 620-ish 1580 bytes; the residual is the STORE_HOIST
   class these four rows of row builders share (see func_8008E430's note).
*/
/*
void func_8008DE04(u8 arg0, s32 arg1, s32 arg2, u16 arg3) {
    BattleSlotGraphics *p;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x0 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y0 = arg2 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x1 = (arg1 + 8) + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y1 = arg2 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x2 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y2 = arg2 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x3 = (arg1 + 8) + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y3 = arg2 + 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u0 = (g_BattleGraphicsWork[0xA25C] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v0 = g_BattleGraphicsWork[0xA260];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u1 = ((g_BattleGraphicsWork[0xA25C] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v1 = g_BattleGraphicsWork[0xA260];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u2 = (g_BattleGraphicsWork[0xA25C] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v2 = g_BattleGraphicsWork[0xA260] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u3 = ((g_BattleGraphicsWork[0xA25C] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v3 = g_BattleGraphicsWork[0xA260] + 16;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x0 = (arg1 + 8) + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y0 = arg2 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x1 = (arg1 + 8) + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y1 = arg2 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x2 = (arg1 + 8) + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y2 = arg2 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x3 = (arg1 + 8) + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y3 = arg2 + 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u0 = (g_BattleGraphicsWork[0xA25C] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v0 = g_BattleGraphicsWork[0xA260];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u1 = ((g_BattleGraphicsWork[0xA25C] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v1 = g_BattleGraphicsWork[0xA260];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u2 = (g_BattleGraphicsWork[0xA25C] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v2 = g_BattleGraphicsWork[0xA260] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u3 = ((g_BattleGraphicsWork[0xA25C] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v3 = g_BattleGraphicsWork[0xA260] + 16;
    for (i = 0; i < BATTLE_TILE_QUAD_COUNT; i++) {
        BattleSetPrimGrey80BlendModeFromGlobal(&p->tilePrims[0][g_BattlePrimStartIndex + i * 2]);
    }
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008E430);
/* 98.39% at retail's exact 1600 bytes (98.20 with retail's own store order, x0 then
   y0). The only difference is where gcc puts each store's VALUE: retail computes it
   inside that store's block (addiu a1,a1,8 lands after the first index load, the arg4
   load and its sum at the y0 store), while ours hoists the value computations of later
   volatile stores above the first store. Listing y0 first moves `left` to retail's
   exact slot and gives the 98.39% body below, at the price of emitting the two stores
   in swapped order. Permuter: 1791 iterations from the 98.20 body found only that swap
   (base 580 -> 200); a 2129-iteration run from the 98.39 body found nothing.
   Load-bearing spellings: every store must be a volatile lvalue (otherwise cc1 lifts
   the next store's g_BattlePrimStartIndex load above this one and the 27 load-delay
   nops disappear); quad 1 must stay `(p->tilePrims[1] + 2)` so +0x50 is a
   displacement; the index-first sum `(g_BattlePrimStartIndex + p->tilePrims[1])` gives
   retail's addu v0,v0,s1; and `(left = arg1 + 8)` is an embedded assignment on purpose
   - it keeps left in $a1 as retail has it and stops gcc reassociating
   (arg1 + 8) + half into arg1 + (half + 8).
   Tried and worse: an s32 `left` statement before the stores (left, the arg3 mask and
   the arg4 load all hoist; 98.20 at 1604 bytes), a `top` statement local, a
   `*(volatile s32 *)&arg4` read (1612 bytes), gcc-2.7.2 (98.14).
   ROUND 300: the value hoist is cse2's invariant-expression hoist to the top of the
   basic block, and it is NOT source-spellable away: probed with tools/scripts/
   cc1_probe.sh on a 2-store replica (a `quad[1]` stand-in for tilePrims[1]) and the
   arg4 load + `addu` still hoist above the first store with the values inline, in an
   embedded assignment, in brace blocks, in `do { } while (0)` blocks (one per store
   and one for all), with a per-store scoped temporary, with an s16-dest cast, with
   array-index vs pointer-sum addressing, and under gcc-2.6.0, 2.7.2 and 2.7.2-cdk.
   The one lever that DOES keep the load in its own block is taking the argument's
   address (`s32 *pArg4; pArg4 = &arg4;` then `arg2 + *pArg4 - 8`): the escaped
   address stops the hoist - but gcc then narrows the read to `lhu` where retail has
   `lw` (same size, same place, still not 100%). A second datapoint: five locals
   (left/bottom/top/right/farRight, each assigned at its first store) give retail's
   EXACT register assignment - a1=left, t0=bottom, a0=right, a3=half, a2=top - for
   94.88% at 1600 bytes, so the values-as-locals structure is right and only their
   placement is wrong. `-O1` removes the hoist but rewrites everything else (77.44).
   ROUND 372: re-measured, still 98.39% at retail's exact 1600 bytes; the residual is
   only the order of the first two stores (retail emits the x0 `sh a1,488` before
   y0's `addu a2,a2,v1 / addiu t0,a2,-8 / sh t0,490`, the body below emits them
   swapped). Listing x0 first is retail's own order but measures worse, 98.20: the
   y0 value's `lw a0,48(sp) / addu a2,a2,a0` then hoists ABOVE the x0 store and
   `left` hoists above the g_BattlePrimStartIndex load (position 4; retail has 9)
   with the a3 mask following it - three displacements instead of two. So y0-first
   stays. The sibling row builder BattleBuildTileRow2Prims closed at 100.00 this round with an
   s32 `left` local + embedded assignment, which is the only shape that makes cc1
   emit retail's 32-bit `li/li a0,0xfff8 / addu` instead of `addiu -8`; the `left`
   here is already that shape and its placement is exact.
   ROUND 403: the fifth argument as a four-byte aggregate (the trick that closed
   BattleBuildTileRow3Prims, see BattleRowSize) does NOT help here: 98.20% at the same
   1600 bytes, i.e. exactly the plain-s32 x0-first body, because a WORD read is not a
   narrow `lhu` and the whole y0 group still hoists above the x0 store (`left`'s addiu
   to slot 4 where retail has 9, the a3 mask to 6, `lw 48(sp)` to 15 and its sum to 19
   where retail has 21/22). Byte-identical to that body, so do not retry: the same
   aggregate with the field's address taken (`s32 *pHeight = &arg4.height` - the read
   stays a hoisted `lw`), capturing the sum without the outer `- 8`, and making the x0
   store NON-volatile (the two stores still come out in source order, so their object
   order is not a scheduler choice - retail's source really does list x0 first, and the
   hoist is the only thing standing in the way). Preset closed for the whole unit as
   well: gcc-2.7.2-cdk gives this body 92.34% and drops every matched neighbour
   (BattleDrawSpriteQuad 84.90, BattleBuildTileRow2Prims 93.84, BattleInitSlotGraphics
   72.47), and 2.6.0 -O3 does not compile.
*/
/*
void func_8008E430(u8 arg0, s32 arg1, s32 arg2, u16 arg3, s32 arg4) {
    BattleSlotGraphics *p;
    s32 left;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y0 = arg2 + arg4 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x0 = (left = arg1 + 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x1 = left + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y1 = arg2 + arg4 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x2 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y2 = arg2 + arg4 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x3 = left + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y3 = arg2 + arg4 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x0 = left + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y0 = arg2 + arg4 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x1 = left + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y1 = arg2 + arg4 - 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x2 = left + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y2 = arg2 + arg4 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x3 = left + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y3 = arg2 + arg4 + 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 8;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) - 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 16;
    for (i = 0; i < BATTLE_TILE_QUAD_COUNT; i++) {
        BattleSetPrimGrey80BlendModeFromGlobal(&p->tilePrims[1][g_BattlePrimStartIndex + i * 2]);
    }
}
*/

void BattleBuildTileRow2Prims(u8 arg0, s32 arg1, s32 arg2, u16 arg3) {
    BattleSlotGraphics *p;
    s32 left;
    s32 top;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->x0 = (left = arg1 + 0xFFF8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->y0 = (top = arg2 + 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->x1 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->y1 = top;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->x2 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->y2 = top + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->x3 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->y3 = top + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->x0 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->y0 = top + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->x1 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->y1 = top + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->x2 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->y2 = top + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->x3 = arg1 + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->y3 = top + (arg3 - 16) / 2 + (arg3 - 16) / 2;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[2])->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[2] + 2))->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 2 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    for (i = 0; i < BATTLE_TILE_QUAD_COUNT; i++) {
        BattleSetPrimGrey80BlendModeFromGlobal(&p->tilePrims[2][g_BattlePrimStartIndex + i * 2]);
    }
}

// Builds the two greyscale quads of tile row 3, the row shifted to the right end of
// arg1..arg1 + arg3, at the slot's current prim index, then sets both quads' blend
// mode. The slot-graphics load comes first; `centre` is the row's right edge.
void BattleBuildTileRow3Prims(u8 arg0, s32 arg1, s32 arg2, s32 arg3, BattleRowSize arg4) {
    BattleSlotGraphics *p;
    s32 centre;
    s32 left;
    s32 top;
    s32 half;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    centre = arg1 + arg3;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->x0 = (left = centre - 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->y0 = (top = arg2 + 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->x1 = centre + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->y1 = top;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->x2 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->y2 = top + (half = (arg4.height - 16) / 2);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->x3 = centre + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->y3 = top + half;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->x0 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->y0 = top + half;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->x1 = centre + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->y1 = top + half;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->x2 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->y2 = top + half + half;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->x3 = centre + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->y3 = top + half + half;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[3])->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->u0 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->u2 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 14;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE] & 0x3F) << 1) + 30;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[3] + 2))->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + 3 * BATTLE_TILE_SOURCE_STRIDE + BATTLE_TILE_SOURCE_V] + 7;
    for (i = 0; i < BATTLE_TILE_QUAD_COUNT; i++) {
        BattleSetPrimGrey80BlendModeFromGlobal(&p->tilePrims[3][g_BattlePrimStartIndex + i * 2]);
    }
}

// Fills both greyscale quads' corners at the slot's current prim index, draws the
// slot's four sprite packages and records the prim index they were built at. The
// callees take u16 coordinates, so each argument is truncated once here.
/* 69.38% (532 bytes against retail's 528, 133 insns against 132) - one extra `move` per
   coordinate is the whole difference. Source shape, callee-saved assignment and every
   instruction offset are right; gcc 2.6.0 masks each u16 coordinate into a FRESH
   register (`move t3,t1; andi s4,t3,0xffff`) where retail masks its callee-saved copy
   in place (`move s1,t0; andi s1,s1,0xffff`), i.e. retail keeps 2 pseudos per
   coordinate (raw for the x+w / y+h sums, masked for the eight stores and the five
   call argument sets) and this C gets 3. That copy per coordinate drops the whole
   callee-saved assignment (retail p=s2 index=s3 x=s1 y=s0 w=s5 h=s4; here p=s0
   index=s1 x=s4 y=s3 w=s5 h=s2) and is the extra 4 bytes.
   From the -da/-dl dumps (cc1 -O2 -G8 ... -da on build/src/battle/main/misc16.c.i,
   greg/lreg), the allocno priorities of the six are p 1.45 (refs 10 / live 82),
   index 1.13 (9/95), h 1.60 (5/24), y 0.59 (6/82), x 0.57 (6/84), w 0.53 (5/75) and
   find_reg hands out $s0..$s5 along that order, so the numbers are close enough that
   the copy, not a tie, is what has to go.
   Measured and no better: u16 x/y/w/h locals assigned from the params and used for
   both the stores and the call arguments (byte-identical output - the copies
   coalesce); the `(g_BattlePrimStartIndex + p->prims)` index-first sum idiom this
   unit's tile-row builders use (identical output); sums as right/bottom locals (516);
   s32 params (508); p loaded after the 0xB0 store (61.88%); u32/u8 index; a 4-minute
   permuter run (540 bytes, 73.98%).
   The call to BattleBuildTileRow3Prims below passes its h as a BattleRowSize aggregate, not the
   bare value, because that function's retail object reads h from the caller's frame:
   declare a BattleRowSize, set its height to h, and pass that.
*/
/*
void func_8008F6E4(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4) {
    BattleSlotGraphics *p;
    u8 index;

    index = arg0;
    p = g_BattleSlotGraphics[index];
    ((u8 *)g_BattleDisplayState)[index + 0xB0] = 0;
    p->prims[g_BattlePrimStartIndex].x0 = arg1;
    p->prims[g_BattlePrimStartIndex].y0 = arg2;
    p->prims[g_BattlePrimStartIndex].x1 = arg1 + arg3;
    p->prims[g_BattlePrimStartIndex].y1 = arg2;
    p->prims[g_BattlePrimStartIndex].x2 = arg1;
    p->prims[g_BattlePrimStartIndex].y2 = arg2 + arg4;
    p->prims[g_BattlePrimStartIndex].x3 = arg1 + arg3;
    p->prims[g_BattlePrimStartIndex].y3 = arg2 + arg4;
    BattleDrawSpriteQuad(index, arg1, arg2, arg3, arg4);
    func_8008DE04(index, arg1, arg2, arg3);
    func_8008E430(index, arg1, arg2, arg3, arg4);
    BattleBuildTileRow2Prims(index, arg1, arg2, arg4);
    BattleBuildTileRow3Prims(index, arg1, arg2, arg3, arg4);
    p->packagePrimStart = *(u8 *)&g_BattlePrimStartIndex;
    ((u8 *)g_BattleDisplayState)[index + 0xB0] = 1;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F6E4);

// Allocates a slot's draw block and rect list on first use, then either fills the
// rect list from the four coordinates and marks the slot dirty (arg5) or hands
// them to func_8008F6E4 and advances the frame (arg6).
void BattleInitSlotGraphics(s32 arg0, s32 arg1, s32 arg2, s32 arg3, u16 arg4, u8 arg5, u8 arg6) {
    u16 slot;
    u8 index;
    BattleSlotRect *pRect;
    void *q;

    slot = arg0;
    index = slot; // two steps: the u16 copy is what puts arg0 in s2, the u8 index in s1
    if (g_BattleDisplayState->primFlags[index] == 0) {
        q = BattleAllocHeapBlock(sizeof(BattleSlotGraphics), 0);
        g_BattleSlotGraphics[index] = q;
        bzero(q, sizeof(BattleSlotGraphics));
        q = BattleAllocHeapBlock(sizeof(BattleSlotRect), 0);
        g_BattleSlotRects[index] = q;
        bzero(q, sizeof(BattleSlotRect));
        BattleInitSlotPrims(index);
    }
    if (arg5 != 0) {
        pRect = g_BattleSlotRects[index];
        pRect->slot = arg0;
        pRect->x = arg1;
        pRect->y = arg2;
        pRect->targetW = arg3;
        pRect->targetH = arg4;
        pRect->w = 0;
        pRect->h = 0;
        g_BattleDisplayState->rectExpandDone[index] = 0;
        g_BattleDisplayState->rectExpandActive[index] = 1;
    } else {
        func_8008F6E4(index, arg1, arg2, arg3, arg4);
        if (arg6 != 0) {
            BattleUpdateFrame();
        }
    }
}
