#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/text_box.h"
#include "field/effects.h"
#include "system/memory.h"
#include "system/archive.h"
#include "system/sound.h"
#include "psyq/libetc.h"
#include "psyq/libcd.h"
#include "system/controller.h"

// Frame counter shared by every text box: it cycles 0..4 and picks the frame
// rect the cursor and the continue arrow are drawn with.
extern s32 g_FieldTextBoxFrameIndex;
extern RECT g_FieldCursorFrameRects[5];
extern RECT g_FieldContinueArrowFrameRects[5];
extern s16 g_FieldTextScaleDivisor;
extern int SystemStringGetColumnOffset(void* pString);
extern int SystemStringGetRowOffset(void* pString);

// Same SystemString helpers as the group further down; the text box keeps its
// string at _pad[0].
extern void SystemStringSetLineOffset(void* pString, int value);
extern void SystemStringClearLineOffset(void* pString);

// Field's own pad button state, filled by FieldPollControllers.
extern u_short g_FieldButtonsPressedOnce;

// The clip rects of the sixteen draw modes below, and the halfword view of the
// same table the window code seeds with x and w.
extern RECT D_800AFC80[];
extern short D_800AFC84[];
// Two render contexts of sixteen draw modes each.
extern DR_MODE D_800B1DF4[2][0x10];
extern int g_FieldTextBoxAllocationStates[4];

// Cursor up/down input for the text box holding the top message; moves the
// cursor within maxPosition and forwards the line offset to the string.
void func_8007DCF8(int index, u_long* pOt, int ctx) {
    if (g_FieldTextBoxes[index].cursor.visibility == 0) {
        if (g_FieldTextBoxes[index].windowOpenTimer == 0) {
            if (g_FieldTextBoxes[index].order == 0) {
                if (g_FieldButtonsPressedOnce & CTRL_BTN_DOWN) {
                    g_FieldTextBoxes[index].cursor.curPosition++;
                    if (g_FieldTextBoxes[index].cursor.curPosition > g_FieldTextBoxes[index].cursor.maxPosition - 1) {
                        g_FieldTextBoxes[index].cursor.curPosition = 0;
                    }
                }
                if (g_FieldButtonsPressedOnce & CTRL_BTN_UP) {
                    g_FieldTextBoxes[index].cursor.curPosition--;
                    if (g_FieldTextBoxes[index].cursor.curPosition < 0) {
                        g_FieldTextBoxes[index].cursor.curPosition = g_FieldTextBoxes[index].cursor.maxPosition - 1;
                    }
                }
                SystemStringSetLineOffset(&g_FieldTextBoxes[index]._pad[0],
                    g_FieldTextBoxes[index].cursor.curPosition + g_FieldTextBoxes[index].cursor.startPosition);
            } else {
                SystemStringClearLineOffset(&g_FieldTextBoxes[index]._pad[0]);
            }
        }
    }
}

// PARKED 97.46% (round 534): 588 bytes against retail's 584, 147 insns against
// 146. Everything except the preheader of the second rect family is
// instruction-identical: the y/h stores ride a base RECOMPUTED each iteration
// (sll + lui/addiu + addu) that is also the SetDrawMode clip-rect argument, the
// eight stores and their order, both GetTPage/SetDrawMode pairs, the latch
// increments, the 4-box loop and the frame (0x48) all match.
// The one residual is the AFC84 family's anchor: gcc anchors on the x giv
// (`la v0, D_800AFC80; addiu s3, v0, 4; move s2, v0` = 4 insns) where retail
// anchors on the +0 (w) giv (`la s2, D_800AFC84; addiu s3, s2, -4` = 3) and
// swaps s2/s3, so the only instruction surplus is that `move`. The object names
// D_800AFC80 for the anchor because gas re-anchors `D_800AFC84 - 4` to the label
// at that address, so both spellings link to the same bytes.
// Measured this round, best first: this body 97.46 / 588 / 147; `pRect->x` for
// the x store (drops the x giv) 92.06 / 576 / 144 (two short - the x giv is
// real); all four stores through the D_800AFC84 halfword view with no pRect
// 92.89 / 604 / 151; one `D_800AFC80[]` RECT array for all four 87.63 / 608 /
// 152; the old two-symbol form 82.05 / 624 / 156.
// Load-bearing spellings, each measured up: (1) the y/h/x stores must go through
// a `RECT*` local assigned in the body, not through `D_800AFC80[i].y` - the
// local is what keeps that base recomputed and shared with the call argument
// (the indexed form makes gcc fold +2/+6 into two more induction registers);
// (2) x and w must come from the halfword view of D_800AFC84 (`[i * 4 - 2]` /
// `[i * 4]`), which is what produces retail's two stepping pointers; (3) the
// second draw mode is `&D_800B1DF4[1][i]` on the [2][0x10] array, i.e. base +
// 0xC0, not a second symbol.
// Not tried (no budget left): anything that makes gcc pick the +0 giv as the
// anchor instead of the -4 one; if that can be had, the body closes.
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", FieldTextBoxInitialize);
/*
void FieldTextBoxInitialize(void) {
    int i;
    RECT rect;
    RECT* pRect;

    rect.y = 0;
    rect.x = 0;
    rect.h = 0xFF;
    rect.w = 0xFF;

    for (i = 0; i < 16; i++) {
        pRect = &D_800AFC80[i];
        pRect->y = 0;
        D_800AFC84[i * 4 - 2] = 0;
        pRect->h = 0xFF;
        D_800AFC84[i * 4] = 0xFF;
        pRect->y = 0;
        D_800AFC84[i * 4 - 2] = 0;
        pRect->h = 0xFF;
        D_800AFC84[i * 4] = 0xFF;
        SetDrawMode(&D_800B1DF4[0][i], NULL, 0, GetTPage(0, 0, 0x380, 0x100), pRect);
        SetDrawMode(&D_800B1DF4[1][i], NULL, 0, GetTPage(0, 0, 0x380, 0x100), pRect);
    }

    for (i = 0; i < 4; i++) {
        g_FieldTextBoxes[i].ownerActorID = 0xFF;
        g_FieldTextBoxes[i].talkingActorID = 0xFF;
        g_FieldTextBoxes[i].cursor.visibility = -1;
        g_FieldTextBoxes[i].continueArrow.visibility = -1;
        g_FieldTextBoxes[i].visibility = -1;
        g_FieldTextBoxes[i].status = -1;
        g_FieldTextBoxes[i].order = TEXT_BOX_UNINITIALIZED;
        g_FieldTextBoxes[i].ownerActorID = 0xFF;
        g_FieldTextBoxes[i].unk412 = 0;

        FieldTextBoxInitializePrimitives(i);
        g_FieldTextBoxAllocationStates[i] = -1;
        SetDrawMode(&g_FieldTextBoxes[i].drawModes[0], NULL, 0, GetTPage(0, 0, 0x300, 0x100), &rect);
        SetDrawMode(&g_FieldTextBoxes[i].drawModes[1], NULL, 0, GetTPage(0, 0, 0x300, 0x100), &rect);
    }
}
*/

// _pad[0x4A] is possibly a RECT?
void FieldTextBoxSetRect(int index, int arg1, int arg2, int arg3, s32 arg4) {
    g_FieldTextBoxes[index]._pad[0x4A] = arg1;
    g_FieldTextBoxes[index]._pad[0x4B] = arg2;
    g_FieldTextBoxes[index]._pad[0x4C] = arg3;
    g_FieldTextBoxes[index]._pad[0x4D] = arg4;
}

// Place a portrait quad: x - 1 and x + width when facing right, mirrored when
// flipped.
void FieldTextBoxSetPortraitQuad(POLY_FT4* pPoly, int x, int y, int width, int height, int flip) {
    if (!flip) {
        x--;
        pPoly->x0 = x;
        pPoly->x1 = x + width;
        pPoly->x2 = x;
        pPoly->x3 = x + width;
    } else {
        pPoly->x1 = x;
        pPoly->x0 = x + width;
        pPoly->x3 = x;
        pPoly->x2 = x + width;
    }
    pPoly->y0 = y;
    pPoly->y1 = y;
    pPoly->y2 = y + height;
    pPoly->y3 = y + height;
}

