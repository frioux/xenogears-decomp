#include "common.h"
#include "psyq/libgpu.h"
#include "battle/battle_work.h"
#include "battle/actor.h"

// Size of the pool BattleAllocPacketPrimitives hands out (g_BattleGraphicsWork's
// pPacketPrims): the 0x50-byte POLY_FT4 pairs BattleInitPacketPrimitives fills,
// Size of the pulse-triangle pool BattleInitPulseTrianglePrims clears: the eight
// POLY_G3 at +0x0 and the fade ramp and flags that follow them at +0xE0.
#define BATTLE_PULSE_TRIANGLE_POOL_SIZE 0xEC

// Height of one text-slot row in VRAM: the slot rects are 13 px tall
// (BattleInitTextSlotGraphics sets h = 0xD) and the row's y offset is (slot / 2) * 13.
#define BATTLE_TEXT_ROW_HEIGHT 13

// Bytes of one slot's text buffer: the size BattleAllocTextBuffer is asked for and
// the capacity SystemRenderStringEntry renders the slot's string into.
#define BATTLE_TEXT_SLOT_BUFFER_SIZE 0x39

// VRAM x of the text texture page the slot textures are loaded into and drawn from.
#define BATTLE_TEXT_TEX_PAGE_X 0x3C0

// The action-row graphics BattleLoadActionRowGraphics builds: one text buffer per
// row, each rendered with room for this many characters.
#define BATTLE_ACTION_ROW_COUNT 11
#define BATTLE_ACTION_ROW_TEXT_CAPACITY 27

// The digit glyphs g_BattleDigitImageBuffers holds, 0 through 9: the one row of
// six-pixel digits BattleLoadActionRowGraphics uploads beside the action rows.
#define BATTLE_DIGIT_IMAGE_COUNT 10

// tpage's semi-transparency rate field, the `abr` arg of GetTPage (bits 5-6): the
// add and subtract modes the BlendAdd and BlendSub setters below turn on in it.
#define BATTLE_PRIM_TPAGE_BLEND_ADD 0x20
#define BATTLE_PRIM_TPAGE_BLEND_SUB 0x40

// What g_BattleSlotDisplayValues holds for a slot whose flag is clear:
// BattleResetSlotDisplayValues writes it to both of the slot's values.
#define BATTLE_SLOT_DISPLAY_VALUE_EMPTY 0xFF

// The pulse-triangle pool g_BattlePulseTrianglePrims points at: the eight POLY_G3
// triangles this unit's BattleInitPulseTrianglePrims builds, then the ramp and the
// flags the pulse reads (src/battle/main/misc2.c carries the same view).
typedef struct {
    /* 0x00 */ POLY_G3 prims[8];
    /* 0xE0 */ s32 fade;     // ramp word: seeded to 0xFF here, stepped by 4 in misc2
    /* 0xE4 */ u8 firstPrim; // index of the first triangle drawn this frame
    /* 0xE5 */ u8 fading;    // 1 while fading down, 0 while fading up
    /* 0xE6 */ u8 unkE6[6];
} BattlePulseWork; // Size: 0xEC

// One 0x60-byte row of the screen slot table at g_BattleTextSlotRects: the VRAM rect
// the slot's text texture is loaded into, the buffer it is rendered into and the width
// that came out of the render (src/battle/main/misc5.c carries the same view).
typedef struct {
    /* 0x00 */ RECT rect;
    /* 0x08 */ u_long *pBuf;
    /* 0x0C */ u8 unkC[2];
    /* 0x0E */ u8 textWidth;
    /* 0x0F */ u8 unkF[0x51];
} BattleScreenSlot; // Size: 0x60

// One 8-byte record of the tile table at g_BattleCommandTiles, which callers index
// by command: BattleQueueCommandSlotAndTile reads its two bytes as the 16-bit tile it stores in the
// command record's param.
typedef struct {
    /* 0x00 */ u8 tileLo;
    /* 0x01 */ u8 tileHi;
    /* 0x02 */ u8 unk2[6];
} BattleCommandTile; // Size: 0x8

// One 0x48-byte row of the command record table at g_BattleCommandRecords, as the
// queueing tail of this unit writes it: the actor slot at +0xD, the word the
// command carries at +0x24 and the command byte at +0x31.
typedef struct {
    /* 0x00 */ u8 unk0[0xD];
    /* 0x0D */ u8 actorSlot;
    /* 0x0E */ u8 unkE[0x16];
    /* 0x24 */ u16 param;
    /* 0x26 */ u8 unk26[0xB];
    /* 0x31 */ u8 command;
    /* 0x32 */ u8 unk32[0x16];
} BattleCommandRecord; // Size: 0x48

// One row of a battle sub-image: 198 halfwords of texels, uploaded as a 198x1
// RECT because StoreImage takes its size from the rect.
#define BATTLE_SUB_IMAGE_ROW_HALFWORDS 198
#define BATTLE_SUB_IMAGE_ROW_BYTES (BATTLE_SUB_IMAGE_ROW_HALFWORDS * 2)
// Bytes of one sub-image (its four rows), and how many of them
// g_BattleGraphicsWork->subImages holds (0x18C0 bytes).
#define BATTLE_SUB_IMAGE_BYTES (BATTLE_SUB_IMAGE_ROW_BYTES * 4)
#define BATTLE_SUB_IMAGE_COUNT 4
// Where the first of them sits in the work block (BattleGraphicsWork.subImages).
#define BATTLE_GRAPHICS_SUB_IMAGES_OFFSET 0x8970

// A texture page is 256 halfwords square, the size the two full-page rects in
// the display state's rect table are set to.
#define BATTLE_TEX_PAGE_SIZE 0x100
// The page coordinate of a tile is in 64-halfword units: the display state's
// texel rects take its low six bits, doubled to a halfword offset.
#define BATTLE_TEX_PAGE_COORD_MASK 0x3F

// The four DR_MODE packets of g_BattleGraphicsWork, each 0xC bytes, sitting in
// the gap between its scroll state and its upload rects.
#define BATTLE_DRAW_MODES_OFFSET 0x8908

// The tile records (BattleTileTexture) as the display state's rect table takes
// them: the page coordinates it copies out are 16-bit values, the low halfword
// of the record's s32 coordinate fields.
typedef struct {
    /* 0x00 */ s32 tp;
    /* 0x04 */ s32 clutX;
    /* 0x08 */ u16 clutY;
    /* 0x0A */ u16 padA;
    /* 0x0C */ u16 texPageX;
    /* 0x0E */ u16 padE;
    /* 0x10 */ u16 texPageY;
    /* 0x12 */ u16 pad12;
    /* 0x14 */ s32 unk14;
} BattleTilePageCoords; // Size: 0x18

