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
extern void func_80034874(void* pString, int value);
extern void func_8003487C(void* pString);

// Field's own pad button state, filled by FieldPollControllers.
extern u_short D_800C3900;

// Cursor up/down input for the text box holding the top message; moves the
// cursor within maxPosition and forwards the line offset to the string.
void func_8007DCF8(int index) {
    if (g_FieldTextBoxes[index].cursor.visibility == 0) {
        if (g_FieldTextBoxes[index].windowOpenTimer == 0) {
            if (g_FieldTextBoxes[index].order == 0) {
                if (D_800C3900 & CTRL_BTN_DOWN) {
                    g_FieldTextBoxes[index].cursor.curPosition++;
                    if (g_FieldTextBoxes[index].cursor.curPosition > g_FieldTextBoxes[index].cursor.maxPosition - 1) {
                        g_FieldTextBoxes[index].cursor.curPosition = 0;
                    }
                }
                if (D_800C3900 & CTRL_BTN_UP) {
                    g_FieldTextBoxes[index].cursor.curPosition--;
                    if (g_FieldTextBoxes[index].cursor.curPosition < 0) {
                        g_FieldTextBoxes[index].cursor.curPosition = g_FieldTextBoxes[index].cursor.maxPosition - 1;
                    }
                }
                func_80034874(&g_FieldTextBoxes[index]._pad[0],
                    g_FieldTextBoxes[index].cursor.curPosition + g_FieldTextBoxes[index].cursor.startPosition);
            } else {
                func_8003487C(&g_FieldTextBoxes[index]._pad[0]);
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
// 82.32% below. Also note D_800B068C/D_800B2174 must be declared above this
// function if it becomes C.
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", FieldTextBoxInitialize);
/*
extern DR_MODE D_800B1DF4[];
extern DR_MODE D_800B1E00[];
extern RECT D_800AFC80[];
extern RECT D_800AFC84[];

extern void FieldTextBoxInitializePrimitives(int index);

void FieldTextBoxInitialize(void) {
    int i;
    RECT rect;

    rect.y = 0;
    rect.x = 0;
    rect.h = 0xFF;
    rect.w = 0xFF;

    for (i = 0; i < 16; i++) {
        D_800AFC80[i].y = 0;
        D_800AFC80[i].x = 0;
        D_800AFC80[i].h = 0xFF;
        D_800AFC84[i].x = 0xFF;
        D_800AFC80[i].y = 0;
        D_800AFC80[i].x = 0;
        D_800AFC80[i].h = 0xFF;
        D_800AFC84[i].x = 0xFF;
        SetDrawMode(&D_800B1DF4[i], NULL, 0, GetTPage(0, 2, 0x380, 0x100), &D_800AFC80[i]);
        SetDrawMode(&D_800B1E00[i], NULL, 0, GetTPage(0, 2, 0x380, 0x100), &D_800AFC80[i]);
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
        D_800B068C[i] = -1;
        SetDrawMode(&g_FieldTextBoxes[i].drawModes[0], NULL, 0, GetTPage(0, 0, 0x300, 0x100), &rect);
        SetDrawMode(&g_FieldTextBoxes[i].drawModes[1], NULL, 0, GetTPage(0, 0, 0x300, 0x100), &rect);
    }
}
*/

// _pad[0x4A] is possibly a RECT?
void func_8007E114(int index, int arg1, int arg2, int arg3, s32 arg4) {
    g_FieldTextBoxes[index]._pad[0x4A] = arg1;
    g_FieldTextBoxes[index]._pad[0x4B] = arg2;
    g_FieldTextBoxes[index]._pad[0x4C] = arg3;
    g_FieldTextBoxes[index]._pad[0x4D] = arg4;
}

// Place a portrait quad: x - 1 and x + width when facing right, mirrored when
// flipped.
void func_8007E16C(POLY_FT4* pPoly, int x, int y, int width, int height, int flip) {
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
extern u8 D_800594D4;
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
    setRGB0(pBackgroundTile, D_800594D4, D_800594D5, D_800594D6);
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

// Each portrait is a 0x40 x 0x40 tile of the portrait texture; D_800ADF34 holds
// the top left U and V of each of the four portraits.
extern u_char D_800ADF34[];

// 98.61%: pUVTable is what makes gcc emit `la D_800ADF34` before the `sll` of
// portraitID * 4, but it costs a register: the table base ends up in a2 and the
// scaled index in v1, where the original uses v0 and a0. Writing pU/pV straight
// as &D_800ADF34[portraitID * 4] / [+2] gets the original's registers (97.59%)
// but emits the `sll` one instruction earlier. Needs the table hidden behind a
// construct that keeps v0 as the base.
//
// Tried and no better: `D_800ADF34 + portraitID * 4` (same 97.59%), the same
// with a (u_char*)/(u_int) cast, portraitID << 2, and `u_int table = (u_int)
// D_800ADF34; ... table + portraitID * 4` (98.61%, base lands in a2). The order
// is gcc's: expand_binop swaps a constant first operand to op1, so the shift of
// a directly-indexed table is always expanded before the `la`. Getting the `la`
// first needs a preceding movsi, and gcc then allocates that base to a2/an
// a-register instead of the temp v0.
INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", func_8007F5AC);

// Best, 97.59%: one `sll` too high, everything else byte-identical.
/*
void func_8007F5AC(int index, int portraitID) {
    FieldTextBox* pBox = &g_FieldTextBoxes[index];
    u_char* pU = &D_800ADF34[portraitID * 4];
    u_char* pV = &D_800ADF34[portraitID * 4 + 2];
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
*/

// Alternative, 98.61%: pUVTable keeps the `la` first but moves the base to a2.
/*
void func_8007F5AC(int index, int portraitID) {
    FieldTextBox* pBox = &g_FieldTextBoxes[index];
    u_char* pUVTable = D_800ADF34;
    u_char* pU = &pUVTable[portraitID * 4];
    u_char* pV = &pUVTable[portraitID * 4 + 2];
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
*/

extern u16 D_800B2174[];
extern int D_800B068C[4];

// The string state helpers take a SystemString, which only system.c names; the
// text box keeps one at _pad[0].
extern void func_800345E0(void* pString);
extern void func_80034614(void* pString);
extern void func_800346D4(void* pString);

// Bit per text box index, set while the box is in use.
int func_8007F6F8(short index) {
    if (g_FieldTextBoxes[index].visibility == 0) {
        func_80034614(&g_FieldTextBoxes[index]._pad[0]);
        func_800345E0(&g_FieldTextBoxes[index]._pad[0]);
        func_800346D4(&g_FieldTextBoxes[index]._pad[0]);

        g_FieldTextBoxes[index].cursor.visibility = -1;
        g_FieldTextBoxes[index].visibility = -1;
        g_FieldTextBoxes[index].status = -1;
        g_FieldTextBoxes[index].order = TEXT_BOX_UNINITIALIZED;
        D_800B068C[index] = -1;
        D_800B2174[0] &= (1 << index) ^ 0xFF;
        g_FieldTextBoxes[index].ownerActorID = 0xFF;
        g_FieldTextBoxes[index].unk412 = 0;

        return 0;
    }

    return -1;
}

// Project (0, Y, 0) from actor's model/local space to screen
void func_8007F814(int actorIndex, int* pScreenX, int* pScreenY, int height) {
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

INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", func_8007F8DC);

void func_8007FFE8(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (!g_FieldTextBoxes[i].visibility) {
            func_8007F6F8((s16) i); // TODO: Clean up cast w/ function sig
        }
    }
}

INCLUDE_ASM("asm/field/nonmatchings/dialogue/text_box_render", func_8008004C);

void func_800805F4(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (!g_FieldTextBoxes[i].visibility) {
            // TODO: _pad[0x8] is probably a short holding some flags
            if (g_FieldTextBoxes[i].windowOpenTimer == 0 && !(g_FieldTextBoxes[i]._pad[0x8] & 4)) {
                func_8007F6F8( (i*0x10000) >> 0x10);
            }
            if (g_FieldTextBoxes[i].status == 0) {
                func_8007F6F8( (i*0x10000) >> 0x10);
            }
            if (g_FieldTextBoxes[i].windowOpenTimer) {
                g_FieldTextBoxes[i].windowOpenTimer--;
            }
        }
    }
}

// Get index of textbox with order 0
int func_800806E4(void) {
    int i;
    
    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].order == 0) {
            return i;
        }
    }
    return TEXT_BOX_UNINITIALIZED;
}


// Is textbox free to use?
int func_80080720(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (g_FieldTextBoxes[i].order == TEXT_BOX_UNINITIALIZED) {
            return 0;
        }
    }
    return -1;
}

// Get index of textbox with lowest order
int func_80080760(void) {
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
int func_800807B4(void) {
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