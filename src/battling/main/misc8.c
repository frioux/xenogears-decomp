#include "common.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "system/controller.h"

extern u16 D_800915BE;
extern u16 D_80091636;
extern u8 g_BattleScreenRestoreCount;
extern u8 g_BattleScreenRestorePending;
extern u16 D_80091762;
extern u16 D_80091672;
extern u16 D_800916AE;
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
extern u16 D_8009553C;
extern u16 D_8009553E;
extern u8 g_BattleTextPadIndex;
extern u8 D_8009A1C4;
extern u8 D_8009A1C5;
extern u8 D_8009A1C6;
extern u16 D_8009A1CA;
extern u16 D_8009A1CE;
extern u8 D_8009A2BC;
extern u8 D_8009A2BD;
extern u8 D_8009A2BE;
extern u16 D_8009A2C2;
extern u16 D_8009A2C6;
extern s32 g_BattleMode;
extern s32 g_BattleSpriteArchiveBuffer;
extern s32 g_BattleStateAdvanceRequested;
extern s32 g_BattleSpriteArchiveFreed;

extern void BattleClearTextSpriteEntries(void);
extern void BattleInitTutorialMessage(void);
extern void BattleLoadSlotEntry(s32 a0, s32 a1);
extern void SoundStopAllElements(void);
extern void func_8007313C(void *pDst, void *pSrc);
extern void BattlePlaySoundEffectById(s32 arg);
extern void BattleSetScreenMode(s32 mode);
extern void BattleResetTextListState(void);
extern void BattleSetTextCursor(s16 a, s16 b);
extern void BattleSetRowTextColor(void *p, s32 arg);
extern void BattleDrawTextLeft(u8 *p);
extern void BattleDrawTextRightOffset(u8 *p, s32 dy);
extern void ControllerMergeStates(void);
typedef struct {
    /* 0x0  */ u8 unk0[4];
    /* 0x4  */ u32 unk4;
    /* 0x8  */ u8 unk8[0xA];
    /* 0x12 */ s16 unk12;
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
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1[3];
    /* 0x04 */ BattleTextEntry *pEntries;
    /* 0x08 */ s16 count;
    /* 0x0A */ u8 unkA[2];
    /* 0x0C */ void (*unkC)(void *p);
    /* 0x10 */ u8 unk10[2];
    /* 0x12 */ s16 unk12;
    /* 0x14 */ u16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u8 unk18[4];
    /* 0x1C */ BattleTextFrame frames[2]; /* current, saved */
} BattleTextList; // Size: 0x3C

extern BattleTextList g_BattleTextLists[8];
extern BattleTextList D_80091660[];
extern BattleTextList D_800916D8[];
extern BattleTextList *g_BattleCurrentTextList;
extern BattleTextList *g_BattlePrevTextList;

// Per-draw-buffer slot record, 0xF8 bytes: it starts with the screen area the
// slot's text window covers.
typedef struct {
    /* 0x00 */ RECT rect;
    /* 0x08 */ u8 unk8[0xF0];
} Misc8SlotEntry; // Size: 0xF8

extern Misc8SlotEntry g_BattleDrawBuffers[];

extern void BattleDrawTextSprites(u8 *p);
extern void func_800812BC(BattleTextList *p);
extern void BattleDrawTextList(BattleTextList *p);
extern void func_8008162C(BattleTextList *p, s32 a1);
extern void func_80081A44(void);
extern s32 BattleGetTextWidth(u32 a0);
extern void BattleSetTextColor(s32 a0);
extern void func_80080F04(void);
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
    return g_BattleCurrentTextList == D_80091660;
}

