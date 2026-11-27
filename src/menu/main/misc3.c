#include "common.h"

#include "system/menu.h"
#include "psyq/libapi.h"

// The arrays menuUnk2 indexes as card * MENU_CARD_RECORD_COUNT + record hold 16 records
// per card, so a flat record index divides by it to get the card it is on.
// MenuLoadWindow2Text's second text id when there is no second text to load.
// MenuFindFreeChunkIndex's index argument when the caller has none and wants a free one.
#define MENU_CHUNK_INDEX_NONE 0xFF
// The frame budget MenuWaitWindowInputOrCardChange spends waiting for a memory card status byte to change.
#define MENU_CARD_CHANGE_WAIT_FRAMES 180

extern u8 g_MenuCardChunkInUse[][0x10];
extern u8* g_MenuCardChunkData;
extern s32 D_801E9810[];
extern s32 D_801E9818[];
extern s32 D_801E9820[];
extern s32 D_801E9828[];
extern u16 D_8005A3A0[0x10];
extern u16 D_8006F958[0x10];
extern int g_SpuChannelMask;
extern int MenuSaveSlotReadGameState(u32*, void*);
extern int MenuCreateWindow2Strings(u_char);
extern u_char MenuWaitWindowInputOrCardChange(u_char);
extern int MenuFreeWindow2Strings(void);
extern void func_801CB184(void);
extern int MenuEnumerateCardSaveFiles(u_char);
extern void MenuRenderFrame(void);
extern u_char MenuLoadWindow2Text(u_char, u_char, u_char);
extern void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);
extern s32 g_MenuCardSlotTotals[];
extern void func_801C9EF4(s32 a0, s32 a1);
extern void func_801CA1D4(s32 a0, s32 a1);
extern void func_801CA480(s32 a0, s32 a1);
extern void func_801CA5F0(s32 a0, s32 a1);
extern void MenuUpdateCharacterWindow(s32 arg0, u_char arg1);

// The memory card directory prefix a slot's filename is appended to: g_MenuCardPathPrefix0
// reads "bu00:", g_MenuCardPathPrefix1 "bu10:".
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ s16 unk4;
} MenuCardPrefix; // Size: 0x6

extern MenuCardPrefix g_MenuCardPathPrefix0;
extern MenuCardPrefix g_MenuCardPathPrefix1;

/* Enumerate one memory card's save directory into records[]: the per-card index
   range is cleared to MENU_SLOT_NONE, the card's "buXX:" prefix is listed with
   firstfile/nextfile, and the record names are copied in.
   The one-iteration do/while around the scan is load-bearing: without a label
   between the scan and the cardSlotCounts store gcc gives the u_char parameter
   $s1 and a0 * MENU_CARD_RECORD_COUNT $s2, and retail has that pair the other
   way round (99.27% against 99.78%, retail's 368 bytes / 92 insns either way).
   The only remaining diff is a relocation: retail names D_801C50AC for prefix's
   s16, this reaches g_MenuCardPathPrefix0 + 4, the same address. */
int MenuEnumerateCardSaveFiles(u_char a0) {
    struct DIRENTRY dir;
    MenuCardPrefix prefix;
    u_char count;
    s32 retry;
    s32 i;

    g_MenuCardSlotTotals[a0] = 0;
    retry = 5;
    for (i = 0; i < MENU_CARD_RECORD_COUNT; i++) {
        g_Menu->menuUnk2->slotRecordIndexes[a0 * MENU_CARD_RECORD_COUNT + i] = MENU_SLOT_NONE;
        g_MenuCardChunkInUse[a0][i] = 0;
    }
    if (a0 == 0) {
        memcpy(&prefix, &g_MenuCardPathPrefix0, 6);
    } else {
        memcpy(&prefix, &g_MenuCardPathPrefix1, 6);
    }
    if (--retry != 0) {
        count = 0;
        do {
            if (firstfile((char*)&prefix, &dir) == &dir) {
                do {
                    strcpy(g_Menu->menuUnk2->records[a0 * MENU_CARD_RECORD_COUNT + count].ids, dir.name);
                    count++;
                } while (nextfile(&dir) == &dir);
            }
        } while (0);
        g_Menu->menuUnk2->cardSlotCounts[a0] = count;
    }
    return count;
}






void MenuRunCardSlotOperations(void) {
    switch (g_Menu->menuUnk2->cardPollMode) {
    case 1:
        if (g_Menu->menuUnk2->cardEnumerated[0] == 0) {
            if ((u_char)MenuEnumerateCardSaveFiles(0) != 0) {
                g_Menu->unk334 = 1;
            }
            g_Menu->menuUnk2->cardEnumerated[0] = 1;
        }
        if (g_Menu->menuUnk2->cardEnumerated[1] == 0) {
            if ((u_char)MenuEnumerateCardSaveFiles(1) != 0) {
                g_Menu->unk334 = 1;
            }
            g_Menu->menuUnk2->cardEnumerated[1] = 1;
        }
        break;
    case 2:
        if (g_Menu->menuUnk2->cardEnumerated[0] == 0) {
            MenuEnumerateCardSaveFiles(0);
            g_Menu->menuUnk2->cardEnumerated[0] = 1;
        }
        if (g_Menu->menuUnk2->cardEnumerated[1] == 0) {
            MenuEnumerateCardSaveFiles(1);
            g_Menu->menuUnk2->cardEnumerated[1] = 1;
        }
        break;
    }
}

