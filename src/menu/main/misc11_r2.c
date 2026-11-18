#include "common.h"

#include "system/menu.h"


// Quantity MenuFillInventoryTables gives every filled slot.
#define MENU_INVENTORY_FILL_QUANTITY 10

// Byte sizes of the two menu scratch buffers this unit allocates: one prim block
// per character cell (g_Menu->pCellPrimBlocks) and g_Menu->pPrimBuffer.
#define MENU_CELL_PRIM_BLOCK_SIZE 344
#define MENU_PRIM_BUFFER_SIZE 11712

// Pixel size of one character cell: the quad extent and the vertex offsets
// MenuInitializeCellLinePrims writes.
#define MENU_CELL_SIZE 16

// VRAM rect of the glyph texture MenuClearGlyphTexture clears: 64x32 VRAM words,
// the 0x1000-byte 4bpp bitmap the glyphs are rasterised into.
#define MENU_GLYPH_TEXTURE_X 0x140
#define MENU_GLYPH_TEXTURE_Y 0xE0
#define MENU_GLYPH_TEXTURE_WIDTH 64
#define MENU_GLYPH_TEXTURE_HEIGHT 32

// Digits MenuDrawSlotResourceSprites draws of each of the two slot stats it
// formats: MenuParseNumberToDigits leaves the value in the tail of
// g_Menu->digits, any leading digit marked MENU_DIGIT_NONE.
#define MENU_SLOT_STAT_DIGIT_COUNT 3


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
    /* 0x2C */ u32 hp;
    /* 0x30 */ u32 unk30;
    /* 0x34 */ u16 fuel;
    /* 0x36 */ u16 unk36;
    /* 0x38 */ u8 unk38;
    /* 0x39 */ u8 attackPower;
    /* 0x3A */ u8 baseWeightClass;
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

// Cell position in the menu glyph atlas: 21 cells per row, the row's y in
// D_801E99E8.
typedef struct {
    /* 0x0 */ u16 coord;
    /* 0x2 */ u16 unk2;
} MenuGlyphCellPos; // Size: 0x4

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