// PARKED 68.38% (re-measured): 3228 bytes against retail's 3148, 807
// instructions against 787. The structural pass is right -- every call,
// branch, store and reloc, and the whole open animation, frame, portrait,
// cursor and tile pass come out instruction-identical; the diff is a register
// cascade. Two residuals, both measured:
//  1. The box stride `index * 0x498` is computed into $t0 here and into $a2 in
//     retail at the same site (instruction 11, retail's dead incoming index
//     register), and retail rematerialises it at the top of every region: 15
//     chain sites against this body's 14. Same local-alloc contest for a dead
//     caller-saved register.
//  2. The frame is 0x78 against retail's 0xA8. Every used local already sits at
//     retail's offsets (rect 0x18, ctx 0x20, x 0x28, y 0x30, w 0x38, h 0x40);
//     retail's extra 56 bytes are DEAD slots at 0x48..0x7F, i.e. ABOVE the used
//     locals, and the stub has no `$sp` reference above 0x43 at all. They are
//     reload spill slots -- an expand-time slot moves them all up by its size,
//     which is what the dead-local experiments below show -- so they come from
//     pseudos retail spilled and then rematerialised, consistent with residual
//     1. Not reachable from source; see tools/rounds/levers.md
//     [j-an-expand-time-slot-is-always-the-lowest].
// Measured and dead: a dead `u_long scratch[12]` declared first gives retail's
// exact 0xA8 frame and save offsets but pushes ctx to 0x50 and every used local
// up by 0x30 (68.34%, 750 differing rows); declared last it lays out the same
// way; `scratch[14]` first overshoots to 0xB0. The frame is worth 2 rows, the
// register cascade is the rest of the diff.
// From earlier rounds, still valid: the preset is right (2.7.2 -O2 -G8 68.3,
// 2.6.0 67.4, -O3 63.2, -O1 35.7, cdk 54.4); s16 locals for x/y/w/h are worse
// (61.04%, re-narrowed with lhu + sll/sra at every use, which also settles that
// they are s32); the sprite stores match, so no store order is left to fix.
// The frame sprites and their draw modes are two render contexts laid out one
// after the other in FieldTextBoxBorders, so piece n of a context is
// sprites1[ctx * 10 + n] / drawModes1[ctx * 10 + n].
// g_FieldCursorFrameRects, g_FieldContinueArrowFrameRects and
// g_FieldTextBoxFrameIndex live inside g_FieldTableQuadTexParams and are
// declared as externs at the top of this file.
// PARKED 81.03% (round 462): 3236 bytes against retail 3148, 809 instructions against 787.
// Two source-order fixes bought +12.6 points from the round-458 park (68.38) with no
// structural change, both found by reading the ORIGINAL's store/branch order, not the
// disassembler's:
//  1. The eight frame sprites are stored VALUE-GROUPED, not piece-grouped. The original
//     writes x-8 to pieces 0,4,3, then x+w-8 to 2,6,1 (with 3.x0 = x-8 in between), then
//     y+h-9 to 4,6, y+9 to 3,1, then pieceHeight to 3,1, then y-7/x+8 to 5,7 and finally
//     w-0x10 to 5,7. pieceHeight is computed at the TOP of the block and clamped just
//     before its two h stores; pieceWidth is not a local at all (w-0x10 is written twice
//     and cse folds it). 68.38 -> 79.49.
//  2. The continue-arrow timer test is written as if (timer == 0) { draw } else { timer--; }
//     (the original's bnez jumps to an out-of-line decrement and the draw falls through).
//     Same semantics, 79.49 -> 81.03.
// Residual, unchanged and measured again today: the box stride index*0x498 is computed
// into $t0 here and $a2 in the original at the same site, and every temp below shifts by
// one register because of it (ours t7/t8/t1 where retail has t8/t9/t0); the frame is 0x78
// against retail's 0xA8 with retail's 56 bytes at 0x48..0x7F untouched by any instruction
// (no sp reference above 0x43), while every used local is already at retail's offset
// (rect 0x18, ctx 0x20, x 0x28, y 0x30, w 0x38, h 0x40) - so the hole is above the spills
// and no dead local reaches it (levers.md j-an-expand-time-slot-is-always-the-lowest and
// j-the-frame-and-the-slot-offsets both record this function).
// Also ruled out today: m2c_draft.sh emits the same statement order as this body (so no
// block is missing), and deleting the now-unused pieceWidth local changes nothing.
/*
void func_8007E1C0(u_long* pOt, int ctx, int index, u_long primAddr) {
    s32 x;
    s32 y;
    s32 w;
    s32 h;
    s32 scale;
    s32 shrink;
    s32 pieceHeight;
    s32 portraitX;
    s32 portraitWidth;
    s32 portraitHeight;
    s32 cursorX;
    s32 colOffset;
    s32 rowOffset;
    RECT rect;
    int i;

    if (g_FieldTextBoxes[index].visibility != 0) {
        return;
    }

    x = g_FieldTextBoxes[index]._pad[0x4A];
    y = g_FieldTextBoxes[index]._pad[0x4B];
    w = g_FieldTextBoxes[index]._pad[0x4C];
    h = g_FieldTextBoxes[index]._pad[0x4D];

    // While the window opens it grows out of the middle of its rect and the two
    // position offsets slide it the rest of the way.
    if (g_FieldTextBoxes[index].windowOpenTimer != 0) {
        scale = g_FieldTextScaleDivisor;
        shrink = (((s16)w << 16) / (scale * 2)) * (scale - g_FieldTextBoxes[index].windowOpenTimer);
        x = x + w / 2 - (shrink >> 16);
        w = (shrink * 2) >> 16;
        shrink = (((s16)h << 16) / (scale * 2)) * (scale - g_FieldTextBoxes[index].windowOpenTimer);
        y = y + h / 2 - (shrink >> 16);
        h = (shrink * 2) >> 16;
        if (w < 16) {
            x -= (16 - w) / 2;
            w = 16;
        }
        if (h < 16) {
            y -= (16 - h) / 2;
            h = 16;
        }
        g_FieldTextBoxes[index].positionOffsetX += g_FieldTextBoxes[index].positionOffsetDeltaX;
        g_FieldTextBoxes[index].positionOffsetY += g_FieldTextBoxes[index].positionOffsetDeltaY;
        x += g_FieldTextBoxes[index].positionOffsetX >> 16;
        y += g_FieldTextBoxes[index].positionOffsetY >> 16;
    }

    // The continue arrow is drawn only in a fully open top window that is not
    // waiting on the cursor; the timer is the frame delay before it appears.
    if (g_FieldTextBoxes[index].continueArrow.visibility == 0 &&
        g_FieldTextBoxes[index].order == 0 &&
        g_FieldTextBoxes[index].windowOpenTimer == 0 &&
        !(g_FieldTextBoxes[index].flags & 0x40) &&
        g_FieldTextBoxes[index].cursor.visibility != 0) {
        if (g_FieldTextBoxes[index].continueArrowTimer == 0) {
            colOffset = SystemStringGetColumnOffset(&g_FieldTextBoxes[index]._pad[0]);
            rowOffset = SystemStringGetRowOffset(&g_FieldTextBoxes[index]._pad[0]);
            rect.x = g_FieldContinueArrowFrameRects[g_FieldTextBoxFrameIndex].x;
            rect.y = g_FieldContinueArrowFrameRects[g_FieldTextBoxFrameIndex].y;
            rect.w = g_FieldContinueArrowFrameRects[g_FieldTextBoxFrameIndex].w;
            rect.h = g_FieldContinueArrowFrameRects[g_FieldTextBoxFrameIndex].h;
            SetDrawMode(&g_FieldTextBoxes[index].continueArrow.drawModes[ctx], NULL, 0,
                        GetTPage(0, 0, 0x298, 0x1C0), &rect);
            g_FieldTextBoxes[index].continueArrow.sprites[ctx].x0 = colOffset;
            g_FieldTextBoxes[index].continueArrow.sprites[ctx].y0 = rowOffset + 4;
            addPrim(pOt, (u_long*)&g_FieldTextBoxes[index].continueArrow.sprites[ctx]);
            addPrim(pOt, (u_long*)&g_FieldTextBoxes[index].continueArrow.drawModes[ctx]);
        } else {
            g_FieldTextBoxes[index].continueArrowTimer--;
        }
    } else {
        g_FieldTextBoxes[index].continueArrowTimer = 2;
    }

    // The window frame is eight sprites: four corners and the four edges
    // stretched between them.
    pieceHeight = h - 0x12;

    // Stores are grouped by value, not by piece: the original computes each
    // edge coordinate once and writes every piece that shares it.
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 0].x0 = x - 8;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 0].y0 = y - 7;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 2].y0 = y - 7;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 4].x0 = x - 8;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 2].x0 = x + w - 8;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 6].x0 = x + w - 8;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 3].x0 = x - 8;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 1].x0 = x + w - 8;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 4].y0 = y + h - 9;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 6].y0 = y + h - 9;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 3].y0 = y + 9;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 1].y0 = y + 9;
    if (pieceHeight < 0) {
        pieceHeight = 0;
    }
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 3].h = pieceHeight;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 1].h = pieceHeight;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 5].y0 = y - 7;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 5].x0 = x + 8;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 7].x0 = x + 8;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 7].y0 = y + h - 9;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 5].w = w - 0x10;
    g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + 7].w = w - 0x10;

    if (!(g_FieldTextBoxes[index].flags & 0x40)) {
        for (i = 0; i < 8; i++) {
            addPrim(pOt, (u_long*)&g_FieldTextBoxes[index].borders.sprites1[ctx * 10 + i]);
            addPrim(pOt, (u_long*)&g_FieldTextBoxes[index].borders.drawModes1[ctx * 10 + i]);
        }
    }

    portraitWidth = 0x40;
    if (w - 4 < 0x40) {
        portraitWidth = w - 8;
    }
    portraitHeight = 0x40;
    if (h - 4 < 0x40) {
        portraitHeight = h - 8;
    }
    portraitX = x + 4;
    if (g_FieldTextBoxes[index].flags & 0x20) {
        portraitX = x + w - portraitWidth - 4;
    }
    FieldTextBoxSetPortraitQuad(&g_FieldTextBoxes[index].portrait.polys[ctx], portraitX, y + 4,
                               portraitWidth, portraitHeight,
                               g_FieldTextBoxes[index].flags & 0x20);
    if (g_FieldTextBoxes[index].portrait.shouldRenderPortrait == 1) {
        addPrim(pOt, (u_long*)&g_FieldTextBoxes[index].portrait.polys[ctx]);
        addPrim(pOt, (u_long*)&g_FieldTextBoxes[index].portrait.drawModes[ctx]);
    }

    // The cursor sits inset from the window's left edge, or further in when a
    // portrait takes that side, and follows the current line down the box.
    if (g_FieldTextBoxes[index].cursor.visibility == 0 &&
        g_FieldTextBoxes[index].order == 0 &&
        g_FieldTextBoxes[index].windowOpenTimer == 0) {
        if (g_FieldTextBoxes[index].portrait.shouldRenderPortrait == 1 &&
            !(g_FieldTextBoxes[index].flags & 0x20)) {
            cursorX = x + 0x5A;
        } else {
            cursorX = x + 0x16;
        }
        g_FieldTextBoxes[index].cursor.sprites[ctx].x0 = cursorX;
        g_FieldTextBoxes[index].cursor.sprites[ctx].y0 =
            (g_FieldTextBoxes[index].cursor.curPosition +
             g_FieldTextBoxes[index].cursor.startPosition) * 14 + y + 8;
        rect.x = g_FieldCursorFrameRects[g_FieldTextBoxFrameIndex].x;
        rect.y = g_FieldCursorFrameRects[g_FieldTextBoxFrameIndex].y;
        rect.w = g_FieldCursorFrameRects[g_FieldTextBoxFrameIndex].w;
        rect.h = g_FieldCursorFrameRects[g_FieldTextBoxFrameIndex].h;
        SetDrawMode(&g_FieldTextBoxes[index].cursor.drawModes[ctx], NULL, 0,
                    GetTPage(0, 0, 0x288, 0x1C0), &rect);
        addPrim(pOt, (u_long*)&g_FieldTextBoxes[index].cursor.sprites[ctx]);
        addPrim(pOt, (u_long*)&g_FieldTextBoxes[index].cursor.drawModes[ctx]);
    }

    g_FieldTextBoxes[index].background.tiles[ctx].x0 = x;
    g_FieldTextBoxes[index].background.tiles[ctx].y0 = y + 1;
    g_FieldTextBoxes[index].background.tiles[ctx].w = w;
    g_FieldTextBoxes[index].background.tiles[ctx].h = h - 2;
    if (!(g_FieldTextBoxes[index].flags & 0x40)) {
        addPrim(pOt, (u_long*)&g_FieldTextBoxes[index].background.tiles[ctx]);
        addPrim(pOt, (u_long*)&g_FieldTextBoxes[index].background.drawModes[ctx]);
    }
}*/
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", func_8007E1C0);