void BattleInitTextWindow(void) {
    SoundStopAllElements();
    g_BattleCurrentTextList = 0;
    BattleSetCurrentTextList(3);
    D_80091672 = 0;
    D_800916AE = 0;
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

    if (g_BattleScreenImageBuffer != 0) {
        DrawSync(0);
        rect.x = 0x140;
        rect.y = 0x100;
        rect.w = 0x140;
        rect.h = 0xDA;
        func_8007313C(g_BattleScreenImageBuffer, (u8 *)g_BattleScreenImageBuffer + 0x21E80);
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080C48);
/* PARK 95.60% (204 bytes vs retail 200).  Best body: the tail duplicated in
   both arms, which cross-jumps to retail's block order
   [test1][A][j tail][test2][B][tail][else].  Only difference: retail sets the
   BattleSetScreenMode argument with one `addu a0,zero,zero` inside the shared
   tail, in the jal delay slot, and leaves a plain nop in A's `j` slot; ours
   emits it twice - once in A's `j` delay slot and once in B before its
   D_80091762 store - and a nop in the jal slot (2 extra insns).
   Also tried: a switch with break / default-return, which lays the case bodies
   out before the shared tail and scores 66.70%; the tail written once after
   the if/else-if chain with the else returning first (79.80%, gcc moves the
   else above the tail); `s32 mode = 0` as the argument; gcc 2.7.2-cdk (85.36%).
   The duplicated-tail shape below is the one to permute.
void func_80080C48(s32 a0) {
    SoundStopAllElements();
    BattlePlaySoundEffectById(0x1F);
    if (a0 == 1) {
        g_BattleCurrentTextList = 0;
        BattleSetCurrentTextList(0);
        D_800915BE = 0;
        D_80091636 = 1;
        BattleSetScreenMode(0);
        g_BattleTextWindowVisible = 1;
        g_BattleScreenRestoreCount = 0;
        g_BattleScreenRestorePending = 1;
        BattleSaveScreenImage();
    } else if (a0 == 2) {
        g_BattleCurrentTextList = 0;
        BattleSetCurrentTextList(7);
        D_80091762 = 1;
        BattleSetScreenMode(0);
        g_BattleTextWindowVisible = 1;
        g_BattleScreenRestoreCount = 0;
        g_BattleScreenRestorePending = 1;
        BattleSaveScreenImage();
    } else {
        BattleResetTextListState();
    }
}
*/

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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080F04);
/* 83.89%: both sprite halves match except the four sprite-scaling stores.
   Retail writes x0/y0 as one sw and u0/v0 as one sh to the ABSOLUTE symbols
   D_8009551C/D_80095520 (and D_8009554C/D_80095550 for the second half), and
   h/clut to D_80095526/D_80095522: the retail immediates are
   `sh $s3,%lo(D_80095526)($at)` (0x5526), so the address is not formed as
   base+0x16 off g_BattleCenteredTextSprites.  Spelling them as extern scalars gives those
   instruction shapes but only 63.38%: gcc then sinks all four of those stores
   BELOW the 0x14-byte struct copy (they do not alias it as far as it knows),
   while retail keeps h/clut before the copy.  Writing them as sprite fields
   (pSprt->x0 ...) keeps retail's order but uses s0-relative addressing and
   fails to merge the two halfword pairs.
   Also missing: the last two words are reloaded through %hi(g_BattleTextSpriteTpagePrims)
   instead of `lw $v1,4($s0)`.  Explicit statement order has no effect (the
   copy is emitted first and the scalar stores after it either way).
void func_80080F04(void) {
    u_long *pData;
    Misc8Text *p;
    SPRT *pSprt;

    pData = HeapAlloc(0x6B4, 0);
    p = g_BattleCenteredTextSprites;
    D_80095540.pData = pData;
    p[0].pData = pData;
    pSprt = &p[0].sprt[0];
    pSprt->x0 = 0;
    pSprt->y0 = 0xB4;
    pSprt->u0 = 0;
    pSprt->v0 = 0x30;
    SetSprt(pSprt);
    SetShadeTex(pSprt, 1);
    pSprt->h = 0xD;
    pSprt->clut = g_SystemPalette1;
    p[0].sprt[1] = p[0].sprt[0];
    pSprt = &p[1].sprt[0];
    pSprt->x0 = 0;
    pSprt->y0 = 0xB4;
    pSprt->u0 = 0;
    pSprt->v0 = 0x30;
    SetSprt(pSprt);
    SetShadeTex(pSprt, 1);
    pSprt->h = 0xD;
    pSprt->clut = g_SystemPalette2;
    p[1].sprt[1] = p[1].sprt[0];
    SetDrawTPage((DR_TPAGE *)g_BattleTextSpriteTpagePrims, 0, 0, GetTPage(0, 0, 0x140, 0x30));
    D_80095578 = *(u32 *)g_BattleTextSpriteTpagePrims;
    D_8009557C = *((u32 *)g_BattleTextSpriteTpagePrims + 1);
}
*/

typedef struct {
    /* 0x00 */ u_long *pData;
    /* 0x04 */ SPRT sprt[2];
    /* 0x2C */ s16 width;
    /* 0x2E */ s16 x;
} Misc8Text; // Size: 0x30

