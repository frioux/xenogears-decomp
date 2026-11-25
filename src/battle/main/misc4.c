#include "common.h"
#include "battle/actor.h"

// The command byte of a queue record that holds no command: the clear loops below fill the
// whole record table with it (the same name battle/main/misc13 uses for the byte).
#define BATTLE_COMMAND_NONE 0xFF
// The command argument records BattleDispatchCommandRecords walks: the 0x100-byte
// g_BattleCommandArgs buffer holds exactly this many 8-byte BattleCommandArg entries.
#define BATTLE_COMMAND_ARG_COUNT 32
// The non-party actor slots: slot numbers MAX_PARTY_MEMBERS through BATTLE_ACTOR_SLOT_COUNT - 1,
// which are the ones the slot register file is indexed from 0 for.
#define BATTLE_OTHER_SLOT_COUNT (BATTLE_ACTOR_SLOT_COUNT - MAX_PARTY_MEMBERS)
// The rows of the 0x48-byte command record table, which the clear loops below walk backwards by
// byte offset from the last row's command byte: (count - 1) * 0x48 is the 0x8B8 they start at.
#define BATTLE_COMMAND_RECORD_COUNT 32

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

// The head of the block g_BattleCommandQueue points at: one descriptor per battle actor
// slot, indexed by the actor slot the command is being built for.
typedef struct {
    /* 0x00 */ u8 unk00[0x3C];
    /* 0x3C */ u8 targetSlot; // the slot this descriptor's queued command targets
    /* 0x3D */ u8 unk3D[0x3];
} BattleCommandSlot; // Size: 0x40

// The command queue header g_BattleCommandQueue points at. Only the tail this unit fills
// is named.
typedef struct {
    /* 0x000 */ BattleCommandSlot slots[BATTLE_ACTOR_SLOT_COUNT];
    /* 0x2C0 */ u8 unk2C0[0x13];
    /* 0x2D3 */ u8 actorIndex; // the slot whose command is being built, as in misc11's view of this block
    /* 0x2D4 */ u8 unk2D4[6];
    /* 0x2DA */ u8 count; // queued commands
    /* 0x2DB */ u8 commandFinished; // 0 while a queued command runs: cleared before func_80079778, which spins on it
    /* 0x2DC */ u8 unk2DC;
} BattleCommandQueue; // Size: 0x2DD

extern u8 D_800D2CA4[];
extern BattleCommandArg g_BattleCommandArgs[];
extern u16 g_BattlePendingSlotMask;
extern BattleSlotRegs g_BattleSlotRegs[];
extern s32 BattleExecScriptOpcode(u8** arg0, u8 arg1, u8 arg2);
extern s32 func_8007F8C0(u8** arg0, u8 arg1);

// One 4-byte group of the per-slot script flags at g_BattleSlotScriptFlags, a group per
// non-party slot at a (slot - MAX_PARTY_MEMBERS) * 4 stride. Battle code also names three of
// the four bytes by absolute address: +0x0 is g_BattleSlotScriptFlags, +0x1 D_800C3D19,
// +0x2 D_800C3D1A and +0x3 D_800C3D1B.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3; // set to 1 for the slot BattleQueueCommandF9 queues its command for
} BattleSlotScriptFlags; // Size: 0x4

extern u16 g_BattleSlotFlag8000Mask;
extern s32 BattleSlotBitMask(u8 arg0);
extern u8 g_BattleActorCountdownState[];
extern BattleSlotScriptFlags g_BattleSlotScriptFlags[];
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
// One 0x48-byte row of the command record table at g_BattleCommandRecords, as the queueing
// commands below fill it: the slot mask the command applies to, the actor slot it was queued
// for, the word the command carries and the command byte itself. (src/battle/main/misc3.c
// carries the same view of this table.)
typedef struct {
    /* 0x00 */ u16 slotMask;
    /* 0x02 */ u8 unk2[0xB];
    /* 0x0D */ u8 actorSlot;
    /* 0x0E */ u8 unkE[0x16];
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u8 unk26[0xB];
    /* 0x31 */ u8 command;
    /* 0x32 */ u8 unk32[0x16];
} BattleCommandRecord; // Size: 0x48

