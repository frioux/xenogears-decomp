#include "common.h"

#include "system/menu.h"

extern void func_801CE198(s32 count, SVECTOR* pVertices, POLY_FT4* pPolys, u_char renderContext);
extern void func_801D12D4(MenuCharacter* pCharacter, s32 flag);
extern void func_801CE2B4(u_char length, void* pPolys, u_char renderContext);
extern void func_801D0954(SVECTOR* pVertices, POLY_FT4* pPolys, s32 polyIndex, s32 otIndex);
extern void func_801D09F0(s32 menuIndex, u_char hasScrollBar);

extern void func_801CF308(void);
extern void func_801D01D0(void);
extern void func_801D02D8(void);
extern void func_801D0C78(void);
extern void func_801D0D90(void);
extern void func_801D0E20(void);
extern void func_801D0E38(void);
extern void func_801D0EBC(void);
extern void func_801D0ED4(void);
extern void func_801D0F54(void);
extern void func_801D0FD4(void);
extern void func_801D1030(void);
extern void func_801D10DC(void);
extern void func_801D1160(void);
extern void func_801D11F0(void);
extern void func_801CE338(void);
extern void func_801CE3C8(void);
extern void func_801CEC40(void);
extern void func_801D3B00(void);
extern void func_801CE540(void);
extern void func_801CE660(void);
extern void func_801CEB5C(void);
extern void func_801CEBB4(void);
extern void func_801CE464(void);
extern void func_801D14FC(void);
extern void func_801D1640(void);
extern void func_801D17C4(void);
extern void func_801D1914(void);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D09F0);
/* Best attempt: 94.75%. Only the AddPrim argument setup differs (the original
   puts addiu 0x70 on the zIndex product and addu pGfxEnv last).
void func_801D09F0(s32 menuIndex, u_char hasScrollBar) {
    MenuWindow* pWindow;
    s32 i;
    long interpolated;
    long flag;

    pWindow = g_Menu->windows[menuIndex];
    for (i = 0; i < 4; i++) {
        func_801D0954(&pWindow->vertsWindowBorderCorners[i * 4], &pWindow->polysWindowBorderCorners[i * 2],
                      pWindow->renderContext, pWindow->zIndex);
    }
    if (hasScrollBar & 0xFF) {
        for (i = 0; i < 2; i++) {
            func_801D0954(&pWindow->vertsScrollBarEnds[i * 4], &pWindow->polysScrollBarEnds[i * 2],
                          pWindow->renderContext, pWindow->zIndex);
        }
        func_801D0954(&pWindow->vertsScrollBarEmpty[0], &pWindow->polysScrollBarEmpty[0], pWindow->renderContext,
                      pWindow->zIndex);
    }
    func_801D0954(&pWindow->vertsWindowBorderTop1[0], &pWindow->polysWindowBorderTop[0], pWindow->renderContext,
                  pWindow->zIndex);
    func_801D0954(&pWindow->vertsWindowBorderTop2[0], &pWindow->polysWindowBorderTop[2], pWindow->renderContext,
                  pWindow->zIndex);
    func_801D0954(&pWindow->vertsWindowBorderBottom1[0], &pWindow->polysWindowBorderBottom[0],
                  pWindow->renderContext, pWindow->zIndex);
    func_801D0954(&pWindow->vertsWindowBorderBottom2[0], &pWindow->polysWindowBorderBottom[2],
                  pWindow->renderContext, pWindow->zIndex);
    func_801D0954(&pWindow->vertsWindowBorderLeft1[0], &pWindow->polysWindowBorderLeft[0], pWindow->renderContext,
                  pWindow->zIndex);
    func_801D0954(&pWindow->vertsWindowBorderLeft2[0], &pWindow->polysWindowBorderLeft[2], pWindow->renderContext,
                  pWindow->zIndex);
    func_801D0954(&pWindow->vertsWindowBorderRight1[0], &pWindow->polysWindowBorderRight[0],
                  pWindow->renderContext, pWindow->zIndex);
    func_801D0954(&pWindow->vertsWindowBorderRight2[0], &pWindow->polysWindowBorderRight[2],
                  pWindow->renderContext, pWindow->zIndex);
    RotTransPers4(&pWindow->vertsBackground[0], &pWindow->vertsBackground[1], &pWindow->vertsBackground[2],
                  &pWindow->vertsBackground[3], (long*)&pWindow->polysBackground[pWindow->renderContext].x0,
                  (long*)&pWindow->polysBackground[pWindow->renderContext].x1,
                  (long*)&pWindow->polysBackground[pWindow->renderContext].x2,
                  (long*)&pWindow->polysBackground[pWindow->renderContext].x3, &interpolated, &flag);
    AddPrim((void*)((u32)g_Menu->pGfxEnv + (0x70 + pWindow->zIndex * sizeof(u32))),
            &pWindow->polysBackground[pWindow->renderContext]);
    AddPrim((void*)((u32)g_Menu->pGfxEnv + (0x70 + pWindow->zIndex * sizeof(u32))),
            &pWindow->drawModes[pWindow->renderContext]);
}*/
void func_801D0C78(void) {
    s32 i;
    SVECTOR rot;
    VECTOR trans;
    MATRIX m;
    long unk48[2]; // unused: the original frame keeps 8 bytes after the matrix

    for (i = 0; i < 7; i++) {
        if (g_Menu->pManager->shouldRenderWindow[i]) {
            if (g_Menu->windows[i]->unk714 == 0) {
                PushMatrix();
                rot.vx = rot.vy = rot.vz = 0;
                trans.vx = trans.vy = 0;
                trans.vz = 0x200;
                RotMatrix(&rot, &m);
                TransMatrix(&m, &trans);
                SetRotMatrix(&m);
                SetTransMatrix(&m);
                func_801D09F0(i, g_Menu->windows[i]->hasScrollBar);
                PopMatrix();
            } else {
                func_801D09F0(i, g_Menu->windows[i]->hasScrollBar);
            }
        }
    }
}

