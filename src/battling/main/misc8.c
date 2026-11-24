#include "common.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "system/controller.h"

// the battle tile grid is 128 rows by 128 columns of 4-byte cells
#define BATTLE_GRID_CELL_COUNT 16384

extern u16 D_800915BE;
extern u16 D_80091636;
extern u8 g_BattleScreenRestoreCount;
extern u8 g_BattleScreenRestorePending;
extern u16 D_80091762;
extern u8 g_BattleTextWindowVisible;
extern u_long *g_BattleScreenImageBuffer;
extern u8 g_BattleDrawBufferIndex;
extern s32 g_BattleTextPrimCount;
extern s32 g_BattleCursorIndex0;
extern s32 g_BattleCursorIndex1;
extern u8 g_BattleTextSpriteEntry0;
extern u8 g_BattleTextSpriteEntry1;
extern u16 g_BattleScreenHeight;
extern void *g_BattleMessageStringTable;
extern u8 g_BattleTextPadIndex;
extern s32 g_BattleMode;
extern s32 g_BattleSpriteArchiveBuffer;
extern s32 g_BattleStateAdvanceRequested;
extern s32 g_BattleSpriteArchiveFreed;
extern s32 g_BattleSlotConfirmFlags;
extern s32 g_BattleEntryCount;
extern s32 g_BattleSlotConfirmLatched;
extern u16 D_800916E2;
extern u16 D_8009171E;
extern u8 g_BattlePlayer1ComControlled;
extern u8 g_BattlePlayer2ComControlled;

extern void BattleClearTextSpriteEntries(void);
extern void BattleInitTutorialMessage(void);
extern void BattleLoadSlotEntry(s32 a0, s32 a1);
extern void SoundStopAllElements(void);
extern void func_8007313C(u16 *pPixels, u16 *pEnd);
extern void BattlePlaySoundEffectById(s32 arg);
extern void BattleSetScreenMode(s32 mode);
extern void BattleResetTextListState(void);
extern void BattleSetTextCursor(s16 a, s16 b);
extern void BattleSetRowTextColor(void *p, s32 arg);
extern void BattleDrawTextLeft(u8 *p);
extern void BattleDrawTextRightOffset(u8 *p, s32 dy);
extern void ControllerMergeStates(void);
typedef struct {
    /* 0x0  */ u8 flags;
    /* 0x1  */ u8 messageId;
    /* 0x2  */ u8 pad2[2];
    /* 0x4  */ u8 *pData;
    /* 0x8  */ void (*pCallback)(void *p);
    /* 0xC  */ void *pCallbackArg;
    /* 0x10 */ u8 unk10[2];
    /* 0x12 */ s16 xOffset;
} BattleTextEntry; // Size: 0x14

typedef struct {
    /* 0x0 */ u8 unk0[3];
    /* 0x3 */ u8 code;
    /* 0x4 */ u32 mode;
    /* 0x8 */ u16 x;
    /* 0xA */ u16 y;
    /* 0xC */ u16 w;
    /* 0xE */ u16 h;
} BattleTextFrame; // Size: 0x10

typedef struct {
    /* 0x00 */ u8 extraWidth;
    /* 0x01 */ u8 unk1[3];
    /* 0x04 */ BattleTextEntry *pEntries;
    /* 0x08 */ s16 count;
    /* 0x0A */ s16 nextListIndex;
    /* 0x0C */ void (*pCallback)(void *p);
    /* 0x10 */ u8 unk10[2];
    /* 0x12 */ s16 row;
    /* 0x14 */ u16 y;
    /* 0x16 */ u16 x;
    /* 0x18 */ u8 unk18[4];
    /* 0x1C */ BattleTextFrame frames[2]; /* current, saved */
} BattleTextList; // Size: 0x3C

extern BattleTextList g_BattleTextLists[8];
extern BattleTextList g_BattleDefaultTextList[];
extern BattleTextList g_BattleOptionTextList[];
extern BattleTextList *g_BattleCurrentTextList;
extern BattleTextList *g_BattlePrevTextList;
extern s32 g_BattleCurrentTextPadIndex;
extern s32 g_BattleTextEntryCursor;
extern s32 g_BattleButtonState;
extern s32 g_BattleButtonStatePressedOnce;
extern s32 g_BattleButtonStateReleased;
extern u8 g_BattleStickPushSoundPlayed;
extern int ControllerGetType(int controllerIndex);

// Per-draw-buffer slot record, 0xF8 bytes: the screen area the slot's text
// window covers at +0, and the tile primitive BattleSetScreenTileGrey fills at
// +0xE8.
typedef struct {
    /* 0x00 */ RECT rect;
    /* 0x08 */ u8 unk8[0xE0];
    /* 0xE8 */ TILE tile;
} Misc8SlotEntry; // Size: 0xF8