extern u8 g_SystemBackdropColor;
extern u8 D_800594D5;
extern u8 D_800594D6;
extern RECT D_800ADE9C[];

// PARKED 63.05% (re-measured round 458): 1784 bytes against retail 1952, 446
// instructions against 488. The structure is not missing a block:
// tools/scripts/m2c_draft.sh produces the same statement order, and m2c reads the
// same anchor (g_FieldTextBoxes->_pad[0x4A], i.e. symbol plus 0xAC), so what is
// left is allocation.
// Round 458 census, instruction count per call-delimited region (original vs this
// body): 17/17, 19/20, 6/6, 7/7, 4/4, tiles+arrow 45/40, arrow draw modes 13/10,
// arrow sprite 81/55, cursor draw modes 13/9, 6/6, 7/7, 3/3, cursor sprite 85/75,
// borders loop head 20/8, 6/6, 8/8, 6/3, 15/13, 9/9, loop sprite 78/57, portrait
// 14/20, 6/6, 7/7, 9/3, 8/9, 11/11, tail 42/38.
// The three big deficits are exactly the blocks where retail rebuilds the element
// base: retail keeps index in s6 (never spilled) and index times 0x498 in s0 for
// the whole function, then for every scalar field store emits lui at,%hi of
// g_FieldTextBoxes, addu at,at,s0 and the store with the RAW field offset as its
// immediate (sh t2,0x3F0(at) and friends: eight of those in the arrow sprite
// block alone, most of its 26-instruction lead). This body CSEs one
// g_FieldTextBoxes[index].<sub> pointer into a callee-saved register and writes
// +offset relative to it, which is two instructions cheaper per store; that is
// where the 42 missing instructions are, and the 63% fuzzy score is those address
// forms. Retail setRGB0 too goes through that pointer (sb s2,4(s1)), so retail
// mixes both forms inside one block.
// The borders loop recomputes the whole 7-instruction index times 0x498 chain
// every iteration (into s4) and spills five walking offsets to stack words
// 0x20/0x28/0x30/0x38/0x40: 0x140 plus i times 0x14, i times 0x14, 0xC8 plus i
// times 0xC, 0x50 plus i times 0xC and D_800ADE9C plus i times 8. The first four
// share ONE combined base, symbol plus 0xAC (0xAC+0x50 is borders.drawModes1,
// +0xC8 drawModes2, +0x140 borders.sprites1); the two sprite copies use a second
// anchor at symbol plus 0x1EC (sprites1 itself) plus i times 0x14. So retail runs
// two symbolic address families, not one box pointer.
// Frame: retail 0x70 saves s0 to s8; this body 0x80 and spills the index
// parameter at 0x20, i.e. retail has one more register of pressure than this body.
// Preset screen round 458 on this body: 2.7.2 -O2 -G8 63.05, 2.7.2 -G0 63.05,
// 2.6.0 66.33; 2.7.2-cdk and -O3 fail to compile this unit (cc1 exit 33 on
// another function in the file), so the preset is right.
// Measured and dead earlier: pointer locals per sub-struct 56.13; longhand 63.05
// (this one); longhand with pRect and a pHeight walking pointer in the borders
// loop 61.89; round 445 walking pointers for all four border arrays plus a RECT
// pointer (SetSprt(pSprites1); then pSprites2[p] = pSprites1[p]) 61.54, 1628
// bytes, 407 instructions, so adding live values does not buy retail address forms.
// Next idea for a round with budget: stop the scalar field stores from sharing the
// CSE that the call arguments create, e.g. pass a sprite pointer local to
// SetSprt/setRGB0 but write w/clut/u/v/h/x/y through the full
// g_FieldTextBoxes[index].continueArrow.sprites[0].field expression, and see
// whether each store then rebuilds at instead of using pointer plus offset.
/*
void FieldTextBoxInitializePrimitives(int index) {
    int i;
    RECT rect;

    // Background
    SetDrawMode(&g_FieldTextBoxes[index].background.drawModes[0], NULL, 0,
                GetTPage(0, 2, 0x280, 0x1F0), NULL);
    SetDrawMode(&g_FieldTextBoxes[index].background.drawModes[1], NULL, 0,
                GetTPage(0, 2, 0x280, 0x1F0), NULL);
    SetTile(&g_FieldTextBoxes[index].background.tiles[0]);
    setRGB0(&g_FieldTextBoxes[index].background.tiles[0], g_SystemBackdropColor, D_800594D5,
            D_800594D6);
    SetSemiTrans(&g_FieldTextBoxes[index].background.tiles[0], 1);
    g_FieldTextBoxes[index].background.tiles[1] = g_FieldTextBoxes[index].background.tiles[0];

    // Continue arrow
    rect.x = g_FieldContinueArrowFrameRects[0].x;
    rect.y = g_FieldContinueArrowFrameRects[0].y;
    rect.w = g_FieldContinueArrowFrameRects[0].w;
    rect.h = g_FieldContinueArrowFrameRects[0].h;
    SetDrawMode(&g_FieldTextBoxes[index].continueArrow.drawModes[0], NULL, 0,
                GetTPage(0, 0, 0x298, 0x1C0), &rect);
    SetDrawMode(&g_FieldTextBoxes[index].continueArrow.drawModes[1], NULL, 0,
                GetTPage(0, 0, 0x298, 0x1C0), &rect);
    SetSprt(&g_FieldTextBoxes[index].continueArrow.sprites[0]);
    setRGB0(&g_FieldTextBoxes[index].continueArrow.sprites[0], 0x80, 0x80, 0x80);
    g_FieldTextBoxes[index].continueArrow.sprites[0].w = 0xC;
    g_FieldTextBoxes[index].continueArrow.sprites[0].clut = GetClut(0x100, 0xF6);
    setUV0(&g_FieldTextBoxes[index].continueArrow.sprites[0], 0x80, 0xC0);
    g_FieldTextBoxes[index].continueArrow.sprites[0].h = 0x8;
    setXY0(&g_FieldTextBoxes[index].continueArrow.sprites[0], 0x0, 0x0);
    g_FieldTextBoxes[index].continueArrow.sprites[1] =
        g_FieldTextBoxes[index].continueArrow.sprites[0];

    // Cursor
    rect.x = g_FieldCursorFrameRects[0].x;
    rect.y = g_FieldCursorFrameRects[0].y;
    rect.w = g_FieldCursorFrameRects[0].w;
    rect.h = g_FieldCursorFrameRects[0].h;
    SetDrawMode(&g_FieldTextBoxes[index].cursor.drawModes[0], NULL, 0,
                GetTPage(0, 0, 0x288, 0x1C0), &rect);
    SetDrawMode(&g_FieldTextBoxes[index].cursor.drawModes[1], NULL, 0,
                GetTPage(0, 0, 0x288, 0x1C0), &rect);
    SetSprt(&g_FieldTextBoxes[index].cursor.sprites[0]);
    setRGB0(&g_FieldTextBoxes[index].cursor.sprites[0], 0x80, 0x80, 0x80);
    g_FieldTextBoxes[index].cursor.sprites[0].h = 0xC;
    g_FieldTextBoxes[index].cursor.sprites[0].clut = GetClut(0x100, 0xF6);
    setUV0(&g_FieldTextBoxes[index].cursor.sprites[0], 0x80, 0xC0);
    g_FieldTextBoxes[index].cursor.sprites[0].w = 0x8;
    setXY0(&g_FieldTextBoxes[index].cursor.sprites[0], 0x0, 0x0);
    g_FieldTextBoxes[index].cursor.sprites[1] = g_FieldTextBoxes[index].cursor.sprites[0];
    g_FieldTextBoxes[index].continueArrowTimer = 2;

    // Borders
    for (i = 0; i < 8; i++) {
        rect.x = D_800ADE9C[i].x;
        rect.y = D_800ADE9C[i].y;
        rect.w = D_800ADE9C[i].w;
        rect.h = D_800ADE9C[i].h;
        SetDrawMode(&g_FieldTextBoxes[index].borders.drawModes1[i], NULL, 0,
                    GetTPage(0, 2, 0x280, 0x1F0), &rect);
        SetDrawMode(&g_FieldTextBoxes[index].borders.drawModes2[i], NULL, 0,
                    GetTPage(0, 2, 0x280, 0x1F0), &rect);
        SetSprt(&g_FieldTextBoxes[index].borders.sprites1[i]);
        setRGB0(&g_FieldTextBoxes[index].borders.sprites1[i], 0x80, 0x80, 0x80);
        g_FieldTextBoxes[index].borders.sprites1[i].clut = GetClut(0x100, 0xF4);
        SetSemiTrans(&g_FieldTextBoxes[index].borders.sprites1[i], 1);
        setUV0(&g_FieldTextBoxes[index].borders.sprites1[i], 0x80, 0xC0);
        g_FieldTextBoxes[index].borders.sprites1[i].w = D_800ADE9C[i].w;
        setXY0(&g_FieldTextBoxes[index].borders.sprites1[i], 0x0, 0x0);
        g_FieldTextBoxes[index].borders.sprites1[i].h = D_800ADE9C[i].h;
        g_FieldTextBoxes[index].borders.sprites2[i] =
            g_FieldTextBoxes[index].borders.sprites1[i];
    }

    // Portrait
    rect.y = 0;
    rect.x = 0;
    rect.h = 0xFF;
    rect.w = 0xFF;
    SetDrawMode(&g_FieldTextBoxes[index].portrait.drawModes[0], NULL, 0,
                GetTPage(1, 0, 0x2C0, 0x100), &rect);
    SetDrawMode(&g_FieldTextBoxes[index].portrait.drawModes[1], NULL, 0,
                GetTPage(1, 0, 0x2C0, 0x100), &rect);
    SetPolyFT4(&g_FieldTextBoxes[index].portrait.polys[0]);
    setRGB0(&g_FieldTextBoxes[index].portrait.polys[0], 0x80, 0x80, 0x80);
    g_FieldTextBoxes[index].portrait.polys[0].clut = GetClut(0, 0xE0);
    g_FieldTextBoxes[index].portrait.polys[0].tpage = GetTPage(1, 0, 0x2C0, 0x100);
    g_FieldTextBoxes[index].portrait.polys[1] = g_FieldTextBoxes[index].portrait.polys[0];
}*/
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", FieldTextBoxInitializePrimitives);



