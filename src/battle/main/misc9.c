#include "common.h"

extern u8 D_800D2C60[];
extern u8 D_800D2C8B[];
extern u8 D_800D2DC0;
extern u8 *D_800D3278;
extern u8 *D_800D3364;
extern u8 D_800D3410[];
extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u8 D_800C3EB7[];
extern u8 D_800CCD62[];
extern u8 D_800CD1B2[];
extern u8 D_800D32A1[];
extern u8 D_800D32B1[];
extern u8 D_800D32F1[];
extern u8 D_800C3EB4[];
extern u8 func_8007A628(u8 arg0, u8 arg1);
extern s32 func_8007A280(u8 arg0, u8 arg1, s32 arg2, s32 arg3);
extern u8 func_8001BD40(s32 arg0, u8 arg1);
extern u16 func_80089C08(u8 arg0);

/* best: 80.37%. Same family as func_8007DA1C/DB78/DCF8/DE78/DFD4 (all still
   stubs). Structure recovered from the asm: zero the row slot, then a do-while
   over the operator slots collecting the matching ones into an 8-byte stack
   buffer, then pick one at random. The `idx = i++` copy is real: retail keeps
   it in a separate register (s3) and re-forms it at the loop latch, and the
   buffer stores idx, not i. Remaining misses, all at 80.37% (336 bytes, frame
   and saved-register set already exact): (1) the final store re-forms the row
   base with a fresh `lui a1 / addiu a1 / addu a0,a0,a1` instead of reusing the
   loop's base register - tried a pOut pointer (77-80%, gcc reuses the loop
   base or spills), `(s32)D_800D3420` staged local (76.10%, shape right but one
   extra insn) and the plain inline expression (the $at form);
   (2) retail steals `i++` into the beqz delay slot after the func_8007A628
   call, gcc emits it at the loop head;
   (3) declaration order of the two zeroed accumulators (retail i then count).
void func_8007D8C0(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 count = 0;
    s32 i = 0;
    s32 idx;
    u16 *pField = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);

    pField[(*arg0)[2]] = 0;
    idx = i;
    do {
        idx = i++;
        if (func_8007A628(idx, 0) != 0) {
            if (pField[(*arg0)[3]] & func_8007A280(idx, (*arg0)[1], 0, 1)) {
                buf[count] = idx;
                count++;
            }
        }
    } while (i < 3);
    if (count != 0) {
        u8 *pBase = D_800D3420 + (arg1 & 0xFF) * 0x40;

        *(u16 *)(pBase + (*arg0)[2] * 2) = func_80089C08(buf[func_8001BD40(0, count - 1)]);
    }
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007D8C0);
/* best: 75.77%. Same family as func_8007D8C0/DE78 (see D8C0's note). Structure
   recovered and confirmed byte-for-byte against the -(13) prologue, the row
   zero, the count loop and the call sequence; two misses remain:
   (1) the tail store. Retail re-forms the row base with `lui a1,%hi / addiu a1 /
   addu a0,a0,a1` (a real register via `la`) and then adds the index; every shape
   tried (inline byte expression, (u16*)cast array index, s32 base local in both
   operand orders, u8 *pBase local, reuse of the pRow local) makes gcc fold the
   two register terms first and emit `sh v0,D_800D3420(a0)`, i.e. maspsx's
   $at form. So it is gcc's combine merging (sym+reg) into the store address that
   retail's compiler did not do, not a source detail.
   (2) retail steals `addiu s1,s1,1` into the beqz delay slot after the
   func_8007A628 call; gcc emits it at the loop bottom.
   func_8007DE78 is this function with i starting at 0, the loop bound 3 and
   func_8007A628's second argument 1, so it shares both misses.
void func_8007DA1C(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 count = 0;
    s32 i;
    u8 idx;
    u16 *pRow = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);

    pRow[(*arg0)[2]] = 0;
    for (i = 3; i < 0xB; i++) {
        idx = i;
        if (func_8007A628(idx, 0) != 0) {
            if (pRow[(*arg0)[3]] & func_8007A280(idx, (*arg0)[1], 0, 1)) {
                buf[count] = i;
                count++;
            }
        }
    }
    if (count != 0) {
        u8 *pBase = D_800D3420 + (arg1 & 0xFF) * 0x40;

        *(u16 *)(pBase + (*arg0)[2] * 2) = func_80089C08(buf[func_8001BD40(0, count - 1)]);
    }
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DA1C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DB78);
/* best: 80.29%. func_8007DB78/DE78 with i starting at 3, the loop bound 0xB and
   a D_800D32A1[offset] gate whose offset starts at 0x18 and steps +8 (written as
   a for-increment so the original's addiu s4,s4,8 lands in the bnez delay
   slot). The tail store uses the same fix as func_8007DE78's note (value
   local + int-built base + pointer local). Remaining: idx = i at the loop top
   is propagated away by gcc (retail keeps move s3,s1 and an extra saved
   register, frame 0x40 vs my 0x38), plus DE78's nits (1) and (3).
void func_8007DCF8(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 offset = 0x18;
    s32 idx;
    u16 *pRow = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);

    pRow[(*arg0)[2]] = 0;
    for (; i < 0xB; i++, offset += 8) {
        idx = i;
        if (func_8007A628(idx, 0) != 0) {
            if (D_800D32A1[offset] != 0) {
                if (pRow[(*arg0)[3]] & func_8007A280(idx, (*arg0)[1], 0, 1)) {
                    buf[count] = idx;
                    count++;
                }
            }
        }
    }
    if (count != 0) {
        u16 value = func_80089C08(buf[func_8001BD40(0, count - 1)]);
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)D_800D3420);

        pOut[(*arg0)[2]] = value;
    }
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DCF8);
/* best: 89.33% (336 bytes; mine 340). This is the func_8007D8C0/DA1C
   family and the family's documented blocker (the tail store re-forming the row
   base) IS SOLVED here: write the tail as a u16 value local, then a pointer
   local whose base is built in INT arithmetic, then the store:

       u16 value = func_80089C08(buf[func_8001BD40(0, count - 1)]);
       u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)D_800D3420);
       pOut[(*arg0)[2]] = value;

   That reproduces retail's andi/sll/lw p/lui a1/addiu a1/lbu p[2]/addu a0,a0,a1/
   sll/addu/sh exactly (the (s32) cast stops gcc folding the symbol into the
   store as %lo(sym)(reg), and the separate `value` statement keeps the address
   computation after the two jal's instead of hoisting it). Remaining, 3 insns:
   (1) retail keeps a temp copy of the row base (addu v1,v1,v0 then move s5,v1,
   and the la lands in v0 before the remaining saves) where gcc computes
   addu s5,v1,a0 with the la in a0 after them - tried the plain pointer form,
   (arg1 & 0xFF) * 0x40 + (s32)D_800D3420, and an s32 base local converted to
   u16* (all 89.33%, gcc coalesces the copy away).
   (2) sw s6/move s6,a1 is emitted before the i/count zero-inits here, after
   them in retail (declaration order i then count already matches retail's
   s1-then-s2 zeroing).
   (3) retail steals addiu s1,s1,1 into the beqz delay slot after the
   func_8007A628 call; with idx = i++ the whole loop body is byte-exact
   otherwise, and that shape is required (idx must stay a separate register;
   idx = i inside the body makes gcc propagate it and lose s3).
void func_8007DE78(u8 **arg0, s32 arg1) {
    u8 buf[8];
    s32 i = 0;
    s32 count = 0;
    s32 idx;
    u16 *pRow = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);

    pRow[(*arg0)[2]] = 0;
    do {
        idx = i++;
        if (func_8007A628(idx, 1) != 0) {
            if (pRow[(*arg0)[3]] & func_8007A280(idx, (*arg0)[1], 0, 1)) {
                buf[count] = idx;
                count++;
            }
        }
    } while (i < 3);
    if (count != 0) {
        u16 value = func_80089C08(buf[func_8001BD40(0, count - 1)]);
        u16 *pOut = (u16 *)((arg1 & 0xFF) * 0x40 + (s32)D_800D3420);

        pOut[(*arg0)[2]] = value;
    }
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DE78);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007DFD4);
void func_8007E154(u8 **arg0, s32 arg1) {
    s32 idx = (arg1 & 0xFF) + 3;

    D_800C3EB7[idx * 0x1C] = (*arg0)[1];
    if ((*arg0)[1] & 0x80) {
        func_80087EDC((u8)(arg1 + 3), (u8)(arg1 + 3));
    } else {
        func_800883AC((u8)(arg1 + 3));
    }
}
void func_8007E1D0(u8 **arg0, s32 arg1) {
    u16 value = func_80089C08((arg1 + 3) & 0xFF);
    u8 *p = *arg0;
    u16 *pField = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);

    pField[p[1]] = value;
}
void func_8007E234(u8 **arg0, u8 arg1) {
    s32 acc = 0;
    s32 i;
    u16 *pRow;

    // the slot table is walked downwards from D_800D32B1
    for (i = 2; i >= 0; i--) {
        if ((u8)func_8007A744((u8)i) != 0) {
            switch ((*arg0)[2]) {
            case 1:
                if (D_800D32B1[(i - 2) * 8] != 0) {
                    acc |= 1;
                }
                break;
            case 2:
                if (D_800D32B1[(i - 2) * 8] == 0) {
                    acc |= 1;
                }
                break;
            default:
                acc |= 1;
                break;
            }
        }
        acc <<= 1;
    }
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = (u16)acc >> 1;
}
/* best: 95.31%. Same family as func_8007E234 (walk the slot table downwards,
   `(i - 0xA) * 8` keeps D_800D32F1 as the base symbol). Only difference: the
   original doubles the accumulator into a temp and copies it back (sll a1,s0,1
   / move s0,a1) and reuses a1 for the final shift, gcc does it in place
   (sll s0,s0,1). Tried: acc*=2, acc=acc*2, acc=2*acc, acc=acc+acc,
   next=acc*2;acc=next, doubling in the for-increment clause (91.06%).
void func_8007E334(u8 **arg0, u8 arg1) {
    s32 acc = 0;
    s32 i;
    u16 *pRow;

    // the slot table is walked downwards from D_800D32F1
    for (i = 0xA; i >= 3; i--) {
        if ((u8)func_8007A744((u8)i) != 0) {
            switch ((*arg0)[2]) {
            case 1:
                if (D_800D32F1[(i - 0xA) * 8] != 0) {
                    acc |= 1;
                }
                break;
            case 2:
                if (D_800D32F1[(i - 0xA) * 8] == 0) {
                    acc |= 1;
                }
                break;
            default:
                acc |= 1;
                break;
            }
        }
        acc *= 2;
    }
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = acc << 2;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E334);
/* best: 80.63%. The record table takes an explicit byte offset
   (D_800C3EB4[offset], offset from 0x38 stepping -0x1C): that is the only form
   that reproduces the original's two `lui at / addu at / lbu %lo(at)` loads and
   the `li s2,0x38` prologue. Remaining: gcc splits the row base into
   `(arg1 & 0xFF) << 6` plus a hoisted `&D_800D3420` (the original keeps the
   whole sum in s4) and hoists a third symbol base, so the frame grows to 0x38.
   Tried: row base as a pRow local (77.49%, gcc CSEs it with the store's), row
   base inline + pRow local for the store (80.63%).
void func_8007E438(u8 **arg0, u8 arg1) {
    s32 acc = 0;
    s32 offset = 0x38;
    s32 i;
    u16 *pRow;

    for (i = 2; i >= 0; i--) {
        if ((u8)func_8007A744((u8)i) != 0) {
            if (D_800C3EB4[(u8)func_80079E7C(*(u16 *)(D_800D3420 + arg1 * 0x40 + (*arg0)[2] * 2)) * 0x1C] ==
                D_800C3EB4[offset]) {
                acc |= 1;
            }
        }
        acc <<= 1;
        offset -= 0x1C;
    }
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = (u16)acc >> 1;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E438);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E554);
/* best: 89.08%. Structure: for (i = 0xA; i >= 3; i--) over the operator slots,
   comparing D_800C3EB4[(u8)func_80079E7C(pField[p[2]]) * 0x1C] against
   D_800C3EB4[offset] with offset = 0x118 stepping -0x1C, then storing
   `acc << 2` into the row. The row base MUST be a local pointer computed before
   the loop (inline gives 74.68%: gcc hoists &D_800D3420 and the <<6 separately
   and does the addu inside the loop, growing the frame to 0x38).
   Remaining: (1) gcc hoists &D_800C3EB4 into a register for the first of the two
   record loads only (retail uses the `lui at / addu at / lbu %lo(at)` form for
   both, so its 6th saved reg is absent); spelling the index through a separate
   `s32 slot` local is the next thing to try. (2) the accumulator doubling: retail
   emits `a1 = s1 << 1` on both paths and copies back at the join (sll a1,s1,1 /
   move s1,a1, and the final store shifts a1), gcc doubles in place
   (sll s1,s1,1) - same miss as func_8007E334, so `acc = acc * 2` vs
   `acc <<= 1` should be re-screened here.
void func_8007E554(u8 **arg0, u8 arg1) {
    s32 acc = 0;
    s32 offset = 0x118;
    s32 i;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);
    u16 *pRow;

    for (i = 0xA; i >= 3; i--) {
        if ((u8)func_8007A744((u8)i) != 0) {
            if (D_800C3EB4[(u8)func_80079E7C(pField[(*arg0)[2]]) * 0x1C] ==
                D_800C3EB4[offset]) {
                acc |= 1;
            }
        }
        acc <<= 1;
        offset -= 0x1C;
    }
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = acc << 2;
} */
void func_8007E674(u8 arg0) {
    ((u32 *)D_800D2C60)[arg0] = 0;
    D_800D2C8B[arg0] = 4;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E6A0);
