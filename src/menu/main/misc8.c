#include "common.h"

#include "psyq/libapi.h"
#include "system/menu.h"
#include "psyq/libcd.h"

extern u8 D_80059460; // Index of the menu overlay the menu was entered with
extern u16 D_801EA53C[]; // String IDs of the menu strings
extern u16 D_801EA542[]; // String IDs of the menu strings
extern u16 D_801EA548[]; // String IDs of the menu strings
extern s32 D_801EA900;
extern s32 D_801EA904;
extern s32 D_801EA6FC;
extern s32 D_801EA700;
extern s32 D_801EA704;
extern s32 D_801EA708;
extern s32 D_801EA70C;
extern u8 D_801EA710;
extern u8 D_801EA714;
extern CdlCB D_801EA718;
extern CdlCB D_801EA71C;
extern CdlCB D_801EA720;

void func_801D22F4(s32 arg0);
void func_801D3444(void);
void func_801D4EA0(s32 arg0);
void func_801C72BC(s32 arg0);
void func_801D7F50(s32 arg0, s32 arg1, u_char arg2);
void func_801D8644(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4);
void func_801E8018(s32 count, void* pStrings, void* pStringIds, void* pFlags);
void func_801E64E0(void);
void func_801E5B3C(void);
void func_801E649C(void);
void Vsync(s32 mode);
void func_801C8960(void);
void func_801C7BF4(void);
u_char func_801C93A8(void);
void func_801D2F4C(s32 arg0);
void func_801D32B4(void);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D7CFC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D7F50);
void func_801D827C(POLY_G4* pPolys, u_char mode) {
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
void func_801D83AC(POLY_FT4* pPolys, u_char mode, u_char count, u_char startIndex) {
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

/* 99.73%. Identical instruction multiset and frame; the only difference is
   that gcc puts the HI result of the divide-by-10000 in $t0 and shifts it
   into the destination, while the original shifts HI in place (mfhi v0;
   sra v0,v0,12 vs mfhi t0; sra v0,t0,12), in both divisions. Tried: nested
   expression, 6400*(x)/10000 vs (x)*6400/10000, a pct local, a scale local
   holding 6400*pct. All give the same 99.73%.
void func_801D84B4(u_short arg0, u_short arg1, s32 arg2) {
    s32 diff;

    D_801EA6FC = arg0;
    D_801EA700 = arg1;
    diff = arg1 - arg0;
    D_801EA704 = diff;
    D_801EA708 = (100 * arg0 / arg2) * 6400 / 10000;
    if (diff >= 0) {
        D_801EA710 = 2;
        D_801EA714 = 0xE3;
    } else {
        D_801EA710 = 3;
        D_801EA714 = 0xE5;
        D_801EA704 = arg0 - arg1;
    }
    D_801EA70C = (100 * D_801EA704 / arg2) * 6400 / 10000;
}
*/

s32 func_801D85DC(s32 unusedArg, volatile u16* p0, volatile u16* p1) {
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
void func_801D8DE4(s32 arg0, u_char arg1, s32 arg2, s32 arg3) {
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
    g_Menu->pManager->unk8 = 1;
    g_Menu->unk35C->unk32F1 = *(u_char*)&g_Menu->renderContext;
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
            } else if (g_Menu->pManager->unk60[arg0] != 0) {
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
            } else if (g_Menu->pManager->unk60[arg0] != 0) {
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
void func_801D9B08(void) {
    func_801C8960();
    Vsync(0);
    InitCARD(1);
    StartCARD();
    _bu_init();
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    ((s32*)&g_Menu->menuUnk2->unk4FE4)[2] = OpenEvent(0xF4000001, 4, 0x2000, 0);
    ((s32*)&g_Menu->menuUnk2->unk4FE4)[3] = OpenEvent(0xF4000001, 0x8000, 0x2000, 0);
    ((s32*)&g_Menu->menuUnk2->unk4FE4)[4] = OpenEvent(0xF4000001, 0x100, 0x2000, 0);
    ((s32*)&g_Menu->menuUnk2->unk4FE4)[5] = OpenEvent(0xF4000001, 0x2000, 0x2000, 0);
    EnableEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[2]);
    EnableEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[3]);
    EnableEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[4]);
    EnableEvent(((s32*)&g_Menu->menuUnk2->unk4FE4)[5]);
    ExitCriticalSection();
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9C84);

/* 92.55%. Everything but the wait loop matches; gcc rotates the loop
   (entry guard `beqz` plus a duplicated test at the bottom) while the
   original keeps one test and enters it with a `j` (body emitted first).
   Also the original reuses $s0 (the result local, == 1) for the
   pManager->unk33 store. Tried: while, for(;;cond;call), u_char return for
   func_801C93A8 (that one is needed for the andi 0xff) and a pMenu local
   for the two 0xFF stores (also needed).
s32 func_801D9C84(void) {
    MenuUnk2* pMenu;
    s32 result = 1;

    func_801D2F4C(0x20);
    g_Menu->pManager->_pad33 = 1;
    for (; g_Menu->transitionEffectState != 0; func_801C7BF4()) {
    }
    g_Menu->menuUnk2->unk4FE7[0] = 1;
    func_801C7BF4();
    func_801D9B08();
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    D_801EA718 = CdSyncCallback(0);
    D_801EA71C = CdReadyCallback(0);
    D_801EA720 = CdReadCallback(0);
    ExitCriticalSection();
    pMenu = g_Menu->menuUnk2;
    pMenu->unk4FE7[2] = 0xFF;
    pMenu->unk4FE7[1] = 0xFF;
    g_Menu->menuUnk2->unk4F84[4] = 0;
    g_Menu->menuUnk2->unk4F84[5] = 0;
    g_Menu->menuUnk2->unk4FE6 = 2;
    g_Menu->unk326 = 0x3C;
    func_801C7BF4();
    func_801C7BF4();
    if (func_801C93A8() == 0) {
        result = 0;
        if (g_Menu->pManager->_pad33 != 0) {
            func_801D32B4();
            g_Menu->pManager->_pad33 = 0;
        }
    }
    return result;
}
*/
void func_801D9E3C(void) {
    s32 i;

    g_Menu->unk4D8 = 0;
    func_801E64E0();
    func_801E5B3C();
    func_801E649C();
    for (i = 0; i < 0x20; i++) {
        g_Menu->menuUnk2->records[i].unk58 = 0;
    }
    g_Menu->menuUnk2->unk4F8C = 0xFF;
    g_Menu->menuUnk2->unk4F8D = 0xFF;
    D_801EA904 = 0;
    D_801EA900 = 0;
    DrawSync(0);
    Vsync(0);
    EnterCriticalSection();
    CdSyncCallback(D_801EA718);
    CdReadyCallback(D_801EA71C);
    CdReadCallback(D_801EA720);
    ExitCriticalSection();
}

void func_801D9F34(void) {
    if (D_80059460 != 2) {
        func_801E8018(6, g_Menu->unkDE0, D_801EA53C, g_Menu->pManager->unk1A);
    } else {
        func_801E8018(6, g_Menu->unkDE0, D_801EA542, g_Menu->pManager->unk1A);
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D9F98);

void func_801DA4A8(void) {
    MenuUnk9* pMenu;

    func_801D22F4(2);
    func_801E8018(8, g_Menu->unk10E0, D_801EA548, g_Menu->pManager->unk38);
    pMenu = HeapAlloc(0x1198, 0);
    g_Menu->unk42C = pMenu;
    bzero(pMenu, 0x1198);
    func_801C72BC(0);
}

void func_801DA518(void) {
    func_801D3444();
    func_801D4EA0(3);
    func_801D4EA0(4);
    g_Menu->pManager->unk48 = 0;
    func_801C72BC(0x10);
    HeapFree(g_Menu->unk42C->pVramBuffer);
    HeapFree(g_Menu->unk42C);
    HeapFree(g_Menu->pDressingRoom->pItemsData);
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DA5BC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DA9A8);

void func_801DB02C(u_char index) {
    g_Menu->arrowCursors[index] = HeapAlloc(0x78, 0);
    bzero(g_Menu->arrowCursors[index], 0x78);
    g_Menu->arrowCursors[index]->curAnimFrame = 4;
    g_Menu->arrowCursors[index]->animFrameDuration = 0;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DB0A8);

void func_801DB340(u_char index) {
    HeapFree(g_Menu->arrowCursors[index]);
    g_Menu->pManager->shouldRenderArrowCursor[index] = 0;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DB39C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DB5E4);
