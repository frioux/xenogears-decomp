#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"

// .sdata/.bss of the overlay (supplied by splat's asm segments)
extern u8 g_BattleDrawBufferIndex;
extern s16 g_BattleEffectFrameTimer;
extern s32 g_BattleScreenMode;
extern s32 g_BattleMode;
extern u8 g_BattleTransitionFlag;
extern u8* g_BattleSlotEntryBuffers[];
extern s32 g_BattleSedsEntryId;
extern s32 g_BattleAudioManagerEnabled;
extern s32 g_BattlingAudioManager;
extern u8 g_BattleSlot1EntryId;
extern u8 g_BattleSlot0EntryId;
extern u8 g_BattleRotatedPartModel[];
extern u8 g_BattleControlFlags;
extern u8 g_BattlePlayer1ComControlled;
extern u8 g_BattlePlayer2ComControlled;
extern s32 g_SpuChannelMask;
extern s32 g_BattleMotionSpeedScale;
extern u8* g_BattleRootOrderingTable;
extern u8* D_80092784;


extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveCdDataSync(s32 mode);
extern void func_8008E120(void);
extern void func_8007F258(void* pOt, s32 arg);
extern void BattleDrawTextPrims(void* pOt);
extern void func_8008E3CC(void* pOt, s32 arg1, s32 arg2);
extern void func_8008BC04(void);
extern void SoundSetAudioManagerMasterVolumeWithFade(s32 arg0, s32 arg1, s32 arg2);
extern void BattleEffectFreeResources(void);
extern void BattleClearScreen(s32 arg);
extern void BattleSetScreenMode(s32 arg);
extern void SoundRemoveSedsEntry(s32 arg);
extern void SoundStopAudioManager(s32 arg);
extern void SoundDestroyAudioManager(s32 arg);
extern void func_80088A40(void);
extern void ChangeGameState(s32 state);
extern void Vsync(s32 mode);
extern void MainLoop(s32 arg);
extern s32 BattleCreateObject(void);
extern s32 BattleCreateModelPart(void);
extern void BattleSetNodeModelPart(void* pA, void* pB);
extern void BattleAttachModelPart(void* pA, void* pB);
extern void BattleAppendListEntry(s32 handle, void* pB);
extern void* BattleLoadArchiveEntry(s32 arg);
extern void BattleAttachActorPrims(void* pData);
extern s32 BattleCheckActorBelowBreakThreshold(s32 arg0, s32 arg1);
void BattleAddSharedPrim(void);

// The header at the start of the block pData->pHeader points at, once
// BattleRelocateBlockPointers has fixed up its +0x30 offset.
typedef struct {
    /* 0x00 */ u8 unk00[0xE];
    /* 0x0E */ u8 visiblePartCount;
    /* 0x0F */ u8 unk0F;
    /* 0x10 */ u8 effectColorR;
    /* 0x11 */ u8 effectColorG;
    /* 0x12 */ u8 effectColorB;
    /* 0x13 */ u8 unk13[0x1D];
    /* 0x30 */ s32 visiblePartIndicesOffset; // byte offset of the sprite table from the block start
} Misc10SpriteHeader; // Size: 0x34

// One 4-byte action slot of the block's +0x18 array; only the flag byte is read.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 flags;
} Misc10ActionSlot; // Size: 0x4

// One 0x20-byte entry of the D_80092874 table (the same table src/battling/
// main/misc6.c calls BattleFilterEntry); +0xA holds one flag byte per action
// slot and this unit counts 14 of them.
typedef struct {
    /* 0x00 */ u8 unk00[0x4];
    /* 0x04 */ s16 requiredProgress; // the word src/battling/main/misc6.c reads as the entry's progress gate
    /* 0x06 */ u8 unk06[0x4];
    /* 0x0A */ u8 slotFlags[0xE];
    /* 0x18 */ u8 unk18[0x8];
} Misc10ActionRow; // Size: 0x20

