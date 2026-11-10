#include "common.h"

#include "system/menu.h"
#include "psyq/libapi.h"

extern u8 D_801EA6D0[][0x10];
extern u8* D_801EA6F4;
extern s32 D_801E9818[];
extern u16 D_8005A3A0[0x10];
extern u16 D_8006F958[0x10];
extern int D_80059488;
extern int func_801E4D10(u32*, void*);
extern int func_801D2F4C(u_char);
extern u_char func_801CAA38(u_char);
extern int func_801D32B4(void);
extern void func_801CB184(void);
extern int func_801C8D78(u_char);
extern void func_801C7BF4(void);
extern u_char func_801CACF8(u_char, u_char, u_char);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C8D78);

void func_801C8EE8(void) {
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

int func_801C9038(u8* pPath, u8* pBuffer) {
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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C90B0);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9270);

/* Best attempt: 97.56%. Instruction multiset identical; `matched = 1` is
   scheduled before the menuUnk2 load (original: after the unk4FAE load), which
   then swaps the record base and pMenu between a2/a3 and flips the `chunks[k]`
   addu operand order. Tried: matched placed before/after the pRecord
   assignment, `u_char matched` (96.15%), `u_short` shape. */
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9BCC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9D34);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C9EF4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA1D4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA480);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA5F0);

/* Best attempt: 83.16%. Whole shape matches; two diffs remain.
   1) Original keeps the loop-carried register as the D_801E9818 index
   (`a2 = a1 - 1` in the beqz delay slot, `v1 = &D_801E9818[arg1]`), mine folds
   the +1 into the base constant and decrements the store value instead.
   2) The `/ 0x14` magic differs: original uses 0x88888889 + add + sra 3, mine
   (same compiler, same dividend) uses 0x66666667 + sra 3; gcc picks the
   "negative magic" form from the dividend's precision, so the original's
   dividend type is probably narrower (s16?) than the s32 used here. */
/*
void func_801CA5F0(s32 arg0, s32 arg1) {
    s32 i;
    s32 j;
    s32 x;

    switch (arg0) {
    case 0:
        for (i = arg1 - 1; i >= 0; i--) {
            j = D_801E9818[i + 1];
            if (g_Menu->menuUnk2->unk4FAE[j] != 0xFF) {
                g_Menu->menuUnk2->unk4F7C = i;
                break;
            }
        }
        break;
    case 1:
        for (i = arg1 - 1; i >= 0; i--) {
            j = D_801E9818[i + 1];
            if (g_Menu->menuUnk2->unk4FAE[j] != 0xFF && g_Menu->menuUnk2->unk4F8E[j] != 0) {
                g_Menu->menuUnk2->unk4F7C = i;
                break;
            }
        }
        break;
    case 2:
        x = arg1 - 1;
        if ((&g_Menu->menuUnk2->unk4FE4)[x / 0x14] != 0 && x >= 0) {
            g_Menu->menuUnk2->unk4F7C = x;
        }
        break;
    }
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA750);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CA8C0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CAA38);
u_char func_801CACF8(u_char a0, u_char a1, u_char a2) {
    u_char result;

    func_801D2F4C(a0);
    g_Menu->pCursors->shouldRender[3] = 1;
    result = func_801CAA38(a2);
    func_801D32B4();
    if (a1 != 0xFF && result != 0) {
        func_801D2F4C(a1);
        g_Menu->pCursors->shouldRender[3] = 1;
        result = func_801CAA38(a2);
        func_801D32B4();
    }
    return result;
}

void func_801CADB0(void) {
    g_Menu->menuUnk2->unk4F80 = 0xFF;
    g_Menu->menuUnk2->unk4F7C = 0;
    if (g_Menu->menuUnk2->unk4FE4 == 0 && g_Menu->menuUnk2->unk4FE5 != 0) {
        g_Menu->menuUnk2->unk4F7C = 0xF;
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CAE08);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB184);

/* Best attempt: 78.88%. Original keeps the entry pointer on an advancing
   register (s0 += 0x14) and rematerialises the odd byte base as
   (g_GameState + 1) + (i + j) with a fresh lui/addiu each row; gcc instead
   hoists &g_GameState into a callee-saved register and merges the two byte
   addresses into one base with 0/1 offsets. The 74.21% variant (pEntry
   advanced by hand, symbol used directly for the odd byte) had the right
   inner-loop shape but an extra s4 for the symbol base. */
/*
void func_801CB184(void) {
    u8 inBuf[0x14];
    u8 outBuf[0x14];
    u8* pChars;
    u8* pEntry;
    u8* pEven;
    u8* pOdd;
    s8* pDst;
    s8* pOut;
    int i;
    int j;

    pChars = (u8*)&g_GameState;
    for (i = 0; i < 0x26C; i += 0x14) {
        pEntry = pChars + i;
        pEven = inBuf;
        pOdd = inBuf + 1;
        for (j = 0; j < 0x14; j += 2) {
            *pEven = pEntry[j];
            *pOdd = pChars[i + j + 1];
            if (pEntry[j] == 0 && pChars[i + j + 1] == 0) {
                break;
            }
            pEven += 2;
            pOdd += 2;
        }
        func_80033B34((u16*)inBuf, outBuf, j / 2);
        pOut = (s8*)outBuf;
        pDst = (s8*)pEntry;
        while (pDst < (s8*)pEntry + 0x14) {
            *pDst++ = *pOut++;
        }
    }
}*/

void func_801CB28C(u32* pSrc) {
    int i;

    func_801E4D10(pSrc, g_Menu->pDressingRoom);
    D_80059488 = *pSrc;
    for (i = 0; i < 0x10; i++) {
        D_8005A3A0[i] = D_8006F958[i];
    }
    func_801CB184();
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB304);
u_char func_801CB8AC(u_char a0) {
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
        func_801C7BF4();
        pMenu = g_Menu->menuUnk2;
        if (prev[0] != pMenu->unk4FE4 || prev[1] != pMenu->unk4FE5) {
            changed = 1;
            break;
        }
    }
    func_801D32B4();
    if (changed == 0) {
        result = func_801CACF8(0x2F, 0xFF, 1);
    }
    return result;
}
u_char func_801CB9E8(u_char a0, u_char a1) {
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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CBA4C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CBD90);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CC6D8);
