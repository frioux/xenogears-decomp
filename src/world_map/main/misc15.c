#include "common.h"
#include "system/memory.h"
#include "psyq/libgte.h"
#include "psyq/libcd.h"

typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x12];
    /* 0x18 */ s32 unk18;
    /* 0x1C */ s32 unk1C;
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[0x28];
    /* 0x4C */ s32 unk4C;
    /* 0x50 */ u8 unk50[0x30];
} Misc15Slot; // Size: 0x80

typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ u32 unk4;
    /* 0x8 */ s32 unk8;
    /* 0xC */ u8 unkC[0x18];
} Misc15CdRequest; // Size: 0x24

typedef struct {
    /* 0x0 */ u32 unk0[4];
} Misc15Block16; // Size: 0x10


extern Misc15Slot *D_8009BE24;
extern void *D_8009C184[];
extern MATRIX D_8009A180;
extern MATRIX D_8009D534;
extern u32 D_8009C580[];
extern s32 D_8009C5BC;
extern s32 D_8009C618;
extern u8 *D_8009BC3C;
extern u8 *D_8009BCB4;
extern s32 D_8009BCB8;
extern s32 D_8009BD2C;
extern s32 D_8009CD44;
extern void *D_8009D788[16];
extern void *D_8009C624[16];
extern void func_800981C8(s32);
extern void func_80097DC0(void);

extern void func_800976C8(void);
extern CdlLOC D_8009CEBC;
extern u8 *D_8009D3BC;
extern s32 D_8009D7F4;
extern s32 D_8009D614;
extern s32 D_8009D56C;
extern s32 D_8009CEB8;
extern s32 D_8009C590;
extern s32 D_8009BE48;
extern s32 D_8009CCB0;
extern s32 D_8009CCA8;
extern s32 D_8009CCA0;
extern void func_80096A6C(void);
extern u32 ArchiveGetDebugTable(void);
extern s32 func_800968E0(void);
extern void func_800966CC(void *p);

