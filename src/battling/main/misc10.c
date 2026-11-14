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
extern void func_8008E064(void);
extern void func_80083BB4(s32 arg);
extern void func_80083C0C(s32 arg);
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
extern s32 func_8008F530(s32 arg0, s32 arg1);
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
    /* 0x00 */ u8 unk00[0x04];
    /* 0x04 */ u8* unk04;
    /* 0x08 */ u8 unk08[0x04];
    /* 0x0C */ u8* unk0C;
} Misc10TagSrc; // Size: 0x10

typedef struct {
    /* 0x00 */ u32 unk00[0xA];
} Misc10TagPair; // Size: 0x28

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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084B48);

/* 95.98% - frame, the `addu v1,v0` copy, the store and both arms now line up
   (inverted arms: expired case is the fall-through). One instruction off: the
   original sign-extends the decremented value into its own register
   (`sll v0; sra a1,v0,0x10; bgtz a1; nop`), ours lets combine fold the sra
   into the branch (`sll v0; bgtz v0; move a1,zero`). Tried: `D_80092780 -= 4`
   + test the global, s16 local for store and test, s32 local with `(s16)timer`
   and `(s32)timer` casts, int local read back, non-inverted arms (`timer > 0`,
   which also costs the 0x20 frame) - all 95.98%.
void func_80084B48(void) {
    s16 timer;

    timer = D_80092780;
    if (timer != 0) {
        timer = timer - 4;
        D_80092780 = timer;
        if ((s32)timer <= 0) {
            D_80092780 = 0;
            D_80092784 = 0;
            SoundSetAudioManagerMasterVolumeWithFade(D_80092948, 0x100, 0);
            D_8009292C = 0x100;
            func_8008E064();
        } else {
            func_8008E3CC(D_80092938, 0, 1);
        }
    } else {
        D_80092784 = 0;
    }
}
*/

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
    func_80083BB4(0);
    func_80083C0C(5);
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800864B4);

/* 87.29% (556 vs 544 bytes). Structure below is right; left over: the s2/s3/s4
   rotation (mode/scale/arg1 -- global-alloc tie-break), and `x + unk4 * scale`
   must be hoisted into a local before the `scale == 2` if so gcc keeps the
   `mult` and the two stores instead of strength-reducing *2 inside the arm.
   Field access goes through u8* + offset because the two source structs
   (p->unk04, p->unk0C) are read with mixed widths (lh/lbu/lhu at the same
   offsets). The 0x28-byte tail copy is a struct assignment of a u32[0xA] type:
   a u8[0x28] one makes gcc emit an alignment-check path, the original has none.
void func_800864B4(Misc10TagSrc* p, s32 x, s32 y, u8* pOut, s32 mode) {
    s32 scale;
    u16 value;

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

    pOut[3] = 9;
    pOut[7] = 0x2D;
    *(u16*)(pOut + 0xE) = GetClut(*(s16*)(p->unk04 + 0), *(s16*)(p->unk04 + 2));
    *(u16*)(pOut + 0x16) = GetTPage(mode, 0, *(s16*)(p->unk0C + 0), *(s16*)(p->unk0C + 2));
    *(u16*)(pOut + 0x18) = x;
    *(u16*)(pOut + 0x08) = x;
    if (scale == 2) {
        *(u16*)(pOut + 0x20) = x + *(s16*)(p->unk0C + 4) * scale + 1;
    } else {
        *(u16*)(pOut + 0x20) = x + *(s16*)(p->unk0C + 4) * scale;
    }
    value = *(u16*)(pOut + 0x20);
    *(u16*)(pOut + 0x12) = y;
    *(u16*)(pOut + 0x0A) = y;
    *(u16*)(pOut + 0x20) = value;
    *(u16*)(pOut + 0x10) = value;
    value = *(u16*)(p->unk0C + 6) + y;
    *(u16*)(pOut + 0x22) = value;
    *(u16*)(pOut + 0x1A) = value;
    value = *(s16*)(p->unk0C + 0) * scale;
    pOut[0x1C] = value;
    pOut[0x0C] = value;
    value = (*(s16*)(p->unk0C + 0) + *(s16*)(p->unk0C + 4)) * scale;
    pOut[0x24] = value;
    pOut[0x14] = value;
    value = *(u8*)(p->unk0C + 2);
    pOut[0x15] = value;
    pOut[0x0D] = value;
    value = *(u8*)(p->unk0C + 2) + *(u8*)(p->unk0C + 6);
    pOut[0x25] = value;
    pOut[0x1D] = value;
    *(Misc10TagPair*)(pOut + 0x28) = *(Misc10TagPair*)pOut;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800866D4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800868E0);

void BattleAddSharedPrim(void) {
    AddPrim(D_80092938, &D_8009A300[D_800928A0]);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80086E70);

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

    if (func_8008F530(arg0, 0) != 0) {
        return 0;
    }
    value = 0x28000000;
    if (func_8008F530(arg0, 1) != 0) {
        value |= 0xFFFF;
    } else {
        value |= 0xFF;
    }
    pOut[3] = 5;
    *(s32*)(pOut + 4) = value;
    return 1;
}
*/
