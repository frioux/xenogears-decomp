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
extern u8 D_801EA574[];
extern u8 D_801EA578[];
extern u8 D_801E9F88[];
extern s32 D_801E977C[]; // String IDs of the menu scroll bar textures

void func_801D22F4(s32 arg0);
void func_801D3444(void);
void func_801D4EA0(s32 arg0);
void func_801C72BC(s32 arg0);
void func_801D7F50(s32 arg0, s32 arg1, u_char arg2);
void func_801D8644(u_char arg0, s32 arg1, s32 arg2, u_char arg3, u_char arg4);
void func_801E8018(s32 count, void* pStrings, void* pStringIds, void* pFlags);
void func_801E8070(s32 arg0, void* pArg1, void* pArg2, void* pArg3, void* pArg4, u8 arg5, u8 arg6, u8 arg7);
void func_801E86C8(s32 arg0);
void func_801E8B4C(s32 arg0);
void func_801E8044(s32 arg0, void* pArg1);
s32 func_80038824(void);
void func_800386C4(s32 arg0);
void func_801E64E0(void);
void func_801E5B3C(void);
void func_801E649C(void);
void Vsync(s32 mode);
void func_801C8960(void);
void func_801C7BF4(void);
u_char func_801C93A8(void);
void func_801D2F4C(s32 arg0);
void func_801D32B4(void);
void func_801E8EAC(POLY_FT4* pPoly, u_char mode);
void func_801E8F60(s32 mode, u_char index);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void func_801C851C(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void func_801E91C4(P_TAG* pTag);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801D7CFC);

/* 78.55% (596 bytes, same size as the original). Full decode: arg1 != 0
   guards the whole body. ((u8*)g_Menu->unk358)[0x2AED] = 0, then two
   windows: func_8002675C(g_Menu->resources, D_801E977C[i], 0x1E00 + i*0x50
   + unk358, g_Menu->renderContext, 0x78 + i*0x20, 0x5A, 0x1000) and
   func_801C851C(0x2AA0 + i*0x20 + unk358, x, y, right - x, bottom - y) with
   a u8* pEntry = unk358 + (renderContext + i*2)*0x28 and fields +0x1E08,
   +0x1E0A, +0x1E10, +0x1E22 (i.e. 0x1E00-relative 40-byte records).
   Then func_801E91C4(unk358 + (renderContext + 2*(arg2 == 0))*0x28),
   *(u16*)(pEntry+0x1E16) |= 0x20, *(u8*)(pEntry+0x1E04/05/06) = 0x20,
   ((u8*)unk358)[0x2AEE] = *(u8*)&g_Menu->renderContext,
   ((u8*)unk358)[0x2AED] = 2. Post-call statements must repeat the whole
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
   Also declared: func_801C851C, func_801E91C4, D_801E977C.
void func_801D7CFC(s32 arg0, u_char arg1, u_char arg2) {
    s32 selected = (arg2 == 0);
    u8* pEntry;
    s32 i;

    if (arg1 != 0) {
        ((u8*)g_Menu->unk358)[0x2AED] = 0;
        for (i = 0; i < 2; i++) {
            func_8002675C(g_Menu->resources, D_801E977C[i],
                          (POLY_FT4*)((u8*)g_Menu->unk358 + 0x1E00 + i * 0x50),
                          g_Menu->renderContext, 0x78 + i * 0x20, 0x5A, 0x1000);
            pEntry = (u8*)g_Menu->unk358 + (g_Menu->renderContext + i * 2) * 0x28;
            func_801C851C((SVECTOR*)((u8*)g_Menu->unk358 + 0x2AA0 + i * 0x20),
                          *(u16*)(pEntry + 0x1E08), *(u16*)(pEntry + 0x1E0A),
                          *(u16*)(pEntry + 0x1E10) - *(u16*)(pEntry + 0x1E08),
                          *(u16*)(pEntry + 0x1E22) - *(u16*)(pEntry + 0x1E0A));
        }
        func_801E91C4((P_TAG*)((u8*)g_Menu->unk358 + (g_Menu->renderContext + selected * 2) * 0x28));
        *(u16*)((u8*)g_Menu->unk358 + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E16) |= 0x20;
        *(u8*)((u8*)g_Menu->unk358 + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E04) = 0x20;
        *(u8*)((u8*)g_Menu->unk358 + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E05) = 0x20;
        *(u8*)((u8*)g_Menu->unk358 + (g_Menu->renderContext + selected * 2) * 0x28 + 0x1E06) = 0x20;
        ((u8*)g_Menu->unk358)[0x2AEE] = *(u8*)&g_Menu->renderContext;
        ((u8*)g_Menu->unk358)[0x2AED] = 2;
    }
}
*/
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
   into the destination (mfhi t0; sra v0,t0,12 vs the original mfhi v0;
   sra v0,v0,12), in both divisions. Tried: nested expression, 6400*(x)/10000
   vs (x)*6400/10000, a pct local (96.42), a temp for the whole product
   (99.59), scale local with *= (90.47). Also tried s32 x,y args of
   func_8002675C - irrelevant here.