void func_801D0D90(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 4; i++) {
        if (g_Menu->pManager->unk34[i]) {
            offset = i * sizeof(MenuString) + 0x4E0;
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (void*)(offset + (u32)g_Menu + g_Menu->unk4E0[i].renderContext * 40));
        }
    }
}

void func_801D0E20(void) {
    s32 i;

    for (i = 7; i >= 0; i--) {
    }
}

void func_801D0E38(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->unk14[i] && g_Menu->unkAE0[i].unk7F) {
            offset = i * sizeof(MenuString) + 0xAE0;
            func_801CE198(1, (SVECTOR*)(offset + (u32)g_Menu + 0x50),
                          (POLY_FT4*)(offset + (u32)g_Menu), g_Menu->unkAE0[i].renderContext);
        }
    }
}

void func_801D0EBC(void) {
    s32 i;

    for (i = 5; i >= 0; i--) {
    }
}

void func_801D0ED4(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 8; i++) {
        if (g_Menu->pManager->unk38[i]) {
            offset = i * sizeof(MenuString) + 0x10E0;
            func_801CE198(1, (SVECTOR*)(offset + (u32)g_Menu + 0x50), (POLY_FT4*)(offset + (u32)g_Menu),
                          g_Menu->unk10E0[i].renderContext);
        }
    }
}

void func_801D0F54(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->unk40[i]) {
            offset = i * sizeof(MenuString) + 0x14E0;
            func_801CE198(1, (SVECTOR*)(offset + (u32)g_Menu + 0x50), (POLY_FT4*)(offset + (u32)g_Menu),
                          g_Menu->unk14E0[i].renderContext);
        }
    }
}

void func_801D0FD4(void) {
    u32 offset;

    if (g_Menu->pManager->unk4E) {
        offset = g_Menu->unk17E0[0].renderContext * 40 + 0x17E0;
        AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)(offset + (u32)g_Menu));
    }
}

void func_801D1030(void) {
    s32 i;
    MenuString* pString;

    if (((u8*)g_Menu->pManager)[0x2E] == 0) {
        return;
    }
    for (i = 0; i < 3; i++) {
        pString = g_Menu->unk1DE0[i];
        if (pString->unk7F) {
            func_801CE198(1, pString->vertices, pString->polys, pString->renderContext);
        } else {
            AddPrim(&g_Menu->pGfxEnv->ot[4], &pString->polys[pString->renderContext]);
        }
    }
}