// Each portrait is a 0x40 x 0x40 tile of the portrait texture; g_FieldTextBoxPortraitUVs holds
// the top left U and V of each of the four portraits.
extern u_char g_FieldTextBoxPortraitUVs[];

// The V half of the table needs its own base local: with a single base gcc
// attaches the +2 to the scaled index instead of the base and the addiu lands on
// the wrong register.
void FieldTextBoxSetPortraitUV(int index, int portraitID) {
    FieldTextBox* pBox = &g_FieldTextBoxes[index];
    u_char* pUVTable = g_FieldTextBoxPortraitUVs;
    u_char* pU = &pUVTable[portraitID * 4];
    u_char* pVTable = pUVTable + 2;
    u_char* pV = &pVTable[portraitID * 4];
    u_short clut;

    g_FieldTextBoxes[index].portrait.polys[1].u0 = pBox->portrait.polys[0].u0 = *pU;
    g_FieldTextBoxes[index].portrait.polys[1].v0 = pBox->portrait.polys[0].v0 = *pV;
    g_FieldTextBoxes[index].portrait.polys[1].u1 = pBox->portrait.polys[0].u1 = *pU + 0x40;
    g_FieldTextBoxes[index].portrait.polys[1].v1 = pBox->portrait.polys[0].v1 = *pV;
    g_FieldTextBoxes[index].portrait.polys[1].u2 = pBox->portrait.polys[0].u2 = *pU;
    g_FieldTextBoxes[index].portrait.polys[1].v2 = pBox->portrait.polys[0].v2 = *pV + 0x40;
    g_FieldTextBoxes[index].portrait.polys[1].u3 = pBox->portrait.polys[0].u3 = *pU + 0x40;
    g_FieldTextBoxes[index].portrait.polys[1].v3 = pBox->portrait.polys[0].v3 = *pV + 0x40;

    clut = GetClut(0, portraitID + 0xE0);
    pBox->portrait.polys[0].clut = clut;
    g_FieldTextBoxes[index].portrait.polys[1].clut = clut;
}

