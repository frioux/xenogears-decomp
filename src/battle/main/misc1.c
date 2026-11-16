#include "common.h"
#include "psyq/libgpu.h"
#include "battle/actor.h"

// The per-slot digit-panel bookkeeping the renderers below advance. g_BattleDisplayState
// points at the battle work block that battle_work.h's BattleWork describes for
// misc2/misc3; the counters these two functions use sit past the fields that
// view names, so misc1 spells out just those.
typedef struct {
    /* 0x000 */ u8 unk0[0x99];
    /* 0x099 */ u8 unk99[3];
    /* 0x09C */ u8 unk9C[0x44];
    /* 0x0E0 */ s32 statSpriteCount[3];  // packaged sprites so far, x 0x50 into the slot's prim area
    /* 0x0EC */ s32 valueSpriteCount[3];
} BattleDigitState; // Size: 0xF8

extern u8 D_800C3D48;
extern u8 D_800C48EA;
extern s32 *D_800C3E54;
extern u8 *g_BattleCommandQueue;
extern BattleDigitState *g_BattleDisplayState;
extern u8 D_800D3725[];
extern s32 D_800D3284;
extern s32 D_800D328C;
extern s32 *g_DebuggerState;
extern u8 *g_BattleCurOT;
extern u8 *g_BattleGraphicsWork;

extern void BattleSetArchive20Entry0(void);
extern s32 BattleAllocHeapBlock(s32, s32);
extern void BattleWaitForArchiveSync(void);
extern void func_801E5160(void);
extern s32 func_801E563C(void);
extern void func_801E879C(s32);
extern void func_8028022C(void);
extern void func_800BE790(void);
extern void SoundSetAudioManagerVolumeWithFade(s32 *, s32, s32);
extern void ArchiveReadFileToBuffer(s32, void *, u32, u32);
extern u8 D_800D2CAF;
extern u16 g_BattleActionSlotMask;
extern u16 g_BattleSlotFlag8000Mask;
extern u8 *D_800D39F0;
extern s32 D_800D39C0[];
extern u8 D_800D39C6;
extern u8 *GetStringEntry(u8 *);
extern u8 g_BattleMenuEvent;
extern u8 g_BattleActorCountdownState[];
extern u8 g_BattlePendingCharIndex;
extern u8 D_800D2DD7[];
extern u8 D_800D2DD8[];
extern u8 D_800D2DE3[];
extern u16 D_800D2E04[];
extern u16 D_800D2DF0[];
extern u16 g_BattleSlotCountdownTimers[];
extern u16 D_800D2C9E;
extern u16 BattleMaskHasSlotBit(u16, u8);
extern u16 g_BattlePendingSlotMask;
extern u8 D_800C402F[];
extern u8 g_BattleCommandArgs;
extern u8 D_800D2E60;
extern void BattleResetActorStatDisplay(u8);
extern void func_80071B94(s32);
extern u16 func_80098AF8(s32, s32);
extern void BattleSetSlotFlagB4(s32);
extern void BattleClearSlotFlagB4(s32);
extern s32 SystemRenderStringEntry(u8 *, s32, s32, s32);
extern u8 g_BattlePrimStartIndex;
extern u8 D_800D2D88[];
extern u8 D_800D3280;
extern u16 D_800C3076[];
extern u16 D_800C3254[];
extern s32 BattleDrawSpritePackage(s32, s32, s16, s16);
extern void func_80072938(POLY_FT4 *, s32, s32, s32);
extern u8 D_800D3298;
extern u8 g_BattleSlotEnabledFlags[];
extern u16 g_BattleSlotEntries[];
extern u16 D_800CCD68[];
extern u16 D_800CCD6C[];
extern u16 D_800CCD6E[];
extern u8 D_800CCE44[];
extern u16 D_800D2E1C[];
extern u8 D_800C3CF9[];
extern u8 D_800D32A0[];
extern BattleTableEntry D_800CCDC4[]; // per-slot table family, stride 0x170
extern void BattleParseNumberToDigits(s32);

void BattleLoadOverlay(void) {
    s32 first;
    s32 second;

    if (D_800C3D48 != 0) {
        BattleSetArchive20Entry0();
        first = BattleAllocHeapBlock(4, 1);
        D_800D3284 = first;
        second = BattleAllocHeapBlock(first + 0x7FE1B000, 1);
        D_800D328C = second;
        ArchiveReadFileToBuffer(1, (void *)0x801E5000, 0, 0x80);
        BattleWaitForArchiveSync();
        func_801E5160();
    }
}

