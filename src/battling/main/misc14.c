#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/inline_c.h"
#include "system/memory.h"
#include "system/archive.h"

// .bss/.sdata of the overlay (supplied by splat's asm segments)
extern u8 D_800928A0;
extern u32 D_80092818[];
extern s32 D_80092810;
extern s32 D_80092820;
extern u32 D_800928E4;
extern s32 D_80050100;
extern s32 D_80050104;
extern s32 D_80091C30;
extern s32 D_80096D88;
extern s32 D_80096D8C;
extern s32 D_80092914;
extern u8 *D_80092938;
extern void *D_80092868;

extern void func_80030A30(s32 index, void *pLight);
extern u32 func_800405E4(void);
extern void func_8008BB3C(void *pCtx);

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u8 unk8[4];
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u16 pad12;
} Misc14Entry; // Size: 0x14

void func_8008A618(void) {
    D_80092810 = 1;
}

void func_8008A62C(void) {
    D_80092810 = 0;
}

extern void ModelRenderPacket(void *pPart, void *pPacket, u_long *pOT, s32 flags);

void func_8008A63C(u8 *pActor) {
    D_80050104 = 0;
    if (D_80092810 != 0) {
        if (*(u32 *)pActor & 0x10) {
            gte_SetBackColor(pActor[0x18], pActor[0x19], pActor[0x1A]);
        } else {
            gte_SetBackColor(0x40, 0x40, 0x40);
        }
    }
    ModelRenderPacket(*(void **)(pActor + 0x14), *(void **)(pActor + D_800928A0 * 4 + 8),
                      (u_long *)D_800928E4, *(s32 *)(pActor + 4));
}

typedef struct {
    u8 b[4];
} Misc14Quad; // Size: 0x4

void func_8008A6F8(u8 *pSrc) {
    u8 *pDst = (u8 *)&D_80092818[D_800928A0];

    if (pSrc[0] == pDst[0] && pSrc[1] == pDst[1] && pSrc[2] == pDst[2]) {
        D_80092914 = 0;
    } else {
        D_80092914 = 1;
        *(Misc14Quad *)pDst = *(Misc14Quad *)pSrc;
        pDst[3] = 0x20;
    }
}

void func_8008A78C(u8 *pActor) {
    s32 index = D_800928A0;
    u8 *pModel = *(u8 **)(pActor + 4);
    u8 *pData = *(u8 **)(pModel + 0xC);
    s32 count = *(s16 *)(pData + 2);
    Misc14Entry *pEntry = *(Misc14Entry **)(pData + index * 4 + 0x10);
    u32 value = D_80092818[index];

    while (--count != -1) {
        pEntry->unk4 = value;
        pEntry++;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008A7E0);

void func_8008ABAC(u8 *pActor) {
    func_80030A30(0, *(u8 **)(*(u8 **)pActor + 4));
    func_80030A30(1, *(u8 **)(*(u8 **)(pActor + 4) + 4));
    func_80030A30(2, *(u8 **)(*(u8 **)(pActor + 8) + 4));
}

void func_8008AC0C(u8 *pActor) {
    s32 n = *(s16 *)(pActor + 0x14);

    ClearOTagR(*(u_long **)(pActor + D_800928A0 * 4 + 4), n);
    D_800928E4 = *(u_long *)(pActor + D_800928A0 * 4 + 4);
    D_80050100 = *(u8 *)(pActor + 0x17);
}

void func_8008AC7C(s32 arg0) {
    D_80091C30 = arg0;
}

void func_8008AC8C(void) {
    D_80092820 = GetRCnt(0xF2000001);
}

void func_8008ACB8(s32 arg0) {
    u8 *pCtx = (u8 *)D_80091C30;
    s32 timeout;
    s32 start;
    s32 diff;
    s32 i;
    u32 *pOT;
    u32 *pPrim;
    u32 *pNext;

    if (pCtx == 0) {
        return;
    }
    start = D_80092820;
    D_80091C30 = 0;
    if (arg0 != 0) {
        timeout = ((arg0 << 4) - arg0) << 4;
    } else {
        timeout = 0xC0;
    }
    i = *(s16 *)(pCtx + 0x14) - 1;
    if (i < 4) {
        return;
    }
    do {
        diff = GetRCnt(0xF2000001) - start;
        if (diff < 0) {
            diff += 0x10000;
        }
        if (timeout < diff) {
            break;
        }
        pOT = *(u32 **)(pCtx + D_800928A0 * 4 + 4);
        pPrim = (u32 *)pOT[i];
        pNext = (u32 *)((*pPrim & 0xFFFFFF) + 0x80000000);
        if (*(u8 *)((u8 *)pNext + 3) == 0) {
            do {
                if (i < 5) {
                    break;
                }
                pNext = (u32 *)((*pNext & 0xFFFFFF) + 0x80000000);
                i--;
            } while (*(u8 *)((u8 *)pNext + 3) == 0);
            *pPrim = (*pPrim & 0xFF000000) | ((u32)pNext & 0xFFFFFF);
        }
        i--;
    } while (i >= 4);
}

extern void func_800453E8(void *pDrmode, void *pRect);
extern void func_8004546C(void *pDrmode, void *pRect);

void func_8008AE1C(u8 *pActor) {
    func_8008AC7C((s32)pActor);
    AddPrims(D_80092938, *(void **)(pActor + D_800928A0 * 4 + 0xC), *(void **)(pActor + D_800928A0 * 4 + 4));
    if ((*(u8 *)(pActor + 0x16) & 4) == 0) {
        func_800453E8(pActor + (D_800928A0 * 12 + 0x18), D_80092868);
    }
    if ((*(u8 *)(pActor + 0x16) & 8) == 0) {
        func_8004546C(pActor + (D_800928A0 * 12 + 0x30), (u8 *)D_80092868 + 8);
    }
    if (*(u8 *)(pActor + 0x16) & 0x10) {
        AddPrim(D_80092938, pActor + (D_800928A0 * 16 + 0x48));
    }
    AddPrim(D_80092938, pActor + (D_800928A0 * 12 + 0x30));
    AddPrim(D_80092938, pActor + (D_800928A0 * 12 + 0x18));
}

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u32 unk8;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ u32 unk18;
    /* 0x1C */ u32 unk1C;
    /* 0x20 */ u32 unk20;
    /* 0x24 */ u32 unk24;
} Misc14Reloc; // Size: 0x28

