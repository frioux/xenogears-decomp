#include "common.h"
#include "battle/actor.h"

// One 8-byte battle command argument record, indexed by actor. Entry 0 sits at
// g_BattleCommandArgs, the same address this unit clears as a 0x100-byte scratch buffer.
typedef struct {
    /* 0x0 */ u8 opcode;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
    /* 0x6 */ u16 slotMask;
} BattleCommandArg; // Size: 0x8

// The command queue header g_BattleCommandQueue points at. Only the tail this unit fills
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
extern BattleCommandArg g_BattleCommandArgs[];
extern u16 g_BattlePendingSlotMask;
extern BattleSlotRegs g_BattleSlotRegs[];
extern s32 BattleExecScriptOpcode(u8** arg0, u8 arg1, u8 arg2);
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
extern u16 g_BattleSlotFlag8000Mask;
extern s32 BattleSlotBitMask(u8 arg0);
extern u8 g_BattleActorCountdownState[];
extern u8 g_BattleSlotScriptFlags[];
extern BattleTableEntry g_BattleSlotEntries[];
extern u8 g_BattleSlotFlags[];
extern u16 g_BattleActionSlotMask[];
extern void BattleBeginActorAction(u8 arg0, u16 arg1, u8 arg2);
extern void BattleApplySlotCurveAndStep(u8 arg0);
extern u8 g_BattleSoundEnabled;
extern s32* g_DebuggerState;
extern char g_StrSpace[];
extern char g_StrLanguageError[];
extern char g_StrActorNoFormat[];
extern void func_800877E0(u8 arg0, u8 arg1);
extern void func_80087EDC(u8 arg0, u8 arg1);
extern void func_800881B8(u8 arg0, u8 arg1);
extern void BattleBeginAnimTask(u16 arg0);
// A 0x48-byte row of the per-battle-slot table at g_BattleCommandRecords.
typedef u16 BattleSlotRow[36]; // Size: 0x48

extern u8 g_BattleTextSlotCount;
extern u8 D_800C400B[];
extern BattleSlotRow D_800C4022[];
extern BattleSlotRow g_BattleCommandRecords[];
extern BattleCommandQueue *g_BattleCommandQueue;
extern u8 D_800C402F[];
extern void func_800785D4(u8 arg0, u8 arg1);
extern void func_80078658(u8 arg0, u8 arg1);
extern void BattleQueueCommandF7(u8 arg0, u8 arg1);
extern void BattleQueueCommandF8(u8 arg0);
extern s32 BattleSlotWordVarGetSet(u8 arg0, u8 arg1, s32 arg2, s32 arg3);
extern void BattleUpdateFrame(void);
extern void BattleDispatchCommandRecords(u8 arg0);
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
extern BattleTableEntry g_BattleWorkBlock[];
extern u16 BattleMaskHasSlotBit(u16 arg0, u8 arg1);
extern void func_80078998(u8 arg0, u8 arg1, u8 arg2);
extern void func_80078E24(u8 arg0, u8 arg1, u8 arg2);

void BattleQueueCommandF8(u8 arg0) {
    if (g_BattleTextSlotCount != 0) {
        D_800C400B[g_BattleCommandQueue->count * 72] = arg0;
        D_800C402F[g_BattleCommandQueue->count * 72] = 0xF8;
        D_800C4022[g_BattleCommandQueue->count][0] = g_BattleTextSlotCount - 1;
        g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
    }
}
void BattleQueueCommandF8IfFlagged(u8 arg0, u8 arg1) {
    if (g_BattleCommandArgs[arg0].unk3 != 0) {
        func_80078658(arg0, arg1);
        BattleQueueCommandF7(0x1E, arg1);
        BattleQueueCommandF8(arg1);
    }
}
/* Best: 95.44%, 400 bytes vs retail's 412 (103 insns). The whole gap is the loop PREHEADER:
   retail has "move t0,s1" (mask base), "move a2,v1" (queue pointer) and a separate "move a0,zero"
   after a nop in the beq delay slot; ours cse-merges both pointers into the register that already
   holds them and reorg steals i = 0 into the beq slot, so we are exactly 3 instructions short.
   Both copies only appear when the value is NOT cse-equal to the pre-existing register: writing the
   loop's mask as g_BattleActionSlotMask[0] does emit a copy (c6, 412 bytes) but it also needs a
   second callee-saved register (frame 0x30, saves s3) - retail saves s0/s1/s2 only. Writing the loop
   test against g_BattleCommandQueue instead of a local reloads the pointer every iteration (t1,
   412 bytes, 91.63%). Tried and measured no better than 95.44: pMask/pValue assignment in every
   position (93.91), for-loops with one or two induction variables (88.64/95.44), do/while wrapper
   (95.44), n*0x48 strength-reduction forms (84.32), non-void return type (unchanged), array element
   at each of the three mask sites (8 combinations, 86.7-95.4). Permuter: 1936 iterations, no
   improvement from base=474. */