void BattleSetOverlayArg(u8 arg0) {
    if (D_800C3D48 != 0) {
        func_801E879C(arg0);
    }
}

void BattleFadeOutAudioIfOverlayIdle(void) {
    u8 result;

    result = 0;
    if (D_800C3D48 != 0) {
        result = func_801E563C();
    }
    if (result == 0 && D_800C48EA == 0x81) {
        SoundSetAudioManagerVolumeWithFade(D_800C3E54, 0, 0xF0);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80070F40);

s32 BattleUpdateFrame(void) {
    if (g_DebuggerState[0] != -1) {
        func_8028022C();
    }
    func_800BE790();
    return 0;
}


INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_8007171C);
/* 77.62% under 2.6.0. The whole control flow (the 1/2 decrement, the 0x1000
   bit toggle on D_800D2E1C, the 0x2000 timer, the 0x80/0x1000 guards and the
   0xB wrap) is right; the miss is that gcc strength-reduces the most-used
   `(u8 *)SYM + off` (g_BattleSlotEntries, four uses) into a pointer biv, so its base is
   hoisted once into a3 and the loads become `lhu 0(a3)`, where retail re-forms
   `lui $at,%hi(SYM); addu $at,$at,$a0` at every use and keeps `off` an integer
   biv. Tried: `off` as a running for-clause biv, `off = i * 0x170` inline (both
   77.62%), one-`if`-per-level nesting without continue (70.32%), splitting the
   two entry tests. The other four symbols already come out in the retail form.
void func_8007171C(void) {
    u16 v;
    u8 *pTimer;
    s32 dec;
    s32 i;
    s32 off;

    if (D_800D3298 == 0) {
        return;
    }

    for (i = 0; i < 0xB; i++) {
        off = i * 0x170;
        if (g_BattleSlotEnabledFlags[i] == 0) {
            continue;
        }
        if (g_BattleActorCountdownState[i] != 0) {
            continue;
        }
        dec = 1;
        if ((*(u16 *)((u8 *)D_800CCD6C + off) | *(u16 *)((u8 *)D_800CCD6E + off)) & 0x8000) {
            dec = 2;
        }
        if (*(u16 *)((u8 *)g_BattleSlotEntries + off) & 0x1000) {
            v = *(u16 *)((u8 *)D_800D2E1C + i * 2) ^ 1;
            *(u16 *)((u8 *)D_800D2E1C + i * 2) = v;
            if (v != 0) {
                continue;
            }
        }
        v = *(u16 *)((u8 *)g_BattleSlotEntries + off);
        if (v & 0x2000) {
            pTimer = (u8 *)((u8 *)D_800CCE44 + off);
            *pTimer -= dec;
            if (*pTimer != 0) {
                continue;
            }
            v = *(u16 *)((u8 *)g_BattleSlotEntries + off);
            *pTimer = 0;
            *(u16 *)((u8 *)g_BattleSlotEntries + off) = v & 0xDFFF;
            continue;
        }
        if (v & 0x80) {
            continue;
        }
        if (*(u16 *)((u8 *)D_800CCD68 + off) & 0x1000) {
            continue;
        }
        pTimer = (u8 *)((u8 *)g_BattleSlotCountdownTimers + i * 2);
        v = *(u16 *)pTimer - dec;
        *(u16 *)pTimer = v;
        if ((s16)v > 0) {
            continue;
        }
        g_BattleActorCountdownState[i] = 1;
        *(u16 *)pTimer = 0;
    }
}
*/
void BattleResetActorCountdown(void) {
    u8 index;

    index = g_BattleCommandQueue[0x2D3];
    if (g_BattleActorCountdownState[index] != 0xFF) {
        g_BattleActorCountdownState[index] = 0;
    }
    g_BattleSlotCountdownTimers[g_BattleCommandQueue[0x2D3]] = func_80098AF8(g_BattleCommandQueue[0x2D3], 0);
    g_BattleSlotCountdownTimers[g_BattleCommandQueue[0x2D3] - 0xB] = g_BattleSlotCountdownTimers[g_BattleCommandQueue[0x2D3]];
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80071964);
/* 99.51% under 2.6.0 and 2.7.2: every instruction is identical except the
   leading flag load, which retail puts in $a1 (a dead argument register at
   entry) and gcc in $v0 (reused by the later lhu). Both pseudos have one use
   and no live range overlap, so this is an allocator tie; the and-operand
   register pair is identical. Tried: compound &&, !(), nested ifs, volatile on
   the flag, an int local for the flag.
void func_80071964(void) {
    u8 *pEntry;
    s32 i;

    if (D_800D2CAF != 0 && (g_BattleActionSlotMask & g_BattleSlotFlag8000Mask) == 0) {
        pEntry = GetStringEntry(D_800D39F0);
        D_800D39C6 = SystemRenderStringEntry(pEntry, D_800D39C0[0], 0x39, 1);
        LoadImage((RECT *)&D_800D39C0[-2], (u_long *)D_800D39C0[0]);
        for (i = 3; i != 0; i--) {
            BattleUpdateFrame();
        }
    }
}
*/
void BattleClearCharRedrawFlags(void) {
    s32 i;
    u8 *p;

    for (i = 0; i < 3; i++) {
        p = g_BattleCommandQueue;
        *(p + i + 0x2EB) = 0;
    }
}

