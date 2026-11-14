#include "common.h"

#include "system/menu.h"


// The save-slot record this unit writes: a packed copy of the parts of
// g_GameState that live in a save file.  Everything is word-aligned so the
// copies compile to lw/sw.
typedef struct {
    /* 0x00 */ u32 unk0[0x29];
} MenuSaveCharacter; // Size: 0xA4

typedef struct {
    /* 0x00 */ u32 unk0[0x8];
} MenuSaveGearHead20; // Size: 0x20

typedef struct {
    /* 0x00 */ u32 unk0[0x2];
} MenuSaveGearHead08; // Size: 0x08

typedef struct {
    /* 0x00 */ MenuSaveGearHead20 head;
    /* 0x20 */ MenuSaveGearHead08 headTail;
    /* 0x28 */ u32 unk28;
    /* 0x2C */ u32 unk2C;
    /* 0x30 */ u32 unk30;
    /* 0x34 */ u16 unk34;
    /* 0x36 */ u16 unk36;
    /* 0x38 */ u8 unk38;
    /* 0x39 */ u8 unk39;
    /* 0x3A */ u8 unk3A;
    /* 0x3B */ u8 unk3B;
} MenuSaveGear; // Size: 0x3C

typedef struct {
    /* 0x00 */ StringEntry entry[0xB];
} MenuSaveNamesA; // Size: 0xDC

typedef struct {
    /* 0x00 */ StringEntry entry[0x14];
} MenuSaveNamesB; // Size: 0x190

typedef struct {
    /* 0x00 */ u32 unk0[0x1E];
} MenuSaveBlob0x78; // Size: 0x78

typedef struct {
    /* 0x00 */ u32 unk0[0x58];
} MenuSaveBlob0x160; // Size: 0x160

typedef struct {
    /* 0x00 */ u32 unk0[0x40];
} MenuSaveBlob0x100; // Size: 0x100

typedef struct {
    /* 0x00 */ u8 unk0[0xA38];
} MenuSaveBlob0xA38; // Size: 0xA38

typedef struct {
    /* 0x0000 */ u32 unk0[0x9];
    /* 0x0024 */ MenuSaveNamesA namesA;
    /* 0x0100 */ MenuSaveNamesB namesB;
    /* 0x0290 */ MenuSaveCharacter characters[MAX_GAME_CHARACTERS];
    /* 0x099C */ MenuSaveGear gears[MAX_GAME_GEARS];
    /* 0x0E4C */ MenuSaveBlob0x78 unk1648;
    /* 0x0EC4 */ MenuSaveBlob0x160 unk16C0;
    /* 0x1024 */ MenuSaveBlob0x100 unk1820;
    /* 0x1124 */ MenuSaveBlob0xA38 unk1920;
} MenuSaveSlot; // Size: 0x1B5C

