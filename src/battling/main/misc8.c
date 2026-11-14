#include "common.h"
#include "system/memory.h"
#include "psyq/libgpu.h"
#include "system/controller.h"

extern u8 D_800915AC[];
extern u16 D_800915BE;
extern u16 D_80091636;
extern u8 D_80091660[];
extern u8 D_800926FC;
extern u8 D_8009275C;
extern u16 D_80091762;
extern u16 D_80091672;
extern u16 D_800916AE;
extern s32 *D_80092734;
extern s32 *D_80092738;
extern u8 D_80092758;
extern s32 *D_80092760;
extern u8 D_8009A0D8[];
extern u8 D_800928A0;
extern s32 D_800926DC;
extern u8 D_800916D8[];
extern s32 D_80092700;
extern s32 D_80092704;
extern u8 D_8009273C;
extern u8 D_80092740;
extern u16 D_8009286C;
extern void *D_80092880;
extern u16 D_8009553C;
extern u16 D_8009553E;
extern u8 D_800928FC;
extern u8 D_8009A1C4;
extern u8 D_8009A1C5;
extern u8 D_8009A1C6;
extern u16 D_8009A1CA;
extern u16 D_8009A1CE;
extern u8 D_8009A2BC;
extern u8 D_8009A2BD;
extern u8 D_8009A2BE;
extern u16 D_8009A2C2;
extern u16 D_8009A2C6;
extern s32 D_800928C8;
extern s32 D_800928D8;
extern s32 D_80092924;
extern s32 D_80092940;

extern void func_8007F834(void);
extern void func_800719F0(void);
extern void BattleLoadSlotEntry(s32 a0, s32 a1);
extern void func_80039FF8(void);
extern void func_8007313C(void *pDst, void *pSrc);
extern void func_8008EB4C(s32 arg);
extern void func_80083C0C(s32 mode);
extern void BattleResetTextListState(void);
extern void func_800812BC(u8 *p);
extern void func_8007E894(s16 a, s16 b);
extern void BattleSetRowTextColor(void *p, s32 arg);
extern void func_8007EBE0(u8 *p);
extern void func_8007ED84(u8 *p, s32 dy);
extern void func_80036420(void);
typedef struct {
    /* 0x0  */ u8 unk0[4];
    /* 0x4  */ u32 unk4;
    /* 0x8  */ u8 unk8[0xA];
    /* 0x12 */ s16 unk12;
} BattleTextEntry; // Size: 0x14

typedef struct {
    /* 0x0  */ u8 unk0;
    /* 0x4  */ u32 pEntries;
    /* 0x8  */ s16 count;
    /* 0xC  */ void (*unkC)(void *p);
    /* 0x10 */ u8 unk10[4];
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
} BattleTextList; // Size: 0x18

extern void func_800811AC(u8 *p);
extern void func_8008151C(BattleTextList *p);
extern void func_8008162C(s32 *p, s32 a1);
extern void func_80081A44(void);
extern void func_80080F04(void);
extern void *GetStringEntry(void *pStringData, u32 entry);
extern s32 SystemRenderStringEntry(void *pCur, void *pCharBuffer, s32 mode, s32 flag);
extern s32 func_800891C0(s32 a0);
extern void ArchiveCdDataSync(s32 mode);

void func_800808F4(void) {
    D_80092924 = 1;
    func_8007F834();
}

void func_80080920(void) {
    D_80092924 = 1;
    func_800719F0();
    BattleLoadSlotEntry(0, 0);
    BattleLoadSlotEntry(1, 1);
}

void func_80080964(s32 a0) {
    if (a0 == 0xFF) {
        D_80092734 = D_80092738;
    } else {
        D_80092738 = D_80092734;
        D_80092734 = (s32 *)(D_800915AC + a0 * 0x3C);
    }
}

s32 func_800809BC(void) {
    return D_80092734 == (s32 *)D_80091660;
}

void func_800809D8(void) {
    func_80039FF8();
    D_80092734 = 0;
    func_80080964(3);
    D_80091672 = 0;
    D_800916AE = 0;
    D_800928C8 = 0;
    D_80092758 = 0;
    func_8007F834();
    D_80092924 = 0;
    func_80080AA0(0);
    D_80092940 = 0;
    D_800928D8 = func_800891C0(6);
}

void func_80080A58(void) {
    if (D_80092940 == 0) {
        ArchiveCdDataSync(0);
        HeapFree(D_800928D8);
        D_80092940 = 1;
    }
}