void func_801D10DC(void) {
    s32 i;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->unk54[i] && g_Menu->unk18E0[i].unk7F) {
            func_801CE198(1, g_Menu->unk18E0[i].vertices, g_Menu->unk18E0[i].polys,
                          g_Menu->unk18E0[i].renderContext);
        }
    }
}

void func_801D1160(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 4; i++) {
        if (g_Menu->pManager->unk5C[i]) {
            offset = i * sizeof(MenuString) + 0x1BE0;
            AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)(offset + (u32)g_Menu + g_Menu->unk1BE0[i].renderContext * 40));
        }
    }
}

void func_801D11F0(void) {
    func_801D0D90();
    func_801D0E20();
    func_801D0E38();
    func_801D0EBC();
    func_801D10DC();
    func_801D1160();
    func_801D0ED4();
    func_801D0F54();
    func_801D0FD4();
    func_801D1030();
}

void func_801D1258(void) {
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->unk348->polysDimEffect[g_Menu->renderContext]);
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->unk348->drawModeDimEffect[g_Menu->renderContext]);
}
void func_801D12D4(MenuCharacter* pCharacter, s32 flag) {
    if (pCharacter->unkBE7 == 0) {
        return;
    }
    AddPrim(&g_Menu->pGfxEnv->ot[4], &pCharacter->polysPortraitSmall[pCharacter->renderContext]);
    AddPrim(&g_Menu->pGfxEnv->ot[4], &pCharacter->polys4B0[pCharacter->renderContext]);
    func_801CE2B4(pCharacter->descriptionStringsLength, pCharacter, pCharacter->renderContext);
    func_801CE2B4(pCharacter->levelStringLength, pCharacter->polysLevelString, pCharacter->renderContext);
    func_801CE2B4(pCharacter->unkBE1, pCharacter->polys5F0, pCharacter->renderContext);
    func_801CE2B4(pCharacter->hpStringLength, pCharacter->polysHpString, pCharacter->renderContext);
    func_801CE2B4(pCharacter->maxHpStringLength, pCharacter->polysMaxHpString, pCharacter->renderContext);
    func_801CE2B4(pCharacter->mpStringLength, pCharacter->polysMpString, pCharacter->renderContext);
    func_801CE2B4(pCharacter->maxMpStringLength, pCharacter->polysMaxMpString, pCharacter->renderContext);
    if (flag & 0xFF) {
        func_801CE2B4(5, pCharacter->polys2D0, pCharacter->renderContext);
    }
}
void func_801D13F8(void) {
    s32 i;

    if (g_Menu->pManager->unk46) {
        for (i = 0; i < 3; i++) {
            func_801D12D4(g_Menu->currentCharacters[i], 1);
        }
    }
}

void func_801D1464(void) {
    if (g_Menu->pManager->scrollHandleActive) {
        func_801CE198(1, g_Menu->pScrollHandle->vertices, g_Menu->pScrollHandle->polys,
                      g_Menu->pScrollHandle->renderContext);
    }
}

void func_801D14B0(void) {
    if (g_Menu->pManager->shoulderButtonUiActive) {
        func_801CE198(4, g_Menu->pShoulderButtonUi->vertices, g_Menu->pShoulderButtonUi->polys,
                      g_Menu->pShoulderButtonUi->renderContext);
    }
}