// The relocated resource block pData points at: the layout
// BattleRelocateBlockPointers in src/battling/main/misc14.c fixes up, so each
// pointer below is a byte offset from the block start before that call.
typedef struct {
    /* 0x00 */ u8 unk00[0x10];
    /* 0x10 */ Misc10SpriteHeader* pHeader;
    /* 0x14 */ u8* pPartTimings;
    /* 0x18 */ Misc10ActionSlot* pActions;
    /* 0x1C */ u8 unk1C[0x4];
    /* 0x20 */ u8* pImage;
} Misc10Block; // Size: 0x24

typedef struct {
    /* 0x000 */ u8 unk000[0x58];
    /* 0x058 */ u8* pTimData;
    /* 0x05C */ u8* pModelLink;
    /* 0x060 */ u8* pObject;
    /* 0x064 */ u8 unk064[0x18];
    /* 0x07C */ u8* pPartTimings;
    /* 0x080 */ Misc10ActionSlot* pActions;
    /* 0x084 */ u8 unk084[0x4C];
    /* 0x0D0 */ u32 flags;
    /* 0x0D4 */ u8 unk0D4[0x4];
    /* 0x0D8 */ u8* pTarget;
    /* 0x0DC */ u8 unk0DC[0x1A8];
    /* 0x284 */ u32* pDisplayWork;
    /* 0x288 */ u8 unk288[0x674];
    /* 0x8FC */ u8* pScript;
    /* 0x900 */ u8* unk900;
    /* 0x904 */ u8* pVisiblePartIndices;
    /* 0x908 */ u8 visiblePartCount;
    /* 0x909 */ u8 typeId;
    /* 0x90A */ u8 typeFlags;
    /* 0x90B */ u8 unk90B[0x6];
    /* 0x911 */ u8 flaggedActionCount;
    /* 0x912 */ u8 unflaggedActionCount;
    /* 0x913 */ u8 unk913[0xCC1];
    /* 0x15D4 */ u8 effectColorR;
    /* 0x15D5 */ u8 effectColorG;
    /* 0x15D6 */ u8 effectColorB;
    /* 0x15D7 */ u8 unk15D7[0x29];
    /* 0x1600 */ Misc10ActionRow* pActionRow;
} Misc10Actor; // Size: 0x1604

typedef struct {
    /* 0x00 */ u8 unk00[0x44];
    /* 0x44 */ s16 rotX;
    /* 0x46 */ s16 rotY;
    /* 0x48 */ s16 rotZ;
} Misc10Draw; // Size: unknown

typedef struct {
    /* 0x0 */ u16 entryIndex;
    /* 0x2 */ u16 pad2;
    /* 0x4 */ s32 buffer;
} Misc10Resource; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 unk00[0x310];
} Misc10Prim; // Size: 0x310

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
} Misc10Point; // Size: 0x4

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 w;
    /* 0x6 */ u16 h;
} Misc10TexRect; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 unk00[0x04];
    /* 0x04 */ Misc10Point* crect;
    /* 0x08 */ u8 unk08[0x04];
    /* 0x0C */ Misc10TexRect* prect;
} Misc10TagSrc; // Size: 0x10

extern Misc10ActionRow* D_80092874;
extern u8 g_BattleActor1[];
extern u8 g_BattleActor0[];
extern Misc10Prim g_BattleSharedPrimBuffer[];

extern void BattleSetTpageAndClut(s16 x, s16 y, s16 clutX, s16 clutY);
extern void BattleClearTpageAndClut(void);
extern void* BattleRelocateBlockPointers(void* pData);
extern u8* func_8008B38C(u8* pArg);
extern u8* BattleSetActorSoundTable(Misc10Actor* pActor);

s32 BattleQueueActorPrims(Misc10Actor* pActor) {
    u32* pData = pActor->pDisplayWork;

    func_8008E3CC((u8*)pData[g_BattleDrawBufferIndex + 1] + 8, 0xC0, 0);
    BattleDrawTextPrims((u8*)pData[g_BattleDrawBufferIndex + 1] + 4);
    BattleAttachActorPrims(pData);
    BattleAddSharedPrim();
    return 0;
}

