#include "common.h"

#include "psyq/libapi.h"
#include "system/menu.h"
#include "psyq/libcd.h"

extern u8 g_MenuOverlayIndex; // Index of the menu overlay the menu was entered with
extern u16 D_801EA53C[]; // String IDs of the menu strings
extern u16 D_801EA542[]; // String IDs of the menu strings
extern u16 g_MenuItemRowStringIds[]; // String IDs of the menu strings
extern s32 g_MenuCardSlotTotals;
extern s32 D_801EA904;
extern s32 D_801EA6FC;
extern s32 D_801EA700;
extern s32 D_801EA704;
extern s32 D_801EA708;
extern s32 D_801EA70C;
extern u8 D_801EA710;
extern u8 D_801EA714;
extern CdlCB g_MenuSavedCdSyncCallback;
extern CdlCB g_MenuSavedCdReadyCallback;
extern CdlCB g_MenuSavedCdReadCallback;
extern u8 D_801EA574[];
extern u8 g_MenuNameTextureX[];
extern u8 D_801E9F88[];
extern s32 D_801E977C[]; // String IDs of the menu scroll bar textures
extern s32 g_MenuStatRowLabelOffsetX[]; // X offset of each stat row of the stat table
extern s32 g_MenuStatRowLabelOffsetY[]; // Y offset of each stat row of the stat table
extern s32 g_MenuStatRowLabelTexIds[]; // Texture id of each stat row's label, 7 rows per stat group

void MenuSetupPointerCursors(s32 arg0);
void MenuFreeScrollHandle(void);
void MenuFreeWindow(s32 arg0);
void func_801C72BC(s32 arg0);
void func_801D7F50(s32 arg0, s32 arg1, u_char arg2);
void func_801D8644(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4);
void MenuLoadStringsFromIds(s32 count, void* pStrings, void* pStringIds, void* pFlags);
void MenuSetRowQuadVertices(s32 arg0, void* pArg1, void* pArg2, void* pArg3, void* pArg4, u8 arg5, u8 arg6, u8 arg7);
void func_801E86C8(s32 arg0);
void MenuSetSubMenuChoicePrims(s32 arg0);
void MenuClearBytes(s32 arg0, void* pArg1);
s32 SoundGetVolumeMode(void);
void func_800386C4(s32 arg0);
void MenuClearGlyphTexture(void);
void MenuFreeCellPrimBlocks(void);
void MenuFreePrimBuffer(void);
void Vsync(s32 mode);
void MenuCloseCardEvents(void);
void MenuRenderFrame(void);
u_char func_801C93A8(void);
void MenuCreateWindow2Strings(s32 arg0);
void MenuFreeWindow2Strings(void);
void MenuSetPrimShadingMode(POLY_FT4* pPoly, u_char mode);
void func_801E8F60(s32 mode, u_char index);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void MenuSetPolySemiTransGrey(P_TAG* pTag);
void ResourceHelperGetTexCoords(MenuResourceDirectory* pDir, u32 index, s32* pDirentCount, s32* pTexPageId, s32* pClutX, s32* pClutY, s32* pTexPageX, s32* pTexPageY);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D7CFC);

