#include "common.h"
#include "battle/actor.h"

// One 8-byte battle command argument record, indexed by actor. Entry 0 sits at
// D_800D2E5C, the same address this unit clears as a 0x100-byte scratch buffer.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
    /* 0x6 */ u16 unk6;
} BattleCommandArg; // Size: 0x8

// The command queue header D_800C3EAC points at. Only the tail this unit fills
// is named.
typedef struct {
    /* 0x000 */ u8 unk000[0x2D3];
    /* 0x2D3 */ u8 unk2D3;
    /* 0x2D4 */ u8 unk2D4[6];
    /* 0x2DA */ u8 count; // queued commands
    /* 0x2DB */ u8 unk2DB;
    /* 0x2DC */ u8 unk2DC;
} BattleCommandQueue; // Size: 0x2DD

extern u8 D_800D2CA4[];
extern BattleCommandArg D_800D2E5C[];
extern u16 D_800D39E0;
extern BattleSlotRegs D_800D3400[];
extern s32 func_8007EF6C(u8** arg0, u8 arg1, u8 arg2);
extern s32 func_8007F8C0(u8** arg0, u8 arg1);

// A 4-byte status entry; only the first byte is written here.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1[3];
} BattleStatusEntry; // Size: 0x4

extern BattleStatusEntry D_800C3D1B[];
extern u8 D_800D342E[];
extern u8 D_800D3439[];
extern u8 D_800D343E[];
extern u16 D_800C48E8;
extern s32 BattleSlotBitMask(u8 arg0);
extern u8 D_800D2DE4[];
extern u8 D_800C3D18[];
extern BattleTableEntry D_800CCD64[];
extern u8 D_800D32A1[];
extern u16 D_800D2C94;
extern void BattleBeginActorAction(u8 arg0, u16 arg1, u8 arg2);
extern void BattleApplySlotCurveAndStep(u8 arg0);
extern u8 D_800D366C;
extern s32* D_8005917C;
extern char D_8006FB08[];
extern char D_8006FB0C[];
extern char D_8006FB24[];
extern void func_800877E0(u8 arg0, u8 arg1);
extern void func_80087EDC(u8 arg0, u8 arg1);
extern void func_800881B8(u8 arg0, u8 arg1);
extern void BattleBeginAnimTask(u16 arg0);
// A 0x48-byte row of the per-battle-slot table at D_800C3FFE.
typedef u16 BattleSlotRow[36]; // Size: 0x48

extern u8 D_800C3E8C;
extern u8 D_800C400B[];
extern BattleSlotRow D_800C4022[];
extern BattleSlotRow D_800C3FFE[];
extern BattleCommandQueue *D_800C3EAC;
extern u8 D_800C402F[];
extern void func_800785D4(u8 arg0, u8 arg1);
extern void func_80078658(u8 arg0, u8 arg1);
extern void BattleQueueCommandF7(u8 arg0, u8 arg1);
extern void BattleQueueCommandF8(u8 arg0);
extern s32 func_8007A280(u8 arg0, u8 arg1, s32 arg2, s32 arg3);
extern void BattleUpdateFrame(void);
extern void func_800793F0(u8 arg0);
extern void BattleQueueCommand1BAndFE(u8 arg0);
extern void BattleClearSlotCurveState(void);
extern void BattleClearSlotCurveEntry(void);
extern s32 BattleFindMinValueOtherSlot(u8 arg0);
extern void func_800B89FC(s32 arg0, u8 arg1, s32 arg2, s32 arg3);
extern s32 func_80079ED8(u8 arg0, u8 arg1, u8 arg2, s32 arg3);
extern u8 D_800C3D19[];
extern u8 D_800C3D1A[];
extern u8 D_800D340C[];
extern BattleTableEntry D_800CCD1C[];
extern void BattleSetOverlayArg(u8 arg0);

