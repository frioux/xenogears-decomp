#include "common.h"

#include "system/memory.h"
#include "system/menu.h"

extern u8 D_80059460;

void func_801C7BF4(void);
void func_801C851C(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void func_801D1258(void);
void func_801D1B20(void);
void func_801D1BE8(void);
void func_801D1C48(void);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void func_801C8574(u_char soundEffectId);
void func_801D397C(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4, u_char a5, u_char a6, s32 a7, u_char a8);
void func_801E53CC(u_char a0);
void func_801D4EA0(u_char a0);
void func_801D5BA4(u_char a0, u_char a1);
void func_801D5CF8(u_char a0, u_char a1);
void func_801D7C3C(u_char a0, u_char a1);
void func_801D7CFC(u_char a0, u_char a1, u_char a2);
void func_801D8DE4(u_char a0, s32 a1, s32 a2, u_char a3);
void func_801D8EA4(u_char a0, s32 a1, s32 a2, u_char a3);

void func_801D4D1C(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4, u_char a5, s32 a6, u_char a7);
void func_801D29A8(s32 a0, s32 a1);
void func_801E8DA8(u_char a0, u_char a1);
void func_801E8474(s32 a0, u32* a1);

extern u32 D_801EA19C[];
extern u32 D_801EA164[];
extern s16 D_801E9A1C[][2];
extern s16 D_801E9A48[][2];
extern s16 D_801E9E94[][2];
extern u32 D_801EA16C[];
extern u32 D_801E9A58[];
extern u32 D_801E9A68[];

void func_801D1CA0(void) {
    if (g_Menu->shouldDrawMenu) {
        switch (D_80059460) {
            case 0:
                func_801D1B20();
                break;
            case 2:
                func_801D1BE8();
                break;
            case 6:
                func_801D1C48();
                break;
        }
    }
    func_801D1258();
}
void func_801D1D40(void) {
    SystemMenu *pMenu = g_Menu;

    switch (pMenu->transitionEffectState) {
        case 4:
            pMenu->translation.vz = 0x200;
            pMenu->rotation.vx = pMenu->rotation.vy = pMenu->rotation.vz = 0;
            pMenu->translation.vx = pMenu->translation.vy = 0;
            pMenu->transitionEffectState = 2;
            break;
        case 3:
            pMenu->translation.vz = 0x800;
            pMenu->rotation.vx = pMenu->rotation.vy = pMenu->rotation.vz = 0;
            pMenu->translation.vx = pMenu->translation.vy = 0;
            pMenu->transitionEffectState = 1;
            break;
        case 2:
            pMenu->rotation.vy -= 0x60;
            pMenu->translation.vz += 0x40;
            if (pMenu->translation.vz >= 0xE00) {
                pMenu->transitionEffectState = 0;
            }
            break;
        case 1:
            pMenu->rotation.vx += 0x7C;
            pMenu->translation.vz -= 0x30;
            if (pMenu->translation.vz < 0x200) {
                pMenu->translation.vz = 0x200;
                pMenu->rotation.vz = 0;
                pMenu->rotation.vx = 0;
                pMenu->rotation.vy = 0;
                pMenu->transitionEffectState = 0;
            }
            break;
    }
    RotMatrix(&g_Menu->rotation, &g_Menu->matTransform);
    TransMatrix(&g_Menu->matTransform, &g_Menu->translation);
    SetRotMatrix(&g_Menu->matTransform);
    SetTransMatrix(&g_Menu->matTransform);
}
void func_801D1E80(void) {
    g_Menu->transitionEffectState = MENU_OPEN_ANIMATION_START;
    func_801C8574(0x5B);
}
void func_801D1EB0(void) {
    g_Menu->transitionEffectState = MENU_CLOSE_ANIMATION_START;
    func_801C8574(0x5C);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D1EE0);
void func_801D22C4(void) {
    g_Menu->pManager->unk4 = 0;
    g_Menu->pManager->unk3 = 0;
}
void func_801D22F4(u_char a0) {
    s32 i;

    g_Menu->pManager->shouldRenderCursors = 0;
    switch (a0) {
        case 0:
            g_Menu->pManager->shouldRenderCursors = 1;
            g_Menu->pCursors->unk144[0] = 1;
            g_Menu->pCursors->unk144[1] = 1;
        case 2:
            for (i = 0; i < 4; i++) {
                func_8002675C(g_Menu->resources, 0x108, &g_Menu->pCursors->polysCursor[i * 2],
                              g_Menu->renderContext, D_801E9A58[i], D_801E9A68[i], 0x800);
                g_Menu->pCursors->renderContexts[i] = *(u_char*)&g_Menu->renderContext;
            }
            break;
        case 1:
            break;
        case 3:
            func_8002675C(g_Menu->resources, 0x108, g_Menu->pCursors->polysCursor,
                          g_Menu->renderContext, 0, 0, 0x800);
            g_Menu->pCursors->renderContexts[0] = *(u_char*)&g_Menu->renderContext;
            break;
    }
}
void func_801D2484(void) {
    g_Menu->pManager->shouldRenderCursors = 0;
}
extern u_char D_801EA534[]; // String IDs of the menu strings
extern u16 D_801E9E4C[];
extern u16 D_801E9E58[];

void func_801E7E68(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count);

void func_801D249C(u_char a0) {
    s32 i;

    if (a0 != 0) {
        func_801E7E68(g_Menu->unkAE0, D_801EA534, 4, 6);
        for (i = 0; i < 3; i++) {
            func_801C851C(g_Menu->unkAE0[i + 3].vertices, D_801E9E4C[i * 2], D_801E9E58[i * 2], g_Menu->unkAE0[i + 3].width, 0xD);
            g_Menu->unkAE0[i + 3].renderContext = *(u_char*)&g_Menu->renderContext;
            g_Menu->unkAE0[i + 3].unk7F = 1;
            g_Menu->pManager->unk14[i + 3] = 1;
        }
    } else {
        for (i = 0; i < 3; i++) {
            g_Menu->pManager->unk14[i] = 0;
        }
    }
}

void func_801D25E4(void) {
    int i;

    for (i = 0; i < 6; i++) {
        g_Menu->pManager->unk14[i] = 0;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D261C);
/* PARKED 86.91% (652 bytes = retail's exact size). Decoded: set the four corners of
   g_Menu->unkAE0[subMenuChoice].polys[renderContext] from three tables, then copy
   renderContext into that MenuString and set pManager->unk14[subMenuChoice] = 1.
   x0 = x2 = D_801E9A1C[c] + (D_801E9E94[c] + 0x16);  x1 = x3 = width + x0;
   y0 = y1 = D_801E9A48[c] - 0x22;                    y2 = y3 = D_801E9A48[c] - 0x15;
   the source repeats every g_Menu-> load in all eight statements (retail re-reads
   0x308/0x338 per statement; a pointer or index local makes gcc hoist them).
   Findings that matter for the next attempt: (1) the three tables are indexed with
   stride 4 and read with lhu, so they are arrays of 2 halfwords - declared here as
   s16 [][2] and read [c][0]; declaring D_801E9A48 s16 is also what makes gcc use
   lhu + addiu -0x22 instead of lhu + li 0xffde + addu (a 1-D s16 spelling scores
   88.04% but indexes with stride 2, i.e. reads the wrong entry);
   (2) retail's frame is 0x60 (ra at 0x58) and ours is 0x18, so the original has an
   unused ~0x54-byte stack local that has to be declared (the usual unused-local
   trick) before the register allocation will line up.
*/
/* Best attempt (86.91%): the body below is what the note above was measured with.
void func_801D261C(void) {
    func_801D249C(0);

    g_Menu->unkAE0[g_Menu->subMenuChoice].polys[g_Menu->renderContext].x0 = D_801E9A1C[g_Menu->subMenuChoice][0] + (D_801E9E94[g_Menu->subMenuChoice][0] + 0x16);
    g_Menu->unkAE0[g_Menu->subMenuChoice].polys[g_Menu->renderContext].y0 = D_801E9A48[g_Menu->subMenuChoice][0] - 0x22;
    g_Menu->unkAE0[g_Menu->subMenuChoice].polys[g_Menu->renderContext].x1 = g_Menu->unkAE0[g_Menu->subMenuChoice].width + (D_801E9A1C[g_Menu->subMenuChoice][0] + 0x16) + D_801E9E94[g_Menu->subMenuChoice][0];
    g_Menu->unkAE0[g_Menu->subMenuChoice].polys[g_Menu->renderContext].y1 = D_801E9A48[g_Menu->subMenuChoice][0] - 0x22;
    g_Menu->unkAE0[g_Menu->subMenuChoice].polys[g_Menu->renderContext].x2 = D_801E9A1C[g_Menu->subMenuChoice][0] + (D_801E9E94[g_Menu->subMenuChoice][0] + 0x16);
    g_Menu->unkAE0[g_Menu->subMenuChoice].polys[g_Menu->renderContext].y2 = D_801E9A48[g_Menu->subMenuChoice][0] - 0x15;
    g_Menu->unkAE0[g_Menu->subMenuChoice].polys[g_Menu->renderContext].x3 = g_Menu->unkAE0[g_Menu->subMenuChoice].width + (D_801E9A1C[g_Menu->subMenuChoice][0] + 0x16) + D_801E9E94[g_Menu->subMenuChoice][0];
    g_Menu->unkAE0[g_Menu->subMenuChoice].polys[g_Menu->renderContext].y3 = D_801E9A48[g_Menu->subMenuChoice][0] - 0x15;
    g_Menu->unkAE0[g_Menu->subMenuChoice].renderContext = *(u_char*)&g_Menu->renderContext;
    g_Menu->pManager->unk14[g_Menu->subMenuChoice] = 1;
}
*/
void func_801D28A8(void) {
    func_801D397C(0, 0xD4, 0xB2, 0x60, 0x10, 0, 0, 4, 0);
    func_801D5BA4(0xD8, 0xB6);
}
void func_801D28FC(void) {
    func_801D397C(1, 0xCC, 0xC6, 0x50, 0x10, 0, 0, 4, 0);
    func_801D5CF8(0xD0, 0xCA);
    // volatile keeps reorg from sinking this store past the epilogue restores
    *(volatile u8*)&g_Menu->pManager->unk5[1] = 1;
}
void func_801D2968(void) {
    if (g_Menu->pManager->unk5[1] != 0) {
        func_801D5CF8(0xD0, 0xCA);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D29A8);
/* PARKED 99.18% (392 vs 388 bytes, and the instruction multiset is identical).
   Only two allocator ties are left:
   1. the two loop registers come out swapped (retail: the 3-count index in s0 and the
      strength-reduced 6 + i*2 in s1; ours the other way round; the same swap appears
      with an explicit second loop counter "i++, characterSlot += 2");
   2. the gearId value is loaded into a0 in retail (lbu a0 / beq a0 / addiu a0,a0,0xB /
      andi a0,a0,0xff, all in place) while ours keeps it in v0 and truncates straight
      into a0 (andi a0,v0,0xff).  Repeating the memory access in the ternary instead of
      a local, and declaring the local first/last, both leave it in v0.
   What got 97.09 -> 99.18: put the whole ternary in the call argument
   (func_801E8DA8((characterId == 0xFF) ? 0xFF : (u_char)(characterId + 0xB), 6 + (i << 1)))
   instead of assigning it back to the u_char local - that reproduces retail's
   beq/j/andi-in-the-delay-slot shape exactly.
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D2D38);
/* PARKED 99.74% (392 bytes, instruction multiset identical). Only ONE difference is
   left: retail loads the gearId into a0 and truncates in place (lbu a0 / beq a0,s2 /
   addiu a0,a0,0xB / andi a0,a0,0xff); ours loads it into v0 and does andi a0,v0,0xff.
   What got 99.18 -> 99.74: give the TWO loops separate counter variables (i and j).
   With one shared i the allocator hands the first loop's counter s0 and the second
   loop's s1; with i/j retail's shape falls out exactly (first loop in s1, then
   loop2's counter s0 and its 6 + j*2 biv back in s1).
   Tried for the gearId: u_char/int/u32 local, the cast outside the ternary, the cast
   inside, both ternary arm orders. It stays in v0.
*/
/* Best attempt (99.74%): the body below is what the note above was measured with.
void func_801D2D38(void) {
    s32 i;
    s32 j;
    u_char menuCharacterId;
    u_char characterIndex;
    u32 characterId;

    if (D_80059460 == 0) {
        for (i = 0; i < 2; i++) {
            g_Menu->windows[i] = HeapAlloc(0x720, 0);
            bzero(g_Menu->windows[i], 0x720);
            g_Menu->windowParameters[i] = HeapAlloc(0x18, 0);
            bzero(g_Menu->windowParameters[i], 0x18);
            func_801E53CC(i);
        }
        func_801C8574(0x5E);
    }

    for (j = 0; j < 3; j++) {
        menuCharacterId = g_Menu->pManager->currentCharacterIDs[j];
        if (menuCharacterId != 0xFF) {
            characterIndex = j;
            func_801E8DA8(menuCharacterId, characterIndex * 2);
            characterId = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[j]].gearId;
            func_801E8DA8((u_char)((characterId == 0xFF) ? 0xFF : characterId + 0xB), 6 + (j << 1));
        }
    }
    func_801E8474(8, D_801EA19C);
    func_801D29A8(1, 0);
    func_801D28FC();
}
*/
void func_801D2EC0(u_char a0, u_char a1) {
    func_801D7C3C(a0, a1);
    func_801D7CFC(a0, a1, g_GameState.gearRide[a0]);
    func_801C7BF4();
    func_801D8DE4(a0, 0, 0, a1);
    func_801C7BF4();
    func_801D8EA4(a0, 0, 0, a1);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D2F4C);
void func_801D32B4(void) {
    s32 i;

    if (g_Menu->pManager->shouldRenderWindow[2] != 0) {
        func_801D4EA0(2);
        g_Menu->pManager->unk2B = 0;
        for (i = 0; i < 4; i++) {
            HeapFree(g_Menu->unk1DE0[i]);
        }
    }
    func_801C7BF4();
}
void func_801D3344(s32 a0, s32 a1, s32 a2) {
    if (g_Menu->pManager->scrollHandleActive == 0) {
        g_Menu->pScrollHandle = HeapAlloc(0x74, 0);
        bzero(g_Menu->pScrollHandle, 0x74);
    }
    func_8002675C(g_Menu->resources, 0x107, (POLY_FT4*)g_Menu->pScrollHandle, g_Menu->renderContext, a0, a1, 0x1000);
    func_801C851C(g_Menu->pScrollHandle->vertices, a0, a1, 8, a2);
    // the original reads renderContext as a byte here, but as a word for the arg above
    g_Menu->pScrollHandle->renderContext = *(u_char*)&g_Menu->renderContext;
    g_Menu->pManager->scrollHandleActive = 1;
}
void func_801D3444(void) {
    g_Menu->pManager->scrollHandleActive = 0;
    HeapFree(g_Menu->pScrollHandle);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D3488);
/* PARKED 98.90% (492 bytes, identical instruction multiset). The first loop, the
   early-return ternary (write it as "a1 == 0 ? unk32B : unk33B", not the other way
   round), the HeapAlloc/bzero block and the tail all match exactly. The only
   difference left is in the 4-iteration func_801C851C loop: the allocator swaps two
   registers (retail keeps pUi in v1 and the strength-reduced i*2 in a1; ours the
   other way round), so every displacement/operand of that loop differs by the pair.
   Tried: declaring the poly pointer inside/outside the loop, an explicit polys base
   local (96.50), an explicit index local, and both ternary spellings. It is the same
   a1/v1 allocator tie as func_801D2D38 above.
*/
/* Best attempt (98.90%): the body below is what the note above was measured with.
void func_801D3488(u_char a0, u_char a1) {
    s32 i;

    if ((a1 == 0 ? g_Menu->unk32B : g_Menu->unk33B) == 1) {
        return;
    }

    if (g_Menu->pManager->unk63[4] == 0) {
        g_Menu->pShoulderButtonUi = HeapAlloc(0x1C4, 0);
        bzero(g_Menu->pShoulderButtonUi, 0x1C4);
        g_Menu->pManager->unk63[4] = 1;
    }

    for (i = 0; i < 2; i++) {
        func_8002675C(g_Menu->resources, 0x164 + i, &g_Menu->pShoulderButtonUi->polys[i * 4], g_Menu->renderContext, D_801EA164[i], D_801EA16C[a0], 0x1000);
    }

    for (i = 0; i < 4; i++) {
        s32 polyIndex = i * 2 + g_Menu->renderContext;
        POLY_FT4* pPoly = &g_Menu->pShoulderButtonUi->polys[polyIndex];
        func_801C851C(&g_Menu->pShoulderButtonUi->vertices[i * 4], pPoly->x0, pPoly->y0, pPoly->x1 - pPoly->x0, pPoly->y3 - pPoly->y0);
    }

    // the original reads renderContext as a byte here, but as a word for the arg above
    g_Menu->pShoulderButtonUi->renderContext = *(u_char*)&g_Menu->renderContext;
    g_Menu->pManager->shoulderButtonUiActive = 1;
}
*/
void func_801D3674(void) {
    if (g_Menu->pManager->unk5C[0xB] != 0) {
        g_Menu->pManager->shoulderButtonUiActive = 0;
        g_Menu->pManager->unk5C[0xB] = 0;
        HeapFree(g_Menu->pShoulderButtonUi);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D36E0);
void func_801D397C(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4, u_char a5, u_char a6, s32 a7, u_char a8) {
    MenuWindowParameters* pParams;

    if (a0 >= 2) {
        g_Menu->windows[a0] = HeapAlloc(sizeof(MenuWindow), 0);
        bzero(g_Menu->windows[a0], sizeof(MenuWindow));
        g_Menu->windowParameters[a0] = HeapAlloc(sizeof(MenuWindowParameters), 0);
        bzero(g_Menu->windowParameters[a0], sizeof(MenuWindowParameters));
        func_801E53CC(a0);
    }

    pParams = g_Menu->windowParameters[a0];
    if (a5 != 0) {
        pParams->index = a0;
        pParams->unk11 = 0;
        pParams->x = a1;
        pParams->y = a2;
        pParams->width = a3;
        pParams->height = a4;
        pParams->unk8 = 0;
        pParams->unkA = 0;
        g_Menu->pManager->unk27[a0] = 1;
        pParams->unk12 = a6;
        pParams->zIndex = a7;
        return;
    }

    func_801D4D1C(a0, a1, a2, a3, a4, a6, a7, a8);
}

void func_801D3B00(void) {
    u_char flag;
    int i;
    MenuWindowParameters* pWindowInfo;

    for (i = 0; i < MENU_MAX_NUM_WINDOWS; i++) {
        pWindowInfo = g_Menu->windowParameters[i];
        if ((g_Menu->pManager->unk27[i]) && pWindowInfo->unk11 == 0) {
            flag = 0;

            if ((pWindowInfo->unk8 + 32) >= pWindowInfo->width) {
                pWindowInfo->unk8 = pWindowInfo->width;
                flag += 1;
            } else {
                pWindowInfo->unk8 += 32;
            }

            if ((pWindowInfo->unkA + 32) >= pWindowInfo->height) {
                pWindowInfo->unkA = pWindowInfo->height;
                flag += 1;
            } else {
                pWindowInfo->unkA += 32;
            }

            if (flag == 2) {
                pWindowInfo->unk11 = 1;
            }

            func_801D4D1C(
                pWindowInfo->index,
                (pWindowInfo->x + (pWindowInfo->width / 2)) - (pWindowInfo->unk8 / 2),
                (pWindowInfo->y + (pWindowInfo->height / 2)) - (pWindowInfo->unkA / 2),
                pWindowInfo->unk8,
                pWindowInfo->unkA,
                pWindowInfo->unk12,
                pWindowInfo->zIndex,
                pWindowInfo->hasScrollBar
            );
        }
    }
}