extern void BattleLoadImageWithTpageClut(void *pImg);

Misc14Reloc *func_8008AF6C(Misc14Reloc *pData) {
    s32 delta = (s32)pData - (s32)pData->unk1C;
    u32 *pSlot;
    u32 i;

    pData->unk1C = (u32)pData;
    pData->unk0 += delta;
    pData->unk4 += delta;
    pData->unk10 += delta;
    pData->unk14 += delta;
    pData->unk18 += delta;
    pData->unk20 += delta;
    pData->unk24 += delta;
    if (pData->unkC != 0) {
        pData->unkC += delta;
        BattleLoadImageWithTpageClut((void *)pData->unkC);
    }
    if (pData->unk8 != 0) {
        pData->unk8 += delta;
        for (i = 1; i < ((u32 *)pData->unk8)[0] + 1; i++) {
            pSlot = (u32 *)(i * 4 + pData->unk8);
            if (*pSlot != 0) {
                *pSlot += delta;
            }
        }
    }
    return pData;
}

void func_8008B070(s32 entryIndex) {
    void *pBuffer;

    HeapSetCurrentContentType(0xA);
    pBuffer = HeapAlloc(ArchiveDecodeSize(entryIndex), 0);
    ArchiveReadFileToBuffer(entryIndex, pBuffer, 0, 0);
    ArchiveCdDataSync(0);
    func_8008AF6C(pBuffer);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B0D8);

/* Best attempt: objdiff 99.80%, same 100 bytes. The instruction order and the
   registers ($a1 = entry, $v1 = entry+0x10, $v0 = value, $a2 = i) all match;
   the only difference is which derived giv gcc re-bases onto: the original uses
   entry+0x10 (sh at -4/-2/0, sw at -0xC), gcc entry+4 (sh at 8/10/12, sw at 0).
   Both serve exactly four refs, so this is a loop.c combine_givs tie. Confirmed
   rule: the base is the address of the body's LAST memory reference, so putting
   `unk4 = unk0` first flips the base to entry+0x10 (correct!) but moves its lw/sw
   to the top of the body, and putting `unk4 = value` last flips it back to
   entry+4. A separate s16 *pField = entry+0x10 pointer gives 75.84% and 112
   bytes (two strength-reduced bivs). An indexed pEntry[i] loop gives 88.24% and
   92 bytes (single base, no derived giv). */