extern Misc8Text g_BattleCenteredTextSprites[2];
extern Misc8Text D_80095540;
extern u16 D_8009556C;
extern u16 D_8009556E;
extern DR_TPAGE g_BattleTextSpriteTpagePrims[];

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
        BattleRenderCenteredText(&D_80095540, entry, 1);
    }
    rect.x = 0x140;
    rect.y = 0x30;
    rect.w = 0x42;
    rect.h = 0xD;
    LoadImage(&rect, g_BattleCenteredTextSprites[0].pData);
}

void BattleDrawTextSprites(u8 *p) {
    u16 off;
    u8 *pBase;

    if (g_BattleTextSpriteEntry0 != 0) {
        pBase = (u8 *)&g_BattleCenteredTextSprites[0];
        off = g_BattleDrawBufferIndex * 0x14;
        *(u16 *)(pBase + off + 0x14) = D_8009553C;
        *(u16 *)(pBase + off + 0xC) = D_8009553E;
        AddPrim(p, pBase + (off + 4));
    }
    if (g_BattleTextSpriteEntry1 != 0) {
        pBase = (u8 *)&D_80095540;
        off = g_BattleDrawBufferIndex * 0x14;
        *(u16 *)(pBase + off + 0x14) = D_8009556C;
        *(u16 *)(pBase + off + 0xC) = D_8009556E;
        AddPrim(p, pBase + (off + 4));
    }
    if (g_BattleTextSpriteEntry0 | g_BattleTextSpriteEntry1) {
        AddPrim(p, &g_BattleTextSpriteTpagePrims[g_BattleDrawBufferIndex]);
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_800812BC);
/* 77.44% (492 bytes vs retail 496).  Shape fully decoded from the asm and
   verified: two loops over p->count at 0x14 stride through p->pEntries;
   loop 1 does entry->unk12 = BattleGetTextWidth(entry->unk4) / 2 and keeps the
   running max, loop 2 stores max into entry->unk12 for the entries whose
   unk0 byte has bit 1; then the frame header stores, the p->unk0 width
   fixups, a 16-byte copy of the frame at 0x1C into 0x2C (frames[1] =
   frames[0] gives retail's four lw then four sw) and BattleSetTextColor(0).
   Learned: max must be s32, not s16 - as s16 gcc sign-extends max before
   the compare where retail sign-extends the fresh quotient.
   Remaining levers: (a) retail materialises p + 0x1C into $a0 once, before
   the unk14 store, and reaches frame.x / frame.w in the tail block as
   0x8($a0) / 0xC($a0), while ours keeps every frame access s1-relative -
   an untried `BattleTextFrame *pFrame = &p->frames[0];` pointer local used in
   the tail block; (b) retail's max = 0 lands in the blez delay slot while
   ours hoists it into the prologue, and i/off keep swapping register
   roles; (c) the order of the count reload, the lhu of unk14 and the
   0x2A / 0x26 stores differs.
   Note this unit's original compiler merges adjacent constant stores (see
   the func_80080F04 and func_80081ECC parks), which is why 0x20 here is a
   single sw of 0x60102020.
void func_800812BC(BattleTextList *p) {
    s32 off;
    s32 i;
    s16 w;
    s32 max;
    BattleTextEntry *pEntry;

    max = 0;
    for (i = 0, off = 0; i < p->count; i++, off += 0x14) {
        pEntry = (BattleTextEntry *)(p->pEntries + off);
        w = BattleGetTextWidth(pEntry->unk4) / 2;
        pEntry->unk12 = w;
        if (max < w) {
            max = w;
        }
    }
    i = 0;
    p->frames[0].code = 3;
    p->frames[0].w = max * 2 + 0x14;
    p->frames[0].x = 0x96 - max;
    p->unk12 = 0;
    p->frames[0].mode = 0x60102020;
    p->unk14 = 0x6D - p->count * 10;
    ((u8 *)&p->frames[0].mode)[3] |= 2;
    p->frames[0].h = p->count * 20 + 0x14;
    p->frames[0].y = p->unk14 - 0xA;
    for (off = 0; i < p->count; i++, off += 0x14) {
        pEntry = (BattleTextEntry *)(p->pEntries + off);
        if (pEntry->unk0[0] & 2) {
            pEntry->unk12 = max;
        }
    }
    if (p->unk0 != 0) {
        p->frames[0].w += p->unk0;
        p->frames[0].x -= p->unk0 / 2;
        p->unk16 = 0xA0 - p->unk0 / 2 - max;
    }
    p->frames[1] = p->frames[0];
    BattleSetTextColor(0);
}*/

void BattleInitTextLists(void) {
    u32 i;
    BattleTextList *p;

    for (i = 0, p = g_BattleTextLists; i < 8; i++, p++) {
        func_800812BC(p);
    }
    g_BattleCurrentTextList = 0;
    g_BattleCursorIndex0 = 0;
    g_BattleCursorIndex1 = 1;
    func_80080F04();
}

void BattleDrawTextList(BattleTextList *p) {
    s32 i;

    if (p == 0) {
        return;
    }
    g_BattleCurrentTextList = p;
    if (p == D_800916D8) {
        return;
    }
    if (p->unk0 != 0) {
        BattleSetTextCursor(p->unk16, p->unk14);
    } else {
        BattleSetTextCursor(0xA0, p->unk14);
    }
    for (i = 0; i < p->count; i++) {
        BattleSetRowTextColor(p, i);
        if (p->unk0 != 0) {
            BattleDrawTextLeft((u8 *)p->pEntries[i].unk4);
        } else {
            BattleDrawTextRightOffset((u8 *)p->pEntries[i].unk4, p->pEntries[i].unk12);
        }
    }
    if (p->unkC != 0) {
        p->unkC(p);
    }
}
/* 99.63%: the duplicated call (one per arm of the unk0 test) is what makes
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
    if ((s32 *)p == (s32 *)D_800916D8) {
        return;
    }
    if (p->unk0 != 0) {
        BattleSetTextCursor(p->unk16, p->unk14);
    } else {
        BattleSetTextCursor(0xA0, p->unk14);
    }
    i = 0;
    for (off = 0; i < p->count; i++) {
        BattleSetRowTextColor(p, i);
        if (p->unk0 != 0) {
            BattleDrawTextLeft((u8 *)*(u32 *)(off + p->pEntries + 4));
        } else {
            BattleDrawTextRightOffset((u8 *)*(u32 *)(off + p->pEntries + 4), *(s16 *)(off + p->pEntries + 0x12));
        }
        off += 0x14;
    }
    if (p->unkC != 0) {
        p->unkC(p);
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_8008162C);

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
    if (p == D_800916D8) {
        func_80081A44();
    } else {
        if (p != 0) {
            if (g_C1ButtonStateReleased & 1) {
                if (g_BattleScreenRestoreCount < 5) {
                    g_BattleScreenRestoreCount++;
                    BattleRestoreScreenImage();
                }
            }
            func_8008162C(p, g_BattleTextPadIndex);
        }
        BattleDrawTextList(g_BattleCurrentTextList);
    }
}

void BattleSetScreenTileGrey(void) {
    u16 v = g_BattleScreenHeight;

    D_8009A1CA = 0x60;
    D_8009A2C2 = 0x60;
    D_8009A1C4 = 0x7F;
    D_8009A1C5 = 0x7F;
    D_8009A1C6 = 0x7F;
    D_8009A2BC = 0x7F;
    D_8009A2BD = 0x7F;
    D_8009A2BE = 0x7F;
    D_8009A1CE = v - 0x60;
    D_8009A2C6 = v - 0x60;
}

void BattleSetScreenTileBlack(void) {
    u16 v = g_BattleScreenHeight;

    D_8009A1CA = 0;
    D_8009A2C2 = 0;
    D_8009A1C4 = 0;
    D_8009A1C5 = 0;
    D_8009A1C6 = 0;
    D_8009A2BC = 0;
    D_8009A2BD = 0;
    D_8009A2BE = 0;
    D_8009A1CE = v;
    D_8009A2C6 = v;
}

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
} Misc8BattleGrid; // Size: 0x4

typedef struct {
    /* 0x00 */ SPRT sprt;
    /* 0x14 */ SPRT copies[5];
} Misc8DimSprites; // Size: 0x78