void BattleDrawEffectFrame(void) {
    if (g_BattleEffectFrameTimer != 0) {
        func_8008E120();
        func_8007F258(g_BattleRootOrderingTable, 0);
        BattleDrawTextPrims(g_BattleRootOrderingTable);
        func_8008E3CC(g_BattleRootOrderingTable, 0xC0, 1);
        func_8008BC04();
    }
}

void BattleUpdateEffectTimer(void) {
    s16 timer;

    timer = g_BattleEffectFrameTimer;
    if (timer != 0) {
        timer = timer - 4;
        g_BattleEffectFrameTimer = timer;
        if (timer <= 0) {
            g_BattleEffectFrameTimer = 0;
            D_80092784 = 0;
            SoundSetAudioManagerMasterVolumeWithFade(g_BattlingAudioManager, 0x100, 0);
            g_BattleMotionSpeedScale = 0x100;
            BattleEffectFreeResources();
        } else {
            func_8008E3CC(g_BattleRootOrderingTable, timer, 1);
        }
    } else {
        D_80092784 = 0;
    }
}

void BattleActorCreateRotatedPart(Misc10Actor* pActor) {
    u8* pScript;
    s32 arg0;
    Misc10Draw* pDraw;
    u8* pBuf;

    pScript = *(u8**)((u8*)pActor + 0x5C);
    pScript = *(u8**)(pScript + 4);
    pScript = *(u8**)(pScript + 4);
    arg0 = *(s32*)(pScript + 0x30);
    pDraw = (Misc10Draw*)BattleCreateObject();
    pBuf = (u8*)BattleCreateModelPart();
    BattleSetNodeModelPart(pDraw, pBuf);
    BattleAttachModelPart(pBuf, g_BattleRotatedPartModel);
    BattleAppendListEntry(arg0, pDraw);
    pDraw->rotY = 0xC00;
    pDraw->rotX = 0;
    pDraw->rotZ = 0x400;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084C88);