/* 78.55% (596 bytes, same size as the original). Full decode: arg1 != 0
   guards the whole body. ((u8*)g_Menu->pGearWindowPrims)[0x2AED] = 0, then two
   windows: func_8002675C(g_Menu->resources, D_801E977C[i], 0x1E00 + i*0x50
   + pGearWindowPrims, g_Menu->renderContext, 0x78 + i*0x20, 0x5A, 0x1000) and
   MenuSetQuadVertices(0x2AA0 + i*0x20 + pGearWindowPrims, x, y, right - x, bottom - y) with
   a u8* pEntry = pGearWindowPrims + (renderContext + i*2)*0x28 and fields +0x1E08,
   +0x1E0A, +0x1E10, +0x1E22 (i.e. 0x1E00-relative 40-byte records).
   Then MenuSetPolySemiTransGrey(pGearWindowPrims + (renderContext + 2*(arg2 == 0))*0x28),
   *(u16*)(pEntry+0x1E16) |= 0x20, *(u8*)(pEntry+0x1E04/05/06) = 0x20,
   ((u8*)pGearWindowPrims)[0x2AEE] = *(u8*)&g_Menu->renderContext,
   ((u8*)pGearWindowPrims)[0x2AED] = 2. Post-call statements must repeat the whole
   address expression inline (a pEntry local kept in a register gives 46.14%,
   this version 78.55%). Remaining diffs: (1) gcc's loop.c strength-reduces
   the D_801E977C[i] giv into an incrementing pointer, which costs a sixth
   saved register and shifts every other saved reg one up (retail keeps the
   indexed lui %hi + addu + lw %lo at-form, and has s0=i, s1=0x1E00+0x50i,
   s2=0x78+0x20i, s3=0x2AA0+0x20i, s4=(arg2 == 0)); frame 0x40 vs 0x38
   follows from that. (2) retail does not CSE the twin lhu of +0x1E08 (loads
   it into a1 and again into v0 for the width subtraction) and loads x/y
   straight into a1/a2, where gcc emits move a1,v0 / move a2,v1.
   func_8002675C args must be s32 x,y,scale (u16 costs an extra andi).
   Also declared: MenuSetQuadVertices, MenuSetPolySemiTransGrey, D_801E977C.
void func_801D7CFC(s32 arg0, u_char arg1, u_char arg2) {
    s32 selected = (arg2 == 0);
    u8* pEntry;
    s32 i;

    if (arg1 != 0) {
        ((u8*)g_Menu->pGearWindowPrims)[0x2AED] = 0;
        for (i = 0; i < 2; i++) {
            func_8002675C(g_Menu->resources, D_801E977C[i],
                          (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + 0x1E00 + i * 0x50),
                          g_Menu->renderContext, 0x78 + i * 0x20, 0x5A, 0x1000);
            pEntry = (u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + i * 2) * 0x28;
            MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x2AA0 + i * 0x20),
                          *(u16*)(pEntry + 0x1E08), *(u16*)(pEntry + 0x1E0A),
                          *(u16*)(pEntry + 0x1E10) - *(u16*)(pEntry + 0x1E08),
                          *(u16*)(pEntry + 0x1E22) - *(u16*)(pEntry + 0x1E0A));
        }
        MenuSetPolySemiTransGrey((P_TAG*)((u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + selected * 2) * 0x28));
        *(u16*)((u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E16) |= 0x20;
        *(u8*)((u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E04) = 0x20;
        *(u8*)((u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E05) = 0x20;
        *(u8*)((u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E06) = 0x20;
        ((u8*)g_Menu->pGearWindowPrims)[0x2AEE] = *(u8*)&g_Menu->renderContext;
        ((u8*)g_Menu->pGearWindowPrims)[0x2AED] = 2;
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D7F50);

/* 99.75% (812 bytes, the original's size). The stat table's row labels: reset the
   prim count at unk35C+0x32F3, draw 7 - statGroup rows of label prims with
   func_8002675C (7 texture ids per stat group in g_MenuStatRowLabelTexIds, x/y from
   g_MenuStatRowLabelOffsetX/g_MenuStatRowLabelOffsetY), shade the prims of every other row 0x40, then set the
   four quad vertices of each prim from its own x0/y0/x1/y3.
   The LAST loop differs only in two addu operand orders: retail is
   `addu v1,a0,v1` (base first) for the prim address and `addu a0,s0,a0`
   (offset first) for the vertex address, gcc emits the reverse of both, which is
   its canonical "lower register number first" order. Tried: a pPrim pointer
   local (97.28, gcc also re-bases 0x2420 and uses v0), a volatile pPrim local
   (98.56), (off + (u32)base) for either address (95.66 for the vertex one, no
   change for the prim one), and two permuter runs - PERMUTER_BETTER 490->426
   from the non-volatile body (a second copy of g_Menu->unk35C, +4 bytes) and
   PERMUTER_NONE base=50 best=50 from this body.
   The six lhu (retail reads x0 and y0 TWICE) come from making the width/height
   operands volatile: without that gcc cse's each pair into one load and adds two
   moves (98.33%, also 812 bytes).
void func_801D7F50(s32 x, s32 y, u_char statGroup) {
    s32 texCoords[6]; // Filled and ignored
    s32 start;
    s32 i;
    s32 j;

    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_STRING_DEFENSE, &texCoords[0], &texCoords[1], &texCoords[2], &texCoords[3], &texCoords[4], &texCoords[5]);
    ((u8*)g_Menu->unk35C)[0x32F3] = 0;
    for (i = 0; i < 7 - statGroup; i++) {
        start = ((u8*)g_Menu->unk35C)[0x32F3];
        ((u8*)g_Menu->unk35C)[0x32F3] += func_8002675C(g_Menu->resources, g_MenuStatRowLabelTexIds[statGroup * 7 + i], (POLY_FT4*)((u8*)g_Menu->unk35C + start * 0x50), g_Menu->renderContext, x + g_MenuStatRowLabelOffsetX[i], y + g_MenuStatRowLabelOffsetY[i], 0x1000);
        if (i & 1) {
            for (j = start; j < ((u8*)g_Menu->unk35C)[0x32F3]; j++) {
                SetShadeTex((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + j * 2) * 0x28), 0);
                ((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + j * 2) * 0x28))->r0 = 0x40;
                ((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + j * 2) * 0x28))->g0 = 0x40;
                ((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + j * 2) * 0x28))->b0 = 0x40;
            }
        }
    }
    for (i = 0; i < ((u8*)g_Menu->unk35C)[0x32F3]; i++) {
        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->unk35C + 0x2420 + i * 0x20),
                      ((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + i * 2) * 0x28))->x0,
                      ((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + i * 2) * 0x28))->y0,
                      ((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + i * 2) * 0x28))->x1 - ((volatile POLY_FT4*)((g_Menu->renderContext + i * 2) * 0x28 + (u32)g_Menu->unk35C))->x0,
                      ((POLY_FT4*)((u8*)g_Menu->unk35C + (g_Menu->renderContext + i * 2) * 0x28))->y3 - ((volatile POLY_FT4*)((g_Menu->renderContext + i * 2) * 0x28 + (u32)g_Menu->unk35C))->y0);
    }
}
*/
void MenuSetStatBarQuadColors(POLY_G4* pPolys, u_char mode) {
    POLY_G4* pPoly;
    u8 rgb[3];
    s32 i;

    switch (mode) {
    case 0:
        rgb[0] = 0xFF;
        rgb[1] = 0x80;
        rgb[2] = 0x80;
        break;
    case 1:
        rgb[0] = 0x80;
        rgb[1] = 0xFF;
        rgb[2] = 0x80;
        break;
    case 2:
        rgb[0] = 0xFF;
        rgb[1] = 0;
        rgb[2] = 0;
        break;
    case 3:
        rgb[0] = 0;
        rgb[1] = 0;
        rgb[2] = 0xFF;
        break;
    }
    for (i = 0; i < 2; i++) {
        pPoly = &pPolys[i];
        SetPolyG4(pPoly);
        pPoly->r0 = rgb[0];
        pPoly->g0 = rgb[1];
        pPoly->b0 = rgb[2];
        pPoly->r1 = rgb[0];
        pPoly->g1 = rgb[1];
        pPoly->b1 = rgb[2];
        pPoly->r2 = 0;
        pPoly->g2 = 0;
        pPoly->b2 = 0;
        pPoly->r3 = 0;
        pPoly->g3 = 0;
        pPoly->b3 = 0;
    }
}
void MenuSetStatBarTileColors(POLY_FT4* pPolys, u_char mode, u_char count, u_char startIndex) {
    POLY_FT4* pPoly;
    u8 rgb[3];
    s32 i;

    rgb[1] = 0x40;
    switch (mode) {
    case 0:
        rgb[0] = 0x80;
        rgb[2] = 0x40;
        break;
    case 1:
        rgb[0] = 0x40;
        rgb[2] = 0x80;
        break;
    case 2:
        rgb[0] = 0x40;
        rgb[2] = 0x40;
        break;
    }
    for (i = 0; i < count; i++) {
        pPoly = &pPolys[startIndex + i * 2];
        SetShadeTex(pPoly, 0);
        pPoly->r0 = rgb[0];
        pPoly->g0 = rgb[1];
        pPoly->b0 = rgb[2];
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D84B4);

/* 99.73% (296 bytes, the original's size). The only residual, in BOTH
   divisions, is the HI result of the /10000: retail does mfhi v0; sra v0,v0,12
   (HI lands straight in the destination), gcc 2.7.2 does mfhi t0; sra v0,t0,12
   (HI into a fresh temp, then shifted). This is a PRESET question, measured:
   the body below is 100.00 under gcc-2.6.0-psx (try_flags.sh), and so is every
   other C function in this unit (MenuSetStatBarQuadColors, MenuSetStatBarTileColors,
   MenuMaxStatValue, MenuDrawStatTable, MenuOpenCardEvents all 100.00 under 2.6.0).
   See report: PRESET_260 menu/main/misc8 func_801D84B4 100.00 99.73.
   Tried under 2.7.2: nested expression 6400 * (100*x/a2) / 10000 (this body,
   best), a scale local with *= (90.47), a temp for the whole product (99.59),
   a pct local (96.42).
void func_801D84B4(u_short arg0, u_short arg1, s32 arg2) {
    s32 diff;

    D_801EA6FC = arg0;
    D_801EA700 = arg1;
    diff = arg1 - arg0;
    D_801EA704 = diff;
    D_801EA708 = 6400 * (100 * arg0 / arg2) / 10000;
    if (diff >= 0) {
        D_801EA710 = 2;
        D_801EA714 = 0xE3;
    } else {
        D_801EA710 = 3;
        D_801EA714 = 0xE5;
        D_801EA704 = arg0 - arg1;
    }
    D_801EA70C = 6400 * (100 * D_801EA704 / arg2) / 10000;
}
*/

s32 MenuMaxStatValue(s32 unusedArg, volatile u16* p0, volatile u16* p1) {
    SVECTOR unusedFrame[3]; // original allocates a 0x18 frame it never writes
    u16 max = 0;
    s32 i;

    for (i = 0; i < 7; p0++) {
        i++;
        if (max < *p0) {
            max = *p0;
        }
    }
    for (i = 0; i < 7; p1++) {
        i++;
        if (max < *p1) {
            max = *p1;
        }
    }

    return max;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D8644);
void MenuDrawStatTable(s32 arg0, u_char arg1, s32 arg2, s32 arg3) {
    s32 width = 0x98;
    s32 height = 0x26;
    u_char value;

    if (arg1 != 0) {
        width = 0x80;
        height = 0x90;
    }
    value = arg3;
    func_801D7F50(width, height, value);
    func_801D8644(arg0, width, height, arg2, value);
    g_Menu->pManager->shouldRenderUnk35C = 1;
    g_Menu->unk35C->renderContext = *(u_char*)&g_Menu->renderContext;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D8EA4);
/* 84.26%. Same instruction multiset; gcc lays the `arg2` if/else out the
   other way in case 1 (it falls through to the 0x60 block and jumps to
   0x30) and therefore places the return block differently. */
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9704);

