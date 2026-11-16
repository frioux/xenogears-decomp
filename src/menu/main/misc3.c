#include "common.h"

#include "system/menu.h"
#include "psyq/libapi.h"

extern u8 D_801EA6D0[][0x10];
extern u8* D_801EA6F4;
extern s32 D_801E9810[];
extern s32 D_801E9818[];
extern s32 D_801E9820[];
extern u16 D_8005A3A0[0x10];
extern u16 D_8006F958[0x10];
extern int g_SpuChannelMask;
extern int func_801E4D10(u32*, void*);
extern int func_801D2F4C(u_char);
extern u_char func_801CAA38(u_char);
extern int MenuFreeWindow2Strings(void);
extern void func_801CB184(void);
extern int func_801C8D78(u_char);
extern void MenuRenderFrame(void);
extern u_char MenuLoadWindow2Text(u_char, u_char, u_char);
extern void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);
extern s32 D_801EA900[];

typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ u16 unk4;
} MenuCardPrefix; // Size: 0x6

extern MenuCardPrefix D_801C50A8;
extern MenuCardPrefix D_801C50B0;

/* Best attempt: 90.05% (380 bytes vs retail's 368). Every construct is right -- the
   DIRENTRY do/while over firstfile/nextfile, the 6-byte prefix copy, the retry
   counter -- but gcc hoists `&dir` into a callee-saved register across the loop
   (an extra s-pair) where retail rematerialises `addiu v1,sp,0x10` at each of the
   three uses, which also moves the parameter from s2 to s1.
   Learned: `dir` must be declared BEFORE `prefix` (gcc 2.7.2 gives the earlier
   declaration the lower stack slot: dir at sp+0x10, prefix at sp+0x38; the other
   order costs a 0x58 frame with an extra s3 save). firstfile/nextfile are the
   PSY-Q libapi calls and the compare is against &dir, not 0 -- `bne v0,v1` with
   v1 = sp+0x10. The store at 0x4F8A is `unk4F84[6 + a0]` (a byte, not the s32
   cursor at unk4F84[0] that func_801C90B0 uses).
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
    D_801EA900[a0] = 0;
    for (i = 0; i < 0x10; i++) {
        g_Menu->menuUnk2->unk4FAE[a0 * 0x10 + i] = 0xFF;
        D_801EA6D0[a0][i] = 0;
    }
    if (a0 == 0) {
        memcpy(&prefix, &D_801C50A8, 6);
    } else {
        memcpy(&prefix, &D_801C50B0, 6);
    }
    if (--retry != 0) {
        count = 0;
        if (firstfile((char*)&prefix, &dir) == &dir) {
            do {
                strcpy(g_Menu->menuUnk2->records[a0 * 0x10 + count].ids, dir.name);
                count++;
            } while (nextfile(&dir) == &dir);
        }
        g_Menu->menuUnk2->unk4F84[6 + a0] = count;
    }
    return count;
}
*/


