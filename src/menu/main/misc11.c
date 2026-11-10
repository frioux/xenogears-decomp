#include "common.h"

#include "system/menu.h"

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E433C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4754);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E4928);
/* Best: 91.61% - instruction multiset identical, allocator puts the dividend in
   $a0 instead of $v0 (and the /120 magic in $v0 instead of $a1); 6 declaration
   orders and shapes leave the tie untouched.
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
/* Best: 87.97% - everything matches except that gcc folds the +0x5C8 into the
   member offset (0x5EC) where the original keeps addiu base,0x5C8 + 0x24(base),
   which also costs the mfhi destination registers ($v0/$v1 vs $a3). The first
   store has to be volatile so the following read is not forwarded; the second
   store must not be, or reorg cannot put it in the jr delay slot.
void func_801E4998(MenuDressingRoom* pDressingRoom, u8 gearId) {
    u16* p;
    u16 value;
    u32 hp;

    hp = g_GameState.gears[gearId].maxHp / 10 * 2 / 9;
    p = (u16*)(*(u32*)((u8*)pDressingRoom + 0x4C + gearId * 4) + 0x5C8);
    *(volatile u16*)((u8*)p + 0x24) = hp;
    value = *(volatile u16*)((u8*)p + 0x24) / 10;
    *(u16*)((u8*)p + 0x24) = value * 10;
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E5924);
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11", func_801E6544);
/* Best: 68.70% (same under gcc-2.6.0).  The transformation is an in-place
   compaction of each 16-byte block: 4 groups of (x[0], x[1]|x[2], x[3]) from
   x[0..3] go to p[3g..3g+2], stores ascending.  Every instruction matches once
   the base register does: retail keeps the loop pointer in $a2 and uses
   offsets 0..0xF, while gcc always invents a SECOND induction variable for the
   group (addu $6,$7,11 in the entry block, offsets -10..4 in the body) - it
   picks 0xB, the highest offset that is both read and written, as the new base.
   Reversing the statement order moves the secondary base to the last offset
   (+1), duplicating the self store does not change it, and |= for the OR sites
   changes nothing.  Volatile on the first store keeps the pair but splits $a3
   in two bases as well.
void func_801E6544(u8* p) {
    int i;

    for (i = 0; i < 0x10; i++, p += 0x10) {
        p[0x0] = p[0x0];
        p[0x1] = p[0x1] | p[0x2];
        p[0x2] = p[0x3];
        p[0x3] = p[0x4];
        p[0x4] = p[0x5] | p[0x6];
        p[0x5] = p[0x7];
        p[0x6] = p[0x8];
        p[0x7] = p[0x9] | p[0xA];
        p[0x8] = p[0xB];
        p[0x9] = p[0xC];
        p[0xA] = p[0xD] | p[0xE];
        p[0xB] = p[0xF];
    }
}
*/
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