extern Misc8SlotEntry g_BattleDrawBuffers[];

extern void BattleDrawTextSprites(u8 *p);
extern void BattleLayoutTextWindowList(BattleTextList *p);
extern void BattleDrawTextList(BattleTextList *p);
extern s32 BattleUpdateTextPadInput(BattleTextList *p, s32 a1);
extern void func_80081A44(void);
extern void BattleDrawVibrationOptions(void);
extern s32 BattleGetTextWidth(u32 a0);
extern void BattleSetTextColor(s32 a0);
extern void BattleInitCenteredTextSprites(void);
extern void *GetStringEntry(void *pStringData, u32 entry);
extern s32 SystemRenderStringEntry(void *pCur, void *pCharBuffer, s32 mode, s32 flag);
extern s32 BattleLoadArchiveEntry(s32 a0);
extern void ArchiveCdDataSync(s32 mode);

void BattleRequestStateAdvance(void) {
    g_BattleStateAdvanceRequested = 1;
    BattleClearTextSpriteEntries();
}

void BattleStartTutorial(void) {
    g_BattleStateAdvanceRequested = 1;
    BattleInitTutorialMessage();
    BattleLoadSlotEntry(0, 0);
    BattleLoadSlotEntry(1, 1);
}

void BattleSetCurrentTextList(s32 a0) {
    if (a0 == 0xFF) {
        g_BattleCurrentTextList = g_BattlePrevTextList;
    } else {
        g_BattlePrevTextList = g_BattleCurrentTextList;
        g_BattleCurrentTextList = &g_BattleTextLists[a0];
    }
}

s32 BattleIsDefaultTextList(void) {
    return g_BattleCurrentTextList == g_BattleDefaultTextList;
}

void BattleInitTextWindow(void) {
    SoundStopAllElements();
    g_BattleCurrentTextList = 0;
    BattleSetCurrentTextList(3);
    g_BattleTextLists[3].row = 0;
    g_BattleTextLists[4].row = 0;
    g_BattleMode = 0;
    g_BattleTextWindowVisible = 0;
    BattleClearTextSpriteEntries();
    g_BattleStateAdvanceRequested = 0;
    BattleFreeScreenBuffer(0);
    g_BattleSpriteArchiveFreed = 0;
    g_BattleSpriteArchiveBuffer = BattleLoadArchiveEntry(6);
}

void BattleFreeTextArchive(void) {
    if (g_BattleSpriteArchiveFreed == 0) {
        ArchiveCdDataSync(0);
        HeapFree(g_BattleSpriteArchiveBuffer);
        g_BattleSpriteArchiveFreed = 1;
    }
}

void BattleFreeScreenBuffer(s32 a0) {
    if (a0 != 0) {
        g_BattleScreenImageBuffer = 0;
    }
    if (g_BattleScreenImageBuffer != 0) {
        HeapFree(g_BattleScreenImageBuffer);
        g_BattleScreenImageBuffer = 0;
    }
}

void BattleRestoreScreenImage(void) {
    RECT rect;
    u16 *pPixels;

    if (g_BattleScreenImageBuffer != 0) {
        DrawSync(0);
        rect.x = 0x140;
        rect.y = 0x100;
        rect.w = 0x140;
        rect.h = 0xDA;
        // The filter reads one 320-pixel row below each pixel, so it stops at
        // the last of the 218 rows.
        pPixels = (u16 *)g_BattleScreenImageBuffer;
        func_8007313C(pPixels, pPixels + 217 * 320);
        LoadImage(&rect, g_BattleScreenImageBuffer);
    }
}

void BattleSaveScreenImage(void) {
    RECT rect;

    if (g_BattleScreenImageBuffer == 0) {
        HeapToggleErrorHandler(1);
        g_BattleScreenImageBuffer = HeapAlloc(0x22100, 0);
        HeapToggleErrorHandler(0);
    }
    DrawSync(0);
    rect = g_BattleDrawBuffers[((g_BattleDrawBufferIndex + 1) & 1)].rect;
    MoveImage(&rect, 0x140, 0x100);
    if (g_BattleScreenImageBuffer != 0) {
        StoreImage(&rect, g_BattleScreenImageBuffer);
    }
    DrawSync(0);
}

