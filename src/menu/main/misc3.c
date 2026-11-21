#include "common.h"

#include "system/menu.h"
#include "psyq/libapi.h"

// The arrays menuUnk2 indexes as card * MENU_CARD_RECORD_COUNT + record hold 16 records
// per card, so a flat record index divides by it to get the card it is on.
// MenuLoadWindow2Text's second text id when there is no second text to load.
// MenuFindFreeChunkIndex's index argument when the caller has none and wants a free one.
#define MENU_CHUNK_INDEX_NONE 0xFF

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
extern u_char func_801CAA38(u_char);
extern int MenuFreeWindow2Strings(void);
extern void func_801CB184(void);
extern int func_801C8D78(u_char);
extern void MenuRenderFrame(void);
extern u_char MenuLoadWindow2Text(u_char, u_char, u_char);
extern void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);
extern s32 g_MenuCardSlotTotals[];

// The memory card directory prefix a slot's filename is appended to: g_MenuCardPathPrefix0
// reads "bu00:", g_MenuCardPathPrefix1 "bu10:".
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ s16 unk4;
} MenuCardPrefix; // Size: 0x6

extern MenuCardPrefix g_MenuCardPathPrefix0;
extern MenuCardPrefix g_MenuCardPathPrefix1;

/* Best attempt: 90.05% (380 bytes vs retail's 368). Every construct is right -- the
   DIRENTRY do/while over firstfile/nextfile, the 6-byte prefix copy, the retry
   counter -- but gcc hoists `&dir` into a callee-saved register across the loop
   (an extra s-pair) where retail rematerialises `addiu v1,sp,0x10` at each of the
   three uses, which also moves the parameter from s2 to s1.
   Learned: `dir` must be declared BEFORE `prefix` (gcc 2.7.2 gives the earlier
   declaration the lower stack slot: dir at sp+0x10, prefix at sp+0x38; the other
   order costs a 0x58 frame with an extra s3 save). firstfile/nextfile are the
   PSY-Q libapi calls and the compare is against &dir, not 0 -- `bne v0,v1` with
   v1 = sp+0x10. The store at 0x4F8A is `cardSlotCounts[a0]` (a byte, not the s32
   cursor at slotWriteCursor that MenuLoadCardChunk uses).
   Tried: prefix declared first (frame 0x58, an extra s3), both == operand orders,
   returning u_char vs int (int, matching the file's extern). */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C8D78);
/*
int func_801C8D78(u_char a0) {
    struct DIRENTRY dir;
    MenuCardPrefix prefix;
    u_char count;
    s32 retry;
    s32 i;

    retry = 5;
    g_MenuCardSlotTotals[a0] = 0;
    for (i = 0; i < 0x10; i++) {
        g_Menu->menuUnk2->slotRecordIndexes[a0 * 0x10 + i] = 0xFF;
        g_MenuCardChunkInUse[a0][i] = 0;
    }
    if (a0 == 0) {
        memcpy(&prefix, &g_MenuCardPathPrefix0, 6);
    } else {
        memcpy(&prefix, &g_MenuCardPathPrefix1, 6);
    }
    if (--retry != 0) {
        count = 0;
        if (firstfile((char*)&prefix, &dir) == &dir) {
            do {
                strcpy(g_Menu->menuUnk2->records[a0 * 0x10 + count].ids, dir.name);
                count++;
            } while (nextfile(&dir) == &dir);
        }
        g_Menu->menuUnk2->cardSlotCounts[a0] = count;
    }
    return count;
}
*/


