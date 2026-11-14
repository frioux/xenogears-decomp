#include "common.h"
#include "main/game.h"

// A 0x170-byte battle table entry; only the flag word at +0 is read here.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 unk02[0x16E];
} BattleTableEntry; // Size: 0x170

extern BattleTableEntry D_800CCD64[];
extern u8 D_800C3EB4[];
extern u8 D_800C3EB7[];
extern u8 D_800D2C8B[];
extern u8 D_800D2CB0[];
extern u8 D_800D2CE0[];
extern u8 D_800D2DCC[];
extern u8 D_800D2E0C[];
extern u8 D_800D32A1[];
extern u8 D_800CCDEC[];
extern u8 D_800D3410[];
extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u8 func_8007A628(u8 arg0, u8 arg1);
extern u16 func_80089C08(u8 arg0);
extern u8 func_80079E7C(u16 arg0);
extern u8 func_8007A744(u8 arg0);
extern u8 D_800CCD3E[];
extern u8 D_800D2E06[];
extern u8 *D_800D3364;
extern u8 D_800CCD34[];
extern u8 func_8007A6C8(u8 arg0, u8 arg1);
extern u8 func_8001BD40(s32 arg0, u8 arg1);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C1A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C33C);
/* best: 89.69% (364 bytes vs retail 356). The whole structure matches: the
   prologue builds (arg1 + 3) * 0x1C into s5, the "record" offset 0x54, the
   three-condition body, `buf[count++] = idx`, and the tail store with a local
   index + row pointer. Two remaining differences: (1) gcc strength-reduces
   D_800D32A1[i * 8] into an induction pointer (&D_800D32A1 + 0x18 stored in a
   saved register) where retail keeps `sll a0,s0,3` and the at-form load, which
   also costs the extra saved register; (2) with that block shared by three
   predecessors retail keeps a nop in the `beqz` delay slot and increments i in
   the fall-through block, gcc steals the addiu into the slot.

void func_8007C33C(u8 **arg0, u8 arg1) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 slot = (arg1 + 3) * 0x1C;
    s32 record = 0x54;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);

    pField[(*arg0)[1]] = 0;
    for (; i < 0xB; i++, record += 0x1C) {
        u8 idx = i;

        if (func_8007A628(idx, (*arg0)[2]) != 0 && D_800C3EB4[slot] != D_800C3EB4[record]
            && D_800D32A1[i * 8] == 0) {
            buf[count++] = idx;
        }
    }
    if (count != 0) {
        u16 result = func_80089C08(buf[func_8001BD40(0, count - 1)]);
        s32 index = (*arg0)[1];
        u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

        pRow[index] = result;
    }
}
*/
void func_8007C4A0(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u8 best = 0xFF;
    s32 bestIndex = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && best >= *(s16 *)&D_800D2E06[i * 2]) {
            best = D_800D2E06[i * 2];
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
void func_8007C580(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u8 best = 0xFF;
    s32 bestIndex = 0;
    s32 i = 3;
    s32 slot = arg1 + 3;

    for (; i < 0xB; i++) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && best >= *(s16 *)&D_800D2E0C[(i - 3) * 2] && i != slot) {
            best = D_800D2E0C[(i - 3) * 2];
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
void func_8007C678(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u16 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;
    s32 offset;

    for (i = 0, offset = 0; i < 3; i++, offset += 0x170) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && best >= *(u16 *)(D_800CCD34 + offset)) {
            best = *(u16 *)(D_800CCD34 + offset);
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
void func_8007C75C(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u16 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;
    s32 offset;

    for (i = 3, offset = 0x450; i < 0xB; i++, offset += 0x170) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && best >= *(u16 *)(D_800CCD34 + offset)) {
            best = *(u16 *)(D_800CCD34 + offset);
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C840);
/* best: 82.94% (408 bytes vs retail 396). Whole structure is right (pre-test
   rotation, the (arg1+3)*0x1C preheader value, s5 = arg1*0x40 reused by the
   tail store, the sh/andi/sb tail). Two blockers: (1) gcc hoists the invariant
   ADDRESS D_800C3EB4 + (arg1+3)*0x1C into s4 and keeps &D_800C3EB4 in an extra
   saved register s6 (frame 0x40 vs 0x38), where retail keeps the OFFSET in s4
   and re-forms `lui at/%hi + addu at,at,s4 + lbu %lo(at)` per use; (2) the tail
   store's base/index land in the opposite registers (v1/a0 vs a0/v1).
   Tried: the offset inline in the loop (this, 82.94%) and as a pre-loop s32
   `record` local (78.44%: the multiply stops being strength-reduced).

void func_8007C840(u8 **arg0, u8 arg1) {
    u8 buf[3];
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    buf[0] = buf[1] = buf[2] = 0;
    pRow[(*arg0)[1]] = 0;
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = func_8001BD40(0, 2);
        u8 idx = slot;

        if (buf[idx] == 0) {
            if (func_8007A628(idx, (*arg0)[2]) != 0 && D_800D32A1[idx * 8] != 0
                && D_800C3EB4[(arg1 + 3) * 0x1C] == D_800C3EB4[idx * 0x1C]) {
                u16 result = func_80089C08(idx);
                s32 index = (*arg0)[1];
                u16 *pTail = (u16 *)(D_800D3420 + arg1 * 0x40);

                pTail[index] = result;
                break;
            }
            buf[idx] = 1;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C9D4);
/* best: 91.53% (340 bytes vs retail 336). Everything matches except two
   allocation details: (1) gcc computes the row base straight into s4
   (`addu s4,a1,v1`) where retail forms it in a caller-saved temp and copies it
   into s4 at the loop preheader (`addu a1,a1,v1` / `move s4,a1`); (2) gcc hoists
   `&D_800D32A1` into s5 (frame 0x38 vs 0x30) where retail re-forms it per use
   as lui at/%hi + addu at,at,v0 + lbu %lo(at).
   Tried: inline ((u16 *)(D_800D3420 + arg1 * 0x40))[(*arg0)[1]] for both stores
   (92.07%, 344 bytes: two copies of the base), for the zeroing store only
   (92.07%/344), and the plain u8 *pRow + pRow[i] form (this one).

void func_8007C9D4(u8 **arg0, u8 arg1) {
    u8 buf[3];
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    buf[0] = buf[1] = buf[2] = 0;
    pRow[(*arg0)[1]] = 0;
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = func_8001BD40(0, 2);
        u8 idx = slot;

        if (buf[idx] == 0) {
            if (func_8007A628(idx, (*arg0)[2]) != 0 && D_800D32A1[idx * 8] != 0) {
                pRow[(*arg0)[1]] = func_80089C08(idx);
                break;
            }
            buf[idx] = 1;
        }
    }
}
*/
void BattlePickSlotByFlagTable(u8 **arg0, u8 arg1) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 record = 0x18;
    u8 *pBuf = buf;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);

    pField[(*arg0)[1]] = 0;
    for (; i < 0xB; i++, record += 8) {
        u8 idx = i;

        if (func_8007A628(idx, (*arg0)[2]) != 0 && D_800D32A1[record] != 0) {
            *pBuf++ = idx;
            count++;
        }
    }
    if (count != 0) {
        u16 result = func_80089C08(buf[func_8001BD40(0, count - 1)]);
        s32 index = (*arg0)[1];
        u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

        pRow[index] = result;
    }
}
void func_8007CC50(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i;
    s32 offset;

    for (i = 0, offset = 0; i < 3; offset += 0x1C, i++) {
        if (func_8007A628(i, 0) != 0 && D_800C3EB4[offset] == (*arg0)[2]) {
            count++;
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
void func_8007CD10(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i = 3;
    s32 offset = 0x54;

    for (; i < 0xB; offset += 0x1C, i++) {
        if (func_8007A628(i, 0) != 0 && D_800C3EB4[offset] == (*arg0)[2]) {
            count++;
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CDD0);
/* best: 78.26%. Every instruction of the loop/tail is present; only the
   callee-saved set and two intra-block orders differ: the original keeps the
   D_800D3420 base in s0 across the func_80079E7C call (4 saved regs), gcc
   folds the whole row base into s1 before the call (3 saved regs), and it
   emits the two D_800C3EB4 loads in the opposite order. Tried: pRow/pField as
   separate locals, slot = arg1 + 3 assigned after the call, and the two table
   bytes split into operator/row locals - all three give the same code.

void func_8007CDD0(u8 **arg0, u8 arg1) {
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);
    u8 *pRow = D_800D3420 + 0x10 + arg1 * 0x40;
    u8 value = func_80079E7C(pField[(*arg0)[2]]);
    s32 slot = arg1 + 3;
    u8 operator = D_800C3EB4[value * 0x1C];
    u8 row = D_800C3EB4[slot * 0x1C];

    pRow[(*arg0)[1]] = D_800D3364[operator * 8 + row * 0x40 + 0x140];
}
*/
void func_8007CEA4(u8 **arg0, u8 arg1) {
    s32 count = 0;
    s32 i = 0;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);
    s32 offset = 0;
    u8 *pRow;

    for (; i < 3; offset += 0x1C, i++) {
        if (func_8007A628(i, 0) != 0) {
            s32 other = func_80079E7C(pField[(*arg0)[2]]) * 0x1C;
            if (D_800C3EB4[offset] == D_800C3EB4[other]) {
                count++;
            }
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
void func_8007CFB8(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i = 3;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);
    s32 offset = 0x54;

    for (; i < 0xB; offset += 0x1C, i++) {
        if (func_8007A628(i, 0) != 0) {
            s32 other = func_80079E7C(pField[(*arg0)[2]]) * 0x1C;
            if (D_800C3EB4[offset] == D_800C3EB4[other]) {
                count++;
            }
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D0CC);
/* best: 85.97% (identical instruction multiset, 4 insns of ordering). Retail
   forms the D_800D3430 base in a1, then orders
   [sll][lui/addiu][lw *arg0][addu base][lbu idx][move i=0][addu][sb];
   gcc emits [move i=0][sll][lw *arg0][lui/addiu][lbu][addu base][addu][sb] -
   i.e. the scheduler hoists `i = 0` to the very top and interleaves the two
   address chains the other way round. Tried: i = 0 between store and loop,
   the inline *(D_800D3430 + arg1*0x40 + (*arg0)[1]) spelling (folds into $at,
   frame 8), index local. This is the scheduler-priority tie class.

void func_8007D0CC(u8 **arg0, u8 arg1) {
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 i;

    pRow[(*arg0)[1]] = 0;
    for (i = 0; i < 0x30; i++) {
        if (D_800D2CE0[i] == (*arg0)[2]) {
            pRow[(*arg0)[1]] = D_800D2CB0[i];
            break;
        }
    }
}
*/
void BattleWriteRowFieldBytes(u8 **arg0, u8 *arg1, u8 arg2, u8 arg3) {
    u16 *pField = (u16 *)(D_800D3420 + arg2 * 0x40);
    u16 value = pField[(*arg0)[2]];

    arg1[arg3 * 8 + (*arg0)[1]] = value;
    *(arg1 + (arg3 * 8 + (*arg0)[1]) + 1) = value >> 8;
}
void BattleStoreGoldWord32(u8 **arg0, u8 arg1) {
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);
    u8 *p = *arg0;

    pRow[p[1]] = g_GameState.gold;
}
void func_8007D1DC(u8 **arg0, u8 arg1) {
    u8 buf[0x10];
    s32 i = 0;
    s32 count = 0;
    s32 record = 0;
    u8 *pBuf = buf;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);

    pField[(*arg0)[1]] = 0;
    for (; i < 0xB; i++, record += 0x170) {
        u8 idx = i;

        if (func_8007A744(idx) != 0 && D_800CCD3E[record] == (*arg0)[2]) {
            *pBuf++ = idx;
            count++;
        }
    }
    if (count != 0) {
        u16 result = func_80089C08(buf[func_8001BD40(0, count - 1)]);
        s32 index = (*arg0)[1];
        u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

        pRow[index] = result;
    }
}
void BattleStoreStatWord32(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);

    pRow[p[1]] = D_800D2C8B[arg1];
}
void func_8007D344(u8 **arg0, u8 arg1) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 record = 0x54;
    u8 *pBuf = buf;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);

    pField[(*arg0)[1]] = 0;
    for (; i < 0xB; i++, record += 0x1C) {
        u8 idx = i;

        if (func_8007A6C8(idx, (*arg0)[2]) != 0 && (D_800C3EB7[record] & 0x80) != 0) {
            *pBuf++ = idx;
            count++;
        }
    }
    if (count != 0) {
        u16 result = func_80089C08(buf[func_8001BD40(0, count - 1)]);

        s32 index = (*arg0)[1];
        u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

        pRow[index] = result;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D478);
/* best: 98.05%. Every instruction matches except the saved-register numbering
   and one preheader copy: retail defines the row base in a caller-saved temp
   (addu a1,a1,v1), uses it for the zeroing store, and copies it to s2 at the
   end of the block (move s2,a1) - so retail allocates s1 to arg0 and s2 to the
   base, and the function is one instruction longer. gcc 2.6 homes the base
   straight into s1 (addu s1,a1,v1) and gives s2 to arg0. With the zeroing
   store spelled inline instead (retail's CSE shape) the base is materialised
   twice (97.24%, addu s2,a1,v1 + addu v0,a1,v0/addu v0,v0,v1) and with both
   stores inline it is 88.06%. Also tried: u8 buf first vs u16 *pRow first
   (identical), buf[idx] = 1 vs buf[slot] = 1 (the latter loses the move/andi
   split, 92.79%).

void func_8007D478(u8 **arg0, u8 arg1) {
    u8 buf[3];
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);

    buf[0] = buf[1] = buf[2] = 0;
    pRow[(*arg0)[1]] = 0;
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = func_8001BD40(0, 2);
        u8 idx = slot;

        if (buf[idx] == 0) {
            if ((D_800CCD64[idx].flags & 0x8000) && (D_800CCD64[idx].flags & 0x4002) == 0) {
                pRow[(*arg0)[1]] = func_80089C08(idx);
                break;
            }
            buf[idx] = 1;
        }
    }
}
*/
void func_8007D5B0(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if ((D_800CCD64[i].flags & 0xC000) == 0) {
            count++;
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
void func_8007D610(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i;
    s32 record;
    s32 offset;

    for (i = 3, record = 0x54, offset = 0x450; i < 0xB; i++, record += 0x1C, offset += 0x170) {
        if (D_800D2DCC[i] != 0 && (*(u16 *)((u8 *)D_800CCD64 + offset) & 0xC000) == 0 && D_800C3EB7[record] == 0) {
            count++;
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
void func_8007D6A8(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u32 best = -1;
    s32 bestIndex = 0;
    s32 i;
    s32 offset;
    s32 record;

    for (i = 0, offset = 0, record = 0; i < 3; offset += 0x170, i++, record += 8) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && D_800D32A1[record] != 0 && best >= *(u32 *)(D_800CCDEC + offset)) {
            best = *(u32 *)(D_800CCDEC + offset);
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
void func_8007D7B4(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u32 best = -1;
    s32 bestIndex = 0;
    s32 i;
    s32 offset;
    s32 record;

    for (i = 3, offset = 0x450, record = 0x18; i < 0xB; offset += 0x170, i++, record += 8) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && D_800D32A1[record] != 0 && best >= *(u16 *)(D_800CCD34 + offset)) {
            best = *(u16 *)(D_800CCD34 + offset);
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