void BattleStartTextScreen(s32 a0) {
    SoundStopAllElements();
    BattlePlaySoundEffectById(0x1F);
    if (a0 == 1) {
        g_BattleCurrentTextList = 0;
        BattleSetCurrentTextList(0);
        D_800915BE = 0;
        D_80091636 = 1;
        do { } while (0); // the barrier keeps the two shared tails merging at the call
        BattleSetScreenMode(0);
        g_BattleTextWindowVisible = 1;
        g_BattleScreenRestoreCount = 0;
        g_BattleScreenRestorePending = 1;
        BattleSaveScreenImage();
    } else if (a0 == 2) {
        g_BattleCurrentTextList = 0;
        BattleSetCurrentTextList(7);
        D_80091762 = 1;
        do { } while (0);
        BattleSetScreenMode(0);
        g_BattleTextWindowVisible = 1;
        g_BattleScreenRestoreCount = 0;
        g_BattleScreenRestorePending = 1;
        BattleSaveScreenImage();
    }
    else {
        BattleResetTextListState();
    }
}


void BattleResetTextPrimQueue(void) {
    g_BattleTextPrimCount = 0;
}

extern u8 *g_BattleTextPrimBuffers[];
extern s32 g_BattleScreenCopyRequest;
extern DR_MOVE g_BattleScreenMovePrims[];
extern DR_TPAGE g_BattleTextWindowTpagePrims[];

void BattleDrawTextPrims(u8 *p) {
    s32 i;
    u8 *pEntry;
    RECT rect;

    pEntry = g_BattleTextPrimBuffers[g_BattleDrawBufferIndex];
    for (i = 0; i < g_BattleTextPrimCount; i++, pEntry += 0x28) {
        AddPrim(p, pEntry);
    }
    g_BattleTextPrimCount = 0;
    BattleDrawTextSprites(p);
    if (((g_BattleCurrentTextList != 0) && (g_BattleTextWindowVisible != 0)) || (g_BattleScreenCopyRequest != 0)) {
        if (g_BattleCurrentTextList != 0) {
            AddPrim(p, &g_BattleCurrentTextList->frames[g_BattleDrawBufferIndex]);
            SetDrawTPage(&g_BattleTextWindowTpagePrims[g_BattleDrawBufferIndex], 0, 0, GetTPage(0, 2, 0, 0));
            AddPrim(p, &g_BattleTextWindowTpagePrims[g_BattleDrawBufferIndex]);
        }
        rect.x = 0x140;
        rect.y = 0x100;
        rect.w = 0x140;
        rect.h = 0xDA;
        SetDrawMove(&g_BattleScreenMovePrims[g_BattleDrawBufferIndex], &rect,
                    g_BattleDrawBuffers[g_BattleDrawBufferIndex].rect.x,
                    g_BattleDrawBuffers[g_BattleDrawBufferIndex].rect.y);
        AddPrim(p, &g_BattleScreenMovePrims[g_BattleDrawBufferIndex]);
    }
    g_BattleScreenCopyRequest = 0;
}

extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

typedef struct {
    /* 0x00 */ u_long *pData;
    /* 0x04 */ SPRT sprt[2];
    /* 0x2C */ u16 width;
    /* 0x2E */ u16 x;
} Misc8Text; // Size: 0x30

extern Misc8Text g_BattleCenteredTextSprites[2];
extern DR_TPAGE g_BattleTextSpriteTpagePrims[];

void BattleInitCenteredTextSprites(void) {
    u_long *pData;
    Misc8Text *pText;

    pData = HeapAlloc(0x6B4, 0);
    pText = g_BattleCenteredTextSprites;
    pText[1].pData = pData;
    pText[0].pData = pData;
    // the sprite geometry is written packed: x0,y0 as one word and u0,v0 as one
    // halfword, the way setXY0/setUV0 do it
    *(u32 *)&pText[0].sprt[0].x0 = 0xB40000;
    *(u16 *)&pText[0].sprt[0].u0 = 0x3000;
    SetSprt(&pText[0].sprt[0]);
    SetShadeTex(&pText[0].sprt[0], 1);
    pText[0].sprt[0].h = 0xD;
    pText[0].sprt[0].clut = g_SystemPalette1;
    pText[0].sprt[1] = pText[0].sprt[0];
    *(u32 *)&pText[1].sprt[0].x0 = 0xC30000;
    *(u16 *)&pText[1].sprt[0].u0 = 0x3000;
    SetSprt(&pText[1].sprt[0]);
    SetShadeTex(&pText[1].sprt[0], 1);
    pText[1].sprt[0].h = 0xD;
    pText[1].sprt[0].clut = g_SystemPalette2;
    pText[1].sprt[1] = pText[1].sprt[0];
    SetDrawTPage(&g_BattleTextSpriteTpagePrims[0], 0, 0, GetTPage(0, 0, 0x140, 0x30));
    g_BattleTextSpriteTpagePrims[1] = g_BattleTextSpriteTpagePrims[0];
}