/* 89.37% objdiff, 832 bytes against retail 840. Structure, call order, the
   prologue save order and every s-register except one pair match, and the
   jump table comes out at the unit rodata start where retail has it. Because
   the stub owns a jtbl_ table, this function can never print MATCH in
   diff_func.sh (the original dump stops at the first case label, which gas
   makes a symbol when .rodata references it) - objdiff% and check_bin.sh are
   the only judges.
   What is still wrong:
   1. Best body below keeps pActor->pModelLink = pNode AFTER the switch, which is
      what makes gcc hand pActor s0 / pNode s1 (global-alloc priority order
      pActor, pData, arg2, pNode, with s1 already reserved by the flip-loop
      buffer through local-alloc). Moving that store to its retail position
      (middle block, before the pActionRow store) makes pNode block-local, so
      local-alloc takes s0 for it and pActor falls to s1 - the whole function
      then reads 86% with s0/s1 swapped. That trade-off is the whole gap.
   2. pActor->typeFlags |= 1; pActor->typeFlags |= 2; (retail case 35/36) is cse-d
      into one |= 3 by gcc; retail has two lbu/ori/sb pairs. Untried:
      bitfields, or a union member for that byte.
   3. gcc cross-jumps the three case-body sb v0,0x90A into one shared tail;
      retail has one per case (2 instructions, the size difference).
   Key trick that fixed the prologue: assign pSrc = pData->pImage + 0x200
   BEFORE HeapAlloc in the arg2 == 0 branch - retail keeps it in s0 across
   the HeapAlloc call (its addiu lands in the call delay slot), which is what
   pushes the heap buffer to s1 and pData/arg2 to s2/s3.
   void func_80084C88(Misc10Actor* pActor, Misc10Block* pData, s32 arg2) {
       RECT rect;
       Misc10SpriteHeader* pHeader;
       Misc10ActionRow* pRow;
       u8* pNode;
       u8* pObj;
       u8* pBuf;
       u8* pSrc;
       u8* pDst;
       u8* pRead;
       s32 row;
       s32 col;
       s32 i;

       pActor->flags = (pActor->flags & 0xF7FFFFFF) | ((arg2 & 1) << 27);
       if (arg2 != 0) {
           BattleSetTpageAndClut(0x380, 0, 0, 0x1FE);
           pActor->pTarget = g_BattleActor1;
       } else {
           BattleSetTpageAndClut(0x3C0, 0, 0, 0x1FF);
           pActor->pTarget = g_BattleActor0;
       }
       BattleRelocateBlockPointers(pData);
       pNode = func_8008B38C((u8*)pData);
       pObj = (u8*)BattleCreateObject();
       pActor->pObject = pObj;
       BattleAppendListEntry(pObj, pNode);
       pActor->pActionRow = &D_80092874[pActor->typeId];
       pActor->typeFlags = 0;
       switch (pActor->typeId) {
       case 3:
       case 14:
       case 27:
       case 34:
       case 39:
       case 41:
       case 42:
           pActor->typeFlags |= 4;
           break;
       case 13:
           BattleClearTpageAndClut();
           BattleActorCreateRotatedPart(pActor);
           break;
       case 35:
       case 36:
           pActor->typeFlags |= 1;
           pActor->typeFlags |= 2;
           break;
       case 38:
           pActor->typeFlags |= 2;
           break;
       }
       pActor->pModelLink = pNode;
       BattleSetActorSoundTable(pActor);
       pHeader = pData->pHeader;
       pActor->pScript = (u8*)pHeader;
       pActor->pPartTimings = pData->pPartTimings;
       pActor->unk900 = (u8*)pHeader + 0x34;
       pActor->pActions = pData->pActions;
       pActor->pVisiblePartIndices = (u8*)pHeader + pHeader->visiblePartIndicesOffset;
       pActor->visiblePartCount = pHeader->visiblePartCount;
       pActor->effectColorR = pHeader->effectColorR;
       pActor->effectColorG = pHeader->effectColorG;
       pActor->effectColorB = pHeader->effectColorB;
       pActor->flaggedActionCount = 0;
       pActor->unflaggedActionCount = 0;
       pRow = pActor->pActionRow;
       for (i = 0; i < 0xE; i++) {
           if (pRow->slotFlags[i] != 0) {
               if (pActor->pActions[i].flags != 0) {
                   pActor->flaggedActionCount++;
               } else {
                   pActor->unflaggedActionCount++;
               }
           }
       }
       rect.x = 0;
       rect.y = arg2 + 0x1F6;
       rect.w = 0x100;
       rect.h = 1;
       LoadImage(&rect, (u_long*)pData->pImage);
       rect.x = arg2 * 0x10 + 0x380;
       rect.y = 0x100;
       rect.w = 0xB;
       rect.h = 0x16;
       if (arg2 != 0) {
           LoadImage(&rect, (u_long*)(pData->pImage + 0x200));
       } else {
           pSrc = pData->pImage + 0x200;
           pBuf = (u8*)HeapAlloc(0x1E4, 0);
           pDst = pBuf;
           for (row = 0; row < 0x16; row++) {
               pRead = pSrc + 0x15;
               for (col = 0; col < 0x16; col++) {
                   *pDst++ = *pRead--;
               }
               pSrc += 0x16;
           }
           LoadImage(&rect, (u_long*)pBuf);
           HeapDelayedFree(pBuf, 1);
       }
       rect.x = 0x3A0;
       rect.y = arg2 * 8 + 0x100;
       rect.w = 0x10;
       rect.h = 8;
       LoadImage(&rect, (u_long*)(pData->pImage + 0x3E4));
   }
*/

