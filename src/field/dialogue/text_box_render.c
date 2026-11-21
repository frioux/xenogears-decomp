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

INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", func_8007E1C0);

INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", FieldTextBoxInitializePrimitives);
/*
extern u8 g_SystemBackdropColor;
extern u8 D_800594D5;
extern u8 D_800594D6;
extern RECT D_800ADE9C[];
extern RECT D_800ADEDC;
extern RECT D_800ADF04;

void FieldTextBoxInitializePrimitives(int index) {
    int i;
    RECT rect;
    POLY_FT4* pPoly;
    POLY_FT4* pPoly2;
    TILE* pBackgroundTile;
    TILE* pBackgroundTile2;
    SPRT* pBorderSprite1;
    SPRT* pBorderSprite2;
    SPRT* pArrowSprite;
    SPRT* pArrowSprite2;
    SPRT* pCursorSprite2;
    u16* pWidth;
    u16* pHeight;

    FieldTextBoxBackground* pBackground;
    FieldTextBoxCursor* pCursor;
    FieldTextBoxContinueArrow* pArrow;
    FieldTextBoxBorders* pBorders;
    FieldTextBoxPortrait* pPortrait;

    // Background
    pBackground = &g_FieldTextBoxes[index].background;
    SetDrawMode(&pBackground->drawModes[0], NULL, 0, GetTPage(0, 2, 0x280, 0x1F0), NULL);
    SetDrawMode(&pBackground->drawModes[1], NULL, 0, GetTPage(0, 2, 0x280, 0x1F0), NULL);
    pBackgroundTile = &pBackground->tiles[0];
    SetTile(pBackgroundTile);
    setRGB0(pBackgroundTile, g_SystemBackdropColor, D_800594D5, D_800594D6);
    SetSemiTrans(pBackgroundTile, 1);
    pBackgroundTile2 =  &pBackground->tiles[1];
    *pBackgroundTile2 = *pBackgroundTile;

    // Arrow
    pArrow = &g_FieldTextBoxes[index].continueArrow;
    rect.x = D_800ADF04.x;
    rect.y = D_800ADF04.y;
    rect.w = D_800ADF04.w;
    rect.h = D_800ADF04.h;
    SetDrawMode(&pArrow->drawModes[0], NULL, 0, GetTPage(0, 0, 0x298, 0x1C0), &rect);
    SetDrawMode(&pArrow->drawModes[1], NULL, 0, GetTPage(0, 0, 0x298, 0x1C0), &rect);
    SetSprt(&pArrow->sprites[0]);
    pArrowSprite = &pArrow->sprites[0];
    setRGB0(pArrowSprite, 0x80, 0x80, 0x80);
    setWH0(pArrowSprite, 0xC, 0x8);
    pArrowSprite->clut = GetClut(0x100, 0xF6);
    pArrowSprite2 = &pArrow->sprites[1];
    setUV0(pArrowSprite, 0x80, 0xC0);
    setXY0(pArrowSprite, 0x0, 0x0);
    *pArrowSprite2 = *pArrowSprite;

    // Cursor
    pCursor = &g_FieldTextBoxes[index].cursor;
    rect.x = D_800ADEDC.x;
    rect.y = D_800ADEDC.y;
    rect.w = D_800ADEDC.w;
    rect.h = D_800ADEDC.h;
    SetDrawMode(&pCursor->drawModes[0], NULL, 0, GetTPage(0, 0, 0x288, 0x1C0), &rect);
    SetDrawMode(&pCursor->drawModes[1], NULL, 0, GetTPage(0, 0, 0x288, 0x1C0), &rect);
    SetSprt(&pCursor->sprites[0]);
    setRGB0(&pCursor->sprites[0], 0x80, 0x80, 0x80);
    pCursor->sprites[0].clut = GetClut(0x100, 0xF6);
    pCursorSprite2 = &pCursor->sprites[1];
    setWH0(&pCursor->sprites[0], 0x8, 0xC);
    setUV0(&pCursor->sprites[0], 0x80, 0xC0);
    setXY0(&pCursor->sprites[0], 0x0, 0x0);
    *pCursorSprite2 = pCursor->sprites[0];
    g_FieldTextBoxes[index].continueArrowTimer = 2;

    // Borders
    pBorders = &g_FieldTextBoxes[index].borders;
    for (i = 0; i < 8; i++) {
        pWidth = &D_800ADE9C[i].w;
        pHeight = &D_800ADE9C[i].h;
        
        rect.x = D_800ADE9C[i].x;
        rect.y = D_800ADE9C[i].y;
        rect.w = *pWidth;
        rect.h = *pHeight;
        
        SetDrawMode(&pBorders->drawModes1[i], NULL, 0, GetTPage(0, 2, 0x280, 0x1F0), &rect);
        SetDrawMode(&pBorders->drawModes2[i], NULL, 0, GetTPage(0, 2, 0x280, 0x1F0), &rect);
        pBorderSprite1 = &pBorders->sprites1[i];
        SetSprt(pBorderSprite1);
        setRGB0(pBorderSprite1, 0x80, 0x80, 0x80);
        pBorderSprite1->clut = GetClut(0x100, 0xF4);
        SetSemiTrans(pBorderSprite1, 1);
        setUV0(pBorderSprite1, 0x80, 0xC0);
        setWH0(pBorderSprite1, *pWidth, *pHeight);
        setXY0(pBorderSprite1, 0x0, 0x0);
        pBorderSprite2 =  &pBorders->sprites2[i];
        *pBorderSprite2 = *pBorderSprite1;
    }

    // Portrait
    pPortrait = &g_FieldTextBoxes[index].portrait;
    rect.y = 0;
    rect.x = 0;
    rect.h = 0xFF;
    rect.w = 0xFF;
    SetDrawMode(&pPortrait->drawModes[0], NULL, 0, GetTPage(1, 0, 0x2C0, 0x100), &rect);
    SetDrawMode(&pPortrait->drawModes[1], NULL, 0, GetTPage(1, 0, 0x2C0, 0x100), &rect);
    pPoly = &pPortrait->polys[0];
    SetPolyFT4(pPoly);
    setRGB0(pPoly, 0x80, 0x80, 0x80);
    pPoly->clut = GetClut(0, 0xE0);
    pPoly2 = &pPortrait->polys[1];
    pPoly->tpage = GetTPage(1, 0, 0x2C0, 0x100);
    *pPoly2 = *pPoly;
}
*/

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

INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", func_8007F8DC);

// 73.38%, 1684 bytes against retail's 1628: the whole body is in place (the
// allocation loop, the four script variable reads, the three mode arms, the
// portrait, the rect and string seeding, both position offsets with their two
// division arms, and the flags tail) and every call, branch and reloc matches.
// What is left is allocation and one address-form family.
//  1. Retail re-forms `%hi(g_FieldTextBoxes + K)` at EVERY box access, including
//     the eight string stores at 0x28/0x80/0x88..0x98/0xA8 (relocs read
//     `g_FieldTextBoxes + 0x88` with `addu $at,$at,$s0`). A typed pointer
//     (`TEXT_BOX_STRING(i)->args[n]`, what text_box.c does) makes gcc CSE the
//     string base into a callee-saved register instead, which also costs the
//     frame: measured 1684 bytes / vars=24. Spelling those eight accesses as raw
//     byte offsets (`*(s32 *)((u_char *)&g_FieldTextBoxes[i] + 0x88)`) scores
//     75.16% and brings the frame to 104 (retail 112) with x finally spilled to
//     the stack like retail's, but emits `la $sN,g_FieldTextBoxes+K` + an extra
//     addu per store and ends at 1744 bytes. The header's `short _pad[0x56]`
//     covers 0x18-0xC4, so the string cannot be reached as a member chain
//     without re-splitting that shared field.
//  2. Retail spills x (0x28) and top (0x30) to s16 stack slots and reloads them
//     with lhu+sll/sra at each of the five uses; this build keeps one or both in
//     callee-saved registers, and the slots at 0x2C/0x34/0x3C suggest retail's
//     frame also carries two unreferenced 4-byte locals (a `{ s16 left; s16
//     top; }` struct forced to memory measures 67.48% / 1664 bytes, so the shape
//     is not just "make them aggregates").
//  3. Retail addresses g_FieldTextScaleDivisor through a pointer (`lui`/`addiu`
//     into $a0 then `lh $v1,0($a0)` twice, one per division) where this build
//     emits two direct `lui`/`lh` pairs.
// Measured and rejected: dialogIndex as u16 (retail spills it with `sw`, so s32
// is right), the branchless `(faceFlags & 0x800) ? 1 : 0` for box->unk412
// (retail branches on `andi 0x800` and stores a `li 1`), and reading dialogFlags
// straight from the field (gcc narrows `>> 16` into `lhu 0x86`; a local word
// read is required and is in the body).
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
}
*/


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
extern s32 D_800ADE94;
// Frames counted by this function; the low two bits clock D_800ADE94.
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
        D_800ADE94++;
    }
    if (D_800ADE94 >= 5) {
        D_800ADE94 = 0;
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