int MenuReadFileChunk(u8* pPath, u8* pBuffer) {
    int fd;

    fd = open(pPath, 3);
    if (fd == -1) {
        return -1;
    }
    if (read(fd, pBuffer, MENU_IMAGE_CHUNK_SIZE) != MENU_IMAGE_CHUNK_SIZE) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

extern void MenuWaitCardOperationFrames(u_char index);
extern u32 D_801C50E0[];
extern u8 g_MenuCardSaveListStale;
extern u8 g_MenuCardChangeDetectionEnabled;
extern u8 g_MenuWindowTextBackPressed;

/* The prefix is a 6-byte block move out of an 8-byte {u32; s16} aggregate (a
   6-byte 4-aligned type cannot be declared in C), so it is a memcpy of 6; a
   field-wise copy cross-jumps the two arms. Both address sums are written
   index-first over a mem_addr base (the MenuMarkSlotsWithGameSave idiom): only
   that spelling emits retail's offset-first `addu a1,s0,a1`, and a typed
   subscript canonicalises back to base-first (measured: `chunks[idx].pad0`
   99.73%, both subscripts 99.11%). A struct assignment gives 95.16% -- it
   reads the 6-byte prefix as two `lw` and stores `sw` where retail has `sh`. */
void MenuLoadCardChunk(u_char a0, u_char a1) {
    MenuCardPrefix prefix;
    char path[0x40];
    s32 retry = 1;
    s32 i;

    if (a0 == 0) {
        memcpy(&prefix, &g_MenuCardPathPrefix0, 6);
    } else {
        memcpy(&prefix, &g_MenuCardPathPrefix1, 6);
    }
    strcpy(path, (char*)&prefix);
    strcat(path, ((MenuUnk2Record*)((a0 * MENU_CARD_RECORD_COUNT + a1) * sizeof(MenuUnk2Record) + (mem_addr)g_Menu->menuUnk2->records))->ids);
    while (1) {
        if (MenuReadFileChunk((u8*)path, ((MenuUnk2ImageChunk*)((a0 * MENU_CARD_RECORD_COUNT + a1) * sizeof(MenuUnk2ImageChunk) + (mem_addr)g_Menu->menuUnk2->chunks))->pad0) != -1) {
            break;
        }
        if (--retry == 0) {
            break;
        }
        MenuWaitCardOperationFrames(a0);
    }
    for (i = 0; i < g_Menu->menuUnk2->chunks[a0 * MENU_CARD_RECORD_COUNT + a1].pad0[3]; i++) {
        g_Menu->menuUnk2->slotRecordIndexes[a0 * MENU_CARD_RECORD_COUNT + g_Menu->menuUnk2->slotWriteCursor] = a0 * MENU_CARD_RECORD_COUNT + a1;
        g_Menu->menuUnk2->slotWriteCursor++;
    }
}
void MenuMarkSlotsWithGameSave(int a0) {
    MenuUnk2ImageChunk* pChunk;
    int i;
    int j;
    int idx;
    int matched;

    for (i = 0; i < MENU_CARD_RECORD_COUNT; i++) {
        g_Menu->menuUnk2->slotHasGameSave[a0 * MENU_CARD_RECORD_COUNT + i] = 0;
    }
    for (j = 0; j < MENU_CARD_SLOT_COUNT; j++) {
        /* matched=1 in the for-init is what puts retail's li t1,1 after the
           hoisted record base; as a separate statement it lands above it. */
        for (i = 0, matched = 1; i < 0xC; i++) {
            if (g_Menu->menuUnk2->records[g_Menu->menuUnk2->slotRecordIndexes[a0 * MENU_CARD_RECORD_COUNT + j]].ids[i] != g_Menu->menuUnk2->gameSerialCode[i]) {
                matched = 0;
                break;
            }
        }
        if (matched) {
            g_Menu->menuUnk2->slotHasGameSave[a0 * MENU_CARD_RECORD_COUNT + j] = 1;
            idx = g_Menu->menuUnk2->slotRecordIndexes[a0 * MENU_CARD_RECORD_COUNT + j];
            /* index-first into a mem_addr base: retail's addu puts the offset in
               the destination and pMenu in the addend, and &chunks[idx] alone
               gives the opposite order (99.55%). */
            pChunk = (MenuUnk2ImageChunk*)(idx * sizeof(MenuUnk2ImageChunk) + (mem_addr)g_Menu->menuUnk2->chunks);
            g_MenuCardChunkData = pChunk->data;
            g_MenuCardChunkInUse[a0][pChunk->index] = 1;
        }
    }
}


/* Round 286, best body: objdiff 94.04% but only SIX differing instructions (the
   permuter's instruction-diff score 155, against 165 for the 97.56% pointer-local
   body below - objdiff's fuzzy percent weights this one lower even though it is
   closer, so judge this body by the diff row count). Inlining the record
   expression into the `if` (no `pRecord` local) is what removed the a2/a3 register
   swap of the loop body: gcc then re-forms `pRecord + i` in the inner loop's delay
   slot as retail does. The two remaining rows are `li t1,1` (matched) scheduled
   two instructions above retail's (mine before the slotRecordIndexes lbu, retail
   after it) and the `chunks[k]` address built base-first (`addu v1,v1,v0`) where
   retail has offset-first (`addu v0,v0,v1`). The index-first spellings canonicalise
   back.
   The pointer-local variant (a `pRecord = &records[slotRecordIndexes[...]]` line
   before the inner loop) reads objdiff 97.56% but differs in ~20 rows: it keeps the
   record base out of the loop, swaps the record base and pMenu between a2/a3 and
   also flips the `chunks[k]` order. Also tried: matched before/after the pRecord
   assignment, `u_char matched` (96.15%), `u_short` shape, and hoisting the
   slotRecordIndexes byte into a `u8 idx` local (86.54% - retail really does reload
   slotRecordIndexes inside the `if`, so the index must stay inline). */
/*
void MenuMarkSlotsWithGameSave(int a0) {
    MenuUnk2ImageChunk* pChunk;
    int i;
    int j;
    int idx;
    int matched;

    for (i = 0; i < 0x10; i++) {
        g_Menu->menuUnk2->slotHasGameSave[a0 * 0x10 + i] = 0;
    }
    for (j = 0; j < 0xF; j++) {
        matched = 1;
        for (i = 0; i < 0xC; i++) {
            if (g_Menu->menuUnk2->records[g_Menu->menuUnk2->slotRecordIndexes[a0 * 0x10 + j]].ids[i] != g_Menu->menuUnk2->gameSerialCode[i]) {
                matched = 0;
                break;
            }
        }
        if (matched) {
            g_Menu->menuUnk2->slotHasGameSave[a0 * 0x10 + j] = 1;
            idx = g_Menu->menuUnk2->slotRecordIndexes[a0 * 0x10 + j];
            pChunk = (MenuUnk2ImageChunk*)(idx * sizeof(MenuUnk2ImageChunk) + (mem_addr)g_Menu->menuUnk2->chunks);
            g_MenuCardChunkData = pChunk->data;
            g_MenuCardChunkInUse[a0][pChunk->index] = 1;
        }
    }
}
*/

extern void MenuOpenCardEvents(void);
extern void func_801E78C8(s32 index);
extern void Vsync(s32 count);
extern u8 D_801C50B8[];
extern u8 D_801EA6F8;
extern u8 g_MenuCardCheckInterval;
extern u16 g_MenuCellPrimXs[];
extern u16 g_MenuCellPrimYs[];
extern s32 g_MenuCardSlotOrder[];

// Best attempt: 95.78% (2092 bytes / 523 insns against retail's 2084 / 521). This note
// supersedes the 95.74 one: its "one j and its delay slot" is wrong, see THE +8 below.
// REACHED IN ROUND 467 (95.74 -> 95.78, 118 -> 112 differing lines, same 2092 bytes): the
// Vsync countdown is its OWN local (`frames`), not `i`. Retail's `li s0,0x3B` /
// `addiu s0,s0,-1` / `bnez s0` runs in $s0 while `i` (the record index) is $s1; reusing `i`
// put the countdown in $s1 and made three rows differ.
// THE +8 BYTES: the instruction COUNT is level - retail's six-instruction
// break2/break1/shared-tail block exists on both sides (this body splits it into the break2
// pair and the break1+tail quad, a block-order difference that only relabels branch
// targets). The +2 instructions are two `nop`s in the FIRST of the eight cursor-prim store
// groups: retail's group is lw a1 / lw v0 / lbu a0 / lw v0 / sll / addu (6), this body's is
// lw a1 / nop / lbu v0 / lw a0 / sll / addu / lw v0 / nop (8). That group is the one the
// shared HImode -6 constant disturbs: the body materialises `ori a2,zero,0xfffa` in the
// `beqz` delay slot that retail fills with the return copy `addu v0,s4,zero`, then loads
// menuUnk2 late and needs both nops. So residual (b) and the size are ONE cause.
// REMAINING: (a) g_Menu lands in $a3 for the eight store groups where retail has $a2 (which
// is why the -6 constant gets $a2 at all); (b) that -6 CSE; (c) the entry
// `move s4,zero` / `move s6,s4` swap, plus `move s6,s4` in the joined block's delay slot
// where retail has `andi v0,s3,0xff` (round-454 lever: the join copy in the entry slot is
// not source-reachable - the entry spelling folds to 0 and deletes the store block, 94.27%;
// reading `waited` directly there is 95.40% at retail's exact 2084 bytes); (d) the two
// path-prefix rows are RELOCATION-ONLY: retail names D_801C50AC / D_801C50B4, we reach
// g_MenuCardPathPrefix0/1 + 4, same linked bytes.
// Preset screens on the old body: gcc-2.7.2 96.70, 2.7.2-cdk 89.02, unit preset 2.6.0
// 95.74 (no 100 anywhere, so no switch).
// Learned earlier, still true: the two 0xFF byte pairs (previousCardSlotCounts[1]/[0]
// and previousCardStatus[1]/[0]) each come from ONE `MenuUnk2* pMenu = g_Menu->menuUnk2;`
// load - two `g_Menu->menuUnk2->...` assignments cost 9 instructions because a `sb`
// kills the reload of the g_Menu pointer; the saved card status must be a local ARRAY
// (u8 savedCardStatus[2]) or gcc keeps it in registers instead of retail's sp+0x50/0x51
// bytes; the 8 cursor prim stores need 8 full expressions with the index-first
// `(ctx * sizeof(POLY_FT4) + (mem_addr)pCursors)` address (a pPrim local or a
// base-first subscript loses 1.8%). Tried and rejected: `continue` vs `if (enumerated
// == 0) break` (same beqz), `status = 0` initialised (costs retail's missing `move
// s3,zero`; retail reads `status` uninitialised on the card-wait path, a latent bug in
// the original), and no permuter run - the residual is a register hand-out plus one CSE
// constant, which it cannot move.
// best 99.70% at retail's EXACT 2084 bytes / 521 insns (up from 95.78). Residual is TWO things only: (1) retail's `addu s6,s4,zero` copy sits in the ENTRY block (right after move s4,zero, before the first bnez) and ours lands in the 0x45A8 bnez delay slot, which pushes retail's `andi v0,s3,0xff` out of that slot (offsets ripple ~4 bytes); (2) the typed-reloc spellings retail names D_801C50AC / D_801C50B4 and we reach g_MenuCardPathPrefix0/1 + 4 (identical linked bytes, SECOND EXCEPTION class).
//    MEASURED DEAD on (1): `waited = 0; rebuildWindow = waited;` or `rebuildWindow = 0;` at the top DOES move the copy into the entry block but lets cprop prove rebuildWindow==0, so the whole `if (rebuildWindow != 0) pManager->characterWindowBuilt = 1` block is folded away: 514 insns / 2056 bytes, 94.31%. Retail keeps that block ALIVE with s6 set once from the (zero) s4, so its source related the two locals in a way gcc 2.6.0 cannot constant-fold; not found. The late placement is the closest reachable body (it also differs semantically: ours can pass 1).
//    TWO FIXES THAT WERE WORTH 2 POINTS EACH, keep them: (a) `(s16)g_MenuCellPrimYs[...] - 6` - WITHOUT the cast gcc hoists -6 into a register (li a2,0xfffa) in the shouldUpdatePosition branch's delay slot, which steals a2 so g_Menu moves to a3 and every one of the eight cursor stores reloads through the wrong base (112 -> 50 differing lines at identical size); with it retail's per-site addiu v0,v0,-6 and a2 as the g_Menu base come back. (b) the card-sample loop must be the POSITIVE conjunction `if (cardStatus[card] != 0 && saved0 == cardStatus[0] && saved1 == cardStatus[1]) { load...; if (cardEnumerated[card] != 0) continue; status = card + 1; } else { status = 2; } card = MENU_CARD_COUNT; waited = 0; break;` - the negative form and the two-separate-breaks form both make gcc invert the third test and add a `j` (98.80 and 97.15).
// Re-measured round 493 (XenoMenu260): best 99.70% / 2084 bytes / 521 insns = retail's EXACT size.
// Residual is still exactly two things. (1) Retail's `addu s4,zero,zero` sits in the 0x43F8
// `lw v0,0x32C(v0)` delay slot (before the lhu) with `addu s6,s4,zero` in the 0x4408 bnez slot; this
// body has `nop` there, `addu s4,zero,zero` in that bnez slot, and `addu s6,s4,zero` in the 0x45A8
// slot where retail has `andi v0,s3,0xff` - the same five instructions in another order, sched2/reorg's
// pick, not the C: both bodies' join copy is in the same basic block (lever
// a-copy-of-a-local-in-an-entry-delay-slot-is-a-join-copy already established that the entry-position
// copy IS the join copy, so the source placement of `rebuildWindow = waited;` is right).
// (2) relocation only: D_801C50AC / D_801C50B4 vs g_MenuCardPathPrefix0/1 + 4 (SECOND EXCEPTION).
// Measured dead this round: `waited = 0; rebuildWindow = waited;` as the first two statements, and the
// chained `rebuildWindow = waited = 0;` - both 98.22% / 2048 bytes / 512 insns, retail's entry slot
// reproduced but as `move s6,zero`, so cprop folds the `if (rebuildWindow != 0)` block away (7 insns).
// `waited = 0` written as a statement immediately before the first `if` (retail's position) instead of
// as the declaration initialiser is BYTE-IDENTICAL to the body below (the sink pass moves it anyway).
// permute.sh cannot score this body: the base fails to compile in the permuter (8x 'conversion to
// non-scalar type requested', 'Unable to compile base.c'), so it has no permuter number.
/*
u_char func_801C93A8(void) {
    char path[0x40];
    u8 savedCardStatus[MENU_CARD_COUNT];
    u_char waited = 0;
    u_char rebuildWindow;
    u_char status;
    MenuUnk2* pMenu;
    s32 card;
    s32 i;
    s32 frames;
    s32 recordIndex;

    D_801EA6F8 = 0;
    g_Menu->menuUnk2->cardPollMode = 2;
    MenuRenderFrame();
    if (*(u16*)g_Menu->menuUnk2->cardStatus == 0) {
        MenuFreeWindow2Strings();
        g_Menu->pManager->window2StringsActive = 0;
        g_Menu->pManager->characterWindowBuilt = 0;
        MenuRenderFrame();
        MenuCreateWindow2Strings(0x23);
        g_Menu->cardSlotState = 1;
        g_Menu->pManager->shouldRenderCursors = 0;
        g_Menu->menuUnk2->previousSelectedSlot = MENU_SLOT_NONE;
        g_Menu->menuUnk2->selectedSlot = 0;
        pMenu = g_Menu->menuUnk2;
        if (*(u16*)pMenu->cardStatus == 0) {
            pMenu->previousCardSlotCounts[1] = MENU_SLOT_NONE;
            pMenu->previousCardSlotCounts[0] = MENU_SLOT_NONE;
            MenuRenderFrame();
            for (frames = 0x3B; frames != 0; frames--) {
                Vsync(0);
            }
            waited = 1;
        }
        if (waited == 0) {
            MenuOpenCardEvents();
            pMenu = g_Menu->menuUnk2;
            pMenu->previousCardStatus[1] = MENU_SLOT_NONE;
            pMenu->previousCardStatus[0] = MENU_SLOT_NONE;
            g_Menu->menuUnk2->cardEnumerated[0] = 0;
            g_Menu->menuUnk2->cardEnumerated[1] = 0;
            g_Menu->cardCheckTimer = 0x3C;
            MenuRenderFrame();
            D_801EA6F8 = 1;
        }
        MenuFreeWindow2Strings();
    }
    rebuildWindow = waited;
    g_MenuCardCheckInterval = 1;
    savedCardStatus[0] = g_Menu->menuUnk2->cardStatus[0];
    savedCardStatus[1] = g_Menu->menuUnk2->cardStatus[1];
    if (waited == 0) {
        g_Menu->soundEffectsEnabled = 0;
        for (card = 0; card < MENU_CARD_COUNT; card++) {
            status = 0;
            if (g_Menu->menuUnk2->cardSlotCounts[card] != g_Menu->menuUnk2->previousCardSlotCounts[card] &&
                g_Menu->menuUnk2->cardSlotCounts[card] != 0) {
                MenuOpenCardEvents();
                for (i = 0; i < MENU_CARD_RECORD_COUNT; i++) {
                    g_Menu->menuUnk2->slotRecordIndexes[card * MENU_CARD_RECORD_COUNT + i] = MENU_SLOT_NONE;
                }
                if (card == 0) {
                    memcpy(path, &g_MenuCardPathPrefix0, 6);
                } else {
                    memcpy(path, &g_MenuCardPathPrefix1, 6);
                }
                strcat(path, (char*)D_801C50B8);
                erase(path);
                g_Menu->menuUnk2->slotWriteCursor = 0;
                g_MenuCardSlotTotals[card] = 0;
                for (i = 0; i < g_Menu->menuUnk2->cardSlotCounts[card]; i++) {
                    if (g_Menu->menuUnk2->cardStatus[card] != 0 &&
                        savedCardStatus[0] == g_Menu->menuUnk2->cardStatus[0] &&
                        savedCardStatus[1] == g_Menu->menuUnk2->cardStatus[1]) {
                        MenuLoadCardChunk(card, i);
                        recordIndex = card * MENU_CARD_RECORD_COUNT + i;
                        func_801E78C8(recordIndex);
                        g_Menu->menuUnk2->records[recordIndex].unk58 = 1;
                        MenuRenderFrame();
                        if (g_Menu->menuUnk2->cardEnumerated[card] != 0) {
                            continue;
                        }
                        status = card + 1;
                    } else {
                        status = 2;
                    }
                    card = MENU_CARD_COUNT;
                    waited = 0;
                    break;
                }
                if (status == 0) {
                    for (; i < MENU_CARD_RECORD_COUNT; i++) {
                        g_Menu->menuUnk2->records[card * MENU_CARD_RECORD_COUNT + i].unk58 = 0;
                    }
                    g_Menu->menuUnk2->previousCardSlotCounts[card] = g_Menu->menuUnk2->cardSlotCounts[card];
                }
            }
            if (status == 0 && g_Menu->menuUnk2->cardSlotCounts[card] == 0) {
                for (i = 0; i < MENU_CARD_RECORD_COUNT; i++) {
                    g_Menu->menuUnk2->records[card * MENU_CARD_RECORD_COUNT + i].unk58 = 0;
                }
                g_Menu->menuUnk2->previousCardSlotCounts[card] = MENU_SLOT_NONE;
            }
        }
        if (status == 0) {
            MenuMarkSlotsWithGameSave(0);
            MenuMarkSlotsWithGameSave(1);
        }
    }
    if (status == 0) {
        if (rebuildWindow != 0) {
            g_Menu->pManager->characterWindowBuilt = 1;
        }
        if (g_Menu->pManager->shouldRenderCursors != 0 && g_Menu->pCursors->shouldUpdatePosition[0] != 0) {
            ((POLY_FT4*)(g_Menu->pCursors->renderContexts[0] * sizeof(POLY_FT4) + (mem_addr)g_Menu->pCursors))->x0 =
                g_MenuCellPrimXs[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot] * 2] + 8;
            ((POLY_FT4*)(g_Menu->pCursors->renderContexts[0] * sizeof(POLY_FT4) + (mem_addr)g_Menu->pCursors))->y0 =
                (s16)g_MenuCellPrimYs[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot] * 2] - 6;
            ((POLY_FT4*)(g_Menu->pCursors->renderContexts[0] * sizeof(POLY_FT4) + (mem_addr)g_Menu->pCursors))->x1 =
                g_MenuCellPrimXs[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot] * 2] + 0x18;
            ((POLY_FT4*)(g_Menu->pCursors->renderContexts[0] * sizeof(POLY_FT4) + (mem_addr)g_Menu->pCursors))->y1 =
                (s16)g_MenuCellPrimYs[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot] * 2] - 6;
            ((POLY_FT4*)(g_Menu->pCursors->renderContexts[0] * sizeof(POLY_FT4) + (mem_addr)g_Menu->pCursors))->x2 =
                g_MenuCellPrimXs[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot] * 2] + 8;
            ((POLY_FT4*)(g_Menu->pCursors->renderContexts[0] * sizeof(POLY_FT4) + (mem_addr)g_Menu->pCursors))->y2 =
                g_MenuCellPrimYs[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot] * 2] + 0xA;
            ((POLY_FT4*)(g_Menu->pCursors->renderContexts[0] * sizeof(POLY_FT4) + (mem_addr)g_Menu->pCursors))->x3 =
                g_MenuCellPrimXs[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot] * 2] + 0x18;
            ((POLY_FT4*)(g_Menu->pCursors->renderContexts[0] * sizeof(POLY_FT4) + (mem_addr)g_Menu->pCursors))->y3 =
                g_MenuCellPrimYs[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot] * 2] + 0xA;
        }
    }
    g_Menu->soundEffectsEnabled = 1;
    g_MenuCardCheckInterval = 0x1E;
    return waited;
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C93A8);




s32 MenuIsSlotUsable(s32 a0) {
    MenuUnk2* pMenu = g_Menu->menuUnk2;
    s32 result = 1;
    s32 j;
    s32 idx = pMenu->selectedSlot;

    switch (a0) {
    case 0:
        if (pMenu->cardSlotCounts[0] != 0 || pMenu->cardSlotCounts[1] != 0) {
            j = g_MenuCardSlotOrder[idx];
            if (pMenu->cardStatus[j / MENU_CARD_RECORD_COUNT] != 0) {
                if (pMenu->slotRecordIndexes[j] != MENU_SLOT_NONE) {
                    break;
                }
            }
        }
        result = 0;
        break;
    case 1:
        if (pMenu->cardSlotCounts[0] != 0 || pMenu->cardSlotCounts[1] != 0) {
            j = g_MenuCardSlotOrder[idx];
            if (pMenu->cardStatus[j / MENU_CARD_RECORD_COUNT] != 0) {
                if (pMenu->slotRecordIndexes[j] != MENU_SLOT_NONE) {
                    if (pMenu->slotHasGameSave[j] != 0) {
                        break;
                    }
                }
            }
        }
        result = 0;
        break;
    case 2:
        j = g_MenuCardSlotOrder[idx];
        if (pMenu->cardStatus[j / MENU_CARD_RECORD_COUNT] == 0) {
            result = 0;
        }
        break;
    }
    if (result != 0) {
        g_Menu->cardSlotState = MENU_CARD_SLOT_STATE_READY;
    }
    /* keeping `idx` live here is what makes the allocator put the result flag in a2 */
    if (idx) {
        return result;
    }
    return result;
}