void func_801D14FC(void) {
    s32 i;
    u32 offset;
    u32 offset2;

    if (g_Menu->pManager->unk48 == 0) {
        return;
    }
    for (i = 0; i < 16; i++) {
        if (g_Menu->unk42C->unk1184[i]) {
            offset = i * sizeof(MenuString);
            func_801CE198(1, (SVECTOR*)(offset + (u32)g_Menu->unk42C + 0x50),
                          (POLY_FT4*)(offset + (u32)g_Menu->unk42C),
                          ((u8*)(offset + (u32)g_Menu->unk42C))[0x7D]);
            offset2 = i * sizeof(MenuString) + 0x800;
            func_801CE198(1, (SVECTOR*)(offset2 + (u32)g_Menu->unk42C + 0x50),
                          (POLY_FT4*)(offset2 + (u32)g_Menu->unk42C),
                          ((u8*)((u32)g_Menu->unk42C + offset))[0x87D]);
        }
    }
    if (g_Menu->unk42C->unk1184[0x10]) {
        func_801CE198(1, (SVECTOR*)((u32)g_Menu->unk42C + 0x1050), (POLY_FT4*)((u32)g_Menu->unk42C + 0x1000),
                      ((u8*)g_Menu->unk42C)[0x107D]);
        func_801CE198(1, (SVECTOR*)((u32)g_Menu->unk42C + 0x10D0), (POLY_FT4*)((u32)g_Menu->unk42C + 0x1080),
                      ((u8*)g_Menu->unk42C)[0x10FD]);
        func_801CE198(1, (SVECTOR*)((u32)g_Menu->unk42C + 0x1150), (POLY_FT4*)((u32)g_Menu->unk42C + 0x1100),
                      ((u8*)g_Menu->unk42C)[0x117D]);
    }
}
void func_801D1640(void) {
    u32 offset;
    s32 i;
    u32 offset2;
    u32 offset3;

    if (g_Menu->pManager->unk4A[0] == 0) {
        return;
    }
    for (i = 0, offset2 = 0x700; i < 14; i++, offset2 += sizeof(MenuString)) {
        if (((u8*)((u32)g_Menu->unk430 + i))[0x1084]) {
            offset = i * sizeof(MenuString);
            func_801CE198(1, (SVECTOR*)(offset + (u32)g_Menu->unk430 + 0x50),
                          (POLY_FT4*)(offset + (u32)g_Menu->unk430),
                          ((u8*)(offset + (u32)g_Menu->unk430))[0x7D]);
            func_801CE198(1, (SVECTOR*)(offset2 + (u32)g_Menu->unk430 + 0x50),
                          (POLY_FT4*)(offset2 + (u32)g_Menu->unk430),
                          ((u8*)((u32)g_Menu->unk430 + offset))[0x77D]);
        }
    }
    if (((u8*)g_Menu->unk430)[0x1092]) {
        func_801CE198(1, (SVECTOR*)((u32)g_Menu->unk430 + 0xE50), (POLY_FT4*)((u32)g_Menu->unk430 + 0xE00),
                      ((u8*)g_Menu->unk430)[0xE7D]);
        func_801CE198(1, (SVECTOR*)((u32)g_Menu->unk430 + 0xED0), (POLY_FT4*)((u32)g_Menu->unk430 + 0xE80),
                      ((u8*)g_Menu->unk430)[0xEFD]);
        for (i = 0, offset3 = 0xF00; i < 2; i++, offset3 += sizeof(MenuString)) {
            func_801CE198(1, (SVECTOR*)(offset3 + (u32)g_Menu->unk430 + 0x50),
                          (POLY_FT4*)(offset3 + (u32)g_Menu->unk430),
                          ((u8*)((u32)g_Menu->unk430 + (i << 7)))[0xF7D]);
        }
    }
    func_801CE198(1, (SVECTOR*)((u32)g_Menu->unk430 + 0x1050), (POLY_FT4*)((u32)g_Menu->unk430 + 0x1000),
                  ((u8*)g_Menu->unk430)[0x107D]);
}
void func_801D17C4(void) {
    s32 i;
    u32 offset2;
    u32 offset;

    if (g_Menu->pManager->unk4A[2] == 0) {
        return;
    }
    for (i = 0, offset2 = 0x400; i < 8; i++, offset2 += sizeof(MenuString)) {
        if (g_Menu->unk434->unkA10[i]) {
            offset = i * sizeof(MenuString);
            func_801CE198(1, (SVECTOR*)(offset + (u32)g_Menu->unk434 + 0x50),
                          (POLY_FT4*)(offset + (u32)g_Menu->unk434),
                          ((u8*)(offset + (u32)g_Menu->unk434))[0x7D]);
            func_801CE198(1, (SVECTOR*)(offset2 + (u32)g_Menu->unk434 + 0x50),
                          (POLY_FT4*)(offset2 + (u32)g_Menu->unk434),
                          ((u8*)((u32)g_Menu->unk434 + offset))[0x47D]);
        }
    }
    func_801CE198(1, (SVECTOR*)((u32)g_Menu->unk434 + 0x850), (POLY_FT4*)((u32)g_Menu->unk434 + 0x800),
                  ((u8*)g_Menu->unk434)[0x87D]);
    if (g_Menu->unk434->unkA18[0]) {
        for (i = 0; i < 3; i++) {
            offset2 = i * sizeof(MenuString) + 0x880;
            func_801CE198(1, (SVECTOR*)(offset2 + (u32)g_Menu->unk434 + 0x50),
                          (POLY_FT4*)(offset2 + (u32)g_Menu->unk434), g_Menu->unk434->unk880[i].renderContext);
        }
    }
}