extern Misc8BattleGrid g_BattleGridCells[];
extern POLY_G4 D_80095580[2];
extern DR_TPAGE D_800955C8[4];
extern Misc8DimSprites D_800955F8;
extern void BattleInitDrawEnvPair(void);
extern void func_800732AC(void *pDst, void *pSrc, s32 n);
extern void func_80087830(void);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081ECC);
/* PARK 70.16% (752 bytes vs retail 684).  The whole geometry of the function
   is recovered and every block matches except the store WIDTHS of the first
   two packets: retail combines adjacent field writes into wider stores and our
   gcc 2.7.2 does not.  Retail writes the quad colours as
   `li v0,0x6010 ; sh v0,D_8009558C` (r1,g1 merged), `li v0,0x7F7F ; sh` for
   (r2,g2) and (r3,g3), `sw $zero,D_80095588` for (x0,y0), `sw 0x140` for
   (x1,y1), `sw 0x600000` for (x2,y2) and `sw 0x600140` for (x3,y3); the same
   on the sprite template, where `sw 0x64707070,D_800955FC` is r0,g0,b0 and the
   setSprt code byte merged, and `sw 0xDB0080,D_80095608` is w,h.  We emit two
   `sh`/`sb` per pair instead, so the instruction stream is ~68 bytes longer -
   a per-pair difference, not a scheduling one (reordering the source does not
   merge them).  Everything else is byte-exact: the prologue/epilogue, the
   0x24-byte block move of D_80095580[0] into D_80095580[1] (loop of 4 words
   plus one trailing word), the four GetTPage/SetDrawTPage pairs with
   D_800955C8 in $s0, the header stores, the func_800732AC copy of the 0x14-byte
   sprite template into five 0x14-byte copies, the eight copy fixups, the
   0x4000-entry x12 scale loop on g_BattleGridCells and the func_80087830 tail.
   Tried: per-vertex interleaved field writes (70.16%), all four setRGB first
   then setXYWH(0,0,0x140,0x60) (67.89% - setXYWH is what makes retail's
   x3,y3 come out as the single 0x600140 constant, so it is probably the right
   shape and only the store merging is missing).  Next lever: find the source
   form that makes cc1 emit the wider stores, e.g. writing the colour pairs
   through a u16 lvalue and the coordinate pairs through a u32 lvalue.
   NOTE the x3,y3 store must stay absolute (retail uses %lo(D_800955A0)($at),
   ours re-used the copy's base register as `sh a0,32(a2)`).
void func_80081ECC(void) {
    s32 i;

    BattleInitDrawEnvPair();

    setPolyG4(&D_80095580[0]);
    setRGB0(&D_80095580[0], 0x10, 0x60, 0x7F);
    D_80095580[0].x0 = 0;
    D_80095580[0].y0 = 0;
    setRGB1(&D_80095580[0], 0x10, 0x60, 0x7F);
    D_80095580[0].x1 = 0x140;
    D_80095580[0].y1 = 0;
    setRGB2(&D_80095580[0], 0x7F, 0x7F, 0x7F);
    D_80095580[0].x2 = 0;
    D_80095580[0].y2 = 0x60;
    setRGB3(&D_80095580[0], 0x7F, 0x7F, 0x7F);
    D_80095580[0].x3 = 0x140;
    D_80095580[0].y3 = 0x60;
    D_80095580[1] = D_80095580[0];

    SetDrawTPage(&D_800955C8[0], 0, 0, GetTPage(2, 2, 0, 0x100));
    SetDrawTPage(&D_800955C8[1], 0, 0, GetTPage(2, 2, 0, 0));
    SetDrawTPage(&D_800955C8[2], 0, 0, GetTPage(2, 2, 0x100, 0x100));
    SetDrawTPage(&D_800955C8[3], 0, 0, GetTPage(2, 2, 0x100, 0));

    setSprt(&D_800955F8.sprt);
    setRGB0(&D_800955F8.sprt, 0x70, 0x70, 0x70);
    D_800955F8.sprt.x0 = 0;
    D_800955F8.sprt.y0 = 0;
    D_800955F8.sprt.u0 = 0;
    D_800955F8.sprt.v0 = 0;
    D_800955F8.sprt.w = 0x80;
    D_800955F8.sprt.h = 0xDB;
    setSemiTrans(&D_800955F8.sprt, 1);

    func_800732AC(&D_800955F8.copies[0], &D_800955F8.sprt, 0x64);

    D_800955F8.copies[1].x0 = 0x80;
    D_800955F8.copies[1].u0 = 0x80;
    D_800955F8.copies[2].x0 = 0x80;
    D_800955F8.copies[2].u0 = 0x80;
    D_800955F8.copies[3].x0 = 0x100;
    D_800955F8.copies[3].w = 0x40;
    D_800955F8.copies[4].x0 = 0x100;
    D_800955F8.copies[4].w = 0x40;

    for (i = 0; i < 0x4000; i++) {
        g_BattleGridCells[i].unk0 = g_BattleGridCells[i].unk0 * 12;
    }
    func_80087830();
}
*/