extern u16 g_FieldTextBoxInUseMask[];

// The string state helpers take a SystemString, which only system.c names; the
// text box keeps one at _pad[0].
extern void SystemStringClearStateFlags(void* pString);
extern void SystemStringResetIfIdle(void* pString);
extern void SystemStringFree(void* pString);

// Bit per text box index, set while the box is in use.
int FieldTextBoxFree(short index) {
    if (g_FieldTextBoxes[index].visibility == 0) {
        SystemStringResetIfIdle(&g_FieldTextBoxes[index]._pad[0]);
        SystemStringClearStateFlags(&g_FieldTextBoxes[index]._pad[0]);
        SystemStringFree(&g_FieldTextBoxes[index]._pad[0]);

        g_FieldTextBoxes[index].cursor.visibility = -1;
        g_FieldTextBoxes[index].visibility = -1;
        g_FieldTextBoxes[index].status = -1;
        g_FieldTextBoxes[index].order = TEXT_BOX_UNINITIALIZED;
        g_FieldTextBoxAllocationStates[index] = -1;
        g_FieldTextBoxInUseMask[0] &= (1 << index) ^ 0xFF;
        g_FieldTextBoxes[index].ownerActorID = 0xFF;
        g_FieldTextBoxes[index].unk412 = 0;

        return 0;
    }

    return -1;
}

// Project (0, Y, 0) from actor's model/local space to screen
void FieldProjectActorPointToScreen(int actorIndex, int* pScreenX, int* pScreenY, int height) {
    SVECTOR vertex;
    MATRIX transformMatrix;
    long sxy;
    long p;
    long flag;

    CompMatrix(&g_Scene.worldToScreenMatrix, &g_FieldActors[actorIndex].transformMatrix, &transformMatrix);
    SetRotMatrix(&transformMatrix);
    SetTransMatrix(&transformMatrix);
    vertex.vx = 0;
    vertex.vy = height;
    vertex.vz = 0;
    RotTransPers(&vertex, &sxy, &p, &flag);
    *pScreenY = ((short*) &sxy)[1];
    *pScreenX = ((short*) &sxy)[0];
}

// The text box keeps a SystemString at _pad[0]. system.c owns the full layout; this
// unit seeds its number arguments, the string ref and the buffer it pushes.
typedef struct {
    /* 0x00 */ undefined8 unk0[0x10];
    /* 0x10 */ u_short flags;
    /* 0x12 */ undefined8 unk12[0x56];
    /* 0x68 */ u_char unk68;
    /* 0x69 */ undefined8 unk69[0x7];
    /* 0x70 */ s32 args[4];
    /* 0x80 */ s16 stringRef;
    /* 0x82 */ s16 unk82;
    /* 0x84 */ undefined8 unk84[0xC];
    /* 0x90 */ void* pBuffer;
} FieldTextBoxString;

// The text box keeps its string at _pad[0], which is box + 0x18.
#define TEXT_BOX_STRING(index) ((FieldTextBoxString*)g_FieldTextBoxes[index]._pad)

// Where the window ends up: the projected actor point, or the point the mode's
// arithmetic produces.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
} FieldTextBoxAnchor;

// The VRAM strip each text box uploads its glyph row to: one (x, y) pair per box.
typedef struct {
    /* 0x0 */ u_short vramX;
    /* 0x2 */ u_short vramY;
} FieldTextBoxGlyphStrip;

// The portrait index sits in bits 1-3 of ActorData's script flag word at 0x12C,
// which the header declares only as bitfields (a bitfield read narrows the load).
typedef struct {
    /* 0x000 */ undefined8 unk0[0x12C];
    /* 0x12C */ u_int scriptFlags;
} ActorScriptFlagWord;

extern FieldTextBoxGlyphStrip D_800ADF54[];
extern s32 D_800ADE90;
extern s16 g_FieldTextScaleDivisor;
// FieldDialogs is only ever handed straight back to GetStringEntry here.
extern void* g_FieldDialogs;
extern u_char* GetStringEntry(void* pDialogs, s32 dialogIndex);
// The string init takes the values unsigned: the VRAM strip and the two extents
// are passed without a sign extension.
extern void SystemStringInit(void* pString, s32 vramX, s32 vramY, s32 x, s32 y, s32 maxWidth,
                             s32 maxChars);

// A character cell is 4 x 14 pixels and the window keeps an 8 pixel border on
// every side, so the offsets to its centre are 2 * width + 8 and 7 * height + 8.
#define FIELD_TEXT_BOX_CHAR_WIDTH 4
#define FIELD_TEXT_BOX_LINE_HEIGHT 14
#define FIELD_TEXT_BOX_BORDER_WIDTH 8
// A mode 2 window is centred on the screen.
#define FIELD_TEXT_BOX_CENTER_X 0xA0
// Extra left inset the text takes when a portrait is drawn beside it.
#define FIELD_TEXT_BOX_PORTRAIT_INDENT 0x44