void func_801D1914(void) {
    u32 offset;
    s32 i;
    u32 offset2;
    u32 offset3;
    u32 offset4;

    if (g_Menu->pManager->unk4A[3] == 0) {
        return;
    }
    for (i = 0, offset2 = 0x680; i < 13; i++, offset2 += sizeof(MenuString)) {
        if (((u8*)((u32)g_Menu->unk438 + i))[0x2596]) {
            offset = i * sizeof(MenuString);
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (POLY_FT4*)(offset + (u32)g_Menu->unk438 +
                                ((u8*)(offset + (u32)g_Menu->unk438))[0x7D] * sizeof(POLY_FT4)));
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (POLY_FT4*)(offset2 + (u32)g_Menu->unk438 +
                                ((u8*)((u32)g_Menu->unk438 + offset))[0x7D] * sizeof(POLY_FT4)));
        }
    }
    func_801CE198(1, (SVECTOR*)((u32)g_Menu->unk438 + 0xD50), (POLY_FT4*)((u32)g_Menu->unk438 + 0xD00),
                  ((u8*)g_Menu->unk438)[0xD7D]);
    for (i = 0, offset4 = 0x21D0, offset3 = 0xD80; i < 13; offset3 += 0x190, i++, offset4 += 0x48) {
        func_801CE2B4(((u8*)((u32)g_Menu->unk438 + i))[0x257C], (void*)(offset3 + (u32)g_Menu->unk438),
                      ((u8*)((u32)g_Menu->unk438 + i))[0x2589]);
        if (((u8*)((u32)g_Menu->unk438 + i))[0x25A3]) {
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (POLY_G4*)(offset4 + (u32)g_Menu->unk438 +
                               ((u8*)((u32)g_Menu->unk438 + i))[0x25B0] * sizeof(POLY_G4)));
        }
    }
}
void func_801D1AAC(void) {
    s32 i;

    for (i = 0; i < 2; i++) {
        if (g_Menu->pManager->shouldRenderArrowCursor[i]) {
            func_801CE198(1, g_Menu->arrowCursors[i]->vertices, g_Menu->arrowCursors[i]->polys,
                          g_Menu->arrowCursors[i]->renderContext);
        }
    }
}
void func_801D1B20(void) {
    func_801D3B00();
    func_801D11F0();
    func_801CE3C8();
    func_801CE338();
    func_801D02D8();
    func_801D01D0();
    func_801CE540();
    func_801CE660();
    func_801CEB5C();
    func_801CEBB4();
    func_801CE464();
    func_801D13F8();
    func_801D1AAC();
    func_801D14FC();
    func_801D1640();
    func_801D17C4();
    func_801D1914();
    func_801D1464();
    func_801D14B0();
    func_801D0C78();
    func_801CEC40();
    func_801CF308();
}

void func_801D1BE8(void) {
    func_801D3B00();
    func_801D11F0();
    func_801CE3C8();
    func_801CE338();
    func_801D02D8();
    func_801D01D0();
    func_801CEC40();
    func_801CF308();
    func_801D0C78();
}

void func_801D1C48(void) {
    func_801D3B00();
    func_801CE3C8();
    func_801D11F0();
    func_801CE338();
    func_801D02D8();
    func_801D01D0();
    func_801D0C78();
    func_801CF308();
}