s32 func_800967E4(void) {
    s32 result = 0;
    u32 first;
    u32 second;

    first = ArchiveGetDebugTable();
    second = ArchiveGetDebugTable();
    if ((first == 0) | (second == 0xFFFFFFFF)) {
        result = func_800968E0();
        if (result == 0) {
            if (D_8009D788[D_8009BCB8] != NULL) {
                func_8009699C((Misc15CdRequest *)D_8009D788[D_8009BCB8]);
            }
        }
    } else {
        if (D_8009C624[D_8009BCB8] != NULL) {
            func_800966CC(D_8009C624[D_8009BCB8]);
            D_8009C624[D_8009BCB8] = NULL;
            D_8009BCB8 = (D_8009BCB8 + 1) & 0xF;
        }
    }
    return result;
}
// Best: 99.79% -- blocked, not a match problem: this switch's table is
// jtbl_80070CA0 in asm/world_map/data/0.rodata.s, whose entries are
// .word .L80096910/.L80096918/.L80096950/.L80096958. Converting the function
// deletes those local labels from the asm and the overlay stops linking
// (undefined reference to .L80096910 at .rodata+0x11b0). The table would have
// to move to C with the function, and a C definition cannot name code labels.
/*
s32 func_800968E0(void) {
    switch (D_8009CD44) {
    case 0:
        return 0;
    case 1:
    case 2:
    case 3:
        return 1;
    case 4:
        if (--D_8009BD2C == 0) {
            D_8009CD44++;
        }
        return 1;
    case 5:
        D_8009CD44 = 0;
        D_8009D788[D_8009BCB8] = 0;
        D_8009BCB8 = (D_8009BCB8 + 1) & 0xF;
        return 2;
    }
    return 3;
}
 */
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800968E0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_8009699C);
// Best: 85.17%. Structurally right (every global and call in place; the four
// zero stores, the (size+0x7FF)>>11 and the call sequence line up) but gcc
// drops the first `D_8009D3BC = p` store as dead (the original keeps both) and
// then re-orders the three loads ahead of it. Marking D_8009D3BC volatile
// keeps both stores (87.21%) but loses the `move a1,s0` in the CdIntToPos
// delay slot.
/*
void func_8009699C(Misc15CdRequest *p) {
    CdlLOC *pLoc;
    s32 lba;
    u32 size;
    u32 sizeCopy;
    s32 unk8;

    lba = p->unk0;
    D_8009CD44 = 1;
    pLoc = &D_8009CEBC;
    D_8009D3BC = (u8 *)p;
    D_8009D3BC = (u8 *)p + 0xC;
    size = p->unk4;
    sizeCopy = p->unk4;
    unk8 = p->unk8;
    D_8009BE48 = 0;
    D_8009CCB0 = 0;
    D_8009CCA8 = 0;
    D_8009CCA0 = 0;
    D_8009D7F4 = lba;
    D_8009D614 = lba;
    D_8009D56C = (size + 0x7FF) >> 11;
    D_8009CEB8 = sizeCopy;
    D_8009C590 = unk8;
    CdIntToPos(lba, pLoc);
    CdSyncCallback(func_80096A6C);
    CdControlF(CdlSetloc, (u8 *)pLoc);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096A6C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096C0C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80096F18);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097070);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097244);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097440);

void func_8009766C(void) {
    D_8009BE24 = HeapAlloc(0x2000, 0);
    func_800976C8();
}

void func_800976A0(void) {
    HeapFree(D_8009BE24);
}

void func_800976C8(void) {
    s32 i;

    for (i = 0; i < 0x40; i++) {
        D_8009BE24[i].unk4C = 0;
        D_8009BE24[i].unk18 = 0;
        D_8009BE24[i].unk1C = 0;
    }
}

void func_800976FC(s32 value, s32 index) {
    D_8009BE24[index].unk0 = 0;
    D_8009BE24[index].unk18 = value;
}

void func_80097718(s32 value18, s32 value1C) {
    s32 i;

    for (i = 0; i < 0x40; i++) {
        if (D_8009BE24[i].unk1C == 0) {
            D_8009BE24[i].unk0 = 0;
            D_8009BE24[i].unk2 = 0;
            D_8009BE24[i].unk4 = 0;
            D_8009BE24[i].unk18 = value18;
            D_8009BE24[i].unk1C = value1C;
            D_8009BE24[i].unk20 = 0;
            D_8009BE24[i].unk22 = 0;
            break;
        }
    }
}

s32 func_80097770(s32 index, s16 value) {
    Misc15Slot *pSlot = &D_8009BE24[index];

    if (pSlot->unk4 != 0) {
        return 0;
    }
    pSlot->unk0 = 1;
    pSlot->unk4 = value;
    return 1;
}

void func_800977A8(s32 index) {
    *(u16 *)((u8 *)D_8009BE24 + (index << 7)) = 3;
}

void func_800977C4(s32 index) {
    *(u16 *)((u8 *)D_8009BE24 + (index << 7)) = 4;
}

void func_800977E0(s32 index, u16 value) {
    u8 *pSlot = (u8 *)D_8009BE24 + (index << 7);

    *(u16 *)pSlot = 2;
    *(u16 *)(pSlot + 2) = value;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097800);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800978FC);
// Best: 82.47%. Everything matches except that the original holds s0 =
// &D_8009BC3C across the second HeapAlloc and reads the buffer back with
// `lw v1,0(s0)`; every shape tried (global stores, u8 **ppBuffer, a
// u8 *pBuffer local) gets lui/at for the store plus a fresh lui for the read,
// so the frame has no s0 slot and the copy loop's 0x10000 constant lands in
// v0 instead of t0. The two do-while loops and the +6 slot-filler bias are
// required for the rest to match.
/*
void func_800978FC(void) {
    u8 **ppBuffer;
    u8 *pBuffer;
    u8 *pSlot;
    Misc15Block16 *pSrc;
    Misc15Block16 *pDst;
    Misc15Block16 *pEnd;
    s32 i;

    ppBuffer = &D_8009BC3C;
    *ppBuffer = HeapAlloc(0x10000, 1);
    D_8009BCB4 = HeapAlloc(0x10000, 1);
    pBuffer = *ppBuffer;
    pSlot = pBuffer;
    i = 0;
    do {
        i++;
        pSlot[3] = 7;
        pSlot[7] = 0x24;
        pSlot[4] = 0x80;
        pSlot[5] = 0x80;
        pSlot[6] = 0x80;
        pSlot += 0x20;
    } while (i < 0x800);
    pSrc = (Misc15Block16 *)D_8009BC3C;
    pDst = (Misc15Block16 *)D_8009BCB4;
    pEnd = pSrc + 0x1000;
    do {
        *pDst++ = *pSrc++;
    } while (pSrc != pEnd);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_800979C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097BC0);
void func_80097CB8(s32 arg0) {
    u32 *pEnd;
    s32 i;

    D_8009D534 = D_8009A180;
    pEnd = D_8009C580;
    for (i = 0xFF; i >= 0; i--) {
        *pEnd-- = 0;
    }
    D_8009C5BC = 0;
    D_8009C618 = 0x400;
    func_800981C8(arg0);
    func_80097DC0();
}

void func_80097D64(void) {
    s32 i;

    for (i = 0; i < 0x100; i++) {
        if (D_8009C184[i] != NULL) {
            HeapFree(D_8009C184[i]);
        }
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc15", func_80097DC0);
