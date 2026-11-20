#include "common.h"
#include "psyq/libgpu.h"
#include "battle/battle_work.h"
#include "battle/actor.h"

// Size of the pool BattleAllocPacketPrimitives hands out (g_BattleGraphicsWork's
// pPacketPrims): the 0x50-byte POLY_FT4 pairs BattleInitPacketPrimitives fills,
// Size of the pulse-triangle pool BattleInitPulseTrianglePrims clears: the eight
// POLY_G3 at +0x0 and the fade ramp and flags that follow them at +0xE0.
#define BATTLE_PULSE_TRIANGLE_POOL_SIZE 0xEC

// tpage's semi-transparency rate field, the `abr` arg of GetTPage (bits 5-6): the
// add and subtract modes the BlendAdd and BlendSub setters below turn on in it.
#define BATTLE_PRIM_TPAGE_BLEND_ADD 0x20
#define BATTLE_PRIM_TPAGE_BLEND_SUB 0x40

// What g_BattleSlotDisplayValues holds for a slot whose flag is clear:
// BattleResetSlotDisplayValues writes it to both of the slot's values.
#define BATTLE_SLOT_DISPLAY_VALUE_EMPTY 0xFF

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
extern u8 D_800C400B[];
extern u8 D_800C4022[];
extern u8 D_800C402F[];
extern u8 g_BattleCommandTiles[];
extern u8 D_800D2E61[];
extern s32 g_SystemSemiTransMode;
extern u8 g_BattleSlotEnabledFlags[];
extern u8 g_BattleActorCountdownState[];
extern u16 g_BattleSlotDisplayValues[];
extern u8 g_BattleTextSlotCount;
extern u8 *g_BattleTextStringBank;
extern u8 g_BattleTextActorStringIds[];
extern u8 g_BattleTextSlotRects[];
extern u8 g_BattleTextSlotBuffers[];
extern u8 g_BattleTextSlotFlags[];
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
    p->y2 = y + 13;
    p->y3 = y + 13;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + dx;
    p->x3 = x + dx;
    p->v0 = v;
    p->u1 = u + dx;
    p->v1 = v;
    p->v2 = v + 13;
    p->u3 = u + dx;
    p->v3 = v + 13;
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
/* 92.31%: the do-while below reproduces retail's loop tail exactly
   (`addiu s0,s0,0x28; addiu v0,s2,0x50; slt; bnez`, signed, no pre-loop
   guard) and the whole switch. The only difference is gcc's combine_givs
   re-basing the induction onto the address of the last memory reference:
   mine keeps p (calls + compare) in one register AND p+0xE (all the field
   accesses, displacements -0xA/-9/-8/0/8) in another, so it saves a 6th
   register (frame 0x30 vs 0x28). Retail keeps one register (p) with literal
   displacements 4/5/6/0xE/0x16. Tried: POLY_FT4* p++ with p < pInfo+2
   (83.14, also sltu + hoisted guard), u8* for-loop (83.14), do-while on u8*
   (92.31, this one), s32 cursor (85.66). Not found: the shape that stops the
   re-base.
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
/* 98.19%: every instruction but ONE operand order is byte-exact (464 bytes,
   same frame, same save order). Retail's store into the local packet-pointer
   array is `sll s0,s2,2 / addiu s5,sp,0x10 / addu s0,s5,s0` (base first), we
   get `addu s0,s0,s5`. `pBuf[i]`, i[pBuf], *(pBuf+i), (u8 *)pBuf + i * 4,
   (s32)i * 4 + (s32)pBuf and a `u8 **pBase = pBuf;` local all give index-first,
   so the order is a combine/allocator choice, not source-driven. A separate
   `pSlot = pBuf + i` statement does give retail's order, but the address is
   then formed before the call (468 bytes, `li a0,0x1B` falls out of the jal
   delay slot, 90.83).
   Two things this body had to get right: `i = 0;` must be the first statement
   (not the for-init) because the prologue's save+init pairs are emitted in
   first-assignment order and retail saves s2 first (96.80 -> 98.15); and the
   third loop's offset must be zeroed in the for-init clause
   (`for (i = 0, off2 = 0; ...)`) so `move s1,zero` lands after `li s3,0xD`
   where retail has it (98.15 -> 98.19). That third loop walks a byte offset
   because g_BattleDigitImageBuffers is typed u8 *, so `g_BattleDigitImageBuffers + off` double-scales.
   Parked body (98.19):
void func_80076EA4(void) {
    u8 *pBuf[11];
    RECT rect;
    u8 **p;
    u8 *pEntry;
    u8 result;
    s32 i;
    s32 off;
    s32 off2;
    s32 y;
    s32 h;

    i = 0;
    y = 0x10D;
    h = 0xD;
    off = 0x2C;
    for (; i < 11; i++) {
        pBuf[i] = BattleAllocTextBuffer(0x1B);
        bzero(pBuf[i], 0x30C);
        D_800D2F3C[off] = SystemRenderStringEntry(GetStringEntryFromBank12(i + 0xA), pBuf[i], 0x1B, 0);
        result = SystemRenderStringEntry(GetStringEntryFromBank12(i + 0x15), pBuf[i], 0x1B, 1);
        g_BattleActionRowIconSprites[off] = result;
        pEntry = (u8 *)(g_BattleActionRowIconSprites + off);
        D_800D2F3E[off] = pEntry[2] = 0x78;
        D_800D2F3F[off] = pEntry[3] = h;
        D_800D2F3D[off] = 0;
        D_800D2F69[off] = 1;
        rect.x = 0x3DE;
        rect.y = y;
        rect.w = 0x1E;
        rect.h = 0xD;
        BattleLoadImageAndSync(&rect, pBuf[i]);
        y += 0xD;
        h += 0xD;
        off += 4;
    }
    p = pBuf;
    for (i = 0; i < 11; i++) {
        HeapFree(*p++);
    }
    for (i = 0, off2 = 0; i < 0xA; i++) {
        rect.x = 0x3DE + i * 2;
        rect.y = 0x100;
        rect.w = 6;
        rect.h = 0xD;
        BattleLoadImageAndSync(&rect, *(u8 **)((u8 *)g_BattleDigitImageBuffers + off2));
        off2 += 4;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076EA4);
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
    *(s32 *)(g_BattlePulseTrianglePrims + 0xE0) = 0xFF;
    g_BattlePulseTrianglePrims[0xE5] = 1;
    g_BattlePulseTrianglePrims[0xE4] = g_BattlePrimStartIndex;
    g_BattleDisplayState->pulseTriangleActive = 1;
}

void BattleClearFlagC6(void) {
    g_BattleDisplayState->pulseTriangleActive = 0;
}

/* 92.19% (1820 bytes against retail's 1816): everything from the 16 StoreImage
   calls to the end of the function is byte-exact - the four sub-image base
   registers (s6/s1/s2/s0 = gfx + 0x8970/0x8FA0/0x95D0/0x9C00, each with the
   0x18C/0x318/0x4A4 row offsets), the four ResourceHelperGetTexCoords blocks
   with their six out-pointers, the display state's six RECTs, and the four
   GetTPage+SetDrawMode pairs (calls 1-2 pass the rect after the page rect as
   `tw`, calls 3-4 pass the page rect itself). The whole diff is in the RECT
   init block at the top:
   1. retail's `li v0,0x1` is the first thing in the function, before the
      g_BattleGraphicsWork load; ours comes after it.
   2. retail's first `lhu` of tiles[0].clutY is hoisted ABOVE the four
      `sh 0xC6` width stores while the other three stay below them: one load
      moved across four independent stores, the STORE_HOIST shape the playbook
      lists as not source-controllable, in the mirror direction. Tried: the load
      inline per rect (this body), a chained `h = w = 0x100` order swap, w and
      y interleaved per rect, and the four base pointers derived from one
      `pImage` local (that one loses the four base registers: 87.68%).
   Struct facts it establishes: the upload rects' y values are tiles[0].clutY
   - 1, clutY, clutY - 2, clutY - 3, and every read the display state's RECT
   table makes of the tile records is 16-bit (`lhu`) while GetTPage reads the
   same words as s32 - hence the BattleTilePageCoords view at the top of this
   file. The four DR_MODE packets are at gfx + 0x8908, 0xC apart, inside what
   battle_work.h still calls unk6417.
   Parked body (92.19):
   // Stages the battle's tile atlas: stores the four sub-images of
   // g_BattleGraphicsWork->subImages into the four VRAM rows they are drawn from,
   // asks the sprite archive for the texel parameters of the four tile textures,
   // and points the display state's rect table and draw modes at them.
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
       pRects[2].x = (pPageCoords[1].texPageX & BATTLE_TEX_PAGE_COORD_MASK) * 2;
       pRects[2].w = 8;
       pRects[2].h = 0x10;
       pRects[2].y = pPageCoords[1].texPageY;
       pRects[3].x = (pPageCoords[2].texPageX & BATTLE_TEX_PAGE_COORD_MASK) * 2;
       pRects[3].w = 8;
       pRects[3].h = 0x10;
       pRects[3].y = pPageCoords[2].texPageY;
       pRects[4].x = (pPageCoords[3].texPageX & BATTLE_TEX_PAGE_COORD_MASK) * 2 + 0xE;
       pRects[4].w = 0x10;
       pRects[4].h = 8;
       pRects[4].y = pPageCoords[3].texPageY;
       pRects[5].x = (pPageCoords[4].texPageX & BATTLE_TEX_PAGE_COORD_MASK) * 2 + 0xE;
       pRects[5].w = 0x10;
       pRects[5].h = 8;
       pRects[5].y = pPageCoords[4].texPageY;

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
/* 61.46%: retail walks two 0x28-byte packets from pInfo to pInfo+0x50 and
   finishes with pInfo[0x5D] = 0. What retail does that no attempt so far
   reproduces:
   1. its loop compare is `slt` (signed) against a register holding pInfo+0x50,
      and it re-forms that bound inside the loop (addiu s0,s0,0x28; addiu
      v0,s1,0x50; slt; bnez) with NO pre-loop guard. A POLY_FT4* compare (the
      body below) still gives `sltu` plus a hoisted `addiu v0,s2,0x50; sltu;
      beqz` guard, and so does a u8* compare, so the signed compare is not a
      pointer-vs-u8* question; it wants a signed integer cursor/limit.
   2. retail stores the flag and RE-READS it (andi v0,s3,1; sb v0,0x5C(s1);
      lbu v0,0x5C(s1); beqz) - every plain store-then-test spelling lets cse
      forward the store and compare the register instead.
   3. retail's GetTPage has a0=0 preloaded in the beqz delay slot and
      p->clut = tile lands in the jal's delay slot.
   Parked body (61.46):
void func_800780A8(u8 *pInfo, s32 arg) {
    POLY_FT4 *p;
    u16 clut;
    s32 offset;

    for (p = (POLY_FT4 *)pInfo; p < (POLY_FT4 *)(pInfo + 0x50); p++) {
        SetPolyFT4(p);
        p->r0 = 0x80;
        p->g0 = 0x80;
        p->b0 = 0x80;
        SetSemiTrans(p, 0);
        SetShadeTex(p, 1);
        pInfo[0x5C] = arg & 1;
        if (pInfo[0x5C] != 0) {
            clut = g_SystemPalette2;
        } else {
            clut = g_SystemPalette1;
        }
        p->clut = clut;
        offset = arg / 2;
        p->tpage = GetTPage(0, 0, 0x3C0, offset * 13);
    }
    pInfo[0x5D] = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_800780A8);
/* 94.68% (372 bytes = retail's size; only address shape differs): retail keeps
   ONE bare offset register (0x60 + 0xC0k, `li s3,0x60` incremented in the loop)
   shared by all three uses - two `lui $at,%hi(sym); addu $at,$at,s3; sw
   %lo(sym)($at)` stores and one `addu v0,s3,s5` for the RECT copy destination -
   plus a SECOND offset register (s0 = 0xC0k, `addu a0,s0,a0` for the first
   func_800780A8 base) and a third giv for base+0x10+offset (s4). With the
   symbol+0x60 folded into the address (this body) gcc instead makes s3 a
   pointer giv onto g_BattleTextSlotBuffers. Tried this round: explicit `offset` local
   incremented in the loop (88.25, 356 bytes - one giv short, the copy
   destination becomes base+0x60+offset); `offset` + `offset2 = offset + 0x60`
   (94.45, 368 bytes - the s3 and s0 shape is right but base+0x10+offset is
   computed inline instead of kept as a giv); the same plus a `pPrims` pointer
   for the second call (87.98, 380 bytes - a 7th register, s6). The two offset
   givs and the base+0x10 giv coexist only with six registers, which none of
   these spellings reach. Parked body (94.68):
void func_8007819C(void) {
    RECT *pRect;
    u8 *pBuf;
    s32 i;

    BattleInitSlotGraphics(5, 8, 0x2A, 0x70, 0x12, 0, 0);
    ((u8 *)g_BattleDisplayState)[0xB5] = 0;
    BattleInitSlotGraphics(4, 0x20, 0xC8, 0xF4, 0x12, 0, 0);
    ((u8 *)g_BattleDisplayState)[0xB4] = 0;
    for (i = 0; i < 8; i += 2) {
        pBuf = BattleAllocTextBuffer(0x39);
        *(u8 **)(D_800D36C0 + i * 0x60 + 0x60) = pBuf;
        *(u8 **)(g_BattleTextSlotBuffers + i * 0x60 + 0x60) = pBuf;
        pRect = (RECT *)(g_BattleTextSlotRects + i * 0x60);
        pRect->x = 0x3C0;
        pRect->y = (i / 2) * 13;
        pRect->w = 0x3C;
        pRect->h = 0xD;
        *(RECT *)(g_BattleTextSlotRects + i * 0x60 + 0x60) = *pRect;
        func_800780A8(g_BattleTextSlotRects - 0x50 + i * 0x60, i);
        func_800780A8(g_BattleTextSlotRects + i * 0x60 + 0x10, i + 1);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_8007819C);
/* 56.44%: first attempt, body below. The locals are right and they are the
   hard part: retail's frame is 0xC8 with a TIM_IMAGE at sp+0x20 (ReadTIM's
   output) and a 3-element array at sp+0x38 of a 0x18-byte struct, whose SIX
   s32 fields are exactly ResourceHelperGetTexCoords' six out-pointers
   (count, texPageId, clutX, clutY, texPageX, texPageY - see
   system/temp2.c:539 for that signature and member_change_menu/main/misc.c:357
   for the call shape). Retail also keeps running offsets in two stack slots
   (sp+0x90 += 6, sp+0x98 += 0x18) because it runs out of saved registers: six
   address givs (array base and each field) plus i, n, arg0, arg1.
   Unresolved when I ran out of time:
   1. the tex entry stride is 0x460 (sll/subu/sll chain on the index byte) and
      the TIM base is arg0, i.e. OpenTIM((u_long *)(pTimData + n * 0x460));
   2. the two g_BattleActorCharIds[i] loads and the `D_800D3294 != 0` guard: retail's
      second test is `sltiu s2,2` on a counter that starts at -1 and is the
      g_BattleActorCharIds index minus one, i.e. the guard is `i != 0` in some spelling
      gcc re-based onto that biv (0xB replaces the index for the non-first
      entries);
   3. mine came out 356 bytes against retail's 504, so the loop was optimised
      much harder than retail's - the six &coords[i].field arguments must be
      taken through one index expression gcc keeps as six bivs, not as
      coords[i].field.
   Parked body (56.44):
typedef struct {
    s32 count;
    s32 texPageId;
    s32 clutX;
    s32 clutY;
    s32 texPageX;
    s32 texPageY;
} BattleTextureCoords;

void func_80078310(u8 *pTimData, u8 index) {
    TIM_IMAGE tim;
    BattleTextureCoords coords[3];
    s32 i;
    s32 texPageYOffset;
    s32 yOffset;

    i = 0;
    texPageYOffset = 0;
    yOffset = 0;
    for (; i < 3; i++) {
        s32 n;

        if (g_BattleActorCharIds[i] == 0x7F) {
            continue;
        }
        n = g_BattleActorCharIds[i];
        if (D_800D3294 != 0 && i != 0) {
            n = 0xB;
        }
        OpenTIM((u_long *)(pTimData + n * 0x460));
        ReadTIM(&tim);
        ResourceHelperGetTexCoords(g_BattleSpriteArchive, index + i,
                                   &coords[i].count, &coords[i].texPageId,
                                   &coords[i].clutX, &coords[i].clutY,
                                   &coords[i].texPageX, &coords[i].texPageY);
        tim.crect->x = coords[i].clutX;
        tim.crect->y = coords[i].clutY;
        tim.prect->x = coords[i].texPageX + texPageYOffset;
        tim.prect->y = coords[i].texPageY;
        LoadImage(tim.crect, tim.caddr);
        LoadImage(tim.prect, tim.paddr);
        DrawSync(0);
        texPageYOffset += 6;
        yOffset += 0x18;
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
/* 75.45%: with a pEntry local (one load of the global shared by the two uses
   after the sb) the first store and everything from the `sh` tail on is
   byte-identical; the ONLY difference is that retail's two tile `lbu`s are
   scheduled BEFORE the sb while gcc puts them after it. Source order does not
   matter (tile first / tile last / an s32 offset local all compile identically),
   so it is gcc's scheduler placement. Also tried: no pEntry (three loads of the
   global, 48.18), inline tile; this round: `do { } while (0)` around the color
   store (17.12), a volatile color store (57.27), volatile tile loads (62.42),
   tile computed before the pEntry load and after the color store (both 75.45,
   identical output). The 4-minute permuter run found nothing better than 585
   against this body. Retail's order is loads-then-sb, i.e. loads HOISTED above
   an independent store; gcc 2.7.2 sinks them here.
   Parked body (75.45):
void func_800785D4(u8 color, u8 index) {
    u16 tile;
    u8 *pEntry;

    pEntry = g_BattleCommandQueue;
    tile = g_BattleCommandTiles[index * 8] | (D_800D2E61[index * 8] << 8);
    D_800C400B[pEntry[0x2DA] * 0x48] = color;
    pEntry = g_BattleCommandQueue;
    *(u16 *)(D_800C4022 + pEntry[0x2DA] * 0x48) = tile;
    pEntry[0x2DA]++;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_800785D4);

// A battle text slot: one 0x60-byte row of the buffer at g_BattleTextSlotRects, holding the
// rect and the buffer its string is uploaded through (the row is reached through
// three separate symbols: the rect array, the row's buffer pointer and the byte
// SystemRenderStringEntry's result is kept in).
#define BATTLE_TEXT_SLOT_SIZE 0x60
// A slot is only taken while g_BattleTextSlotCount is below this, the number of
// rows the buffer has room for.
#define BATTLE_TEXT_SLOT_MAX 9

void BattleQueueCommandFA(u8 index, u8 color) {
    u8 *pEntry;

    if (g_BattleTextSlotCount < BATTLE_TEXT_SLOT_MAX) {
        *(u8 *)((u8 *)&g_BattleTextSlotFlags + g_BattleTextSlotCount * BATTLE_TEXT_SLOT_SIZE) =
            SystemRenderStringEntry(GetStringEntry(g_BattleTextStringBank, g_BattleTextActorStringIds[index * 8]),
                                    *(u8 **)((u8 *)&g_BattleTextSlotBuffers + g_BattleTextSlotCount * BATTLE_TEXT_SLOT_SIZE), 0x39,
                                    g_BattleTextSlotCount & 1);
        // D_800D36C0 + 0x60 is g_BattleTextSlotBuffers: naming that symbol in both calls makes
        // cse keep the row base in one pseudo across SystemRenderStringEntry and
        // spill it, which costs 8 bytes of frame retail does not have.
        BattleLoadImageAndSync((RECT *)((u8 *)&g_BattleTextSlotRects + g_BattleTextSlotCount * BATTLE_TEXT_SLOT_SIZE),
                      *(u_long **)((u8 *)&D_800D36C0 + g_BattleTextSlotCount * BATTLE_TEXT_SLOT_SIZE + BATTLE_TEXT_SLOT_SIZE));
        D_800C400B[g_BattleCommandQueue[0x2DA] * 0x48] = color;
        D_800C402F[g_BattleCommandQueue[0x2DA] * 0x48] = 0xFA;
        pEntry = g_BattleCommandQueue;
        *(u16 *)(D_800C4022 + pEntry[0x2DA] * 0x48) = g_BattleTextSlotCount++;
        pEntry[0x2DA]++;
    }
}
void BattleQueueCommandF7(u8 color, u8 flag) {
    u8 *pEntry;

    D_800C400B[g_BattleCommandQueue[0x2DA] * 0x48] = flag;
    D_800C402F[g_BattleCommandQueue[0x2DA] * 0x48] = 0xF7;
    pEntry = g_BattleCommandQueue;
    *(u16 *)(D_800C4022 + pEntry[0x2DA] * 0x48) = color;
    pEntry[0x2DA]++;
}
