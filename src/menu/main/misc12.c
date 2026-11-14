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

extern s32 D_801EA1EC[];
extern s32 D_801EA1F0[];
extern u8 D_801EA578[];
extern u8 D_801EA5C4[];
extern s32 SystemRenderStringEntry(u8* pSrc, void* pDst, s32 len, s32 mode);
extern u8 D_801EA590[];
extern u8 D_801EA5DC[];
extern void SystemDecodeGlyphCodesN(u16* pInput, u8* pOutput, int count);
extern s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, u16 x, u16 y, u16 scale);
extern void MenuMovePointerCursor(s32 arg0, s32 arg1);

extern s32 D_801EA02C;
extern s32 D_801EA030;
extern s32 D_801EA034;
extern s32 D_801EA038;
extern s32 D_801EA03C;
extern s32 D_801EA040;
extern s32 D_801EA044;
extern s32 D_801EA048;

INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6CFC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E6F5C);
/* Best: 68.99% (556 vs 484 bytes). Shape is certain (it is the func_801D51EC
   idiom from misc7: MenuParseNumberToDigits, then a counter byte at
   unk34C + index*0x87C + 0x130A, then two loops over g_Menu->digits[6+i] that
   accumulate func_8002675C return values, the second one with an n counter for
   the x spacing). What is still wrong:
   - gcc 2.6 hoists `(u_char)index * 0x50` (the x spacing term) out of loop 1
     into the preheader; retail recomputes it in the body every iteration
     (`sll v0,s2,2; addu v0,v0,s2; sll v0,v0,4`, first insn in the beq delay
     slot). Tried: naming the sum in an s32 local (still hoisted), i*8 first,
     (u_char)index inline. This is the blocker.
   - retail keeps `(u_char)index` in s2 across both calls (`addu s2,v1,zero` in
     the jal delay slot) and i in s0 initialised in the PROLOGUE (move s0,zero
     right after sw s0,32(sp)); mine recomputes the andi after the call and puts
     `move i,0` in the jal delay slot.
   - retail's two addresses off the base differ in order: counter is
     addu a0,unk34C,offset (base first) but the poly base is
     addiu v0,offset,0xA98 then addu v0,v0,unk34C.
   - x/y need s32 (not u16) in the func_8002675C prototype or gcc narrows the
     load to `lhu` of the global; with u16 at 66.5%, with s32 68.99%.
   func_801E6F5C is the same body with: lbu fields pEntry[index+0x10] and
   pEntry[index+0x13], counter bytes 0x130C/0x130D, tables D_801EA03C/40/44/48,
   poly offsets 0x6E0/0x780, loop bound 2, digits[7+i].
void func_801E6CFC(s32 index, u8* pEntry) {
    s32 i = 0;
    s32 n;
    u32 offset;

    MenuParseNumberToDigits(*(u16*)(pEntry + (u_char)index * 2 + 4));
    offset = (u_char)index * 0x87C;
    ((u8*)g_Menu->unk34C + offset)[0x130A] = 0;
    for (; i < 3; i++) {
        if (g_Menu->digits[6 + i] != 0xFF) {
            ((u8*)g_Menu->unk34C + offset)[0x130A] += func_8002675C(g_Menu->resources,
                g_Menu->digits[6 + i],
                (POLY_FT4*)((u8*)g_Menu->unk34C + offset + 0xA98
                    + (((u8*)g_Menu->unk34C + offset)[0x130A] * 0x50 + 0x500)),
                g_Menu->renderContext, D_801EA02C + (u_char)index * 0x50 + i * 8, D_801EA030, 0x1000);
        }
    }
    i = 0;
    MenuParseNumberToDigits(*(u16*)(pEntry + (u_char)index * 2 + 0xA));
    ((u8*)g_Menu->unk34C + offset)[0x130B] = 0;
    n = 0;
    for (; i < 3; i++) {
        if (g_Menu->digits[6 + i] != 0xFF) {
            ((u8*)g_Menu->unk34C + offset)[0x130B] += func_8002675C(g_Menu->resources,
                g_Menu->digits[6 + i],
                (POLY_FT4*)((u8*)g_Menu->unk34C + offset + 0xA98
                    + (((u8*)g_Menu->unk34C + offset)[0x130B] * 0x50 + 0x5F0)),
                g_Menu->renderContext, D_801EA034 + (u_char)index * 0x50 + n * 8, D_801EA038, 0x1000);
            n++;
        }
    }
}
*/
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
    SystemDecodeGlyphCodesN((u16*)pairs, text, i / 2);
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