void BattleUpdateEffectImage(void) {
    if (D_80092784 != NULL && (g_SpuChannelMask & 0x1)) {
        func_8008E120();
    }
}

void BattleSetBothSidesCom(void) {
    g_BattleControlFlags &= 0xFE;
    BattleClearScreen(0);
    BattleSetScreenMode(5);
    g_BattlePlayer2ComControlled = 1;
    g_BattlePlayer1ComControlled = 1;
    g_BattleMode = 6;
}

void BattleSetBothSidesUser(void) {
    u8 flags = g_BattleControlFlags;

    g_BattlePlayer2ComControlled = 0;
    g_BattlePlayer1ComControlled = 0;
    g_BattleControlFlags = flags | 0x1;
}

void BattleLoadSlotEntry(s32 index, s32 value) {
    if (index != 0) {
        g_BattleSlot1EntryId = value;
    } else {
        g_BattleSlot0EntryId = value;
    }
    if (g_BattleSlotEntryBuffers[index] != NULL) {
        HeapFree(g_BattleSlotEntryBuffers[index]);
        g_BattleSlotEntryBuffers[index] = NULL;
    }
    ArchiveSetIndex(0x30, 1);
    g_BattleSlotEntryBuffers[index] = BattleLoadArchiveEntry(value + 2);
    ArchiveSetIndex(0x30, 0);
}

void BattleFreeSlotEntry(s32 index) {
    ArchiveCdDataSync(0);
    if (g_BattleSlotEntryBuffers[index] != NULL) {
        HeapFree(g_BattleSlotEntryBuffers[index]);
        g_BattleSlotEntryBuffers[index] = NULL;
    }
}

void BattleAllocResourceBuffer(Misc10Resource* pResource, u_int allocFlags) {
    pResource->buffer = (s32)HeapAlloc(ArchiveDecodeAlignedSize(pResource->entryIndex), allocFlags);
}

void BattleReturnToMainLoop(void) {
    SoundRemoveSedsEntry(g_BattleSedsEntryId);
    if (g_BattleAudioManagerEnabled != 0) {
        SoundStopAudioManager(g_BattlingAudioManager);
        SoundDestroyAudioManager(g_BattlingAudioManager);
    }
    func_80088A40();
    ChangeGameState(1);
    DrawSync(0);
    Vsync(2);
    g_BattleTransitionFlag = 1;
    MainLoop(0);
}

s32 BattleCheckModeOver(void) {
    s32 result = 0;

    if (g_BattleScreenMode == 5 || g_BattleScreenMode == 7) {
        result = 1;
    } else if (g_BattleScreenMode == 1 && g_BattleMode == 4 && g_BattleTransitionFlag == 0) {
        result = 1;
    }
    return result;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800852C4);

void BattleCopyVertexOffsetAdd(u16* pSrc, u16* pDst) {
    pDst[0] = pSrc[0] + 0x18;
    pDst[1] = pSrc[1] + 0x6;
    pDst[0] += 0x4F;
}

void BattleCopyVertexOffsetFlip(u16* pSrc, u16* pDst) {
    pDst[0] = 0x8B - pSrc[0];
    pDst[1] = 0x20 - pSrc[1];
    pDst[0] += 0x4F;
}

void BattleSetVertexXForFlipped(s32 flag, u16* pDst, s32 value) {
    if (flag != 0) {
        *pDst = 0xDA - value;
    } else {
        *pDst = 0x67 + value;
    }
}

