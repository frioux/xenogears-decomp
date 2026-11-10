#include "common.h"

#include "system/menu.h"

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E433C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4754);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4928);
/* Best: 91.61% - instruction multiset identical, only register assignment
   differs: retail has the u16 dividend in $v0, the /120 magic in $a1 and mfhi
   back into $v0 (which lets the scheduler hoist the magic's lui to the very
   first slot); gcc always puts the dividend in $a0, the magic in $v0 and mfhi
   in $a1.  Tried: pointer local vs inline indexing, u8* + 0xA4 byte arithmetic,
   a separate s32 diff local, 6 declaration orders.  Everything from
   `andi v0,v0,0xffff` to the epilogue already matches, so this is a local-alloc
   tie on the three pre-multu quantities, not a source shape.
u8 func_801E4928(u8 gearId) {
    GameGear* pGear;
    u_short weightClass;
    s16 value;

    pGear = &g_GameState.gears[gearId];
    weightClass = pGear->weightFromEquipment / 120;
    value = (weightClass - pGear->unk75) / 2;
    if (value < 0) {
        value = 0;
    }
    return value;
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4998);
/* Best: 93.61% (was 87.97% - the Folded offset was the whole problem).
   The +0x5C8 pointer offset must NOT be folded into the field offset: retail
   keeps addiu base,0x5C8 then a 0x24(base) access, and gcc only stops folding
   when the base is kept as an integer (u32) instead of a pointer type - with a
   u16* / u8* intermediate cse folds 0x5C8+0x24 into 0x5EC and drops the addiu.
   What is left is the same allocator tie as func_801E4928: retail puts every
   mfhi result in $v0 (and so can hoist the /10 magic's lui into slot 0), gcc
   always picks $a3.  Inlining the division into the store changes nothing.
   The first store is volatile so the following read is not forwarded; the
   second store must not be, or reorg cannot put it in the jr delay slot.
void func_801E4998(MenuDressingRoom* pDressingRoom, u8 gearId) {
    u32 p;
    u32 hp;
    u16 value;

    hp = g_GameState.gears[gearId].maxHp / 10 * 2 / 9;
    p = *(u32*)((u8*)pDressingRoom + 0x4C + gearId * 4);
    p = p + 0x5C8;
    *(volatile u16*)(p + 0x24) = hp;
    value = *(volatile u16*)(p + 0x24) / 10;
    *(u16*)(p + 0x24) = value * 10;
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4A28);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4D10);
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E5178);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E53CC);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E56E8);
void func_801C851C(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);

extern u16 D_801E9894[];
extern u16 D_801E9914[];

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
        func_801C851C((SVECTOR*)(pBase + 0x100), pX[0], pY[0], 0x10, 0x10);
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
        func_801C851C((SVECTOR*)(pBase + 0x120), pX[0], pY[0], 0x10, 0x10);
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E5B88);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E5E4C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E61B0);
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E6668);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E68AC);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);

void func_801E6AE8(u8 arg0, u8* pArg1) {
    u8* pPrims;
    u32 offset;

    offset = 0xA98 + arg0 * 0x87C;
    pPrims = (u8*)(offset + (u32)g_Menu->unk34C);
    func_8002675C(g_Menu->resources, (pArg1 + arg0)[0x1C] + 0x14E, pPrims,
                  g_Menu->renderContext, D_801EA004[arg0], D_801EA010[arg0], 0x1000);
}

void func_801C80B8(u32 number);

extern u32 D_801EA01C;
extern u32 D_801EA020;

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E6B70);
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

       func_801C80B8((pArg1 + arg0)[0x16]);
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
       func_801C80B8((pArg1 + arg0)[0x19]);
       ((u8*)g_Menu->unk34C)[arg0 * 0x87C + 0x1309] = 0;
   }
*/
