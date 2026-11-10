#include "common.h"

#include "system/menu.h"
#include "psyq/libapi.h"

extern u8 D_801EA6D0[][0x10];
extern u8* D_801EA6F4;
extern u16 D_8005A3A0[0x10];
extern u16 D_8006F958[0x10];
extern int D_80059488;
extern int func_801E4D10(u32*, void*);
extern int func_801D2F4C(u_char);
extern u_char func_801CAA38(u_char);
extern int func_801D32B4(void);
extern void func_801CB184(void);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C8D78);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801C8EE8);

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

/* Best attempt: 97.56%. Instruction multiset identical; two allocator/order
   ties remain: `matched = 1` is hoisted above the menuUnk2 load (original
   emits `li t1,1` after the unk4FAE load), and menuUnk2 lands in a0 with the
   record/chunk bases swapping a2/a3. Moving `matched = 1` after the pRecord
   assignment changed nothing; `int matched` -> u8/u_short untried. */
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc3", func_801CB8AC);
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