/*
void func_8008B0D8(u8 *pActor) {
    Misc14Entry *pEntry = *(Misc14Entry **)(pActor + 8);
    s32 i;
    s32 value;

    *(s16 *)(pActor + 0x10) = 0;
    *(s16 *)(pActor + 0x12) = 0;
    for (i = 0; i < *(s16 *)(*(u8 **)pActor + 6); i++) {
        pEntry->unkC = 0;
        pEntry->unkE = 0;
        value = pEntry->unk0;
        pEntry->unk10 = 0;
        pEntry->unk4 = value;
        pEntry++;
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B13C);

extern u32 ModelResolvePointers(void *pModel);
extern void *BattleCreateObject(void);
extern void BattleAppendListEntry(void *pList, void *pNode);
extern void BattleSetNodeModelPart(void *pNode, void *pData);
extern void BattleSetNodeEntryList(void *pNode, void *pData);
extern void *BattleCreateEntryList(void);
extern void *BattleCreateModelPart(void);
extern void BattleAttachModelPart(void *pPart, void *pModelPart);
extern void func_8008B13C(void *pA, void *pB, void *pC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B38C);

/* Best attempt: objdiff 77.11%, 608 bytes against 592. Structure, call order,
   both loops and every offset match; three stack homes instead of retail's two
   (mine: pData 0x10, count 0x18, pNode 0x20; retail: pNode 0x10, pOther 0x18),
   so the frame is 0x60 against 0x58 and every s-register is one higher. Retail
   spills pOther (loaded once, read twice at the end) and keeps pData, count,
   pBuf, pScale, pCur, pPart, pSlot and i in registers.
   Two things did help and should be kept: reading *(s16 *)(pCur + 2) inline
   twice instead of through an `idx` local (71.54 -> 77.11: retail reloads the
   field after the BattleSetNodeModelPart call), and *(s16 *)pCur inline. Dead ends:
   pSlot as ((u8 **)pBuf)[i] (71.80), every pointer declared before use (71.54). */
/*
void *func_8008B38C(u8 *pArg) {
    u8 *pData = *(u8 **)(pArg + 4);
    u8 *pList = *(u8 **)pArg;
    u8 *pOther = *(u8 **)(pArg + 8);
    u32 count = *(u32 *)pList;
    u8 *pCur = pList + 4;
    u8 *pScale;
    u8 *pNode;
    u8 *pBuf;
    u8 *pPart;
    u8 *pListArg;
    u8 **pSlot;
    u32 i;

    ModelResolvePointers(pData);
    HeapSetCurrentContentType(0x12);
    pBuf = HeapAlloc(count * 4, 0);
    pScale = BattleCreateEntryList();
    pNode = BattleCreateObject();
    BattleSetNodeEntryList(pNode, pScale);
    *(u8 **)(pScale + 4) = pBuf;
    *(u16 *)pScale = count;
    *(u8 **)(pScale + 0x18) = pCur;
    pSlot = (u8 **)pBuf;
    for (i = 0; i < count; i++) {
        pPart = BattleCreateObject();
        *pSlot = pPart;
        if (*(s16 *)(pCur + 2) != -1) {
            pListArg = BattleCreateModelPart();
            BattleSetNodeModelPart(pPart, pListArg);
            BattleAttachModelPart(pListArg, pData + (*(s16 *)(pCur + 2) * 8 - *(s16 *)(pCur + 2)) * 8 + 0x10);
        }
        if (*(s16 *)pCur == -1) {
            pListArg = pNode;
        } else {
            pListArg = *(u8 **)(pBuf + *(s16 *)pCur * 4);
        }
        BattleAppendListEntry(pListArg, pPart);
        *(u16 *)(pPart + 0x44) = *(u16 *)(pCur + 4);
        *(u16 *)(pPart + 0x46) = *(u16 *)(pCur + 6);
        *(u16 *)(pPart + 0x48) = *(u16 *)(pCur + 8);
        *(u16 *)(pPart + 0x2C) = *(u16 *)(pCur + 0xA);
        *(u16 *)(pPart + 0x2E) = *(u16 *)(pCur + 0xC);
        *(u16 *)(pPart + 0x30) = *(u16 *)(pCur + 0xE);
        pSlot++;
        pCur += 0x10;
    }
    if (pOther != 0) {
        HeapSetCurrentContentType(0x11);
        pBuf = HeapAlloc(*(u32 *)pOther * 0x14, 0);
        *(u8 **)(pScale + 8) = pBuf;
        *(u16 *)(pScale + 2) = *(u32 *)pOther;
        for (i = 0; i < *(u32 *)pOther; i++) {
            if (*(u32 *)(pOther + 4) != 0) {
                func_8008B13C(*(void **)(pOther + 4), pBuf, pNode);
            } else {
                *(u32 *)pBuf = 0;
            }
            pBuf += 0x14;
            pOther += 4;
        }
    }
    return pNode;
}
*/

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} Misc14AnimCfg; // Size: 0x8

