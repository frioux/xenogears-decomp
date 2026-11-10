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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D22F4);
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc6", func_801D2D38);
/* PARKED 97.09%, 388 bytes = same size as the original (was 90.10 before). The loop
   skeleton now agrees instruction for instruction. Two differences remain: gcc
   allocates the two loop registers the other way round (ours: i in s1, the
   strength-reduced 6 + i*2 in s0; retail: i in s0, 6 + i*2 in s1), and the gearId
   ternary comes out with the opposite branch polarity (ours bne plus a trailing
   andi 0xFF, retail beq with an extra j whose delay slot holds the andi).
   What unlocked 90.10 -> 97.09: reading the character id into a menuCharacterId
   local so gcc CSEs the 0xFF test with the first call's argument (retail does two
   lbu of currentCharacterIDs, not three), and copying i to a u_char characterIndex
   before "characterIndex * 2" so the masked index does not become a third induction
   variable. Declaration order of the three u_char locals does not move the tie.
   Shapes that scored 90.10: (u_char)(i << 1), (i << 1), (i * 2) & 0xFE, an if block
   instead of the negated test, and "6 + characterIndex * 2" (90.55, 396 bytes).
void func_801D2D38(void) {
    s32 i;
    u_char menuCharacterId;
    u_char characterIndex;
    u_char characterId;

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

    for (i = 0; i < 3; i++) {
        menuCharacterId = g_Menu->pManager->currentCharacterIDs[i];
        if (menuCharacterId != 0xFF) {
            characterIndex = i;
            func_801E8DA8(menuCharacterId, characterIndex * 2);
            characterId = g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].gearId;
            characterId = (characterId == 0xFF) ? 0xFF : characterId + 0xB;
            func_801E8DA8(characterId, 6 + (i << 1));
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