void MenuRunCardSlotOperations(void) {
    switch (g_Menu->menuUnk2->unk4FE6) {
    case 1:
        if (g_Menu->menuUnk2->unk4F84[4] == 0) {
            if ((u_char)func_801C8D78(0) != 0) {
                g_Menu->unk334 = 1;
            }
            g_Menu->menuUnk2->unk4F84[4] = 1;
        }
        if (g_Menu->menuUnk2->unk4F84[5] == 0) {
            if ((u_char)func_801C8D78(1) != 0) {
                g_Menu->unk334 = 1;
            }
            g_Menu->menuUnk2->unk4F84[5] = 1;
        }
        break;
    case 2:
        if (g_Menu->menuUnk2->unk4F84[4] == 0) {
            func_801C8D78(0);
            g_Menu->menuUnk2->unk4F84[4] = 1;
        }
        if (g_Menu->menuUnk2->unk4F84[5] == 0) {
            func_801C8D78(1);
            g_Menu->menuUnk2->unk4F84[5] = 1;
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
    if (read(fd, pBuffer, 0x200) != 0x200) {
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}

extern void MenuWaitCardOperationFrames(u_char index);

/* Best attempt: 99.11%. Instruction multiset identical, byte count equal (448);
   the only difference is the operand order of the two `addu`s that add a hoisted
   offset to `g_Menu->menuUnk2`: retail emits `addu a1,s0,a1` (offset first) where
   this emits `addu a1,a1,s0`. Tried: with/without the (char*) cast on the strcat
   argument, `memcpy(&prefix, &D_801C50A8, 6)` vs two field assignments (the two
   statements get cross-jumped), and the `.pad0` vs `&chunks[idx]` spelling.
   Learned: the 6-byte prefix copy is `memcpy(&prefix, &D_801C50A8, 6)` -- a 6-byte
   aggregate copy is impossible in C (a {u32;s16;} struct is 8 bytes and copies 8),
   and two field assignments are merged by -fcross-jumping. func_801CB304 and
   misc4's func_801CD2AC use the same idiom. Also: unk4F84 is a s32 write cursor
   here (lw/addiu/sw), so it is reached through a cast over the header's u8[8].
   Permuter (4 min, 1289 iterations, base=80 best=80) found nothing for the addu
   order, so the remaining two diffs are a1/v0 register swap (GIV_TIE) in the
   strcat index math and the MenuReadFileChunk argument's addu operand order. */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C90B0);
/*
void func_801C90B0(u_char a0, u_char a1) {
    MenuCardPrefix prefix;
    char path[0x40];
    s32 retry = 1;
    s32 i;

    if (a0 == 0) {
        memcpy(&prefix, &D_801C50A8, 6);
    } else {
        memcpy(&prefix, &D_801C50B0, 6);
    }
    strcpy(path, (char*)&prefix);
    strcat(path, g_Menu->menuUnk2->records[a0 * 0x10 + a1].ids);
    while (1) {
        if (MenuReadFileChunk((u8*)path, g_Menu->menuUnk2->chunks[a0 * 0x10 + a1].pad0) != -1) {
            break;
        }
        if (--retry == 0) {
            break;
        }
        MenuWaitCardOperationFrames(a0);
    }
    for (i = 0; i < g_Menu->menuUnk2->chunks[a0 * 0x10 + a1].pad0[3]; i++) {
        g_Menu->menuUnk2->unk4FAE[a0 * 0x10 + ((s32*)g_Menu->menuUnk2->unk4F84)[0]] = a0 * 0x10 + a1;
        ((s32*)g_Menu->menuUnk2->unk4F84)[0]++;
    }
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9270);

/* Best attempt: 97.56%. Instruction multiset identical; `matched = 1` is
   scheduled before the menuUnk2 load (original: after the unk4FAE load), which
   then swaps the record base and pMenu between a2/a3 and flips the `chunks[k]`
   addu operand order. Tried: matched placed before/after the pRecord
   assignment, `u_char matched` (96.15%), `u_short` shape, and hoisting the
   unk4FAE byte into a `u8 idx` local (86.54% - the original really does reload
   unk4FAE inside the `if`, so the index must stay inline). */
/*
void func_801C9270(int a0) {
    MenuUnk2Record* pRecord;
    MenuUnk2ImageChunk* pChunk;
    int i;
    int j;
    int matched;

    for (i = 0; i < 0x10; i++) {
        g_Menu->menuUnk2->unk4F8E[a0 * 0x10 + i] = 0;
    }
    for (j = 0; j < 0xF; j++) {
        pRecord = &g_Menu->menuUnk2->records[g_Menu->menuUnk2->unk4FAE[a0 * 0x10 + j]];
        matched = 1;
        for (i = 0; i < 0xC; i++) {
            if (pRecord->ids[i] != g_Menu->menuUnk2->unk4FCE[i]) {
                matched = 0;
                break;
            }
        }
        if (matched) {
            g_Menu->menuUnk2->unk4F8E[a0 * 0x10 + j] = 1;
            pChunk = &g_Menu->menuUnk2->chunks[g_Menu->menuUnk2->unk4FAE[a0 * 0x10 + j]];
            D_801EA6F4 = pChunk->data;
            D_801EA6D0[a0][pChunk->index] = 1;
        }
    }
}

*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C93A8);
extern s32 D_801E981C[];

typedef struct {
    /* 0x4FE4 */ u8 pad0[0x4FE4];
    /* 0x4FE4 */ u8 flags[0x4];
} MenuUnk2Flags;

u_char MenuIsSlotUsable(s32 a0) {
    MenuUnk2* pMenu = g_Menu->menuUnk2;
    s32 result = 1;
    s32 j;
    s32 idx = pMenu->unk4F7C;

    switch (a0) {
    case 0:
        if (*(s32*)&pMenu->unk4F84[4] & 0xFFFF0000) {
            j = D_801E981C[idx];
            if (((MenuUnk2Flags*)pMenu)->flags[j / 16] != 0) {
                if (pMenu->unk4FAE[j] != 0xFF) {
                    break;
                }
            }
        }
        result = 0;
        break;
    case 1:
        if (*(s32*)&pMenu->unk4F84[4] & 0xFFFF0000) {
            j = D_801E981C[idx];
            if (((MenuUnk2Flags*)pMenu)->flags[j / 16] != 0) {
                if (pMenu->unk4FAE[j] != 0xFF) {
                    if (pMenu->unk4F8E[j] != 0) {
                        break;
                    }
                }
            }
        }
        result = 0;
        break;
    case 2:
        j = D_801E981C[idx];
        if (((MenuUnk2Flags*)pMenu)->flags[j / 16] == 0) {
            result = 0;
        }
        break;
    }
    if (result != 0) {
        g_Menu->unk4D8 = 2;
    }
    /* keeping `idx` live here is what makes the allocator put the result flag in a2 */
    if (idx) {
        return result;
    }
    return result;
}

s32 MenuFindFirstValidSlot(s32 a0) {
    s32 result = 0xFF;
    s32 searching = 1;
    s32 start;
    s32 limit;
    s32 i;
    s32 j;

    start = (g_Menu->menuUnk2->unk4FE4 == 0) ? 0xF : 0;
    limit = 0x1E;
    if (g_Menu->menuUnk2->unk4FE5 == 0) {
        limit = 0xF;
    }
    for (i = start; i < limit; i++) {
        if (!searching) {
            break;
        }
        switch (a0) {
        case 0:
            if (*(s32*)&g_Menu->menuUnk2->unk4F84[4] & 0xFFFF0000) {
                j = D_801E981C[i];
                if (((MenuUnk2Flags*)g_Menu->menuUnk2)->flags[j / 16] != 0 && g_Menu->menuUnk2->unk4FAE[j] != 0xFF) {
                    result = i;
                    searching = 0;
                }
            } else {
                searching = 0;
            }
            break;
        case 1:
            if (*(s32*)&g_Menu->menuUnk2->unk4F84[4] & 0xFFFF0000) {
                j = D_801E981C[i];
                if (((MenuUnk2Flags*)g_Menu->menuUnk2)->flags[j / 16] != 0 && g_Menu->menuUnk2->unk4FAE[j] != 0xFF &&
                    g_Menu->menuUnk2->unk4F8E[j] != 0) {
                    result = i;
                    searching = 0;
                }
            } else {
                searching = 0;
            }
            break;
        case 2:
            j = D_801E981C[i];
            if (((MenuUnk2Flags*)g_Menu->menuUnk2)->flags[j / 16] != 0) {
                result = i;
                searching = 0;
            }
            break;
        }
    }
    g_Menu->unk4D8 = 2;
    return result;
}
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA1D4);
/*
void func_801CA1D4(s32 a0, s32 a1) {
    s32 i;
    s32 j;

    switch (a0) {
    case 0:
        for (i = a1; i - 3 >= 0; i -= 3) {
            j = D_801E9810[i];
            if (g_Menu->menuUnk2->unk4FAE[j] != 0xFF) {
                g_Menu->menuUnk2->unk4F7C = i - 3;
                break;
            }
        }
        if (i - 3 >= 0) {
            break;
        }
        i = g_Menu->menuUnk2->unk4F7C - 3;
        if (i >= 0) {
            for (; i - 1 >= 0; i--) {
                j = D_801E9818[i];
                if (g_Menu->menuUnk2->unk4FAE[j] != 0xFF) {
                    g_Menu->menuUnk2->unk4F7C = i - 1;
                    break;
                }
            }
        }
        break;
    case 1:
        for (i = a1; i - 3 >= 0; i -= 3) {
            j = D_801E9810[i];
            if (g_Menu->menuUnk2->unk4FAE[j] != 0xFF && g_Menu->menuUnk2->unk4F8E[j] != 0) {
                g_Menu->menuUnk2->unk4F7C = i - 3;
                break;
            }
        }
        if (i - 3 >= 0) {
            break;
        }
        i = g_Menu->menuUnk2->unk4F7C - 3;
        if (i >= 0) {
            for (; i - 1 >= 0; i--) {
                j = D_801E9818[i];
                if (g_Menu->menuUnk2->unk4FAE[j] != 0xFF && g_Menu->menuUnk2->unk4F8E[j] != 0) {
                    g_Menu->menuUnk2->unk4F7C = i - 1;
                    break;
                }
            }
        }
        break;
    case 2:
        i = a1 - 3;
        if (((u8*)&g_Menu->menuUnk2->unk4FE4)[i / 15] != 0) {
            if (i >= 0) {
                g_Menu->menuUnk2->unk4F7C = i;
            } else {
                i = g_Menu->menuUnk2->unk4F7C - 3;
                if (i >= 0) {
                    if (i - 1 >= 0) {
                        g_Menu->menuUnk2->unk4F7C = i - 1;
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
            if (g_Menu->menuUnk2->unk4FAE[j] != 0xFF) {
                g_Menu->menuUnk2->unk4F7C = i + 1;
                break;
            }
        }
        break;
    case 1:
        for (i = a1; i + 1 < 0x1E; i++) {
            j = D_801E9820[i];
            if (g_Menu->menuUnk2->unk4FAE[j] != 0xFF && g_Menu->menuUnk2->unk4F8E[j] != 0) {
                g_Menu->menuUnk2->unk4F7C = i + 1;
                break;
            }
        }
        break;
    case 2:
        i = a1 + 1;
        if (((u8*)&g_Menu->menuUnk2->unk4FE4)[i / 15] != 0 && i < 0x1E) {
            g_Menu->menuUnk2->unk4F7C = i;
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
            if (g_Menu->menuUnk2->unk4FAE[j] != 0xFF) {
                g_Menu->menuUnk2->unk4F7C = i - 1;
                break;
            }
        }
        break;
    case 1:
        for (i = a1; i - 1 >= 0; i--) {
            j = D_801E9818[i];
            if (g_Menu->menuUnk2->unk4FAE[j] != 0xFF && g_Menu->menuUnk2->unk4F8E[j] != 0) {
                g_Menu->menuUnk2->unk4F7C = i - 1;
                break;
            }
        }
        break;
    case 2:
        i = a1 - 1;
        pFlags = (u8*)&g_Menu->menuUnk2->unk4FE4;
        j = i / 15;
        if (pFlags[j] != 0 && i >= 0) {
            g_Menu->menuUnk2->unk4F7C = i;
        }
        break;
    }
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA750);
/* Best attempt: 96.91%. The template copy into 0x4B98 is 0x1B bytes in retail
   (lw x6, then lh at source+0x18 and lb at source+0x1A storing sh/sb = a 3-byte
   block move at alignment 2). A C aggregate of size 3 cannot exist, and a
   0x1C-byte struct (u32[6] + s16 + s8) copies a seventh word; splitting the
   tail into two statements makes gcc rematerialise &D_801C50E0 and emit
   lhu/lbu with a fresh lui instead of reusing the copy's a3 base. Second diff:
   mfhi lands in a3 here vs v1 in retail. Tried: single 0x1C struct copy,
   0x18-byte head copy + two u16/s8 statements (77.87%), pointer locals for
   source and destination.
void func_801CA8C0(u_char a0) {
    s32 n;
    s32 tens;

    n = a0 + 1;
    *(MenuUnk2Text*)g_Menu->menuUnk2->unk4B98 = D_801C50E0;
    tens = n / 10;
    ((u8*)g_Menu->menuUnk2->unk4B98)[0x1A] = 0x82;
    ((u8*)g_Menu->menuUnk2->unk4B98)[0x1B] = tens + 0x4F;
    ((u8*)g_Menu->menuUnk2->unk4B98)[0x1C] = 0x82;
    ((u8*)g_Menu->menuUnk2->unk4B98)[0x1D] = n % 10 + 0x4F;
    ((u8*)g_Menu->menuUnk2->unk4B98)[0x1E] = 0x81;
    ((u8*)g_Menu->menuUnk2->unk4B98)[0x1F] = 0x40;
    ((u8*)g_Menu->menuUnk2->unk4B98)[0x20] = 0;
    ((u8*)g_Menu->menuUnk2->unk4B98)[0x21] = 0;
    strcat(g_Menu->menuUnk2->unk4B98, &g_Menu->menuUnk2->unk4FE7[0x15]);
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA8C0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CAA38);
u_char MenuLoadWindow2Text(u_char a0, u_char a1, u_char a2) {
    u_char result;

    func_801D2F4C(a0);
    g_Menu->pCursors->shouldRender[3] = 1;
    result = func_801CAA38(a2);
    MenuFreeWindow2Strings();
    if (a1 != 0xFF && result != 0) {
        func_801D2F4C(a1);
        g_Menu->pCursors->shouldRender[3] = 1;
        result = func_801CAA38(a2);
        MenuFreeWindow2Strings();
    }
    return result;
}

void MenuResetSlotSelection(void) {
    g_Menu->menuUnk2->unk4F80 = 0xFF;
    g_Menu->menuUnk2->unk4F7C = 0;
    if (g_Menu->menuUnk2->unk4FE4 == 0 && g_Menu->menuUnk2->unk4FE5 != 0) {
        g_Menu->menuUnk2->unk4F7C = 0xF;
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CAE08);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB184);

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

    func_801E4D10(pSrc, g_Menu->pDressingRoom);
    g_SpuChannelMask = *pSrc;
    for (i = 0; i < 0x10; i++) {
        D_8005A3A0[i] = D_8006F958[i];
    }
    func_801CB184();
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB304);
u_char MenuWaitForCardStatusChange(u_char a0) {
    MenuUnk2* pMenu;
    u8 prev[2];
    u_char changed;
    u_char result;

    prev[0] = g_Menu->menuUnk2->unk4FE4;
    prev[1] = g_Menu->menuUnk2->unk4FE5;
    func_801D2F4C(a0 * 3 + 0x29);
    g_Menu->input = 8;
    g_Menu->menuUnk2->unk4FE6 = 2;
    result = 0;
    changed = 0;
    while (g_Menu->input == 8) {
        MenuRenderFrame();
        pMenu = g_Menu->menuUnk2;
        if (prev[0] != pMenu->unk4FE4 || prev[1] != pMenu->unk4FE5) {
            changed = 1;
            break;
        }
    }
    MenuFreeWindow2Strings();
    if (changed == 0) {
        result = MenuLoadWindow2Text(0x2F, 0xFF, 1);
    }
    return result;
}
u_char MenuFindFreeChunkIndex(u_char a0, u_char a1) {
    u_char found;
    int i;

    found = 0;
    if (a1 == 0xFF) {
        for (i = 0; i < 0xF; i++) {
            if (D_801EA6D0[a0][i] == 0) {
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
    /* 0x23 */ u8 unk23;
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
void func_801CBA4C(MenuSaveSlotHeader* pSlot, s32 unused, u8 unk23) {
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
    pSlot->unk23 = unk23;
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