void func_80080AA0(s32 a0) {
    if (a0 != 0) {
        D_80092760 = 0;
    }
    if (D_80092760 != 0) {
        HeapFree(D_80092760);
        D_80092760 = 0;
    }
}

void func_80080AE8(void) {
    RECT rect;

    if (D_80092760 != 0) {
        DrawSync(0);
        rect.x = 0x140;
        rect.y = 0x100;
        rect.w = 0x140;
        rect.h = 0xDA;
        func_8007313C(D_80092760, (u8 *)D_80092760 + 0x21E80);
        LoadImage(&rect, (u_long *)D_80092760);
    }
}

void func_80080B58(void) {
    RECT rect;

    if (D_80092760 == 0) {
        HeapToggleErrorHandler(1);
        D_80092760 = HeapAlloc(0x22100, 0);
        HeapToggleErrorHandler(0);
    }
    DrawSync(0);
    rect = *(RECT *)(D_8009A0D8 + ((D_800928A0 + 1) & 1) * 0xF8);
    MoveImage(&rect, 0x140, 0x100);
    if (D_80092760 != 0) {
        StoreImage(&rect, (u_long *)D_80092760);
    }
    DrawSync(0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080C48);
/* 95.60%: the tail MUST be duplicated in both branches (that is what makes gcc
   place it between branch 2 and the else block, matching retail's
   [test1][A][j tail][test2][B][tail][else] layout).  Only difference left: the
   func_80083C0C argument setup.  Retail emits `addu a0,zero,zero` once, inside
   the tail, in the jal delay slot and a plain `nop` in branch 1's `j` slot;
   ours hoists it out of the merged tail in both copies - one lands in branch
   1's `j` delay slot and one above branch 2's D_80091762 store, and the jal
   delay slot is a `nop` (2 extra insns, all other insns identical).
   Also tried: the tail written once after the if/else chain with the else
   returning first (79.80% - gcc then puts the else block before the tail, so
   branch 2 and the else both need a `j` epilogue), and the argument as an
   `s32 mode = 0` local.  gcc 2.7.2-cdk scores worse (85.36%), so the unit
   preset is right.
void func_80080C48(s32 a0) {
    func_80039FF8();
    func_8008EB4C(0x1F);
    if (a0 == 1) {
        D_80092734 = 0;
        func_80080964(0);
        D_800915BE = 0;
        D_80091636 = 1;
        func_80083C0C(0);
        D_80092758 = 1;
        D_800926FC = 0;
        D_8009275C = 1;
        func_80080B58();
    } else if (a0 == 2) {
        D_80092734 = 0;
        func_80080964(7);
        D_80091762 = 1;
        func_80083C0C(0);
        D_80092758 = 1;
        D_800926FC = 0;
        D_8009275C = 1;
        func_80080B58();
    } else {
        BattleResetTextListState();
    }
}
*/

void func_80080D10(void) {
    D_800926DC = 0;
}

extern u8 *D_800926D4[];
extern s32 D_800912F0;
extern u8 D_80095498[];
extern u8 D_800954C8[];
extern u8 D_8009A0DA[];

void func_80080D20(u8 *p) {
    s32 i;
    u8 *pEntry;
    RECT rect;

    pEntry = D_800926D4[D_800928A0];
    for (i = 0; i < D_800926DC; i++, pEntry += 0x28) {
        AddPrim(p, pEntry);
    }
    D_800926DC = 0;
    func_800811AC(p);
    if (((D_80092734 != 0) && (D_80092758 != 0)) || (D_800912F0 != 0)) {
        if (D_80092734 != 0) {
            AddPrim(p, (u8 *)D_80092734 + (D_800928A0 * 0x10 + 0x1C));
            SetDrawTPage((DR_TPAGE *)(D_800954C8 + D_800928A0 * 8), 0, 0, GetTPage(0, 2, 0, 0));
            AddPrim(p, D_800954C8 + D_800928A0 * 8);
        }
        rect.x = 0x140;
        rect.y = 0x100;
        rect.w = 0x140;
        rect.h = 0xDA;
        SetDrawMove((DR_MOVE *)(D_80095498 + D_800928A0 * 0x18), &rect,
                    *(s16 *)(D_8009A0D8 + D_800928A0 * 0xF8),
                    *(s16 *)(D_8009A0DA + D_800928A0 * 0xF8));
        AddPrim(p, D_80095498 + D_800928A0 * 0x18);
    }
    D_800912F0 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080F04);
/* 83.89%: both sprite halves match except the four sprite-scaling stores.
   Retail writes x0/y0 as one sw and u0/v0 as one sh to the ABSOLUTE symbols
   D_8009551C/D_80095520 (and D_8009554C/D_80095550 for the second half), and
   h/clut to D_80095526/D_80095522: the retail immediates are
   `sh $s3,%lo(D_80095526)($at)` (0x5526), so the address is not formed as
   base+0x16 off D_80095510.  Spelling them as extern scalars gives those
   instruction shapes but only 63.38%: gcc then sinks all four of those stores
   BELOW the 0x14-byte struct copy (they do not alias it as far as it knows),
   while retail keeps h/clut before the copy.  Writing them as sprite fields
   (pSprt->x0 ...) keeps retail's order but uses s0-relative addressing and
   fails to merge the two halfword pairs.
   Also missing: the last two words are reloaded through %hi(D_80095570)
   instead of `lw $v1,4($s0)`.  Explicit statement order has no effect (the
   copy is emitted first and the scalar stores after it either way).
void func_80080F04(void) {
    u_long *pData;
    Misc8Text *p;
    SPRT *pSprt;

    pData = HeapAlloc(0x6B4, 0);
    p = D_80095510;
    D_80095540.pData = pData;
    p[0].pData = pData;
    pSprt = &p[0].sprt[0];
    pSprt->x0 = 0;
    pSprt->y0 = 0xB4;
    pSprt->u0 = 0;
    pSprt->v0 = 0x30;
    SetSprt(pSprt);
    SetShadeTex(pSprt, 1);
    pSprt->h = 0xD;
    pSprt->clut = g_SystemPalette1;
    p[0].sprt[1] = p[0].sprt[0];
    pSprt = &p[1].sprt[0];
    pSprt->x0 = 0;
    pSprt->y0 = 0xB4;
    pSprt->u0 = 0;
    pSprt->v0 = 0x30;
    SetSprt(pSprt);
    SetShadeTex(pSprt, 1);
    pSprt->h = 0xD;
    pSprt->clut = g_SystemPalette2;
    p[1].sprt[1] = p[1].sprt[0];
    SetDrawTPage((DR_TPAGE *)D_80095570, 0, 0, GetTPage(0, 0, 0x140, 0x30));
    D_80095578 = *(u32 *)D_80095570;
    D_8009557C = *((u32 *)D_80095570 + 1);
}
*/

typedef struct {
    /* 0x00 */ u_long *pData;
    /* 0x04 */ SPRT sprt[2];
    /* 0x2C */ s16 width;
    /* 0x2E */ s16 x;
} Misc8Text; // Size: 0x30

extern Misc8Text D_80095510[2];
extern Misc8Text D_80095540;
extern u16 D_8009556C;
extern u16 D_8009556E;
extern u8 D_80095570[];

void func_80081094(Misc8Text *p, s32 entry, s32 flag) {
    s32 len;

    len = SystemRenderStringEntry(GetStringEntry(D_80092880, entry), p->pData, 0x3F, flag);
    p->width = len;
    p->x = (0x140 - len) / 2;
}

void func_80081100(s32 entry, s32 mode) {
    RECT rect;

    if (mode == 0) {
        if (entry == D_8009273C) {
            return;
        }
        D_8009273C = entry;
        func_80081094(&D_80095510[0], entry, 0);
    } else {
        if (entry == D_80092740) {
            return;
        }
        D_80092740 = entry;
        func_80081094(&D_80095540, entry, 1);
    }
    rect.x = 0x140;
    rect.y = 0x30;
    rect.w = 0x42;
    rect.h = 0xD;
    LoadImage(&rect, D_80095510[0].pData);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_800811AC);
/* 98.68%: only the `D_800928A0 * 0x14` chain's temp register differs - retail
   keeps it in a1 for all three insns (sll/addu/sll) because `off` itself is
   allocated to a1, ours gives the temp v0 and only the last sll a1.  Every
   other insn, the base address in a2 and both addu operand orders match.
   Tried: off as s32 and as u32, an intermediate `s32 n = D_800928A0` feeding
   off, off/pBase declaration order swapped - identical output.  Inlining the
   multiply in each address instead of an `off` local gives 90.15% (264 bytes,
   recomputed), and gcc-2.7.2-cdk gives 95.29%, so Default + the `off` local is
   the right shape.  Keeping the base pointer in a local and forming the
   addresses as `pBase + off` / `pBase + (off + 4)` from it is what fixes the
   a1/a2 tie-break (87.21% -> 98.68%).
void func_800811AC(u8 *p) {
    u32 off;
    u8 *pBase;

    if (D_8009273C != 0) {
        pBase = (u8 *)&D_80095510[0];
        off = D_800928A0 * 0x14;
        *(u16 *)(pBase + off + 0x14) = D_8009553C;
        *(u16 *)(pBase + off + 0xC) = D_8009553E;
        AddPrim(p, pBase + (off + 4));
    }
    if (D_80092740 != 0) {
        pBase = (u8 *)&D_80095540;
        off = D_800928A0 * 0x14;
        *(u16 *)(pBase + off + 0x14) = D_8009556C;
        *(u16 *)(pBase + off + 0xC) = D_8009556E;
        AddPrim(p, pBase + (off + 4));
    }
    if (D_8009273C | D_80092740) {
        AddPrim(p, D_80095570 + D_800928A0 * 8);
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_800812BC);

void func_800814AC(void) {
    u32 i;
    u8 *p;

    for (i = 0, p = D_800915AC; i < 8; i++, p += 0x3C) {
        func_800812BC(p);
    }
    D_80092734 = 0;
    D_80092700 = 0;
    D_80092704 = 1;
    func_80080F04();
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_8008151C);
/* 99.63%: the duplicated call (one per arm of the unk0 test) is what makes
   gcc cross-jump the two `jal func_8007E894` tails into one and keep retail's
   `lh a1,0x14` in BOTH arms; assigning a1 in both arms and calling once merges
   the load instead (95.15%).
   Only difference left: retail sets i=0 before the count load (so `move s1,zero`
   lands in the func_8007E894 jal delay slot) and sinks off=0 after the `blez`
   (in the fall-through, after the `move a0,s0` that fills the blez slot); ours
   emits off=0 first (into the jal delay slot) and i=0 into the blez delay slot.
   Tried: `i = 0;` before the for, `i = 0;` immediately after the call with
   `for (off = 0; ...)`, the init clause written `i = 0, off = 0` and
   `off = 0, i = 0`, both declaration orders, and a while loop (97.47%) - the
   two loop-initialisation insns keep retail's order inverted.
void func_8008151C(BattleTextList *p) {
    s32 i;
    s32 off;

    if (p == 0) {
        return;
    }
    D_80092734 = (s32 *)p;
    if ((s32 *)p == (s32 *)D_800916D8) {
        return;
    }
    if (p->unk0 != 0) {
        func_8007E894(p->unk16, p->unk14);
    } else {
        func_8007E894(0xA0, p->unk14);
    }
    i = 0;
    for (off = 0; i < p->count; i++) {
        BattleSetRowTextColor(p, i);
        if (p->unk0 != 0) {
            func_8007EBE0((u8 *)*(u32 *)(off + p->pEntries + 4));
        } else {
            func_8007ED84((u8 *)*(u32 *)(off + p->pEntries + 4), *(s16 *)(off + p->pEntries + 0x12));
        }
        off += 0x14;
    }
    if (p->unkC != 0) {
        p->unkC(p);
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_8008162C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081A44);

void func_80081D2C(void) {
    s32 unk[2]; /* retail reserves 8 bytes of locals here */
    s32 *p;

    if (D_8009275C != 0) {
        func_80080AE8();
        D_8009275C = 0;
    }
    func_80036420();
    p = D_80092734;
    if (p == (s32 *)D_800916D8) {
        func_80081A44();
    } else {
        if (p != 0) {
            if (g_C1ButtonStateReleased & 1) {
                if (D_800926FC < 5) {
                    D_800926FC++;
                    func_80080AE8();
                }
            }
            func_8008162C(p, D_800928FC);
        }
        func_8008151C(D_80092734);
    }
}

void func_80081E00(void) {
    u16 v = D_8009286C;

    D_8009A1CA = 0x60;
    D_8009A2C2 = 0x60;
    D_8009A1C4 = 0x7F;
    D_8009A1C5 = 0x7F;
    D_8009A1C6 = 0x7F;
    D_8009A2BC = 0x7F;
    D_8009A2BD = 0x7F;
    D_8009A2BE = 0x7F;
    D_8009A1CE = v - 0x60;
    D_8009A2C6 = v - 0x60;
}

void func_80081E6C(void) {
    u16 v = D_8009286C;

    D_8009A1CA = 0;
    D_8009A2C2 = 0;
    D_8009A1C4 = 0;
    D_8009A1C5 = 0;
    D_8009A1C6 = 0;
    D_8009A2BC = 0;
    D_8009A2BD = 0;
    D_8009A2BE = 0;
    D_8009A1CE = v;
    D_8009A2C6 = v;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081ECC);