/*
void func_80078998(u8 arg0, u8 arg1, u8 arg2) {
    BattleCommandQueue *pQueue;
    u16 *pValue;
    u16 *pMask;
    s32 i;
    s32 n;

    BattleQueueCommandF8IfFlagged(arg1, arg0);
    g_BattleCommandQueue->unk2DC = g_BattleCommandArgs[arg1].unk1 + 1;
    BattleBeginActorAction(arg0, g_BattleCommandArgs[arg1].slotMask, g_BattleCommandArgs[arg1].unk2);
    BattleApplySlotCurveAndStep(g_BattleCommandQueue->count);
    D_800C402F[g_BattleCommandQueue->count * 72] = g_BattleCommandArgs[arg1].unk2;
    pValue = g_BattleActionSlotMask;
    g_BattleCommandRecords[g_BattleCommandQueue->count][0] = *(pMask = pValue);
    func_800785D4(arg0, arg1);
    if (g_BattleCommandArgs[arg1].slotMask != *pValue) {
        pQueue = g_BattleCommandQueue;
        i = 0;
        n = 0;
        while (n < pQueue->count) {
            if (D_800C402F[i] == 0xFD) {
                *(u16 *)((u8 *)g_BattleCommandRecords + i) = *pMask;
                break;
            }
            n++;
            i += 0x48;
        }
    }
}
 */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078998);




void BattleQueueCommandFD(u8 arg0, u8 arg1, u8 arg2) {
    D_800C402F[g_BattleCommandQueue->count * 72] = 0xFD;
    D_800C4022[g_BattleCommandQueue->count][0] = 0;
    g_BattleCommandRecords[g_BattleCommandQueue->count][0] = g_BattleCommandArgs[arg1].slotMask;
    func_800877E0(arg0, arg2);
    if (g_BattleCommandArgs[arg1].unk4 != 1) {
        if (g_BattleSlotFlags[arg0 * 8] == 0) {
            func_80087EDC(arg0, arg2);
        } else {
            func_800881B8(arg0, arg2);
        }
    }
    BattleBeginAnimTask(BattleSlotBitMask(arg0) | g_BattleCommandArgs[arg1].slotMask);
    func_800785D4(arg0, arg1);
}
void BattleQueueCommandFC(u8 arg0, u8 arg1, u8 arg2) {
    D_800C402F[g_BattleCommandQueue->count * 72] = 0xFC;
    func_800785D4(arg0, arg1);
}
void BattleQueueCommandFromRecordByte(u8 arg0, u8 arg1, u8 arg2) {
    D_800C402F[g_BattleCommandQueue->count * 72] = g_BattleCommandArgs[arg1].unk4;
    func_800785D4(arg0, arg1);
}

void BattleSetPendingActorMask(u8 arg0, u8 arg1, u8 arg2) {
    g_BattlePendingSlotMask = g_BattleCommandArgs[arg1].slotMask;
}

void BattleQueueCommandF9(u8 arg0, u8 arg1, u8 arg2) {
    D_800C402F[g_BattleCommandQueue->count * 72] = 0xF9;
    func_800785D4(arg0, arg1);
    g_BattleActorCountdownState[arg0] = 0xFF;
    // The empty-block wrapper keeps this store ahead of the g_BattleSlotEntries read-modify-write.
    do {
        D_800C3D1B[arg0 - 3].unk0 = 1;
    } while (0);
    g_BattleSlotEntries[arg0].flags &= 0x8000;
}

/* Best: 70.25%, retail's exact 560 bytes. The prologue, the slot-bit scan and everything from the
   memmove on match; the difference is INSTRUCTION ORDER inside the copy block, and it is a
   SCHEDULER difference, not a CSE one. Retail keeps the four register-file copies interleaved
   load/store with one register (v0) and emits the index chains between them
   ((slot-3)*0x40, (arg0-3)*0x40 first, then &g_BattleWorkBlock[arg0], then &g_BattleWorkBlock[slot]
   between the 2nd and 4th copies); our compile hoists all four `lw` above every store and puts both
   &g_BattleWorkBlock chains at the top of the block. Retail's four copies are four DISTINCT symbol
   relocations (g_BattleSlotRegs, D_800D3404, D_800D3408, D_800D340C, each with the same src/dst
   index), which is four lui/addiu $at pairs - a single 0x40-byte struct with 4 u32 fields would need
   one base and 3 fewer instructions, so the four-symbol form is the right one.
   Measured and discarded: -O1 63.53, -O3 70.25 (same C, so not a flag/level question), u8 src/dst
   locals 62.56, (slot-3)*0x40 inline at every use 68.14/552 bytes, the four copies in a do/while
   block 68.89, 2-argument definition (does not compile - the extern takes 3). */