// 75.80%, 1748 bytes / 437 insns against retail's 1804 bytes / 451 insns. The body is
// structurally complete (the allocation loop, the four script variable reads, the three
// mode arms, the portrait, the rect and string seeding, both position offsets with their
// two division arms, and the flags tail) and every call, branch and reloc matches.
//  1. The flat store form is SOLVED, and round 457's note below was wrong about it. Retail
//     never holds a box base in a register: every box access is
//     lui $at,%hi(g_FieldTextBoxes+K) / addu $at,$at,<textBoxIndex*0x498> /
//     op %lo(g_FieldTextBoxes+K)($at), i.e. gas expanding the flat sym+addend($reg)
//     macro. That is NOT special to a typed MEMBER: gcc emits it for any access reached
//     as a member or as a _pad[K] cast, and what defeated it here was the
//     TEXT_BOX_STRING(i) pointer macro, whose value gcc CSEs into one callee-saved
//     register (la $3,g_FieldTextBoxes+24 / addu $16,$16,$3) so that EVERY later box
//     access came out off(reg) instead of flat. Probed with cc1_probe.sh on this unit's
//     cc1: five typed member accesses share one index register and are all flat (exactly
//     func_8007DCF8's matched shape), and _pad[K] casts are flat too EXCEPT for
//     CONTIGUOUS offsets, where gcc derives them from the first
//     (la $17,g_FieldTextBoxes+136 / addu $3,$17,4 / addu $3,$16,$3). The body now spells
//     the string fields as _pad[K] casts and keeps the macro only for the
//     SystemStringInit argument, where retail also forms the address with la+addu;
//     that alone took 73.38% to 75.80% (421 -> 437 insns).
//  2. What is left is the frame and the register set: retail's frame is 112 to this
//     body's 104 with the same ten saved registers. Retail spills x to a sh slot at 0x28
//     and top at 0x30, reloads them with lhu+sll/sra at five uses, keeps dialogIndex at
//     0x38 and leaves 0x2C/0x34/0x3C empty (its source declared more locals than survive);
//     this body keeps top in $fp and x in a slot, so retail had live values this one does
//     not. Rejected spellings below are all below 75.80 now, including the typed view
//     struct.
// Also measured and rejected: raw byte-offset stores 75.16 (1744 bytes), typed view
// struct 73.29 (1688), s16-pair aggregate for x/top 67.48 (1664), dialogIndex as u16
// (retail sws it, so s32 is right), branchless (faceFlags & 0x800) ? 1 : 0, and reading
// dialogFlags straight from the field (gcc narrows >> 16 into lhu 0x86, so a local word
// read is required and is in the body).
// Confirmed correct, do NOT "fix" it: D_800ADF54 is indexed by textBoxIndex, not by the
// allocation slot - retail copies the box index into $s5 right after the allocation loop,
// so the loop's slot dies there. All 9 calls, every branch and every store match. The same
// base-in-a-register blocker is shared by this unit's other box parks (FieldTextBoxDraw
// 68.38, FieldTextBoxInitializePrimitives, FieldTextBoxInitialize 82.32).
// Preset question closed: gcc-2.6.0 71.13, -O1 55.25, gcc-2.7.2-cdk fails to compile, so
// Default (-O2 -G8) stands. twin_for.py has no twin.
// Shipping mechanism 2 needs more live values in the source, not a header decision: 2.7.2
// rejects an anonymous union, so _pad cannot sit beside a typed string member in the same
// slot, and re-splitting it breaks text_box.c's TEXT_BOX_STRING macro.
// Remaining, in order of size: (1) frame 104 vs retail's 112 with the same ten saved registers, and retail spills x (sh 0x28), top (sh 0x30) and dialogIndex (sw 0x38) and reloads top with lhu+sll/sra at five uses while this body keeps top in $fp - retail's source carried more live values than this one; (2) the four contiguous arg stores still derive from la g_FieldTextBoxes+136 instead of being flat; (3) the register assignment cascades (s2/s3, s6/s7 swapped, storage order of the args at SystemStringInit). Preset question closed (2.6.0 71.13, -O1 55.25, 2.7.2-cdk does not compile). twin_for.py: no twin.
/*
s32 func_8007F8DC(s16 x, s16 y, s32 dialogIndex, s32 textBoxIndex, s32 width, s32 height,
                  s32 curActorIndex, s32 actorIndex, s32 mode, u32 flags, u32 faceFlags) {
    FieldTextBoxAnchor anchor;
    u32 dialogFlags;
    u32 upperFlags;
    u32 lowerFlags;
    u_int scriptFlags;
    s32 inset;
    s32 value;
    s16 top;
    s32 i;
    s32 slot;

    top = y - FIELD_TEXT_BOX_BORDER_WIDTH;

    dialogFlags = g_FieldActors[curActorIndex].pActorData->dialogFlags;
    upperFlags = dialogFlags >> 0x10;
    if (upperFlags == 0) {
        lowerFlags = dialogFlags & 0xFFFF;
    } else {
        lowerFlags = upperFlags & 0xFFFF;
    }
    flags |= lowerFlags;

    for (i = 0; i < 4; i++) {
        slot = D_800ADE90 & 3;
        D_800ADE90++;
        if (g_FieldTextBoxAllocationStates[slot] == -1) {
            g_FieldTextBoxAllocationStates[slot] = 0;
            break;
        }
    }

    *(s32*)&g_FieldTextBoxes[textBoxIndex]._pad[0x38] = FieldScriptVMGetVariableValue(0x16);
    *(s32*)&g_FieldTextBoxes[textBoxIndex]._pad[0x3A] = FieldScriptVMGetVariableValue(0x18);
    *(s32*)&g_FieldTextBoxes[textBoxIndex]._pad[0x3C] = FieldScriptVMGetVariableValue(0x1A);
    value = FieldScriptVMGetVariableValue(0x1C);
    *(s32*)&g_FieldTextBoxes[textBoxIndex]._pad[0x3E] = value;
    g_FieldTextBoxes[textBoxIndex]._pad[0x40] = value;

    if (mode == 2) {
        anchor.x = FIELD_TEXT_BOX_CENTER_X;
        anchor.y = top + 0x20;
    } else if (mode == 3) {
        anchor.x = x + FIELD_TEXT_BOX_BORDER_WIDTH + width * 2;
        anchor.y = top + FIELD_TEXT_BOX_BORDER_WIDTH + height * 7;
    } else {
        FieldProjectActorPointToScreen(actorIndex, &anchor.x, &anchor.y, -0x40);
    }

    if (g_FieldActors[curActorIndex].pActorData->faceId != 0xFF && !(flags & 0x2)) {
        scriptFlags = ((ActorScriptFlagWord*)g_FieldActors[curActorIndex].pActorData)->scriptFlags;
        if (flags & 0x402) {
            FieldTextBoxSetPortraitUV(textBoxIndex, (scriptFlags >> 1) & 0xE);
        } else {
            FieldTextBoxSetPortraitUV(textBoxIndex, ((scriptFlags >> 1) & 0xE) | 1);
        }
        g_FieldTextBoxes[textBoxIndex].portrait.shouldRenderPortrait = 1;
        g_FieldTextBoxes[textBoxIndex].portrait.portraitID =
            g_FieldActors[curActorIndex].pActorData->faceId;
    } else {
        g_FieldTextBoxes[textBoxIndex].portrait.portraitID = 0x80;
        g_FieldTextBoxes[textBoxIndex].portrait.shouldRenderPortrait = 0;
    }

    g_FieldTextBoxes[textBoxIndex].cursor.visibility = -1;
    FieldTextBoxSetRect(textBoxIndex, x, top, width * FIELD_TEXT_BOX_CHAR_WIDTH + FIELD_TEXT_BOX_BORDER_WIDTH * 2,
                        height * FIELD_TEXT_BOX_LINE_HEIGHT + FIELD_TEXT_BOX_BORDER_WIDTH * 2);

    inset = 0;
    if (g_FieldActors[curActorIndex].pActorData->faceId != 0xFF) {
        inset = (flags & 0x402) == 0 ? FIELD_TEXT_BOX_PORTRAIT_INDENT : 0;
    }
    SystemStringInit(TEXT_BOX_STRING(textBoxIndex), D_800ADF54[textBoxIndex].vramX,
                     D_800ADF54[textBoxIndex].vramY, x + FIELD_TEXT_BOX_BORDER_WIDTH + inset,
                     top + FIELD_TEXT_BOX_BORDER_WIDTH, width, height);

    if (flags & 0x400) {
        g_FieldTextBoxes[textBoxIndex].flags |= 0x20;
    }
    *(u_char*)&g_FieldTextBoxes[textBoxIndex]._pad[0x34] = g_FieldTextScaleDivisor == 8 ? 1 : 2;
    *(void**)&g_FieldTextBoxes[textBoxIndex]._pad[0x48] = GetStringEntry(g_FieldDialogs, dialogIndex);
    g_FieldTextBoxes[textBoxIndex].visibility = 0;
    g_FieldTextBoxes[textBoxIndex]._pad[0x08] |= 0x2;
    g_FieldTextBoxes[textBoxIndex].ownerActorID = curActorIndex;
    g_FieldTextBoxes[textBoxIndex].talkingActorID = actorIndex;
    g_FieldTextBoxes[textBoxIndex].windowOpenTimer = g_FieldTextScaleDivisor;
    if (faceFlags & 0x800) {
        g_FieldTextBoxes[textBoxIndex].unk412 = 1;
    } else {
        g_FieldTextBoxes[textBoxIndex].unk412 = 0;
    }

    g_FieldTextBoxes[textBoxIndex].positionOffsetX =
        (anchor.x - (width * 2 + FIELD_TEXT_BOX_BORDER_WIDTH) - x) << 16;
    g_FieldTextBoxes[textBoxIndex].positionOffsetY =
        (anchor.y - (height * 7 + FIELD_TEXT_BOX_BORDER_WIDTH) - top) << 16;

    if (!(flags & 0x100)) {
        g_FieldTextBoxes[textBoxIndex].positionOffsetDeltaX =
            -(g_FieldTextBoxes[textBoxIndex].positionOffsetX / g_FieldTextScaleDivisor);
        g_FieldTextBoxes[textBoxIndex].positionOffsetDeltaY =
            -(g_FieldTextBoxes[textBoxIndex].positionOffsetY / g_FieldTextScaleDivisor);
    } else {
        g_FieldTextBoxes[textBoxIndex].windowOpenTimer = 1;
        g_FieldTextBoxes[textBoxIndex].positionOffsetDeltaX =
            -g_FieldTextBoxes[textBoxIndex].positionOffsetX;
        g_FieldTextBoxes[textBoxIndex].positionOffsetDeltaY =
            -g_FieldTextBoxes[textBoxIndex].positionOffsetY;
    }

    if ((g_FieldActors[actorIndex].pActorData->flags & 0x200) && !(flags & 0x1)) {
        g_FieldTextBoxes[textBoxIndex].status = 0;
        return -1;
    }
    return 0;
}*/
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", func_8007F8DC);

