#include "common.h"

#include "psyq/libcd.h"
#include "psyq/pc.h"
#include "system/menu.h"

extern u_char D_801EA8F4;
extern int Vsync(int mode);
extern void ArchiveCdDataSync(int mode);
extern u_char ArchiveCdSetMode(u_char mode);
extern u32 ArchiveGetDebugTable(void);
extern void func_8002A498(int mode);

void func_801E64E0(void);
void func_801E6668(s32 arg0);
void func_801E76EC(s32 arg0);
void func_801E7C50(MenuString* pStringList, s32 index, s32 yOffset, s32 flags);
extern void* GetStringEntry(void* pStringData, u32 index);
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
void func_801E7E68(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count);
void func_801E61B0(void);
void func_801E68AC(u8* pEntry);
void func_801E6AE8(u8 index, u8* pEntry);
void func_801E6B70(u8 index, u8* pEntry);
void func_801E6CFC(u8 index, u8* pEntry);
void func_801E6F5C(u8 index, u8* pEntry);
void func_801E71B4(s32 index, u8* pEntry, s32 arg2);
void func_801E733C(void);

extern u8 D_801EA578[];
extern u8 D_801EA5C4[];
extern s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 len, s32 mode);
extern u8 D_801EA590[];
extern u8 D_801EA5DC[];
extern void func_80033B34(u16* pInput, u8* pOutput, int count);
extern s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, u16 x, u16 y, u16 scale);
extern void func_801D1EE0(u_char arg0, s32 arg1);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6CFC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6F5C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E71B4);
/* Best: 97.81%. Four shapes were needed: i = 0 before pEntry += (u_char)index,
   pTable = (u8*)(arg2 * 0x200 + (s32)g_Menu->menuUnk2) + 0xC94 (the plain
   additive form mirrors the addu operands), a pRow local recomputed for each of
   the two table reads, and the loop exit spelled !*pKey && !*pValue with
   pValue += 2 before pKey += 2. What is left is one register: retail loads the
   second table byte into v0 (and so leaves a nop in the bnez delay slot), gcc
   picks v1 and reorg then fills the slot with the load.
void func_801E71B4(s32 index, u8* pEntry, s32 arg2) {
    RECT rect;
    u8 pairs[0x14];
    u8 text[0x18];
    u8* pKey;
    u8* pValue;
    u8* pTable;
    u8* pRow;
    u8* pBuf;
    s32 i;

    i = 0;
    pEntry += (u_char)index;
    pTable = (u8*)(arg2 * 0x200 + (s32)g_Menu->menuUnk2) + 0xC94;
    pValue = pairs + 1;
    pKey = pairs;
    for (; i < 0x14; i += 2) {
        pRow = pTable + pEntry[0x1C] * 0x14 + i;
        *pKey = pRow[0x24];
        pRow = pTable + pEntry[0x1C] * 0x14 + i;
        *pValue = pRow[0x25];
        if (!*pKey && !*pValue) {
            break;
        }
        pValue += 2;
        pKey += 2;
    }
    func_80033B34((u16*)pairs, text, i / 2);
    pBuf = HeapAlloc(0x3F6, 0);
    bzero(pBuf, 0x3F6);
    SystemRenderStringEntry(text, pBuf, 0x24, 0);
    rect.x = *(u16*)(D_801EA590 + (u_char)index * 4) + 0x180;
    rect.y = *(u16*)(D_801EA5DC + (u_char)index * 4);
    rect.w = 0x28;
    rect.h = 0xD;
    LoadImage(&rect, (u_long*)pBuf);
    DrawSync(0);
    HeapFree(pBuf);
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E733C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E76EC);
/* Best: 99.87%. Two register pairs, both mirrored: retail puts g_Menu->menuUnk2
   in v1 and the arg0 * 0x200 shift in v0, and i in s3 with offset in s2.
   Spelling pEntry as (u8*)(arg0 * 0x200 + (s32)g_Menu->menuUnk2) + 0xC94 fixes
   the v0/v1 pair but then flips i to s2 (98.99%); every declaration order and
   init style of i/offset keeps i in s2 under that spelling. The index must be
   spelled *(pEntry + i + 0x1C) or the addu operands swap. Declaration
   permutations of i/offset/pEntry, operand order, a named scaled local, a
   named base local and the array-index form were all tried.
   ROUND-NOTE (this worker): the i/offset pair is a pure allocator tie, confirmed
   by a byte-identical instruction stream with s2<->s3 swapped. Declaring
   `u32 offset = 0;` BEFORE `s32 i = 0;` is what makes the prologue's
   sw/move interleave match, and i must stay s32 (u32 turns slti into sltu,
   98.55). With that decl order plus the fixed v0/v1 spelling above, the only
   remaining diff is the tie (99.28); with the original spelling both pairs stay
   mirrored (99.87). The i * 0x87C strength-reduced form scores 98.66.
void func_801E76EC(s32 arg0) {
    s32 i = 0;
    u32 offset = 0;
    u8* pEntry;

    pEntry = (u8*)g_Menu->menuUnk2 + arg0 * 0x200 + 0xC94;
    func_801E61B0();
    for (; i < 3; i++) {
        if (*(pEntry + i + 0x1C) != 0xFF) {
            *(u8*)((u8*)g_Menu->unk34C + offset + 0x1310) = 1;
            func_801E6AE8(i, pEntry);
            func_801E6B70(i, pEntry);
            func_801E6CFC(i, pEntry);
            func_801E6F5C(i, pEntry);
            func_801E71B4(i, pEntry, arg0);
        } else {
            *(u8*)((u8*)g_Menu->unk34C + offset + 0x1310) = 0;
        }
        *(u8*)((u8*)g_Menu->unk34C + offset + 0x130F) = *(u8*)&g_Menu->renderContext;
        offset += 0x87C;
    }
    func_801E68AC(pEntry);
    func_801E733C();
}
*/