/*
void func_80078E24(u8 arg0, u8 arg1, u8 arg2) {
    u8 slot;
    u8 idx;
    s32 i;
    s32 src;
    s32 dst;

    for (i = 0; i < 11; i++) {
        idx = i;
        if (BattleMaskHasSlotBit(g_BattleCommandArgs[arg1].slotMask, idx) != 0) {
            slot = idx;
            break;
        }
    }
    src = (slot - 3) * 0x40;
    dst = (arg0 - 3) * 0x40;
    D_800C3E3D[arg0] = D_800C3E3D[slot];
    *(u32 *)((u8 *)g_BattleSlotRegs + dst) = *(u32 *)((u8 *)g_BattleSlotRegs + src);
    *(u32 *)(D_800D3404 + dst) = *(u32 *)(D_800D3404 + src);
    *(u32 *)(D_800D3408 + dst) = *(u32 *)(D_800D3408 + src);
    *(u32 *)(D_800D340C + dst) = *(u32 *)(D_800D340C + src);
    g_BattleSlotScriptFlags[(arg0 - 3) * 4] = g_BattleSlotScriptFlags[(slot - 3) * 4];
    D_800C3D19[(arg0 - 3) * 4] = D_800C3D19[(slot - 3) * 4];
    memmove(&g_BattleWorkBlock[arg0], &g_BattleWorkBlock[slot], sizeof(BattleTableEntry));
    D_800C4022[g_BattleCommandQueue->count][0] = slot;
    D_800C402F[g_BattleCommandQueue->count * 72] = 0xFB;
    D_800C400B[g_BattleCommandQueue->count * 72] = arg0;
    g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
}
 */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078E24);

void BattleApplyCommandByteVar(u8 arg0, u8 arg1, u8 arg2) {
    BattleSlotByteVarGetSet(arg0, g_BattleCommandArgs[arg1].unk1, g_BattleCommandArgs[arg1].unk4, 0);
}