typedef struct {
    /* 0x0 */ u16 *unk0;
    /* 0x4 */ u16 unk4;
    /* 0x6 */ s16 unk6;
} Misc14AnimEntry; // Size: 0x8

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u8 *unk4;
    /* 0x08 */ u16 *unk8;
    /* 0x0C */ u16 unkC;
    /* 0x0E */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ s16 unk12;
} Misc14AnimTrack; // Size: 0x14

typedef struct {
    /* 0x00 */ Misc14AnimCfg *unk0;
    /* 0x04 */ Misc14AnimEntry *unk4;
    /* 0x08 */ Misc14AnimTrack *unk8;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ s16 unk12;
} Misc14Anim; // Size: 0x14

extern s32 func_8008B730(Misc14Anim *pAnim, s32 arg1, s32 arg2);

void func_8008B5DC(void *pA, s32 arg1) {
    func_8008B730(pA, arg1, 1);
}

s16 func_8008B5FC(s32 arg0, s32 arg1, s32 arg2) {
    s32 value;
    s32 diff;

    arg0 &= 0xFFF;
    diff = (arg0 - arg1) & 0xFFF;
    if (diff != 0) {
        if (diff < 0x800) {
            value = arg0 - diff / arg2;
        } else {
            value = arg0 + (0x1000 - diff) / arg2;
        }
    } else {
        value = arg0;
    }
    return value;
}

/* Parked: best objdiff 72.62%, 228 bytes (original 216). The instruction
   multiset and both clamp tests (slti 0x801 keep-if-0x800, slti 0x800 clamp)
   are right; the miss is allocation. Retail keeps var in $s0 with an
   out-of-place update (subu v0,s0,s2 then move s0,v0) and arg1 in $s1; gcc
   keeps the saved arg0 in $s0 and var in $a0, so it saves arg0 across the
   rand() call and updates var in place. Shapes tried: var as s16 (67.25%: right
   copies, var in $a0), var as s32 (72.62%: in-place updates, var coalesced),
   explicit (s16)arg0 cast, diff before var, var -= arg2 vs var = arg0 - arg2,
   two s16 locals for the clamp (adds a fourth s-reg, 56.25%). */
/*
extern s32 rand(void);

s16 func_8008B650(s32 arg0, s32 arg1, s32 arg2) {
    s16 var;
    s32 diff;

    diff = (arg0 - arg1) & 0xFFF;
    var = arg0;
    if (diff != 0) {
        if (diff == 0x800) {
            if (rand() & 1) {
                var -= arg2;
                if ((((s16)var - (s16)arg1) & 0xFFF) > 0x800) {
                    var = arg1;
                }
            } else {
                var += arg2;
                if ((((s16)var - (s16)arg1) & 0xFFF) >= 0x800) {
                    var = arg1;
                }
            }
        } else if (diff < 0x800) {
            var += arg2;
            if ((((s16)var - (s16)arg1) & 0xFFF) >= 0x800) {
                var = arg1;
            }
        } else {
            var -= arg2;
            if ((((s16)var - (s16)arg1) & 0xFFF) > 0x800) {
                var = arg1;
            }
        }
    }
    return var;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B650);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B730);

/* Best attempt: objdiff 91.66%, 760 bytes against 756 - one instruction too many.
   Whole structure, all four loops, the script decoder and the unused 0x10 stack
   slot are right (keep the pad[4]: without it the frame is 0x50, not retail's
   0x60). Also confirmed: the arg1 == 0 early exit returns whatever is in $v0 (the
   config pointer), so it is not `return 0/1` - it is the enclosing
   `if (arg1 != 0) { ...; return X; }` with no trailing return, which is why that
   path has a nop, not a li, in the beqz delay slot.
   What is still wrong:
   1. The three loops emit `lw count; nop; lh count; addiu i,1` where retail has
      `lw count; addiu i,1; lh count` (maspsx then needs no nop). Tried i++/++i
      and pEntry++/++pEntry - no change. Next: `i = 0; while (i < count) { ...;
      i++; }` so the increment is body code.
   2. The script decoder needs `move v1,v0` after the `lb` (retail keeps the char
      in v1 and the load in v0); gcc loads straight into v1 here.
   3. The final `return pAnim->unk12 == pAnim->unk0->unk0 && arg2 == 1;` costs an
      extra `move v0,a0` (gcc puts the sltiu in a0) and picks v1/v0 where retail
      picks a0/v1. */