extern u8 *g_BattlePulseTrianglePrims;
extern u8 g_BattlePrimStartIndex;
extern u8 g_BattleActorCharIds[];
extern u8 D_800D3294;
extern u8 *g_BattleSpriteArchive;
extern BattleGraphicsWork *g_BattleGraphicsWork;
extern u8 *g_BattleCommandQueue;
extern BattleWork *g_BattleDisplayState;
extern BattleCommandRecord g_BattleCommandRecords[];
extern BattleCommandTile g_BattleCommandTiles[];
extern s32 g_SystemSemiTransMode;
extern u8 g_BattleSlotEnabledFlags[];
extern u8 g_BattleActorCountdownState[];
extern u16 g_BattleSlotDisplayValues[];
extern u8 g_BattleTextSlotCount;
extern u8 *g_BattleTextStringBank;
extern u8 g_BattleTextActorStringIds[];
extern BattleScreenSlot g_BattleTextSlotRects[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

extern s32 BattleAllocHeapBlock(s32, s32);
extern s32 GetStringEntryFromBank12(s32);
extern u16 BattleSelectActionRow(s32, s32);
extern void BattleInitPacketPrimitives(void);
extern void HeapFree(void *pMem);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern void BattleLoadImageAndSync(RECT *, u_long *);
extern void ResourceHelperGetTexCoords(void *, u32, s32 *, s32 *, s32 *, s32 *, s32 *, s32 *);
extern void bzero(void *p, s32 n);
extern void BattleInitSlotGraphics(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4, u8 arg5, u8 arg6);
extern void *BattleAllocTextBuffer(s32 arg0);
extern u8 D_800D36C0[];
extern u8 g_BattleTextSlotBuffers[];
extern u8 g_BattlePrimBuffer[];
extern BattleSlotGraphics *g_BattleSlotGraphics[];
extern u8 g_SystemBackdropColor[];
extern u8 D_800D2F3C[];
extern u8 D_800D2F3D[];
extern u8 D_800D2F3E[];
extern u8 D_800D2F3F[];
extern u8 g_BattleActionRowIconSprites[];
extern u8 D_800D2F69[];
extern u8 *g_BattleDigitImageBuffers[];


void BattleSetPrimSemiTransTexture(POLY_FT4 *p) {
    SetSemiTrans(p, 1);
    SetShadeTex(p, 0);
}
void BattleSetPrimGrey80BlendModeFromGlobal(POLY_FT4 *p) {
    BattleSetPrimSemiTransTexture(p);
    p->r0 = 0x80;
    p->g0 = 0x80;
    p->b0 = 0x80;
    if (g_SystemSemiTransMode != 0) {
        p->tpage |= BATTLE_PRIM_TPAGE_BLEND_SUB;
    } else {
        p->tpage &= ~BATTLE_PRIM_TPAGE_BLEND_SUB;
    }
}
void BattleSetPrimGrey80BlendAdd(POLY_FT4 *p) {
    BattleSetPrimSemiTransTexture(p);
    p->r0 = 0x80;
    p->g0 = 0x80;
    p->b0 = 0x80;
    p->tpage |= BATTLE_PRIM_TPAGE_BLEND_ADD;
}

void BattleSetPrimGrey40BlendAdd(POLY_FT4 *p) {
    BattleSetPrimSemiTransTexture(p);
    p->r0 = 0x40;
    p->g0 = 0x40;
    p->b0 = 0x40;
    p->tpage |= BATTLE_PRIM_TPAGE_BLEND_ADD;
}

void BattleSetPrimGrey80BlendSub(POLY_FT4 *p) {
    BattleSetPrimSemiTransTexture(p);
    p->r0 = 0x80;
    p->g0 = 0x80;
    p->b0 = 0x80;
    p->tpage |= BATTLE_PRIM_TPAGE_BLEND_SUB;
}

void BattleSetPrimGrey40BlendSub(POLY_FT4 *p) {
    BattleSetPrimSemiTransTexture(p);
    p->r0 = 0x40;
    p->g0 = 0x40;
    p->b0 = 0x40;
    p->tpage |= BATTLE_PRIM_TPAGE_BLEND_SUB;
}

void BattleSetPolyFT4RectH13(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, u8 dx) {
    p->x0 = x;
    p->y0 = y;
    p->y1 = y;
    p->x2 = x;
    p->y2 = y + BATTLE_TEXT_ROW_HEIGHT;
    p->y3 = y + BATTLE_TEXT_ROW_HEIGHT;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + dx;
    p->x3 = x + dx;
    p->v0 = v;
    p->u1 = u + dx;
    p->v1 = v;
    p->v2 = v + BATTLE_TEXT_ROW_HEIGHT;
    p->u3 = u + dx;
    p->v3 = v + BATTLE_TEXT_ROW_HEIGHT;
}
void BattleSetPolyFT4Rect(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, s16 dx, s16 dy) {
    p->x0 = x;
    p->y0 = y;
    p->y1 = y;
    p->x2 = x;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + dx;
    p->y2 = y + dy;
    p->x3 = x + dx;
    p->y3 = y + dy;
    p->v0 = v;
    p->u1 = u + dx;
    p->v1 = v;
    p->v2 = v + dy;
    p->u3 = u + dx;
    p->v3 = v + dy;
}
/* 92.31% (344 bytes / 86 insns against retail "s 332 / 83): the do-while, the
   switch chain and the whole tail are already exact (`addiu s0,s0,0x28;
   addiu v0,s2,0x50; slt; bnez`, signed compare, no pre-loop guard). The residual
   is ONE extra induction variable minted by loop.c: retail keeps a single cursor
   (s0 = p) and reaches every field as a displacement off it (sb 4/5/6, sh 0xE,
   sh 0x16), while every body below also gets a second giv at p+0xE (s0) for the
   five field accesses with displacements -0xA/-9/-8/0/8, i.e. +1 addiu, a 6th
   saved register and frame 0x30 instead of 0x28. Tried and measured: u8 *p with
   byte offsets (92.31, the park below in its first shape), typed POLY_FT4 *p
   with -> members (92.31, identical RTL - so the g-index-form lever does NOT
   transfer here), the cursor in the parameter with a separate start local
   (84.96, 90 insns), and a 4-minute permuter run (2188 iterations, base 952
   best 520) whose only lead DELETES the advancing cursor and reads every field
   through a cast of pInfo (wrong code - rejected).
   Parked body (92.31): 
void func_80076D58(u8 *pInfo, u8 flag, u8 mode) {
    u8 *p;

    p = pInfo;
    do {
        SetPolyFT4(p);
        p[4] = 0x80;
        p[5] = 0x80;
        p[6] = 0x80;
        SetSemiTrans(p, 0);
        SetShadeTex(p, 1);
        switch (mode) {
        case 0:
            *(u16 *)(p + 0x16) = GetTPage(0, 0, 0x380, 0);
            break;
        case 1:
            *(u16 *)(p + 0x16) = GetTPage(0, 0, 0x380, 0x100);
            break;
        case 2:
            *(u16 *)(p + 0x16) = GetTPage(0, 0, 0x3C0, 0x100);
            break;
        case 3:
            *(u16 *)(p + 0x16) = GetTPage(0, 0, 0x3C0, 0);
            break;
        }
        if (flag != 0) {
            *(u16 *)(p + 0xE) = g_SystemPalette2;
        } else {
            *(u16 *)(p + 0xE) = g_SystemPalette1;
        }
        p += 0x28;
    } while ((s32)p < (s32)pInfo + 0x50);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076D58);
void BattleLoadActionRowGraphics(void) {
    u8 *pBuf[BATTLE_ACTION_ROW_COUNT];
    RECT rect;
    u8 **p;
    u8 *pEntry;
    u8 result;
    s32 i;
    s32 off;
    s32 off2;
    s32 y;
    s32 h;
    u8 **pSlot;

    i = 0;
    y = 0x10D;
    h = BATTLE_TEXT_ROW_HEIGHT;
    off = 0x2C;
    for (; i < BATTLE_ACTION_ROW_COUNT; i++) {
        // the assignment inside the store's address expression is what makes gcc
        // form the slot address after the call and base-first, as retail does
        *(pSlot = pBuf + i) = BattleAllocTextBuffer(BATTLE_ACTION_ROW_TEXT_CAPACITY);
        bzero(*pSlot, 0x30C);
        D_800D2F3C[off] = SystemRenderStringEntry(GetStringEntryFromBank12(i + 0xA), *pSlot, BATTLE_ACTION_ROW_TEXT_CAPACITY, 0);
        result = SystemRenderStringEntry(GetStringEntryFromBank12(i + 0x15), *pSlot, BATTLE_ACTION_ROW_TEXT_CAPACITY, 1);
        g_BattleActionRowIconSprites[off] = result;
        pEntry = (u8 *)(g_BattleActionRowIconSprites + off);
        D_800D2F3E[off] = pEntry[2] = 0x78;
        D_800D2F3F[off] = pEntry[3] = h;
        D_800D2F3D[off] = 0;
        D_800D2F69[off] = 1;
        rect.x = 0x3DE;
        rect.y = y;
        rect.w = 0x1E;
        rect.h = BATTLE_TEXT_ROW_HEIGHT;
        BattleLoadImageAndSync(&rect, *pSlot);
        y += BATTLE_TEXT_ROW_HEIGHT;
        h += BATTLE_TEXT_ROW_HEIGHT;
        off += 4;
    }
    p = pBuf;
    for (i = 0; i < BATTLE_ACTION_ROW_COUNT; i++) {
        HeapFree(*p++);
    }
    for (i = 0; i < BATTLE_DIGIT_IMAGE_COUNT; i++) {
        rect.x = 0x3DE + i * 2;
        rect.y = 0x100;
        rect.w = 6;
        rect.h = BATTLE_TEXT_ROW_HEIGHT;
        off2 = i * sizeof(u8 *);
        BattleLoadImageAndSync(&rect, *(u8 **)((u8 *)g_BattleDigitImageBuffers + off2));
    }
}

void BattleInitPacketPrimitives(void) {
    g_BattleGraphicsWork->pPacketPrims[0x669] = 0;
    g_BattleGraphicsWork->pPacketPrims[0x66B] = 0;
    g_BattleGraphicsWork->pPacketPrims[0x66D] = 0;
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x000, 0, 1);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x050, 0, 1);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x0A0, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x0F0, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x140, 0, 1);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x190, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x1E0, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x230, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x280, 0, 3);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x2D0, 1, 3);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x320, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x370, 1, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x3C0, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x410, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x460, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x4B0, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x500, 0, 2);
    func_80076D58(g_BattleGraphicsWork->pPacketPrims + 0x550, 0, 2);
}
void BattleInitTilePrims(POLY_FT4 *p, u8 index) {
    s32 i;

    for (i = 0; i < 4; i++) {
        SetPolyFT4(&p[i]);
        SetShadeTex(&p[i], 1);
        p[i].r0 = 0xFF;
        p[i].g0 = 0xFF;
        p[i].b0 = 0xFF;
        p[i].tpage = GetTPage(g_BattleGraphicsWork->tiles[index].tp, 0,
                              g_BattleGraphicsWork->tiles[index].texPageX,
                              g_BattleGraphicsWork->tiles[index].texPageY);
        p[i].clut = GetClut(g_BattleGraphicsWork->tiles[index].clutX,
                            g_BattleGraphicsWork->tiles[index].clutY);
    }
}
void BattleInitSlotPrims(u8 index) {
    BattleSlotGraphics *pGfx;
    u8 i;

    pGfx = g_BattleSlotGraphics[index];
    // the index-first spelling is what makes gcc emit retail's addu order
    for (i = 0; i < 2; i++) {
        SetPolyG4(&pGfx->prims[i]);
        (i + pGfx->prims)->r0 = g_SystemBackdropColor[0];
        (i + pGfx->prims)->g0 = g_SystemBackdropColor[1];
        (i + pGfx->prims)->b0 = g_SystemBackdropColor[2];
        (i + pGfx->prims)->r1 = g_SystemBackdropColor[0];
        (i + pGfx->prims)->g1 = g_SystemBackdropColor[1];
        (i + pGfx->prims)->b1 = g_SystemBackdropColor[2];
        (i + pGfx->prims)->r2 = g_SystemBackdropColor[0];
        (i + pGfx->prims)->g2 = g_SystemBackdropColor[1];
        (i + pGfx->prims)->b2 = g_SystemBackdropColor[2];
        (i + pGfx->prims)->r3 = g_SystemBackdropColor[0];
        (i + pGfx->prims)->g3 = g_SystemBackdropColor[1];
        (i + pGfx->prims)->b3 = g_SystemBackdropColor[2];
        SetSemiTrans(&pGfx->prims[i], 1);
        SetDrawMode(&pGfx->drawModes[i], 0, 0,
                    GetTPage(0, g_SystemSemiTransMode, g_BattleGraphicsWork->tiles[1].texPageX,
                             g_BattleGraphicsWork->tiles[1].texPageY),
                    (RECT *)g_BattleDisplayState);
    }
    BattleInitTilePrims(pGfx->tilePrims[0], 1);
    BattleInitTilePrims(pGfx->tilePrims[1], 2);
    BattleInitTilePrims(pGfx->tilePrims[2], 3);
    BattleInitTilePrims(pGfx->tilePrims[3], 4);
}

void BattleAllocPacketPrimitives(void) {
    u8 *p = (u8 *)BattleAllocHeapBlock(BATTLE_PACKET_PRIM_POOL_SIZE, 0);

    g_BattleGraphicsWork->pPacketPrims = p;
    bzero(p, BATTLE_PACKET_PRIM_POOL_SIZE);
    BattleInitPacketPrimitives();
}

void BattleFreePacketPrimitives(void) {
    BattleUpdateFrame();
    HeapFree(g_BattleGraphicsWork->pPacketPrims);
}

void BattleInitPulseTrianglePrims(void) {
    s32 i;
    s32 j;

    bzero(g_BattlePulseTrianglePrims, BATTLE_PULSE_TRIANGLE_POOL_SIZE);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            SetPolyG3((POLY_G3 *)((i * 2 + j) * sizeof(POLY_G3) + (mem_addr)g_BattlePulseTrianglePrims));
            switch (i) {
            case 0:
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x0 = 0xC0;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y0 = 0x70;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x1 = 0xB0;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y1 = 0x78;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x2 = 0xB0;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y2 = 0x68;
                break;
            case 1:
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x0 = 0xA0;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y0 = 0x90;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x1 = 0x98;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y1 = 0x80;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x2 = 0xA8;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y2 = 0x80;
                break;
            case 2:
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x0 = 0x80;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y0 = 0x70;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x1 = 0x90;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y1 = 0x68;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x2 = 0x90;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y2 = 0x78;
                break;
            case 3:
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x0 = 0xA0;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y0 = 0x50;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x1 = 0x98;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y1 = 0x60;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->x2 = 0xA8;
                ((POLY_G3 *)(g_BattlePulseTrianglePrims + (i * 2 + j) * sizeof(POLY_G3)))->y2 = 0x60;
                break;
            }
            ((u8 *)g_BattlePulseTrianglePrims)[(i * 2 + j) * sizeof(POLY_G3) + 4] = 0xFF;
            ((u8 *)g_BattlePulseTrianglePrims)[(i * 2 + j) * sizeof(POLY_G3) + 5] = 0;
            ((u8 *)g_BattlePulseTrianglePrims)[(i * 2 + j) * sizeof(POLY_G3) + 6] = 0;
            ((u8 *)g_BattlePulseTrianglePrims)[(i * 2 + j) * sizeof(POLY_G3) + 0xC] = 0x40;
            ((u8 *)g_BattlePulseTrianglePrims)[(i * 2 + j) * sizeof(POLY_G3) + 0xD] = 0;
            ((u8 *)g_BattlePulseTrianglePrims)[(i * 2 + j) * sizeof(POLY_G3) + 0xE] = 0;
            ((u8 *)g_BattlePulseTrianglePrims)[(i * 2 + j) * sizeof(POLY_G3) + 0x14] = 0x40;
            ((u8 *)g_BattlePulseTrianglePrims)[(i * 2 + j) * sizeof(POLY_G3) + 0x15] = 0;
            ((u8 *)g_BattlePulseTrianglePrims)[(i * 2 + j) * sizeof(POLY_G3) + 0x16] = 0;
        }
    }
    // The ramp word stays an untyped store: written as `->fade` (s32 or a union member
    // alike) gcc 2.7.2 sinks it behind the two flag stores and hoists 0xFF late, 97.87%.
    *(s32 *)(g_BattlePulseTrianglePrims + 0xE0) = 0xFF;
    ((BattlePulseWork *)g_BattlePulseTrianglePrims)->fading = 1;
    ((BattlePulseWork *)g_BattlePulseTrianglePrims)->firstPrim = g_BattlePrimStartIndex;
    g_BattleDisplayState->pulseTriangleActive = 1;
}

void BattleClearFlagC6(void) {
    g_BattleDisplayState->pulseTriangleActive = 0;
}

// Stages the battle's tile atlas: stores the four sub-images of
// g_BattleGraphicsWork->subImages into the four VRAM rows they are drawn from,
// asks the sprite archive for the texel parameters of the four tile textures,
// and points the display state's rect table and draw modes at them.
// Parked at 92.83% (1820 bytes / 455 insns against retail's 1816 / 454 - the one
// extra instruction is a load-delay nop). Everything from the 16 StoreImage calls
// to the end of the function is byte-exact: the four sub-image base registers
// (s6/s1/s2/s0 = gfx + 0x8970/0x8FA0/0x95D0/0x9C00, each with the 0x18C/0x318/0x4A4
// row offsets), the four ResourceHelperGetTexCoords blocks with their six
// out-pointers, the display state's six RECTs, and the four GetTPage+SetDrawMode
// pairs (calls 1-2 pass the rect after the page rect as tw, calls 3-4 pass the
// page rect itself).
// What is left is ORDER in the two straight-line blocks at the head: retail's
// orders are pure RTL (each statement expands value-then-address, no store moved
// across another) while this unit's cc1 2.6.0 SCHEDULES the block - it hoists
// each rect's texPageX/texPageY lhu above the neighbouring constant stores and
// sinks the andi/sll/sh behind them, which is also what costs the extra nop.
// The lever that moved it: a do-while(0) block around each rect's x assignment
// (and around the w/h/y trio that follows it) pins the group order and the
// allocator's register - 92.19 -> 92.83 and 97 -> 61 differing lines, and it is
// what puts pRects in v1 and pPageCoords in a1 as retail has them. Tried on top
// of that, all measured: rect1/rect4 statement orders x,w,h,y and x,w,y,h
// (91.96), a block per statement instead of per group (identical, so only the
// group boundary is a lever), and blocks around rect0 plus the tiles texPageX/
// texPageY stores (91.74 / 92.20). The head's remaining rows - li v0,1 and
// li v1,0xC6 placed before the pGfx load in retail, and retail's first clutY lhu
// above the four w stores - are the same scheduler class, and a scheduler hoist
// (retail's StoreImage argument setup, addu a0,s0,s7 / move a1,s6, sits ABOVE the
// eight rect stores) cannot be typed in C at all.
// ROUND 474 RE-MEASURED, all from the live body: 92.83 / 1820 bytes / 455 insns
// (retail 1816 / 454, and the 4 bytes are one load-delay nop - this body carries
// four mid-function nops, retail three, so the C cannot link until retail's slot
// holds a store). try_flags --sweep settles the compiler: 2.6.0 -O2/-O3 x -G4/-G8
// all 92.83, gcc-2.7.2 91.81, gcc-2.7.2-cdk 72.3. The scheduler probe (this unit's
// own .i through cc1 with -fno-schedule-insns 78.92, -fno-schedule-insns2 79.61,
// both 54.74) says retail IS scheduled, so the unscheduled stream is not it.
// The do-while(0) wraps below are worth 0.22 under 2.6.0: with every wrap removed
// the same body scores 92.61 with 97 differing lines, and a wrap around EVERY
// rect statement instead of around each x/w-h-y group scores the same 92.83 / 61,
// so only the presence of the boundaries matters, not their granularity. Three
// more shapes measured on the head block, all BYTE-IDENTICAL to this body (source
// order does not reach those rows): the four uploadRect w stores moved after the
// four y stores; a `s32 rowHalfwords` local for 0xC6 (cc1 folds it back to the
// constant); and a do-while(0) wrap of the eight rect stores plus the y/w quartet,
// or an empty `do { } while (0);` between them. The residual stays what it was:
// sched2 fills retail's load-delay slots with the neighbouring w/h constant stores
// and hoists li v1,0xC6 into the prologue, while this body sinks the first clutY
// lhu below the four w stores and leaves one nop. Options for a next attempt: find
// group boundaries that reproduce retail's per-rect order (rect2/3/5 store
// x,w,h,y, rect4 x,w,y,h; the x load still has to hoist above the tiles texPageY
// stores as retail's does).
// Struct facts it establishes: the upload rects' y values are tiles[0].clutY - 1,
// clutY, clutY - 2, clutY - 3, and every read the display state's rect table
// makes of the tile records is 16-bit (lhu) while GetTPage reads the same words as
// s32 - hence the BattleTilePageCoords view at the top of this file. The four
// DR_MODE packets are at gfx + 0x8908, 0xC apart, inside what battle_work.h still
// calls unk6417. Parked body (92.83):
/*
void func_80077990(void) {
    BattleGraphicsWork *pGfx;
    RECT *pRects;
    BattleTilePageCoords *pPageCoords;
    u8 *pImage0;
    u8 *pImage1;
    u8 *pImage2;
    u8 *pImage3;

    pGfx = g_BattleGraphicsWork;
    pImage0 = (u8 *)pGfx + BATTLE_GRAPHICS_SUB_IMAGES_OFFSET;
    pImage1 = (u8 *)pGfx + BATTLE_GRAPHICS_SUB_IMAGES_OFFSET + BATTLE_SUB_IMAGE_BYTES;
    pImage2 = (u8 *)pGfx + BATTLE_GRAPHICS_SUB_IMAGES_OFFSET + BATTLE_SUB_IMAGE_BYTES * 2;
    pImage3 = (u8 *)pGfx + BATTLE_GRAPHICS_SUB_IMAGES_OFFSET + BATTLE_SUB_IMAGE_BYTES * 3;

    // the four VRAM rows the sub-images are stored into: one sub-image row each,
    // the last four rows above the CLUT block
    pGfx->uploadRect[0].x = 1;
    pGfx->uploadRect[0].h = 1;
    pGfx->uploadRect[1].x = 1;
    pGfx->uploadRect[1].h = 1;
    pGfx->uploadRect[2].x = 1;
    pGfx->uploadRect[2].h = 1;
    pGfx->uploadRect[3].x = 1;
    pGfx->uploadRect[3].h = 1;
    pGfx->uploadRect[0].w = BATTLE_SUB_IMAGE_ROW_HALFWORDS;
    pGfx->uploadRect[1].w = BATTLE_SUB_IMAGE_ROW_HALFWORDS;
    pGfx->uploadRect[2].w = BATTLE_SUB_IMAGE_ROW_HALFWORDS;
    pGfx->uploadRect[3].w = BATTLE_SUB_IMAGE_ROW_HALFWORDS;
    pPageCoords = (BattleTilePageCoords *)pGfx->tiles;
    pGfx->uploadRect[0].y = pPageCoords[0].clutY - 1;
    pGfx->uploadRect[1].y = pPageCoords[0].clutY;
    pGfx->uploadRect[2].y = pPageCoords[0].clutY - 2;
    pGfx->uploadRect[3].y = pPageCoords[0].clutY - 3;

    StoreImage(&g_BattleGraphicsWork->uploadRect[0], (u_long *)pImage0);
    StoreImage(&g_BattleGraphicsWork->uploadRect[1], (u_long *)pImage1);
    StoreImage(&g_BattleGraphicsWork->uploadRect[2], (u_long *)pImage2);
    StoreImage(&g_BattleGraphicsWork->uploadRect[3], (u_long *)pImage3);
    StoreImage(&g_BattleGraphicsWork->uploadRect[0], (u_long *)(pImage0 + BATTLE_SUB_IMAGE_ROW_BYTES));
    StoreImage(&g_BattleGraphicsWork->uploadRect[1], (u_long *)(pImage1 + BATTLE_SUB_IMAGE_ROW_BYTES));
    StoreImage(&g_BattleGraphicsWork->uploadRect[2], (u_long *)(pImage2 + BATTLE_SUB_IMAGE_ROW_BYTES));
    StoreImage(&g_BattleGraphicsWork->uploadRect[3], (u_long *)(pImage3 + BATTLE_SUB_IMAGE_ROW_BYTES));
    StoreImage(&g_BattleGraphicsWork->uploadRect[0], (u_long *)(pImage0 + BATTLE_SUB_IMAGE_ROW_BYTES * 2));
    StoreImage(&g_BattleGraphicsWork->uploadRect[1], (u_long *)(pImage1 + BATTLE_SUB_IMAGE_ROW_BYTES * 2));
    StoreImage(&g_BattleGraphicsWork->uploadRect[2], (u_long *)(pImage2 + BATTLE_SUB_IMAGE_ROW_BYTES * 2));
    StoreImage(&g_BattleGraphicsWork->uploadRect[3], (u_long *)(pImage3 + BATTLE_SUB_IMAGE_ROW_BYTES * 2));
    StoreImage(&g_BattleGraphicsWork->uploadRect[0], (u_long *)(pImage0 + BATTLE_SUB_IMAGE_ROW_BYTES * 3));
    StoreImage(&g_BattleGraphicsWork->uploadRect[1], (u_long *)(pImage1 + BATTLE_SUB_IMAGE_ROW_BYTES * 3));
    StoreImage(&g_BattleGraphicsWork->uploadRect[2], (u_long *)(pImage2 + BATTLE_SUB_IMAGE_ROW_BYTES * 3));
    StoreImage(&g_BattleGraphicsWork->uploadRect[3], (u_long *)(pImage3 + BATTLE_SUB_IMAGE_ROW_BYTES * 3));

    // the texel parameters of the four tile textures: the count word that closes
    // tiles[n] and the five texture fields of tiles[n + 1]
    ResourceHelperGetTexCoords(g_BattleSpriteArchive, 0x4B,
                               &g_BattleGraphicsWork->tiles[0].direntCount, &g_BattleGraphicsWork->tiles[1].tp,
                               &g_BattleGraphicsWork->tiles[1].clutX, &g_BattleGraphicsWork->tiles[1].clutY,
                               &g_BattleGraphicsWork->tiles[1].texPageX, &g_BattleGraphicsWork->tiles[1].texPageY);
    ResourceHelperGetTexCoords(g_BattleSpriteArchive, 0x50,
                               &g_BattleGraphicsWork->tiles[1].direntCount, &g_BattleGraphicsWork->tiles[2].tp,
                               &g_BattleGraphicsWork->tiles[2].clutX, &g_BattleGraphicsWork->tiles[2].clutY,
                               &g_BattleGraphicsWork->tiles[2].texPageX, &g_BattleGraphicsWork->tiles[2].texPageY);
    ResourceHelperGetTexCoords(g_BattleSpriteArchive, 0x4D,
                               &g_BattleGraphicsWork->tiles[2].direntCount, &g_BattleGraphicsWork->tiles[3].tp,
                               &g_BattleGraphicsWork->tiles[3].clutX, &g_BattleGraphicsWork->tiles[3].clutY,
                               &g_BattleGraphicsWork->tiles[3].texPageX, &g_BattleGraphicsWork->tiles[3].texPageY);
    ResourceHelperGetTexCoords(g_BattleSpriteArchive, 0x4E,
                               &g_BattleGraphicsWork->tiles[3].direntCount, &g_BattleGraphicsWork->tiles[4].tp,
                               &g_BattleGraphicsWork->tiles[4].clutX, &g_BattleGraphicsWork->tiles[4].clutY,
                               &g_BattleGraphicsWork->tiles[4].texPageX, &g_BattleGraphicsWork->tiles[4].texPageY);

    // the rect table the draw modes below take their texture window from: the
    // whole page, then the five texel rects
    pPageCoords = (BattleTilePageCoords *)g_BattleGraphicsWork->tiles;
    pRects = g_BattleDisplayState->texWindowRects;
    pRects[0].x = pRects[0].y = 0;
    pRects[0].h = BATTLE_TEX_PAGE_SIZE;
    pRects[0].w = BATTLE_TEX_PAGE_SIZE;
    g_BattleGraphicsWork->tiles[1].texPageX = 0x3C0;
    g_BattleGraphicsWork->tiles[2].texPageX = 0x3C8;
    pRects[1].x = (pPageCoords[0].texPageX & BATTLE_TEX_PAGE_COORD_MASK) * 2;
    g_BattleGraphicsWork->tiles[1].texPageY = 0x34;
    g_BattleGraphicsWork->tiles[2].texPageY = 0x34;
    pRects[1].y = pPageCoords[0].texPageY;
    pRects[1].w = BATTLE_TEX_PAGE_SIZE;
    pRects[1].h = BATTLE_TEX_PAGE_SIZE;
    do { pRects[2].x = (pPageCoords[1].texPageX & BATTLE_TEX_PAGE_COORD_MASK) * 2; } while (0);
    do {
        pRects[2].w = 8;
        pRects[2].h = 0x10;
        pRects[2].y = pPageCoords[1].texPageY;
    } while (0);
    do { pRects[3].x = (pPageCoords[2].texPageX & BATTLE_TEX_PAGE_COORD_MASK) * 2; } while (0);
    do {
        pRects[3].w = 8;
        pRects[3].h = 0x10;
        pRects[3].y = pPageCoords[2].texPageY;
    } while (0);
    do { pRects[4].x = (pPageCoords[3].texPageX & BATTLE_TEX_PAGE_COORD_MASK) * 2 + 0xE; } while (0);
    do {
        pRects[4].w = 0x10;
        pRects[4].y = pPageCoords[3].texPageY;
        pRects[4].h = 8;
    } while (0);
    do { pRects[5].x = (pPageCoords[4].texPageX & BATTLE_TEX_PAGE_COORD_MASK) * 2 + 0xE; } while (0);
    do {
        pRects[5].w = 0x10;
        pRects[5].h = 8;
        pRects[5].y = pPageCoords[4].texPageY;
    } while (0);

    SetDrawMode((DR_MODE *)((u8 *)g_BattleGraphicsWork + BATTLE_DRAW_MODES_OFFSET), 0, 0,
                GetTPage(g_BattleGraphicsWork->tiles[0].tp, 0,
                         g_BattleGraphicsWork->tiles[0].texPageX,
                         g_BattleGraphicsWork->tiles[0].texPageY),
                &g_BattleDisplayState->texWindowRects[1]);
    SetDrawMode((DR_MODE *)((u8 *)g_BattleGraphicsWork + BATTLE_DRAW_MODES_OFFSET + sizeof(DR_MODE)), 0, 0,
                GetTPage(g_BattleGraphicsWork->tiles[0].tp, 0,
                         g_BattleGraphicsWork->tiles[0].texPageX,
                         g_BattleGraphicsWork->tiles[0].texPageY),
                &g_BattleDisplayState->texWindowRects[1]);
    SetDrawMode((DR_MODE *)((u8 *)g_BattleGraphicsWork + BATTLE_DRAW_MODES_OFFSET + sizeof(DR_MODE) * 2), 0, 0,
                GetTPage(g_BattleGraphicsWork->tiles[0].tp, 0,
                         g_BattleGraphicsWork->tiles[0].texPageX,
                         g_BattleGraphicsWork->tiles[0].texPageY),
                g_BattleDisplayState->texWindowRects);
    SetDrawMode((DR_MODE *)((u8 *)g_BattleGraphicsWork + BATTLE_DRAW_MODES_OFFSET + sizeof(DR_MODE) * 3), 0, 0,
                GetTPage(g_BattleGraphicsWork->tiles[0].tp, 0,
                         g_BattleGraphicsWork->tiles[0].texPageX,
                         g_BattleGraphicsWork->tiles[0].texPageY),
                g_BattleDisplayState->texWindowRects);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077990);
/* Prior findings for BattleInitTextSlotPrims (now MATCHED at 100.00%): retail walks two
   0x28-byte packets from pInfo to pInfo+0x50 and finishes with pInfo[0x5D] = 0.
   Retail facts that the matched body below reproduces:
   1. loop compare is `slt` (signed) vs a bound re-formed inside the loop
      (addiu v0,s1,0x50; slt; bnez) with NO pre-loop guard: a do-while whose
      cursor is a POLY_FT4 derived from an index i, compared as (s32).
   2. retail stores the flag and RE-READS it (andi v0,s3,1; sb v0,0x5C(s1);
      lbu v0,0x5C(s1); beqz): the flag field is volatile in the struct.
   3. retail keeps the (slot/2)*13 multiply chain INSIDE the loop: the in-loop
      `slot = arg;` copy is the invariant gcc hoists instead of the multiply. */
/* Two POLY_FT4 packets plus the head of the next g_BattleTextSlotRects row:
   what BattleInitTextSlotPrims walks. Callers hand it a row's polys at +0x10 (0x50 back
   into the row above), so its palette/enabled land on the NEXT row's +0xC/+0xD
   bytes (misc5.c's BattleScreenSlot view names them). palette is volatile
   because the function stores it and re-reads it. */
typedef struct {
    /* 0x00 */ POLY_FT4 prims[2];
    /* 0x50 */ u8 unk50[0xC];
    /* 0x5C */ volatile u8 palette;
    /* 0x5D */ u8 enabled;
} BattleSlotPrimBlock; // Size: 0x5E

void BattleInitTextSlotPrims(BattleSlotPrimBlock *pInfo, s32 arg) {
    POLY_FT4 *p;
    u16 clut;
    s32 i;
    s32 slot;

    i = 0;
    do {
        p = &pInfo->prims[i];
        SetPolyFT4(p);
        p->r0 = 0x80;
        p->g0 = 0x80;
        p->b0 = 0x80;
        SetSemiTrans(p, 0);
        SetShadeTex(p, 1);
        pInfo->palette = arg & 1;
        // loop-local copy of the slot number: gcc hoists this invariant
        // instead of the row multiply, which retail keeps inside the loop
        slot = arg;
        if (pInfo->palette != 0) {
            clut = g_SystemPalette2;
        } else {
            clut = g_SystemPalette1;
        }
        p->clut = clut;
        p->tpage = GetTPage(0, 0, BATTLE_TEXT_TEX_PAGE_X, (slot / 2) * BATTLE_TEXT_ROW_HEIGHT);
        i++;
    } while ((s32)&pInfo->prims[i] <
             (s32)&pInfo->prims[2]);
    pInfo->enabled = 0;
}

/* Matched at 100.00% (372 bytes / 93 insns) only with the byte-offset spelling
   below: name the two offset givs (tools/rounds/levers.md,
   g-a-named-offset-local-pins-the-giv) - offPrev = i * sizeof(BattleScreenSlot)
   is slot i (0xC0 per pass) and pins the giv cse otherwise re-folds onto its
   sibling as symbol+0x60, off = (i + 1) * sizeof(BattleScreenSlot) is slot i+1,
   used by the buffer stores and by the RECT copy destination. The typed form
   (BattleScreenSlot *pSlot/pNext with ->rect/->pBuf) is structural, not a
   register tie: retail keeps one base register plus an offset giv where the
   typed access folds the destination into a small immediate, 82.41% / 84 insns;
   the pure struct-array index form is 73.85% / 78 insns. Round 419's warning
   that un-parking this grew BattleInitTextSlotPrims from 0xF4 to 0x114 does not reproduce
   under this unit's 2.6.0 preset (it stays 0xF4, objdiff 100.00).
   What the addresses say: the slot table is 0x60-byte rows of
   {RECT, pBuf, palette, enabled, textWidth, pad, POLY_FT4 polys[2]} (misc5.c's
   view), this loop runs over the even slots: it gives even slot i the rect and
   copies it to slot i + 1, puts the new buffer in slot i + 1, and hands
   BattleInitTextSlotPrims slot i's quads at 0x50 before row i and slot i + 1's at row i. */
void BattleInitTextSlotGraphics(void) {
    RECT *pRect;
    u8 *pBuf;
    s32 i;
    s32 off;
    s32 offPrev;

    BattleInitSlotGraphics(5, 8, 0x2A, 0x70, 0x12, 0, 0);
    ((u8 *)g_BattleDisplayState)[0xB5] = 0;
    BattleInitSlotGraphics(4, 0x20, 0xC8, 0xF4, 0x12, 0, 0);
    ((u8 *)g_BattleDisplayState)[0xB4] = 0;
    for (i = 0; i < 8; i += 2) {
        pBuf = BattleAllocTextBuffer(BATTLE_TEXT_SLOT_BUFFER_SIZE);
        offPrev = i * sizeof(BattleScreenSlot);
        off = (i + 1) * sizeof(BattleScreenSlot);
        *(u8 **)(D_800D36C0 + off) = pBuf;
        *(u8 **)(g_BattleTextSlotBuffers + off) = pBuf;
        pRect = (RECT *)((u8 *)g_BattleTextSlotRects + offPrev);
        pRect->x = BATTLE_TEXT_TEX_PAGE_X;
        pRect->y = (i / 2) * BATTLE_TEXT_ROW_HEIGHT;
        pRect->w = 0x3C;
        pRect->h = BATTLE_TEXT_ROW_HEIGHT;
        *(RECT *)((u8 *)g_BattleTextSlotRects + off) = *pRect;
        // slot i's quads sit 0x50 before its row, slot i + 1's at the row's +0x10
        BattleInitTextSlotPrims((BattleSlotPrimBlock *)((u8 *)g_BattleTextSlotRects + offPrev - 0x50), i);
        BattleInitTextSlotPrims((BattleSlotPrimBlock *)((u8 *)g_BattleTextSlotRects + offPrev + 0x10), i + 1);
    }
}
/* 81.38% (484 bytes / 121 insns against retail's 504 / 126): the body is right
   and the residual is ONE register. Under this unit's preset gcc 2.7.2 forms the
   address of g_BattleActorCharIds[i] from the loop counter (addu at,at,i) and
   reads the four metadata halfwords with lw, while retail keeps a SEPARATE giv
   register holding i-1, adds it to %hi(D_800D2D25) (the +1 folded into the
   symbol), fills the delay slot of the D_800D3294 test with sltiu (i-1),2 for
   "i != 0", and so runs one register short: the sixth out-pointer (metadata
   offset 0x14) is spilled at 0x98(sp), which is the whole 0xC8 vs 0xC0 frame and
   the 126 vs 121 instruction gap.
   RETAIL IS gcc 2.6.0 - see PRESET_260 in the round report.
   tools/scripts/try_flags.sh battle/main/misc3 func_80078310 --gcc
   gcc-2.6.0-psx scores this same body 86.57, and its assembled object is
   instruction-identical to retail from the lhu readbacks to the return: same
   "addu v0,sp,s0 / lhu 0x40..0x4C(v0)" sequence, same slti loop test, same six
   offset givs. Under 2.6.0 only the i-1 giv and the 0x14 spill above are
   missing. try_flags --all shows every already-matched function in this unit
   stays 100.00 under 2.6.0, so the unit can be switched to it.
   Tried, all measured: s32 charId 81.06; u8 charId (it produces the andi 0xFF
   retail has before the x0x460 multiply) 81.38; "i > 0" for "i != 0" 86.13 under
   2.6.0; struct-typed metadata rows (BattleTextureCoords, u16 fields) 60.00.
   An s32 metadata[18] is right: gcc narrows the four RECT stores to lhu itself.
   What the body establishes: ResourceHelperGetTexCoords' six out-parameters are
   six consecutive s32, one 0x18-byte row per party member, read back as
   halfwords only because they feed RECT.s16; the six offsets start at 0/4/8/0xC
   /0x10/0x14 and the tex-page x offset accumulates 6 per member, skipped slots
   included (so the += 6 is the loop tail's statement, not part of the if).
   Parked body (81.38): */
/*
// Where the six ResourceHelperGetTexCoords out-parameters of one texture sit in
// a metadata row: the row is six s32 and each texture uses one whole row.
#define BATTLE_TEX_COUNT(_i)  ((_i) * 6) + 0
#define BATTLE_TEX_PAGE(_i)   ((_i) * 6) + 1
#define BATTLE_TEX_CLUT_X(_i) ((_i) * 6) + 2
#define BATTLE_TEX_CLUT_Y(_i) ((_i) * 6) + 3
#define BATTLE_TEX_PAGE_X(_i) ((_i) * 6) + 4
#define BATTLE_TEX_PAGE_Y(_i) ((_i) * 6) + 5
// Bytes of one party member's sprite TIM block in the archive pTimData points at.
#define BATTLE_CHAR_TIM_BYTES 0x460
// An empty party slot in g_BattleActorCharIds.
#define BATTLE_CHAR_ID_EMPTY 0x7F
// The character sprite every party member but the first is drawn with while
// D_800D3294 (the character override) is set.
#define BATTLE_OVERRIDE_CHAR_ID 0xB
// Halfwords a texture page is shifted by per party member to pack the three
// sprites side by side in VRAM.
#define BATTLE_TEX_PAGE_STEP 6

void func_80078310(u8 *pTimData, u8 index) {
    TIM_IMAGE tim;
    s32 metadata[18];
    u8 charId;
    s32 texPageXOffset;
    s32 i;

    texPageXOffset = 0;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_BattleActorCharIds[i] != BATTLE_CHAR_ID_EMPTY) {
            charId = g_BattleActorCharIds[i];
            if (D_800D3294 != 0 && i != 0) {
                charId = BATTLE_OVERRIDE_CHAR_ID;
            }
            OpenTIM((u_long *)(pTimData + charId * BATTLE_CHAR_TIM_BYTES));
            ReadTIM(&tim);
            ResourceHelperGetTexCoords(g_BattleSpriteArchive, index + i,
                                       &metadata[BATTLE_TEX_COUNT(i)], &metadata[BATTLE_TEX_PAGE(i)],
                                       &metadata[BATTLE_TEX_CLUT_X(i)], &metadata[BATTLE_TEX_CLUT_Y(i)],
                                       &metadata[BATTLE_TEX_PAGE_X(i)], &metadata[BATTLE_TEX_PAGE_Y(i)]);
            tim.crect->x = metadata[BATTLE_TEX_CLUT_X(i)];
            tim.crect->y = metadata[BATTLE_TEX_CLUT_Y(i)];
            tim.prect->x = metadata[BATTLE_TEX_PAGE_X(i)] + texPageXOffset;
            tim.prect->y = metadata[BATTLE_TEX_PAGE_Y(i)];
            LoadImage(tim.crect, tim.caddr);
            LoadImage(tim.prect, tim.paddr);
            DrawSync(0);
        }
        texPageXOffset += BATTLE_TEX_PAGE_STEP;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078310);
void BattleResetSlotDisplayValues(u8 *pOut) {
    s32 i;
    u16 emptyValue;
    u8 *pFlags;
    u16 *pBase;
    u16 *pExtra;
    u16 *pValue;
    u16 *pValue2;

    i = 0;
    emptyValue = BATTLE_SLOT_DISPLAY_VALUE_EMPTY;
    pFlags = pOut;
    // g_BattleSlotDisplayValues is three banks of one u16 per slot
    pBase = g_BattleSlotDisplayValues;
    pExtra = pBase + BATTLE_ACTOR_SLOT_COUNT * 2;
    pValue = pBase;
    pValue2 = pValue + BATTLE_ACTOR_SLOT_COUNT;
    do {
        if (g_BattleSlotEnabledFlags[i] != 0) {
            u16 value = BattleSelectActionRow(i, 0);

            *pValue2 = value;
            *pValue = value;
        } else {
            *pValue2 = emptyValue;
            *pValue = emptyValue;
        }
        g_BattleActorCountdownState[i] = 0;
        *pExtra = 0;
        *pFlags++ = 0;
        pExtra++;
        pValue++;
        pValue2++;
        i++;
    } while (i < BATTLE_ACTOR_SLOT_COUNT);
}
void BattleQueueCommandSlotAndTile(u8 color, u8 index) {
    u8 *pEntry;
    u16 tile;

    tile = (g_BattleCommandTiles[index].tileHi << 8) | g_BattleCommandTiles[index].tileLo;
    g_BattleCommandRecords[g_BattleCommandQueue[0x2DA]].actorSlot = color;
    pEntry = g_BattleCommandQueue;
    g_BattleCommandRecords[pEntry[0x2DA]].param = tile;
    pEntry[0x2DA]++;
}
// A slot is only taken while g_BattleTextSlotCount is below this, the number of
// rows the buffer has room for.
#define BATTLE_TEXT_SLOT_MAX 9

void BattleQueueCommandFA(u8 index, u8 color) {
    u8 *pEntry;

    if (g_BattleTextSlotCount < BATTLE_TEXT_SLOT_MAX) {
        g_BattleTextSlotRects[g_BattleTextSlotCount].textWidth =
            SystemRenderStringEntry(GetStringEntry(g_BattleTextStringBank, g_BattleTextActorStringIds[index * 8]),
                                    g_BattleTextSlotRects[g_BattleTextSlotCount].pBuf,
                                    BATTLE_TEXT_SLOT_BUFFER_SIZE,
                                    g_BattleTextSlotCount & 1);
        BattleLoadImageAndSync(&g_BattleTextSlotRects[g_BattleTextSlotCount].rect,
                               g_BattleTextSlotRects[g_BattleTextSlotCount].pBuf);
        g_BattleCommandRecords[g_BattleCommandQueue[0x2DA]].actorSlot = color;
        g_BattleCommandRecords[g_BattleCommandQueue[0x2DA]].command = 0xFA;
        pEntry = g_BattleCommandQueue;
        g_BattleCommandRecords[pEntry[0x2DA]].param = g_BattleTextSlotCount++;
        pEntry[0x2DA]++;
    }
}
void BattleQueueCommandF7(u8 color, u8 flag) {
    u8 *pEntry;

    g_BattleCommandRecords[g_BattleCommandQueue[0x2DA]].actorSlot = flag;
    g_BattleCommandRecords[g_BattleCommandQueue[0x2DA]].command = 0xF7;
    pEntry = g_BattleCommandQueue;
    g_BattleCommandRecords[pEntry[0x2DA]].param = color;
    pEntry[0x2DA]++;
}