void BattleSetVertexYForFlipped(s32 flag, u16* pDst, s32 value) {
    if (flag != 0) {
        *pDst = 0x20 - value;
    } else {
        *pDst = 0x6 + value;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80085EC8);

void FieldSoundResetActorEffects(Misc10TagSrc* p, s32 x, s32 y, POLY_FT4* pPoly, s32 mode) {
    s32 scale;
    s32 right;

    switch (mode) {
    case 0:
        scale = 4;
        break;
    case 1:
        scale = 2;
        break;
    case 2:
        scale = 1;
        break;
    }

    ((u8*)pPoly)[3] = 9;
    pPoly->code = 0x2D;
    pPoly->clut = GetClut(p->crect->x, p->crect->y);
    pPoly->tpage = GetTPage(mode, 0, p->prect->x, p->prect->y);
    pPoly->x0 = pPoly->x2 = x;
    right = x + p->prect->w * scale;
    if (scale == 2) {
        pPoly->x3 = right + 1;
    } else {
        pPoly->x3 = right;
    }
    pPoly->y0 = pPoly->y1 = y;
    pPoly->x1 = pPoly->x3 = pPoly->x3;
    pPoly->y2 = pPoly->y3 = y + p->prect->h;
    pPoly->u0 = pPoly->u2 = p->prect->x * scale;
    pPoly->u1 = pPoly->u3 = (p->prect->x + p->prect->w) * scale;
    pPoly->v0 = pPoly->v1 = p->prect->y;
    pPoly->v2 = pPoly->v3 = p->prect->y + p->prect->h;
    pPoly[1] = pPoly[0];
}


void BattleBuildFlippedTexturedQuad(Misc10TagSrc* p, s32 x, s32 y, POLY_FT4* pPoly, s32 mode) {
    s32 scale;

    switch (mode) {
    case 0:
        scale = 4;
        break;
    case 1:
        scale = 2;
        break;
    case 2:
        scale = 1;
        break;
    }

    ((u8*)pPoly)[3] = 9;
    pPoly->code = 0x2D;
    pPoly->clut = GetClut(p->crect->x, p->crect->y);
    pPoly->tpage = GetTPage(mode, 0, p->prect->x, p->prect->y);
    pPoly->x0 = pPoly->x2 = x + p->prect->w * scale;
    pPoly->x1 = pPoly->x3 = x;
    pPoly->u0 = pPoly->u2 = p->prect->x * scale - 1;
    pPoly->u1 = pPoly->u3 = (p->prect->x + p->prect->w) * scale - 1;
    pPoly->y0 = pPoly->y1 = y;
    pPoly->y2 = pPoly->y3 = y + p->prect->h;
    pPoly->v0 = pPoly->v1 = p->prect->y;
    pPoly->v2 = pPoly->v3 = p->prect->y + p->prect->h;
    pPoly[1] = pPoly[0];
}

extern POLY_FT4 D_80095698[16];
extern DR_TPAGE D_80095918[4];
extern u8 D_80091814[];
extern u8 D_80092860;
extern u8 D_80092864;

// Packet word store, as include/system/font.h's setXY0Fast / setUV0Fast family.
#define Misc10SetXY0(p, _x0, _y0) *(s32*)&(p)->x0 = (_x0) | ((_y0) << 0x10)
#define Misc10SetXY1(p, _x1, _y1) *(s32*)&(p)->x1 = (_x1) | ((_y1) << 0x10)
#define Misc10SetUV0(p, _u0, _v0) *(u16*)&(p)->u0 = (_u0) | ((_v0) << 0x8)
#define Misc10SetRGBC(p, _r0, _g0, _b0, _code) \
    *(s32*)&(p)->r0 = (_r0) | ((_g0) << 0x8) | ((_b0) << 0x10) | ((_code) << 0x18)

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800868E0);