void BattleRenderCenteredText(Misc8Text *p, s32 entry, s32 flag) {
    s32 len;

    len = SystemRenderStringEntry(GetStringEntry(g_BattleMessageStringTable, entry), p->pData, 0x3F, flag);
    p->width = len;
    p->x = (0x140 - len) / 2;
}

void BattleSetTextSpriteEntry(s32 entry, s32 mode) {
    RECT rect;

    if (mode == 0) {
        if (entry == g_BattleTextSpriteEntry0) {
            return;
        }
        g_BattleTextSpriteEntry0 = entry;
        BattleRenderCenteredText(&g_BattleCenteredTextSprites[0], entry, 0);
    } else {
        if (entry == g_BattleTextSpriteEntry1) {
            return;
        }
        g_BattleTextSpriteEntry1 = entry;
        BattleRenderCenteredText(&g_BattleCenteredTextSprites[1], entry, 1);
    }
    rect.x = 0x140;
    rect.y = 0x30;
    rect.w = 0x42;
    rect.h = 0xD;
    LoadImage(&rect, g_BattleCenteredTextSprites[0].pData);
}

void BattleDrawTextSprites(u8 *p) {
    Misc8Text *pText;

    // A text object holds one sprite per draw buffer; the indexed sprite is
    // moved to the object's centred x and given its rendered width.
    if (g_BattleTextSpriteEntry0 != 0) {
        pText = &g_BattleCenteredTextSprites[0];
        pText->sprt[g_BattleDrawBufferIndex].w = pText->width;
        pText->sprt[g_BattleDrawBufferIndex].x0 = pText->x;
        AddPrim(p, &pText->sprt[g_BattleDrawBufferIndex]);
    }
    if (g_BattleTextSpriteEntry1 != 0) {
        pText = &g_BattleCenteredTextSprites[1];
        pText->sprt[g_BattleDrawBufferIndex].w = pText->width;
        pText->sprt[g_BattleDrawBufferIndex].x0 = pText->x;
        AddPrim(p, &pText->sprt[g_BattleDrawBufferIndex]);
    }
    if (g_BattleTextSpriteEntry0 | g_BattleTextSpriteEntry1) {
        AddPrim(p, &g_BattleTextSpriteTpagePrims[g_BattleDrawBufferIndex]);
    }
}

void BattleLayoutTextWindowList(BattleTextList *p) {
    s32 i;
    s16 w;
    s32 max;
    BattleTextEntry *pEntry;
    BattleTextFrame *pFrame;

    max = 0;
    for (i = 0; i < p->count; i++) {
        pEntry = &p->pEntries[i];
        w = BattleGetTextWidth(pEntry->pData) / 2;
        pEntry->xOffset = w;
        max = max < w ? w : max;
    }
    i = 0;
    p->frames[0].code = 3;
    p->frames[0].w = max * 2 + 0x14;
    p->frames[0].x = 0x96 - max;
    p->row = 0;
    p->frames[0].mode = 0x60102020;
    p->y = 0x6D - p->count * 10;
    ((u8 *)&p->frames[0].mode)[3] |= 2;
    p->frames[0].h = p->count * 20 + 0x14;
    p->frames[0].y = p->y - 0xA;
    pFrame = &p->frames[0];
    for (; i < p->count; i++) {
        pEntry = &p->pEntries[i];
        if (pEntry->flags & 2) {
            pEntry->xOffset = max;
        }
    }
    if (p->extraWidth != 0) {
        pFrame->w += p->extraWidth;
        pFrame->x -= p->extraWidth / 2;
        p->x = 0xA0 - p->extraWidth / 2;
        p->x -= max;
    }
    p->frames[1] = p->frames[0];
    BattleSetTextColor(0);
}

void BattleInitTextLists(void) {
    u32 i;
    BattleTextList *p;

    for (i = 0, p = g_BattleTextLists; i < 8; i++, p++) {
        BattleLayoutTextWindowList(p);
    }
    g_BattleCurrentTextList = 0;
    g_BattleCursorIndex0 = 0;
    g_BattleCursorIndex1 = 1;
    BattleInitCenteredTextSprites();
}