void BattleAddCommandByteVar(u8 arg0, u8 arg1, u8 arg2) {
    s32 value;

    value = BattleSlotByteVarGetSet(arg0, g_BattleCommandArgs[arg1].unk1, 0, 1);
    BattleSlotByteVarGetSet(arg0, g_BattleCommandArgs[arg1].unk1, g_BattleCommandArgs[arg1].unk4 + value, 0);
}
void BattleApplyCommandWordVar(u8 arg0, u8 arg1, u8 arg2) {
    BattleSlotWordVarGetSet(arg0, g_BattleCommandArgs[arg1].unk1, g_BattleCommandArgs[arg1].unk4 | (g_BattleCommandArgs[arg1].unk5 << 8), 0);
}
void BattleAddCommandWordVar(u8 arg0, u8 arg1, u8 arg2) {
    s32 value;

    value = BattleSlotWordVarGetSet(arg0, g_BattleCommandArgs[arg1].unk1, 0, 1);
    BattleSlotWordVarGetSet(arg0, g_BattleCommandArgs[arg1].unk1,
                  (u16)(g_BattleCommandArgs[arg1].unk4 + (value + (g_BattleCommandArgs[arg1].unk5 << 8))), 0);
}
void BattleQueueCommandF4(u8 arg0, u8 arg1, u8 arg2) {
    BattleQueueCommandF8IfFlagged(arg1, arg0);
    D_800C402F[g_BattleCommandQueue->count * 72] = 0xF4;
    func_800785D4(arg0, arg1);
}
void BattleQueueCommandF6(u8 arg0, u8 arg1, u8 arg2) {
    D_800C402F[g_BattleCommandQueue->count * 72] = 0xF6;
    g_BattleCommandRecords[g_BattleCommandQueue->count][0] = g_BattleCommandArgs[arg1].slotMask;
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
    if (*g_DebuggerState != -1) {
        for (;;) {
            for (k = 0; k < i; k++) {
                FontPrintf(g_StrSpace);
            }
            FontPrintf(g_StrLanguageError);
            j++;
            FontPrintf(g_StrActorNoFormat, arg0, arg1);
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
void BattleDispatchCommandRecords(u8 arg0) {
    u8 again;
    u8 idx;
    u8 slot;
    s32 i;
    s32 j;

    i = 0;
    again = 1;
    do {
        j = 0;
        slot = 0;
        for (; j < 0xB; j++) {
            idx = j;
            if (BattleMaskHasSlotBit(g_BattleCommandArgs[i].slotMask, idx) != 0) {
                slot = idx;
                break;
            }
        }
        BattleClearSlotCurveEntry();
        switch (g_BattleCommandArgs[i].opcode) {
        case 0x0:
            again = 0;
            break;
        case 0x1:
            func_80078998(arg0, i, slot);
            break;
        case 0x2:
            BattleQueueCommandFD(arg0, i, slot);
            break;
        case 0x3:
            BattleQueueCommandFC(arg0, i, slot);
            break;
        case 0x4:
            BattleQueueCommandFromRecordByte(arg0, i, slot);
            break;
        case 0x5:
            BattleSetPendingActorMask(arg0, i, slot);
            break;
        case 0x6:
            BattleQueueCommandF9(arg0, i, slot);
            break;
        case 0x7:
            func_80078E24(arg0, i, slot);
            break;
        case 0x8:
            BattleApplyCommandByteVar(arg0, i, slot);
            break;
        case 0x9:
            BattleAddCommandByteVar(arg0, i, slot);
            break;
        case 0xA:
            BattleApplyCommandWordVar(arg0, i, slot);
            break;
        case 0xB:
            BattleAddCommandWordVar(arg0, i, slot);
            break;
        case 0xC:
            func_80078658(i, arg0);
            break;
        case 0xD:
            BattleQueueCommandF4(arg0, i, slot);
            break;
        case 0xE:
            BattleQueueCommandF7(g_BattleCommandArgs[i].unk4, arg0);
            break;
        case 0xF:
            BattleQueueCommandF6(arg0, i, slot);
            break;
        case 0x10:
            BattleQueueCommandF8(arg0);
            break;
        default:
            BattlePrintLanguageErrorLoop(arg0, i);
            break;
        }
        i++;
    } while (i < 0x20 || again);
}
void BattleQueueCommand1BAndFE(u8 arg0) {
    if (g_BattleSlotFlags[arg0 * 8] != 0) {
        D_800C400B[g_BattleCommandQueue->count * 72] = arg0;
        D_800C402F[g_BattleCommandQueue->count * 72] = 0x1B;
        g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
    }
    D_800C400B[g_BattleCommandQueue->count * 72] = arg0;
    D_800C402F[g_BattleCommandQueue->count * 72] = 0xFE;
    g_BattleSoundEnabled = 0;
}
void func_80079778(u8 arg0) {
    g_BattleCommandQueue->count = 0;
    BattleClearSlotCurveState();
    BattleClearSlotCurveEntry();
    D_800C400B[g_BattleCommandQueue->count * 72] = arg0;
    BattleUpdateFrame();
    func_800B89FC(1, arg0, 0, BattleFindMinValueOtherSlot(arg0));
    BattleDispatchCommandRecords(arg0);
    BattleQueueCommand1BAndFE(arg0);
    while (g_BattleCommandQueue->unk2DB == 0) {
        BattleUpdateFrame();
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079840);
/* Best: 96.48%. Two emit-order/allocator tie-breaks left: the loop preheader
   puts `move a1,v1` after `la a2` where the original has it first, and the
   g_BattleCommandQueue load adds index-first (`addu v0,v0,v1`) where the original adds
   base-first (`addu v1,v1,v0`). Tried: g_BattleCommandQueue[slot * 0x40 + 0x3C] inline,
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
        pBuf = g_BattleCommandQueue + (arg0 & 0xFF) * 0x40;
        if (g_BattleSlotFlag8000Mask & BattleSlotBitMask(pBuf[0x3C])) {
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
    pCursor = g_BattleSlotRegs[slot].pScriptCursor;
    p = (u8 *)g_BattleCommandArgs;
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
            value = BattleExecScriptOpcode(&pCursor, slot, value);
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
    if ((g_BattleSlotEntries[slot + 3].flags & 0x8000) != 0) {
        if ((D_800CCD1C[slot + 3].flags & 0x800) == 0) {
            goto done;
        }
    }
    g_BattleCommandArgs[0].opcode = 0;
    if (g_BattleSlotScriptFlags[slot * 4] != 0) {
        pCursor = g_BattleSlotRegs[slot].pScanCursor;
        p = g_BattleCommandArgs;
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
                value = BattleExecScriptOpcode(&pCursor, slot, value);
            }
        }
    }
    if (g_BattleCommandArgs[0].opcode != 0) {
        BattleDispatchCommandRecords(slot + 3);
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

    saved = g_BattleCommandQueue->unk2D3;
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
        g_BattleCommandQueue->unk2D3 = idx;
        pCursor = *(u8**)(D_800D340C + off2);
        flags = *(u16*)((u8*)g_BattleSlotEntries + off);
        p = g_BattleCommandArgs;
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
                value = BattleExecScriptOpcode(&pCursor, i, value);
            }
        }
        g_BattleCommandQueue->unk2DB = 0;
        func_80079778(idx);
        BattleSetOverlayArg(0);
    }
    g_BattleCommandQueue->unk2D3 = saved;
}