void MenuSaveSlotWriteGameState(MenuSaveSlot* pSlot) {
    GameGear* pGear;
    MenuSaveGear* pSaveGear;
    u8 i;
    s32 offset;
    u32 size;

    for (i = 0; i < MAX_GAME_CHARACTERS; i++) {
        offset = i * sizeof(MenuSaveCharacter) + 0x290;
        *(MenuSaveCharacter*)((mem_addr)pSlot + offset) =
            *(MenuSaveCharacter*)&g_GameState.characters[i];
    }
    for (i = 0; i < MAX_GAME_GEARS; i++) {
        s32 gearOffset = i * sizeof(MenuSaveGear) + 0x99C;
        pSaveGear = (MenuSaveGear*)((mem_addr)pSlot + gearOffset);
        pGear = &g_GameState.gears[i];
        pSaveGear->head = *(MenuSaveGearHead20*)pGear;
        pSaveGear->headTail = *(MenuSaveGearHead08*)((u32*)pGear + 8);
        pSaveGear->unk28 = *(u32*)pGear->unk5C;
        pSaveGear->fuel = pGear->fuel;
        pSaveGear->hp = pGear->hp;
        pSaveGear->unk38 = pGear->unk99;
        pSaveGear->attackPower = pGear->attackPower;
        pSaveGear->baseWeightClass = pGear->baseWeightClass;
    }
    pSlot->namesA = *(MenuSaveNamesA*)((u32*)&g_GameState);
    pSlot->namesB = *(MenuSaveNamesB*)((u32*)&g_GameState + 0xDC / 4);
    pSlot->unk1648 = *(MenuSaveBlob0x78*)((u32*)&g_GameState + 0x1648 / 4);
    pSlot->unk16C0 = *(MenuSaveBlob0x160*)((u32*)&g_GameState + 0x16C0 / 4);
    pSlot->unk1820 = *(MenuSaveBlob0x100*)((u32*)&g_GameState + 0x1820 / 4);
    size = sizeof(MenuSaveBlob0xA38);
    memcpy((u8*)pSlot + 0x1124, (u8*)&g_GameState + 0x1920, size);
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E4D10);
/* Best: 91.40% (840 vs 744 bytes), the inverse of MenuSaveSlotWriteGameState (which
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
void MenuApplyGearAccessories(MenuDressingRoom*, u8);

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
        MenuApplyGearAccessories(pDressingRoom, i);
        pGear->fuel = pSaveGear->fuel;
        pGear->hp = pSaveGear->hp;
        pGear->unk99 = pSaveGear->unk38;
        pGear->attackPower = pSaveGear->attackPower;
        pGear->baseWeightClass = pSaveGear->baseWeightClass;
    }
    *(MenuSaveNamesA*)((u32*)&g_GameState) = pSlot->namesA;
    *(MenuSaveNamesB*)((u32*)&g_GameState + 0xDC / 4) = pSlot->namesB;
    *(MenuSaveBlob0x78*)((u32*)&g_GameState + 0x1648 / 4) = pSlot->unk1648;
    *(MenuSaveBlob0x100*)((u32*)&g_GameState + 0x1820 / 4) = pSlot->unk1820;
    size = 0xA38;
    memcpy((u8*)&g_GameState + 0x1920, (u8*)pSlot + 0x1124, size);
}
*/
void MenuFillInventoryTables(void) {
    u8 i;

    for (i = 1; i < MENU_WEAPON_ID_COUNT; i++) {
        g_GameState.weaponIDs[i] = i;
        g_GameState.weaponQuantities[i] = MENU_INVENTORY_FILL_QUANTITY;
    }
    for (i = 1; i < MENU_ACCESSORY_ID_COUNT; i++) {
        g_GameState.accessoryIDs[i] = i;
        g_GameState.accessoryQuantities[i] = MENU_INVENTORY_FILL_QUANTITY;
    }
    for (i = 1; i < MENU_ITEM_ID_COUNT; i++) {
        g_GameState.itemIDs[i + 2] = i;
        g_GameState.itemQuantities[i + 2] = MENU_INVENTORY_FILL_QUANTITY;
    }
    for (i = 1; i < MENU_GEAR_WEAPON_ID_COUNT; i++) {
        g_GameState.gearWeaponsIds[i] = i;
        g_GameState.gearWeaponsQty[i] = MENU_INVENTORY_FILL_QUANTITY;
    }
    for (i = 1; i < MENU_GEAR_PART_ID_COUNT; i++) {
        g_GameState.gearPartsIds[i] = i;
        g_GameState.gearPartsQty[i] = MENU_INVENTORY_FILL_QUANTITY;
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E5178);
/* Best: 94.34% (592 bytes, same size as retail).  The 66 stores are in retail order
   and the field widths are right (four s16 at +0/+2/+4/+6, u8 at +0x17, u16 at
   +0x1A; eleven records of 0x20 bytes at g_GameState+0x16C0).  What is left is only
   the placement of the constant loads: retail emits the li for a constant several
   statements before its first store (li a3,0xFF00 and li v1,0xFFF0 ride in front of
   the 0x16C0 store and are first used at 0x16C2/0x16C4) and the allocator swaps my
   a0/a1 pair for 7 and 0xFFC0; the store sequence itself matches instruction for
   instruction.  The table overlaps the header fields unk16BC[10] and unk17FC.
   // Per-gear stat limits reset by func_801E5178.  The table lives at
   // g_GameState+0x16C0, eleven records of 0x20 bytes.
   typedef struct {
       // 0x00 u16 unk0;
       // 0x02 u16 unk2;
       // 0x04 u16 unk4;
       // 0x06 u16 unk6;
       // 0x08 u8 pad8[0xF];
       // 0x17 u8 unk17;
       // 0x18 u8 pad18[2];
       // 0x1A u16 unk1A;
       // 0x1C u8 pad1C[4];
   } MenuGearLimits; // Size: 0x20

   void func_801E5178(void) {
       MenuGearLimits* pTable;

       pTable = (MenuGearLimits*)((u8*)&g_GameState + 0x16C0);
       g_GameState.characterFlags = 0x7FF;
       pTable[0].unk0 = 0xFFF8;
       pTable[0].unk6 = 0xFE00;
       pTable[1].unk1A = 0xC000;
       pTable[0].unk2 = 0xFF00;
       pTable[0].unk4 = 0xFFF0;
       pTable[0].unk1A = 0xE000;
       pTable[0].unk17 = 0x7;
       pTable[1].unk0 = 0xFFE0;
       pTable[1].unk2 = 0xFFF0;
       pTable[1].unk4 = 0xFFF0;
       pTable[1].unk6 = 0xFFF0;
       pTable[1].unk17 = 0x7;
       pTable[2].unk0 = 0xFFE0;
       pTable[2].unk2 = 0xFFE0;
       pTable[2].unk4 = 0xFFF0;
       pTable[2].unk6 = 0xFF00;
       pTable[2].unk1A = 0x8000;
       pTable[2].unk17 = 0x7;
       pTable[3].unk0 = 0xFFE0;
       pTable[3].unk2 = 0xFFC0;
       pTable[3].unk4 = 0xFFF0;
       pTable[3].unk6 = 0xFF00;
       pTable[3].unk1A = 0xC000;
       pTable[3].unk17 = 0x7;
       pTable[4].unk0 = 0xFFC0;
       pTable[4].unk2 = 0xFFC0;
       pTable[4].unk4 = 0xFFF0;
       pTable[4].unk6 = 0xFFC0;
       pTable[4].unk1A = 0xE000;
       pTable[4].unk17 = 0x7;
       pTable[5].unk0 = 0xFFC0;
       pTable[5].unk2 = 0xC000;
       pTable[5].unk4 = 0xFFF0;
       pTable[5].unk6 = 0xC000;
       pTable[5].unk1A = 0x8000;
       pTable[5].unk17 = 0x7;
       pTable[6].unk0 = 0xFFC0;
       pTable[6].unk2 = 0xFF00;
       pTable[6].unk4 = 0xFFF0;
       pTable[6].unk6 = 0xFF00;
       pTable[6].unk1A = 0x8000;
       pTable[6].unk17 = 0x7;
       pTable[7].unk0 = 0x0;
       pTable[7].unk2 = 0xFF00;
       pTable[7].unk4 = 0x0;
       pTable[7].unk6 = 0xFF00;
       pTable[7].unk1A = 0x0;
       pTable[7].unk17 = 0x7;
       pTable[8].unk0 = 0x0;
       pTable[8].unk2 = 0xF800;
       pTable[8].unk4 = 0xFFF0;
       pTable[8].unk6 = 0x0;
       pTable[8].unk1A = 0xE000;
       pTable[8].unk17 = 0x7;
       pTable[9].unk0 = 0xFFE0;
       pTable[9].unk2 = 0xFFE0;
       pTable[9].unk4 = 0xFFF0;
       pTable[9].unk6 = 0xFFE0;
       pTable[9].unk1A = 0x8000;
       pTable[9].unk17 = 0x7;
       pTable[10].unk0 = 0xFFC0;
       pTable[10].unk2 = 0xFF00;
       pTable[10].unk4 = 0xFFF0;
       pTable[10].unk6 = 0xFF00;
       pTable[10].unk1A = 0x8000;
       pTable[10].unk17 = 0x7;
   }
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E53CC);
void MenuInitializePolyFt4(POLY_FT4* pPoly);
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);

extern u16 g_MenuCellPrimXs[];
extern u16 g_MenuCellPrimYs[];

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

       pBase = (u8*)g_Menu->pCellPrimBlocks[index];
       pX = &g_MenuCellPrimXs[index * 2];
       pY = &g_MenuCellPrimYs[index * 2];
       offset = 0xB0;
       for (i = 0; i < 2; i++) {
           MenuInitializePolyFt4((POLY_FT4*)(pBase + i * 0x28));
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
void MenuInitializeCellLinePrims(s32 index) {
    u8* pBase;
    u16* pX;
    u16* pY;
    s32 offA;
    s32 offB;
    s32 i;

    pBase = (u8*)g_Menu->pCellPrimBlocks[index];
    pX = &g_MenuCellPrimXs[index * 2];
    pY = &g_MenuCellPrimYs[index * 2];
    for (i = 0; i < 2; i++) {
        offA = i * 0x18 + 0x50;
        offB = i * 0x18 + 0x80;
        SetLineF3((LINE_F3*)(pBase + offA));
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->r0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->g0 = 0xFF;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->b0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->x0 = pX[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->y0 = pY[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->x1 = pX[0] + MENU_CELL_SIZE;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->y1 = pY[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->x2 = pX[0] + MENU_CELL_SIZE;
        ((LINE_F3*)(pBase + i * 0x18 + 0x50))->y2 = pY[0] + MENU_CELL_SIZE;
        MenuSetQuadVertices((SVECTOR*)(pBase + 0x100), pX[0], pY[0], MENU_CELL_SIZE, MENU_CELL_SIZE);
        SetLineF3((LINE_F3*)(pBase + offB));
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->r0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->g0 = 0xFF;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->b0 = 0;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->x0 = pX[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->y0 = pY[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->x1 = pX[0];
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->y1 = pY[0] + MENU_CELL_SIZE;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->x2 = pX[0] + MENU_CELL_SIZE;
        ((LINE_F3*)(pBase + i * 0x18 + 0x80))->y2 = pY[0] + MENU_CELL_SIZE;
        MenuSetQuadVertices((SVECTOR*)(pBase + 0x120), pX[0], pY[0], MENU_CELL_SIZE, MENU_CELL_SIZE);
    }
}
void MenuCreateCellPrimBlocks(void) {
    void* pMem;
    int i;

    for (i = 0; i < (s32)ARRAY_COUNT(g_Menu->pCellPrimBlocks); i++) {
        pMem = HeapAlloc(MENU_CELL_PRIM_BLOCK_SIZE, 0);
        g_Menu->pCellPrimBlocks[i] = pMem;
        bzero(pMem, MENU_CELL_PRIM_BLOCK_SIZE);
        func_801E56E8(i);
        MenuInitializeCellLinePrims(i);
    }
}
extern u16 D_801EA5D0[];
extern u32 g_MenuSlotSpriteX[];
extern u32 g_MenuSlotSpriteY[];
extern u8 g_MenuGlyphIsDoubleByte;

void MenuFreeCellPrimBlocks(void) {
    int i;

    for (i = 0; i < (s32)ARRAY_COUNT(g_Menu->pCellPrimBlocks); i++) {
        HeapFree(g_Menu->pCellPrimBlocks[i]);
    }
}
extern MenuGlyphCellPos D_801E9994[0x15];
extern MenuGlyphCellPos D_801E99E8[2];

INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E5B88);
/* Park: 99.94% under the unit preset (708 bytes = retail's size, the instruction
   stream is identical word for word except ONE mfhi destination: retail has
   "mfhi a1 / sra a1,a1,0x2", gcc 2.7.2 emits "mfhi t0 / sra a1,t0,0x2".  Every other
   register, relocation and delay slot already agrees, so this is not a source shape:
   gcc's local-alloc coalesces the multiply-high temp with the quotient pseudo only in
   2.6.0.  The body below scores 100.00 (byte-identical) under gcc 2.6.0 with the
   Default flag set; two permuter runs (1136 and 2017 mutants) and every spelling of
   the divide (i % 0x15 via CSE, col = i - row * 0x15, row/col locals) leave 2.7.2 at
   99.94.
   FIX: a one-function preset split.  In config/menu.yaml replace
   [0x1FA28, c, main/misc11_r2] with
   [0x1FA28, c, main/misc11_r2], [0x20B88, c, main/misc11_r2_260],
   [0x20E4C, c, main/misc11_r2]
   and put the body below in src/menu/main/misc11_r2_260.c on the XenoLibrary preset
   (gcc 2.6.0, Default flags).  It cannot be a whole-unit switch: under 2.6.0
   MenuFillInventoryTables drops to 93.06 and MenuDrawSlotResourceSprites to 99.39,
   every other C function of the unit stays 100.00.
   Shape: nested loops i = 0..0x1F, j = 0..1, prim at g_Menu->pPrimBuffer + (i * 2 + j) * 0x28;
   the atlas cell is D_801E9994[i % 0x15] plus the row offset (i / 0x15) * 8 in x and
   D_801E99E8[i / 0x15] in y (both tables have to be pointer locals, the plain array form
   folds the address through $at); uv from (i % 0x10) * 0x10 and (i / 0x10) * 0x10 - 0x20.
   `i = 0` FIRST in the outer for-init fixes the prologue save order, and carrying
   (i * 2 + j) * 0x28 through a separate temp before assigning the s32 offset is what
   gives retail's s0.  The prim address is re-formed from g_Menu->pPrimBuffer at every store
   (retail reloads 0x34C each time), so it must NOT be a pointer local.

void func_801E5B88(void) {
    MenuGlyphCellPos* pCellX;
    MenuGlyphCellPos* pCellY;
    s32 offsetTmp;
    s32 offset;
    s32 row;
    s32 col;
    s32 i;
    s32 j;

    for (i = 0, pCellX = D_801E9994, pCellY = D_801E99E8; i < 0x20; i++) {
        for (j = 0; j < 2; j++) {
            offsetTmp = (i * 2 + j) * 0x28;
            offset = offsetTmp;
            MenuInitializePolyFt4((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer));
            row = i / 0x15;
            col = i - row * 0x15;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->x0 =
                pCellX[col].coord + row * 8;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->y0 = pCellY[row].coord;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->x1 =
                pCellX[col].coord + row * 8 + 0xC;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->y1 = pCellY[row].coord;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->x2 =
                pCellX[col].coord + row * 8;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->y2 = pCellY[row].coord + 0x10;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->x3 =
                pCellX[col].coord + row * 8 + 0xC;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->y3 = pCellY[row].coord + 0x10;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->u0 = (i % 0x10) * 0x10;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->v0 = (i / 0x10) * 0x10 - 0x20;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->u1 = (i % 0x10) * 0x10 + 0xC;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->v1 = (i / 0x10) * 0x10 - 0x20;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->u2 = (i % 0x10) * 0x10;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->v2 = (i / 0x10) * 0x10 - 0x11;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->u3 = (i % 0x10) * 0x10 + 0xC;
            ((POLY_FT4*)(offset + (u32)g_Menu->pPrimBuffer))->v3 = (i / 0x10) * 0x10 - 0x11;
            ((POLY_FT4*)((u8*)g_Menu->pPrimBuffer + offset))->tpage = GetTPage(0, 0, 0x140, 0x80);
            ((POLY_FT4*)((u8*)g_Menu->pPrimBuffer + offset))->clut = GetClut(0, 0x1C0);
        }
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E5E4C);
extern s32 D_801E9F98[];
extern s32 D_801E9FBC[];
extern s32 D_801EA494[];
extern u32 g_MenuSlotNameTextureX[];
extern u8 g_MenuSlotNameTextureY[];
extern u16 g_SystemPalette1;
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);

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
   the g_MenuSlotNameTextureY load and ours after the call; (3) the MenuSetFt4QuadVertices argument
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
           ((u8*)g_Menu->pPrimBuffer + offset)[0x1312] = 0;
           for (i = 0; i < 9; i++) {
               if (D_801EA494[i] != 0xFFFF) {
                   ((u8*)g_Menu->pPrimBuffer + offset)[0x1312] += func_8002675C(
                       g_Menu->resources, D_801EA494[i],
                       (POLY_FT4*)(primOffset + (u32)g_Menu->pPrimBuffer +
                                   (((u8*)g_Menu->pPrimBuffer + offset)[0x1312] * 0x50 + 0x50)),
                       g_Menu->renderContext, j * 0x50 + D_801E9F98[i], D_801E9FBC[i], 0x1000);
               }
           }
           ((u8*)g_Menu->pPrimBuffer + offset)[0x130E] = (u_char)g_Menu->renderContext;
           pPrim = (u8*)(primOffset + (u32)g_Menu->pPrimBuffer);
           MenuInitializePolyFt4((POLY_FT4*)(pPrim + (g_Menu->renderContext * 0x28 + 0x820)));
           *(u16*)((u8*)g_Menu->pPrimBuffer + (g_Menu->renderContext * 0x28 + offset) + 0x12CE) =
               GetTPage(0, 0, 0x180, 0);
           *(u16*)((u8*)g_Menu->pPrimBuffer + (g_Menu->renderContext * 0x28 + offset) + 0x12C6) =
               g_SystemPalette1;
           MenuSetFt4QuadVertices((POLY_FT4*)(pPrim + (g_Menu->renderContext * 0x28 + 0x820)),
                         *(u16*)D_801E9F98 + j * 0x50, *(u16*)D_801E9FBC + 7,
                         (u_char)(g_MenuSlotNameTextureX[j] << 2), g_MenuSlotNameTextureY[j * 4], 0x48, 0xD);
           ((u8*)g_Menu->pPrimBuffer + offset)[0x1311] = (u_char)g_Menu->renderContext;
       }
   }
*/
void MenuAllocatePrimBuffer(void) {
    void* pMem;
    u32 unk[2]; // unused: the original frame has 8 bytes of locals we cannot express

    pMem = HeapAlloc(MENU_PRIM_BUFFER_SIZE, 0);
    g_Menu->pPrimBuffer = pMem;
    bzero(pMem, MENU_PRIM_BUFFER_SIZE);
    func_801E5B88();
    func_801E5E4C();
}
void MenuFreePrimBuffer(void) {
    void* pMem;
    MenuManager* pManager;

    pMem = g_Menu->pPrimBuffer;
    HeapFree(pMem);
    pManager = g_Menu->pManager;
    *(volatile u8*)&pManager->characterWindowBuilt = 0;
}
void MenuClearGlyphTexture(void) {
    RECT rect;

    rect.x = MENU_GLYPH_TEXTURE_X;
    rect.y = MENU_GLYPH_TEXTURE_Y;
    rect.w = MENU_GLYPH_TEXTURE_WIDTH;
    rect.h = MENU_GLYPH_TEXTURE_HEIGHT;
    ClearImage(&rect, 0, 0, 0);
    *(volatile u8*)&g_Menu->pManager->characterWindowBuilt = 0;
}
void MenuCompressGlyphRows(u8* p) {
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
s32 MenuGetGlyphKromAddress(u8* p) {
    u8 hi;
    u8 lo;

    hi = p[0];
    lo = p[1];
    g_MenuGlyphIsDoubleByte = 1;
    if (hi < 0x80) {
        if (hi >= 0x20) {
            lo = *(u8*)&D_801EA5D0[hi];
            hi = D_801EA5D0[hi] >> 8;
            g_MenuGlyphIsDoubleByte = 0;
        } else {
            hi = 0x81;
            lo = 0x40;
            g_MenuGlyphIsDoubleByte = 0;
        }
    }
    return Krom2RawAdd((hi << 8) | lo);
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc11_r2", func_801E6668);
/* Best: 98.90% (584 bytes, same size as retail).  Decoded shape: index selects a
   0x200-byte MenuUnk2ImageChunk; the 0x40-entry byte table at chunk+4 is walked one
   entry per glyph (two bytes when MenuGetGlyphKromAddress sets g_MenuGlyphIsDoubleByte), each entry is
   unpacked to 0x100 bits, MenuCompressGlyphRows compresses a row, and the bits are OR ed into
   a 0x1000-byte 4bpp texture at (i/16)*0x400 + (i%16)*4 + row*0x40 + col/4 before
   LoadImage of the 0x140,0xE0 RECT.  Everything matches except ONE insn: retail
   keeps addiu a0,sp,16 (the RECT address) in BOTH branch delay slots that reach the
   epilogue and has none at the head of the epilogue block; gcc keeps it in the
   epilogue block and fills only the loop-top beqz slot.  Tried: RECT declared
   before/after the loops, one shared col variable for the bit loops and the repack
   column (that fixed a v1/a0 register swap and took it from 98.34 to 98.90), the
   s32-address form (adds a move a0,v1), LoadImage args reordered.
   void func_801E6668(s32 index) {
       u8* pGlyphBits;
       u8* pBits;
       u8* pTexture;
       u8* pSrc;
       u16* pGlyph;
       RECT rect;
       s32 i;
       s32 count;
       s32 row;
       s32 col;

       pBits = HeapAlloc(0x100, 1);
       pTexture = HeapAlloc(0x1000, 1);
       bzero(pTexture, 0x1000);
       count = 0;
       pSrc = &g_Menu->menuUnk2->chunks[index].pad0[4];
       for (i = 0; i < 0x20; i++) {
           if (*pSrc == 0) {
               break;
           }
           pGlyphBits = pBits;
           pGlyph = (u16*)MenuGetGlyphKromAddress(pSrc);
           if (pGlyph != (u16*)-1) {
               for (row = 0; row < 0x10; row++) {
                   for (col = 7; col >= 0; col--) {
                       *pGlyphBits++ = (*pGlyph >> col) & 1;
                   }
                   for (col = 0xF; col >= 8; col--) {
                       *pGlyphBits++ = (*pGlyph >> col) & 1;
                   }
                   pGlyph++;
               }
               MenuCompressGlyphRows(pBits);
               for (row = 0; row < 0x10; row++) {
                   for (col = 0; col < 0xC; col++) {
                       ((u16*)pTexture)[row * 0x40 + col / 4 + (i % 0x10) * 4 +
                                         (i / 0x10) * 0x400] |=
                           pBits[row * 0x10 + col] << ((col % 4) * 4);
                   }
               }
           }
           pSrc++;
           count++;
           if (g_MenuGlyphIsDoubleByte != 0) {
               pSrc++;
               count++;
           }
           if (count >= 0x40) {
               break;
           }
       }
       rect.x = 0x140;
       rect.y = 0xE0;
       rect.w = 0x40;
       rect.h = 0x20;
       LoadImage(&rect, (u_long*)pTexture);
       DrawSync(0);
       HeapFree(pBits);
       HeapFree(pTexture);
   }
*/
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
   0x240C/0x24AC/0x254C+i*0x50 off g_Menu->pPrimBuffer, then 0x2C7C, 0x2CCC,
   0x2D1C=(number/10), 0x2D6C=(number%10) with number = pArg[0x23]+1.
   Tried: an explicit u32 offset accumulator (loop addu becomes
   "addu a2,s0,a2" but gcc spills s7 and the frame grows to 0x48, 87.42%).
   void func_801E68AC(u8* pArg) {
       s32 number;
       s32 i;

       func_8002675C(g_Menu->resources, 0xEE,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x240C),
                     g_Menu->renderContext, D_801E9FE0[0], 0x7A, 0x1000);
       func_8002675C(g_Menu->resources, 0xEE,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x24AC),
                     g_Menu->renderContext, D_801E9FE4[0], 0x7A, 0x1000);
       MenuSetPlayTimeDigits(*(u32*)pArg);
       for (i = 0; i < 7; i++) {
           func_8002675C(g_Menu->resources, ((u32*)g_Menu->playTimeDigitIds)[i],
                         (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x254C + i * 0x50),
                         g_Menu->renderContext, D_801E9FE0[i + 2], 0x7A,
                         0x1000);
       }
       func_8002675C(g_Menu->resources, 0x17,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2C7C),
                     g_Menu->renderContext, 0x8, 0x66, 0x1000);
       func_8002675C(g_Menu->resources, 0x32,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2CCC),
                     g_Menu->renderContext, 0x10, 0x66, 0x1000);
       number = pArg[0x23] + 1;
       func_8002675C(g_Menu->resources, number / 10,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2D1C),
                     g_Menu->renderContext, 0x10, 0x6E, 0x1000);
       number = pArg[0x23] + 1;
       func_8002675C(g_Menu->resources, number % 10,
                     (POLY_FT4*)((u8*)g_Menu->pPrimBuffer + 0x2D6C),
                     g_Menu->renderContext, 0x18, 0x6E, 0x1000);
   }