void FieldTextBoxFreeAll(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (!g_FieldTextBoxes[i].visibility) {
            FieldTextBoxFree((s16) i); // TODO: Clean up cast w/ function sig
        }
    }
}

extern int SystemStringGetUnk6B(FieldTextBoxString* pString);
extern void SystemStringPushBuffer(FieldTextBoxString* pString, void* pBuffer);
extern void func_80034888(FieldTextBoxString* pString, u_long* pOt, int ctx);
extern void func_8007E1C0(u_long* pOt, int ctx, int index, u_long primAddr);

// Text animation frame, advanced every fourth frame and wraps after five.
extern s32 g_FieldTextBoxFrameIndex;
// Frames counted by this function; the low two bits clock g_FieldTextBoxFrameIndex.
extern s32 D_800ADE98;
// Field's own pad button state, filled by FieldPollControllers.
extern u_short g_FieldP1ButtonsReleased;
// 91.62%, 1420 bytes against retail's 1448, 355 insns against 362, frame 0x58 (matches).
// Round 505: the control flow was the find. The `order == TEXT_BOX_UNINITIALIZED`
// test is NOT an `else if` of `order == n` in retail: its block sits at .L800804FC,
// reached BOTH in fallthrough from the draws and by the two early jumps out of the
// `order == n` body (visibility != 0, i == activeIndex), and the `order != n` guard
// branches straight to the loop latch. Moving that test inside the `order == n` body
// (after the draws, re-reading order because the calls may store it) is what makes
// gcc reduce the order address the way retail does: retail's $s3 =
// &g_FieldTextBoxes[0].order, formed with lui+addiu in the OUTER loop preheader and
// stepped 0x498 in the latch, read as `lhu 0($s3)` three times. The old else-if body
// scored 91.11 / 1428 bytes / 357 insns; this shape scored 90.40 / 1440 / 360 with a
// 0x60 frame, and spelling the string base out at each use in the order loop (no
// pString local there, drop the `_pad` casts) bought the frame back to 0x58 and
// reached the measured 91.62 / 1420 / 355. With pString kept in the order loop the
// frame stays 0x60, so the two changes belong together.
// Residual (all one cause, retail has less register pressure): retail re-forms
// base+const per iteration in the order loop instead of keeping a register -- the
// string base goes back to lui+addiu g_FieldTextBoxes+0x18 plus `addu s1,s0,v0` in
// the body (2 insns, this body keeps it as a third 1176-byte biv in $s1, +1 in the
// latch), the pBuffer argument of SystemStringPushBuffer is read flat off the box
// (`lw a1,168(at)`, this body does `lw a1,144(s1)`), and the addPrim chain re-forms
// the base twice. Retail also re-materialises `li v0,-1` at both visibility stores
// where this body hoists -1 into $s8. Measured this round: pBuffer flat through the
// box in both loops, `*(void**)&g_FieldTextBoxes[i]._pad[0x48]` (0xA8 - 0x18 = 0x90
// bytes = _pad[0x48] of `short _pad[]`) 89.45 / 1452 bytes / 363 insns -- 4 bytes
// LONG, so that spelling over-shoots and is not the answer either.
// Older rounds, still true: the second argument is the render context (drawModes[ctx],
// ctx*0xC); the string helpers take the box's _pad[0] (box+0xA8 is its pBuffer);
// func_8007DCF8 takes three arguments (retail sets a0=i, a1=pOt, a2=ctx) though its
// body reads only the first. gcc 2.7.2-cdk scores 69.63 and -O3 is unchanged, so the
// preset is right. Round 478/491 (order address as the biv, pString variants, alias
// decls for the order reads) are summarised in the round-491 note this replaces.
// Measured dead (do not retry): pString at all five order-loop sites with the old
// else-if body 89.42 / 1432 / 358; dropping pString there 88.93 / 1436 / 359;
// spelling the pBuffer argument through the box in the first loop only 89.74 / 1432;
// an element-pointer local for addPrim 91.16 / 1420; a second extern of
// g_FieldTextBoxes for the order reads is byte-identical to the plain decl (the decls
// unify, so no alias lever); pBuffer flat through the box in both loops on the old
// body 89.09 / 1480 / 370.
// 95.91%, 1452 bytes against retail's 1448, 363 insns against 362, frame 0x58 (matches).
// Round 506. Two finds, both measured.
// 1. pBuffer IS read through the box, in BOTH loops: retail emits
//    lui at,%hi(g_FieldTextBoxes+0xA8); addu at,at,s0; lw a1,%lo(...)(at) at each
//    use. The spelling that makes gcc produce that is a fresh address-of-slot local
//    right before the deref (permuter find): pBufferSlot = &box._pad[0x48] (0xA8 -
//    0x18 = 0x90 bytes = _pad[0x48] of `short _pad[0x56]`), then *(void**)pBufferSlot.
//    The direct `*(void**)&g_FieldTextBoxes[i]._pad[0x48]` spelling instead makes gcc
//    hoist base+0xA8 into a loop biv (lui+addiu in the preheader, +1 in the latch)
//    and the 0xFF000000 mask then spills to 48(sp): 367 insns / 1468 bytes.
// 2. Keep the pString local in the ORDER loop. With the cast spelled out at each use
//    gcc keeps a third 1176-byte biv in $s2 (addiu in the latch); with pString gcc
//    forms retail's per-iteration lui v0,%hi(base+0x18); addiu v0,v0,24;
//    addu s1,s0,v0 and the frame returns to 0x58 once find 1 lands. pString alone
//    (pBuffer through pString) is 89.45% with a 0x60 frame; the two finds belong
//    together: 91.62 -> 92.06 -> 93.73 -> 95.91% as they went in.
// Residual, one instruction: in the order loop's addPrim, retail loads the
// 0xFF000000 mask once with lui t0,0xff00, uses it for both ands, and re-forms the
// g_FieldTextBoxes base into that same t0 afterwards; this body keeps the prim
// address in t0 across the second base form, so the mask is re-materialised
// (lui t1,0xff00, +4 bytes), and `sw v0,0(a2)` lands after the second base form
// instead of before it. Everything else in both loops is instruction-identical up to
// register choice (a2 vs t0 for the prim address, t0/t1 for -1 and count) and the
// position of `move a2,s2` in the draw block. Permuter found nothing from this body
// (3 min, 1267 iters) or from the 92.06 body (4 min, 1557 iters).
// Round 531 re-measured this body (95.91 / 1452 / 363, unchanged) and tried two more
// spellings of the prim address in both draw blocks, both worse but both informative:
//  (a) a `u_long* pDrawMode` local used for the addPrim argument AND for
//      `(u_long)pDrawMode & 0xFFFFFF`: 92.55 / 1424 / 356. One register then serves the
//      address and the masked value, so six instructions vanish and every register in
//      the draw block renames -- the two forms are separate pseudos in retail.
//  (b) `primAddr = (u_long)&g_FieldTextBoxes[i].drawModes[ctx] & 0xFFFFFF;` on the line
//      before addPrim, the call passing primAddr: 92.72 / 1448 / 362, i.e. RETAIL'S
//      EXACT BYTE COUNT. The residual below disappears there (the 0xFF000000 mask is
//      materialised once and the base re-form reuses its register), so the whole
//      remaining gap is one hoist: the invariant half of the address, ctx*12, moves out
//      of the draw block to the top of the loop body (sll a3,s6,1 in the bnez delay slot
//      at .L800801E0), where retail recomputes it inside each draw block. Next idea for
//      a round with budget: get the masked value computed before the p store WITHOUT
//      letting gcc hoist ctx*12 out of the draw block.
// Dead ends measured this round, do not retry: pBuffer through the box with the
// direct cast in both loops 89.45 / 1452 / 363; that plus pString in the order loop
// 92.06 / 1468 / 367; pString in the order loop with pBuffer through pString
// 90.40 / 1440 / 360 (0x60 frame); the round-505 baseline 91.62 / 1420 / 355.
/*
void func_8008004C(u_long* pOt, int ctx) {
    s32 order[4];
    s32 count;
    s32 activeIndex;
    int i;
    int n;
    FieldTextBoxString* pString;
    short* pBufferSlot;

    D_800ADE98++;
    if ((D_800ADE98 & 3) == 0) {
        g_FieldTextBoxFrameIndex++;
    }
    if (g_FieldTextBoxFrameIndex >= 5) {
        g_FieldTextBoxFrameIndex = 0;
    }

    activeIndex = 0xFF;
    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].unk412 != 0) {
            activeIndex = i;
        }
    }

    count = 0;
    for (i = 3; i >= 0; i--) {
        order[i] = TEXT_BOX_UNINITIALIZED;
    }

    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].visibility == 0 && g_FieldTextBoxes[i].unk412 != 0) {
            pString = (FieldTextBoxString*)g_FieldTextBoxes[i]._pad;
            g_FieldTextBoxes[i].continueArrow.visibility = -1;
            if (g_FieldTextBoxes[i].windowOpenTimer == 0) {
                if (SystemStringGetUnk6B(pString) != 0) {
                    if (g_FieldTextBoxes[i].cursor.visibility != 0) {
                        g_FieldTextBoxes[i].continueArrow.visibility = 0;
                    }
                }
                if (g_FieldP1ButtonsReleased & CTRL_BTN_CIRCLE) {
                    g_FieldTextBoxes[i].cursor.visibility = -1;
                    g_FieldActors[g_FieldTextBoxes[i].ownerActorID].pActorData->dialogCursorPosition =
                        (u_char)g_FieldTextBoxes[i].cursor.curPosition +
                        (u_char)g_FieldTextBoxes[i].cursor.startPosition;
                    SystemStringClearStateFlags(pString);
                }
                if (pString->unk82 == 0) {
                    pBufferSlot = &g_FieldTextBoxes[i]._pad[0x48];
                    SystemStringPushBuffer(pString, *(void**)pBufferSlot);
                }
                func_80034888(pString, pOt, ctx);
            }
            addPrim(pOt, (u_long*)&g_FieldTextBoxes[i].drawModes[ctx]);
            func_8007E1C0(pOt, ctx, i, (u_long)&g_FieldTextBoxes[i].drawModes[ctx] & 0xFFFFFF);
            func_8007DCF8(i, pOt, ctx);
        }
    }

    for (n = 0; n < 4; n++) {
        for (i = 0; i < 4; i++) {
            if (g_FieldTextBoxes[i].order == n) {
                order[i] = count++;
                if (g_FieldTextBoxes[i].visibility == 0 && i != activeIndex) {
                    pString = (FieldTextBoxString*)g_FieldTextBoxes[i]._pad;
                    g_FieldTextBoxes[i].continueArrow.visibility = -1;
                    if (g_FieldTextBoxes[i].windowOpenTimer == 0) {
                        if ((g_FieldP1ButtonsReleased & CTRL_BTN_CIRCLE) && g_FieldTextBoxes[i].order == 0) {
                            g_FieldTextBoxes[i].cursor.visibility = -1;
                            g_FieldActors[g_FieldTextBoxes[i].ownerActorID].pActorData->dialogCursorPosition =
                                (u_char)g_FieldTextBoxes[i].cursor.curPosition +
                                (u_char)g_FieldTextBoxes[i].cursor.startPosition;
                            SystemStringClearStateFlags(pString);
                        }
                        if (pString->unk82 == 0) {
                            pBufferSlot = &g_FieldTextBoxes[i]._pad[0x48];
                            SystemStringPushBuffer(pString, *(void**)pBufferSlot);
                        }
                        func_80034888(pString, pOt, ctx);
                        if (SystemStringGetUnk6B(pString) != 0) {
                            if (g_FieldTextBoxes[i].cursor.visibility != 0) {
                                g_FieldTextBoxes[i].continueArrow.visibility = 0;
                            }
                        }
                    }
                    addPrim(pOt, (u_long*)&g_FieldTextBoxes[i].drawModes[ctx]);
                    func_8007E1C0(pOt, ctx, i, (u_long)&g_FieldTextBoxes[i].drawModes[ctx] & 0xFFFFFF);
                    func_8007DCF8(i, pOt, ctx);
                }
                if (g_FieldTextBoxes[i].order == TEXT_BOX_UNINITIALIZED) {
                    order[i] = TEXT_BOX_UNINITIALIZED;
                }
            }
        }
    }

    for (n = 0; n < 4; n++) {
        g_FieldTextBoxes[n].order = order[n];
    }

    addPrim(pOt, (u_long*)&D_800B1DF4[g_FieldCurRenderContextIndex][0]);
}*/
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", func_8008004C);