// The clear loops below walk the table backwards by BYTE offset, which is what retail keeps in
// its induction register (ori $v0,zero,0x8B8 / addiu $v0,$v0,-0x48), so they reach the command
// byte through the field's own symbol: g_BattleCommandSlotIds is row +0x31. Rewriting such a loop as an
// index walk over the rows (g_BattleCommandRecords[i].command) costs 8 bytes - 92.94%, 256
// against retail's 248 - because gcc then synthesises the index times 0x48 in the preheader.
extern u8 g_BattleTextSlotCount;
extern BattleCommandRecord g_BattleCommandRecords[];
extern BattleCommandQueue *g_BattleCommandQueue;
extern u8 g_BattleCommandSlotIds[];
extern void BattleQueueCommandSlotAndTile(u8 arg0, u8 arg1);
extern void BattleQueueCommandFA(u8 arg0, u8 arg1);
extern void BattleQueueCommandF7(u8 arg0, u8 arg1);
extern void BattleQueueCommandF8(u8 arg0);
extern s32 BattleSlotWordVarGetSet(u8 arg0, u8 arg1, s32 arg2, s32 arg3);
extern void BattleUpdateFrame(void);
extern void BattleDispatchCommandRecords(u8 arg0);
extern void BattleQueueCommand1BAndFE(u8 arg0);
extern void BattleClearSlotCurveState(void);
extern void BattleClearSlotCurveEntry(void);
extern s32 BattleFindMinValueOtherSlot(u8 arg0);
extern void BattleInitActorSlotAnimation(s32 arg0, u8 arg1, s32 arg2, s32 arg3);
extern s32 BattleSlotByteVarGetSet(u8 arg0, u8 arg1, u8 arg2, s32 arg3);
extern u8 D_800D340C[];
extern BattleTableEntry D_800CCD1C[];
extern void BattleSetOverlayArg(u8 arg0);

extern u8 g_BattleCommandSlotMasks[];
extern u8 g_BattleCommandCodes[];
extern u8 D_800C3E3D[];
extern u8 D_800D3404[];
extern u8 D_800D3408[];
extern BattleTableEntry g_BattleWorkBlock[];
extern u16 BattleMaskHasSlotBit(u16 arg0, u8 arg1);
extern void func_80078998(u8 arg0, u8 arg1, u8 arg2);
extern void BattleQueueCommandFB(u8 arg0, u8 arg1, u8 arg2);

void BattleQueueCommandF8(u8 arg0) {
    if (g_BattleTextSlotCount != 0) {
        g_BattleCommandRecords[g_BattleCommandQueue->count].actorSlot = arg0;
        g_BattleCommandRecords[g_BattleCommandQueue->count].command = 0xF8;
        g_BattleCommandRecords[g_BattleCommandQueue->count].unk24 = g_BattleTextSlotCount - 1;
        g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
    }
}
void BattleQueueCommandF8IfFlagged(u8 arg0, u8 arg1) {
    if (g_BattleCommandArgs[arg0].unk3 != 0) {
        BattleQueueCommandFA(arg0, arg1);
        BattleQueueCommandF7(0x1E, arg1);
        BattleQueueCommandF8(arg1);
    }
}
// 99.42%, 412 bytes and 103 instructions: retail's byte count and instruction count exactly. The whole
// body is retail's; the remaining diff is reloc spelling only (this C names g_BattleCommandArgs + 1 and
// g_BattleCommandRecords + 0x31 where the original object names D_800D2E5D and g_BattleCommandSlotIds, the
// same addresses), so objdiff cannot read 100.00 for it - judge it with check_bin.sh.
// What the loop needs, measured: ONE strength-reduced byte offset (a0) shared by both tables, and a TYPED
// store, which is what lets gcc hoist the loop-invariant g_BattleCommandQueue load into the latch (the
// move a2,v1 copy retail has). A raw *(u16 *)((u8 *)g_BattleCommandRecords + i) store blocks that hoist
// and reloads the pointer every iteration (93.20 / 416), and g_BattleCommandSlotIds[n * 0x48] pairing with
// a record-index walk makes gcc hoist the table BASE into s0 instead (95.38 / 412, frame 0x28 -> 0x30).
// A pQueue local collapses the copy away (98.16 / 408, one instruction short, cannot link).
void func_80078998(u8 arg0, u8 arg1, u8 arg2) {
    s32 n;

    BattleQueueCommandF8IfFlagged(arg1, arg0);
    g_BattleCommandQueue->unk2DC = g_BattleCommandArgs[arg1].unk1 + 1;
    BattleBeginActorAction(arg0, g_BattleCommandArgs[arg1].slotMask, g_BattleCommandArgs[arg1].unk2);
    BattleApplySlotCurveAndStep(g_BattleCommandQueue->count);
    g_BattleCommandSlotIds[g_BattleCommandQueue->count * 72] = g_BattleCommandArgs[arg1].unk2;
    g_BattleCommandRecords[g_BattleCommandQueue->count].slotMask = g_BattleActionSlotMask[0];
    BattleQueueCommandSlotAndTile(arg0, arg1);
    if (g_BattleCommandArgs[arg1].slotMask != g_BattleActionSlotMask[0]) {
        for (n = 0; n < g_BattleCommandQueue->count; n++) {
            if (g_BattleCommandRecords[n].command == 0xFD) {
                g_BattleCommandRecords[n].slotMask = g_BattleActionSlotMask[0];
                break;
            }
        }
    }
}


