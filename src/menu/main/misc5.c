#include "common.h"

#include "system/menu.h"

extern void func_801CE198(s32 count, SVECTOR* pVertices, POLY_FT4* pPolys, u_char renderContext);
extern void func_801D12D4(MenuCharacter* pCharacter, s32 flag);
extern void func_801CE2B4(u_char length, void* pPolys, u_char renderContext);
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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D10DC);
/* Best attempt: 99.39%. Draws the six records at Menu+0x18E0 (array element 2 of
   unk17E0) but every add is one operand order off: the original addu a3,v1,a2
   (base + index) where this emits addu a3,a2,v1. Writing the address as
   g_Menu->unk17E0[2 + i] makes gcc strength-reduce the i+2 induction variable
   into a second saved register instead. Needs a field named at 0x18E0 to match.
void func_801D10DC(void) {
    s32 i;

    for (i = 0; i < 6; i++) {
        if (g_Menu->pManager->unk54[i] && ((MenuString*)&g_Menu->unk17E0[2])[i].unk7F) {
            func_801CE198(1, ((MenuString*)&g_Menu->unk17E0[2])[i].vertices,
                          ((MenuString*)&g_Menu->unk17E0[2])[i].polys,
                          ((MenuString*)&g_Menu->unk17E0[2])[i].renderContext);
        }
    }
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D1160);
/* Best attempt: 99.72%. Same shape as func_801D0D90 but the records start at
   Menu+0x1BE0 (array element 8 of unk17E0), which needs a named field to keep
   the index chain a plain lbu 0x1C5D(g_Menu + (i << 7)); through a cast gcc
   emits addu v0,v0,a0 instead of the original addu v0,a0,v0.
void func_801D1160(void) {
    s32 i;
    u32 offset;

    for (i = 0; i < 4; i++) {
        if (g_Menu->pManager->unk5C[i]) {
            offset = i * sizeof(MenuString) + 0x1BE0;
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (void*)(offset + (u32)g_Menu + ((MenuString*)&g_Menu->unk17E0[8])[i].renderContext * 40));
        }
    }
}
*/

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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D1258);
/* Best attempt: 99.35%. The original emits addu a1,a1,v0 (offset + base) where
   this emits addu a1,v0,a1 (base + offset); the registers holding the two values
   are otherwise identical, as is the whole 24 and 12 byte index chain.
void func_801D1258(void) {
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->unk348->polysDimEffect[g_Menu->renderContext]);
    AddPrim(&g_Menu->pGfxEnv->ot[8], &g_Menu->unk348->drawModeDimEffect[g_Menu->renderContext]);
}
*/
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D1640);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D17C4);
/* Best attempt: 92.88%. Same shape as func_801D14FC against Menu+0x434, but
   gcc keeps the i*0x80 index in a saved register (and drops the 0x400
   induction variable), and the three flag/byte chains come out with the add
   operands the other way round (addu v0,s1,v1 for the original addu v0,v1,s1).
void func_801D17C4(void) {
    s32 i;
    u32 offset;
    u32 offset2;

    if (g_Menu->pManager->unk4A[2] == 0) {
        return;
    }
    for (i = 0; i < 8; i++) {
        if (((u8*)((u32)g_Menu->unk434 + 0xA10))[i]) {
            offset = i * sizeof(MenuString);
            func_801CE198(1, (SVECTOR*)(offset + (u32)g_Menu->unk434 + 0x50),
                          (POLY_FT4*)(offset + (u32)g_Menu->unk434),
                          ((u8*)((u32)g_Menu->unk434 + offset))[0x7D]);
            offset2 = i * sizeof(MenuString) + 0x400;
            func_801CE198(1, (SVECTOR*)(offset2 + (u32)g_Menu->unk434 + 0x50),
                          (POLY_FT4*)(offset2 + (u32)g_Menu->unk434),
                          ((u8*)((u32)g_Menu->unk434 + offset))[0x47D]);
        }
    }
    func_801CE198(1, (SVECTOR*)((u32)g_Menu->unk434 + 0x850), (POLY_FT4*)((u32)g_Menu->unk434 + 0x800),
                  ((u8*)g_Menu->unk434)[0x87D]);
    if (((u8*)((u32)g_Menu->unk434 + 0xA18))[0]) {
        for (i = 0; i < 3; i++) {
            offset2 = i * sizeof(MenuString) + 0x880;
            func_801CE198(1, (SVECTOR*)(offset2 + (u32)g_Menu->unk434 + 0x50),
                          (POLY_FT4*)(offset2 + (u32)g_Menu->unk434),
                          ((u8*)((u32)g_Menu->unk434 + i * sizeof(MenuString)))[0x8FD]);
        }
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D1914);
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