void BattleQueueCommandF8(u8 arg0) {
    if (D_800C3E8C != 0) {
        D_800C400B[D_800C3EAC->count * 72] = arg0;
        D_800C402F[D_800C3EAC->count * 72] = 0xF8;
        D_800C4022[D_800C3EAC->count][0] = D_800C3E8C - 1;
        D_800C3EAC->count = D_800C3EAC->count + 1;
    }
}
void BattleQueueCommandF8IfFlagged(u8 arg0, u8 arg1) {
    if (D_800D2E5C[arg0].unk3 != 0) {
        func_80078658(arg0, arg1);
        BattleQueueCommandF7(0x1E, arg1);
        BattleQueueCommandF8(arg1);
    }
}
/* Best: 89.11% (408 bytes vs retail 420), now COMPILES (the old 68.28% note
   was a body that did not build: it indexed the BattleCommandQueue pointer as an
   array, D_800C3EAC[0x2DA], which the typed struct rejects). Structure is exact -
   all four calls, the 0xFD scan and both table stores match. Three insns short:
   retail forms &D_800D2C94 ONCE into $s1 (lui/addiu) and reuses it for the
   pre-call store and both post-call compares, and copies it to $t0 for the loop
   plus the queue pointer to $a2 and i=0 in the preheader; gcc here rematerialises
   the pValue address at each use (lui %hi + lhu %lo) and hoists pQueue/i above
   the beq. Tried: pValue assigned at the top of the body and just before the
   D_800C3FFE store (identical), no pQueue local (reloads D_800C3EAC inside the
   loop). The remaining gap is remat/scheduling, not source shape. *//*
void func_80078998(u8 arg0, u8 arg1) {
    BattleCommandQueue *pQueue;
    u16 *pValue;
    s32 i;
    s32 n;

    BattleQueueCommandF8IfFlagged(arg1, arg0);
    D_800C3EAC->unk2DC = D_800D2E5C[arg1].unk1 + 1;
    BattleBeginActorAction(arg0, D_800D2E5C[arg1].unk6, D_800D2E5C[arg1].unk2);
    BattleApplySlotCurveAndStep(D_800C3EAC->count);
    D_800C402F[D_800C3EAC->count * 72] = D_800D2E5C[arg1].unk2;
    pValue = &D_800D2C94;
    D_800C3FFE[D_800C3EAC->count][0] = *pValue;
    func_800785D4(arg0, arg1);
    if (D_800D2E5C[arg1].unk6 != *pValue) {
        pQueue = D_800C3EAC;
        i = 0;
        n = 0;
        while (n < pQueue->count) {
            if (D_800C402F[i] == 0xFD) {
                *(u16 *)((u8 *)D_800C3FFE + i) = *pValue;
                break;
            }
            n++;
            i += 0x48;
        }
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078998);


void BattleQueueCommandFD(u8 arg0, u8 arg1, u8 arg2) {
    D_800C402F[D_800C3EAC->count * 72] = 0xFD;
    D_800C4022[D_800C3EAC->count][0] = 0;
    D_800C3FFE[D_800C3EAC->count][0] = D_800D2E5C[arg1].unk6;
    func_800877E0(arg0, arg2);
    if (D_800D2E5C[arg1].unk4 != 1) {
        if (D_800D32A1[arg0 * 8] == 0) {
            func_80087EDC(arg0, arg2);
        } else {
            func_800881B8(arg0, arg2);
        }
    }
    BattleBeginAnimTask(BattleSlotBitMask(arg0) | D_800D2E5C[arg1].unk6);
    func_800785D4(arg0, arg1);
}
void BattleQueueCommandFC(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC->count * 72] = 0xFC;
    func_800785D4(arg0, arg1);
}
void BattleQueueCommandFromRecordByte(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC->count * 72] = D_800D2E5C[arg1].unk4;
    func_800785D4(arg0, arg1);
}

void BattleSetPendingActorMask(u8 arg0, u8 arg1) {
    D_800D39E0 = D_800D2E5C[arg1].unk6;
}

void func_80078D6C(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC->count * 72] = 0xF9;
    func_800785D4(arg0, arg1);
    D_800D2DE4[arg0] = 0xFF;
    // The empty-block wrapper keeps this store ahead of the D_800CCD64 read-modify-write.
    do {
        D_800C3D1B[arg0 - 3].unk0 = 1;
    } while (0);
    D_800CCD64[arg0].flags &= 0x8000;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078E24);
void BattleApplyCommandByteVar(u8 arg0, u8 arg1) {
    func_80079ED8(arg0, D_800D2E5C[arg1].unk1, D_800D2E5C[arg1].unk4, 0);
}
void BattleAddCommandByteVar(u8 arg0, u8 arg1) {
    s32 value;

    value = func_80079ED8(arg0, D_800D2E5C[arg1].unk1, 0, 1);
    func_80079ED8(arg0, D_800D2E5C[arg1].unk1, D_800D2E5C[arg1].unk4 + value, 0);
}
void BattleApplyCommandWordVar(u8 arg0, u8 arg1) {
    func_8007A280(arg0, D_800D2E5C[arg1].unk1, D_800D2E5C[arg1].unk4 | (D_800D2E5C[arg1].unk5 << 8), 0);
}
void BattleAddCommandWordVar(u8 arg0, u8 arg1) {
    s32 value;

    value = func_8007A280(arg0, D_800D2E5C[arg1].unk1, 0, 1);
    func_8007A280(arg0, D_800D2E5C[arg1].unk1,
                  (u16)(D_800D2E5C[arg1].unk4 + (value + (D_800D2E5C[arg1].unk5 << 8))), 0);
}
void BattleQueueCommandF4(u8 arg0, u8 arg1) {
    BattleQueueCommandF8IfFlagged(arg1, arg0);
    D_800C402F[D_800C3EAC->count * 72] = 0xF4;
    func_800785D4(arg0, arg1);
}
void BattleQueueCommandF6(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC->count * 72] = 0xF6;
    D_800C3FFE[D_800C3EAC->count][0] = D_800D2E5C[arg1].unk6;
    func_800785D4(arg0, arg1);
}
void BattlePrintLanguageErrorLoop(u8 arg0, u8 arg1) {
    s32 i;
    s32 j;
    s32 k;
    s32 off;
    u8 value;

    value = 0xFF;
    for (off = 0x8B8; off >= 0; off -= 0x48) {
        D_800C402F[off] = value;
    }
    j = 0;
    i = 0;
    if (*D_8005917C != -1) {
        for (;;) {
            for (k = 0; k < i; k++) {
                FontPrintf(D_8006FB08);
            }
            FontPrintf(D_8006FB0C);
            j++;
            FontPrintf(D_8006FB24, arg0, arg1);
            BattleUpdateFrame();
            if (j < 3) {
                continue;
            }
            i++;
            if (i >= 0x15) {
                i = 0;
            }
            j = 0;
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_800793F0);
void BattleQueueCommand1BAndFE(u8 arg0) {
    if (D_800D32A1[arg0 * 8] != 0) {
        D_800C400B[D_800C3EAC->count * 72] = arg0;
        D_800C402F[D_800C3EAC->count * 72] = 0x1B;
        D_800C3EAC->count = D_800C3EAC->count + 1;
    }
    D_800C400B[D_800C3EAC->count * 72] = arg0;
    D_800C402F[D_800C3EAC->count * 72] = 0xFE;
    D_800D366C = 0;
}
void func_80079778(u8 arg0) {
    D_800C3EAC->count = 0;
    BattleClearSlotCurveState();
    BattleClearSlotCurveEntry();
    D_800C400B[D_800C3EAC->count * 72] = arg0;
    BattleUpdateFrame();
    func_800B89FC(1, arg0, 0, BattleFindMinValueOtherSlot(arg0));
    func_800793F0(arg0);
    BattleQueueCommand1BAndFE(arg0);
    while (D_800C3EAC->unk2DB == 0) {
        BattleUpdateFrame();
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079840);
/* Best: 96.48%. Two emit-order/allocator tie-breaks left: the loop preheader
   puts `move a1,v1` after `la a2` where the original has it first, and the
   D_800C3EAC load adds index-first (`addu v0,v0,v1`) where the original adds
   base-first (`addu v1,v1,v0`). Tried: D_800C3EAC[slot * 0x40 + 0x3C] inline,
   the (mem_addr) cast form, and a u8* pBuf local (96.48%). The slot/off split
   matters: `u8 slot = arg1 - 3` with `slot * 0x40` at each use reproduces the
   original's andi-then-sll rematerialisation; folding them into one u8 off
   gives a single sll in the preheader and 93.20%.

void func_80079840(u8 arg0, u8 arg1) {
    u8 slot;
    s32 i;
    u8* pBuf;

    if (arg1 >= 3) {
        slot = arg1 - 3;
        *(u16*)&D_800D342E[slot * 0x40] = BattleSlotBitMask(arg0);
        for (i = 0; i < 5; i++) {
            D_800D3439[slot * 0x40 + i] = D_800D2CA4[i];
        }
        pBuf = D_800C3EAC + (arg0 & 0xFF) * 0x40;
        if (D_800C48E8 & BattleSlotBitMask(pBuf[0x3C])) {
            D_800D343E[slot * 0x40] = 1;
        } else {
            D_800D343E[slot * 0x40] = 0;
        }
    }
}
*/void BattleScriptCursorSkipWord(u8** pCursor) {
    *pCursor += 4;
}
void BattleScriptSkipUnknownOpcodeWords(u8** pCursor) {
    while (**pCursor >= 0x80) {
        BattleScriptCursorSkipWord(pCursor);
    }
    while ((u32)((**pCursor - 0x80) & 0xFF) >= 0x70) {
        BattleScriptCursorSkipWord(pCursor);
    }
}
void BattleScriptResetActorState(u8 arg0) {
    u8* pCursor;
    u8 value;
    u8 slot;
    s32 i;
    u8* p;
    u8* pEnd;

    value = 0;
    slot = arg0 - 3;
    pCursor = *(u8 **)D_800D3400[slot].unk0;
    p = (u8 *)D_800D2E5C;
    pEnd = p + 0x100;
    do {
        *p++ = 0;
    } while (p < pEnd);
    for (i = 0x8B8; i >= 0; i -= 0x48) {
        D_800C402F[i] = 0xFF;
    }
    while (*pCursor != 0xFD) {
        if (*pCursor == 0xFF) {
            break;
        }
        if (*pCursor >= 0x80) {
            if ((func_8007F8C0(&pCursor, slot) & 0xFF) != 0) {
                continue;
            }
            BattleScriptSkipUnknownOpcodeWords(&pCursor);
        } else {
            value = func_8007EF6C(&pCursor, slot, value);
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079AB0);
/* Best: 99.78%, retail's exact 372 bytes, ONE register left: the temp for the
   loop tail's `cmd = *pCursor` load is $v0 here and $a0 in retail (`lw a0,0x10(sp)`
   / `lbu v1,0x0(a0)`). Everything else is identical, including the single 0xFD
   and 0xFF registers in the back edge and the 0x62 in the bnez delay slot.
   The lever that closed the old 98.55% park's two residuals in one change was
   replacing the early `return result` with `goto done` and one shared return:
   gcc had folded that return to `move v0,zero` (it can prove result == 0 there)
   and, with it folded, also re-derived `(u8)(arg0 - 3)` for the slot*4 / slot*0x40
   uses instead of keeping retail's single `andi a0,s0,0xff`; the shared return
   gives one epilogue block and gcc then copies `result` into $v0 in all three
   branch delay slots, exactly as retail does.
   Tried on the last register: permute.sh 4 min / 947 iterations (PERMUTER_NONE,
   base=10), u8 cmd (376 bytes) and s32 cmd (slti, 99.14%), declaration order of
   all eight locals, slot assignment first, `pCursor[0]` for either or both loads.
   */
/*
s32 func_80079AB0(u8 arg0) {
    u8 slot;
    s32 result;
    u8 value;
    u32 cmd;
    u8* pCursor;
    u8* p;
    u8* pEnd;

    result = 0;
    value = 0;
    slot = arg0 - 3;
    if ((D_800CCD64[slot + 3].flags & 0x8000) != 0) {
        if ((D_800CCD1C[slot + 3].flags & 0x800) == 0) {
            goto done;
        }
    }
    D_800D2E5C[0].unk0 = 0;
    if (D_800C3D18[slot * 4] != 0) {
        pCursor = *(u8 **)&D_800D3400[slot].unk0[8];
        p = D_800D2E5C;
        pEnd = p + 0x100;
        do {
            *p++ = 0;
        } while (p < pEnd);
        cmd = *pCursor;
        while (cmd != 0xFD) {
            if (cmd == 0xFF) {
                break;
            }
            if (cmd >= 0x80) {
                if ((func_8007F8C0(&pCursor, slot) & 0xFF) == 0) {
                    BattleScriptSkipUnknownOpcodeWords(&pCursor);
                }
            } else {
                if (cmd == 0x62) {
                    result = 1;
                }
                value = func_8007EF6C(&pCursor, slot, value);
            }
            cmd = *pCursor;
        }
    }
    if (D_800D2E5C[0].unk0 != 0) {
        func_800793F0(slot + 3);
    }
done:
    return result;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079C24);
/* Best: 89.50%, instruction stream identical except two hoist positions (508
   bytes vs retail's 504). Retail stores value/i, then the two loop constants
   (0xFD -> $s7, 0xFF -> $s6) and only then idx/off/off2/index, and it forms
   `la $v1,%lo(D_800D2E5C)` between the D_800CCD64 `lhu` and its `andi`; here
   both hoisted pairs land after the source inits / at the start of the block.
   Everything else is exact: the index/off/off2 biv order (s1/s2/s4), idx as an
   incrementing s32 biv started at 3, the do-while clear loop (rotated, $j with
   the store in its delay slot), the 0x8B8 loop with `li v0,0x8b8` duplicated in
   both flag-test delay slots, and the `while (cmd != 0xFD)` form with the 0xFF
   break (that form, not `for (;;)` with two breaks, is what shares one register
   for 0xFD between the entry test and the back edge — 85.57% -> 89.50%).
   Tried: p/pEnd formed before the flag test (moves the `la` three insns too
   early, 89.17%), for(;;) with two breaks, idx = i + 3 in the body.

/*
void func_80079C24(void) {
    s32 i;
    s32 off;
    s32 off2;
    s32 index;
    s32 j;
    s32 idx;
    u32 value;
    u8 saved;
    u8* pCursor;
    u8* p;
    u8* pEnd;
    u32 cmd;

    saved = D_800C3EAC[0x2D3];
    value = 0;
    i = 0;
    for (idx = 3, off = 0x450, off2 = 0, index = 0; i < 8;
         idx++, off += 0x170, off2 += 0x40, index += 4, i++) {
        if (D_800C3D1A[index] == 0) {
            continue;
        }
        if (D_800C3D19[index] == 0) {
            continue;
        }
        D_800C3EAC[0x2D3] = idx;
        pCursor = *(u8**)(D_800D340C + off2);
        if ((*(u16*)((u8*)D_800CCD64 + off) & 0x8000) != 0) {
            if ((*(u16*)(D_800CCD1C + off) & 0x800) == 0) {
                continue;
            }
        }
        for (j = 0x8B8; j >= 0; j -= 0x48) {
            D_800C402F[j] = 0xFF;
        }
        p = D_800D2E5C;
        pEnd = p + 0x100;
        do {
            *p++ = 0;
        } while (p < pEnd);
        cmd = *pCursor;
        while (cmd != 0xFD) {
            if (cmd == 0xFF) {
                break;
            }
            if (cmd >= 0x80) {
                if ((func_8007F8C0(&pCursor, i) & 0xFF) != 0) {
                    cmd = *pCursor;
                    continue;
                }
                BattleScriptSkipUnknownOpcodeWords(&pCursor);
            } else {
                value = func_8007EF6C(&pCursor, i, value);
            }
            cmd = *pCursor;
        }
        D_800C3EAC[0x2DB] = 0;
        func_80079778(idx);
        BattleSetOverlayArg(0);
    }
    D_800C3EAC[0x2D3] = saved;
}
*/