void MenuRunCardSlotOperations(void) {
    switch (g_Menu->menuUnk2->cardPollMode) {
    case 1:
        if (g_Menu->menuUnk2->cardEnumerated[0] == 0) {
            if ((u_char)func_801C8D78(0) != 0) {
                g_Menu->unk334 = 1;
            }
            g_Menu->menuUnk2->cardEnumerated[0] = 1;
        }
        if (g_Menu->menuUnk2->cardEnumerated[1] == 0) {
            if ((u_char)func_801C8D78(1) != 0) {
                g_Menu->unk334 = 1;
            }
            g_Menu->menuUnk2->cardEnumerated[1] = 1;
        }
        break;
    case 2:
        if (g_Menu->menuUnk2->cardEnumerated[0] == 0) {
            func_801C8D78(0);
            g_Menu->menuUnk2->cardEnumerated[0] = 1;
        }
        if (g_Menu->menuUnk2->cardEnumerated[1] == 0) {
            func_801C8D78(1);
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
extern u8 D_801E9778;
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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C93A8);
extern s32 g_MenuCardSlotOrder[];

u_char MenuIsSlotUsable(s32 a0) {
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
/* Best attempt: 84.07% (720 bytes vs retail's 704). Case 0 is EXACT (both phase-1
   and phase-2 loops, the two `slti 0x1E` guards and the D_801E9828/D_801E9820
   tables); the residual is case 1 and case 2. Case 1: retail keeps `t0 = g_Menu` in
   a register but RELOADS `lw a0,812(t0)` (menuUnk2) at the top of every loop
   iteration, where this compile CSEs menuUnk2 into a saved register (a3) - the extra
   saved value is the whole 16-byte overshoot. Case 2: retail puts `lui v0,0x88888889`
   in the case test's delay slot and runs `addiu a1,a1,3` after it; this compile puts
   `addiu a1,a1,3` in the delay slot. Note the mirror func_801CA1D4 (the descending
   version, same tables) is parked at 83.27% with the identical class of diff, so a
   fix here should transfer both ways. */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9EF4);
/*
void func_801C9EF4(s32 a0, s32 a1) {
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
        for (i = a1; i + 3 < 0x1E; i += 3) {
            j = D_801E9828[i];
            if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF && g_Menu->menuUnk2->slotHasGameSave[j] != 0) {
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
                    if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF && g_Menu->menuUnk2->slotHasGameSave[j] != 0) {
                        g_Menu->menuUnk2->selectedSlot = i + 1;
                        break;
                    }
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
}

*/
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA1D4);
/*
void func_801CA1D4(s32 a0, s32 a1) {
    s32 i;
    s32 j;

    switch (a0) {
    case 0:
        for (i = a1; i - 3 >= 0; i -= 3) {
            j = D_801E9810[i];
            if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF) {
                g_Menu->menuUnk2->selectedSlot = i - 3;
                break;
            }
        }
        if (i - 3 >= 0) {
            break;
        }
        i = g_Menu->menuUnk2->selectedSlot - 3;
        if (i >= 0) {
            for (; i - 1 >= 0; i--) {
                j = D_801E9818[i];
                if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF) {
                    g_Menu->menuUnk2->selectedSlot = i - 1;
                    break;
                }
            }
        }
        break;
    case 1:
        for (i = a1; i - 3 >= 0; i -= 3) {
            j = D_801E9810[i];
            if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF && g_Menu->menuUnk2->slotHasGameSave[j] != 0) {
                g_Menu->menuUnk2->selectedSlot = i - 3;
                break;
            }
        }
        if (i - 3 >= 0) {
            break;
        }
        i = g_Menu->menuUnk2->selectedSlot - 3;
        if (i >= 0) {
            for (; i - 1 >= 0; i--) {
                j = D_801E9818[i];
                if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF && g_Menu->menuUnk2->slotHasGameSave[j] != 0) {
                    g_Menu->menuUnk2->selectedSlot = i - 1;
                    break;
                }
            }
        }
        break;
    case 2:
        i = a1 - 3;
        if (((u8*)&g_Menu->menuUnk2->cardStatus[0])[i / 15] != 0) {
            if (i >= 0) {
                g_Menu->menuUnk2->selectedSlot = i;
            } else {
                i = g_Menu->menuUnk2->selectedSlot - 3;
                if (i >= 0) {
                    if (i - 1 >= 0) {
                        g_Menu->menuUnk2->selectedSlot = i - 1;
                    }
                }
            }
        }
        break;
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA480);

/* Best attempt: 83.32%. Cases 0 and 2 are exact; case 1 is a pure register
   permutation (original keeps the store value in a0 and hoists g_Menu into a3,
   mine uses a2 and holds menuUnk2 in a0). All three cases share one register
   assignment, so they have to be found together: making case 2 exact moved
   case 0's index from a1 to a2.
   Learned: case 2's divisor is FIFTEEN, not 0x14 (magic 0x88888889 + add +
   sra 3); writing /0x14 gives 0x66666667 and can never match. The ascending
   loops are `for (i = a1; i + 1 < 0x1E; i++)` over `D_801E9820[i]` storing
   `unk4F7C = i + 1`, which is what puts the guard on a2 = a1 + 1. */
/*
void func_801CA480(s32 a0, s32 a1) {
    s32 i;
    s32 j;

    switch (a0) {
    case 0:
        for (i = a1; i + 1 < 0x1E; i++) {
            j = D_801E9820[i];
            if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF) {
                g_Menu->menuUnk2->selectedSlot = i + 1;
                break;
            }
        }
        break;
    case 1:
        for (i = a1; i + 1 < 0x1E; i++) {
            j = D_801E9820[i];
            if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF && g_Menu->menuUnk2->slotHasGameSave[j] != 0) {
                g_Menu->menuUnk2->selectedSlot = i + 1;
                break;
            }
        }
        break;
    case 2:
        i = a1 + 1;
        if (((u8*)&g_Menu->menuUnk2->cardStatus[0])[i / 15] != 0 && i < 0x1E) {
            g_Menu->menuUnk2->selectedSlot = i;
        }
        break;
    }
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA5F0);

/* Best attempt: 81.70%. Both descending loops and case 2's shape match; the
   only diff left is an allocator tie in case 2, where the original's mfhi lands
   in v0 (the quotient's own register) and mine lands in t1 with an extra
   `addu v0,t1,a1`.
   Learned: the divisor is 15, not 0x14 (see func_801CA480), and keeping the
   flag array as a pointer local turns the final `addu v0,v0,v1` into the
   original's base-first `addu v0,v1,v0`. The descending loops are
   `for (i = a1; i - 1 >= 0; i--)` over `D_801E9818[i]` storing
   `unk4F7C = i - 1`, which is what makes the guard the bltz on a2 = a1 - 1. */
/*
void func_801CA5F0(s32 a0, s32 a1) {
    u8* pFlags;
    s32 i;
    s32 j;

    switch (a0) {
    case 0:
        for (i = a1; i - 1 >= 0; i--) {
            j = D_801E9818[i];
            if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF) {
                g_Menu->menuUnk2->selectedSlot = i - 1;
                break;
            }
        }
        break;
    case 1:
        for (i = a1; i - 1 >= 0; i--) {
            j = D_801E9818[i];
            if (g_Menu->menuUnk2->slotRecordIndexes[j] != 0xFF && g_Menu->menuUnk2->slotHasGameSave[j] != 0) {
                g_Menu->menuUnk2->selectedSlot = i - 1;
                break;
            }
        }
        break;
    case 2:
        i = a1 - 1;
        pFlags = (u8*)&g_Menu->menuUnk2->cardStatus[0];
        j = i / 15;
        if (pFlags[j] != 0 && i >= 0) {
            g_Menu->menuUnk2->selectedSlot = i;
        }
        break;
    }
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA750);
// Best attempt: 99.89% (376 bytes = retail's exact size; TWO differing
// instructions, both the destination register of the tens division: retail
// `mfhi v1; sra v1,v1,2`, here `mfhi a3; sra v1,a3,2` - the ashiftrt's pseudo is
// not coalesced with the HI extract. Nothing else differs.
// What got here from 74.46%: the title template is a memcpy of exactly 0x1B bytes
// whose SOURCE must be declared with 4-byte alignment (`extern u32 D_801C50E0[]`,
// passed as the bare array): as `u8[]` gcc emits an unaligned lwl/lwr/swl/swr block
// move (432 bytes, 74.46%), with alignment it emits retail's six word pairs plus lh
// at 0x18 / lb at 0x1A off one source base. A 0x1C-byte struct copy copies a
// seventh word, and splitting the tail makes gcc rematerialise the source address.
// Tried for the register: `tens = n / 10` before vs after the memcpy (identical),
// swapping the n/tens declarations (identical), a 1390-iteration 3-minute permuter
// run (base=10 best=10, nothing).
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA8C0);
/*
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
*/
/* Best attempt: 97.36% (688 bytes vs retail's 692). Everything matches except the
   dispatch tail of the {4,5} subtree: retail emits `beq v1,s5,case5` with an extra
   `j` on the default path and both arms out of line, while gcc emits case 5 INLINE
   (`bne v1,s5,test`) because that body lands next to the dispatch and its
   `looping = 0; break;` tail cross-jumps into case 4's. Everything else is exact:
   the do/while shape is `while (1) { ...; if (!looping) break; }`, the counter is a
   `u8` (`addiu`+`andi` at the test), and `looping = 1;` must be assigned BEFORE the
   `if (g_MenuCardChangeDetectionEnabled)` block -- as a later statement gcc saves the constant register
   differently and the whole comparison drops to 95.99% (it also swaps frames/looping
   between s2/s3). At 840k: case-body order in the switch is NOT free: (4,5,2,0) and
   (5,4,2,0) both give 688/97.36, (0,2,4,5) 696/91.27, (0,5,2,4) 700/90.91, (2,5,0,4)
   700/92.56; an explicit `default: break;` changes nothing. `li s1,1`/`li s5,5` come
   from cse lifting the repeated constant 1 (two D_801E9778 stores + shouldRender[2]=1)
   and 5 (two `input = 5` stores + the case test), so they need no source local.
   Swapping the case 4 / case 5 labels changes NOTHING (gcc peels the
   last-tested case's body out to the fall-through slot either way), and a FIFTH
   case node (`case 1: break;`) makes gcc emit a JUMP TABLE (sltiu 6 / jr v0,
   696 bytes) - the round-323 node-count lever fails here. */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CAA38);