void BattleDrawTextList(BattleTextList *p) {
    s32 i;

    if (p == 0) {
        return;
    }
    g_BattleCurrentTextList = p;
    if (p == g_BattleOptionTextList) {
        return;
    }
    if (p->extraWidth != 0) {
        BattleSetTextCursor(p->x, p->y);
    } else {
        BattleSetTextCursor(0xA0, p->y);
    }
    for (i = 0; i < p->count; i++) {
        BattleSetRowTextColor(p, i);
        if (p->extraWidth != 0) {
            BattleDrawTextLeft(p->pEntries[i].pData);
        } else {
            BattleDrawTextRightOffset(p->pEntries[i].pData, p->pEntries[i].xOffset);
        }
    }
    if (p->pCallback != 0) {
        p->pCallback(p);
    }
}
/* 99.63%: the duplicated call (one per arm of the extraWidth test) is what makes
   gcc cross-jump the two `jal BattleSetTextCursor` tails into one and keep retail's
   `lh a1,0x14` in BOTH arms; assigning a1 in both arms and calling once merges
   the load instead (95.15%).
   Only difference left: retail sets i=0 before the count load (so `move s1,zero`
   lands in the BattleSetTextCursor jal delay slot) and sinks off=0 after the `blez`
   (in the fall-through, after the `move a0,s0` that fills the blez slot); ours
   emits off=0 first (into the jal delay slot) and i=0 into the blez delay slot.
   Tried: `i = 0;` before the for, `i = 0;` immediately after the call with
   `for (off = 0; ...)`, the init clause written `i = 0, off = 0` and
   `off = 0, i = 0`, both declaration orders, and a while loop (97.47%) - the
   two loop-initialisation insns keep retail's order inverted.
void BattleDrawTextList(BattleTextList *p) {
    s32 i;
    s32 off;

    if (p == 0) {
        return;
    }
    g_BattleCurrentTextList = (s32 *)p;
    if ((s32 *)p == (s32 *)g_BattleOptionTextList) {
        return;
    }
    if (p->extraWidth != 0) {
        BattleSetTextCursor(p->x, p->y);
    } else {
        BattleSetTextCursor(0xA0, p->y);
    }
    i = 0;
    for (off = 0; i < p->count; i++) {
        BattleSetRowTextColor(p, i);
        if (p->extraWidth != 0) {
            BattleDrawTextLeft((u8 *)*(u32 *)(off + p->pEntries + 4));
        } else {
            BattleDrawTextRightOffset((u8 *)*(u32 *)(off + p->pEntries + 4), *(s16 *)(off + p->pEntries + 0x12));
        }
        off += 0x14;
    }
    if (p->pCallback != 0) {
        p->pCallback(p);
    }
}
*/

/* s32 (never used) is load-bearing: a void return lets reorg steal the branch
   target's first instruction into the final `beqz` slot, retail has a nop. */
s32 BattleUpdateTextPadInput(BattleTextList *p, s32 padIndex) {
    BattleTextEntry *pEntry = &p->pEntries[p->row];
    BattleTextList *pCur;
    void (*pCallback)(void *);
    s32 sfx;
    s32 type;
    s32 stickY;
    s32 stickX;

    if (padIndex == 0 || p == &g_BattleTextLists[7]) {
        BattleSetTextSpriteEntry(g_BattleTextEntryCursor, 0);
        g_BattleTextEntryCursor = p->pEntries[p->row].messageId;
    }
    g_BattleCurrentTextPadIndex = padIndex;
    type = 0;
    if (padIndex == 1) {
        g_BattleButtonState = g_C2ButtonState;
        g_BattleButtonStatePressedOnce = g_C2ButtonStatePressedOnce;
        g_BattleButtonStateReleased = g_C2ButtonStateReleased;
        type = ControllerGetType(1);
        stickY = g_C2LeftStickYAxis - 0x80;
        stickX = g_C2LeftStickXAxis - 0x80;
    } else if (padIndex == 0) {
        g_BattleButtonState = g_C1ButtonState;
        g_BattleButtonStatePressedOnce = g_C1ButtonStatePressedOnce;
        g_BattleButtonStateReleased = g_C1ButtonStateReleased;
        type = ControllerGetType(0);
        stickY = g_C1LeftStickYAxis - 0x80;
        stickX = g_C1LeftStickXAxis - 0x80;
    }
    if ((type == 3 || type == 4) && SquareRoot0(stickY * stickY + stickX * stickX) >= 0x41) {
        if (g_BattleStickPushSoundPlayed == 0) {
            BattlePlaySoundEffectById(0x24);
            g_BattleStickPushSoundPlayed = 1;
        }
    } else {
        g_BattleStickPushSoundPlayed = 0;
    }
    pCallback = pEntry->pCallback;
    if (pCallback != 0 && ((pEntry->flags & 1) || (g_BattleButtonStateReleased & CTRL_BTN_CIRCLE))) {
        if ((pEntry->flags & 1) == 0) {
            BattlePlaySoundEffectById(0x21);
        }
        pCallback(pEntry->pCallbackArg);
    }
    pCur = g_BattleCurrentTextList;
    if (pCur != 0) {
        if (g_BattleButtonStateReleased & CTRL_BTN_CROSS) {
            if (padIndex != 1 || p != &g_BattleTextLists[6]) {
                if (pCur == &g_BattleTextLists[p->nextListIndex] && (p->nextListIndex < 5 || p->nextListIndex > 6)) {
                    sfx = 0x24;
                } else {
                    BattleSetCurrentTextList(p->nextListIndex);
                    sfx = 0x22;
                }
                BattlePlaySoundEffectById(sfx);
            }
        }
        if (g_BattleButtonStatePressedOnce & CTRL_BTN_UP) {
            BattlePlaySoundEffectById(0x1E);
            p->row--;
            if (p->row < 0) {
                p->row = p->count - 1;
            }
            if (p->pEntries[p->row].flags & 4) {
                p->row--;
            }
        }
        if (g_BattleButtonStatePressedOnce & CTRL_BTN_DOWN) {
            BattlePlaySoundEffectById(0x1E);
            p->row++;
            if (p->pEntries[p->row].flags & 4) {
                p->row++;
            }
        }
        if (p->row < 0) {
            p->row = p->count - 1;
        }
        if (p->row >= p->count) {
            p->row = 0;
        }
        if (p->pEntries[p->row].flags & 4) {
            p->row--;
        }
    }
}