void func_801E4A28(MenuSaveSlot* pSlot) {
    GameGear* pGear;
    MenuSaveGear* pSaveGear;
    u8 i;
    s32 offset;
    u32 size;

    for (i = 0; i < MAX_GAME_CHARACTERS; i++) {
        offset = i * 0xA4 + 0x290;
        *(MenuSaveCharacter*)((mem_addr)pSlot + offset) =
            *(MenuSaveCharacter*)&g_GameState.characters[i];
    }
    for (i = 0; i < MAX_GAME_GEARS; i++) {
        s32 gearOffset = i * 0x3C + 0x99C;
        pSaveGear = (MenuSaveGear*)((mem_addr)pSlot + gearOffset);
        pGear = &g_GameState.gears[i];
        pSaveGear->head = *(MenuSaveGearHead20*)pGear;
        pSaveGear->headTail = *(MenuSaveGearHead08*)((u32*)pGear + 8);
        pSaveGear->unk28 = *(u32*)pGear->unk5C;
        pSaveGear->unk34 = pGear->fuel;
        pSaveGear->unk2C = pGear->hp;
        pSaveGear->unk38 = pGear->unk99;
        pSaveGear->unk39 = pGear->attackPower;
        pSaveGear->unk3A = pGear->unk75;
    }
    pSlot->namesA = *(MenuSaveNamesA*)((u32*)&g_GameState);
    pSlot->namesB = *(MenuSaveNamesB*)((u32*)&g_GameState + 0xDC / 4);
    pSlot->unk1648 = *(MenuSaveBlob0x78*)((u32*)&g_GameState + 0x1648 / 4);
    pSlot->unk16C0 = *(MenuSaveBlob0x160*)((u32*)&g_GameState + 0x16C0 / 4);
    pSlot->unk1820 = *(MenuSaveBlob0x100*)((u32*)&g_GameState + 0x1820 / 4);
    size = 0xA38;
    memcpy((u8*)pSlot + 0x1124, (u8*)&g_GameState + 0x1920, size);
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E4D10);
/* Best: 91.40% (840 vs 744 bytes), the inverse of func_801E4A28 (which
   matches): same record layout, same copy shapes, and the four
   MenuApplyGear* calls per gear land in the right order with s2 = i & 0xFF
   shared across them.  What is left is one allocator tie that swaps the two
   loop bases: retail keeps the g_GameState gear pointer in $s1 and the slot
   pointer in $s0, gcc gives $s1 to the slot pointer (the whole 0x28 head
   copy and the five field stores are mirror-imaged, which is why the byte
   count grows).  Tried: pGear/pSaveGear assignment in both orders, and the
   loop body with the head copies first (no change).  The 0x160 copy before
   the loop, the 0xDC/0x190/0x78/0x100 copies after it and the trailing
   memcpy of 0xA38 all match retail.
/*
void MenuApplyGearFrame(MenuDressingRoom*, u8);
void MenuApplyGearArmor(MenuDressingRoom*, u8);
void MenuApplyGearEngine(MenuDressingRoom*, u8);
void func_801E433C(MenuDressingRoom*, u8);

void func_801E4D10(MenuSaveSlot* pSlot, MenuDressingRoom* pDressingRoom) {
    GameGear* pGear;
    MenuSaveGear* pSaveGear;
    u8 i;
    u32 size;

    for (i = 0; i < MAX_GAME_CHARACTERS; i++) {
        s32 charOffset = i * 0xA4 + 0x290;
        *(MenuSaveCharacter*)&g_GameState.characters[i] =
            *(MenuSaveCharacter*)((mem_addr)pSlot + charOffset);
    }
    *(MenuSaveBlob0x160*)((u32*)&g_GameState + 0x16C0 / 4) = *(MenuSaveBlob0x160*)((u32*)pSlot + 0xEC4 / 4);
    for (i = 0; i < MAX_GAME_GEARS; i++) {
        s32 gearOffset = i * 0x3C + 0x99C;
        pGear = &g_GameState.gears[i];
        pSaveGear = (MenuSaveGear*)((mem_addr)pSlot + gearOffset);
        *(MenuSaveGearHead20*)pGear = pSaveGear->head;
        *(MenuSaveGearHead08*)((u32*)pGear + 8) = pSaveGear->headTail;
        *(u32*)pGear->unk5C = pSaveGear->unk28;
        MenuApplyGearFrame(pDressingRoom, i);
        MenuApplyGearArmor(pDressingRoom, i);
        MenuApplyGearEngine(pDressingRoom, i);
        func_801E433C(pDressingRoom, i);
        pGear->fuel = pSaveGear->unk34;
        pGear->hp = pSaveGear->unk2C;
        pGear->unk99 = pSaveGear->unk38;
        pGear->attackPower = pSaveGear->unk39;
        pGear->unk75 = pSaveGear->unk3A;
    }
    *(MenuSaveNamesA*)((u32*)&g_GameState) = pSlot->namesA;
    *(MenuSaveNamesB*)((u32*)&g_GameState + 0xDC / 4) = pSlot->namesB;
    *(MenuSaveBlob0x78*)((u32*)&g_GameState + 0x1648 / 4) = pSlot->unk1648;
    *(MenuSaveBlob0x100*)((u32*)&g_GameState + 0x1820 / 4) = pSlot->unk1820;
    size = 0xA38;
    memcpy((u8*)&g_GameState + 0x1920, (u8*)pSlot + 0x1124, size);
}
*/
void func_801E5058(void) {
    u8 i;

    for (i = 1; i < 0x48; i++) {
        g_GameState.weaponIDs[i] = i;
        g_GameState.weaponQuantities[i] = 10;
    }
    for (i = 1; i < 0x96; i++) {
        g_GameState.accessoryIDs[i] = i;
        g_GameState.accessoryQuantities[i] = 10;
    }
    for (i = 1; i < 0x4C; i++) {
        g_GameState.itemIDs[i + 2] = i;
        g_GameState.itemQuantities[i + 2] = 10;
    }
    for (i = 1; i < 0x48; i++) {
        g_GameState.gearWeaponsIds[i] = i;
        g_GameState.gearWeaponsQty[i] = 10;
    }
    for (i = 1; i < 0x69; i++) {
        g_GameState.gearPartsIds[i] = i;
        g_GameState.gearPartsQty[i] = 10;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E5178);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E53CC);
