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

// The POLY_FT4 prims one sprite package holds (BattleSpritePackage.prims): a package's
// index times this is its first prim in the draw block, a package count times this the
// prims those packages filled.
#define BATTLE_SPRITE_PACKAGE_PRIM_COUNT 2

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
// The commands a slot can be given, one per status condition BattleTryUseSlotCommand
// gates on it: statusMasks, codes and damage hold one entry each, the menu's cursor
// walks all of them, and a slot's text index maps to the first of its command codes.
extern u8 g_BattleActionRows[];
extern u8 D_800CEF34[];

// One sprite package of the per-slot draw block: the two POLY_FT4
// BattleDrawSpritePackage writes, i.e. the 0x50-byte stride the block's package
// counter advances by.
typedef struct {
    /* 0x00 */ POLY_FT4 prims[BATTLE_SPRITE_PACKAGE_PRIM_COUNT];
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
extern void BattleBuildTileRow0Prims(u8, s32, s32, u16);
extern void BattleBuildTileRow1Prims(u8, s32, s32, u16, s32);
extern void BattleBuildTileRow2Prims(u8, s32, s32, u16);
// The fifth argument of BattleBuildTileRow3Prims arrives as an aggregate, not a bare
// u16: gcc 2.6.0 loads a narrow stack parameter into a register in the function
// prologue, while an aggregate parameter stays in the caller's frame, and the
// original reads it with `lhu` at the use (BattleBuildTileRow3Prims's note).
typedef struct {
    /* 0x0 */ u16 height; // the row's height: every y is inset by 8, then advanced by (height - 16) / 2
    /* 0x2 */ u16 unk2;   // unread by this unit
} BattleRowSize; // Size: 0x4
extern void BattleBuildTileRow3Prims(u8, s32, s32, s32, BattleRowSize);
extern void func_80090310(u8, u8);
extern void BattleDrawCommandCellIcons(s32, s32);
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
    u16 statusMasks[BATTLE_SLOT_COMMAND_COUNT];
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
    idx = (arg1 + BATTLE_ACTION_ROW_FIRST) * sizeof(BattleRow) + arg0 * BATTLE_ACTION_ROW_STRIDE;
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
    u8 codes[BATTLE_SLOT_COMMAND_COUNT];
    u16 damage[BATTLE_SLOT_COMMAND_COUNT];
    u16 statusMasks[BATTLE_SLOT_COMMAND_COUNT];
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
    for (i = 0; i < BATTLE_SLOT_COMMAND_COUNT; i++) {
        codes[i] = 0xFF;
        damage[i] = 0;
    }
    for (i = 0; i < (limit = BATTLE_SLOT_COMMAND_COUNT); i++) {
        if ((u16)BattleMaskHasStatusBit((u16)g_GameState.conditionRows[g_BattleActorCharIds[arg0]].statusFlags, i) != 0 &&
            (g_BattleSlotStatusRows[arg0].flags & statusMasks[i]) == 0) {
            codes[i] = g_BattleSlotTextIndex[arg0 * 0x170] * BATTLE_SLOT_COMMAND_COUNT + i;
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
                cursor = BATTLE_SLOT_COMMAND_COUNT - 1;
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
            for (i = 0; i < g_BattleDisplayState->packageCounts[j] * BATTLE_SPRITE_PACKAGE_PRIM_COUNT; i += BATTLE_SPRITE_PACKAGE_PRIM_COUNT) {
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

// One entry of a command's tile list (D_800C2F4C[code]) that this page builder walks:
// the command to resolve, the sprite the package is drawn at, and the grey level its
// prims get. 0xFFFF in cmd ends the list.
typedef struct {
    /* 0x0 */ u16 cmd;   // 0xFFFF ends the list; 0x4000 and above selects one of the slot's 0x2C0 entries
    /* 0x2 */ s16 x;
    /* 0x4 */ s16 y;
    /* 0x6 */ u16 shade; // the level the prims are set to; 0x2000 and above gates it on the slot's command flag
} BattleTileRecord; // Size: 0x8

extern BattleTileRecord *D_800C2F4C[];
extern u8 *D_800C3000[];

// 86.41% at 1664 bytes against retail's 1692 (416 vs 423 instructions); up from the
// previous park's 71.88, unchanged this round after five scored bodies.
// 
// THE WHOLE 7-INSTRUCTION GAP IS THE OUTER-LOOP TAIL AND THE PREHEADER SPILL GROUP.
// Retail keeps four derived pointers in stack slots and walks them in memory - the
// tail is `lw t1,0x38(sp); addiu t1,t1,1; sw t1,0x38(sp); lw t1,0x40(sp);
// addiu t1,t1,2; sw t1,0x40(sp); lw t1,0x30(sp); addiu v0,t1,2; lw t1,0x38(sp);
// slt; bnez; addiu s7,s7,2` (14 insns) against this body's 8-instruction register
// tail, and the preheader spills a matching group at 0x28/0x30/0x38/0x40. Retail's
// `addiu s7,s7,2` is a strength-reduced i*2 and its record walk re-reads
// D_800C2F4C[codes[i]] three times per record, so retail's source IS the index loop
// and the re-fetch spelling this body already writes: the divergence is gcc's loop
// strength reduction and register handout, not a spelling. Retail spends all nine
// callee-saved registers on the j-loop (s5 holds the ADDRESS of
// g_BattlePrimStartIndex, hoisted before the loop, where this body re-forms lui/lw
// at each of three uses); this body spends two of them on the cursors. Those two
// live ranges win the register on live-length x depth, so the fix is to change what
// is live, not how it is spelled.
// 
// Measured this round, all no better than 86.41:
// - the explicit pointer walk (u16 *pList / u8 *pCode cursors, `pCode < codes + 2`
//   as the loop test, `*pList` / `*pCode` at every use, init loop walking the same
//   two cursors): 78.82 at 1540 bytes / 385 insns - the cursors then DO get
//   callee-saved registers and the body loses the loop's other live values.
// - `first = count * 2` moved above the BattleDrawSpritePackage call (retail computes
//   it in that call's delay slot and never needs `count` afterwards, so it has no
//   spill pair; this body's `first` after the call costs `sw t0,64(sp)` plus a
//   reload): 85.84 at 1664 / 416 - the gain is lost because the constant 0x80 of the
//   grey loop is then hoisted all the way to the FUNCTION preheader into a
//   caller-saved register and spilled/reloaded around both loop calls, ~4 insns
//   retail does not have (retail materialises it with `ori a3,zero,0x80` at the grey
//   loop). Killing that hoist is the next thing to try, not the tail.
// - reading records[r].shade a second time in the 0x2000 compare (retail has two
//   adjacent `lhu a0,6(s0)` / `lhu v0,6(s0)`) together with
//   `*(u16 *)(g_BattleCommandQueue + rowOffset + (shade & 0xF) * 2 + 0x1C)` for
//   retail's queue-plus-rowOffset-first `addu v0,v0,t1`: also 85.84 at 1664 / 416.
// - already known-good and not to be changed: the re-fetch of D_800C2F4C at every
//   use (a cached BattleTileRecord *pRecords is 78.35 at 1720), cmd s32, shade u32,
//   action u32, and the record offset r*1 with addiu +8 versus retail's r*2 (same
//   eight bytes).
/*
s32 func_8008D598(u8 arg0, u8 arg1, u8 arg2) {
    u16 listIndex[2];
    u8 codes[2];
    s32 rowOffset;
    u8 **pTableEntry;
    s32 cmd;
    u32 action;
    u32 shade;
    s32 count;
    s32 level;
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
    for (i = 0; i < 2; i++) {
        if (listIndex[i] == 0xFF) {
            break;
        }
        for (r = 0, cmd = D_800C2F4C[codes[i]][0].cmd; cmd != 0xFFFF; r++, cmd = D_800C2F4C[codes[i]][r].cmd) {
            if (cmd >= 0x4000) {
                g_BattleCommandQueue[0x2C0 + (cmd & 3)] = g_BattleCommandQueue[rowOffset + (cmd & 0xFF)];
                action = g_BattleCommandQueue[rowOffset + (cmd & 0xFF)];
                if (action == 0xFF) {
                    continue;
                }
            } else {
                action = cmd;
            }
            count = g_BattleDisplayState->packageCounts[listIndex[i]];
            g_BattleDisplayState->packageCounts[listIndex[i]] += BattleDrawSpritePackage(
                action,
                (u8 *)(listIndex[i] * BATTLE_TILE_PRIM_LIST_STRIDE + BATTLE_TILE_PRIM_LIST_BASE + g_BattleGraphicsWork) +
                    count * sizeof(BattleSpritePackage),
                D_800C2F4C[codes[i]][r].x, D_800C2F4C[codes[i]][r].y);
            first = count * 2;
            shade = D_800C2F4C[codes[i]][r].shade;
            level = (shade & 0xFF) >> 1;
            if (shade >= 0x2000) {
                level = 0x10;
                if (((u16 *)(g_BattleCommandQueue + rowOffset + 0x1C))[shade & 0xF] == 0) {
                    level = (shade & 0xF0) >> 1;
                }
            }
            if (level != 0) {
                if (first < g_BattleDisplayState->packageCounts[listIndex[i]] * 2) {
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
                if (first < g_BattleDisplayState->packageCounts[listIndex[i]] * 2) {
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
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D598);


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
        BattleSetPrimGrey80BlendModeFromGlobal(&pPrims[i * BATTLE_SPRITE_PACKAGE_PRIM_COUNT + g_BattlePrimStartIndex]);
    }
}

// Fills tile row 0's two greyscale quads at the slot's current prim index: both
// quads' corners, then both quads' texcoords, then the blend mode. Every store is a
// volatile lvalue (otherwise cc1 2.6.0 lifts the next store's g_BattlePrimStartIndex
// load above this one and the load-delay nops disappear), quad 1 is
// `(p->tilePrims[0] + 2)` so its +0x50 offset stays a displacement, and
// `(g_BattlePrimStartIndex + p->tilePrims[0])` is index-first for retail's addu
// operand order. The embedded assignments are load-bearing: a plain `arg2 - 8` folds
// to `addiu`, `(bottom = arg2 + 0xFFF8)` gives retail's `li 0xfff8 / addu`, and the
// named half/mid/right locals stop gcc reassociating left+half into arg1+(half+8).
void BattleBuildTileRow0Prims(u8 arg0, s32 arg1, s32 arg2, u16 arg3) {
    BattleSlotGraphics *p;
    s32 left;
    s32 bottom;
    s32 top;
    s32 half;
    s32 mid;
    s32 right;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x0 = (left = arg1 + 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y0 = (bottom = arg2 + 0xFFF8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x1 = (mid = left + (half = (arg3 - 16) / 2));
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y1 = bottom;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x2 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y2 = (top = arg2 + 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->x3 = mid;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->y3 = top;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x0 = mid;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y0 = bottom;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x1 = (right = mid + half);
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y1 = bottom;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x2 = mid;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y2 = top;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->x3 = right;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->y3 = top;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u0 = (g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u2 = (g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_V] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + p->tilePrims[0])->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_V] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u0 = (g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v0 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u1 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v1 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_V];
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u2 = (g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE] & 0x3F) << 1;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v2 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_V] + 16;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->u3 = ((g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE] & 0x3F) << 1) + 7;
    *(volatile u8 *)&(g_BattlePrimStartIndex + (p->tilePrims[0] + 2))->v3 = g_BattleGraphicsWork[BATTLE_TILE_SOURCE_BASE + BATTLE_TILE_SOURCE_V] + 16;
    for (i = 0; i < BATTLE_TILE_QUAD_COUNT; i++) {
        BattleSetPrimGrey80BlendModeFromGlobal(&p->tilePrims[0][g_BattlePrimStartIndex + i * 2]);
    }
}

// Builds the two greyscale quads of tile row 1 at the slot's current prim index, then
// sets both quads' blend mode. Every store is a volatile lvalue (otherwise cc1 lifts the
// next store's g_BattlePrimStartIndex load above this one and the load-delay nops vanish),
// and the five values are locals assigned at their first store: that is what gives retail's
// register set (a1=left, a0=right, a3=half, a2=top, t0=bottom). `pArg4 = &arg4` is
// load-bearing: without the escaped address cc1 hoists the y0 value above the x0 store
// (98.20%); with it the stores keep retail's order and the block matches.
void BattleBuildTileRow1Prims(u8 arg0, s32 arg1, s32 arg2, u16 arg3, s32 arg4) {
    BattleSlotGraphics *p;
    s32 left;
    s32 top;
    s32 bottom;
    s32 half;
    s32 right;
    s32 *pArg4;
    s32 i;

    p = g_BattleSlotGraphics[arg0];
    pArg4 = &arg4;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x0 = (left = arg1 + 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y0 = (bottom = (top = arg2 + *pArg4) - 8);
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x1 = (right = left + (half = (arg3 - 16) / 2));
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y1 = bottom;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x2 = left;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y2 = top + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->x3 = right;
    *(volatile s16 *)&(g_BattlePrimStartIndex + p->tilePrims[1])->y3 = top + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x0 = right;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y0 = bottom;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x1 = right + half;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y1 = bottom;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x2 = right;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y2 = top + 8;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->x3 = right + half;
    *(volatile s16 *)&(g_BattlePrimStartIndex + (p->tilePrims[1] + 2))->y3 = top + 8;
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
// callees take wider-than-u16 coordinates, so each argument is truncated once here.
/* 68.50% at 540 bytes against retail's 528 (135 instructions against 132), up from
   the previous park's 65.12%. Two measured wins over that body:
   (1) retail forms the quads' right/bottom edges ONCE, early - `addu t0,t0,s5` sits
   in the prologue and `addu t1,t1,s4` just before the third store - so `right` and
   `bottom` are u16 locals assigned from the params first; sums inline in the stores
   cost 65.12 -> 68.42.
   (2) the store address is spelled index-first, `(g_BattlePrimStartIndex + p->prims)`,
   which makes the final `addu` retail's `addu v0,v0,s2` (index term + base) instead of
   `addu v0,s1,v0`; 68.42 -> 68.50. Same spelling as the four BattleBuildTileRow*Prims.
   THE REMAINING GAP, named: retail masks each coordinate IN PLACE on its callee-saved
   copy (`andi s5,s5,0xffff` then `move a3,s5`), interleaved among the eight stores in
   the order w,h,y,x, so it needs six callee-saved registers (s0=y s1=x s2=p s3=index
   s4=h s5=w) and no copy. gcc here moves the whole first call's argument setup to the
   TOP of the function (four masks in x,y,w,h order plus `move a0/a1/a2/a3`), which
   keeps the raw params live, so every narrowed value becomes a second pseudo: seven
   callee-saved registers (s6 for the masked x), one extra save/restore pair, and the
   third extra instruction is the `sh zero,18(sp)` half of the BattleRowSize copy. That
   hoist is the one thing no spelling tried changes.
   Measured and no better: u16 x/y/w/h locals fed from the params (byte-identical);
   s32 right/bottom; all four coordinates left inline in the stores; a 4-minute
   permuter run (2479 iterations) whose best edit wraps the last four stores and the
   five calls in a do/while(0) - +0.18% only, not ported.
   BattleRowSize as a UNION {u32 word; u16 height;} (so the caller can pass one `sw`
   like retail's `sw s4,0x10(sp)`) gives exactly 132 instructions and 528 bytes, but
   with a frame of 64 against retail's 56, because constructing the aggregate
   materialises a temp in the frame - and it drops BattleBuildTileRow3Prims to 99.20%,
   the union being SImode, whose `lhu a0,0x10(sp)` at the use becomes `lw` at entry
   plus `andi`. Only a 4-byte, align-2 parameter keeps that callee at 100.00%, and it
   always costs two `sh` here, so the two shapes are incompatible (the previous park
   note found the same wall from the other side).
   To resume: find why gcc 2.6.0 hoists this first call's register-argument setup to
   the entry block when retail keeps it at the call site; the rest of the diff is that
   hoist and the register assignment it forces.
*/

/*
void func_8008F6E4(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4) {
    BattleSlotGraphics *p;
    u16 x;
    u16 y;
    u16 w;
    u16 h;
    u16 right;
    u16 bottom;
    u8 index;

    x = arg1;
    y = arg2;
    w = arg3;
    h = arg4;
    right = x + w;
    bottom = y + h;
    index = arg0;
    p = g_BattleSlotGraphics[index];
    ((u8 *)g_BattleDisplayState)[index + 0xB0] = 0;
    (g_BattlePrimStartIndex + p->prims)->x0 = x;
    (g_BattlePrimStartIndex + p->prims)->y0 = y;
    (g_BattlePrimStartIndex + p->prims)->x1 = right;
    (g_BattlePrimStartIndex + p->prims)->y1 = y;
    (g_BattlePrimStartIndex + p->prims)->x2 = x;
    (g_BattlePrimStartIndex + p->prims)->y2 = bottom;
    (g_BattlePrimStartIndex + p->prims)->x3 = right;
    (g_BattlePrimStartIndex + p->prims)->y3 = bottom;
    BattleDrawSpriteQuad(index, x, y, w, h);
    BattleBuildTileRow0Prims(index, x, y, w);
    BattleBuildTileRow1Prims(index, x, y, w, h);
    BattleBuildTileRow2Prims(index, x, y, h);
    BattleBuildTileRow3Prims(index, x, y, w, (BattleRowSize){h, 0});
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