// PARK 99.70% (744 bytes = retail exact, 186 insns = retail count). Retail is
//    744 bytes (objdiff symbol size and the .s agree: 0x11F54..0x1223C).
//    The body is instruction-identical except THREE BYTES in case 2: retail puts the
//    first ternary result in $v0 (li v0,5 / li v0,4 / sh v0,D_800916E2), ours in
//    $a0. culprit.sh reports exactly "func_80081A44 3 bytes 0x80081AEA-0x80081AF6",
//    so the function cannot link.
//    MECHANISM, measured from the allocator dumps rather than guessed: the HI pseudo
//    holding the ternary value cannot take $v0 - m.i.greg says "84 conflicts: 84 2 3
//    29" and "84 in 4" ($a0). The conflict with $v0 comes from sched1, not from the
//    C: at combine time block 13 is [94 li v1,1][96 sw latched][101 lw flags][102
//    andi][502 li 5][104 bnez], but in m.i.sched the ready list shows 502 with
//    scheduler priority 1 (no intra-block user - its only reader is the sh in the
//    next block), so the backward pass drops it into the FIRST slot: post-sched1
//    order is [502][101][94][96][102][104] and the HI pseudo is live across the lw,
//    which is what bars $v0. Retail sched1 left 502 in the delay slot. Reproduce:
//    tools/gcc-2.7.2-psx/cc1 -O2 -G8 -da on build/src/battling/main/misc8.c.i in the
//    build container (writes m.i.greg / m.i.lreg next to the input).
//    What DID work in an earlier round (all needed for the 744 bytes):
//    - the second nextListIndex read must be a STATEMENT through a volatile cast,
//      flags = *(volatile s32 &g_BattleSlotConfirmFlags); a plain read is CSE d
//      across the store to the other global and never reloaded (2 insns short);
//    - the FIRST store must be volatile too, or sched2 sinks it below the reload
//      (740 bytes, 97.46-97.74);
//    - the second call is BattleUpdateTextPadInput(g_BattleOptionTextList + 1, 1)
//      for retail single addiu a0,s0,0x3C (the typed list costs one more insn);
//    - the two nextListIndex stores must be the D_ globals D_800916E2/D_8009171E
//      (g_BattleTextLists[5].nextListIndex makes cc1 materialise the address).
//    Measured, all worse or equal: no flags local (736, 91.37); volatile inline in
//    the second ternary (748, 94.65); volatile on the first read only (752); flags
//    read twice through volatile (748, 98.60); (x & 1) ? 5 : 4 arm order (744,
//    99.37); plain first store (740, 97.46); the latched store moved after the
//    ternary (744, 96.91); a (u16) cast on the ternary (744, 99.70, unchanged);
//    one s32 flags local reused as the ternary result (flags = (flags & 1) ? 5 : 4,
//    then store flags) to force the load and the arms into ONE pseudo: 748, 98.90,
//    and the andi then reads v1 - the pseudo sharing costs an insn and does not
//    clear the conflict.
//    Preset screens: gcc-2.7.2-cdk 99.70, -O3 99.70, -O1 81.79, -Os compile-failed,
//    gcc-2.6.0 rejects the body - the preset is not the way out.
//    Permuter: base=45 best=45 over 1320 iterations this round (its score is
//    dominated by the branch-label renaming every diff shows, so it cannot see a
//    one-register move); an earlier 3-minute run reported the same base and best.
//    Next idea if anyone retries: raise insn 502 scheduler priority (a reader in the
//    same block) or make the HI pseudo share the load pseudo without adding an insn;
//    no spelling tried so far moves 502 out of the block head.
/*
void func_80081A44(void) {
    switch (g_BattleMode) {
    case 1:
        switch (g_BattleSlotConfirmFlags) {
        case 0:
            g_BattleSlotConfirmLatched = 0;
            BattleUpdateTextPadInput(g_BattleOptionTextList, 0);
            break;
        case 1:
            g_BattleSlotConfirmLatched = 1;
            BattleUpdateTextPadInput(&g_BattleTextLists[6], 0);
            break;
        case 3:
            g_BattleStateAdvanceRequested = 1;
            break;
        }
        g_BattlePlayer1ComControlled = 0;
        g_BattlePlayer2ComControlled = 1;
        break;
    case 2: {
        s32 flags;

        g_BattleSlotConfirmLatched = 1;
        *(volatile u16 *)&D_800916E2 = (g_BattleSlotConfirmFlags & 1) == 0 ? 4 : 5;
        flags = *(volatile s32 *)&g_BattleSlotConfirmFlags;
        D_8009171E = (flags & 2) == 0 ? 4 : 5;
        BattleUpdateTextPadInput(g_BattleOptionTextList, 0);
        BattleUpdateTextPadInput(g_BattleOptionTextList + 1, 1);
        if (g_BattleSlotConfirmFlags == 3) {
            g_BattleStateAdvanceRequested = 1;
        }
        g_BattlePlayer1ComControlled = 0;
        g_BattlePlayer2ComControlled = 0;
        break;
    }
    case 3:
        switch (g_BattleSlotConfirmFlags) {
        case 0:
            g_BattleSlotConfirmLatched = 0;
            BattleUpdateTextPadInput(g_BattleOptionTextList, 0);
            break;
        case 1:
            g_BattleSlotConfirmLatched = 1;
            BattleUpdateTextPadInput(&g_BattleTextLists[6], 0);
            break;
        case 3:
            g_BattleStateAdvanceRequested = 1;
            break;
        }
        g_BattlePlayer1ComControlled = 1;
        g_BattlePlayer2ComControlled = 1;
        break;
    case 4:
        switch (g_BattleSlotConfirmFlags) {
        case 0:
            g_BattleSlotConfirmLatched = 0;
            BattleUpdateTextPadInput(g_BattleOptionTextList, 0);
            break;
        case 1:
            g_BattleSlotConfirmLatched = 1;
            BattleUpdateTextPadInput(&g_BattleTextLists[6], 0);
            break;
        case 3:
            g_BattleStateAdvanceRequested = 1;
            break;
        }
        g_BattlePlayer1ComControlled = 0;
        g_BattlePlayer2ComControlled = 1;
        break;
    case 5:
        switch (g_BattleSlotConfirmFlags) {
        case 0:
            BattleUpdateTextPadInput(g_BattleOptionTextList, 0);
            break;
        case 1:
            if (g_BattleCursorIndex1 == 3) {
                g_BattleSlotConfirmFlags = g_BattleCursorIndex1;
            } else {
                g_BattleCursorIndex1++;
                if (g_BattleEntryCount < g_BattleCursorIndex1) {
                    g_BattleCursorIndex1 -= g_BattleEntryCount;
                }
                if (g_BattleCursorIndex1 < 0) {
                    g_BattleCursorIndex1 += g_BattleEntryCount;
                }
            }
            break;
        case 3:
            g_BattleStateAdvanceRequested = 1;
            break;
        }
        g_BattlePlayer1ComControlled = 0;
        g_BattlePlayer2ComControlled = 1;
        break;
    }
    BattleDrawVibrationOptions();
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081A44);


void BattleUpdateTextWindow(void) {
    s32 unk[2]; /* unused: retail's frame keeps 8 bytes of locals here */
    BattleTextList *p;

    if (g_BattleScreenRestorePending != 0) {
        BattleRestoreScreenImage();
        g_BattleScreenRestorePending = 0;
    }
    ControllerMergeStates();
    p = g_BattleCurrentTextList;
    if (p == g_BattleOptionTextList) {
        func_80081A44();
    } else {
        if (p != 0) {
            if (g_C1ButtonStateReleased & 1) {
                if (g_BattleScreenRestoreCount < 5) {
                    g_BattleScreenRestoreCount++;
                    BattleRestoreScreenImage();
                }
            }
            BattleUpdateTextPadInput(p, g_BattleTextPadIndex);
        }
        BattleDrawTextList(g_BattleCurrentTextList);
    }
}