void BattleQueueCommandFD(u8 arg0, u8 arg1, u8 arg2) {
    g_BattleCommandRecords[g_BattleCommandQueue->count].command = 0xFD;
    g_BattleCommandRecords[g_BattleCommandQueue->count].unk24 = 0;
    g_BattleCommandRecords[g_BattleCommandQueue->count].slotMask = g_BattleCommandArgs[arg1].slotMask;
    func_800877E0(arg0, arg2);
    if (g_BattleCommandArgs[arg1].unk4 != 1) {
        if (g_BattleSlotFlags[arg0 * 8] == 0) {
            func_80087EDC(arg0, arg2);
        } else {
            func_800881B8(arg0, arg2);
        }
    }
    BattleBeginAnimTask(BattleSlotBitMask(arg0) | g_BattleCommandArgs[arg1].slotMask);
    BattleQueueCommandSlotAndTile(arg0, arg1);
}
void BattleQueueCommandFC(u8 arg0, u8 arg1, u8 arg2) {
    g_BattleCommandRecords[g_BattleCommandQueue->count].command = 0xFC;
    BattleQueueCommandSlotAndTile(arg0, arg1);
}
void BattleQueueCommandFromRecordByte(u8 arg0, u8 arg1, u8 arg2) {
    g_BattleCommandRecords[g_BattleCommandQueue->count].command = g_BattleCommandArgs[arg1].unk4;
    BattleQueueCommandSlotAndTile(arg0, arg1);
}

void BattleSetPendingActorMask(u8 arg0, u8 arg1, u8 arg2) {
    g_BattlePendingSlotMask = g_BattleCommandArgs[arg1].slotMask;
}

void BattleQueueCommandF9(u8 arg0, u8 arg1, u8 arg2) {
    g_BattleCommandRecords[g_BattleCommandQueue->count].command = 0xF9;
    BattleQueueCommandSlotAndTile(arg0, arg1);
    g_BattleActorCountdownState[arg0] = 0xFF;
    // The empty-block wrapper keeps this store ahead of the g_BattleSlotEntries read-modify-write.
    do {
        g_BattleSlotScriptFlags[arg0 - MAX_PARTY_MEMBERS].unk3 = 1;
    } while (0);
    g_BattleSlotEntries[arg0].flags &= 0x8000;
}