/*
extern u8 D_801E9778;
extern u8 g_MenuCardChangeDetectionEnabled;
extern u8 g_MenuWindowTextBackPressed;

u_char func_801CAA38(u_char a0) {
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
    frames = 0xB4;
    while (1) {
        if (a0 == 0) {
            g_Menu->pCursors->shouldRender[2] = 0;
            g_Menu->pCursors->shouldRender[3] = 0;
            if (g_Menu->input != 8) {
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
                g_Menu->input = 5;
                D_801E9778 = 1;
            }
            if (cardStatus[1] != g_Menu->menuUnk2->cardStatus[1]) {
                g_Menu->menuUnk2->cardEnumerated[1] = 0;
                g_Menu->input = 5;
                D_801E9778 = 1;
            }
        }
        switch (g_Menu->input) {
        case 5:
            result = 0;
            g_MenuWindowTextBackPressed = 1;
            looping = 0;
            break;
        case 4:
            looping = 0;
            break;
        case 2:
            g_Menu->pCursors->shouldRender[2] = 1;
            g_Menu->pCursors->shouldRender[3] = 0;
            result = 1;
            break;
        case 0:
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
*/

u_char MenuLoadWindow2Text(u_char a0, u_char a1, u_char a2) {
    u_char result;

    MenuCreateWindow2Strings(a0);
    g_Menu->pCursors->shouldRender[3] = 1;
    result = func_801CAA38(a2);
    MenuFreeWindow2Strings();
    if (a1 != MENU_WINDOW_TEXT_NONE && result != 0) {
        MenuCreateWindow2Strings(a1);
        g_Menu->pCursors->shouldRender[3] = 1;
        result = func_801CAA38(a2);
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

/* Best attempt: 86.42% (272 bytes vs retail's 280). `pOdd = (u8*)&g_GameState + 1`
   read as pOdd[offset + j] reproduces retail's two-add shape (offset+j in v0,
   then the gs+1 constant) and its `lui %hi(g_GameState+1)` symbol, but gcc keeps
   the constant in a SAVED register (s2, from the prologue) where retail reloads
   it into the call-clobbered t0 in the inner preheader -- that fifth saved
   value is the whole difference (5 saves vs retail's 4; it also pushes the
   even-dest buffer pointer into s4). The form `((u8*)&g_GameState)[i + j + 1]`
   is the same 86.36%. Tried: pEntry/pAll pointer locals, indexed both reads
   (78-79%), StringEntry* advancing pointer, gcc-2.6.0 (89.15%), cdk (rejects).
   Both loops and the decode/copy-back are otherwise exact. */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB184);
/*
void func_801CB184(void) {
    u8 inBuf[0x14];
    u8 outBuf[0x14];
    StringEntry* pEntry;
    u8* pOdd;
    s32 offset;
    s32 j;

    pEntry = g_GameState.strings;
    pOdd = (u8*)&g_GameState + 1;
    for (offset = 0; offset < 0x26C; offset += 0x14) {
        for (j = 0; j < 0x14; j += 2) {
            inBuf[j] = ((u8*)pEntry)[j];
            inBuf[j + 1] = pOdd[offset + j];
            if (((u8*)pEntry)[j] == 0 && pOdd[offset + j] == 0) {
                break;
            }
        }
        SystemDecodeGlyphCodesN((u16*)inBuf, outBuf, j / 2);
        for (j = 0; j < 0x14; j++) {
            ((u8*)pEntry)[j] = outBuf[j];
        }
        pEntry++;
    }
}

*/

/* Best attempt: 86.36%. The even-byte read wants an advancing pointer
   (s0 = &g_GameState + i, a1 = s0) and the odd-byte read wants the constant
   base %hi(g_GameState + 0x1) rematerialised inside the outer loop, which only
   happens when no register holds &g_GameState. Any source that inits the row
   pointer before the loop lets gcc CSE &g_GameState into a callee-saved
   register, which then folds the +1 into it (85.29%); assigning the pointer
   inside the loop instead makes gcc keep `i` in one register and materialise
   the base per iteration but lose the strength reduction (86.36%, this one).
   Tried: pEntry in the for-increment / in the body / indexed form for both
   reads (78.41%) / int declarations first. */
/*
void func_801CB184(void) {
    u8 inBuf[0x14];
    u8 outBuf[0x14];
    u8* pEntry;
    int i;
    int j;

    for (i = 0; i < 0x26C; i += 0x14) {
        pEntry = (u8*)&g_GameState + i;
        for (j = 0; j < 0x14; j += 2) {
            inBuf[j] = pEntry[j];
            inBuf[j + 1] = ((u8*)&g_GameState)[i + j + 1];
            if (pEntry[j] == 0 && ((u8*)&g_GameState)[i + j + 1] == 0) {
                break;
            }
        }
        SystemDecodeGlyphCodesN((u16*)inBuf, outBuf, j / 2);
        for (j = 0; j < 0x14; j++) {
            pEntry[j] = outBuf[j];
        }
    }
}
*/

void MenuApplySaveSlot(u32* pSrc) {
    int i;

    MenuSaveSlotReadGameState(pSrc, g_Menu->pDressingRoom);
    g_SpuChannelMask = *pSrc;
    for (i = 0; i < 0x10; i++) {
        D_8005A3A0[i] = D_8006F958[i];
    }
    func_801CB184();
}

/* Best attempt: 84.34% (1476 bytes vs retail's 1448, 7 instructions over).
   Every call, global and constant is right; what is left is block layout and one
   register too many:
   - retail hoists the three cold blocks (close(fileFd) at 0x5A4, the
     func_801C72BC/MenuApplySaveSlot arm at 0x5B4) ABOVE the case-1 body, and the
     early exit jumps to a SHARED shouldUpdatePosition[0] = 1 tail (0x814) - this
     body reaches the same shape with `goto updateCursor` + a `goto freeBuffer`
     label but keeps the arm inline, so the byte count is over.
   - retail keeps 0xFF in $fp for the whole function (`bne v0,s8`,
     `sw $fp,0x4F80`, `sb $fp,0x4F8C/8D`); this body rematerialises `li a3,0xFF`
     at each use, i.e. one pseudo short of retail's allocation, and that shows up
     as the s0/s1 rotation (retry in s0 here, s1 in retail; pData in s1 here, s0
     in retail). Tried: s32 vs u_char retry (`--retry` in a do-while gives the
     extra `addiu v0 / move s1,v0` pair, the two-statement form gives `addiu
     s1,s1,-1` + `andi`), `slot` kept vs inlined, `offset = 0` before vs inside
     the `if (fd != 0)`, `result = 0` before vs after the second
     characterWindowBuilt store, duplicated vs `goto` shared tails for both
     shouldUpdatePosition and HeapFree.
   - the checksum loop is retail's `addu v0,a0,v0` + `move a0,v0` in the
     back-branch delay slot; `sum += *pData++;` in a do-while gives `addu
     a0,a0,v0` (no copy), so the sum needs a second variable.
   Not tried: a permuter run (the block layout is structural, which the permuter
   cannot move), and dropping locals to free the register 0xFF wants.
   Verified: the 0x1EFE-byte checksum, the 5-retry loops, the 0x2100-byte read
   buffer and the prefix/records filename build (memcpy 6 bytes + strcat of
   records[slotRecordIndexes[g_MenuCardSlotOrder[selectedSlot]]].ids) all come out as
   retail's instructions. */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB304);