void BattleSetScreenTileGrey(void) {
    u16 v = g_BattleScreenHeight;

    g_BattleDrawBuffers[0].tile.y0 = 0x60;
    g_BattleDrawBuffers[1].tile.y0 = 0x60;
    g_BattleDrawBuffers[0].tile.r0 = 0x7F;
    g_BattleDrawBuffers[0].tile.g0 = 0x7F;
    g_BattleDrawBuffers[0].tile.b0 = 0x7F;
    g_BattleDrawBuffers[1].tile.r0 = 0x7F;
    g_BattleDrawBuffers[1].tile.g0 = 0x7F;
    g_BattleDrawBuffers[1].tile.b0 = 0x7F;
    g_BattleDrawBuffers[0].tile.h = v - 0x60;
    g_BattleDrawBuffers[1].tile.h = v - 0x60;
}

void BattleSetScreenTileBlack(void) {
    u16 v = g_BattleScreenHeight;

    g_BattleDrawBuffers[0].tile.y0 = 0;
    g_BattleDrawBuffers[1].tile.y0 = 0;
    g_BattleDrawBuffers[0].tile.r0 = 0;
    g_BattleDrawBuffers[0].tile.g0 = 0;
    g_BattleDrawBuffers[0].tile.b0 = 0;
    g_BattleDrawBuffers[1].tile.r0 = 0;
    g_BattleDrawBuffers[1].tile.g0 = 0;
    g_BattleDrawBuffers[1].tile.b0 = 0;
    g_BattleDrawBuffers[0].tile.h = v;
    g_BattleDrawBuffers[1].tile.h = v;
}