void func_801E781C(s32 arg0, u_char arg1) {
    func_801E64E0();
    if (arg0 != 0xFF) {
        if (arg1 != 0) {
            func_801E76EC(arg0);
            func_801E6668(arg0);
            ((u8*)g_Menu->unk34C)[0x2DBC] = 1;
        } else {
            func_801E6668(arg0);
            ((u8*)g_Menu->unk34C)[0x2DBC] = 0;
        }
        g_Menu->pManager->unkB = 1;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E78C8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E7C50);
/* Best: 72.41% under gcc-2.6.0 (62.66% under 2.7.2). Shape recovered from the asm:
   row = index / 2 and column = index & 1 are separate locals (retail keeps row in
   s4 and reuses it for the flags != 0 branch's row * 0xD), u = (row & 1) * 0x80 is
   loop-invariant and rides in s3, the else branch computes column * 0x60 as a
   fresh temp (v1, not the u variable), color = 0 is reset at the top of each
   iteration and the yOffset parameter is spilled to 16(sp) by retail. What is
   still wrong is the allocator: retail puts pStringList in s2, index in s8,
   flags in s7 and spills yOffset, my compile keeps yOffset in s8 and gives
   pStringList s3, plus the branch-1 u1/u2 store order and one extra saved reg.
void func_801E7C50(MenuString* pStringList, s32 index, s32 yOffset, s32 flags) {
    POLY_FT4* pPoly;
    u8 color;
    s32 row;
    s32 column;
    s32 u;
    s32 v;
    s32 i;

    row = index / 2;
    column = index & 1;
    u = (row & 1) * 0x80;
    for (i = 0; i < 2; i++) {
        color = 0;
        pPoly = &pStringList->polys[i];
        func_801E927C(pPoly);
        if ((flags & 0xFF) == 0) {
            pStringList->unk7C = column;
            pPoly->tpage = GetTPage(0, 0, 0x140, 0);
            pPoly->u0 = u;
            v = (index + yOffset) / 4 * 0xD;
            pPoly->v0 = v;
            pPoly->u1 = u + pStringList->width;
            pPoly->v1 = v;
            pPoly->u2 = u;
            pPoly->v2 = v + 0xD;
            pPoly->u3 = u + pStringList->width;
            pPoly->v3 = v + 0xD;
        } else {
            if (flags & 0x80) {
                color = 0x20;
                SetSemiTrans(pPoly, 1);
                setRGB0(pPoly, 0x20, 0x20, 0x20);
            }
            pStringList->unk7C = (flags & 0x7F) + 0xFF;
            pPoly->tpage = color | GetTPage(0, 0, 0x180, 0x80);
            v = row * 0xD + yOffset;
            pPoly->u0 = column * 0x60;
            pPoly->v0 = v;
            pPoly->v1 = v;
            pPoly->u2 = column * 0x60;
            pPoly->v2 = v + 0xD;
            pPoly->u1 = column * 0x60 + pStringList->width;
            pPoly->v3 = v + 0xD;
            pPoly->u3 = column * 0x60 + pStringList->width;
        }
        if (pStringList->unk7C != 0) {
            pPoly->clut = g_SystemPalette2;
        } else {
            pPoly->clut = g_SystemPalette1;
        }
    }
    pStringList->unk7F = 0;
}
*/

void func_801E7E68(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count) {
    RECT* pRect;
    s32 i;

    for (i = 0; i < count; i += 2) {
        pStringList[i].width = SystemRenderStringEntry(GetStringEntry(g_Menu->unk2E0, pStringIdList[i]),
            g_Menu->unk4E0[0].pVramBuffer, 0x18, 0);
        pRect = &pStringList[i].vramDest;
        pStringList[i + 1].width = SystemRenderStringEntry(GetStringEntry(g_Menu->unk2E0, pStringIdList[i + 1]),
            g_Menu->unk4E0[0].pVramBuffer, 0x18, 1);
        pStringList[i].vramDest.x = ((i / 2) & 1) * 0x20 + 0x140;
        pStringList[i].vramDest.y = ((i + yOffset) / 4) * 0xD;
        pStringList[i].vramDest.w = 0x1C;
        pStringList[i].vramDest.h = 0xD;
        pStringList[i + 1].vramDest = pStringList[i].vramDest;
        func_801E7C50(&pStringList[i], i, yOffset, 0);
        func_801E7C50(&pStringList[i + 1], i + 1, yOffset, 0);
        LoadImage(pRect, g_Menu->unk4E0[0].pVramBuffer);
        DrawSync(0);
    }
}
void func_801E8018(u_char count, MenuString* pStringList, u_char* pStringIdList, void* pUnused) {
    func_801E7E68(pStringList, pStringIdList, 4, count);
}

void func_801E8044(u_char count, u_char* pBuffer) {
    s32 end;

    if (count != 0) {
        end = count + (s32)pBuffer;
        do {
            *pBuffer++ = 0;
        } while ((s32)pBuffer < end);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8070);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8474);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E86C8);
