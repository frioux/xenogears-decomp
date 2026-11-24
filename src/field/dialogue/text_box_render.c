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

// https://decomp.me/scratch/NE0tE
// 82.32%: the two 16 entry loops look right but gcc picks different induction
// variables for the rect stores. The original addresses the same 8-byte rect
// (D_800AFC80 + i * 8) three ways: a strength reduced pointer for x, a
// recomputed symbol + i * 8 for y/h (also the SetDrawMode rect argument), and
// D_800AFC84 + i * 8 for w, plus it writes each rect twice.
// Postscript: declaring D_800AFC84 as `short[]` and writing D_800AFC84[i * 4]
// (w) / D_800AFC84[i * 4 - 2] (x) does reproduce the original's shared base
// (`la D_800AFC84-4` / `addiu s3, s2, -4`) and scopes the double store right,
// but gcc then strength reduces the D_800AFC80 writes instead of the AFC84 ones
// and re-forms the AFC80 address in the loop body -- 80.22%, worse than the
// 82.32% below. Also note g_FieldTextBoxAllocationStates/g_FieldTextBoxInUseMask must be declared above this
// function if it becomes C.
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", FieldTextBoxInitialize);
// 85.16%: D_800B1DF4/D_800B1E00 are 16 DR_MODEs at 0xC each (SetDrawMode takes
// &D_800B1DF4[i]); GetTPage's abr argument is 0, not 2. Best shape so far: y, x,
// h, w in the order the original stores them, with x/w reached through a short
// view of D_800AFC84 (D_800AFC84[i*4-2] = x, [+i*4] = w) -- that reproduces the
// original's `la D_800AFC84-4` / shared biv, but gcc then strength reduces the
// D_800AFC80 pointer and re-forms the D_800AFC84 one, the reverse of retail.
/*
void FieldTextBoxInitialize(void) {
    int i;
    RECT rect;

    rect.y = 0;
    rect.x = 0;
    rect.h = 0xFF;
    rect.w = 0xFF;

    for (i = 0; i < 16; i++) {
        D_800AFC80[i].y = 0;
        D_800AFC84[i * 4 - 2] = 0;
        D_800AFC80[i].h = 0xFF;
        D_800AFC84[i * 4] = 0xFF;
        D_800AFC80[i].y = 0;
        D_800AFC84[i * 4 - 2] = 0;
        D_800AFC80[i].h = 0xFF;
        D_800AFC84[i * 4] = 0xFF;
        SetDrawMode(&D_800B1DF4[i], NULL, 0, GetTPage(0, 0, 0x380, 0x100), &D_800AFC80[i]);
        SetDrawMode(&D_800B1E00[i], NULL, 0, GetTPage(0, 0, 0x380, 0x100), &D_800AFC80[i]);
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
extern int g_FieldTextBoxAllocationStates[4];

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

// 73.38%, 1684 bytes / 421 insns against retail's 1804 bytes / 451 insns. The body is
// structurally complete (the allocation loop, the four script variable reads, the three
// mode arms, the portrait, the rect and string seeding, both position offsets with their
// two division arms, and the flags tail) and every call, branch and reloc matches. The 30
// missing instructions are two mechanisms, both confirmed:
//  1. Retail NEVER holds a box base in a register: every box access is
//     lui $at,%hi(g_FieldTextBoxes+K) / addu $at,$at,<textBoxIndex*0x498> /
//     op %lo(g_FieldTextBoxes+K)($at), i.e. gas expanding the flat sym+addend($reg)
//     macro, which only a typed MEMBER (g_Box[i].string.args[0]) produces. The string
//     sits at box+0x18, which the header declares only as the opaque short _pad[0x56],
//     so every spelling here starts from a cast. PROBED (cc1_probe.sh with this unit's
//     cc1, round 457): the flat form IS emitted for the FIRST store of a block
//     (sw $2,g_FieldTextBoxes+136($3)) and from the second one on cse keeps sym+136 in a
//     register and derives the rest (la $17,g_FieldTextBoxes+136 / addu $3,$17,4 /
//     addu $3,$16,$3), so no cast spelling reproduces retail's 3-insn-per-store form;
//     the cast forms also make gcc CSE sym+scaled into a callee-saved register instead.
//  2. The frame and the register set carry the larger part: even a local re-split of the
//     header (round 444) measures 79.67% / 1724 bytes / 431 insns, so the typing is a
//     prerequisite and not the answer. Retail spills x to a sh slot at 0x28 and top at
//     0x30, reloads them with lhu+sll/sra at five uses, keeps dialogIndex at 0x38 and
//     leaves 0x2C/0x34/0x3C empty (its source declared more locals than survive); this
//     body keeps x/top in $s6/$s7 and needs only a 0x58 frame.
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
// Shipping mechanism 1 needs a header decision, not a C one: 2.7.2 rejects an anonymous
// union, so _pad cannot sit beside a typed string member in the same slot, and
// re-splitting it breaks text_box.c's TEXT_BOX_STRING macro.
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

    TEXT_BOX_STRING(textBoxIndex)->args[0] = FieldScriptVMGetVariableValue(0x16);
    TEXT_BOX_STRING(textBoxIndex)->args[1] = FieldScriptVMGetVariableValue(0x18);
    TEXT_BOX_STRING(textBoxIndex)->args[2] = FieldScriptVMGetVariableValue(0x1A);
    value = FieldScriptVMGetVariableValue(0x1C);
    TEXT_BOX_STRING(textBoxIndex)->args[3] = value;
    TEXT_BOX_STRING(textBoxIndex)->stringRef = value;

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
    TEXT_BOX_STRING(textBoxIndex)->unk68 = g_FieldTextScaleDivisor == 8 ? 1 : 2;
    TEXT_BOX_STRING(textBoxIndex)->pBuffer = GetStringEntry(g_FieldDialogs, dialogIndex);
    g_FieldTextBoxes[textBoxIndex].visibility = 0;
    TEXT_BOX_STRING(textBoxIndex)->flags |= 0x2;
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
// One draw mode per render context that closes the text box list.
extern DR_MODE D_800B1DF4[2][0x10];

INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", func_8008004C);
// 91.11%, 1428 bytes against retail's 1448: every call, both loops, the order
// renumbering and both addPrim chains are in place; the 61 instructions that
// still differ are all allocation. Retail keeps `count` on the stack and runs a
// second induction variable over g_FieldTextBoxes[i].order; here gcc keeps
// count in $s8, hoists the two -1 stores of the visibility loop into one
// register and re-forms the order address instead. The permuter improved this
// body 3635 -> 2661 by spelling the string out at each use in the order loop
// only (spelling it out in the visibility loop as well drops to 88.93); gcc
// 2.7.2-cdk scores 69.63 and -O3 is unchanged, so the preset is right.
// Things this body proves about the unit: the second argument is the render
// context (drawModes[ctx], ctx*0xC), the order loop's else-if test really is in
// the original (it reloads the order and compares it against 0xFFFF), and the
// string helpers take the box's _pad[0] (box+0xA8 is its buffer pointer, which
// retail addresses through g_FieldTextBoxes). func_8007DCF8 takes three
// arguments (retail sets a0=i, a1=pOt and a2=ctx at each call) even though its
// body reads only the first, which is why its prototype above has three.
// Round 165: retried the spelled-out pBuffer form (89.43%), an element-pointer
// local for the addPrim argument (91.16% but 1420 bytes, further from retail's
// 1448) and re-measured the parked body itself at 91.11%/1428, unchanged.
/*
void func_8008004C(u_long* pOt, int ctx) {
    s32 order[4];
    s32 count;
    s32 activeIndex;
    int i;
    int n;
    FieldTextBoxString* pString;

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
                    SystemStringPushBuffer(pString, pString->pBuffer);
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
                            SystemStringClearStateFlags((FieldTextBoxString*)g_FieldTextBoxes[i]._pad);
                        }
                        if (pString->unk82 == 0) {
                            SystemStringPushBuffer((FieldTextBoxString*)g_FieldTextBoxes[i]._pad,
                                ((FieldTextBoxString*)g_FieldTextBoxes[i]._pad)->pBuffer);
                        }
                        func_80034888((FieldTextBoxString*)g_FieldTextBoxes[i]._pad, pOt, ctx);
                        if (SystemStringGetUnk6B((FieldTextBoxString*)g_FieldTextBoxes[i]._pad) != 0) {
                            if (g_FieldTextBoxes[i].cursor.visibility != 0) {
                                g_FieldTextBoxes[i].continueArrow.visibility = 0;
                            }
                        }
                    }
                    addPrim(pOt, (u_long*)&g_FieldTextBoxes[i].drawModes[ctx]);
                    func_8007E1C0(pOt, ctx, i, (u_long)&g_FieldTextBoxes[i].drawModes[ctx] & 0xFFFFFF);
                    func_8007DCF8(i, pOt, ctx);
                }
            } else if (g_FieldTextBoxes[i].order == TEXT_BOX_UNINITIALIZED) {
                order[i] = TEXT_BOX_UNINITIALIZED;
            }
        }
    }

    for (n = 0; n < 4; n++) {
        g_FieldTextBoxes[n].order = order[n];
    }

    addPrim(pOt, (u_long*)&D_800B1DF4[g_FieldCurRenderContextIndex][0]);
}
*/

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