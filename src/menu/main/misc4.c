#include "common.h"

#include "main/game.h"
#include "system/menu.h"

void func_801C80B8(u32 number);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);

extern u8 D_80059460;
extern u_char func_801CB304(void);
extern u_char func_801CBD90(u_char arg0);
extern u_char func_801CC6D8(void);
extern u_char func_801CD2AC(void);
extern void func_801CD81C(MenuCharacter* pCharacter, u_char arg1, u_char arg2, s32 arg3, s32 arg4, u_char arg5);
extern void func_801CDB1C(MenuCharacter* pCharacter, u_char arg1, u_char arg2, s32 arg3, s32 arg4);
extern void func_801CDC6C(MenuCharacter* pCharacter, u_char arg1, u_char arg2, s32 arg3, s32 arg4, u_char arg5);
extern void func_801CE198(s32 count, SVECTOR* pVertices, POLY_FT4* pPolys, s32 renderContext);
extern void func_801CE2B4(int length, POLY_FT4* pPolys, int renderContext);
extern void func_801CE860(void);
extern void func_801CF37C(void);
extern void func_801CF5E4(s32 arg0, s32 arg1, s32 arg2);
extern void func_801CF8D8(void);
extern void func_801CFB48(void);
extern void func_801CFF64(void);
extern void func_801D22F4(s32 arg0);
extern void func_801D2484(void);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CD2AC);
s32 func_801CD710(u_char arg0) {
    u_char result = 1;
    u8 choice;

    func_801D22F4(0);
    ((u8*)g_Menu->pManager)[0x2F] = 0;
    choice = g_Menu->subMenuChoice;
    switch (choice) {
        case 0:
            g_Menu->unk334 = 7;
            if (func_801CD2AC()) {
                result = 0;
            }
            break;
        case 1:
            g_Menu->unk334 = 6;
            if (func_801CC6D8()) {
                result = 0;
            }
            break;
        case 2:
            if (D_80059460 != g_Menu->subMenuChoice) {
                g_Menu->unk334 = 3;
                if (func_801CBD90(arg0)) {
                    result = 0;
                }
            } else {
                g_Menu->unk334 = 2;
                if (func_801CB304()) {
                    result = 0;
                }
            }
            break;
    }
    func_801D2484();
    return result;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CD81C);
void func_801CDB1C(MenuCharacter* pCharacter, u_char characterIndex, u_char localIndex, s32 arg3, s32 arg4) {
    s32* pPositionsY = (s32*)arg4;
    s32 i;
    u_char digit;

    func_801C80B8(g_GameState.characters[characterIndex].level);
    pCharacter->levelStringLength = 0;
    for (i = 0; i < 3; i++) {
        digit = g_Menu->digits[i + 6];
        if (digit != 0xFF) {
            pCharacter->levelStringLength += func_8002675C(
                g_Menu->resources, digit,
                &pCharacter->polysLevelString[pCharacter->levelStringLength * 2],
                g_Menu->renderContext,
                i * 8 + ((s32*)arg3)[10],
                localIndex * 0x38 + pPositionsY[10],
                0x1000);
        }
    }
    func_801C80B8(g_GameState.characters[characterIndex].unk63);
    pCharacter->unkBE1 = 0;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CDC6C);