/* best: 99.00%. Identical instruction multiset and order; the only difference
   is the register holding the D_800D3410 sum base: original a2 (and it is never
   reused), gcc v1 (which the p[1] scale temp then reuses). Same allocator tie as
   func_8007E6F0/E740/E98C/E8AC and the misc6 D_800D34x0 family. Tried here:
   split summand locals (84.35%, reorders the three lbu's RHS-first), three
   explicit `s32 i3/i1/i2 = p[..]` index locals (99.00%, same base v1), the
   byte-pointer form `(s32 *)((u8 *)D_800D3410 + (arg1 & 0xFF) * 0x40)` (99.00%),
   and `D_800D3410 + ((arg1 & 0xFF) << 6)` (99.00%). Also tried before: pRow/p in
   both orders, `s32 arg1` + explicit mask, element-unit base, all references
   inline. No try_flags.sh row reaches 100 (2.6.0 99.0, cdk 96.5, -O1 51.9), and
   the unit cannot move to cdk at all: func_8007E154 79.68%, func_8007E674 10.45%
   under cdk while matched under Default.
void func_8007E6A0(u8 **arg0, s32 arg1) {
    u8 *p = *arg0;
    s32 *pRow = (s32 *)(D_800D3410 + (arg1 & 0xFF) * 0x40);

    pRow[p[3]] = pRow[p[1]] + pRow[p[2]];
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E6F0);
/* best: 99.00%. Same miss as func_8007E6A0 (the D_800D3410 sum base: original
   a2, gcc v1); the subtraction itself is byte-exact. See the note there for the
   shapes already tried.
void func_8007E6F0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    s32 *pRow = (s32 *)(D_800D3410 + arg1 * 0x40);

    pRow[p[3]] = pRow[p[1]] - pRow[p[2]];
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E740);
/* best: 95.31%. Identical instruction multiset and operand order; the only
   difference is the register pair: the original keeps the D_800D3410 base in
   v0 and the row address in v1 (mflo then reuses the dead v0), gcc emits the
   mirror image (base v1, row v0, mflo a2). Same allocator tie as func_8007E8AC
   and the misc6 D_800D34x0 family. Tried: s32 vs u32 element, pRow before/after
   pArgs, product written as p[2] * pRow[p[1]] (this variant, operand order
   right), compound assignment, and func_8007E780's exact shape copied verbatim
   with `p` declared first (95.00%, the mult operands then swap to `mult v1,a0`).
   Decisive: the MATCHED divide sibling func_8007E780 has byte-identical asm
   (same base v0 / slot v1 allocation) under the same C shape, so the split is
   the multiply's HI/LO pseudo shifting the allocator, not the source.
void func_8007E740(u8 **arg0, u8 arg1) {
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);
    u8 *p = *arg0;

    pRow[p[1]] = p[2] * pRow[p[1]];
} */
void func_8007E780(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);

    pRow[p[1]] /= p[2];
}
void func_8007E7C0(u8 **arg0) {
    u8 *p = *arg0;

    *(u16 *)(D_800D3278 + p[1] * 2 + 0x394) = p[2];
}
/* best: 96.50%. Structure found: do-while with the pointer compared as a SIGNED
   int (a u16* compare gives sltu, the original has slt), and the flag tested as
   `(u8)flag` so it is truncated at each of the two test sites. Only remaining
   difference: gcc fuses the truncation into the preheader (`andi s2,v0,0xff`)
   and drops the copy at the loop back edge, the original keeps the untruncated
   value in s2 (`move s2,v0; andi v0,s2,0xff`) and re-emits the andi in the
   bnez delay slot. Tried: u8 flag (94.60%), s32+cast (96.50%), flag&0xFF
   (94.60%), u8 copy inside the loop (96.50%).
void func_8007E7E4(u8 **arg0) {
    s32 flag = (*arg0)[2] != 0;
    s32 addr = (s32)D_800CCD62;

    do {
        if ((u8)flag) {
            *(u16 *)addr |= func_80089BEC((u8)((*arg0)[1] + 7));
        } else {
            *(u16 *)addr &= ~func_80089BEC((u8)((*arg0)[1] + 7));
        }
        addr += 0x170;
    } while (addr < (s32)D_800CD1B2);
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E7E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E8AC);
/* best: 95.38%. Identical instruction multiset and instruction order; only the
   v0/v1 pair is mirrored (original: p[2] in v1, p[1] in v0, sum in v1, the
   D_800D3364 pointer reusing v0). Same allocator tie as func_8007E740. Tried:
   both summand orders, inline index, s32 idx local, pointer local for the base.
   Confirmed a pure labelling tie: both sides end in the same `addu v0,v0,v1`;
   only the two scale temps and the D_800D3364 pointer swap v0/v1.
void func_8007E8AC(u8 **arg0) {
    u8 *p = *arg0;
    s32 idx = p[2] * 8 + p[1] * 0x40;

    D_800D3364[idx + 0x140] = p[3];
} */
void func_8007E8E0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);

    D_800D2DC0 = func_80079E7C(pField[p[1]]) + 1;
}
void func_8007E934(void) {
    u8 buf[0x10];

    func_80078508(buf);
}
s32 func_8007E954(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return pRow[p[1]] == p[2];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc9", func_8007E98C);
/* best: 96.47%. Identical instruction multiset and order once the 16-bit value
   is a local; only the register pair is mirrored (original: base v1, p in a0,
   p[3] in a2; gcc: base v0, p v1, p[3] a0). Same allocator tie as func_8007E6A0.
   Tried: value inline in the comparison (73.41%, reorders the lbu's), pField/p
   declared in both orders, `s32 arg1` + explicit `(arg1 & 0xFF) * 0x40`, and
   `u8 arg1` + element index `(u16 *)D_800D3420 + arg1 * 0x20` (96.47%, same two
   mirrored pairs).
s32 func_8007E98C(u8 **arg0, s32 arg1) {
    u8 *p = *arg0;
    u16 *pField = (u16 *)(D_800D3420 + (arg1 & 0xFF) * 0x40);
    s32 value = p[2] | (p[3] << 8);

    return pField[p[1]] == value;
} */
s32 func_8007E9D0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    return p[2] >= pRow[p[1]];
}