s32 MenuFindFirstValidSlot(s32 a0) {
    s32 result = MENU_SLOT_NONE;
    s32 searching = 1;
    s32 start;
    s32 limit;
    s32 i;
    s32 j;

    start = (g_Menu->menuUnk2->cardStatus[0] == 0) ? MENU_CARD_SLOT_COUNT : 0;
    limit = MENU_CARD_COUNT * MENU_CARD_SLOT_COUNT;
    if (g_Menu->menuUnk2->cardStatus[1] == 0) {
        limit = MENU_CARD_SLOT_COUNT;
    }
    for (i = start; i < limit; i++) {
        if (!searching) {
            break;
        }
        switch (a0) {
        case 0:
            if (g_Menu->menuUnk2->cardSlotCounts[0] != 0 || g_Menu->menuUnk2->cardSlotCounts[1] != 0) {
                j = g_MenuCardSlotOrder[i];
                if (g_Menu->menuUnk2->cardStatus[j / MENU_CARD_RECORD_COUNT] != 0 && g_Menu->menuUnk2->slotRecordIndexes[j] != MENU_SLOT_NONE) {
                    result = i;
                    searching = 0;
                }
            } else {
                searching = 0;
            }
            break;
        case 1:
            if (g_Menu->menuUnk2->cardSlotCounts[0] != 0 || g_Menu->menuUnk2->cardSlotCounts[1] != 0) {
                j = g_MenuCardSlotOrder[i];
                if (g_Menu->menuUnk2->cardStatus[j / MENU_CARD_RECORD_COUNT] != 0 && g_Menu->menuUnk2->slotRecordIndexes[j] != MENU_SLOT_NONE &&
                    g_Menu->menuUnk2->slotHasGameSave[j] != 0) {
                    result = i;
                    searching = 0;
                }
            } else {
                searching = 0;
            }
            break;
        case 2:
            j = g_MenuCardSlotOrder[i];
            if (g_Menu->menuUnk2->cardStatus[j / MENU_CARD_RECORD_COUNT] != 0) {
                result = i;
                searching = 0;
            }
            break;
        }
    }
    g_Menu->cardSlotState = MENU_CARD_SLOT_STATE_READY;
    return result;
}
// Best attempt: 96.74% (736 bytes = retail's EXACT size, 184 = 184 insns; 34 differing
// lines). 86.49 -> 96.74 this round with ONE change of shape, the same lever that took the
// sibling func_801CA480 from 86.47 to 95.71.
//
// THE LEVER: the two-test loops must be written as an INVERTED fail path with an explicit
// increment-and-continue, so that the store block lands physically INSIDE the loop's
// instruction range. gcc 2.6 has no alias analysis, so a store between a loop's top and
// its back edge makes the loop-invariant "lw 0x32C(g_Menu)" non-hoistable; that is exactly
// what retail shows in every two-test loop here (case 1 phase 1: hit block 0x5084 between
// the continue block 0x5078 and the test 0x508C). Case 1's phase 1 and phase 2 both became
// "while (cond) { pMenu = g_Menu->menuUnk2; j = D_801E98xx[i]; if (== 0xFF || hasGameSave
// == 0) { i += n; continue; } pMenu->selectedSlot = i + n; break; }" with the index the RAW
// parameter and "+n" the tested/stored value; both loops came out instruction-identical
// (case 1 phase 1 whole, including retail's loop-inversion copy "move a2,v1" and its
// separate a2 induction). Case 0 and case 2 keep the non-inverted forms and were already
// exact - do not touch them: their loops have a single test, retail hoists pMenu there too,
// and their hit blocks sit outside the loop range in both compiles.
// The earlier note on this function blamed "gcc 2.6 loop.c move_movables declining the hoist
// on its savings/register-pressure threshold" and the four siblings were parked on it. That
// reading was wrong: the hoist is alias-driven and the source shape controls it.
//
// NAMED RESIDUAL, 34 lines, no missing instruction: one register triple rotated in the LAST
// loop (case 1 phase 2). Retail keeps {value a0, pMenu a1, table biv a2}; this build emits
// {value a2, pMenu a0, table biv a1}, so the loop body is retail's line for line with a1/a2
// swapped and the induction +1 going to a2 instead of a0. Same rotation as func_801CA480
// (there: pMenu a1 vs a2), i.e. this build ranks the pointer biv above pMenu in local-alloc
// where retail ranks it below. Second difference: case 0's phase-2 hit block is emitted at
// +0x2a8 where retail has it at +0x258 (placement only - the shared store block; the branch
// targets are the only lines that differ in that hunk).
/*
void func_801C9EF4(s32 a0, s32 a1) {
    MenuUnk2* pMenu;
    s32 i;
    s32 j;

    switch (a0) {
    case 0:
        for (i = a1; i + 3 < 0x1E; i += 3) {
            j = D_801E9828[i];
            if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF) {
                g_Menu->menuUnk2->selectedSlot = i + 3;
                break;
            }
        }
        if (i + 3 < 0x1E) {
            break;
        }
        i = g_Menu->menuUnk2->selectedSlot + 3;
        if (i < 0x1E) {
            if (i + 1 < 0x1E) {
                for (; i + 1 < 0x1E; i++) {
                    j = D_801E9820[i];
                    if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF) {
                        g_Menu->menuUnk2->selectedSlot = i + 1;
                        break;
                    }
                }
            }
        }
        break;
    case 1:
        i = a1;
        while (i + 3 < 0x1E) {
            pMenu = g_Menu->menuUnk2;
            j = D_801E9828[i];
            if (pMenu->slotRecordIndexes[j] == 0xFF || pMenu->slotHasGameSave[j] == 0) {
                i += 3;
                continue;
            }
            pMenu->selectedSlot = i + 3;
            break;
        }
        if (i + 3 < 0x1E) {
            break;
        }
        i = g_Menu->menuUnk2->selectedSlot + 3;
        if (i < 0x1E) {
            if (i + 1 < 0x1E) {
                while (i + 1 < 0x1E) {
                    pMenu = g_Menu->menuUnk2;
                    j = D_801E9820[i];
                    if (pMenu->slotRecordIndexes[j] == 0xFF || pMenu->slotHasGameSave[j] == 0) {
                        i++;
                        continue;
                    }
                    pMenu->selectedSlot = i + 1;
                    break;
                }
            }
        }
        break;
    case 2:
        i = a1 + 3;
        if (((u8*)&g_Menu->menuUnk2->cardStatus[0])[i / 15] != 0) {
            if (i < 0x1E) {
                g_Menu->menuUnk2->selectedSlot = i;
            } else {
                i = g_Menu->menuUnk2->selectedSlot + 3;
                if (i < 0x1E) {
                    if (i + 1 < 0x1E) {
                        g_Menu->menuUnk2->selectedSlot = i + 1;
                    }
                }
            }
        }
        break;
    }
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9EF4);


/* Best attempt: 83.27% (668 bytes vs retail's 684). Structure is settled: each case
   is a step-3 search over D_801E9810 whose hit stores i - 3 and returns, followed --
   only when that search found nothing -- by a step-1 search over D_801E9818, each
   phase guarded by BOTH `i >= 0` and `i - 1 >= 0` (the first is redundant in C but
   retail emits both compares at all three sites, so keep it). Case 2 is a flag test
   on ((u8*)menuUnk2)[0x4FE4 + i / 15], divisor FIFTEEN, plus the same clamp. Retail's
   D_801E9810/9818/9820/9828 are one s32 table at index +0/+2/+4/+6.
   Remaining diff is allocator-only: case 1 hoists `lw 0x32C(g_Menu)` out of both of
   its loops where retail reloads it every iteration, which also gives retail a
   separate a0 = i - 3 induction variable that mine recomputes; and case 2's
   `lui 0x88888889` sits in the dispatch delay slot in retail.
   Tried: pMenu local vs inline g_Menu->menuUnk2 (inline 83.27 beats local 77.27),
   gcc-2.7.2-cdk 74.56. func_801C9EF4 is the ascending mirror (D_801E9828 / D_801E9820,
   +3 then +1, `slti 0x1E`), so any fix here should transfer. */
