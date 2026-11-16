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
extern s32 BattleSlotWordVarGetSet(u8 arg0, u8 arg1, s32 arg2, s32 arg3);
extern void BattleUpdateFrame(void);
extern void func_800793F0(u8 arg0);
extern void BattleQueueCommand1BAndFE(u8 arg0);
extern void BattleClearSlotCurveState(void);
extern void BattleClearSlotCurveEntry(void);
extern s32 BattleFindMinValueOtherSlot(u8 arg0);
extern void func_800B89FC(s32 arg0, u8 arg1, s32 arg2, s32 arg3);
extern s32 BattleSlotByteVarGetSet(u8 arg0, u8 arg1, u8 arg2, s32 arg3);
extern u8 D_800C3D19[];
extern u8 D_800C3D1A[];
extern u8 D_800D340C[];
extern BattleTableEntry D_800CCD1C[];
extern void BattleSetOverlayArg(u8 arg0);

extern u8 D_800C3E3D[];
extern u8 D_800D3404[];
extern u8 D_800D3408[];
extern BattleTableEntry D_800CCCE8[];
extern u16 BattleMaskHasSlotBit(u16 arg0, u8 arg1);

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
/* Best: 89.11% (408 bytes vs retail 420), and it compiles. Structure is exact -
   all four calls, the 0xFD scan and both table stores line up; the whole gap is
   &D_800D2C94: retail forms it ONCE into $s1 (lui/addiu), keeps it across the
   func_800785D4 call and reads it three times (`lhu v1,0(s1)`, `lhu v0,0(s1)`,
   and through a copy in $t0 for the scan). gcc 2.7.2 folds the constant address
   into each load (`lui %hi; lhu %lo`), which costs the three instructions.
   MEASURED: under -G0 gcc DOES keep &D_800D2C94 in a register (the -G0 build of
   this function scores 94.59%), so this is the -G setting, not the source - but
   this unit cannot move to -G0: matched BattleQueueCommandFD drops to 99.67.
   The same wall is parked in misc15 func_8008ADD0 (D_800D2C94 again), so it is
   one shared question, not a per-function one. Tried: pValue assigned at the
   top and just before the D_800C3FFE store (identical), no pQueue local
   (reloads D_800C3EAC in the loop), u16 *pValue = &D_800D2C94.
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

void BattleQueueCommandF9(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC->count * 72] = 0xF9;
    func_800785D4(arg0, arg1);
    D_800D2DE4[arg0] = 0xFF;
    // The empty-block wrapper keeps this store ahead of the D_800CCD64 read-modify-write.
    do {
        D_800C3D1B[arg0 - 3].unk0 = 1;
    } while (0);
    D_800CCD64[arg0].flags &= 0x8000;
}

/* Best: 70.25% (560 bytes, exactly retail's size). The prologue and the flag
   scan are byte-identical, including the `idx` copy the loop needs (`u8 idx = i`
   at the top of the body, `slot = idx` in the break): with `slot = i` written
   directly gcc keeps the branch inverted and the loop never matches. Everything
   from the 0xC3E3D store to the memmove differs only in instruction ORDER and
   CSE: retail hoists (slot-3)<<6, (arg0-3)<<6 and then arg0*0x170 in that order
   and keeps one register (v0) for all four register-file words, ours hoists
   arg0*0x170 first and merges the four loads above the four stores. The copy is
   four u32 accesses off the four absolute symbols D_800D3400/04/08/0C (that is
   what retail's four distinct relocations are), NOT one typed struct copy:
   `*(BattleSlotRegsHead *)...= *(...)` gives a 4-register block move and
   `D_800D3400[i].unk0` lanes give a base register instead of retail's
   symbol+index `$at` form. Tried: u8 src/dst locals (masks the result, 62.56),
   inline (slot-3)*0x40 at every use (68.14, 552 bytes), the four copies wrapped
   in an empty do/while block (68.89). Remaining gap is cse/insn-order, not
   source shape. NOTE the four loads also want to stay *interleaved* with their
   stores; ours hoists them (t0-t3 vs retail's repeated v0).
void func_80078E24(u8 arg0, u8 arg1) {
    u8 slot;
    u8 idx;
    s32 i;
    s32 src;
    s32 dst;

    for (i = 0; i < 11; i++) {
        idx = i;
        if (BattleMaskHasSlotBit(D_800D2E5C[arg1].unk6, idx) != 0) {
            slot = idx;
            break;
        }
    }
    src = (slot - 3) * 0x40;
    dst = (arg0 - 3) * 0x40;
    D_800C3E3D[arg0] = D_800C3E3D[slot];
    *(u32 *)((u8 *)D_800D3400 + dst) = *(u32 *)((u8 *)D_800D3400 + src);
    *(u32 *)(D_800D3404 + dst) = *(u32 *)(D_800D3404 + src);
    *(u32 *)(D_800D3408 + dst) = *(u32 *)(D_800D3408 + src);
    *(u32 *)(D_800D340C + dst) = *(u32 *)(D_800D340C + src);
    D_800C3D18[(arg0 - 3) * 4] = D_800C3D18[(slot - 3) * 4];
    D_800C3D19[(arg0 - 3) * 4] = D_800C3D19[(slot - 3) * 4];
    memmove(&D_800CCCE8[arg0], &D_800CCCE8[slot], sizeof(BattleTableEntry));
    D_800C4022[D_800C3EAC->count][0] = slot;
    D_800C402F[D_800C3EAC->count * 72] = 0xFB;
    D_800C400B[D_800C3EAC->count * 72] = arg0;
    D_800C3EAC->count = D_800C3EAC->count + 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078E24);
void BattleApplyCommandByteVar(u8 arg0, u8 arg1) {
    BattleSlotByteVarGetSet(arg0, D_800D2E5C[arg1].unk1, D_800D2E5C[arg1].unk4, 0);
}
void BattleAddCommandByteVar(u8 arg0, u8 arg1) {
    s32 value;

    value = BattleSlotByteVarGetSet(arg0, D_800D2E5C[arg1].unk1, 0, 1);
    BattleSlotByteVarGetSet(arg0, D_800D2E5C[arg1].unk1, D_800D2E5C[arg1].unk4 + value, 0);
}
void BattleApplyCommandWordVar(u8 arg0, u8 arg1) {
    BattleSlotWordVarGetSet(arg0, D_800D2E5C[arg1].unk1, D_800D2E5C[arg1].unk4 | (D_800D2E5C[arg1].unk5 << 8), 0);
}
void BattleAddCommandWordVar(u8 arg0, u8 arg1) {
    s32 value;

    value = BattleSlotWordVarGetSet(arg0, D_800D2E5C[arg1].unk1, 0, 1);
    BattleSlotWordVarGetSet(arg0, D_800D2E5C[arg1].unk1,
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
s32 BattleScriptScanForOpcode62(u8 arg0) {
    u8 slot;
    s32 result;
    u8 value;
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
        while (*pCursor != 0xFD) {
            if (*pCursor == 0xFF) {
                break;
            }
            if (*pCursor >= 0x80) {
                if ((func_8007F8C0(&pCursor, slot) & 0xFF) == 0) {
                    BattleScriptSkipUnknownOpcodeWords(&pCursor);
                }
            } else {
                if (*pCursor == 0x62) {
                    result = 1;
                }
                value = func_8007EF6C(&pCursor, slot, value);
            }
        }
    }
    if (D_800D2E5C[0].unk0 != 0) {
        func_800793F0(slot + 3);
    }
done:
    return result;
}
void BattleRunActiveSlotScripts(void) {
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
    u16 flags;
    u32 stopOp;
    u32 endOp;

    saved = D_800C3EAC->unk2D3;
    value = 0;
    i = 0;
    stopOp = 0xFD;
    endOp = 0xFF;
    for (idx = 3, off = 0x450, off2 = 0, index = 0; i < 8;
         idx++, off += 0x170, off2 += 0x40, index += 4, i++) {
        if (D_800C3D1A[index] == 0) {
            continue;
        }
        if (D_800C3D19[index] == 0) {
            continue;
        }
        D_800C3EAC->unk2D3 = idx;
        pCursor = *(u8**)(D_800D340C + off2);
        flags = *(u16*)((u8*)D_800CCD64 + off);
        p = D_800D2E5C;
        if ((flags & 0x8000) != 0) {
            if ((*(u16*)((u8*)D_800CCD1C + off) & 0x800) == 0) {
                continue;
            }
        }
        for (j = 0x8B8; j >= 0; j -= 0x48) {
            D_800C402F[j] = endOp;
        }
        pEnd = p + 0x100;
        do {
            *p++ = 0;
        } while (p < pEnd);
        while (*pCursor != stopOp) {
            if (*pCursor == endOp) {
                break;
            }
            if (*pCursor >= 0x80) {
                if ((func_8007F8C0(&pCursor, i) & 0xFF) != 0) {
                    continue;
                }
                BattleScriptSkipUnknownOpcodeWords(&pCursor);
            } else {
                value = func_8007EF6C(&pCursor, i, value);
            }
        }
        D_800C3EAC->unk2DB = 0;
        func_80079778(idx);
        BattleSetOverlayArg(0);
    }
    D_800C3EAC->unk2D3 = saved;
}
