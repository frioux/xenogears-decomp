#include "common.h"

extern u8 D_800D2CA4[];
extern u8 D_800D2E5D[];
extern u8 D_800D2E60[];
extern u8 D_800D2E62[];
extern u16 D_800D39E0;
extern u8 D_800D2E5C[];
extern u8 D_800D3400[];
extern s32 func_8007EF6C(u8** arg0, u8 arg1, u8 arg2);
extern s32 func_8007F8C0(u8** arg0, u8 arg1);
// A 0x170-byte battle table entry; only the flag word at +0 is read here.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 unk02[0x16E];
} BattleTableEntry; // Size: 0x170

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
extern s32 func_80089C08(u8 arg0);
extern u8 D_800D2DE4[];
extern BattleTableEntry D_800CCD64[];
extern u8 D_800D2E5F[];
extern u8 D_800D2E61[];
extern u8 D_800D32A1[];
extern u8 D_800D366C;
extern s32* D_8005917C;
extern char D_8006FB08[];
extern char D_8006FB0C[];
extern char D_8006FB24[];
extern void func_800877E0(u8 arg0, u8 arg1);
extern void func_80087EDC(u8 arg0, u8 arg1);
extern void func_800881B8(u8 arg0, u8 arg1);
extern void func_800BC404(u16 arg0);
// A 0x48-byte row of the per-battle-slot table at D_800C3FFE.
typedef u16 BattleSlotRow[36]; // Size: 0x48

extern u8 D_800C3E8C;
extern u8 D_800C400B[];
extern BattleSlotRow D_800C4022[];
extern BattleSlotRow D_800C3FFE[];
extern u8* D_800C3EAC;
extern u8 D_800C402F[];
extern void func_800785D4(u8 arg0, u8 arg1);
extern void func_80078658(u8 arg0, u8 arg1);
extern void func_800787E0(u8 arg0, u8 arg1);
extern void func_8007887C(u8 arg0);
extern s32 func_8007A280(u8 arg0, u8 arg1, s32 arg2, s32 arg3);
extern void func_800716D8(void);
extern void func_800793F0(u8 arg0);
extern void func_80079674(u8 arg0);
extern void func_80085350(void);
extern void func_80085388(void);
extern s32 func_80080AE4(u8 arg0);
extern void func_800B89FC(s32 arg0, u8 arg1, s32 arg2, s32 arg3);
extern s32 func_80079ED8(u8 arg0, u8 arg1, u8 arg2, s32 arg3);

void func_8007887C(u8 arg0) {
    if (D_800C3E8C != 0) {
        D_800C400B[D_800C3EAC[0x2DA] * 72] = arg0;
        D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF8;
        D_800C4022[D_800C3EAC[0x2DA]][0] = D_800C3E8C - 1;
        D_800C3EAC[0x2DA] = D_800C3EAC[0x2DA] + 1;
    }
}
void func_8007893C(u8 arg0, u8 arg1) {
    if (D_800D2E5F[arg0 * 8] != 0) {
        func_80078658(arg0, arg1);
        func_800787E0(0x1E, arg1);
        func_8007887C(arg1);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078998);
void func_80078B34(u8 arg0, u8 arg1, u8 arg2) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xFD;
    D_800C4022[D_800C3EAC[0x2DA]][0] = 0;
    D_800C3FFE[D_800C3EAC[0x2DA]][0] = *(u16*)(D_800D2E62 + arg1 * 8);
    func_800877E0(arg0, arg2);
    if (D_800D2E60[arg1 * 8] != 1) {
        if (D_800D32A1[arg0 * 8] == 0) {
            func_80087EDC(arg0, arg2);
        } else {
            func_800881B8(arg0, arg2);
        }
    }
    func_800BC404(func_80089C08(arg0) | *(u16*)(D_800D2E62 + arg1 * 8));
    func_800785D4(arg0, arg1);
}
void func_80078C9C(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xFC;
    func_800785D4(arg0, arg1);
}
void func_80078CEC(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = D_800D2E60[arg1 * 8];
    func_800785D4(arg0, arg1);
}

void func_80078D48(u8 arg0, u8 arg1) {
    *(u16*)&D_800D39E0 = *(u16*)(D_800D2E62 + arg1 * 8);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078D6C);
