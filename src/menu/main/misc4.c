#include "common.h"

#include "system/menu.h"

extern void func_801CD81C(MenuCharacter* pCharacter, u_char arg1, u_char arg2, s32 arg3, s32 arg4, u_char arg5);
extern void func_801CDB1C(MenuCharacter* pCharacter, u_char arg1, u_char arg2, s32 arg3, s32 arg4);
extern void func_801CDC6C(MenuCharacter* pCharacter, u_char arg1, u_char arg2, s32 arg3, s32 arg4, u_char arg5);
extern void func_801CE198(s32 count, SVECTOR* pVertices, POLY_FT4* pPolys, u_char renderContext);
extern void func_801CE2B4(int length, POLY_FT4* pPolys, int renderContext);
extern void func_801CE860(void);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CD2AC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CD710);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CD81C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CDB1C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CDC6C);
void func_801CE0CC(MenuCharacter* pCharacter, u_char arg1, u_char arg2, s32 arg3, s32 arg4, u_char arg5) {
    func_801CD81C(pCharacter, arg1, arg2, arg3, arg4, arg5);
    func_801CDB1C(pCharacter, arg1, arg2, arg3, arg4);
    func_801CDC6C(pCharacter, arg1, arg2, arg3, arg4, arg5);
    pCharacter->unkBE7 = 1;
    pCharacter->renderContext = ((u_char*)g_Menu)[0x308];
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CE198);
void func_801CE2B4(int length, POLY_FT4* pPolys, int renderContext) {
    s32 i;

    for (i = 0; i < length; i++) {
        AddPrim(&g_Menu->pGfxEnv->ot[4], &pPolys[2 * i + renderContext]);
    }
}
void func_801CE338(void) {
    u32 offset;

    offset = g_Menu->unk348->unk159 * sizeof(DR_MODE) + 0x128;
    AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)(offset + (u32)g_Menu->unk348));
    if (g_Menu->pManager->unk4) {
        offset = g_Menu->unk348->cursorRenderContext * sizeof(POLY_FT4);
        AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)(offset + (u32)g_Menu->unk348));
    }
}
void func_801CE3C8(void) {
    s32 i;
    u32 offset;
    MenuPointerCursors* pCursors;

    if (((u8*)g_Menu->pManager)[0x2F]) {
        for (i = 0; i < 4; i++) {
            pCursors = g_Menu->pCursors;
            if (pCursors->shouldRender[i]) {
                offset = (2 * i + pCursors->renderContexts[i]) * sizeof(POLY_FT4);
                AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)(offset + (u32)pCursors));
            }
        }
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CE464);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CE540);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CE660);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CE860);
void func_801CEB5C(void) {
    u8* pData;

    if (g_Menu->pManager->unk8) {
        pData = (u8*)g_Menu->unk35C;
        func_801CE198(pData[0x32F3], (SVECTOR*)(pData + 0x2420), (POLY_FT4*)pData, pData[0x32F1]);
        func_801CE860();
    }
}
void func_801CEBB4(void) {
    s32 i;
    u8* pData;
    u8* pFlag;
    u8* pRow;

    if (g_Menu->pManager->unk4A[1]) {
        for (i = 0; i < 5; i++) {
            pData = (u8*)g_Menu->unk360;
            pFlag = pData + i;
            if (pFlag[0x294]) {
                pRow = (u8*)(i * 0x80 + (u32)pData);
                func_801CE198(1, (SVECTOR*)(pRow + 0x50), (POLY_FT4*)pRow, pData[0x299]);
            }
        }
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CEC40);
void func_801CF308(void) {
    if (g_Menu->pManager->unkA) {
        func_801CE2B4(g_Menu->unk354->unk1404, g_Menu->unk354->polys500, g_Menu->unk354->unk1409);
        func_801CE2B4(g_Menu->unk354->unk1400, g_Menu->unk354->polys0, g_Menu->unk354->unk1408);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CF37C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CF5E4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CF8D8);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CFB48);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CFF64);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801D01D0);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801D02D8);
void func_801D0954(SVECTOR* pVertices, POLY_FT4* pPolys, s32 polyIndex, s32 otIndex) {
    long interpolated;
    long flag;
    u32 offset;

    RotTransPers4(&pVertices[0], &pVertices[1], &pVertices[2], &pVertices[3],
                  (long*)&pPolys[polyIndex].x0, (long*)&pPolys[polyIndex].x1,
                  (long*)&pPolys[polyIndex].x2, (long*)&pPolys[polyIndex].x3,
                  &interpolated, &flag);
    offset = otIndex * sizeof(u32) + 0x70;
    AddPrim((void*)(offset + (u32)g_Menu->pGfxEnv), &pPolys[polyIndex]);
}