void BattleQueueCommandFB(u8 arg0, u8 arg1, u8 arg2) {
    u8 slot;
    u8 idx;
    s32 i;
    s32 dst;
    s32 src;

    for (i = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        idx = i;
        if (BattleMaskHasSlotBit(g_BattleCommandArgs[arg1].slotMask, idx) != 0) {
            slot = idx;
            break;
        }
    }
    dst = (arg0 - MAX_PARTY_MEMBERS) * sizeof(BattleSlotRegs);
    src = (slot - MAX_PARTY_MEMBERS) * sizeof(BattleSlotRegs);
    D_800C3E3D[arg0] = D_800C3E3D[slot];
    *(u32 *)((u8 *)g_BattleSlotRegs + dst) = *(u32 *)((u8 *)g_BattleSlotRegs + src);
    *(u32 *)((u8 *)g_BattleSlotRegs + dst + 0x4) = *(u32 *)((u8 *)g_BattleSlotRegs + src + 0x4);
    *(u32 *)((u8 *)g_BattleSlotRegs + dst + 0x8) = *(u32 *)((u8 *)g_BattleSlotRegs + src + 0x8);
    *(u32 *)((u8 *)g_BattleSlotRegs + dst + 0xC) = *(u32 *)((u8 *)g_BattleSlotRegs + src + 0xC);
    g_BattleSlotScriptFlags[arg0 - MAX_PARTY_MEMBERS].unk0 = g_BattleSlotScriptFlags[slot - MAX_PARTY_MEMBERS].unk0;
    g_BattleSlotScriptFlags[arg0 - MAX_PARTY_MEMBERS].unk1 = g_BattleSlotScriptFlags[slot - MAX_PARTY_MEMBERS].unk1;
    memmove(&g_BattleWorkBlock[arg0], &g_BattleWorkBlock[slot], sizeof(BattleTableEntry));
    g_BattleCommandRecords[g_BattleCommandQueue->count].unk24 = slot;
    g_BattleCommandRecords[g_BattleCommandQueue->count].command = 0xFB;
    g_BattleCommandRecords[g_BattleCommandQueue->count].actorSlot = arg0;
    g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
}
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
    g_BattleCommandRecords[g_BattleCommandQueue->count].command = 0xF4;
    BattleQueueCommandSlotAndTile(arg0, arg1);
}
void BattleQueueCommandF6(u8 arg0, u8 arg1, u8 arg2) {
    g_BattleCommandRecords[g_BattleCommandQueue->count].command = 0xF6;
    g_BattleCommandRecords[g_BattleCommandQueue->count].slotMask = g_BattleCommandArgs[arg1].slotMask;
    BattleQueueCommandSlotAndTile(arg0, arg1);
}
void BattlePrintLanguageErrorLoop(u8 arg0, u8 arg1) {
    s32 i;
    s32 j;
    s32 k;
    s32 off;
    u8 value;

    value = BATTLE_COMMAND_NONE;
    for (off = (BATTLE_COMMAND_RECORD_COUNT - 1) * sizeof(BattleCommandRecord); off >= 0; off -= sizeof(BattleCommandRecord)) {
        g_BattleCommandSlotIds[off] = value;
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
        for (; j < BATTLE_ACTOR_SLOT_COUNT; j++) {
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
            BattleQueueCommandFB(arg0, i, slot);
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
            BattleQueueCommandFA(i, arg0);
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
    } while (i < BATTLE_COMMAND_ARG_COUNT || again);
}
void BattleQueueCommand1BAndFE(u8 arg0) {
    if (g_BattleSlotFlags[arg0 * 8] != 0) {
        g_BattleCommandRecords[g_BattleCommandQueue->count].actorSlot = arg0;
        g_BattleCommandRecords[g_BattleCommandQueue->count].command = 0x1B;
        g_BattleCommandQueue->count = g_BattleCommandQueue->count + 1;
    }
    g_BattleCommandRecords[g_BattleCommandQueue->count].actorSlot = arg0;
    g_BattleCommandRecords[g_BattleCommandQueue->count].command = 0xFE;
    g_BattleSoundEnabled = 0;
}
void func_80079778(u8 arg0) {
    g_BattleCommandQueue->count = 0;
    BattleClearSlotCurveState();
    BattleClearSlotCurveEntry();
    g_BattleCommandRecords[g_BattleCommandQueue->count].actorSlot = arg0;
    BattleUpdateFrame();
    BattleInitActorSlotAnimation(1, arg0, 0, BattleFindMinValueOtherSlot(arg0));
    BattleDispatchCommandRecords(arg0);
    BattleQueueCommand1BAndFE(arg0);
    while (g_BattleCommandQueue->commandFinished == 0) {
        BattleUpdateFrame();
    }
}
void BattleSetSlotRegistersFromActor(u8 arg0, u8 arg1) {
    u8 slot;
    s32 i;
    BattleCommandSlot *pSlot;

    if (arg1 >= 3) {
        slot = arg1 - 3;
        g_BattleSlotRegs[slot].wordRegs[7] = BattleSlotBitMask(arg0);
        for (i = 0; i < 5; i++) {
            g_BattleSlotRegs[slot].byteRegs[9 + i] = D_800D2CA4[i];
        }
        pSlot = &g_BattleCommandQueue->slots[arg0];
        if (g_BattleSlotFlag8000Mask & BattleSlotBitMask(pSlot->targetSlot)) {
            g_BattleSlotRegs[slot].byteRegs[0xE] = 1;
        } else {
            g_BattleSlotRegs[slot].byteRegs[0xE] = 0;
        }
    }
}
void BattleScriptCursorSkipWord(u8** pCursor) {
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
    slot = arg0 - MAX_PARTY_MEMBERS;
    pCursor = g_BattleSlotRegs[slot].pScriptCursor;
    p = (u8 *)g_BattleCommandArgs;
    pEnd = p + BATTLE_COMMAND_ARG_COUNT * sizeof(BattleCommandArg);
    do {
        *p++ = 0;
    } while (p < pEnd);
    for (i = (BATTLE_COMMAND_RECORD_COUNT - 1) * sizeof(BattleCommandRecord); i >= 0; i -= sizeof(BattleCommandRecord)) {
        g_BattleCommandSlotIds[i] = BATTLE_COMMAND_NONE;
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
    slot = arg0 - MAX_PARTY_MEMBERS;
    if ((g_BattleSlotEntries[slot + MAX_PARTY_MEMBERS].flags & 0x8000) != 0) {
        if ((D_800CCD1C[slot + MAX_PARTY_MEMBERS].flags & 0x800) == 0) {
            goto done;
        }
    }
    g_BattleCommandArgs[0].opcode = 0;
    if (g_BattleSlotScriptFlags[slot].unk0 != 0) {
        pCursor = g_BattleSlotRegs[slot].pScanCursor;
        p = g_BattleCommandArgs;
        pEnd = p + BATTLE_COMMAND_ARG_COUNT * sizeof(BattleCommandArg);
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
        BattleDispatchCommandRecords(slot + MAX_PARTY_MEMBERS);
    }
done:
    return result;
}
// Walks the command script of every non-party slot that has one, running each opcode through
// BattleExecScriptOpcode. The cursor and the two script-flag bytes come from typed tables; the two
// 0x170-byte flag words below keep a byte offset, because reading them as
// g_BattleSlotEntries[idx].flags / D_800CCD1C[idx].flags drops the loop's 0x170-byte induction
// variable and rearranges the saved registers (88.08%, 496 bytes, retail 500). Measured against the
// typed body, only the folded relocations differ: %hi(g_BattleSlotRegs) + 12 for D_800D340C and
// %hi(g_BattleSlotScriptFlags) + 1/+2 for D_800C3D19/D_800C3D1A (99.76%, same 500 bytes, same link).
// The g_BattleCommandSlotIds fill below is the command byte of every 0x48-byte record, walked as a byte offset
// into the record table because the typed spelling loses the walk's induction variable.
void BattleRunActiveSlotScripts(void) {
    s32 i;
    s32 off;
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

    saved = g_BattleCommandQueue->actorIndex;
    value = 0;
    i = 0;
    stopOp = 0xFD;
    endOp = 0xFF;
    for (idx = MAX_PARTY_MEMBERS, off = MAX_PARTY_MEMBERS * sizeof(BattleTableEntry);
         i < BATTLE_OTHER_SLOT_COUNT;
         idx++, off += sizeof(BattleTableEntry), i++) {
        if (g_BattleSlotScriptFlags[i].unk2 == 0) {
            continue;
        }
        if (g_BattleSlotScriptFlags[i].unk1 == 0) {
            continue;
        }
        g_BattleCommandQueue->actorIndex = idx;
        pCursor = g_BattleSlotRegs[i].pRunCursor;
        flags = *(u16*)((u8*)g_BattleSlotEntries + off);
        p = g_BattleCommandArgs;
        if ((flags & 0x8000) != 0) {
            if ((*(u16*)((u8*)D_800CCD1C + off) & 0x800) == 0) {
                continue;
            }
        }
        for (j = (BATTLE_COMMAND_RECORD_COUNT - 1) * sizeof(BattleCommandRecord); j >= 0; j -= sizeof(BattleCommandRecord)) {
            g_BattleCommandSlotIds[j] = endOp;
        }
        pEnd = p + BATTLE_COMMAND_ARG_COUNT * sizeof(BattleCommandArg);
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
        g_BattleCommandQueue->commandFinished = 0;
        func_80079778(idx);
        BattleSetOverlayArg(0);
    }
    g_BattleCommandQueue->actorIndex = saved;
}