// Best attempt: 85.81% (668 bytes, 167 insns vs retail's 684 / 171; the old note's 83.27 was stale). Structure is settled: each case is a step-3 search over D_801E9810 whose hit stores i - 3 and returns, followed - only when that search found nothing - by a step-1 search over D_801E9818; case 2 flag test is ((u8*)menuUnk2)[0x4FE4 + i / 15], divisor FIFTEEN.
// 
// Residual: the 4 missing instructions are, in the main, the two in-loop g_Menu->menuUnk2 reloads - case 1 is the only case with a two-byte && test and retail reloads lw 0x32C(g_Menu) at the top of BOTH of its loops (with g_Menu itself hoisted into t0/t1), while this compile hoists menuUnk2 out of the loop in every case. Cases 0 and 2, which have a single test, hoist in retail too, so the difference is the two-load body, i.e. gcc 2.6 loop.c move_movables declining the hoist on its savings/register-pressure threshold - the same residual as its three siblings (func_801CA480 85.27, func_801CA5F0 84.72, func_801C9EF4 86.49), so one spelling that flips it fixes four functions.
// 
// Tried this round: case 1 as for (i = a1 - 3; i >= 0; i -= 3) with j = D_801E9810[i + 3] and selectedSlot = i, phase 1 and phase 2 both (retail's own a0 = i - 3 parameterization) = 84.65% / 656 bytes / 164 insns, worse and still hoists. Previously measured: pMenu local 77.27 vs inline 83.27, gcc-2.7.2-cdk 74.56.
// Best attempt: 95.61% (676 bytes vs retail's 684, 169 vs 171 insns; 109 differing lines).
// 85.81 -> 95.61 this round by porting the func_801CA480 park's lever (see func_801CA5F0 above):
//   * a shared 'MenuUnk2* pMenu' local with 'pMenu = g_Menu->menuUnk2;' read INSIDE every loop, so the
//     menuUnk2 load stays non-hoistable where retail reloads 'lw 0x32C(g_Menu)' at the top of a loop;
//   * case 1 spelled as a non-inverted while whose fail arm is 'i -= 3; continue;' and 'i -= 3;' as the
//     loop step, with the store/break AFTER the continue block, which puts the store block inside the
//     loop's linear instruction range. Case 1's phase 1, case 1's phase 2 (both two-condition sites) and
//     case 0/1 phase 1 came out with retail's exact instruction count and order.
//   * case 0's phase 2 reads the slot inline ('i = g_Menu->menuUnk2->selectedSlot - 3;') and only then
//     names pMenu inside the 'if', which reproduces retail's 'lw v1,812(v0)' + 'move a0,v1' pair.
// Residual, all of it one class: retail has the store-value induction in a0 and pMenu in a2 in case 1's
// two-condition loops; this build mirrors them (a0 = pMenu, a2 = store value). Case 0, which has a
// one-condition test, gets retail's assignment exactly, so it is the extra branch that decides the pair.
// The same mirror is func_801CA5F0's whole residual (95.51) and func_801CA480's (95.71). Two smaller
// residuals ride along: retail has 'sll v1,a1,2' in the 'bltz a2' delay slot of case 0 phase 1 where this
// build leaves a nop and schedules the sll after the la, and case 2's dispatch delay slot holds
// 'lui v0,0x8888' in retail against 'addiu a1,a1,-3' here.
// Tried this round (all under 2.6.0 -O2, retail's own parameterization read off the .s):
//   * case 1 phase 1 as 'for (i = a1 - 3; i >= 0; ...)' with 'D_801E9810[i + 3]' and store 'i' =
//     92.84% / 664 bytes / 166 insns (worse; gcc drops the separate induction altogether).
//   * case 1's pMenu load moved after the D_801E9810 read = 95.51 / 676 / 169, 109 lines (the fuzzy
//     number prefers the current 95.61).
//   * previous notes: pMenu-before-inline 77.27, gcc-2.7.2-cdk 74.56, preset screened (2.6.0 -O2 right).
/*
void func_801CA1D4(s32 a0, s32 a1) {
    MenuUnk2* pMenu;
    s32 i;
    s32 j;

    switch (a0) {
    case 0:
        for (i = a1; i - 3 >= 0; i -= 3) {
            pMenu = g_Menu->menuUnk2;
            j = D_801E9810[i];
            if (pMenu->slotRecordIndexes[j] != MENU_SLOT_NONE) {
                pMenu->selectedSlot = i - 3;
                break;
            }
        }
        if (i - 3 >= 0) {
            break;
        }
        i = g_Menu->menuUnk2->selectedSlot - 3;
        if (i >= 0) {
            pMenu = g_Menu->menuUnk2;
            for (; i - 1 >= 0; i--) {
                j = D_801E9818[i];
                if (pMenu->slotRecordIndexes[j] != MENU_SLOT_NONE) {
                    pMenu->selectedSlot = i - 1;
                    break;
                }
            }
        }
        break;
    case 1:
        i = a1;
        while (i - 3 >= 0) {
            pMenu = g_Menu->menuUnk2;
            j = D_801E9810[i];
            if (pMenu->slotRecordIndexes[j] == MENU_SLOT_NONE || pMenu->slotHasGameSave[j] == 0) {
                i -= 3;
                continue;
            }
            pMenu->selectedSlot = i - 3;
            break;
        }
        if (i - 3 >= 0) {
            break;
        }
        pMenu = g_Menu->menuUnk2;
        i = pMenu->selectedSlot - 3;
        if (i >= 0) {
            while (i - 1 >= 0) {
                pMenu = g_Menu->menuUnk2;
                j = D_801E9818[i];
                if (pMenu->slotRecordIndexes[j] == MENU_SLOT_NONE || pMenu->slotHasGameSave[j] == 0) {
                    i--;
                    continue;
                }
                pMenu->selectedSlot = i - 1;
                break;
            }
        }
        break;
    case 2:
        pMenu = g_Menu->menuUnk2;
        i = a1 - 3;
        if (((u8*)&pMenu->cardStatus[0])[i / MENU_CARD_SLOT_COUNT] != 0) {
            if (i >= 0) {
                pMenu->selectedSlot = i;
            } else {
                i = pMenu->selectedSlot - 3;
                if (i >= 0) {
                    if (i - 1 >= 0) {
                        pMenu->selectedSlot = i - 1;
                    }
                }
            }
        }
        break;
    }
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA1D4);


// Best attempt: 95.71% (368 bytes = retail's exact size, 92 = 92 insns; 48 differing
// lines). 86.47 -> 95.71 this round.
//
// THE LEVER THAT CLOSED 5.2%: THE STORE BLOCK MUST SIT INSIDE THE LOOP'S LINEAR
// INSTRUCTION RANGE. gcc 2.6 has no alias analysis, so a store *physically* between
// the loop top and its back edge makes the loop-invariant "lw 0x32C(g_Menu)"
// non-hoistable (loop.c move_movables), which is what retail's case 1 shows: its hit
// block at 0x5578 sits between the continue block 0x5570 and the test 0x5580. Writing
// case 1 as an inverted fail path whose arm is "i++; continue;" and keeping the
// store/break AFTER it puts the store block in range: 86.47 -> 91.68 at 88 insns (the
// store still cross-jumped to case 2's). Spelling the index as retail's own raw
// "i = a1" with "i + 1" as the tested and stored value (instead of "i = a1 + 1" with
// "i < 0x1E") then reproduces retail's loop-inversion copy register a0 = v1, the
// separate a0 induction and retail's exact instruction count: 91.68 -> 95.71.
//
// Measured dead this round, all under 2.6.0 -O2: "i = a1 + 1" with the non-inverted
// && (85.82), the same with the inverted || (91.68), for (i = a1 + 1; i < 0x1E; i++)
// with a bare continue in the || arm (85.82), an if (i < 0x1E) do {} while (i < 0x1E)
// guard (identical to its while form), declaring pMenu after i/j (byte-identical), a
// case-1-only second pointer pSlotMenu (92.59 / 356 / 89), reading g_Menu->menuUnk2
// inline at all three sites (92.59 / 356 / 89), and j-before-pMenu in the loop body
// (95.58). Keep the shared pMenu local: it is the whole difference at case 1.
//
// NAMED RESIDUAL: one register pair swapped. Retail keeps pMenu in a1 and the table
// biv in a2; this build mirrors them, and since the same pseudo is shared by cases 0
// and 2 their rotation follows (case 0 retail: value a2 / const a3 / pMenu a0; here
// value a3 / const t0 / pMenu a2). pMenu's local-alloc priority is what disagrees;
// neither the declaration order nor a second variable moves it. The parked note that
// used to sit here blamed a loop.c savings threshold - that reading was wrong, the
// hoist is alias-driven and the source fixes it.

/*
void func_801CA480(s32 a0, s32 a1) {
    MenuUnk2* pMenu;
    s32 i;
    s32 j;

    switch (a0) {
    case 0:
        for (i = a1; i + 1 < 0x1E; i++) {
            pMenu = g_Menu->menuUnk2;
            j = D_801E9820[i];
            if (pMenu->slotRecordIndexes[j] != 0xFF) {
                pMenu->selectedSlot = i + 1;
                break;
            }
        }
        break;
    case 1:
        i = a1;
        while (i + 1 < 0x1E) {
            pMenu = g_Menu->menuUnk2;
            j = D_801E9820[i];
            if (pMenu->slotRecordIndexes[j] == 0xFF || pMenu->slotHasGameSave[j] == 0) {
                i++;
                continue;
            }
            pMenu->selectedSlot = i + 1;
            break;
        }
        break;
    case 2:
        pMenu = g_Menu->menuUnk2;
        i = a1 + 1;
        if (((u8*)&pMenu->cardStatus[0])[i / 15] != 0 && i < 0x1E) {
            pMenu->selectedSlot = i;
        }
        break;
    }
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA480);






// Best attempt: 84.72% (348 bytes vs retail's 352 = 88 insns). Case 0 is EXACT.
// Residual, measured this round: (1) case 1 - retail keeps a SEPARATE induction
// register for the store value (a0 = i, index = i + 1) with a non-inverted loop
// ending "j test / addiu a0,a0,-1 / bltz / j loop", and RELOADS g_Menu->menuUnk2
// (lw ...,812(a3)) at the top of every iteration while g_Menu itself stays in a3;
// this compile hoists menuUnk2 out of the loop and recomputes the store value
// (addiu a1,a1,-1 / addiu a2,a1,-1) the way its own case 0 does. All four siblings
// (func_801CA480, func_801CA1D4, func_801C9EF4) have the same case-1 shape, so one
// fix should move all four. (2) case 2 - retail puts lui v0,0x88888889 in the
// dispatch delay slot and addiu a1,a1,-1 inside the body; this compile does the
// reverse, which is the one missing instruction.
// Tried this round: case 1 as "for (i = a1-1; i >= 0; i--) { j = D_801E9818[i+1]; ... selectedSlot = i; }"
// (retail's own parameterization: 86 insns, 83.92% - induction right, but gcc
// inverts the loop so the "j test" block is gone); case 1 with continue and with
// nested if (identical output); case 2 with the inline cast form instead of the
// pFlags local (84.60 against 84.72). Preset screened: 2.6.0 -O2/-O3 84.60, -O1
// 61.64, 2.7.2 -O3 81.59, 2.7.2-cdk -O2/-O3 81.59, so 2.6.0 is right.
// Next lever: make the menuUnk2 load non-hoistable in SOURCE ORDER (retail cannot
// reuse it across the branch because the break block stores through *menuUnk2), or
// find a spelling gcc 2.6 leaves un-inverted.
// Best attempt: 95.51% (352 bytes = retail's exact size, 88 = 88 insns, 48 differing lines).
// 84.72 -> 95.51 this round by PORTING THE func_801CA480 LEVER (its park, 95.71) instead of
// writing from the disassembly: case 1 as a non-inverted while whose fail arm is 'i--; continue;'
// with pMenu read INSIDE the loop and the store/break after it, so the store block sits in the
// loop's linear instruction range and the 'lw 0x32C(g_Menu)' stays non-hoistable; case 0 keeps
// the plain descending for; case 2 reads menuUnk2 into the shared pMenu local. Also swapped the
// raw 0xFF for MENU_SLOT_NONE and 15 for MENU_CARD_SLOT_COUNT (no byte change).
// Residual, and it is one thing: a caller-saved a1/a2 mirror, not a callee-saved pair. Retail's
// case 0 loads pMenu into the register the dead g_Menu pointer just vacated (a0); case 1 keeps
// pMenu in a1 and the table biv in a2. This build puts pMenu in a2 (case 0/case 2 through it) so
// the table biv takes a1 and the 0xFF constant is pushed to t0; 48 of the 48 differing lines are
// that mirror, and case 2 additionally has the 'addiu a1,a1,-1' stolen into the beq delay slot
// where retail has 'lui v0,0x8888'. func_801CA480 has the identical residual at 95.71 (its own
// park note says neither declaration order nor a second pointer moves it), so one spelling that
// flips the pair closes both.
// Tried this round: case 2 with an explicit 'j = i / MENU_CARD_SLOT_COUNT' local read before the
// pMenu load (absorbed, byte-identical at 95.51). Preset already screened by the previous note
// (2.6.0 -O2 is right: -O3 84.60, -O1 61.64, 2.7.2 -O3 81.59, cdk -O2/-O3 81.59).
/*
void func_801CA5F0(s32 a0, s32 a1) {
    MenuUnk2* pMenu;
    s32 i;
    s32 j;

    switch (a0) {
    case 0:
        for (i = a1; i - 1 >= 0; i--) {
            pMenu = g_Menu->menuUnk2;
            j = D_801E9818[i];
            if (pMenu->slotRecordIndexes[j] != MENU_SLOT_NONE) {
                pMenu->selectedSlot = i - 1;
                break;
            }
        }
        break;
    case 1:
        i = a1;
        while (i - 1 >= 0) {
            pMenu = g_Menu->menuUnk2;
            j = D_801E9818[i];
            if (pMenu->slotRecordIndexes[j] == MENU_SLOT_NONE || pMenu->slotHasGameSave[j] == 0) {
                i--;
                continue;
            }
            pMenu->selectedSlot = i - 1;
            break;
        }
        break;
    case 2:
        pMenu = g_Menu->menuUnk2;
        i = a1 - 1;
        if (((u8*)&pMenu->cardStatus[0])[i / MENU_CARD_SLOT_COUNT] != 0 && i >= 0) {
            pMenu->selectedSlot = i;
        }
        break;
    }
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA5F0);


s32 MenuHandleCardSlotInput(s32 a0) {
    s32 result;

    result = 0;
    switch (g_Menu->input) {
    case MENU_INPUT_CONFIRM:
        result = 1;
        break;
    case MENU_INPUT_BACK:
        result = 2;
        break;
    case MENU_INPUT_RIGHT:
        func_801C9EF4(a0, g_Menu->menuUnk2->selectedSlot);
        break;
    case MENU_INPUT_LEFT:
        func_801CA1D4(a0, g_Menu->menuUnk2->selectedSlot);
        break;
    case MENU_INPUT_DOWN:
        func_801CA480(a0, g_Menu->menuUnk2->selectedSlot);
        break;
    case MENU_INPUT_UP:
        func_801CA5F0(a0, g_Menu->menuUnk2->selectedSlot);
        break;
    }
    if (g_Menu->menuUnk2->selectedSlot != g_Menu->menuUnk2->previousSelectedSlot) {
        MenuUpdateCharacterWindow(g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]],
            g_Menu->menuUnk2->slotHasGameSave[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]]);
        g_Menu->menuUnk2->previousSelectedSlot = g_Menu->menuUnk2->selectedSlot;
    }
    return result;
}

void func_801CA8C0(u_char a0) {
    s32 n;
    s32 tens;

    n = a0 + 1;
    memcpy(g_Menu->menuUnk2->saveTitle, D_801C50E0, 0x1B);
    g_Menu->menuUnk2->saveTitle[0x1A] = 0x82;
    tens = n / 10;
    g_Menu->menuUnk2->saveTitle[0x1B] = tens + 0x4F;
    g_Menu->menuUnk2->saveTitle[0x1C] = 0x82;
    g_Menu->menuUnk2->saveTitle[0x1D] = n % 10 + 0x4F;
    g_Menu->menuUnk2->saveTitle[0x1E] = 0x81;
    g_Menu->menuUnk2->saveTitle[0x1F] = 0x40;
    g_Menu->menuUnk2->saveTitle[0x20] = 0;
    g_Menu->menuUnk2->saveTitle[0x21] = 0;
    strcat(g_Menu->menuUnk2->saveTitle, g_Menu->menuUnk2->saveTitleSuffix);
}
// Waits up to MENU_CARD_CHANGE_WAIT_FRAMES frames for the two card status bytes to
// change, rendering a frame each pass, and returns the confirm/back result.
u_char MenuWaitWindowInputOrCardChange(u_char a0) {
    u_char cardStatus[2];
    u_char result;
    u_char looping;
    u8 frames;

    cardStatus[0] = g_Menu->menuUnk2->cardStatus[0];
    cardStatus[1] = g_Menu->menuUnk2->cardStatus[1];
    looping = 1;
    if (g_MenuCardChangeDetectionEnabled != 0) {
        g_Menu->menuUnk2->cardPollMode = 2;
    }
    g_MenuWindowTextBackPressed = 0;
    result = 0;
    frames = MENU_CARD_CHANGE_WAIT_FRAMES;
    while (1) {
        if (a0 == 0) {
            g_Menu->pCursors->shouldRender[2] = 0;
            g_Menu->pCursors->shouldRender[3] = 0;
            if (g_Menu->input != MENU_INPUT_IDLE) {
                break;
            }
            frames -= 1;
            if (frames == 0) {
                break;
            }
        }
        MenuRenderFrame();
        if (a0 != 0 && g_MenuCardChangeDetectionEnabled != 0) {
            if (cardStatus[0] != g_Menu->menuUnk2->cardStatus[0]) {
                g_Menu->menuUnk2->cardEnumerated[0] = 0;
                g_Menu->input = MENU_INPUT_BACK;
                g_MenuCardSaveListStale = 1;
            }
            if (cardStatus[1] != g_Menu->menuUnk2->cardStatus[1]) {
                g_Menu->menuUnk2->cardEnumerated[1] = 0;
                g_Menu->input = MENU_INPUT_BACK;
                g_MenuCardSaveListStale = 1;
            }
        }
        switch (g_Menu->input) {
        case MENU_INPUT_CONFIRM:
            looping = 0;
            break;
        case MENU_INPUT_BACK:
            result = 0;
            // Load-bearing order: with looping assigned last, gcc merges this arm's
            // tail into MENU_INPUT_CONFIRM's and the function comes out 4 bytes short.
            looping = 0;
            g_MenuWindowTextBackPressed = 1;
            break;
        case MENU_INPUT_LEFT:
            g_Menu->pCursors->shouldRender[2] = 1;
            g_Menu->pCursors->shouldRender[3] = 0;
            result = 1;
            break;
        case MENU_INPUT_RIGHT:
            g_Menu->pCursors->shouldRender[2] = 0;
            g_Menu->pCursors->shouldRender[3] = 1;
            result = 0;
            break;
        }
        if (!looping) {
            break;
        }
    }
    g_Menu->pCursors->shouldRender[2] = 0;
    g_Menu->pCursors->shouldRender[3] = 0;
    g_Menu->menuUnk2->cardPollMode = 0;
    return result;
}
u_char MenuLoadWindow2Text(u_char a0, u_char a1, u_char a2) {
    u_char result;

    MenuCreateWindow2Strings(a0);
    g_Menu->pCursors->shouldRender[3] = 1;
    result = MenuWaitWindowInputOrCardChange(a2);
    MenuFreeWindow2Strings();
    if (a1 != MENU_WINDOW_TEXT_NONE && result != 0) {
        MenuCreateWindow2Strings(a1);
        g_Menu->pCursors->shouldRender[3] = 1;
        result = MenuWaitWindowInputOrCardChange(a2);
        MenuFreeWindow2Strings();
    }
    return result;
}

void MenuResetSlotSelection(void) {
    g_Menu->menuUnk2->previousSelectedSlot = MENU_SLOT_NONE;
    g_Menu->menuUnk2->selectedSlot = 0;
    if (g_Menu->menuUnk2->cardStatus[0] == 0 && g_Menu->menuUnk2->cardStatus[1] != 0) {
        g_Menu->menuUnk2->selectedSlot = MENU_CARD_SLOT_COUNT;
    }
}

// The cell grid panel block g_Menu->pCellGrid points at: the row quad and the two
// strings MenuScrollCellGridRow draws, then the scroll that drives the quad's width.
// (misc4_260.c declares the same block locally; it is the same allocation.)
typedef struct {
    /* 0x000 */ POLY_F4 quads[2];          // The row quad, one per render context
    /* 0x030 */ POLY_FT4 glyphs12[12][2];  // A twelve glyph string, one prim per render context
    /* 0x3F0 */ POLY_FT4 glyphs2[2][2];    // A two glyph string
    /* 0x490 */ u8 unk490[0x320];
    /* 0x7B0 */ s32 scrollX;        // Grows by scrollStep per scroll frame, wraps past 0x100
    /* 0x7B4 */ s32 scrollStep;
    /* 0x7B8 */ u_char renderContext; // Picks this frame's prim out of every pair above
    /* 0x7B9 */ u8 unk7B9[3];
} MenuCellGridPanel; // Size: 0x7BC