/* Best: 86.09%. The second store is stolen into the lhu load-delay slot by gcc
   unless its address register is clobbered by the 0x170 multiply chain, and only
   the folded `&D_800C3D1B[arg0 - 3].unk0` address does that (chain reuses v0), but
   it folds -12 into %hi(D_800C3D1B) and costs the original's separate
   addiu/sll + %hi/addu address. Kept in place by volatile; the remaining diff is
   that address shape only. Also: -O1 scores 90.74, so the mover is an -O2 pass.

void func_80078D6C(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF9;
    func_800785D4(arg0, arg1);
    D_800D2DE4[arg0] = 0xFF;
    *(volatile u8*)&D_800C3D1B[arg0 - 3].unk0 = 1;
    D_800CCD64[arg0].flags &= 0x8000;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078E24);
void func_80079054(u8 arg0, u8 arg1) {
    func_80079ED8(arg0, D_800D2E5D[arg1 * 8], D_800D2E60[arg1 * 8], 0);
}
void func_80079098(u8 arg0, u8 arg1) {
    s32 idx;
    s32 value;

    idx = arg1 * 8;
    value = func_80079ED8(arg0, D_800D2E5D[idx], 0, 1);
    func_80079ED8(arg0, D_800D2E5D[idx], D_800D2E60[idx] + value, 0);
}
void func_80079114(u8 arg0, u8 arg1) {
    func_8007A280(arg0, D_800D2E5D[arg1 * 8], D_800D2E60[arg1 * 8] | (D_800D2E61[arg1 * 8] << 8), 0);
}
void func_8007916C(u8 arg0, u8 arg1) {
    s32 idx;
    s32 value;

    idx = arg1 * 8;
    value = func_8007A280(arg0, D_800D2E5D[idx], 0, 1);
    func_8007A280(arg0, D_800D2E5D[idx],
                  (u16)(D_800D2E60[idx] + (value + (D_800D2E61[idx] << 8))), 0);
}
void func_800791FC(u8 arg0, u8 arg1) {
    func_8007893C(arg1, arg0);
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF4;
    func_800785D4(arg0, arg1);
}
void func_80079270(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xF6;
    D_800C3FFE[D_800C3EAC[0x2DA]][0] = *(u16*)(D_800D2E62 + arg1 * 8);
    func_800785D4(arg0, arg1);
}
void func_800792F8(u8 arg0, u8 arg1) {
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
            func_800716D8();
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
void func_80079674(u8 arg0) {
    if (D_800D32A1[arg0 * 8] != 0) {
        D_800C400B[D_800C3EAC[0x2DA] * 72] = arg0;
        D_800C402F[D_800C3EAC[0x2DA] * 72] = 0x1B;
        D_800C3EAC[0x2DA] = D_800C3EAC[0x2DA] + 1;
    }
    D_800C400B[D_800C3EAC[0x2DA] * 72] = arg0;
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xFE;
    D_800D366C = 0;
}
void func_80079778(u8 arg0) {
    D_800C3EAC[0x2DA] = 0;
    func_80085350();
    func_80085388();
    D_800C400B[D_800C3EAC[0x2DA] * 72] = arg0;
    func_800716D8();
    func_800B89FC(1, arg0, 0, func_80080AE4(arg0));
    func_800793F0(arg0);
    func_80079674(arg0);
    while (D_800C3EAC[0x2DB] == 0) {
        func_800716D8();
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
        *(u16*)&D_800D342E[slot * 0x40] = func_80089C08(arg0);
        for (i = 0; i < 5; i++) {
            D_800D3439[slot * 0x40 + i] = D_800D2CA4[i];
        }
        pBuf = D_800C3EAC + (arg0 & 0xFF) * 0x40;
        if (D_800C48E8 & func_80089C08(pBuf[0x3C])) {
            D_800D343E[slot * 0x40] = 1;
        } else {
            D_800D343E[slot * 0x40] = 0;
        }
    }
}
*/void func_80079934(u8** pCursor) {
    *pCursor += 4;
}
void func_80079948(u8** pCursor) {
    while (**pCursor >= 0x80) {
        func_80079934(pCursor);
    }
    while ((u32)((**pCursor - 0x80) & 0xFF) >= 0x70) {
        func_80079934(pCursor);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_800799C8);
/* Best: 87.14%. Whole structure, both clear loops (the *p++ / p < pEnd do-while
   and the 0x8B8 / -0x48 loop) and the if/else layout are right. Left: gcc merges
   the while test and the 0xFF break into two compares at the top of the body
   (0xFD, then 0xFF parked in a callee-saved $s2, hence the extra sw/lw s2) where
   the original has the 0xFD test only at the bottom and materialises 0xFF in the
   bne delay slot. `cmd` must be u32 (u8 gives an andi per use, s32 gives slti)
   and the >= 0x80 test must come first or the arms swap order.
   Tried: while with the load in the condition, for(;;) with two breaks, break vs
   return for the 0xFF exit, u8/u32/s32 cmd, u8 value.

void func_800799C8(u8 arg0) {
    u8 *p;
    u8 *pEnd;
    u8 *pCursor;
    u8 slot;
    u8 value;
    u32 cmd;
    s32 i;

    value = 0;
    slot = arg0 - 3;
    pCursor = *(u8 **)(D_800D3400 + slot * 0x40);
    p = D_800D2E5C;
    pEnd = p + 0x100;
    do {
        *p++ = 0;
    } while (p < pEnd);
    for (i = 0x8B8; i >= 0; i -= 0x48) {
        D_800C402F[i] = 0xFF;
    }
    for (;;) {
        cmd = *pCursor;
        if (cmd == 0xFD) {
            break;
        }
        if (cmd == 0xFF) {
            break;
        }
        if (cmd >= 0x80) {
            if ((func_8007F8C0(&pCursor, slot) & 0xFF) != 0) {
                continue;
            }
            func_80079948(&pCursor);
        } else {
            value = func_8007EF6C(&pCursor, slot, value);
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079AB0);
/* Best: 98.66%. Everything but three allocator/CSE tie-breaks is identical: (1)
   retail keeps the single `slot & 0xFF` in $a0 and reuses it for the 0x170 index
   and D_800C3D18/D_800D3408, ours splits it into a $v0 copy plus a later $a0 one;
   (2) retail's early `return result` loads the variable (`addu v0,s2,zero`) where
   gcc folds it to zero here; (3) `lw a0,0x10(sp)` vs our `lw v0,0x10(sp)` for the
   command byte. The return type must be s32 (as void, gcc deletes `result = 1`);
   D_800C3D18 is a 4-byte-stride table read as bytes, hence the `slot * 4` index.
   Tried: value=0/result=0 in both orders and at both positions (that pair is now
   exact), slot as the 7th-vs-8th local declaration, the else arm written as
   `cmd >= 0x80` first and as `cmd < 0x80` first.

s32 func_80079AB0(u8 arg0) {
    s32 result;
    u8 value;
    u8 slot;
    u32 cmd;
    u8* pCursor;
    u8* p;
    u8* pEnd;

    result = 0;
    value = 0;
    slot = arg0 - 3;
    if ((D_800CCD64[slot + 3].flags & 0x8000) != 0) {
        if ((D_800CCD1C[slot + 3].flags & 0x800) == 0) {
            return result;
        }
    }
    D_800D2E5C[0] = 0;
    if (D_800C3D18[slot * 4] != 0) {
        pCursor = *(u8**)(D_800D3408 + slot * 0x40);
        p = D_800D2E5C;
        pEnd = p + 0x100;
        do {
            *p++ = 0;
        } while (p < pEnd);
        for (;;) {
            cmd = *pCursor;
            if (cmd == 0xFD) {
                break;
            }
            if (cmd == 0xFF) {
                break;
            }
            if (cmd >= 0x80) {
                if ((func_8007F8C0(&pCursor, slot) & 0xFF) != 0) {
                    continue;
                }
                func_80079948(&pCursor);
            } else {
                if (cmd == 0x62) {
                    result = 1;
                }
                value = func_8007EF6C(&pCursor, slot, value);
            }
        }
    }
    if (D_800D2E5C[0] != 0) {
        func_800793F0(slot + 3);
    }
    return result;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079C24);