void func_801E8978(u_char count, u_char selected, s32* pEntries) {
    s32 i;
    u32 index;
    u32 off;

    g_Menu->pSelectionMenu->numCursors = 0;
    g_Menu->pSelectionMenu->numTexts = 0;
    for (i = 0; i < count; i++) {
        if (i == selected) {
            index = pEntries[i * 2] + 0xD;
        } else {
            index = pEntries[i * 2];
        }
        g_Menu->pSelectionMenu->numCursors = func_8002675C(g_Menu->resources, index,
            (POLY_FT4*)(g_Menu->pSelectionMenu->numCursors * 0x50 + (s32)g_Menu->pSelectionMenu),
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSelectionMenu->numCursors;
        // kept in a local so gcc does not fold 0x8C0 onto the base pointer
        off = g_Menu->pSelectionMenu->numTexts * 0x50 + 0x8C0;
        g_Menu->pSelectionMenu->numTexts = func_8002675C(g_Menu->resources, pEntries[i * 2 + 1],
            (POLY_FT4*)(off + (s32)g_Menu->pSelectionMenu),
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->pSelectionMenu->numTexts;
    }
    g_Menu->pSelectionMenu->cursorsRenderCtx = *(u8*)&g_Menu->renderContext;
    g_Menu->pSelectionMenu->textsRenderCtx = *(u8*)&g_Menu->renderContext;
    func_801D1EE0(selected, 1);
    g_Menu->pManager->unk4 = 1;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8B4C);
void func_801E8DA8(s32 arg0, s32 arg1) {
    RECT rect;
    u8* pBuf;
    s32 offset;

    pBuf = HeapAlloc(0x3F6, 0);
    bzero(pBuf, 0x3F6);
    if ((u_char)arg0 != 0xFF) {
        offset = ((u_char)arg0 >> 1) * 0x28;
        SystemRenderStringEntry((u8*)&g_GameState + offset, pBuf, 0x24, 0);
        SystemRenderStringEntry((u8*)&g_GameState + offset + 0x14, pBuf, 0x24, 1);
    }
    rect.x = *(u16*)(D_801EA578 + ((arg1 * 2) & 0x1FC)) + 0x180;
    rect.y = *(u16*)(D_801EA5C4 + ((arg1 * 2) & 0x1FC));
    rect.w = 0x28;
    rect.h = 0xD;
    LoadImage(&rect, (u_long*)pBuf);
    DrawSync(0);
    HeapFree(pBuf);
}

void func_801E8EAC(POLY_FT4* pPoly, u_char mode) {
    SetShadeTex(pPoly, 0);
    switch (mode) {
    case 1:
        pPoly->tpage |= 0x20;
        SetSemiTrans(pPoly, 1);
        // fallthrough
    case 3:
        setRGB0(pPoly, 33, 33, 33);
        break;
    case 0:
        SetSemiTrans(pPoly, 0);
        // fallthrough
    case 2:
        setRGB0(pPoly, 128, 128, 128);
        break;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E8F60);
void func_801E91C4(P_TAG* pTag) {
    SetSemiTrans(pTag, 1);
    SetShadeTex(pTag, 0);
    setRGB0(pTag, 128, 128, 128);
}
void func_801E920C(POLY_FT4* pPoly, short x, short y, u_char u, u_char v, s32 width, s32 height) {
    pPoly->x0 = x;
    pPoly->y0 = y;
    pPoly->y1 = y;
    pPoly->x2 = x;
    pPoly->u0 = u;
    pPoly->u2 = u;
    pPoly->x1 = x + width;
    pPoly->y2 = y + height;
    pPoly->x3 = x + width;
    pPoly->y3 = y + height;
    pPoly->v0 = v;
    pPoly->u1 = u + width;
    pPoly->v1 = v;
    pPoly->v2 = v + height;
    pPoly->u3 = u + width;
    pPoly->v3 = v + height;
}
void func_801E927C(POLY_FT4* pPoly) {
    SetPolyFT4(pPoly);
    SetSemiTrans(pPoly, 0);
    SetShadeTex(pPoly, 0);
    setRGB0(pPoly, 128, 128, 128);
}
void func_801E92CC(void) {
    if (ArchiveGetDebugTable() == 0) {
        func_8002A498(0);
        ArchiveCdDataSync(0);
        ArchiveCdSetMode(0);
        ArchiveCdDataSync(0);
        Vsync(3);
        for (;;) {
            Vsync(3);
            if (CdControlB(8, 0, &D_801EA8F4) != 0) {
                break;
            }
        }
    }
}
void func_801E9340(char* pName, char* pBuffer, int len) {
    int fd;

    fd = PCopen(pName, 0, 0);
    PCread(fd, pBuffer, len);
    PCclose(fd);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E93A0);