/* 71.59% (1228 bytes against retail 1348), 3 attempts, parked for time.
   The unit's data at D_80095698 is POLY_FT4[16] (0x280 bytes, ending exactly at
   D_80095918) followed by DR_TPAGE[4]; every retail offset lands on a POLY_FT4
   field (+3 len, +4 r0/g0/b0/code, +8 x0/y0, +0xC u0/v0, +0xE clut, +0x10
   x1/y1), so the packets are POLY_FT4s. The word/halfword stores retail has
   (sw 0x090007 at +8, sh 0 at +0xC, sw 0x160016 at +0x10, sw 0x2C000080 at +4)
   are the setXY0Fast/setUV0Fast family from include/system/font.h: gcc 2.7.2
   does NOT merge adjacent short/byte stores, so they must be written as word
   stores (the macros above). This body has all the values, both TIM sections,
   the SetDrawTPage/DR_TPAGE pairs, the three GetClut calls, both blocks of
   sprite setups and all five 0x28-byte copies in retail order.
   What is left: (1) about 30 instructions of register/loop-setup difference -
   retail keeps the POLY base in s1 and forms the copy pointers base-relative
   (addiu s2,s1,0xA0 / addiu t0,s1,0xF0) while keeping the scalar field stores
   ABSOLUTE (lui $at,%hi/%lo), and its copies do not re-materialise the base per
   copy; (2) retail's frame is 0x50 with s3-s0 saved (pActor in s3), ours 0x48
   with s2-s0, i.e. we are one callee-saved register short; (3) the two
   D_80092860/D_80092864 lbu's sit earlier in retail than the stores.
   Tried: all-array-symbol accesses (55.28%, copies also go absolute), all
   pointer-local accesses (64.58%, scalar stores then go base-relative).
   The form below is the mixed one.
   void func_800868E0(Misc10Actor* pActor) {
    TIM_IMAGE tim;
    RECT rect;
    POLY_FT4* p;
    u16* pClut;

    p = D_80095698;
    OpenTIM((u_long*)pActor->pTimData);
    ReadTIM(&tim);
    pClut = (u16*)tim.caddr;
    pClut[0] = 0;
    pClut[1] = 0x8000;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    p[5] = p[4];
    FieldSoundResetActorEffects((Misc10TagSrc*)&tim, 6, 7, &p[0], 0);
    BattleBuildFlippedTexturedQuad((Misc10TagSrc*)&tim, 0x13A - tim.prect->w * 4, 7, &p[2], 0);
    SetDrawTPage(&D_80095918[0], 0, 1, GetTPage(1, 0, 0x380, 0x100));
    D_80095918[1] = D_80095918[0];
    SetDrawTPage(&D_80095918[2], 0, 1, GetTPage(0, 0, 0x380, 0x100));
    D_80095918[3] = D_80095918[2];
    setlen(&D_80095698[4], 4);
    D_80095698[4].code = 0x65;
    Misc10SetXY0(&D_80095698[4], 7, 9);
    Misc10SetUV0(&D_80095698[4], 0, 0);
    Misc10SetXY1(&D_80095698[4], 0x16, 0x16);
    D_80095698[4].clut = GetClut(0, 0x1F6);
    p[6] = p[5];
    Misc10SetXY0(&D_80095698[6], 0x123, 0);
    D_80095698[6].u0 = 0x20;
    D_80095698[6].clut = GetClut(0, 0x1F7);
    OpenTIM((u_long*)pActor->pModelLink);
    ReadTIM(&tim);
    LoadImage(tim.prect, tim.paddr);
    FieldSoundResetActorEffects((Misc10TagSrc*)&tim, 6, 0x20, &p[12], 0);
    BattleBuildFlippedTexturedQuad((Misc10TagSrc*)&tim, 0x13A - tim.prect->w * 4, 0x20, &p[14], 0);
    Misc10SetXY1(&D_80095698[8], 0x40, 8);
    Misc10SetRGBC(&D_80095698[12], 0x80, 0, 0, 0x2C);
    Misc10SetRGBC(&D_80095698[13], 0x80, 0, 0, 0x2C);
    Misc10SetRGBC(&D_80095698[14], 0x80, 0, 0, 0x2C);
    Misc10SetRGBC(&D_80095698[15], 0x80, 0, 0, 0x2C);
    setlen(&D_80095698[12], 9);
    setlen(&D_80095698[13], 9);
    setlen(&D_80095698[14], 9);
    setlen(&D_80095698[15], 9);
    setlen(&D_80095698[8], 4);
    D_80095698[8].code = 0x65;
    Misc10SetUV0(&D_80095698[8], 0x80, 0);
    D_80092860 = D_80095698[12].v0;
    D_80092864 = D_80095698[14].v0;
    D_80095698[13].clut = D_80095698[0].clut;
    D_80095698[12].clut = D_80095698[0].clut;
    D_80095698[15].clut = D_80095698[0].clut;
    D_80095698[14].clut = D_80095698[0].clut;
    D_80095698[8].clut = GetClut(0x3A0, 0x110);
    Misc10SetXY0(&D_80095698[8], 0x1E, 0xB);
    p[10] = p[8];
    Misc10SetUV0(&D_80095698[10], 0x80, 8);
    Misc10SetXY0(&D_80095698[10], 0xE3, 0x15);
    p[9] = p[8];
    p[11] = p[10];
    setRECT(&rect, 0x3A0, 0x110, 0x10, 1);
    LoadImage(&rect, (u_long*)D_80091814);
}
*/
void BattleAddSharedPrim(void) {
    AddPrim(g_BattleRootOrderingTable, &g_BattleSharedPrimBuffer[g_BattleDrawBufferIndex]);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80086E70);