*/


void MenuDrawSlotSprite(u8 arg0, u8* pArg1) {
    u8* pPrims;
    u32 offset;

    offset = 0xA98 + arg0 * 0x87C;
    pPrims = (u8*)(offset + (u32)g_Menu->pPrimBuffer);
    func_8002675C(g_Menu->resources, (pArg1 + arg0)[0x1C] + MENU_TEX_CHARACTER_PORTRAITS_SMALL, pPrims,
                  g_Menu->renderContext, g_MenuSlotSpriteX[arg0], g_MenuSlotSpriteY[arg0], 0x1000);
}

void MenuParseNumberToDigits(u32 number);

extern u32 g_MenuSlotDigitX;
extern u32 g_MenuSlotDigitY;

// pSlot and pCount re-derive pCounter's address: gcc emits retail's base-first
// "addu d,base,offset" only when that sum is an assignment's own result.
void MenuDrawSlotResourceSprites(u8 arg0, u8* pArg1) {
    u8* pPrims;
    u8* pCounter;
    u8* pSlot;
    u8* pCount;
    s32 primOffset;
    int i;

    MenuParseNumberToDigits((pArg1 + arg0)[0x16]);
    pCounter = (u8*)g_Menu->pPrimBuffer + arg0 * 0x87C;
    pCounter[0x1308] = 0;
    for (i = 0; i < MENU_SLOT_STAT_DIGIT_COUNT; i++) {
        if (g_Menu->digits[6 + i] == MENU_DIGIT_NONE) {
            continue;
        }
        primOffset = arg0 * 0x87C + 0xA98;
        pPrims = (u8*)(primOffset + (u32)g_Menu->pPrimBuffer);
        *((pSlot = (u8*)g_Menu->pPrimBuffer + arg0 * 0x87C) + 0x1308) += func_8002675C(
            g_Menu->resources, g_Menu->digits[6 + i],
            (POLY_FT4*)(pPrims + (*((pCount = (u8*)g_Menu->pPrimBuffer + arg0 * 0x87C) + 0x1308) * 0x50 + 0x320)),
            g_Menu->renderContext, arg0 * 0x50 + g_MenuSlotDigitX + i * 8,
            g_MenuSlotDigitY, 0x1000);
    }
    MenuParseNumberToDigits((pArg1 + arg0)[0x19]);
    pCounter = (u8*)g_Menu->pPrimBuffer + arg0 * 0x87C;
    pCounter[0x1309] = 0;
}