/*extern u_char func_801C93A8(void);
extern s32 func_801CA750(s32 arg0);
extern void func_801C72BC(u8 arg0);
extern void MenuPlaySoundEffect(u8 arg0);

// Reads the save file of the slot MenuResetSlotSelection/MenuFindFirstValidSlot picked
// off the card, checks its 0x1EFE-byte checksum and hands it to MenuApplySaveSlot.
// Returns the "stay in the menu" flag, and repeats the whole selection loop while
// the load was skipped without finding a slot.
u_char func_801CB304(void) {
    char path[0x40];
    s32 ready = 1;
    u_char result = 1;
    s32 repeat = 1;
    s32 slot;
    s32 fd;
    s32 fileFd;
    u_char retry;
    u_char card;
    u8* pBuffer;
    u8* pData;
    s32 offset;
    s32 sum;
    s32 count;

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
        switch (func_801CA750(1)) {
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
            if (MenuLoadWindow2Text(0x65, MENU_WINDOW_TEXT_NONE, 1) == 0) {
                g_Menu->menuUnk2->previousSelectedSlot = MENU_SLOT_NONE;
                goto updateCursor;
            }
            MenuCreateWindow2Strings(0x3B);
            retry = 5;
            g_Menu->soundEffectsEnabled = 0;
            fd = -1;
            while (1) {
                fd = open(path, 1);
                if (fd == -1) {
                    fd = 0;
                    MenuWaitCardOperationFrames(card);
                }
                fileFd = fd;
                if (fd != 0) {
                    break;
                }
                retry--;
                if (retry == 0) {
                    break;
                }
            }
            offset = 0;
            if (fd != 0) {
                g_Menu->pManager->shouldRenderCursors = 0;
                pBuffer = HeapAlloc(0x2100, 1);
                pData = pBuffer;
                g_Menu->pManager->characterWindowBuilt = 0;
                MenuSetCellGridPanelState(1);
                while (offset < (g_Menu->menuUnk2->saveBlockCount << 13)) {
                    MenuRenderFrame();
                    retry = 5;
                    while (1) {
                        if (read(fd, pData, 0x100) != 0x100) {
                            fd = 0;
                            MenuWaitCardOperationFrames(card);
                        }
                        if (fd != 0) {
                            break;
                        }
                        retry--;
                        if (retry == 0) {
                            close(fileFd);
                            goto freeBuffer;
                        }
                    }
                    offset += 0x100;
                    pData += 0x100;
                }
                close(fd);
                pData = pBuffer + 0x100;
                sum = 0;
                count = 0x1EFE;
                do {
                    sum += *pData++;
                } while (--count >= 0);
                if ((u8)sum == *pData) {
                    func_801C72BC(1);
                    MenuApplySaveSlot((u32*)(pBuffer + 0x100));
                    func_801C72BC(0x11);
                } else {
                    fd = 0;
                }
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
}
*/

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