void func_801D84B4(u_short arg0, u_short arg1, s32 arg2) {
    s32 diff;
    s32 scale;

    D_801EA6FC = arg0;
    D_801EA700 = arg1;
    diff = arg1 - arg0;
    D_801EA704 = diff;
    scale = 100 * arg0 / arg2;
    scale *= 6400;
    D_801EA708 = scale / 10000;
    if (diff >= 0) {
        D_801EA710 = 2;
        D_801EA714 = 0xE3;
    } else {
        D_801EA710 = 3;
        D_801EA714 = 0xE5;
        D_801EA704 = arg0 - arg1;
    }
    scale = 100 * D_801EA704 / arg2;
    scale *= 6400;
    D_801EA70C = scale / 10000;
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
        func_801C7BF4();
        if (firstPass != 0) {
            func_801E8018(4, g_Menu->unk1BE0, D_801EA574, g_Menu->pManager->unk5C);
            func_801E86C8(0);
            g_Menu->subMenuPrevChoice = 0xFF;
            switch (func_80038824()) {
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
            func_801E8070(6, g_Menu->unk1BE0, D_801EA578, D_801E9F88, g_Menu->pManager->unk5C,
                          g_Menu->subMenuChoice, 7, 0);
            func_801E8B4C(0);
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
    func_801E8044(4, g_Menu->pManager->unk5C);
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
    return 1;
}
*/
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
   Round 15: `while (cond) { call(); }` gives the identical 92.55% as the
   for-shape - gcc 2.7.2 rotates this loop (entry `beqz` guard) whatever the
   spelling, and the pManager load also lands in $v1 where the original uses
   $v0 (the original stores $s0, i.e. the `result` local, into _pad33).
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

/* 98.80%. Only difference: the func_801C851C argument setup. Original emits
   `addu a1,v1,s2; addu a2,t0,s1; andi a1,0xffff; andi a2,0xffff` (both sums,
   then both truncations); gcc folds each sum into its argument position and
   emits `addu a1; andi a1; addu a2; andi a2`. Every other line is identical.
   Tried, all with the same interleaving: sums as s32 locals (also with the two
   statements in reverse order), inline expressions, u16 locals, the four poly
   loads hoisted into u16 locals first. cse replaces the use of a single-use sum
   pseudo with the recomputed add, so neither the statement order nor the
   innermost register survives. Also screened (all 98.80): gcc 2.7.2-cdk, -O3,
   -G0.
   Full decode: pCursor = g_Menu->arrowCursors[arg3] (MenuArrowCursor); the
   poly record is &pCursor->polys[g_Menu->renderContext], read as u16 at
   +8/+A/+10/+22 (POLY_FT4.x0/y0/x1/y3, hence lhu), ofsX in $s2, ofsY in $s1.
   void func_801DB0A8(s32 arg0, s32 arg1, u_char arg2, u_char arg3) {
       MenuArrowCursor* pCursor;
       u8* pPoly;
       s32 ofsX;
       s32 ofsY;
       s32 x;
       s32 y;
       u16 lx0;
       u16 ly0;
       u16 lx1;
       u16 ly1;
       s32 shouldDraw = 1;

       pCursor = g_Menu->arrowCursors[arg3];
       if (++pCursor->animFrameDuration >= 6) {
           pCursor->curAnimFrame--;
           if (pCursor->curAnimFrame < 0) {
               pCursor->curAnimFrame = 4;
           }
           pCursor->animFrameDuration = 0;
       }
       switch (arg2) {
       case 0:
           ofsX = (arg0 % 2) * 0x88 + 0x1C;
           ofsY = (arg0 / 2) * 0x10 + 0x11;
           break;
       case 1:
           if (arg0 >= arg1 * 2 && arg0 < arg1 * 2 + 0x10) {
               ofsX = (arg0 % 2) * 0x88 + 0x18;
               ofsY = ((arg0 - arg1 * 2) / 2) * 0x10 + 0x11;
           } else {
               shouldDraw = 0;
           }
           break;
       case 2:
           ofsX = (arg0 % 2) * 0x88 + 0x18;
           ofsY = (arg0 / 2) * 0x10 + 0x14;
           break;
       case 3:
           ofsX = 0xA0;
           ofsY = arg0 * 13 + 0x14;
           shouldDraw = 2;
           break;
       }
       if (shouldDraw != 0) {
           func_8002675C(g_Menu->resources, pCursor->curAnimFrame + 0x15B, (POLY_FT4*)pCursor,
                         g_Menu->renderContext, 0, 0, 0x1000);
           pPoly = (u8*)pCursor + g_Menu->renderContext * 0x28;
           lx0 = *(u16*)(pPoly + 8);
           ly0 = *(u16*)(pPoly + 0xA);
           lx1 = *(u16*)(pPoly + 0x10);
           ly1 = *(u16*)(pPoly + 0x22);
           y = ly0 + ofsY;
           x = lx0 + ofsX;
           func_801C851C(pCursor->vertices, x, y, lx1 - lx0, ly1 - ly0);
           pCursor->renderContext = *(u8*)&g_Menu->renderContext;
           g_Menu->pManager->shouldRenderArrowCursor[arg3] = 1;
       } else {
           g_Menu->pManager->shouldRenderArrowCursor[arg3] = 0;
       }
   }
*/

void func_801DB340(u_char index) {
    HeapFree(g_Menu->arrowCursors[index]);
    g_Menu->pManager->shouldRenderArrowCursor[index] = 0;
}

/* 96.75%. Frame and instruction multiset identical except that the original
   keeps the record pointer in $a2 and re-forms the poly address a second time
   (`addu a0,a2,a1` after the `r0 != 0x20` check) where gcc CSEs it into one
   address, and the original carries the `i * 0x80` offset in an incrementing
   $s2 for the second array. Also `addu` operand order differs in three places
   (original puts the byte offset first, gcc the pointer): spelling the poly
   pointer as `renderContext * 0x28 + (u32)base` fixes the order but relocates
   the shift chain and dropped the function to 87.15%.
void func_801DB39C(u_char arg0) {
    POLY_FT4* pPoly;
    s32 i;

    func_801E8F60(3, arg0);
    func_801E8F60(4, arg0);
    pPoly = &g_Menu->pScrollHandle->polys[g_Menu->pScrollHandle->renderContext];
    func_801E8EAC(pPoly, arg0);
    for (i = 0; i < 0x10; i++) {
        pPoly = &((MenuString*)g_Menu->unk42C)[i].polys[((MenuString*)g_Menu->unk42C)[i].renderContext];
        if (pPoly->r0 != 0x20) {
            func_801E8EAC(pPoly, arg0);
            pPoly = &((MenuString*)g_Menu->unk42C)[i + 0x10]
                         .polys[((MenuString*)g_Menu->unk42C)[i + 0x10].renderContext];
            func_801E8EAC(pPoly, arg0);
        }
    }
    for (i = 0; i < 2; i++) {
        pPoly = &g_Menu->arrowCursors[i]->polys[g_Menu->arrowCursors[i]->renderContext];
        func_801E8EAC(pPoly, arg0);
    }
    pPoly = &((MenuString*)g_Menu->unk42C)[0x20].polys[((MenuString*)g_Menu->unk42C)[0x20].renderContext];
    func_801E8EAC(pPoly, arg0);
    pPoly = &((MenuString*)g_Menu->unk42C)[0x21].polys[((MenuString*)g_Menu->unk42C)[0x21].renderContext];
    func_801E8EAC(pPoly, arg0);
    pPoly = &((MenuString*)g_Menu->unk42C)[0x22].polys[((MenuString*)g_Menu->unk42C)[0x22].renderContext];
    func_801E8EAC(pPoly, arg0);
    for (i = 0; i < 8; i++) {
        pPoly = &g_Menu->unk10E0[i].polys[g_Menu->unk10E0[i].renderContext];
        func_801E8EAC(pPoly, arg0);
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DB39C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8", func_801DB5E4);
