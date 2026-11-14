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

void MenuSetupPointerCursors(s32 arg0);
void MenuFreeScrollHandle(void);
void MenuFreeWindow(s32 arg0);
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
void MenuCloseCardEvents(void);
void MenuRenderFrame(void);
u_char func_801C93A8(void);
void func_801D2F4C(s32 arg0);
void MenuFreeWindow2Strings(void);
void func_801E8EAC(POLY_FT4* pPoly, u_char mode);
void func_801E8F60(s32 mode, u_char index);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
void func_801E91C4(P_TAG* pTag);

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

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801D9F98);

void func_801DA4A8(void) {
    MenuUnk9* pMenu;

    MenuSetupPointerCursors(2);
    func_801E8018(8, g_Menu->unk10E0, D_801EA548, g_Menu->pManager->unk38);
    pMenu = HeapAlloc(0x1198, 0);
    g_Menu->unk42C = pMenu;
    bzero(pMenu, 0x1198);
    func_801C72BC(0);
}

void func_801DA518(void) {
    MenuFreeScrollHandle();
    MenuFreeWindow(3);
    MenuFreeWindow(4);
    g_Menu->pManager->unk48 = 0;
    func_801C72BC(0x10);
    HeapFree(g_Menu->unk42C->pVramBuffer);
    HeapFree(g_Menu->unk42C);
    HeapFree(g_Menu->pDressingRoom->pItemsData);
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DA5BC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DA9A8);

void func_801DB02C(u_char index) {
    g_Menu->arrowCursors[index] = HeapAlloc(0x78, 0);
    bzero(g_Menu->arrowCursors[index], 0x78);
    g_Menu->arrowCursors[index]->curAnimFrame = 4;
    g_Menu->arrowCursors[index]->animFrameDuration = 0;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DB0A8);

/* 98.80%. Only difference: the MenuSetQuadVertices argument setup. Original emits
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
           MenuSetQuadVertices(pCursor->vertices, x, y, lx1 - lx0, ly1 - ly0);
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DB39C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc8_r2", func_801DB5E4);