void func_801E927C(POLY_FT4* pPoly);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);

extern u16 D_801E9894[];
extern u16 D_801E9914[];

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E56E8);
/* Best: 90.78% - every instruction matches except ONE register: retail keeps
   the POLY_F4 colour 0x80 in $v0 with `li v0,0x80` INSIDE the loop, gcc's
   loop.c moves it out into a free call-saved register ($s8), which also swaps
   the $s6/$s7 assignment of `offset`/`i` and costs the extra $s8 save/restore
   (580 vs 576 bytes).  Structure confirmed correct: the FT4 biv is
   pBase + i * 0x28, the F4 biv is pBase + i * 0x18 with displacements 0xB4..,
   the `offset` counter (0xB0, +0x18) is shared by the SetPolyF4 and
   SetSemiTrans arguments, and the RECT is written y,x,h,w.  Tried: literal
   0x80 three times, a u8 local `shade = 0x80`, a walking POLY_F4* instead of
   the indexed form (89.28), -O3 (90.78, same), gcc-2.7.2-cdk (86.17).
   void func_801E56E8(s32 index) {
       u8* pBase;
       u16* pX;
       u16* pY;
       RECT rect;
       s32 offset;
       s32 i;

       pBase = (u8*)g_Menu->unk3A8[index];
       pX = &D_801E9894[index * 2];
       pY = &D_801E9914[index * 2];
       offset = 0xB0;
       for (i = 0; i < 2; i++) {
           func_801E927C((POLY_FT4*)(pBase + i * 0x28));
           ((POLY_FT4*)(pBase + i * 0x28))->x0 = pX[0];
           ((POLY_FT4*)(pBase + i * 0x28))->y0 = pY[0];
           ((POLY_FT4*)(pBase + i * 0x28))->x1 = pX[0] + 0x10;
           ((POLY_FT4*)(pBase + i * 0x28))->y1 = pY[0];
           ((POLY_FT4*)(pBase + i * 0x28))->x2 = pX[0];
           ((POLY_FT4*)(pBase + i * 0x28))->y2 = pY[0] + 0x10;
           ((POLY_FT4*)(pBase + i * 0x28))->x3 = pX[0] + 0x10;
           ((POLY_FT4*)(pBase + i * 0x28))->y3 = pY[0] + 0x10;
           ((POLY_FT4*)(pBase + i * 0x28))->tpage = GetTPage(0, 0, 0x140, 0x80);
           SetPolyF4((POLY_F4*)(pBase + offset));
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->r0 = 0x80;
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->g0 = 0x80;
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->b0 = 0x80;
           SetSemiTrans((POLY_F4*)(pBase + offset), 1);
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->x0 = pX[0];
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->y0 = pY[0];
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->x1 = pX[0] + 0x10;
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->y1 = pY[0];
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->x2 = pX[0];
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->y2 = pY[0] + 0x10;
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->x3 = pX[0] + 0x10;
           ((POLY_F4*)(pBase + i * 0x18 + 0xB0))->y3 = pY[0] + 0x10;
           offset += 0x18;
           MenuSetQuadVertices((SVECTOR*)(pBase + 0xE0), pX[0], pY[0], 0x10, 0x10);
       }
       rect.y = 0;
       rect.x = 0;
       rect.h = 0x100;
       rect.w = 0x100;
       SetDrawMode((DR_MODE*)(pBase + 0x140), 0, 0, GetTPage(0, 2, 0x140, 0x80), &rect);
       SetDrawMode((DR_MODE*)(pBase + 0x14C), 0, 0, GetTPage(0, 2, 0x140, 0x80), &rect);
   }
*/
void func_801E5924(s32 index) {
    u8* pBase;
    u16* pX;
    u16* pY;
    s32 offA;
    s32 offB;
    s32 i;

    pBase = (u8*)g_Menu->unk3A8[index];
    pX = &D_801E9894[index * 2];
    pY = &D_801E9914[index * 2];
    for (i = 0; i < 2; i++) {
        offA = i * 0x18 + 0x50;
        offB = i * 0x18 + 0x80;
        SetLineF3((LINE_F3*)(pBase + offA));
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->r0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->g0 = 0xFF;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->b0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->x0 = pX[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->y0 = pY[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->x1 = pX[0] + 0x10;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->y1 = pY[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->x2 = pX[0] + 0x10;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->y2 = pY[0] + 0x10;
        MenuSetQuadVertices((SVECTOR*)(pBase + 0x100), pX[0], pY[0], 0x10, 0x10);
        SetLineF3((LINE_F3*)(pBase + offB));
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->r0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->g0 = 0xFF;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->b0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->x0 = pX[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->y0 = pY[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->x1 = pX[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->y1 = pY[0] + 0x10;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->x2 = pX[0] + 0x10;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->y2 = pY[0] + 0x10;
        MenuSetQuadVertices((SVECTOR*)(pBase + 0x120), pX[0], pY[0], 0x10, 0x10);
    }
}
void func_801E5ACC(void) {
    void* pMem;
    int i;

    for (i = 0; i < 32; i++) {
        pMem = HeapAlloc(0x158, 0);
        g_Menu->unk3A8[i] = pMem;
        bzero(pMem, 0x158);
        func_801E56E8(i);
        func_801E5924(i);
    }
}
extern u16 D_801EA5D0[];
extern u32 D_801EA004[];
extern u32 D_801EA010[];
extern u8 D_801EA8C0;

void func_801E5B3C(void) {
    int i;

    for (i = 0; i < 32; i++) {
        HeapFree(g_Menu->unk3A8[i]);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E5B88);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E5E4C);
extern s32 D_801E9F98[];
extern s32 D_801E9FBC[];
extern s32 D_801EA494[];
extern u32 D_801EA590[];
extern u8 D_801EA5DC[];
extern u16 g_SystemPalette1;
void func_801E920C(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E61B0);
/* Best: 85.05% (656 vs 660 bytes).  Confirmed shape: outer loop is TWO
   counters in one for ("j = 0, offset = 0; offset < 0x1974; j++, offset +=
   0x87C") which is what makes retail test the 0x87C-scaled IV (s2) and keep
   four separate givs s2/s3/s4/s5 = offset, j*0x50, 0xA98+offset, j*4 with no
   register for j itself.  `primOffset` must be a real s32 local, otherwise
   gcc folds 0xA98 into 0x50 (0xAE8) at the prim address and into 0x820 at the
   two func_801E92{7C,0C} calls; and the prim pointer has to be a u8* local or
   the same fold moves 0x820 onto it.  *(u16*)D_801E9F98 / *(u16*)D_801E9FBC
   are needed to get retail's lhu (a plain D_801E9F98[0] gives lw).
   Left, four instructions: (1) retail's primOffset lands in $s4 and j*0x50 in
   $s3, ours swaps them; (2) retail emits "addiu s5,s5,4" (j++) right after
   the D_801EA5DC load and ours after the call; (3) the func_801E920C argument
   setup order differs (retail forms the 0x820 sum after loading base/ctx);
   (4) retail reads the byte it stores to 0x130E/0x1311 with "lbu 776(v1)",
   ours with "lw" - (u_char), (u8) and (s8) casts on g_Menu->renderContext all
   leave the lw.  Same allocator-tie class as func_801E56E8/func_801E68AC.
   void func_801E61B0(void) {
       s32 i;
       s32 j;
       s32 offset;
       s32 primOffset;
       u8* pPrim;

       for (j = 0, offset = 0; offset < 0x1974; j++, offset += 0x87C) {
           primOffset = offset + 0xA98;
           ((u8*)g_Menu->unk34C + offset)[0x1312] = 0;
           for (i = 0; i < 9; i++) {
               if (D_801EA494[i] != 0xFFFF) {
                   ((u8*)g_Menu->unk34C + offset)[0x1312] += func_8002675C(
                       g_Menu->resources, D_801EA494[i],
                       (POLY_FT4*)(primOffset + (u32)g_Menu->unk34C +
                                   (((u8*)g_Menu->unk34C + offset)[0x1312] * 0x50 + 0x50)),
                       g_Menu->renderContext, j * 0x50 + D_801E9F98[i], D_801E9FBC[i], 0x1000);
               }
           }
           ((u8*)g_Menu->unk34C + offset)[0x130E] = (u_char)g_Menu->renderContext;
           pPrim = (u8*)(primOffset + (u32)g_Menu->unk34C);
           func_801E927C((POLY_FT4*)(pPrim + (g_Menu->renderContext * 0x28 + 0x820)));
           *(u16*)((u8*)g_Menu->unk34C + (g_Menu->renderContext * 0x28 + offset) + 0x12CE) =
               GetTPage(0, 0, 0x180, 0);
           *(u16*)((u8*)g_Menu->unk34C + (g_Menu->renderContext * 0x28 + offset) + 0x12C6) =
               g_SystemPalette1;
           func_801E920C((POLY_FT4*)(pPrim + (g_Menu->renderContext * 0x28 + 0x820)),
                         *(u16*)D_801E9F98 + j * 0x50, *(u16*)D_801E9FBC + 7,
                         (u_char)(D_801EA590[j] << 2), D_801EA5DC[j * 4], 0x48, 0xD);
           ((u8*)g_Menu->unk34C + offset)[0x1311] = (u_char)g_Menu->renderContext;
       }
   }
*/
void func_801E6450(void) {
    void* pMem;
    u32 unk[2]; // unused: the original frame has 8 bytes of locals we cannot express

    pMem = HeapAlloc(0x2DC0, 0);
    g_Menu->unk34C = pMem;
    bzero(pMem, 0x2DC0);
    func_801E5B88();
    func_801E5E4C();
}
void func_801E649C(void) {
    void* pMem;
    MenuManager* pManager;

    pMem = g_Menu->unk34C;
    HeapFree(pMem);
    pManager = g_Menu->pManager;
    *(volatile u8*)&pManager->unkB = 0;
}
void func_801E64E0(void) {
    RECT rect;

    rect.x = 0x140;
    rect.y = 0xE0;
    rect.w = 0x40;
    rect.h = 0x20;
    ClearImage(&rect, 0, 0, 0);
    *(volatile u8*)&g_Menu->pManager->unkB = 0;
}
void func_801E6544(u8* p) {
    int i;

    for (i = 0; i < 0x10; i++) {
        p[i * 0x10 + 0x0] = p[i * 0x10 + 0x0];
        p[i * 0x10 + 0x1] = p[i * 0x10 + 0x1] | p[i * 0x10 + 0x2];
        p[i * 0x10 + 0x2] = p[i * 0x10 + 0x3];
        p[i * 0x10 + 0x3] = p[i * 0x10 + 0x4];
        p[i * 0x10 + 0x4] = p[i * 0x10 + 0x5] | p[i * 0x10 + 0x6];
        p[i * 0x10 + 0x5] = p[i * 0x10 + 0x7];
        p[i * 0x10 + 0x6] = p[i * 0x10 + 0x8];
        p[i * 0x10 + 0x7] = p[i * 0x10 + 0x9] | p[i * 0x10 + 0xA];
        p[i * 0x10 + 0x8] = p[i * 0x10 + 0xB];
        p[i * 0x10 + 0x9] = p[i * 0x10 + 0xC];
        p[i * 0x10 + 0xA] = p[i * 0x10 + 0xD] | p[i * 0x10 + 0xE];
        p[i * 0x10 + 0xB] = p[i * 0x10 + 0xF];
    }
}
s32 func_801E65E4(u8* p) {
    u8 hi;
    u8 lo;

    hi = p[0];
    lo = p[1];
    D_801EA8C0 = 1;
    if (hi < 0x80) {
        if (hi >= 0x20) {
            lo = *(u8*)&D_801EA5D0[hi];
            hi = D_801EA5D0[hi] >> 8;
            D_801EA8C0 = 0;
        } else {
            hi = 0x81;
            lo = 0x40;
            D_801EA8C0 = 0;
        }
    }
    return Krom2RawAdd((hi << 8) | lo);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E6668);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void MenuSetPlayTimeDigits(u32 value);

extern s32 D_801E9FE0[];
extern s32 D_801E9FE4[];

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E68AC);
/* Best: 91.16%.  Everything matches except gcc's loop.c hoists the loop's
   y constant 0x7A out into a free call-saved register ($s6, adding its
   save/restore and shifting the frame), where retail re-materialises it
   with "li v0,0x7A" every iteration; that also flips the addu operand
   order of the loop's prim pointer (retail "addu a2,s0,a2").  Same wall as
   func_801E56E8 below.  Structure confirmed: x/y tables D_801E9FE0[0],
   D_801E9FE4[0] then D_801E9FE0[i+2] in a 7-iteration loop, prim pointers
   0x240C/0x24AC/0x254C+i*0x50 off g_Menu->unk34C, then 0x2C7C, 0x2CCC,
   0x2D1C=(number/10), 0x2D6C=(number%10) with number = pArg[0x23]+1.
   Tried: an explicit u32 offset accumulator (loop addu becomes
   "addu a2,s0,a2" but gcc spills s7 and the frame grows to 0x48, 87.42%).
   void func_801E68AC(u8* pArg) {
       s32 number;
       s32 i;

       func_8002675C(g_Menu->resources, 0xEE,
                     (POLY_FT4*)((u8*)g_Menu->unk34C + 0x240C),
                     g_Menu->renderContext, D_801E9FE0[0], 0x7A, 0x1000);
       func_8002675C(g_Menu->resources, 0xEE,
                     (POLY_FT4*)((u8*)g_Menu->unk34C + 0x24AC),
                     g_Menu->renderContext, D_801E9FE4[0], 0x7A, 0x1000);
       MenuSetPlayTimeDigits(*(u32*)pArg);
       for (i = 0; i < 7; i++) {
           func_8002675C(g_Menu->resources, ((u32*)g_Menu->unk2EC)[i],
                         (POLY_FT4*)((u8*)g_Menu->unk34C + 0x254C + i * 0x50),
                         g_Menu->renderContext, D_801E9FE0[i + 2], 0x7A,
                         0x1000);
       }
       func_8002675C(g_Menu->resources, 0x17,
                     (POLY_FT4*)((u8*)g_Menu->unk34C + 0x2C7C),
                     g_Menu->renderContext, 0x8, 0x66, 0x1000);
       func_8002675C(g_Menu->resources, 0x32,
                     (POLY_FT4*)((u8*)g_Menu->unk34C + 0x2CCC),
                     g_Menu->renderContext, 0x10, 0x66, 0x1000);
       number = pArg[0x23] + 1;
       func_8002675C(g_Menu->resources, number / 10,
                     (POLY_FT4*)((u8*)g_Menu->unk34C + 0x2D1C),
                     g_Menu->renderContext, 0x10, 0x6E, 0x1000);
       number = pArg[0x23] + 1;
       func_8002675C(g_Menu->resources, number % 10,
                     (POLY_FT4*)((u8*)g_Menu->unk34C + 0x2D6C),
                     g_Menu->renderContext, 0x18, 0x6E, 0x1000);
   }
*/


void func_801E6AE8(u8 arg0, u8* pArg1) {
    u8* pPrims;
    u32 offset;

    offset = 0xA98 + arg0 * 0x87C;
    pPrims = (u8*)(offset + (u32)g_Menu->unk34C);
    func_8002675C(g_Menu->resources, (pArg1 + arg0)[0x1C] + 0x14E, pPrims,
                  g_Menu->renderContext, D_801EA004[arg0], D_801EA010[arg0], 0x1000);
}

void MenuParseNumberToDigits(u32 number);

extern u32 D_801EA01C;
extern u32 D_801EA020;

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E6B70);
/* Best: 89.55% (was 87.42% with a u8 loop counter).  Shape is right: the
   counter byte is g_Menu[0x322 + i], the memset target g_Menu->unk34C + arg0 *
   0x87C + 0x1308 and the prim pointer 0xA98 + arg0 * 0x87C + unk34C +
   counter * 0x50 + 0x320.  What is left, all allocator ties:
   (1) retail emits "addu v1,v1,s1" (base first) where gcc always emits
   "addu v1,s1,v1" at all three unk34C+offset sites - flipping the C operand
   order and casting through (u8*) changes nothing;
   (2) retail re-materialises arg0 * 0x87C at the tail instead of reusing s1
   (an explicit u32 offset local makes gcc reuse it, so the source repeats the
   multiply inline);
   (3) retail stores the x stack argument and 0x1000 to 16(sp)/24(sp) before
   it forms the prim pointer; gcc sinks both.
   void func_801E6B70(u8 arg0, u8* pArg1) {
       int i;

       MenuParseNumberToDigits((pArg1 + arg0)[0x16]);
       ((u8*)g_Menu->unk34C)[arg0 * 0x87C + 0x1308] = 0;
       for (i = 0; i < 3; i++) {
           if (((u8*)g_Menu)[i + 0x322] == 0xFF) {
               continue;
           }
           ((u8*)g_Menu->unk34C)[arg0 * 0x87C + 0x1308] += func_8002675C(
               g_Menu->resources, ((u8*)g_Menu)[i + 0x322],
               (POLY_FT4*)(0xA98 + arg0 * 0x87C + (u32)g_Menu->unk34C +
                           (((u8*)g_Menu->unk34C)[arg0 * 0x87C + 0x1308] * 0x50 + 0x320)),
               g_Menu->renderContext, D_801EA01C + arg0 * 0x50 + i * 8,
               D_801EA020, 0x1000);
       }
       MenuParseNumberToDigits((pArg1 + arg0)[0x19]);
       ((u8*)g_Menu->unk34C)[arg0 * 0x87C + 0x1309] = 0;
   }
*/