/*
s32 func_8008B730(Misc14Anim *pAnim, s32 arg1, s32 arg2) {
    Misc14AnimEntry *pEntry;
    Misc14AnimTrack *pTrack;
    u8 *pScript;
    s32 c;
    s32 i;
    s32 j;
    s32 pos;
    s32 pad[4];

    if (pAnim->unk0 == NULL) {
        return 1;
    }
    if (arg1 != 0) {
    pos = pAnim->unk12;
    if (pos + arg1 > pAnim->unk0->unk0) {
        arg1 = pAnim->unk0->unk0 - pos;
    }
    pAnim->unk12 = pos + arg1;
    arg2 -= arg1;
    if (arg2 <= 0) {
        arg2 = 1;
    }
    pEntry = pAnim->unk4;
    if (arg2 == 1) {
        for (i = 0; i < pAnim->unk0->unk4; i++) {
            *(u16 *)pEntry->unk0 = pEntry->unk4;
            ++pEntry;
        }
    } else {
        for (i = 0; i < pAnim->unk0->unk4; i++) {
            u16 value = *pEntry->unk0;

            if (pEntry->unk6 != 0) {
                *pEntry->unk0 = func_8008B5FC((s16)value, (s16)pEntry->unk4, arg2);
            } else {
                *pEntry->unk0 = value + ((s16)pEntry->unk4 - (s16)value) / arg2;
            }
            ++pEntry;
        }
    }
    pTrack = pAnim->unk8;
    for (i = 0; i < pAnim->unk0->unk6; i++) {
        for (j = 0; j < arg1; j++) {
            if (pTrack->unkC != 0) {
                pTrack->unkC--;
            } else {
                pScript = pTrack->unk4;
                pTrack->unk4 = pScript + 1;
                c = *(s8 *)pScript;
                if (c & 0x80) {
                    s32 bits = c & 0x3F;

                    if (c & 0x40) {
                        pTrack->unk4 = pScript + 2;
                        pTrack->unk10 = bits | ((s8)pScript[1] << 6);
                    } else {
                        pTrack->unkC = bits;
                    }
                } else {
                    pTrack->unk10 = (s8)(c << 1) >> 1;
                }
            }
            pTrack->unkE += pTrack->unk10;
        }
        if (arg2 == 1) {
            *pTrack->unk8 = pTrack->unkE;
        } else {
            u16 value = *pTrack->unk8;

            if (pTrack->unk12 != 0) {
                *pTrack->unk8 = func_8008B5FC((s16)value, (s16)pTrack->unkE, arg2);
            } else {
                *pTrack->unk8 = value + ((s16)pTrack->unkE - (s16)value) / arg2;
            }
        }
        pTrack++;
    }
    return pAnim->unk12 == pAnim->unk0->unk0 && arg2 == 1;
    }
}
*/

typedef struct {
    /* 0x00 */ u32 unk0[0x20];
    /* 0x80 */ u32 unk80;
} Misc14Work; // Size: 0x84

Misc14Work *func_8008BA2C(u32 arg0, u32 arg1, u32 arg2, s32 arg3) {
    Misc14Work *p;
    s32 i;
    u32 value;

    HeapSetCurrentContentType(3);
    p = HeapAlloc(0x84, 2);
    for (i = 0x1F; i >= 0; i--) {
        p->unk0[i] = 0;
    }
    p->unk80 = arg2;
    p->unk0[0x1C] = func_800405E4();
    value = p->unk80 + arg3 * 4;
    p->unk0[0x1F] = arg0;
    p->unk0[4] = arg1;
    p->unk0[0x1D] = value;
    p->unk0[0x1E] = value;
    func_8008BB3C(p);
    return p;
}

void func_8008BAE0(void *pMem) {
    HeapFree(pMem);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB00);

/* Best attempt: objdiff 94.29% (same under gcc-2.6.0 and gcc-2.7.2-cdk). Only
   the temporaries differ: the original loads into $t0/$t1, gcc into $v0/$v1; the
   instruction sequence and both symbols are identical. Two locals, direct field
   stores, a struct pointer and an 8-byte struct copy (70.71%, it also bases the
   second load off one symbol) all give $v0/$v1. */
/*
void func_8008BB00(Misc14Entry *pDst) {
    pDst->unk0 = D_80096D88;
    pDst->unk4 = D_80096D8C;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB1C);

/* Best attempt: objdiff 97.50% (97.5 under gcc-2.6.0 and -cdk too) - identical
   except that the original keeps the one reused temporary in $t0, gcc in $v0.
   Reusing a single temp is what reproduces the interleaved load/store order;
   two separate statements make gcc hoist the second load (73.12%). */
/*
void func_8008BB1C(Misc14Entry *pSrc) {
    u32 value;

    value = pSrc->unk0;
    D_80096D88 = value;
    value = pSrc->unk4;
    D_80096D8C = value;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB3C);