/* 80.41% - content matches, layout does not: retail puts the value<0x38 block
   between the first prim and the inner if (as the else of an outer if whose
   else exits), ours puts it last. Also retail computes the tapered x3 as
   `addiu v0,s3,-4 ; subu a2,v0,q` where ours folds to `value - (q + 4)`.
   Tried: if (value >= 0x38) {..} else {small} (80.41%, this one), early
   `return` in the small block (56.79%, +s5, 0x30 frame), `return` in the else
   (72.36%), a `quarter` local for the division (no change).
void func_80086E70(u8* pOt, u8* pBase, s32 value, s32 flag) {
    if (value >= 0x38) {
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0xC), 0x38);
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x14), 0x34);
        AddPrim(pOt, pBase);
        if (value >= 0x48) {
            BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x24), 0x48);
            BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x2C), 0x40);
            BattleSetVertexYForFlipped(flag, (u16*)(pBase + 0x2E), 0x10);
            AddPrim(pOt, pBase + 0x18);
            BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x3C), value);
            BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x44), value - 8);
            AddPrim(pOt, pBase + 0x30);
        } else {
            BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x24), value);
            BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x2C),
                                       (value - 4) - (value - 0x38) / 4);
            BattleSetVertexYForFlipped(flag, (u16*)(pBase + 0x2E),
                                       (value - 0x38) / 2 + 8);
            AddPrim(pOt, pBase + 0x18);
        }
    } else {
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0xC), value);
        BattleSetVertexXForFlipped(flag, (u16*)(pBase + 0x14), value - 4);
        AddPrim(pOt, pBase);
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80086FF8);

/* 84.25% - the two value branches each re-materialise `li 0x28000000` (gcc
   rematerialises the constant per branch); the original hoists one lui into
   the beqz delay slot and keeps `ori v1,v1,imm` in each arm, plus the 1 return
   in v0 (ours puts value in a0 and 5 in v1). Tried: full constants in each
   arm, `value = 0x28000000 | (c ? 0xFFFF : 0xFF)`, the ternary inside the
   store, `value |= c ? 0xFFFF : 0xFF`, u32 value, `0x28 << 24`, inverted
   condition - all 82-84%, and -G0 / 2.6.0 score the same.
s32 func_80086FF8(s32 arg0, u8* pOut) {
    s32 value;

    if (BattleCheckActorBelowBreakThreshold(arg0, 0) != 0) {
        return 0;
    }
    value = 0x28000000;
    if (BattleCheckActorBelowBreakThreshold(arg0, 1) != 0) {
        value |= 0xFFFF;
    } else {
        value |= 0xFF;
    }
    pOut[3] = 5;
    *(s32*)(pOut + 4) = value;
    return 1;
}
*/