/* Best attempt: 94.12% (828 bytes vs retail's 836 -- two instructions short).
   Structure is settled, and three of its four lessons are reusable:
   - the function is MenuSaveSlotHeader* (a0), one dead parameter (a1) and a byte
     (a2) that it drops into header 0x23; it ends by writing the save's disc
     number to g_GameState+0x19D4 and calling MenuSaveSlotWriteGameState +
     func_801CB184, so the 0x24-byte record at the slot front is the save header.
   - case-1/phase-2 allocator note for the family: nothing here.
   Two differences remain:
   (1) mp and maxMp (game state 0x2BC/0x2BE) are BYTE loads in retail, not lhu.
       GameCharacter declares them u_short, so a cast/view does not reproduce it:
       every cast spelling makes gcc hoist the constant base into a register
       ("addu v0,v0,t1 / lbu 0(v0)") instead of retail's per-site
       "lui at,%hi(g_GameState) / addu at,at,v0 / lbu %lo(g_GameState+0x2BC)(at)".
       The only spelling that folds -- i.e. a member access on the declared
       GameCharacter type -- is unk54[-4] / unk54[-2] (0x54-4 = 0x50), which is
       what the body below uses; that is raw but it is what separates 91.49% from
       94.12%. The honest fix is for GameCharacter to declare mp/maxMp as two
       bytes each, but field/party/stats matches on the u_short form (lhu/sh at
       0x2BC), so the header cannot simply be retyped.
   (2) gcc reuses the lbu of pManager->currentCharacterIDs[i] for the store to
       header 0x1C, where retail reloads it (test and store are the same basic
       block in both, so it is not a block-layout difference). That is one of the
       two missing instructions; the other is the mirror-image register tie below.
   Also learned: the text loop is over g_GameState.strings (31 entries of 0x14 =
   StringEntry), zero-filling a 0x14-byte out buffer (both buffers are byte
   arrays: the zero fill is sb) and calling SystemEncodeGlyphCodes, then copying
   the buffer back. Retail keeps the 3-iteration index in s1 and the gs pointer
   in s0 and puts the "id == 0xFF" arm out of line at the end of the loop body
   (beq to it), which is what "if (id != 0xFF) { big } else { small }" gives.
   Permuter not run for the last 2 instructions (ran out of round time). */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CBA4C);
/*
void func_801CBA4C(MenuSaveSlotHeader* pSlot, s32 unused, u8 index) {
    u8 inBuf[0x14];
    u8 outBuf[0x14];
    u8* pText;
    s32 i;
    s32 j;
    s32 discNumber;

    for (i = 0; i < 0x10; i++) {
        D_8006F958[i] = D_8005A3A0[i];
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
    pText = (u8*)g_GameState.strings;
    pSlot->unk1F = 0;
    pSlot->index = index;
    pSlot->spuChannelMask = g_SpuChannelMask;
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
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CBD90);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CC6D8);
