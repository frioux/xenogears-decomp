#include "common.h"

#include "system/menu.h"

extern void func_801CE198(s32 count, SVECTOR* pVertices, POLY_FT4* pPolys, u_char renderContext);

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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D09F0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D0C78);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D0D90);

void func_801D0E20(void) {
    s32 i;

    for (i = 7; i >= 0; i--) {
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D0E38);

void func_801D0EBC(void) {
    s32 i;

    for (i = 5; i >= 0; i--) {
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D0ED4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D0F54);

void func_801D0FD4(void) {
    u8 index;

    if (g_Menu->pManager->unk4E) {
        index = g_Menu->unk0x10E0[0x77D];
        AddPrim(&g_Menu->pGfxEnv->ot[4], (POLY_FT4*)(g_Menu->unk0x10E0 + 0x700) + index);
    }
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D1030);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D10DC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D1160);

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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D12D4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D13F8);

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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D14FC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D1640);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D17C4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D1914);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D1AAC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc5", func_801D1B20);

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