// Was matched (objdiff 100.00%) under 2.7.2 and links 12 bytes wrong under the unit's
// gcc 2.6.0 preset, which this unit moved to in session 7. Re-match under 2.6.0:

/*
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
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E7E68);
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
void MenuRenderFrame(void);

/* Best: 99.79% (688 bytes, exact instruction multiset, only the two table
   bases swap). The missing structural piece: loop 2's guard tests the CURSOR
   word of the same entry, not the text word -- retail compares
   D_801EA1EC[idx] (computed off s4) but passes D_801EA1EC[idx+1] to
   func_8002675C. That is why retail keeps the byte offset in a register and
   materialises the second base once in the entry block as `addiu s5,s4,4`.
   What is left is only how the two bases are formed: retail has
   lui/addiu of D_801EA1EC into s4 then `addiu s5,s4,4`; this body has
   lui/addiu of D_801EA1EC+4 into s5 then `addiu s4,s5,-4`. Two pointer locals
   (`pCursor = D_801EA1EC; pText = pCursor + 1;` or the +1 form) get the right
   pair but gcc then hoists both inits to the top of the entry block, ahead of
   the s2/s1/s3 saves (88.09%); initialising pText inside the outer loop gives
   90.16%. Swapping the pText init between the two unk354 stores does not
   change the order either.
void func_801E86C8(u_char selected) {
       s32 i;
       s32 n;
       s32 allValid;
       u32 off;
       s32* pText;
       allValid = 1;
       g_Menu->unk354->unk1400 = 0;
       g_Menu->unk354->unk1404 = 0;
       g_Menu->pManager->unkA = 1;
       pText = D_801EA1EC + 1;
       for (n = 1; n < 5; n++) {
           g_Menu->unk354->unk1400 = 0;
           g_Menu->subMenuNumChoices = 0;
           for (i = 0; i < n; i++) {
               if (D_801EA1EC[(selected + g_Menu->mainMenuChoice) * 8 + i * 2] != 0xFFFF) {
                   g_Menu->unk354->unk1400 = func_8002675C(g_Menu->resources,
                       D_801EA1EC[(selected + g_Menu->mainMenuChoice) * 8 + i * 2],
                       (POLY_FT4*)(g_Menu->unk354->unk1400 * 0x50 + (s32)g_Menu->unk354),
                       g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->unk354->unk1400;
                   g_Menu->subMenuNumChoices++;
               } else {
                   allValid = 0;
               }
           }
           g_Menu->unk354->unk1408 = *(u8*)&g_Menu->renderContext;
           if (allValid) {
               for (i = 0; i < 2; i++) {
                   MenuRenderFrame();
               }
           }
           g_Menu->unk354->unk1404 = 0;
           for (i = 0; i < n; i++) {
               if (D_801EA1EC[(selected + g_Menu->mainMenuChoice) * 8 + i * 2] != 0xFFFF) {
                   off = g_Menu->unk354->unk1404 * 0x50 + 0x500;
                   g_Menu->unk354->unk1404 = func_8002675C(g_Menu->resources,
                       pText[(selected + g_Menu->mainMenuChoice) * 8 + i * 2],
                       (POLY_FT4*)(off + (s32)g_Menu->unk354),
                       g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->unk354->unk1404;
               }
           }
           g_Menu->unk354->unk1409 = *(u8*)&g_Menu->renderContext;
           if (allValid) {
               for (i = 0; i < 2; i++) {
                   MenuRenderFrame();
               }
           }
       }
   }
*/
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
    MenuMovePointerCursor(selected, 1);
    g_Menu->pManager->unk4 = 1;
}
void func_801E8B4C(u_char selected) {
    s32 i;
    u32 index;
    u32 off;

    g_Menu->unk354->unk1400 = 0;
    g_Menu->unk354->unk1404 = 0;
    for (i = 0; i < g_Menu->subMenuNumChoices; i++) {
        if (i == g_Menu->subMenuChoice) {
            index = D_801EA1EC[(selected + g_Menu->mainMenuChoice) * 8 + i * 2] + 0xD;
        } else {
            index = D_801EA1EC[(selected + g_Menu->mainMenuChoice) * 8 + i * 2];
        }
        g_Menu->unk354->unk1400 = func_8002675C(g_Menu->resources, index,
            (POLY_FT4*)(g_Menu->unk354->unk1400 * 0x50 + (s32)g_Menu->unk354),
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->unk354->unk1400;
        off = g_Menu->unk354->unk1404 * 0x50 + 0x500;
        g_Menu->unk354->unk1404 = func_8002675C(g_Menu->resources,
            D_801EA1F0[(selected + g_Menu->mainMenuChoice) * 8 + i * 2],
            (POLY_FT4*)(off + (s32)g_Menu->unk354),
            g_Menu->renderContext, 0xA0, 0x96, 0x1000) + g_Menu->unk354->unk1404;
    }
    g_Menu->unk354->unk1408 = *(u8*)&g_Menu->renderContext;
    g_Menu->unk354->unk1409 = *(u8*)&g_Menu->renderContext;
    MenuMovePointerCursor(g_Menu->subMenuChoice + 7, 1);
    g_Menu->pManager->unk4 = 1;
}
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
/* Best: 85.90% and the instruction stream is IDENTICAL modulo two allocator
   swaps: retail has arg0 in s2 / (arg0&0xFF)*4 in s3 / pWindow in a2, mine the
   mirror. Same 536 bytes, same insns. The address grouping that retail uses is
   "addiu a0,a0,off" on the multiply result then "addu a0,a0,a2": that needs the
   offset in a named local (offset + (u32)pWindow), the inline form folds to
   (pWindow + off) + x. `s32 mode` (not u8) is required for the per-call
   `andi a1,s1,0xFF`.
/* Best: 95.69% under gcc-2.6.0. Seventh (offset 0x0) loop was missing from
   the previous attempt; the instruction multiset is now identical to retail's
   and only the register allocation of the two operands of the final address add
   differs. Retail: window pointer lands in a2 and the (i*2 + renderContext)*0x28
   chain in a0, so the call delay slot is 'addu a0,a0,a2'; gcc gives the window
   a0 (3 refs beats the chain temps' 2 in local-alloc's qty order) and the chain
   v0, so mine is 'addu a0,v0,a0'. Also mine drops the final call's
   'andi a1,s1,0xFF' (gcc proves mode is 2 or 3 there) where retail keeps it.
   Tried: offset local + offset+pWindow, inline (chain + K + pWindow), and
   pWindow + chain operand order; all 95.69. Pure allocator tie -> parked.
void func_801E8F60(u8 arg0, u8 arg1) {
    MenuWindow* pWindow;
    s32 mode;
    s32 i;

    mode = arg1 ? 3 : 2;
    for (i = 0; i < 4; i++) {
        pWindow = g_Menu->windows[arg0];
        func_801E8EAC((POLY_FT4*)((u32)pWindow + (i * 2 + pWindow->renderContext) * 0x28), mode);
    }
    for (i = 0; i < 4; i++) {
        pWindow = g_Menu->windows[arg0];
        func_801E8EAC((POLY_FT4*)((u32)pWindow + (i * 2 + pWindow->renderContext) * 0x28 + 0x140), mode);
    }
    for (i = 0; i < 4; i++) {
        pWindow = g_Menu->windows[arg0];
        func_801E8EAC((POLY_FT4*)((i * 2 + pWindow->renderContext) * 0x28 + 0x1E0 + (u32)pWindow), mode);
    }
    for (i = 0; i < 4; i++) {
        pWindow = g_Menu->windows[arg0];
        func_801E8EAC((POLY_FT4*)((i * 2 + pWindow->renderContext) * 0x28 + 0x280 + (u32)pWindow), mode);
    }
    for (i = 0; i < 4; i++) {
        pWindow = g_Menu->windows[arg0];
        func_801E8EAC((POLY_FT4*)((i * 2 + pWindow->renderContext) * 0x28 + 0x320 + (u32)pWindow), mode);
    }
    for (i = 0; i < 2; i++) {
        pWindow = g_Menu->windows[arg0];
        func_801E8EAC((POLY_FT4*)((i * 2 + pWindow->renderContext) * 0x28 + 0x410 + (u32)pWindow), mode);
    }
    pWindow = g_Menu->windows[arg0];
    func_801E8EAC((POLY_FT4*)(pWindow->renderContext * 0x28 + 0x3C0 + (u32)pWindow), mode);
}
*/
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
/* Best: 96.65% under gcc-2.6.0 (764 bytes vs retail's 772). Recovered shape:
   a 16-byte sector header struct copied from the zero datum D_801C5294 (that
   copy reproduces exactly), ArchiveCdDataSync(0), then either the PC-file path
   when ArchiveGetDebugTable() != 0 (three func_801E9340 reads, cdrom*.mdg/fid/
   fnd chosen by `disc == 1`, merged tail by -fcross-jumping) or the CD path:
   CdIntToPos(0,&pos) then three CdControlB(1,0,&D_801EA8F4) status loops, the
   0x13/2/0x15 sector-17 header read loop, then ArchiveCdSetMode(0xA0) and the
   0x18/0x28 archive table reads; returns 2 if the header tag word is not
   "_XEN" (0x4E45585F), 3 if header byte 3 is not `disc + 0x30`, else 0.
   What is left: retail keeps &D_801EA8F4 in a saved register (s0) across the
   first three status loops -- a `u_char* pStat = &D_801EA8F4;` local is what
   took it 91.33 -> 96.65 -- and it re-forms the address with a fresh lui/addiu
   for the 0x13 loop and again for the 0x15 call (mine reuses pStat in the 0x13
   loop), so the frame is 8 bytes short and the register/loop-exit shapes differ
   around the 0x13/0x15 retry: retail's is `if ((stat & 1) && (D_801EA8F5 &
   0x40) && ret == 0) return 2; if (ret != 0) break;` inside the 0x13 loop.
typedef struct {
    u32 unk0;
    u32 tag;
    u32 unk8;
    u32 unkC;
} MenuSectorHeader; // Size: 0x10

extern u8 D_801C5294[];
extern char D_801C52A4[];
extern char D_801C52B8[];
extern char D_801C52CC[];
extern char D_801C52E0[];
extern char D_801C52F4[];
extern char D_801C5308[];
extern u_char D_801EA8F4;
extern u_char D_801EA8F5;
extern s32 g_ArchiveTable;
extern s32 g_ArchiveHeader;
extern s32 g_ArchiveDebugTable;
extern s32 ArchiveReadFileFromCdSector(s32 sector, void* pDestBuffer, s32 fileSize, s32 arg3, u32 flags);

s32 func_801E93A0(s32 disc) {
    MenuSectorHeader header;
    CdlLOC pos;
    u_char* pStat;
    s32 result;
    s32 ret;

    header = *(MenuSectorHeader*)D_801C5294;
    ArchiveCdDataSync(0);
    if (ArchiveGetDebugTable() != 0) {
        result = 0;
        if (disc == 1) {
            func_801E9340(D_801C52A4, (char*)g_ArchiveTable, 0x8000);
            func_801E9340(D_801C52B8, (char*)g_ArchiveHeader, 0x7A);
            func_801E9340(D_801C52CC, (char*)g_ArchiveDebugTable, 0x40000);
        } else {
            func_801E9340(D_801C52E0, (char*)g_ArchiveTable, 0x8000);
            func_801E9340(D_801C52F4, (char*)g_ArchiveHeader, 0x7A);
            func_801E9340(D_801C5308, (char*)g_ArchiveDebugTable, 0x40000);
        }
        return result;
    }
    CdIntToPos(0, &pos);
    pStat = &D_801EA8F4;
    do {
        Vsync(3);
        CdControlB(1, 0, pStat);
    } while ((*pStat & 0x10) == 0);
    do {
        Vsync(3);
        CdControlB(1, 0, pStat);
    } while ((*pStat & 0x10) != 0);
    do {
        Vsync(3);
        ret = CdControlB(1, 0, pStat);
    } while (((*pStat & 2) == 0) || (ret == 0));
    for (;;) {
        Vsync(3);
        if (CdControlB(0x13, 0, &D_801EA8F4) == 0) {
            continue;
        }
        do {
            Vsync(3);
        } while (CdControlB(2, (u_char*)&pos, &D_801EA8F4) == 0);
        ret = CdControlB(0x15, 0, &D_801EA8F4);
        if ((D_801EA8F4 & 1) && (D_801EA8F5 & 0x40) && (ret == 0)) {
            return 2;
        }
        if (ret != 0) {
            break;
        }
    }
    ArchiveCdSetMode(0xA0);
    ArchiveCdDataSync(0);
    Vsync(3);
    Vsync(3);
    ArchiveReadFileFromCdSector(0x17, &header, 0x10, 0, 0);
    ArchiveCdDataSync(0);
    result = 2;
    if (header.tag == 0x4E45585F) {
        result = 3;
        if (((u8*)&header)[3] == disc + 0x30) {
            result = 0;
            ArchiveReadFileFromCdSector(0x18, (void*)g_ArchiveTable, 0x8000, 0, 0);
            ArchiveCdDataSync(0);
            ArchiveReadFileFromCdSector(0x28, (void*)g_ArchiveHeader, 0x7A, 0, 0);
            ArchiveCdDataSync(0);
        }
    }
    return result;
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc12", func_801E93A0);