/* 84.26%. Frame and instruction multiset identical (260 bytes both). gcc lays
   the `arg2` test out the other way round in both cases: it makes the 0x60
   (unk60) block the fall-through and branches with `beqz a2` to the 0x30
   (currentCharacterIDs) block, where the original branches `bnez a2` to the
   0x60 block and falls through into the 0x30 one. Tried: if/else-if in both
   orders, `for (arg0++; 1; arg0++)` vs `while (1)` + trailing arg0++, a
   pManager local (66.43%, makes gcc drop the second block's reload), goto
   labels (right order, but the loop stops being recognised so g_Menu and the
   0xFF constant are no longer hoisted, 62.38%). An explicit
   `arg0++; continue;` plus a second `arg0++` at the bottom gets the block
   order and the `bnez a2` right but cross-jumps the two increments into one
   shared block, costing 8 bytes (85.62%). The increment sitting in the branch
   delay slot of both paths is what the for-increment clause produces.
   Round 15: also tried the arms swapped (arg2 != 0 first) - identical
   84.26%; the block order follows whichever arm is written first and gcc
   then puts a beqz to the other one, so the original's `bnez a2` -> 0x60
   with the 0x30 block as fall-through is not reachable by arm order alone.
   With this variant the case-block prologue also puts `addiu a0,a0,1`
   before `andi a2,a2,0xff` whereas the original has it last.
s32 func_801D9704(s32 arg0, u_char arg1, u_char arg2) {
    switch (arg1) {
    case 0:
        for (arg0++; 1; arg0++) {
            if (arg0 >= 3) {
                arg0 = 0;
            }
            if (arg2 == 0) {
                if (g_Menu->pManager->currentCharacterIDs[arg0] != 0xFF) {
                    break;
                }
            } else if (g_Menu->pManager->isGearScreenActive[arg0] != 0) {
                arg0--;
                break;
            }
        }
        break;
    case 1:
        for (arg0--; 1; arg0--) {
            if (arg0 < 0) {
                arg0 = 2;
            }
            if (arg2 == 0) {
                if (g_Menu->pManager->currentCharacterIDs[arg0] != 0xFF) {
                    break;
                }
            } else if (g_Menu->pManager->isGearScreenActive[arg0] != 0) {
                arg0++;
                break;
            }
        }
        break;
    }
    return arg0;
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9808);

/* 97.08%. Only `case 1` of the g_Menu->input switch differs: the original
   lays the then-arm (subMenuChoice--) out first and branches `beqz` forward to
   the subMenuNumChoices - 1 arm; gcc always emits the numChoices - 1 arm first
   and branches `bnez` forward to the decrement, and it reuses the loaded value
   in the branch delay slot instead of reloading it in the arm (the original
   reloads). Both if/else arm orders give byte-identical code. Everything else
   matches: frame 0x38, $s0 = choice, $s1 = firstPass, $s2 = quit (and the
   loop-test mask), $s3 = accepted, $s4 = the hoisted constant 1 of `case 1`,
   and the blocks come out in the order case 4+5 (fall-through), 1, 3, so the
   switch's source order is 4/5, 1, 3. The function returns 1 (the trailing
   `li v0,1` before jr ra). Full decode below.
s32 func_801D9808(void) {
    u_char quit = 1;
    u_char firstPass = 1;
    u_char accepted = 0;
    s32 choice;

    do {
        MenuRenderFrame();
        if (firstPass != 0) {
            MenuLoadStringsFromIds(4, g_Menu->unk1BE0, D_801EA574, g_Menu->pManager->shouldRenderStrings1BE0);
            func_801E86C8(0);
            g_Menu->subMenuPrevChoice = 0xFF;
            switch (SoundGetVolumeMode()) {
            case 0:
                choice = 0;
                break;
            case 1:
                choice = 2;
                break;
            case 2:
                choice = 1;
                break;
            }
            g_Menu->subMenuChoice = choice;
            firstPass = 0;
        }
        if (g_Menu->subMenuChoice != g_Menu->subMenuPrevChoice) {
            MenuSetRowQuadVertices(6, g_Menu->unk1BE0, g_MenuNameTextureX, D_801E9F88, g_Menu->pManager->shouldRenderStrings1BE0,
                          g_Menu->subMenuChoice, 7, 0);
            MenuSetSubMenuChoicePrims(0);
            g_Menu->subMenuPrevChoice = g_Menu->subMenuChoice;
        }
        switch (g_Menu->input) {
        case 4:
            accepted = 1;
        case 5:
            quit = 0;
            break;
        case 1:
            if (g_Menu->subMenuChoice == 0) {
                g_Menu->subMenuChoice = g_Menu->subMenuNumChoices - 1;
            } else {
                g_Menu->subMenuChoice--;
            }
            break;
        case 3:
            if (++g_Menu->subMenuChoice >= g_Menu->subMenuNumChoices) {
                g_Menu->subMenuChoice = 0;
            }
            break;
        }
    } while (quit != 0);
    if (accepted != 0) {
        switch (g_Menu->subMenuChoice) {
        case 0:
            choice = 0;
            break;
        case 1:
            choice = 2;
            break;
        case 2:
            choice = 1;
            break;
        }
        func_800386C4(choice);
    }
    MenuClearBytes(4, g_Menu->pManager->shouldRenderStrings1BE0);
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    return 1;
}
*/
void MenuOpenCardEvents(void) {
    MenuCloseCardEvents();
    Vsync(0);
    InitCARD(1);
    StartCARD();
    _bu_init();
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    g_Menu->menuUnk2->cardEvents[0] = OpenEvent(0xF4000001, 4, 0x2000, 0);
    g_Menu->menuUnk2->cardEvents[1] = OpenEvent(0xF4000001, 0x8000, 0x2000, 0);
    g_Menu->menuUnk2->cardEvents[2] = OpenEvent(0xF4000001, 0x100, 0x2000, 0);
    g_Menu->menuUnk2->cardEvents[3] = OpenEvent(0xF4000001, 0x2000, 0x2000, 0);
    EnableEvent(g_Menu->menuUnk2->cardEvents[0]);
    EnableEvent(g_Menu->menuUnk2->cardEvents[1]);
    EnableEvent(g_Menu->menuUnk2->cardEvents[2]);
    EnableEvent(g_Menu->menuUnk2->cardEvents[3]);
    ExitCriticalSection();
}