void func_801CE0CC(MenuCharacter* pCharacter, u_char arg1, u_char arg2, s32 arg3, s32 arg4, u_char arg5) {
    func_801CD81C(pCharacter, arg1, arg2, arg3, arg4, arg5);
    func_801CDB1C(pCharacter, arg1, arg2, arg3, arg4);
    func_801CDC6C(pCharacter, arg1, arg2, arg3, arg4, arg5);
    pCharacter->unkBE7 = 1;
    pCharacter->renderContext = ((u_char*)g_Menu)[0x308];
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CE198);
/* 96.06%: only ordering of the compiler-made induction variables differs.  The
   4th argument must be s32 (a u_char parameter adds an andi and loses 8%).
   Retail initialises the poly index IV before the loop-invariant li's and
   updates it first in the latch; gcc 2.7.2 does both last.  Tried: index as a
   local recomputed from i (best), index hoisted before the loop with += 2 in
   the body and in the for-increment, u_char vs s32 parameter, a pPoly local
   computed first in the body (same 96.06).  The emitted IV order is the
   reverse of the RTL discovery order, so the poly-index expression would have
   to be expanded after the four pVertices ones; no shape yet achieves it.
void func_801CE198(s32 count, SVECTOR* pVertices, POLY_FT4* pPolys, s32 renderContext) {
    s32 index;
    s32 i;
    long interpolated;
    long flag;

    for (i = 0; i < count; i++) {
        index = 2 * i + renderContext;
        RotTransPers4(&pVertices[2 * i], &pVertices[2 * i + 1], &pVertices[2 * i + 2],
                      &pVertices[2 * i + 3], (long*)&pPolys[index].x0, (long*)&pPolys[index].x1,
                      (long*)&pPolys[index].x2, (long*)&pPolys[index].x3, &interpolated, &flag);
        AddPrim(&g_Menu->pGfxEnv->ot[4], &pPolys[index]);
    }
}
*/
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
void func_801CE464(void) {
    u32 offset;

    if (((u8*)g_Menu->pManager)[0x5]) {
        func_801CE2B4(*(s32*)((u8*)g_Menu->unk340 + 0x320), (POLY_FT4*)g_Menu->unk340,
                      ((u8*)g_Menu->unk340)[0x324]);
        offset = ((u8*)g_Menu->unk340)[0x324] * sizeof(POLY_FT4) + 0x2D0;
        AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)(offset + (u32)g_Menu->unk340));
    }
    if (((u8*)g_Menu->pManager)[0x6]) {
        func_801CE2B4(7, (POLY_FT4*)g_Menu->unk344, ((u8*)g_Menu->unk344)[0x370]);
        func_801CE2B4(4, (POLY_FT4*)((u8*)g_Menu->unk344 + 0x230), ((u8*)g_Menu->unk344)[0x370]);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CE540);
/* 92.14%: every instruction is right except (a) retail does `i++` in the beqz
   delay slot, i.e. before the body, while gcc keeps it at the latch, and (b)
   the second AddPrim computes +0x50 before loading pGfxEnv.  Putting the
   increment in the if condition (i++ in the subscript) fixes the delay slot but
   makes the scheduler hoist the pData load above the flag load (88.88%).
   Also tried: continue-guard, separate u8 flag local, both statement orders,
   i++ in the pData subscript with pData textually first (87.33, 296 bytes),
   i++ only in the flag subscript with an empty for-increment (88.88), flag
   read first via unk39C[i-1] (89.03, 300 bytes), do/while(++i<3) (92.14).
void func_801CE540(void) {
    s32 i;
    u8* pData;

    for (i = 0; i < 3; i++) {
        pData = (u8*)g_Menu->unk39C[i];
        if (((u8*)g_Menu->pManager)[i]) {
            AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)(pData + pData[0x1270] * sizeof(POLY_FT4)));
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (void*)(pData + pData[0x1270] * sizeof(POLY_FT4) + 0x50));
            func_801CE2B4(pData[0x1279], (POLY_FT4*)(pData + 0xA0), pData[0x1270]);
            func_801CE2B4(pData[0x1273], (POLY_FT4*)(pData + 0xAF0), pData[0x1270]);
            func_801CE2B4(pData[0x1274], (POLY_FT4*)(pData + 0xBE0), pData[0x1270]);
            func_801CE2B4(pData[0x1275], (POLY_FT4*)(pData + 0xCD0), pData[0x1270]);
            func_801CE2B4(pData[0x1276], (POLY_FT4*)(pData + 0xD70), pData[0x1270]);
            func_801CE2B4(pData[0x1277], (POLY_FT4*)(pData + 0xE10), pData[0x1270]);
            func_801CE2B4(pData[0x1271], (POLY_FT4*)(pData + 0x910), pData[0x1270]);
        }
    }
}
*/
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
void func_801D01D0(void) {
    func_801CF37C();
    func_801CF5E4(0, 0, 0x10);
    func_801CF5E4(0x10, 0x10, 0x20);
    func_801CFB48();
    func_801CF8D8();
    func_801CFF64();
    if (++g_Menu->unk4D0 == 0xF) {
        g_Menu->unk4D0 = 0;
        if (++g_Menu->unk4CC == 6) {
            g_Menu->unk4CC = 0;
        }
    }
    if (!g_Menu->unk4D9) {
        g_Menu->unk4D4 += 4;
        if (g_Menu->unk4D4 >= 0x81) {
            g_Menu->unk4D9 = 1;
            g_Menu->unk4D4 = 0x7C;
        }
    } else {
        g_Menu->unk4D4 -= 4;
        if (g_Menu->unk4D4 < 0) {
            g_Menu->unk4D9 = 0;
            g_Menu->unk4D4 = 4;
        }
    }
}
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