extern s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, u16 x, u16 y, u16 scale);
extern void SoundPlaySoundEffectOnFreePair(s32 arg0);

// pCellGrid is a void*, so every access reloads and re-casts it: retail reloads it
// for each of the three colour bytes, so a pointer local must not be used here.
// Case 0 frees the panel, 1 builds it, 2 stops its scroll.
void MenuSetCellGridPanelState(u_char mode) {
    switch (mode) {
    case MENU_DATA_FREE:
        if (g_Menu->pManager->cellGridScrollState != 0) {
            g_Menu->pManager->cellGridScrollState = 0;
            MenuRenderFrame();
            HeapFree(g_Menu->pCellGrid);
        }
        break;
    case MENU_DATA_INITIALIZE:
        g_Menu->pCellGrid = HeapAlloc(sizeof(MenuCellGridPanel), 0);
        bzero(g_Menu->pCellGrid, sizeof(MenuCellGridPanel));
        // int-first address so the sum reaches $a0 as `addu a0,a0,v0`; every typed
        // spelling (`->quads[i]`, `quads + i`, a u8* base) gives the swapped operands
        // (measured: `&->quads[renderContext]` 99.96%, `->quads[renderContext].r0` 99.73%).
        SetPolyF4((POLY_F4*)(g_Menu->renderContext * sizeof(POLY_F4) + (s32)g_Menu->pCellGrid));
        (g_Menu->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->r0 = 0xA0;
        (g_Menu->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->g0 = 0xA0;
        (g_Menu->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->b0 = 0;
        func_8002675C(g_Menu->resources, 0x160, ((MenuCellGridPanel*)g_Menu->pCellGrid)->glyphs12[0],
                      g_Menu->renderContext, 0xA0, 0x64, 0x1000);
        func_8002675C(g_Menu->resources, 0x161, ((MenuCellGridPanel*)g_Menu->pCellGrid)->glyphs2[0],
                      g_Menu->renderContext, 0xA0, 0x64, 0x1000);
        ((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext = *(u_char*)&g_Menu->renderContext;
        g_Menu->pManager->cellGridScrollState = 1;
        ((MenuCellGridPanel*)g_Menu->pCellGrid)->scrollStep = 8;
        SoundPlaySoundEffectOnFreePair((g_Menu->pSoundFile->sedId << 16) | 0xE0);
        SoundPlaySoundEffectOnFreePair((g_Menu->pSoundFile->sedId << 16) | 0xE1);
        SoundPlaySoundEffectOnFreePair((g_Menu->pSoundFile->sedId << 16) | 0x8F);
        break;
    case 2: // panel built, stop its scroll
        if (g_Menu->pManager->cellGridScrollState == 0) {
            break;
        }
        ((MenuCellGridPanel*)g_Menu->pCellGrid)->scrollX = 0x100;
        (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->r0 = 0;
        (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->g0 = 0xA0;
        (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->b0 = 0;
        g_Menu->pManager->cellGridScrollState = 2;
        break;
    }
}

/* Best attempt: 89.15% (268 bytes, 66 insns) under XenoMenu260 (gcc-2.6.0); 86.42% under
   Default (2.7.2). Every instruction of both loops, the decode call and the copy-back is
   retail's except the odd-byte base &g_GameState + 1: retail's `lui %hi(g_GameState+1)` /
   `addiu %lo(...)` pair sits at the TOP OF THE OUTER LOOP BODY and lands in the
   call-clobbered $t0 (4 saves, 280 bytes), while this build folds it to `addiu $s2,$s0,1`
   in the PROLOGUE off the strength-reduced row pointer $s0 and keeps it across the
   SystemDecodeGlyphCodesN call (5 saves, the fifth being $s4; 268 bytes, and the odd
   address becomes `addu v1,v0,s2` where retail has `addu v1,v0,t0`).
   Mechanism, i.e. why this is not a missing spelling: loop.c refuses to hoist a `high`
   insn, so retail's lui/addiu pair could never leave the loop; the pair turns into a
   hoistable plain addiu only when a register already holds &g_GameState, and the even
   read's LSR pointer ($s0 = &gs + offset, which retail's own `move a1,s0` /
   `addiu a1,a1,2` walk proves existed) always provides one - a later cse/combine pass then
   folds symbol+1 into s0+1. Measured on this body: `pOdd = (u8*)&g_GameState + 1` read as
   pOdd[offset + j] and the indexed `((u8*)&g_GameState)[offset + j + 1]` are byte-identical
   (both fold to s2 = s0 + 1); pointer locals for the row / both reads, -O3 (87.02), cdk (cc1
   rejects this body), 2.7.2 (86.42).
   TWIN: slus_006.64/system/temp3_b func_8001B970 is the same loop skeleton (that unit
   loads the string bank first, then runs this loop with 0xF as the terminator instead of 0)
   and parks at 86.53% on the SAME fifth save - see the lever
   g-which-four-invariants-are-hoisted, whose .s also shows retail rematerialising
   `lui/addu %lo(g_GameState + 0x1)` per row. Whoever closes one closes both. */
// Best attempt: 87.02% (268 bytes, 67 insns) under XenoMenu260 (gcc-2.6.0) against retail's 264 bytes / 66 insns; 86.42% under Default (2.7.2), 87.02 -O3. Re-measured this round: the note that read 89.15% was measured on a body whose odd base was not folded, and it is not reproducible - this is the honest number for the body below.
//    The residual is ONE hoisted invariant and the fifth save it costs. Retail rematerialises the odd byte's base as lui %hi(g_GameState + 0x1) / addiu %lo(...) into the call-clobbered $t0 at the TOP OF EVERY ROW BODY; the compile folds symbol+1 onto $s0 (which holds &g_GameState in the prologue) into 'addiu $s2,s0,1', hoists it out of the loop, and that pseudo takes a fifth callee-saved register: retail's invariant set is $s0 = pRow, $s1 = offset, $s2 = inBuf + 1, $s3 = inBuf (4 saves, 0x58 frame, ra at 0x50), ours is those plus $s4 = inBuf and $s2 = base + 1 (5 saves, ra at 0x54). Everything else in the body is retail's instruction for instruction, including both induction variables, the j/2 svr/sra pair, the inner loop's double lbu of each byte and the copy-back walk.
//    Measured dead this round: odd index as j + offset + 1 (86.86), odd read via &g_GameState.strings (byte-identical), u8 *pOdd = (u8*)&g_GameState + 1 assigned as the FIRST statement of the row body and read as pOdd[offset + j] (86.94, still folded, so cse2 after loop.c is folding it and not the source spelling - same verdict as the lever g-which-four-invariants-are-hoisted reached on the twin temp3_b func_8001B970). A 3-minute permuter run (1271 iterations, base 801 best 555) won only by moving 'pEntry = g_GameState.strings;' into the break arm, where it is DEAD and leaves pEntry uninitialised in the loop: wrong code, rejected, do not port it. The twin (slus_006.64/system/temp3_b func_8001B970) parks on the same seam at 89.50, so whoever closes one closes both.
/*
void func_801CB184(void) {
    u8 inBuf[0x14];
    u8 outBuf[0x14];
    StringEntry* pEntry;
    s32 offset;
    s32 j;

    pEntry = g_GameState.strings;
    for (offset = 0; offset < 0x26C; offset += 0x14) {
        for (j = 0; j < 0x14; j += 2) {
            inBuf[j] = ((u8*)pEntry)[j];
            inBuf[j + 1] = ((u8*)&g_GameState)[offset + j + 1];
            if (((u8*)pEntry)[j] == 0 && ((u8*)&g_GameState)[offset + j + 1] == 0) {
                break;
            }
        }
        SystemDecodeGlyphCodesN((u16*)inBuf, outBuf, j / 2);
        for (j = 0; j < 0x14; j++) {
            ((u8*)pEntry)[j] = outBuf[j];
        }
        pEntry++;
    }
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB184);




void MenuApplySaveSlot(u32* pSrc) {
    int i;

    MenuSaveSlotReadGameState(pSrc, g_Menu->pDressingRoom);
    g_SpuChannelMask = *pSrc;
    for (i = 0; i < 0x10; i++) {
        D_8005A3A0[i] = D_8006F958[i];
    }
    func_801CB184();
}

/* Best attempt: 98.56% (1440 bytes vs retail's 1448; 360 vs 362 insns) under XenoMenu260
   (gcc-2.6.0), 110 differing lines - the same body the previous lane left at 90.42. THREE
   measured finds this round:
   (1) BLOCK LAYOUT, the residual the old note called unreachable - it IS reachable, by
   source order alone. gcc 2.6.0 emits each block where its label is WRITTEN (the old probe
   that showed "no reordering" was right, and that is exactly why label position is the
   whole control), so retail's cold islands at 0x65A4 (close + j HeapFree) and 0x65B4 (the
   three-call resource arm + j HeapFree) are simply the source's textual order. Put the
   happy path AFTER them, reached by `if (MenuLoadWindow2Text(0x65, ...) != 0) goto
   cardRead;` whose false arm is the two statements ending in `goto updateCursor` - the
   then-arm's trailing goto is what skips the islands, and gcc inverts the test to
   `bnez v0,<happy path>`, exactly retail. 90.42 -> 96.83%, both islands at retail's own
   offsets, and it also turned the close/j and resource-arm/j tails into retail's
   `j <HeapFree>` shape.
   (2) THE -1 IS A VARIABLE: retail's `li s0,-1` in the open-loop preheader feeding
   `bne s2,s0` is a local holding -1, not the literal, which rematerialises as `li a3,-1`
   at the compare. `s32 noFd; ... noFd = -1; if (fd == noFd)` - 96.83 -> 98.26%, 132 -> 110
   differing lines. Found by `permute.sh --minutes 4 -j 2` (PERMUTER_BETTER base=1774 best
   1123, its `new_var` was the only edit that mattered), then hand-checked: it changes no
   value and is the preheader store retail has, so it is right code.
   (3) The checksum-ok arm needs an explicit `pData = pBuffer + 0x100;` before it, and
   MenuApplySaveSlot takes pData: retail reloads pData there and passes it (`addiu
   a0,s4,256` filling the beq delay slot, then `move a0,s0`) rather than forming the sum
   inline. 98.26 -> 98.56%.
   NAMED RESIDUALS left, all allocator or scheduler, 2 instructions in total:
   (a) THE $s0/$s1 MIRROR: retail keeps retry in $s1 and pData in $s0, this body mirrors
   them. Look up tools/rounds/levers.md under regalloc/callee-saved tie first; a permuter
   run from THIS body is the cheap next move - the 4 minutes spent above went on the -1
   find, not on this.
   (b) sum loop, 1 instruction: retail has `addu v0,a0,v0` then `move a0,v0` in the bgez
   delay; `sum += *pData++` gives the in-place `addu a0,a0,v0` in that slot. Measured dead
   this round: a second accumulator (`total = sum + *pData++; sum = total;`) compiles
   byte-identically to the plain form, so the two-pseudo shape cannot be asked for that way.
   (c) HeapAlloc preheader, placement only: retail has `move s3,zero` after the two li's,
   this body puts it in the branch delay slot.
   (d) `result = 0` in the slot==MENU_SLOT_NONE arm is scheduled two instructions earlier
   here than retail's, which puts the sb just before the MenuLoadWindow2Text call.
   (e) the prefix's s16 is spelled g_MenuCardPathPrefix0 + 4 against retail's D_801C50AC -
   same address, relocation only (SECOND EXCEPTION class).
   Keep from the old note, still true: MenuIsSlotUsable must stay declared s32 at the call
   site (retail has no andi after the call), and the old preset numbers (-O1 70.63,
   -O3 51.91, 2.7.2 78.09, 2.7.2-cdk 76.13) were measured on the 84% body - re-measure
   before believing them. This round: 2.7.2 -O2 90.19 / -O3 68.64, cdk -O3 68.48 on the
   90.42 body. */
/* Best attempt: 98.86% (1444 bytes vs retail's 1448; 361 vs 362 insns) under XenoMenu260
//    (gcc-2.6.0), 107 differing lines. Re-measured fresh in this redo lane from the 98.56 park
//    left by the previous round; the ONE find this round is a lever:
//    (1) THE SUM ACCUMULATOR IS A u_char, NOT s32. Retail's loop is `addu v0,a0,v0` + `move a0,v0`
//    in the bgez delay slot where `sum += *pData++` with `s32 sum` gives the in-place
//    `addu a0,a0,v0`, one instruction short; the previous note's "second accumulator is
//    byte-identical" was right and the axis was the TYPE. `u_char sum` (with the then-redundant
//    `(u8)sum` cast dropped from the compare) - semantically the same, because retail masks the
//    promoted value with `andi v0,a0,0xff` only at the compare - reproduces retail's two-insn
//    add exactly: 98.56 -> 98.86%. Found by permute.sh (base 873 -> 573, 1643 iterations).
//    NAMED RESIDUALS, ONE instruction and one register pair in total:
//    (a) THE LAST INSTRUCTION IS A LOAD-DELAY SLOT THIS BODY FILLS. Retail leaves a `nop` in the
//    slot after `lw v0,%lo(g_Menu)(v0)` at 0x63FC and stores `result = 0` (`sb zero,0x58(sp)`) at
//    0x6408 just before the jal; this body's dbr fills that slot with the same store and emits one
//    instruction fewer. Measured dead: writing the arm as `result = 0;` AFTER the second
//    `pManager->characterWindowBuilt = 0;` instead of before it is BYTE-IDENTICAL (gcc reorders the
//    two stores either way), so statement order is not the knob.
//    (b) THE $s0/$s1 MIRROR: retail keeps pData in $s0 (with noFd sharing it) and retry in $s1;
//    this body mirrors them (retry $s0, pData and noFd $s1). Measured dead: splitting the read
//    loop's counter into its own `u_char readRetry` (two disjoint lives could share $s1 the way
//    retail's do) is byte-identical; a second permuter run from this body (base 573, 2062
//    iterations) returned only WRONG-CODE candidates - duplicating `open(path,1)` and
//    `offset += 0x100`/`pData += 0x100` inside both arms of a dead `if`, and substituting the
//    u_char `card` for the s32 `offset` (truncates) - all rejected.
//    Still true from the old note: the copy-loop arm's `MenuIsSlotUsable` must stay declared s32 at
//    the call site (retail has no andi after the call); the block layout (cold islands at 0x65A4
//    and 0x65B4 before the happy path, reached by `if (MenuLoadWindow2Text(0x65,...) != 0) goto
//    cardRead;`), the `s32 noFd = -1` variable, and the explicit `pData = pBuffer + 0x100;` before
//    the checksum arm are all load-bearing. Presets re-measured on the 90.42 body: 2.7.2 -O2 90.19,
//    -O3 68.64, cdk -O3 68.48. */
/*
u_char func_801CB304(void) {
    char path[0x40];
    s32 ready;
    u_char result;
    s32 repeat;
    s32 slot;
    s32 fd;
    s32 noFd;
    s32 fileFd;
    u_char retry;
    u_char card;
    u8* pBuffer;
    u8* pData;
    s32 offset;
    u_char sum;
    s32 count;

    result = 1;
    ready = 1;
    repeat = 1;
    MenuResetSlotSelection();
    while (repeat) {
        if (func_801C93A8() & 0xFF) {
            break;
        }
        if (ready) {
            if (g_Menu->pManager->window2StringsActive) {
                MenuFreeWindow2Strings();
            }
            ready = 0;
            g_Menu->pCursors->shouldRender[0] = 1;
        }
        if (!MenuIsSlotUsable(1)) {
            slot = MenuFindFirstValidSlot(1);
            g_Menu->menuUnk2->selectedSlot = slot;
            if (slot == MENU_SLOT_NONE) {
                g_Menu->pManager->shouldRenderCursors = 0;
                g_Menu->pManager->characterWindowBuilt = 0;
                g_Menu->cardSlotState = 1;
                result = 0;
                g_Menu->pManager->characterWindowBuilt = 0;
                MenuLoadWindow2Text(0x62, MENU_WINDOW_TEXT_NONE, 0);
                break;
            }
        }
        g_Menu->pManager->shouldRenderCursors = 1;
        switch (MenuHandleCardSlotInput(1)) {
        case 1:
            g_Menu->pCursors->shouldUpdatePosition[0] = 0;
            g_Menu->menuUnk2->cardPollMode = 0;
            card = 0;
            if (g_Menu->menuUnk2->selectedSlot < MENU_CARD_SLOT_COUNT) {
                memcpy(path, &g_MenuCardPathPrefix0, 6);
            } else {
                memcpy(path, &g_MenuCardPathPrefix1, 6);
                card = 1;
            }
            strcat(path, g_Menu->menuUnk2->records[g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]]].ids);
            if (MenuLoadWindow2Text(0x65, MENU_WINDOW_TEXT_NONE, 1) != 0) {
                goto cardRead;
            }
            g_Menu->menuUnk2->previousSelectedSlot = MENU_SLOT_NONE;
            goto updateCursor;
        cardReadFailed:
            close(fileFd);
            goto freeBuffer;
        cardChecksumOk:
            MenuLoadResourceSet(1);
            MenuApplySaveSlot((u32*)pData);
            MenuLoadResourceSet(0x11);
            goto freeBuffer;
        cardRead:
            MenuCreateWindow2Strings(0x3B);
            retry = 5;
            g_Menu->soundEffectsEnabled = 0;
            fd = -1;
            noFd = -1;
            do {
                fd = open(path, 1);
                if (fd == noFd) {
                    fd = 0;
                    MenuWaitCardOperationFrames(card);
                }
                fileFd = fd;
            } while (fd == 0 && --retry != 0);
            offset = 0;
            if (fd != 0) {
                g_Menu->pManager->shouldRenderCursors = 0;
                pBuffer = HeapAlloc(0x2100, 1);
                pData = pBuffer;
                g_Menu->pManager->characterWindowBuilt = 0;
                MenuSetCellGridPanelState(1);
                do {
                    MenuRenderFrame();
                    retry = 5;
                    do {
                        if (read(fd, pData, 0x100) != 0x100) {
                            fd = 0;
                            MenuWaitCardOperationFrames(card);
                        }
                    } while (fd == 0 && --retry != 0);
                    if (fd == 0) {
                        goto cardReadFailed;
                    }
                    offset += 0x100;
                    pData += 0x100;
                } while (offset < (g_Menu->menuUnk2->saveBlockCount << 13));
                close(fd);
                pData = pBuffer + 0x100;
                sum = 0;
                count = 0x1EFE;
                do {
                    sum += *pData++;
                } while (--count >= 0);
                if (sum == *pData) {
                    pData = pBuffer + 0x100;
                    goto cardChecksumOk;
                }
                fd = 0;
            freeBuffer:
                HeapFree(pBuffer);
            }
            MenuSetCellGridPanelState(2);
            MenuFreeWindow2Strings();
            if (fd != 0) {
                repeat = 0;
                g_Menu->soundEffectsEnabled = 1;
                MenuPlaySoundEffect(0x34);
                g_Menu->soundEffectsEnabled = 0;
                MenuLoadWindow2Text(0x5C, MENU_WINDOW_TEXT_NONE, 0);
            } else {
                MenuLoadWindow2Text(0x3E, MENU_WINDOW_TEXT_NONE, 0);
                g_Menu->menuUnk2->cardPollMode = 2;
            }
            MenuSetCellGridPanelState(0);
            g_Menu->menuUnk2->cardEnumerated[0] = 0;
            g_Menu->menuUnk2->cardEnumerated[1] = 0;
            g_Menu->menuUnk2->previousCardSlotCounts[0] = MENU_SLOT_NONE;
            g_Menu->menuUnk2->previousCardSlotCounts[1] = MENU_SLOT_NONE;
        updateCursor:
            g_Menu->pCursors->shouldUpdatePosition[0] = 1;
            break;
        case 2:
            repeat = 0;
            result = 0;
            g_Menu->soundEffectsEnabled = 1;
            break;
        }
    }
    g_Menu->menuUnk2->cardPollMode = 1;
    g_Menu->unk334 = 1;
    return result;
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB304);




u_char MenuWaitForCardStatusChange(u_char a0) {
    MenuUnk2* pMenu;
    u8 prev[2];
    u_char changed;
    u_char result;

    prev[0] = g_Menu->menuUnk2->cardStatus[0];
    prev[1] = g_Menu->menuUnk2->cardStatus[1];
    MenuCreateWindow2Strings(a0 * 3 + 0x29);
    g_Menu->input = MENU_INPUT_IDLE;
    g_Menu->menuUnk2->cardPollMode = 2;
    result = 0;
    changed = 0;
    while (g_Menu->input == MENU_INPUT_IDLE) {
        MenuRenderFrame();
        pMenu = g_Menu->menuUnk2;
        if (prev[0] != pMenu->cardStatus[0] || prev[1] != pMenu->cardStatus[1]) {
            changed = 1;
            break;
        }
    }
    MenuFreeWindow2Strings();
    if (changed == 0) {
        result = MenuLoadWindow2Text(0x2F, MENU_WINDOW_TEXT_NONE, 1);
    }
    return result;
}
u_char MenuFindFreeChunkIndex(u_char a0, u_char a1) {
    u_char found;
    int i;

    found = 0;
    if (a1 == MENU_CHUNK_INDEX_NONE) {
        for (i = 0; i < MENU_CARD_SLOT_COUNT; i++) {
            if (g_MenuCardChunkInUse[a0][i] == 0) {
                found = i;
                break;
            }
        }
        return found;
    }
    return a1;
}

extern int ArchiveGetDiscNumber(void);
extern int SystemEncodeGlyphCodes(u8* pInput, u16* pOutput);
extern u8 g_MenuSaveSlotForceDiscOne;

/* Header of a save slot (the first 0x24 bytes of the record
   MenuSaveSlotWriteGameState fills in): a per-party-slot character summary. */
typedef struct {
    /* 0x00 */ u32 spuChannelMask;
    /* 0x04 */ u16 hp[3];
    /* 0x0A */ u16 maxHp[3];
    /* 0x10 */ u8 mp[3];
    /* 0x13 */ u8 maxMp[3];
    /* 0x16 */ u8 level[3];
    /* 0x19 */ u8 unk19[3];
    /* 0x1C */ u8 characterIds[3];
    /* 0x1F */ u8 unk1F;
    /* 0x20 */ u8 unk20[3];
    /* 0x23 */ u8 index; // The chunk index this save is stored at on the card, as in MenuUnk2ImageChunk.index
} MenuSaveSlotHeader; // Size: 0x24

extern void MenuSaveSlotWriteGameState(MenuSaveSlotHeader* pSlot);

/* Best attempt: 96.29% (836 bytes = retail's exact size, 209 = 209 instructions;
   everything matches except that retail has the text walker in $s0 and the loop
   index in $s1 and this build mirrors them, which also puts the g_GameState
   address formation one statement earlier in the preheader).
   Re-measured this round: still 96.29% / 836 bytes / 209 insns, and the whole diff
   is the $s0/$s1 mirror plus the 4-instruction preheader order difference that
   follows from it: retail forms the walker's address (lui/addiu g_GameState) in
   the FIFTH preheader slot, this build in the FIRST, so the walker's live_length
   here is 36 where retail's position gives 32 - and 32 is inside the <= 34 window
   that would flip the pair. Measured this round, that window is NOT reachable from
   source order: moving `pText = (u8*)g_GameState.strings;` from after the three
   header stores to before them measures the same 96.29 with a byte-identical
   emission, because sched2 hoists the address (no dependencies) to the head of the
   block in both spellings. So the order is a consequence of the allocation, not a
   lever on it - do not spend a lane re-ordering the preheader.
   The mirror is DECIDED ARITHMETICALLY, not by spelling: gcc-2.6.0's global.c
   allocno_compare processes the callee-saved candidates in descending
   floor_log2(refs) * refs * 10000 / (live_length * size), and this body's two
   are 76 = the shared index (28 refs over 117 insns -> 2393) and 75 = the
   walker (11 refs over 36 insns -> 2291) per -da m.i.greg / m.i.lreg. For the
   walker to take $s0 first it needs live_length <= 34 (two instructions less)
   or 12 refs at live_length <= 37 (tools/rounds/levers.md
   a-a-near-miss-allocno-quotient records the same numbers). Its live range starts at the preheader
   lui/addiu, which loop.c hoists to the head of the block, so source order
   cannot move it: initialising the walker last in the preheader measures the
   same 96.29, and a separate per-loop index (which frees the walker into $s0
   upstream, at 95.93 with 68 differing lines) is the wrong structure -- retail
   reuses $s1 for the character loop and the text loop, so the index is ONE
   variable live across the SystemEncodeGlyphCodes call. Also measured dead:
   an indexed access (&g_GameState.strings[i] inside the loop) at 93.67% /
   852 bytes / 213 insns, and declaring the walker before the index (no change).
   Two earlier residuals are GONE by using the shape below, so do not redo them:
   - mp/maxMp are BYTE loads in retail (0x2BC/0x2BE) while GameCharacter
     declares them u_short; the only spelling that folds to retail's per-site
     lui at / addu at / lbu is a member access on the declared type, i.e.
     unk54[-4] / unk54[-2] (0x54-4 = 0x50). Retyping the header is not an
     option: field/party/stats matches on the u_short form.
   - the copy loop counter must be its OWN variable (n below): sharing one `i`
     with the later loops makes it live across the calls, so it takes a saved
     register where retail uses $a1.
   Structure notes: MenuSaveSlotHeader* (a0), one dead parameter (a1) and a byte
   (a2) dropped into header 0x23; ends by writing the disc number to
   g_GameState+0x19D4 and calling MenuSaveSlotWriteGameState + func_801CB184.
   The text loop is over g_GameState.strings (31 entries of 0x14 = StringEntry),
   zero-filling a 0x14-byte out buffer (byte arrays, so the zero fill is sb),
   calling SystemEncodeGlyphCodes and copying the buffer back; the "id == 0xFF"
   arm is out of line at the end of the body (beq to it). Fixed-width copy of
   0x10 halfwords from D_8005A3A0 to D_8006F958 at the top. */
// Re-measured again (round 457 redo lane): un-parked, compiled, scored 96.29% / 836 bytes / 209 insns - unchanged, the body in the note above is the best one. Do not un-park again without a NEW idea for the walker's allocno quotient: the ~4% margin (2393 vs 2291) needs the walker's live_length <= 34 or 12 refs, and its def is a dependency-free la that loop.c hoists to the head of the preheader (both source positions measured identical), so statement order cannot move it.
/*
void func_801CBA4C(MenuSaveSlotHeader* pSlot, s32 unused, u8 index) {
    u8 inBuf[0x14];
    u8 outBuf[0x14];
    u8* pText;
    s32 i;
    s32 j;
    s32 n;
    s32 discNumber;

    for (n = 0; n < 0x10; n++) {
        D_8006F958[n] = D_8005A3A0[n];
    }
    for (i = 0; i < 3; i++) {
        if (g_Menu->pManager->currentCharacterIDs[i] != 0xFF) {
            pSlot->characterIds[i] = g_Menu->pManager->currentCharacterIDs[i];
            pSlot->hp[i] = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].hp;
            pSlot->maxHp[i] = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].maxHp;
            pSlot->mp[i] = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].unk54[-4];
            pSlot->maxMp[i] = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].unk54[-2];
            pSlot->level[i] = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].level;
            pSlot->unk19[i] = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].unk63;
        } else {
            pSlot->characterIds[i] = 0xFF;
        }
    }
    pSlot->unk1F = 0;
    pSlot->index = index;
    pSlot->spuChannelMask = g_SpuChannelMask;
    pText = (u8*)g_GameState.strings;
    for (i = 0; i < 0x1F; i++) {
        for (j = 0; j < 0x14; j++) {
            inBuf[j] = pText[j];
            outBuf[j] = 0;
        }
        SystemEncodeGlyphCodes(inBuf, (u16*)outBuf);
        for (j = 0; j < 0x14; j++) {
            pText[j] = outBuf[j];
        }
        pText += 0x14;
    }
    if (g_MenuSaveSlotForceDiscOne == 0) {
        discNumber = ArchiveGetDiscNumber() - 1;
    } else {
        discNumber = 1;
    }
    g_GameState.fieldScriptMemory[0x52] = discNumber;
    MenuSaveSlotWriteGameState(pSlot);
    func_801CB184();
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CBA4C);


// Register map (still current, re-verified with the 85.37 body): retail s0 ok/retry, s1 fd, s2 pData,
// s3 offset, s4 pBuffer, s5 card, s6 chunkIndex, s7 &path, fp the 0xFF constant; spilled arg0 0xA8,
// repeat 0xB0, fileFd 0xB8, ready 0xC0, result 0xC8, chunkIndex-before-the-call 0xD0, vars=200,
// frame 0x100. Ours: s4 &path, s7 chunkIndex, fp repeat, and only FOUR spills - arg0 0xA0, ready
// 0xB0, result 0xB8, fileFd; vars=176, frame 0xE8. prefix 0x90/0x94 and chunkDigit 0x98/0x99 land on
// retail's exact offsets.
// Best attempt: 85.37% (2312 bytes vs retail's 2376, 578 insns vs 594) - re-measured fresh in this
// redo lane; the 78.17 body it replaced was structurally wrong in two places. Both fixes are proven:
// (1) The `format(&prefix)` test is written with the NON-ZERO arm FIRST: `if (format(&prefix) != 0)
//     { MenuFreeWindow2Strings(); MenuLoadWindow2Text(0x5C,...); } else { ok = 0; free; repeat = 0; }`.
//     Retail lays the != 0 arm out as the fall-through (0x6FC8 free, 0x6FD8 load, then the == 0 arm
//     out of line at 0x6FE8), which an `== 0` test does not: 78.17 -> 79.20.
// (2) The recordIndex chain is NESTED with negated tests, not an else-if chain:
//       if (rec != MENU_SLOT_NONE) {
//           if (slotHasGameSave[rec] != 0) {
//               if (MenuLoadWindow2Text(0x38,..,1)) happy else { ok = 0; prev = NONE; }
//           } else { MenuLoadWindow2Text(0xC4,..,0); updateCursor = 1; break; }
//       } else if (MenuLoadWindow2Text(0x5F,..,1) == 0) { ok = 0; prev = NONE; }
//     An else-if chain puts the 0xC4/0x5F arms before the happy path; retail has them after it, in
//     reverse order (0xC4 at 0x7128, 0x5F at 0x7154, the shared ok=0 arm at 0x716C): 79.20 -> 85.37.
//     With (1) and (2) the 62 jal calls are retail's SEQUENCE, not just its multiset, except for the
//     one block in (a).
// NAMED RESIDUALS:
// (a) The copy-loop failure arm (close(fileFd), strcpy(path,prefix), strcat(path,D_801C50B8), erase)
//     is at 0x73C0 in retail - between the header-write retry loop and HeapAlloc - because retail
//     reaches it from the copy loop with a jump BACK; this body emits it in place, after the copy
//     loop. Same calls, only the position differs (~10 instructions misaligned).
// (b) 16 instructions of spill traffic. Frame 232 (vars 176) here vs retail 256 (vars 200), and the
//     callee-saved map rotates by one. Retail spills SIX scalars, 8 bytes apart - arg0 0xA8, repeat
//     0xB0, fileFd 0xB8, ready 0xC0, result 0xC8, chunkIndex 0xD0 - keeps MENU_SLOT_NONE (0xFF) in
//     $fp, live from 0x6DE0 to 0x71FC, and &path in $s7; chunkIndex therefore needs its own $s6.
//     This body spills FOUR (arg0 0xA0, ready 0xB0, result 0xB8, fileFd), keeps repeat in $fp, &path
//     in $s4, and chunkIndex in $s7 - because the 0xFF pseudo dies before chunkIndex is born, so the
//     two share $s7 (retail's fp use in the MenuFindFreeChunkIndex delay slot at 0x71FC proves they
//     were distinct values there).
// Tried and MEASURED in this lane: a source-level `u_char none = MENU_SLOT_NONE` used at all seven
// NONE sites (2316 bytes, code identical - cse constant-propagates it, so the fp-in-0xFF shape is
// still unexplained); inlining the recordIndex expression at each use (identical bytes); presets
// 2.6.0 -O1 70.63, -O3 51.91, -G0 does not compile, 2.7.2 -O2 78.09, 2.7.2 -O3 51.84, 2.7.2-cdk
// 76.13 - so the unit's 2.6.0 -O2 is right. Not tried: the permuter (structure moved twice here, and
// 85 is below the 99 gate).
/*
u_char func_801CBD90(u_char a0) {
    char newPath[0x40];
    char path[0x40];
    MenuCardPrefix prefix;
    char chunkDigit[2];
    u_char arg0;
    s32 repeat;
    s32 fileFd;
    u_char ready;
    u_char result;
    u_char chunkIndex;
    u_char card;
    u_char ok;
    u_char retry;
    u_char textId;
    s32 slot;
    s32 recordIndex;
    s32 fd;
    u8* pBuffer;
    u8* pData;
    u8* pSum;
    s32 offset;
    s32 sum;
    s32 count;

    chunkIndex = MENU_CHUNK_INDEX_NONE;
    ready = 1;
    arg0 = a0;
    repeat = 1;
    result = 0;
    MenuResetSlotSelection();
    while (repeat) {
        if (func_801C93A8() & 0xFF) {
            result = 1;
            break;
        }
        if (ready) {
            if (g_Menu->pManager->window2StringsActive) {
                MenuFreeWindow2Strings();
            }
            ready = 0;
            g_Menu->pCursors->shouldRender[0] = 1;
        }
        if (!MenuIsSlotUsable(2)) {
            slot = MenuFindFirstValidSlot(2);
            g_Menu->menuUnk2->selectedSlot = slot;
            if (slot == MENU_SLOT_NONE) {
                MenuLoadWindow2Text(0xAC, MENU_WINDOW_TEXT_NONE, 0);
                break;
            }
        }
        g_Menu->pManager->shouldRenderCursors = 1;
        switch (MenuHandleCardSlotInput(2)) {
        case 1:
            g_Menu->pCursors->shouldUpdatePosition[0] = 0;
            g_Menu->menuUnk2->cardPollMode = 0;
            ok = 1;
            if (g_Menu->menuUnk2->selectedSlot < MENU_CARD_SLOT_COUNT) {
                memcpy(&prefix, &g_MenuCardPathPrefix0, 6);
                card = 0;
            } else {
                memcpy(&prefix, &g_MenuCardPathPrefix1, 6);
                card = 1;
            }
            if (g_Menu->menuUnk2->cardErrorCodes[card] == -2) {
                if (MenuWaitForCardStatusChange(card) == 0) {
                    g_Menu->pCursors->shouldUpdatePosition[0] = 1;
                    break;
                }
                MenuCreateWindow2Strings(0x26);
                if (format((char*)&prefix) != 0) {
                    MenuFreeWindow2Strings();
                    MenuLoadWindow2Text(0x5C, MENU_WINDOW_TEXT_NONE, 0);
                } else {
                    ok = 0;
                    MenuFreeWindow2Strings();
                    repeat = 0;
                }
            }
            if (ok) {
                recordIndex = g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot];
                if (g_Menu->menuUnk2->slotRecordIndexes[recordIndex] != MENU_SLOT_NONE) {
                    if (g_Menu->menuUnk2->slotHasGameSave[recordIndex] != 0) {
                        if (MenuLoadWindow2Text(0x38, MENU_WINDOW_TEXT_NONE, 1)) {
                            recordIndex = g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]];
                            strcpy(path, (char*)&prefix);
                            strcat(path, (char*)g_Menu->menuUnk2->records[recordIndex].ids);
                            erase(path);
                            chunkIndex = g_Menu->menuUnk2->chunks[recordIndex].index;
                        } else {
                            ok = 0;
                            g_Menu->menuUnk2->previousSelectedSlot = MENU_SLOT_NONE;
                        }
                    } else {
                        MenuLoadWindow2Text(0xC4, MENU_WINDOW_TEXT_NONE, 0);
                        g_Menu->pCursors->shouldUpdatePosition[0] = 1;
                        break;
                    }
                } else if (MenuLoadWindow2Text(0x5F, MENU_WINDOW_TEXT_NONE, 1) == 0) {
                    ok = 0;
                    g_Menu->menuUnk2->previousSelectedSlot = MENU_SLOT_NONE;
                }
            }
            if (ok == 0) {
                goto updateCursor;
            }
            g_Menu->pManager->shouldRenderCursors = 0;
            g_Menu->pManager->characterWindowBuilt = 0;
            g_Menu->menuUnk2->previousSelectedSlot = MENU_SLOT_NONE;
            g_Menu->menuUnk2->previousCardSlotCounts[0] = MENU_SLOT_NONE;
            g_Menu->menuUnk2->previousCardSlotCounts[1] = MENU_SLOT_NONE;
            chunkIndex = MenuFindFreeChunkIndex(card, chunkIndex);
            chunkDigit[0] = chunkIndex + 0x30;
            chunkDigit[1] = 0;
            strcpy(newPath, (char*)&prefix);
            strcpy(path, (char*)&prefix);
            retry = 3;
            strcat(newPath, (char*)g_Menu->menuUnk2->gameSerialCode);
            strcat(newPath, chunkDigit);
            strcat(path, (char*)D_801C50B8);
            MenuCreateWindow2Strings(0x32);
            g_Menu->soundEffectsEnabled = 0;
            erase(path);
            while (1) {
                fd = open(path, (g_Menu->menuUnk2->saveBlockCount << 16) | 0x200);
                if (fd == -1) {
                    fd = 0;
                    MenuWaitCardOperationFrames(card);
                } else {
                    fileFd = fd;
                }
                if (fd != 0) {
                    break;
                }
                retry--;
                if (retry == 0) {
                    break;
                }
            }
            close(fileFd);
            if (fd == 0) {
                goto freeWindow;
            }
            retry = 3;
            while (1) {
                fd = open(path, 2);
                if (fd == -1) {
                    fd = 0;
                    MenuWaitCardOperationFrames(card);
                }
                if (fd != 0) {
                    break;
                }
                retry--;
                if (retry == 0) {
                    erase(path);
                    break;
                }
            }
            MenuRenderFrame();
            if (fd == 0) {
                goto freeWindow;
            }
            func_801CA8C0(chunkIndex);
            retry = 3;
            while (1) {
                if (write(fd, g_Menu->menuUnk2->saveFileMagic, 0x100) == -1) {
                    fd = 0;
                    MenuWaitCardOperationFrames(card);
                }
                if (fd != 0) {
                    break;
                }
                retry--;
                if (retry == 0) {
                    close(0);
                    erase(path);
                    goto freeWindow;
                }
            }
            pBuffer = HeapAlloc(0x1F00, 1);
            bzero(pBuffer, 0x1F00);
            pData = pBuffer;
            func_801CBA4C((MenuSaveSlotHeader*)pBuffer, card, chunkIndex);
            pSum = pBuffer;
            sum = 0;
            count = 0x1EFE;
            do {
                sum += *pSum++;
            } while (--count >= 0);
            *pSum = sum;
            MenuSetCellGridPanelState(1);
            offset = 0x100;
            do {
                MenuRenderFrame();
                retry = 3;
                while (1) {
                    if (write(fd, pData, 0x100) != 0x100) {
                        fd = 0;
                        MenuWaitCardOperationFrames(card);
                    }
                    if (fd != 0) {
                        break;
                    }
                    retry--;
                    if (retry == 0) {
                        close(fileFd);
                        strcpy(path, (char*)&prefix);
                        strcat(path, (char*)D_801C50B8);
                        erase(path);
                        goto freeBuffer;
                    }
                }
                offset += 0x100;
                pData += 0x100;
            } while (offset < (g_Menu->menuUnk2->saveBlockCount << 13));
            close(fd);
            strcpy(path, (char*)&prefix);
            strcat(path, (char*)D_801C50B8);
            retry = 3;
            while (1) {
                if (rename(path, newPath) != 0) {
                    fd = 0;
                    MenuWaitCardOperationFrames(card);
                }
                if (fd != 0) {
                    break;
                }
                retry--;
                if (retry == 0) {
                    erase(path);
                    break;
                }
            }
        freeBuffer:
            HeapFree(pBuffer);
        freeWindow:
            MenuFreeWindow2Strings();
            MenuSetCellGridPanelState(2);
            if (fd == 0) {
                textId = 0x35;
            } else {
                g_Menu->soundEffectsEnabled = 1;
                MenuPlaySoundEffect(0x34);
                g_Menu->soundEffectsEnabled = 0;
                textId = 0x5C;
            }
            MenuLoadWindow2Text(textId, MENU_WINDOW_TEXT_NONE, 0);
            MenuSetCellGridPanelState(0);
            g_Menu->menuUnk2->cardPollMode = 2;
            while (g_Menu->cardCheckTimer != 1) {
                MenuRenderFrame();
            }
            g_Menu->menuUnk2->cardEnumerated[0] = 0;
            g_Menu->menuUnk2->cardEnumerated[1] = 0;
        updateCursor:
            g_Menu->pCursors->shouldUpdatePosition[0] = 1;
            g_Menu->soundEffectsEnabled = 1;
            if (arg0 == 0) {
                repeat = 0;
            }
            break;
        case 2:
            repeat = 0;
            break;
        }
    }
    g_Menu->menuUnk2->cardPollMode = 1;
    g_Menu->unk334 = 1;
    return result;
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CBD90);





/* Best attempt: 81.86% (2860 bytes vs retail's 2960, 715 insns vs retail's 757).
   The save-COPY routine of the card submenu: it moves the record of the selected
   slot (records[slotRecordIndexes[g_MenuCardSlotOrder[selectedSlot]]]) from the
   card the slot lives on to the OTHER card -- read its file a 0x200 block at a
   time and write the blocks to that card's D_801C50B8 temp name, then rename it
   to prefix+ids. Everything below the first MenuHandleCardSlotInput arm therefore
   keys off the DESTINATION card: retail's index is `sltiu` of
   selectedSlot / MENU_CARD_SLOT_COUNT, i.e. otherCard = (ownCard == 0), and that
   same value indexes cardStatus/cardErrorCodes/g_MenuCardSlotTotals/
   cardSlotCounts and is the MenuWaitForCardStatusChange argument, while the
   source-file retries wait on ownCard (retail's sp+0x4E0 slot). Getting this
   backwards costs ~2.4% (78.05 -> 80.48).
   Named residuals:
   (1) 42 MISSING INSTRUCTIONS: retail is 757 insns, this body 715, and the call
       sequence matches except that this body emits THREE copies of the
       close(fileFd)/close(destFd)/erase(tempPath) failure exit where retail has
       two distinct ones (one rebuilds tempPath with strcpy+strcat before erasing,
       the other erases it as is), so most of the gap is elsewhere -- start by
       finding the block retail has that this body lacks, before touching registers.
   (2) MENU_CARD_PREFIX COPY IS A STRUCT ASSIGNMENT in retail (lw + lh, `sh` for
       the 2-byte tail) where `memcpy(&x, &g_MenuCardPathPrefix0, 6)` (the idiom
       MenuLoadCardChunk uses) emits a different shape; this body's
       `ownPrefix = g_MenuCardPathPrefix0;` gives lw + lw + sw + sw (it reads the
       struct as 8 bytes), so neither spelling has been proven here yet.
   (3) MenuIsSlotUsable(0) needs no `andi 0xff` in retail (`bnez v0` straight off
       the jal) but the u_char return type of this file's own definition forces
       one; the prototype is the thing to test, not the call.
   (4) The 0x530 frame and every stack offset still differ (this body's tail
       scalars are 8 apart in retail: 0x4E0 ownCard, 0x4E8 ready, 0x4F0 result,
       0x4F8 chunkMissing).
   Tried: card as the destination index and as the own-card index (2.4% worse),
   the read/write arms of the block loop in both orders (retail's read is the
   fall-through: +1.4%), a `slot` local for MenuFindFirstValidSlot instead of
   re-reading menuUnk2->selectedSlot, and the own/other prefix swap folded out of
   the arm. Not tried: the permuter (structure is still moving), and a search for
   the missing block. */
u_char func_801CC6D8(void) {
    char otherPath[0x40];
    MenuCardPrefix otherPrefix;
    char ownPath[0x40];
    char tempPath[0x40];
    MenuCardPrefix ownPrefix;
    u8 headerBlock[0x200];
    u8 block[0x200];
    s32 ownCard;
    s32 otherCard;
    u_char ready;
    u_char result;
    u_char chunkMissing;
    u_char ok;
    u_char retry;
    u_char textId;
    u_char createTextId;
    u_char dupTextId;
    s32 repeat;
    s32 recordIndex;
    s32 slot;
    s32 fd;
    s32 fileFd;
    s32 destFd;
    s32 offset;
    s32 i;

    chunkMissing = 0;
    ready = 1;
    result = 0;
    MenuResetSlotSelection();
    repeat = 1;
    while (repeat) {
        if (func_801C93A8() & 0xFF) {
            result = 1;
            break;
        }
        if (ready) {
            if (g_Menu->pManager->window2StringsActive) {
                MenuFreeWindow2Strings();
            }
            ready = 0;
            g_Menu->pCursors->shouldRender[0] = 1;
        }
        if (!MenuIsSlotUsable(0)) {
            slot = MenuFindFirstValidSlot(0);
            g_Menu->menuUnk2->selectedSlot = slot;
            if (slot == MENU_SLOT_NONE) {
                g_Menu->pCursors->shouldRender[0] = 0;
                g_Menu->pManager->characterWindowBuilt = 0;
                MenuLoadWindow2Text(0x62, MENU_WINDOW_TEXT_NONE, 0);
                break;
            }
        }
        g_Menu->pManager->shouldRenderCursors = 1;
        switch (MenuHandleCardSlotInput(0)) {
        case 1:
            g_Menu->pCursors->shouldUpdatePosition[0] = 0;
            g_Menu->menuUnk2->cardPollMode = 0;
            ownCard = g_Menu->menuUnk2->selectedSlot / MENU_CARD_SLOT_COUNT;
            otherCard = ownCard == 0;
            ok = 1;
            if (otherCard == 0) {
                textId = 0x47;
                createTextId = 0x4A;
                dupTextId = 0x4D;
            } else {
                textId = 0xBB;
                createTextId = 0xBE;
                dupTextId = 0xC1;
            }
            if (MenuLoadWindow2Text(textId, MENU_WINDOW_TEXT_NONE, 1) == 0) {
                ok = 0;
                g_Menu->menuUnk2->previousSelectedSlot = MENU_SLOT_NONE;
            }
            if (ok) {
                if (g_Menu->menuUnk2->cardStatus[otherCard] == 0) {
                    MenuCreateWindow2Strings(createTextId);
                    if (g_Menu->menuUnk2->cardStatus[otherCard] == 0) {
                        for (i = 0xB3; i != 0; i--) {
                            Vsync(0);
                        }
                        ok = 0;
                        repeat = 0;
                    }
                    MenuFreeWindow2Strings();
                }
                if (ok) {
                    if (g_MenuCardSlotTotals[otherCard] >= MENU_CARD_SLOT_COUNT) {
                        g_MenuCardSlotTotals[otherCard] = 0;
                        ok = 0;
                        repeat = 0;
                        g_Menu->pManager->shouldRenderCursors = 0;
                        MenuLoadWindow2Text(0xAC, MENU_WINDOW_TEXT_NONE, 0);
                    }
                }
            }
            if (ok) {
                if (otherCard == 0) {
                    ownPrefix = g_MenuCardPathPrefix0;
                    otherPrefix = g_MenuCardPathPrefix1;
                } else {
                    ownPrefix = g_MenuCardPathPrefix1;
                    otherPrefix = g_MenuCardPathPrefix0;
                }
                if (g_Menu->menuUnk2->cardErrorCodes[otherCard] == -2) {
                    if (MenuWaitForCardStatusChange(otherCard) == 0) {
                        g_Menu->pCursors->shouldUpdatePosition[0] = 1;
                        break;
                    }
                    MenuCreateWindow2Strings(0x26);
                    if (format((char*)&otherPrefix) != 0) {
                        MenuFreeWindow2Strings();
                        MenuLoadWindow2Text(0x5C, MENU_WINDOW_TEXT_NONE, 0);
                    } else {
                        MenuFreeWindow2Strings();
                        ok = 0;
                    }
                }
                if (ok) {
                    i = 0;
                    if (g_Menu->menuUnk2->cardSlotCounts[otherCard] > 0) {
                        do {
                            if (strcmp(g_Menu->menuUnk2->records[g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]]].ids,
                                    g_Menu->menuUnk2->records[otherCard * MENU_CARD_RECORD_COUNT + i].ids) == 0) {
                                i = MENU_SLOT_NONE;
                                break;
                            }
                            i++;
                        } while (i < g_Menu->menuUnk2->cardSlotCounts[otherCard]);
                    }
                    if (i == MENU_SLOT_NONE) {
                        MenuLoadWindow2Text(dupTextId, MENU_WINDOW_TEXT_NONE, 0);
                        ok = 0;
                    }
                }
                if (ok) {
                    MenuCreateWindow2Strings(0x41);
                    g_Menu->soundEffectsEnabled = 0;
                    g_Menu->pManager->shouldRenderCursors = 0;
                    g_Menu->pManager->characterWindowBuilt = 0;
                    g_Menu->menuUnk2->previousSelectedSlot = MENU_SLOT_NONE;
                    g_Menu->menuUnk2->previousCardSlotCounts[0] = MENU_SLOT_NONE;
                    g_Menu->menuUnk2->previousCardSlotCounts[1] = MENU_SLOT_NONE;
                    strcpy(ownPath, (char*)&ownPrefix);
                    strcpy(otherPath, (char*)&otherPrefix);
                    strcpy(tempPath, (char*)&otherPrefix);
                    recordIndex = g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot];
                    recordIndex = g_Menu->menuUnk2->slotRecordIndexes[recordIndex];
                    strcat(ownPath, g_Menu->menuUnk2->records[recordIndex].ids);
                    retry = 3;
                    strcat(otherPath, g_Menu->menuUnk2->records[recordIndex].ids);
                    strcat(tempPath, D_801C50B8);
                    while (1) {
                        fd = open(ownPath, 1);
                        if (fd == -1) {
                            fd = 0;
                            MenuWaitCardOperationFrames(ownCard);
                        }
                        if (fd != 0) {
                            break;
                        }
                        retry--;
                        if (retry == 0) {
                            break;
                        }
                    }
                    if (fd != 0) {
                        fileFd = fd;
                        retry = 3;
                        while (1) {
                            if (read(fileFd, headerBlock, 0x200) == -1) {
                                fd = 0;
                                MenuWaitCardOperationFrames(ownCard);
                            }
                            if (fd != 0) {
                                break;
                            }
                            retry--;
                            if (retry == 0) {
                                close(fileFd);
                                goto copyDone;
                            }
                        }
                        erase(tempPath);
                        retry = 1;
                        while (1) {
                            destFd = open(tempPath, (headerBlock[3] << 16) | 0x200);
                            if (destFd == -1) {
                                fd = 0;
                                chunkMissing = 1;
                            }
                            if (fd != 0) {
                                break;
                            }
                            retry--;
                            if (retry == 0) {
                                close(fileFd);
                                break;
                            }
                        }
                        close(destFd);
                        if (fd != 0) {
                            retry = 3;
                            while (1) {
                                destFd = open(tempPath, 2);
                                if (destFd == -1) {
                                    fd = 0;
                                    MenuWaitCardOperationFrames(otherCard);
                                }
                                if (fd != 0) {
                                    break;
                                }
                                retry--;
                                if (retry == 0) {
                                    close(fileFd);
                                    erase(tempPath);
                                    goto copyDone;
                                }
                            }
                            retry = 3;
                            offset = 0x200;
                            while (1) {
                                if (write(destFd, headerBlock, 0x200) == -1) {
                                    fd = 0;
                                    MenuWaitCardOperationFrames(otherCard);
                                }
                                if (fd != 0) {
                                    break;
                                }
                                retry--;
                                if (retry == 0) {
                                    close(fileFd);
                                    close(destFd);
                                    erase(tempPath);
                                    goto copyDone;
                                }
                            }
                            MenuSetCellGridPanelState(1);
                            i = 0;
                            while (1) {
                                MenuRenderFrame();
                                if (!i) {
                                    retry = 3;
                                    while (1) {
                                        if (read(fileFd, block, 0x200) != 0x200) {
                                            fd = 0;
                                            MenuWaitCardOperationFrames(ownCard);
                                        }
                                        if (fd != 0) {
                                            break;
                                        }
                                        retry--;
                                        if (retry == 0) {
                                            close(fileFd);
                                            close(destFd);
                                            strcpy(tempPath, (char*)&otherPrefix);
                                            strcat(tempPath, D_801C50B8);
                                            erase(tempPath);
                                            goto copyDone;
                                        }
                                    }
                                    i = 1;
                                } else {
                                    retry = 3;
                                    while (1) {
                                        if (write(destFd, block, 0x200) != 0x200) {
                                            fd = 0;
                                            MenuWaitCardOperationFrames(otherCard);
                                        }
                                        if (fd != 0) {
                                            break;
                                        }
                                        retry--;
                                        if (retry == 0) {
                                            close(fileFd);
                                            close(destFd);
                                            strcpy(tempPath, (char*)&otherPrefix);
                                            strcat(tempPath, D_801C50B8);
                                            erase(tempPath);
                                            goto copyDone;
                                        }
                                    }
                                    offset += 0x200;
                                    if (offset >= (headerBlock[3] << 13)) {
                                        break;
                                    }
                                    i = 0;
                                }
                            }
                            close(destFd);
                            close(fileFd);
                            strcpy(tempPath, (char*)&otherPrefix);
                            strcat(tempPath, D_801C50B8);
                            rename(tempPath, otherPath);
                        }
                    }
                }
            }
        copyDone:
            if (ok) {
                MenuSetCellGridPanelState(2);
                MenuFreeWindow2Strings();
                if (fd != 0) {
                    g_Menu->soundEffectsEnabled = 1;
                    MenuPlaySoundEffect(0x34);
                    g_Menu->soundEffectsEnabled = 0;
                    textId = 0x5C;
                } else if (chunkMissing) {
                    textId = 0xAC;
                } else {
                    textId = 0x44;
                }
                MenuLoadWindow2Text(textId, MENU_WINDOW_TEXT_NONE, 0);
                MenuSetCellGridPanelState(0);
                g_Menu->menuUnk2->cardPollMode = 2;
                while (g_Menu->cardCheckTimer != 1) {
                    MenuRenderFrame();
                }
                for (i = 0; i < MENU_CARD_RECORD_COUNT * MENU_CARD_COUNT; i++) {
                    g_Menu->menuUnk2->slotRecordIndexes[i] = MENU_SLOT_NONE;
                    g_Menu->menuUnk2->slotHasGameSave[i] = 0;
                    g_Menu->menuUnk2->records[i].unk58 = 0;
                }
                g_Menu->menuUnk2->cardEnumerated[0] = 0;
                g_Menu->menuUnk2->cardEnumerated[1] = 0;
                g_MenuCardSaveListStale = 1;
            }
        updateCursor:
            g_Menu->pCursors->shouldUpdatePosition[0] = 1;
            g_Menu->soundEffectsEnabled = 1;
        case 2:
            repeat = 0;
            break;
        }
    }
    g_Menu->menuUnk2->cardPollMode = 1;
    g_Menu->unk334 = 1;
    return result;
}