typedef struct {
    /* 0x0 */ s16 height;
    /* 0x2 */ s16 unk2;
} Misc8BattleGrid; // Size: 0x4

typedef struct {
    /* 0x00 */ SPRT sprt;
    /* 0x14 */ SPRT copies[5];
} Misc8DimSprites; // Size: 0x78

extern Misc8BattleGrid *g_BattleGridCells;
extern POLY_G4 D_80095580[2];
extern DR_TPAGE g_BattleDimTpagePrims[4];
extern u8 g_BattleFogColors;
extern Misc8DimSprites D_800955F8;
extern void BattleInitDrawEnvPair(void);
extern void func_800732AC(void *pDst, void *pSrc, s32 n);
extern void BattleInitSpanBuffers(void);

void func_80081ECC(void) {
    POLY_G4 *pQuad;
    Misc8DimSprites *pDim;
    s32 i;

    BattleInitDrawEnvPair();

    pQuad = &D_80095580[0];
    setPolyG4(pQuad);
    // each corner below is written packed: r,g as one halfword, x,y as one word
    g_BattleFogColors = 0x10;
    pQuad->g0 = 0x60;
    pQuad->b0 = 0x7F;
    *(u16 *)&pQuad->r1 = 0x6010;
    *(u16 *)&pQuad->r2 = 0x7F7F;
    *(u16 *)&pQuad->r3 = 0x7F7F;
    *(u32 *)&pQuad->x1 = 0x140;
    pQuad->b1 = 0x7F;
    pQuad->b2 = 0x7F;
    pQuad->b3 = 0x7F;
    *(u32 *)&pQuad->x0 = 0;
    *(u32 *)&pQuad->x2 = 0x600000;
    *(u32 *)&pQuad->x3 = 0x600140;
    D_80095580[1] = D_80095580[0];

    SetDrawTPage(&g_BattleDimTpagePrims[0], 0, 0, GetTPage(2, 2, 0, 0x100));
    SetDrawTPage(&g_BattleDimTpagePrims[1], 0, 0, GetTPage(2, 2, 0, 0));
    SetDrawTPage(&g_BattleDimTpagePrims[2], 0, 0, GetTPage(2, 2, 0x100, 0x100));
    SetDrawTPage(&g_BattleDimTpagePrims[3], 0, 0, GetTPage(2, 2, 0x100, 0));

    pDim = &D_800955F8;
    *(u32 *)&pDim->sprt.r0 = 0x64707070;
    setlen(&pDim->sprt, 4);
    *(u32 *)&pDim->sprt.x0 = 0;
    *(u16 *)&pDim->sprt.u0 = 0;
    *(u32 *)&pDim->sprt.w = 0xDB0080;
    setShadeTex(&pDim->sprt, 0);
    setSemiTrans(&pDim->sprt, 1);

    func_800732AC(&pDim->copies[0], &pDim->sprt, sizeof(pDim->copies));

    pDim->copies[1].u0 = pDim->copies[2].u0 = 0x80;
    pDim->copies[1].x0 = pDim->copies[2].x0 = 0x80;
    pDim->copies[3].x0 = pDim->copies[4].x0 = 0x100;
    pDim->copies[3].w = pDim->copies[4].w = 0x40;

    for (i = 0; i < BATTLE_GRID_CELL_COUNT; i++) {
        g_BattleGridCells[i].height = g_BattleGridCells[i].height * 12;
    }
    BattleInitSpanBuffers();
}