void BattleApplyCharRedrawFlags(void) {
    s32 i;
    u8 *pDst;
    u8 *pSrc;

    // `*(p + i + k)` and not `p[i + k]`: only the former puts the base first in
    // the addu under both 2.6.0 and 2.7.2, which is the original's order.
    for (i = 0; i < 3; i++) {
        pDst = (u8 *)g_BattleDisplayState;
        pSrc = g_BattleCommandQueue;
        *(pDst + i + 0x7C) = *(pSrc + i + 0x2EB);
    }
    BattleUpdateFrame();
}

void BattleClearAllSlotFlagB4(void) {
    s32 i;

    for (i = 0x2A0; i >= 0; i -= 0x60) {
        D_800D3725[i] = 0;
    }
    ((u8 *)g_BattleDisplayState)[0xB5] = 0;
    ((u8 *)g_BattleDisplayState)[0xB4] = 0;
    BattleUpdateFrame();
}

void BattleWaitWhileStateIs8(void) {
    s32 i;

    if (D_800D2CAF != 0 && (g_BattleActionSlotMask & g_BattleSlotFlag8000Mask) == 0) {
        BattleSetSlotFlagB4(7);
        g_BattleCommandQueue[0x2DB] = 0;
        for (i = 0x3B; i != 0; i--) {
            BattleUpdateFrame();
            if (g_BattleMenuEvent != 8) {
                break;
            }
        }
        g_BattleCommandQueue[0x2DB] = 1;
        BattleClearSlotFlagB4(7);
        BattleUpdateFrame();
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80071B94);
/* 72.24% - body is exact except the flag store: gcc hoists `li s3,1` out of the
   loop (loop.c invariant) and then schedules the g_BattleDisplayState store before the sh,
   costing a callee-saved reg and a 40-byte frame vs the original 32; the original
   re-materialises `li v0,1` inside the block. Tried: plain g_BattleDisplayState[i + 0x7C]
   (index-first addu), (g_BattleDisplayState + i)[0x7C] (base-first addu, now identical),
   store order before/after *pDst = *pSrc (no effect), pFlag pointer local.
   The pointer pair pDst = g_BattleSlotCountdownTimers, pSrc = pDst - 0xB reproduces the original
   addiu s2,s1,-0x16 exactly. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072270);
/* 72.36% under 2.6.0 and 2.7.2. The pointer pair (pSrc = pDst - 0xB gives the
   original addiu s2,s1,-22) and every operand are right; gcc's loop.c hoists
   `li s3,1` for the flag store into a callee-saved register, so the frame is
   0x28 instead of 0x20 and the g_BattleDisplayState load lands after the sh instead of
   before the lhu. Tried under 2.6.0: 0x7C store before/after *pDst = *pSrc.
void func_80072270(void) {
    s32 i;
    u16 *pSrc;
    u16 *pDst;

    if ((D_800D2C9E & 7) != 0) {
        pDst = g_BattleSlotCountdownTimers;
        pSrc = pDst - 0xB;
        for (i = 0; i < 3; i++) {
            if (BattleMaskHasSlotBit(D_800D2C9E, i) != 0) {
                g_BattleActorCountdownState[i] = 0;
                *pDst = *pSrc;
                (g_BattleDisplayState + i)[0x7C] = 1;
            }
            pDst++;
            pSrc++;
        }
    }
    D_800D2C9E = 0;
}
*/

void BattleResetActorCommandSlots(void) {
    u8 *pBuf;
    u8 *p;
    u8 *pEnd;
    s32 i;
    s32 j;

    for (i = 3; i < 0xB; i++) {
        if (BattleMaskHasSlotBit(g_BattlePendingSlotMask, i) != 0) {
            p = pBuf;
            pEnd = p + 0x100;
            do {
                *p++ = 0;
            } while (p < pEnd);
            for (j = 0x8B8; j >= 0; j -= 0x48) {
                D_800C402F[j] = 0xFF;
            }
            g_BattleCommandArgs = 4;
            D_800D2E60 = 0x17;
            BattleResetActorStatDisplay(i);
            func_80071B94(1);
        }
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_800723E0);
/* 68.84% under 2.6.0 (69.63 under 2.7.2). Whole structure is right (0x18 frame,
   both early-exit paths, the 0xB wrap and the *pCurrent = i + 1 tail). Misses:
   gcc invents a 4th lbu of g_BattlePendingCharIndex in the middle block, reloads
   D_800D2DD8[i] after the loop instead of reusing the loop's slot register,
   materialises a second `li 0xb` for the tail compare where the original keeps
   a3, and hoists `move a0,zero` out of the final jal's delay slot.
void func_800723E0(void) {
    u8 *pCurrent;
    s32 i;

    if (g_BattlePendingSlotMask != 0) {
        BattleResetActorCommandSlots();
        return;
    }

    if (g_BattlePendingCharIndex != 0) {
        g_BattleCommandQueue[0x2D3] = g_BattlePendingCharIndex;
        D_800D2DE3[g_BattlePendingCharIndex] = 1;
        D_800D2E04[g_BattlePendingCharIndex] = 0;
        g_BattlePendingCharIndex = 0;
        func_80071B94(0);
        return;
    }

    g_BattleCommandQueue[0x2D3] = 0;
    pCurrent = D_800D2DD7;
    for (i = *pCurrent; g_BattleActorCountdownState[D_800D2DD8[i]] != 1; i++) {
        if (i == 0xB) {
            i = 0;
        }
        if (i == *pCurrent) {
            break;
        }
    }
    g_BattleCommandQueue[0x2D3] = D_800D2DD8[i] + 1;
    *pCurrent = i + 1;
    if (*pCurrent == 0xB) {
        *pCurrent = 0;
    }
    func_80071B94(0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_8007252C);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_800728B8);
/* 78.84% under both 2.6.0 and 2.7.2: the original recomputes index*40 inline
   each iteration (sll 2 / addu / sll 3, the sll 2 rotated into the branch delay
   slot) while gcc strength-reduces an int index into a pointer (+80/iteration)
   and hoists pPrim into it. An s16 index emits the original's inline multiply
   but adds a sll/sra pair per use. Tried under 2.6.0: index += 2 in the body,
   index += 2 in the for clause, u32/s16/s32 index.
void func_800728B8(u8 *pPrim, s32 count, s16 index) {
    s32 i;

    for (i = 0; i < count; i++) {
        AddPrim(g_BattleCurOT + 4, pPrim + index * 40);
        index += 2;
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072938);
/* 83.62% - body is otherwise exact: POLY_FT4 array subscripted by
   `g_BattlePrimStartIndex + i * 2` repeated per store (which is why the original reloads
   g_BattlePrimStartIndex four times per iteration), SetShadeTex first, then r0/g0, then b0.
   The `!= 1` spelling is what reproduces the original's branch layout (beq to the
   out-of-line r0/g0 = 0x80 block, fall through to r0 = 0x80 / g0 = 0).
   Left: 2.7.2 keeps the index `i` in a callee-saved reg and re-does `sll i,1` each
   iteration (6 saved regs, 0x30 frame) and re-masks `mode` in the loop; ours makes
   `i*2` itself the loop induction and hoists `andi mode,0xFF` into the preheader
   (5 saved regs, 0x28 frame), which also shifts s3-s5 down by one.
void func_80072938(POLY_FT4 *pPrim, s32 first, s32 last, s32 mode) {
    s32 i;

    for (i = first; i < last; i++) {
        SetShadeTex(&pPrim[g_BattlePrimStartIndex + i * 2], 0);
        if ((mode & 0xFF) != 1) {
            pPrim[g_BattlePrimStartIndex + i * 2].r0 = 0x80;
            pPrim[g_BattlePrimStartIndex + i * 2].g0 = 0;
        } else {
            pPrim[g_BattlePrimStartIndex + i * 2].r0 = 0x80;
            pPrim[g_BattlePrimStartIndex + i * 2].g0 = 0x80;
        }
        pPrim[g_BattlePrimStartIndex + i * 2].b0 = 0;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072A9C);
void BattleRenderActorStatDigits(s32 slot, u8 arg1) {
    s32 first;
    s32 i;

    first = g_BattleDisplayState->statSpriteCount[slot];
    for (i = 0; i < 5; i++) {
        if (D_800D2D88[i] != 0xFF) {
            g_BattleDisplayState->statSpriteCount[slot] += BattleDrawSpritePackage(
                D_800D2D88[i] + 0x67,
                (s32)(g_BattleGraphicsWork + (slot * 3200 + 0x3A88)) + g_BattleDisplayState->statSpriteCount[slot] * 80,
                (s16)(D_800C3076[slot * 24 + i] + D_800C3254[D_800D3280 * 3 + slot]),
                0x10);
        }
    }
    if (arg1 != 0) {
        func_80072938((POLY_FT4 *)(g_BattleGraphicsWork + (slot * 3200 + 0x3A88)), first,
                      g_BattleDisplayState->statSpriteCount[slot], arg1);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80072F38);
void BattleRenderActorValueDigits(s32 slot) {
    u8 *pSlotGraphics;
    s16 amount;
    s32 off;
    s32 i;

    off = slot * 484;
    pSlotGraphics = g_BattleGraphicsWork;
    pSlotGraphics = pSlotGraphics + off;
    // 0x853D is the slot's (0x1E4 stride) status byte in the graphics work block.
    if (pSlotGraphics[0x853D] == 1) {
        amount = D_800D32A0[slot * 8];
    } else {
        amount = D_800CCDC4[slot].flags;
    }
    BattleParseNumberToDigits(amount);

    for (i = 0; i < 4; i++) {
        if (D_800C3CF9[i] != 0xFF) {
            g_BattleDisplayState->valueSpriteCount[slot] += BattleDrawSpritePackage(
                D_800C3CF9[i] + 0x83,
                (s32)(g_BattleGraphicsWork + (slot * 320 + 0x6008)) + g_BattleDisplayState->valueSpriteCount[slot] * 80,
                (s16)(slot * 96 + (D_800C3254[D_800D3280 * 3 + slot] + 0x4A) + i * 6),
                0x25);
        }
    }
    g_BattleDisplayState->unk99[slot] = g_BattlePrimStartIndex;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073538);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc1", func_80073A58);
/* 91.30% - the three colour stores, the `lbu 0x6415` guard, the reload-after-store
   pattern (g_BattleGraphicsWork is a u8 * so the sb forces a reload) and both `addu a1,a1,v1`
   (offset-first) are exact. The only remaining miss is the link address: retail
   keeps `&g_BattleCurOT` in s0 (one lui/addiu, then `lw a0,0(s0)` per call, 0x18 frame
   with s0 saved) while gcc re-forms lui/lw per use and needs no saved register
   (0x10 frame). Tried: `u8 **pOt = &g_BattleCurOT` then `*pOt` (constant-propagated
   away) and the bare `g_BattleCurOT + 4` form (identical 91.30%). Same open problem as
   misc2's func_800743A4 and func_800744BC - no shape found in the repo makes gcc
   materialise the address of a global once across calls.
void func_80073A58(void) {
    if (((u8 *)g_BattleGraphicsWork)[0x6415] != 0) {
        ((u8 *)g_BattleGraphicsWork)[0x63CC + ((u8 *)g_BattleGraphicsWork)[0x6414] * 24] = ((u8 *)g_BattleGraphicsWork)[0x6410];
        ((u8 *)g_BattleGraphicsWork)[0x63CD + ((u8 *)g_BattleGraphicsWork)[0x6414] * 24] = ((u8 *)g_BattleGraphicsWork)[0x6410];
        ((u8 *)g_BattleGraphicsWork)[0x63CE + ((u8 *)g_BattleGraphicsWork)[0x6414] * 24] = ((u8 *)g_BattleGraphicsWork)[0x6410];
        AddPrim(g_BattleCurOT + 4, (0x63C8 + ((u8 *)g_BattleGraphicsWork)[0x6414] * 24) + (u8 *)g_BattleGraphicsWork);
        AddPrim(g_BattleCurOT + 4, (0x63F8 + ((u8 *)g_BattleGraphicsWork)[0x6414] * 12) + (u8 *)g_BattleGraphicsWork);
    }
}
*/
