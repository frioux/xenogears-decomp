#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"

// .sdata/.bss of the overlay (supplied by splat's asm segments)
extern u8 D_800928A0;
extern s16 D_80092780;
extern s32 D_80092794;
extern s32 D_800928C8;
extern u8 D_8005061C;
extern u8* D_800927B4[];
extern s32 D_800927C4;
extern s32 D_800917F0;
extern s32 D_80092948;
extern u8 D_80097919;
extern u8 D_80099035;
extern u8 D_80091FB0[];
extern u8 D_80092920;
extern u8 D_80099D9D;
extern u8 D_80099D9E;
extern s32 D_80059488;
extern s32 D_8009292C;
extern u8* D_80092938;
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

typedef struct {
    /* 0x000 */ u8 unk000[0x284];
    /* 0x284 */ u32* unk284;
} Misc10Actor; // Size: unknown

typedef struct {
    /* 0x00 */ u8 unk00[0x44];
    /* 0x44 */ s16 unk44;
    /* 0x46 */ s16 unk46;
    /* 0x48 */ s16 unk48;
} Misc10Draw; // Size: unknown

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 pad2;
    /* 0x4 */ s32 unk4;
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
    /* 0x04 */ Misc10Point* unk04;
    /* 0x08 */ u8 unk08[0x04];
    /* 0x0C */ Misc10TexRect* unk0C;
} Misc10TagSrc; // Size: 0x10

extern Misc10Prim D_8009A300[];

s32 BattleQueueActorPrims(Misc10Actor* pActor) {
    u32* pData = pActor->unk284;

    func_8008E3CC((u8*)pData[D_800928A0 + 1] + 8, 0xC0, 0);
    BattleDrawTextPrims((u8*)pData[D_800928A0 + 1] + 4);
    BattleAttachActorPrims(pData);
    BattleAddSharedPrim();
    return 0;
}

void BattleDrawEffectFrame(void) {
    if (D_80092780 != 0) {
        func_8008E120();
        func_8007F258(D_80092938, 0);
        BattleDrawTextPrims(D_80092938);
        func_8008E3CC(D_80092938, 0xC0, 1);
        func_8008BC04();
    }
}

void BattleUpdateEffectTimer(void) {
    s16 timer;

    timer = D_80092780;
    if (timer != 0) {
        timer = timer - 4;
        D_80092780 = timer;
        if (timer <= 0) {
            D_80092780 = 0;
            D_80092784 = 0;
            SoundSetAudioManagerMasterVolumeWithFade(D_80092948, 0x100, 0);
            D_8009292C = 0x100;
            BattleEffectFreeResources();
        } else {
            func_8008E3CC(D_80092938, timer, 1);
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
    BattleAttachModelPart(pBuf, D_80091FB0);
    BattleAppendListEntry(arg0, pDraw);
    pDraw->unk46 = 0xC00;
    pDraw->unk44 = 0;
    pDraw->unk48 = 0x400;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084C88);

void BattleUpdateEffectImage(void) {
    if (D_80092784 != NULL && (D_80059488 & 0x1)) {
        func_8008E120();
    }
}

void BattleSetBothSidesCom(void) {
    D_80092920 &= 0xFE;
    BattleClearScreen(0);
    BattleSetScreenMode(5);
    D_80099D9E = 1;
    D_80099D9D = 1;
    D_800928C8 = 6;
}

void BattleSetBothSidesUser(void) {
    u8 flags = D_80092920;

    D_80099D9E = 0;
    D_80099D9D = 0;
    D_80092920 = flags | 0x1;
}

void BattleLoadSlotEntry(s32 index, s32 value) {
    if (index != 0) {
        D_80097919 = value;
    } else {
        D_80099035 = value;
    }
    if (D_800927B4[index] != NULL) {
        HeapFree(D_800927B4[index]);
        D_800927B4[index] = NULL;
    }
    ArchiveSetIndex(0x30, 1);
    D_800927B4[index] = BattleLoadArchiveEntry(value + 2);
    ArchiveSetIndex(0x30, 0);
}

void BattleFreeSlotEntry(s32 index) {
    ArchiveCdDataSync(0);
    if (D_800927B4[index] != NULL) {
        HeapFree(D_800927B4[index]);
        D_800927B4[index] = NULL;
    }
}

void BattleAllocResourceBuffer(Misc10Resource* pResource, u_int allocFlags) {
    pResource->unk4 = (s32)HeapAlloc(ArchiveDecodeAlignedSize(pResource->unk0), allocFlags);
}

void BattleReturnToMainLoop(void) {
    SoundRemoveSedsEntry(D_800927C4);
    if (D_800917F0 != 0) {
        SoundStopAudioManager(D_80092948);
        SoundDestroyAudioManager(D_80092948);
    }
    func_80088A40();
    ChangeGameState(1);
    DrawSync(0);
    Vsync(2);
    D_8005061C = 1;
    MainLoop(0);
}

s32 BattleCheckModeOver(void) {
    s32 result = 0;

    if (D_80092794 == 5 || D_80092794 == 7) {
        result = 1;
    } else if (D_80092794 == 1 && D_800928C8 == 4 && D_8005061C == 0) {
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
    pPoly->clut = GetClut(p->unk04->x, p->unk04->y);
    pPoly->tpage = GetTPage(mode, 0, p->unk0C->x, p->unk0C->y);
    pPoly->x0 = pPoly->x2 = x;
    right = x + p->unk0C->w * scale;
    if (scale == 2) {
        pPoly->x3 = right + 1;
    } else {
        pPoly->x3 = right;
    }
    pPoly->y0 = pPoly->y1 = y;
    pPoly->x1 = pPoly->x3 = pPoly->x3;
    pPoly->y2 = pPoly->y3 = y + p->unk0C->h;
    pPoly->u0 = pPoly->u2 = p->unk0C->x * scale;
    pPoly->u1 = pPoly->u3 = (p->unk0C->x + p->unk0C->w) * scale;
    pPoly->v0 = pPoly->v1 = p->unk0C->y;
    pPoly->v2 = pPoly->v3 = p->unk0C->y + p->unk0C->h;
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
    pPoly->clut = GetClut(p->unk04->x, p->unk04->y);
    pPoly->tpage = GetTPage(mode, 0, p->unk0C->x, p->unk0C->y);
    pPoly->x0 = pPoly->x2 = x + p->unk0C->w * scale;
    pPoly->x1 = pPoly->x3 = x;
    pPoly->u0 = pPoly->u2 = p->unk0C->x * scale - 1;
    pPoly->u1 = pPoly->u3 = (p->unk0C->x + p->unk0C->w) * scale - 1;
    pPoly->y0 = pPoly->y1 = y;
    pPoly->y2 = pPoly->y3 = y + p->unk0C->h;
    pPoly->v0 = pPoly->v1 = p->unk0C->y;
    pPoly->v2 = pPoly->v3 = p->unk0C->y + p->unk0C->h;
    pPoly[1] = pPoly[0];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800868E0);

void BattleAddSharedPrim(void) {
    AddPrim(D_80092938, &D_8009A300[D_800928A0]);
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