void FieldTextBoxUpdate(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (!g_FieldTextBoxes[i].visibility) {
            // TODO: _pad[0x8] is probably a short holding some flags
            if (g_FieldTextBoxes[i].windowOpenTimer == 0 && !(g_FieldTextBoxes[i]._pad[0x8] & 4)) {
                FieldTextBoxFree( (i*0x10000) >> 0x10);
            }
            if (g_FieldTextBoxes[i].status == 0) {
                FieldTextBoxFree( (i*0x10000) >> 0x10);
            }
            if (g_FieldTextBoxes[i].windowOpenTimer) {
                g_FieldTextBoxes[i].windowOpenTimer--;
            }
        }
    }
}

// Get index of textbox with order 0
int FieldTextBoxGetTopIndex(void) {
    int i;
    
    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].order == 0) {
            return i;
        }
    }
    return TEXT_BOX_UNINITIALIZED;
}


// Is textbox free to use?
int FieldTextBoxHasFreeSlot(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].order == TEXT_BOX_UNINITIALIZED) {
            return 0;
        }
    }
    return -1;
}

// Get index of textbox with lowest order
int FieldTextBoxGetBottomIndex(void) {
    int i;
    int nResult;
    int lowestOrder;
    
    lowestOrder = 0; // 0 = Top
    nResult = 0xFFFF;
    
    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].order != 0xFFFF && g_FieldTextBoxes[i].order >= lowestOrder) {
            lowestOrder = g_FieldTextBoxes[i].order;
            nResult = i;
        }
    }
    return nResult;
}

// Update textbox order
int FieldTextBoxAllocateTop(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].order != TEXT_BOX_UNINITIALIZED) {
            g_FieldTextBoxes[i].order++;
        }
    }

    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].order == TEXT_BOX_UNINITIALIZED) {
            g_FieldTextBoxes[i].order = 0;
            return i;
        }
    }

    return TEXT_BOX_UNINITIALIZED;
}