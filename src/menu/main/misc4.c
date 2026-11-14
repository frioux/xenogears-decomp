#include "common.h"

#include "main/game.h"
#include "system/menu.h"

void MenuParseNumberToDigits(u32 number);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);

extern u8 D_80059460;
extern s32 D_801E981C[];
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
extern void MenuSetupPointerCursors(s32 arg0);
extern void MenuHidePointerCursors(void);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CD2AC);
s32 func_801CD710(u_char arg0) {
    u_char result = 1;
    u8 choice;

    MenuSetupPointerCursors(0);
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
    MenuHidePointerCursors();
    return result;
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CD81C);
void func_801CDB1C(MenuCharacter* pCharacter, u_char characterIndex, u_char localIndex, s32 arg3, s32 arg4) {
    s32* pPositionsY = (s32*)arg4;
    s32 i;
    u_char digit;

    MenuParseNumberToDigits(g_GameState.characters[characterIndex].level);
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
    MenuParseNumberToDigits(g_GameState.characters[characterIndex].unk63);
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
/* 99.80%: only the register holding the g_Menu pointer in the prologue differs
   (retail $v1, gcc $a1; 4 instructions).  Same 99.80 under 2.7.2-cdk, -O3 and
   -G0, so it is an allocator pick, not the preset.  Tried: the shared-header
   cast ((u8*)g_Menu->pManager)[0x7], the named field pManager->unk5[2], an
   explicit SystemMenu* pMenu local, and a u8 flag local read before the if.
void func_801CE660(void) {
    u8* pData;

    if (((u8*)g_Menu->pManager)[0x7]) {
        pData = (u8*)g_Menu->unk358;
        func_801CE198(1, (SVECTOR*)(pData + 0x1EA0), (POLY_FT4*)pData, pData[0x2AE0]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(1, (SVECTOR*)(pData + 0x1EC0), (POLY_FT4*)(pData + 0x50), pData[0x2AE0]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(pData[0x2AED], (SVECTOR*)(pData + 0x2AA0), (POLY_FT4*)(pData + 0x1E00),
                      pData[0x2AEE]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(pData[0x2AEC], (SVECTOR*)(pData + 0x1EE0), (POLY_FT4*)(pData + 0xA0), pData[0x2AE0]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(pData[0x2AE7], (SVECTOR*)(pData + 0x2780), (POLY_FT4*)(pData + 0x1630), pData[0x2AE0]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(pData[0x2AE8], (SVECTOR*)(pData + 0x2820), (POLY_FT4*)(pData + 0x17C0), pData[0x2AE0]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(pData[0x2AE9], (SVECTOR*)(pData + 0x28C0), (POLY_FT4*)(pData + 0x1950), pData[0x2AE0]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(pData[0x2AEA], (SVECTOR*)(pData + 0x2960), (POLY_FT4*)(pData + 0x1AE0), pData[0x2AE0]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(pData[0x2AE1], (SVECTOR*)(pData + 0x2300), (POLY_FT4*)(pData + 0xAF0), pData[0x2AE0]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(pData[0x2AE3], (SVECTOR*)(pData + 0x23C0), (POLY_FT4*)(pData + 0xCD0), pData[0x2AE0]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(pData[0x2AE5], (SVECTOR*)(pData + 0x25C0), (POLY_FT4*)(pData + 0x11D0), pData[0x2AE0]);
        pData = (u8*)g_Menu->unk358;
        func_801CE198(pData[0x2AEB], (SVECTOR*)(pData + 0x2A00), (POLY_FT4*)(pData + 0x1C70), pData[0x2AE0]);
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CE860);
/* 88.90% (804 vs 792 bytes): every expression and call is right; the only
   delta is that loop.c strength-reduces `i * 0x20` and `i * 0x80` here, so gcc
   allocates two extra induction registers ($s4/$s5: +0x20 and +0x80 at the
   latch) and computes the verts base as addiu(pData,K) + biv instead of
   retail's `sll; addiu; addu`.  Retail has no mult-giv registers at all - it
   re-emits `sll $a0,$s0,5` in both blocks - so its givs must have kept a
   non-zero add_val (0x31E0 vs 0x3100) and each stayed below the benefit
   threshold.  Tried: the base as a local pointer, `(pData + i)[K]` for the
   byte indexes (that fixed the addu operand order), a shared `rowOfs = i*0x20`
   local, and -O1 (55.6, so not a preset problem).  Same 88.90 in all shapes.
void func_801CE860(void) {
    s32 i;
    s32 offset0 = 0;
    s32 offset1 = 0x2030;
    s32 offset2 = 0x2228;
    long interpolated;
    long flag;
    SVECTOR* pVerts;
    s32 rowOfs;

    for (i = 0; i < 7; i++) {
        rowOfs = i * 0x20;
        if ((((u8*)g_Menu->unk35C) + i)[0x32EA]) {
            if (((u8*)g_Menu->unk35C)[0x32F2]) {
                pVerts = (SVECTOR*)((rowOfs + 0x31E0) + (u32)g_Menu->unk35C);
                RotTransPers4(&pVerts[0], &pVerts[1], &pVerts[2], &pVerts[3],
                              (long*)(offset2 + (u32)g_Menu->unk35C +
                                      (((u8*)g_Menu->unk35C) + i)[0x32E3] * 36 + 8),
                              (long*)(offset2 + (u32)g_Menu->unk35C +
                                      (((u8*)g_Menu->unk35C) + i)[0x32E3] * 36 + 0x10),
                              (long*)(offset2 + (u32)g_Menu->unk35C +
                                      (((u8*)g_Menu->unk35C) + i)[0x32E3] * 36 + 0x18),
                              (long*)(offset2 + (u32)g_Menu->unk35C +
                                      (((u8*)g_Menu->unk35C) + i)[0x32E3] * 36 + 0x20),
                              &interpolated, &flag);
                AddPrim(&g_Menu->pGfxEnv->ot[4],
                        (void*)(offset2 + (u32)g_Menu->unk35C +
                                (((u8*)g_Menu->unk35C) + i)[0x32E3] * 36));
                func_801CE198((((u8*)g_Menu->unk35C) + i)[0x32CE],
                              (SVECTOR*)((rowOfs * 4 + 0x2D80) + (u32)g_Menu->unk35C),
                              (POLY_FT4*)((offset0 + 0x1770) + (u32)g_Menu->unk35C),
                              (((u8*)g_Menu->unk35C) + i)[0x32D5]);
            }
            pVerts = (SVECTOR*)((rowOfs + 0x3100) + (u32)g_Menu->unk35C);
            RotTransPers4(&pVerts[0], &pVerts[1], &pVerts[2], &pVerts[3],
                          (long*)(offset1 + (u32)g_Menu->unk35C +
                                  (((u8*)g_Menu->unk35C) + i)[0x32DC] * 36 + 8),
                          (long*)(offset1 + (u32)g_Menu->unk35C +
                                  (((u8*)g_Menu->unk35C) + i)[0x32DC] * 36 + 0x10),
                          (long*)(offset1 + (u32)g_Menu->unk35C +
                                  (((u8*)g_Menu->unk35C) + i)[0x32DC] * 36 + 0x18),
                          (long*)(offset1 + (u32)g_Menu->unk35C +
                                  (((u8*)g_Menu->unk35C) + i)[0x32DC] * 36 + 0x20),
                          &interpolated, &flag);
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (void*)(offset1 + (u32)g_Menu->unk35C +
                            (((u8*)g_Menu->unk35C) + i)[0x32DC] * 36));
            func_801CE198((((u8*)g_Menu->unk35C) + i)[0x32C0],
                          (SVECTOR*)((rowOfs * 4 + 0x2A00) + (u32)g_Menu->unk35C),
                          (POLY_FT4*)((offset0 + 0xEB0) + (u32)g_Menu->unk35C),
                          (((u8*)g_Menu->unk35C) + i)[0x32C7]);
        }
        offset0 += 0x140;
        offset1 += 0x48;
        offset2 += 0x48;
    }
}
*/

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
/* 98.89%: everything matches except where gcc puts `valid = 1` relative to the
   `character == 0xFF` compare (retail emits it before the compare and lets reorg
   duplicate the `andi` into the delay slot; gcc puts it in that slot instead).
   Notes: D_801E981C is the overlay dlabel table in asm/menu/data/246A4.data.s so
   it only needs an extern declaration.  The three colour bytes read the low byte
   at 0x4D4 through a u8* cast (retail uses lbu) and the AddPrim offsets must be
   written as &pData[ctx * N + K] (array subscript): that is what makes the addiu
   land before the addu.  Also tried: else-branch assignment to valid (96%), the
   || form valid = character != 0xFF || ... (worse), a u32 offset local hoisted
   above each AddPrim (95.10%), and offset + pData operand order (96.78%).
void func_801CF37C(void) {
    s32 i;
    u8 character;
    u8 valid;
    u8* pData;
    long interpolated;
    long flag;

    if (g_Menu->unk4D8 != 2) {
        return;
    }
    character = g_Menu->menuUnk2->unk4FAE[D_801E981C[g_Menu->menuUnk2->unk4F7C]];
    for (i = 0; i < 0x20; i++) {
        if (character != g_Menu->menuUnk2->unk4FAE[i]) {
            continue;
        }
        valid = 1;
        if (character == 0xFF) {
            valid = i == D_801E981C[g_Menu->menuUnk2->unk4F7C];
        }
        if (!valid) {
            continue;
        }
        pData = (u8*)g_Menu->unk3A8[i];
        *(u8*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xB4) = ((u8*)g_Menu)[0x4D4];
        *(u8*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xB5) = ((u8*)g_Menu)[0x4D4];
        *(u8*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xB6) = ((u8*)g_Menu)[0x4D4];
        RotTransPers4((SVECTOR*)(pData + 0xE0), (SVECTOR*)(pData + 0xE8), (SVECTOR*)(pData + 0xF0),
                      (SVECTOR*)(pData + 0xF8),
                      (long*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xB8),
                      (long*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xBC),
                      (long*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xC0),
                      (long*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xC4), &interpolated, &flag);
        AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)&pData[g_Menu->renderContext * 0x18 + 0xB0]);
        AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)&pData[g_Menu->renderContext * 12 + 0x140]);
    }
}
*/

INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CF5E4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CF8D8);
/* 70.72% (548 vs 620 bytes): the two func_8002675C calls and the AddPrim
   loop are in place; what is still wrong is the register/stack-arg layout of
   the six loop-invariant constants (retail hoists 0x1E/0x1B/0x36/0x1000/0x4D94
   into $s4/$s6/$fp/$s7/$s3 with i in $s1, mine allocates differently) and the
   `rendered[2]` flag array, so the whole prologue and most operands differ.
void func_801CF8D8(void) {
    s32 x0 = 0x1E;
    s32 x1 = 0x1B;
    s32 y = 0x36;
    s32 scale = 0x1000;
    s32 base = 0x4D94;
    u8 rendered[2] = { 0, 0 };
    u8* pData;
    s32 count;
    s32 i;
    s32 j;

    for (i = 0; i < 2; i++) {
        pData = (u8*)g_Menu->menuUnk2;
        if (pData[0x4FE4 + i] && g_Menu->pManager->unk63[5]) {
            func_8002675C(g_Menu->resources,
                          (((u8*)g_Menu->pManager)[0x2F] && i == D_801E981C[pData[0x4F7C]] / 16)
                              ? 0x122
                              : 0x115,
                          (POLY_FT4*)(pData + base), g_Menu->renderContext, x0, y, scale);
            count = func_8002675C(g_Menu->resources, i + 0x162, (POLY_FT4*)(pData + base + 0x50),
                                  g_Menu->renderContext, x1, y, scale);
            rendered[i] = 1;
        }
        if (rendered[i]) {
            for (j = 0; j < count; j++) {
                AddPrim(&g_Menu->pGfxEnv->ot[4],
                        (void*)((u32)((u8*)g_Menu->menuUnk2 + base) +
                                ((2 * j + g_Menu->renderContext) * sizeof(POLY_FT4) + 0x50)));
            }
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (void*)((u32)((u8*)g_Menu->menuUnk2 + base) +
                            g_Menu->renderContext * sizeof(POLY_FT4)));
        }
        base += 0xF0;
        x0 += 0x90;
        x1 += 0x90;
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CFB48);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4", func_801CFF64);
typedef struct {
    /* 0x00 */ u8 unk0[0x8];
    /* 0x08 */ s16 unk8;
    /* 0x0A */ s16 unkA;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
} MenuUnk44CEntry; // Size: 0x18
/* 97.97%: identical instruction multiset; the only deltas are the div-by-6
   magic.  Retail materialises the constant's lui one instruction early (inside
   the last store's address computation) and so keeps 0x68 in $a2, putting
   unk2D8 in $a0 and the magic in $a1; gcc materialises lui+ori both after the
   store, so 0x68 takes $a1 and the div lands in $v1/$v0/$t0.
   The condition must be written `rem >= 4` (sltiu+beqz), not `< 4`.

void func_801CFF64(void) {
    s32 value;

    if (((u8*)g_Menu->pManager)[0x52] == 0) {
        return;
    }
    ((MenuUnk44CEntry*)(((u8**)g_Menu->unk44C)[0] + ((u8**)g_Menu->unk44C)[0][0x7B8] * 0x18))->unk8 = 0x20;
    ((MenuUnk44CEntry*)(((u8**)g_Menu->unk44C)[0] + ((u8**)g_Menu->unk44C)[0][0x7B8] * 0x18))->unkA = 0x61;
    ((MenuUnk44CEntry*)(((u8**)g_Menu->unk44C)[0] + ((u8**)g_Menu->unk44C)[0][0x7B8] * 0x18))->unkC =
        *(u16*)(((u8**)g_Menu->unk44C)[0] + 0x7B0) + 0x20;
    ((MenuUnk44CEntry*)(((u8**)g_Menu->unk44C)[0] + ((u8**)g_Menu->unk44C)[0][0x7B8] * 0x18))->unkE = 0x61;
    ((MenuUnk44CEntry*)(((u8**)g_Menu->unk44C)[0] + ((u8**)g_Menu->unk44C)[0][0x7B8] * 0x18))->unk10 = 0x20;
    ((MenuUnk44CEntry*)(((u8**)g_Menu->unk44C)[0] + ((u8**)g_Menu->unk44C)[0][0x7B8] * 0x18))->unk12 = 0x68;
    ((MenuUnk44CEntry*)(((u8**)g_Menu->unk44C)[0] + ((u8**)g_Menu->unk44C)[0][0x7B8] * 0x18))->unk14 =
        *(u16*)(((u8**)g_Menu->unk44C)[0] + 0x7B0) + 0x20;
    ((MenuUnk44CEntry*)(((u8**)g_Menu->unk44C)[0] + ((u8**)g_Menu->unk44C)[0][0x7B8] * 0x18))->unk16 = 0x68;
    if (g_Menu->unk2D8 % 6 >= 4 || ((u8*)g_Menu->pManager)[0x52] == 2) {
        func_801CE2B4(2, (POLY_FT4*)(((u8**)g_Menu->unk44C)[0] + 0x3F0), ((u8**)g_Menu->unk44C)[0][0x7B8]);
    }
    func_801CE2B4(0xC, (POLY_FT4*)(((u8**)g_Menu->unk44C)[0] + 0x30), ((u8**)g_Menu->unk44C)[0][0x7B8]);
    AddPrim(&g_Menu->pGfxEnv->ot[4],
            (void*)(((u8**)g_Menu->unk44C)[0] + ((u8**)g_Menu->unk44C)[0][0x7B8] * 0x18));
    if (((u8*)g_Menu->pManager)[0x52] == 1) {
        value = *(s32*)(((u8**)g_Menu->unk44C)[0] + 0x7B0) + *(s32*)(((u8**)g_Menu->unk44C)[0] + 0x7B4);
        *(s32*)(((u8**)g_Menu->unk44C)[0] + 0x7B0) = value;
        if (value >= 0x101) {
            *(s32*)(((u8**)g_Menu->unk44C)[0] + 0x7B0) = 0;
        }
    }
}
*/
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
