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
extern void func_8008509C(s32 a0, s32 a1);
extern void func_80039FF8(void);
extern void func_8007313C(void *pDst, void *pSrc);
extern void func_8008EB4C(s32 arg);
extern void func_80083C0C(s32 mode);
extern void func_8007F8B4(void);
extern void func_800812BC(u8 *p);
extern void func_80036420(void);
extern void func_8008151C(s32 *p);
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
    func_8008509C(0, 0);
    func_8008509C(1, 1);
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
/* 95.60%: gcc hoists the `a0 = 0` of the shared tail into both branches (one
   extra `move a0,zero` + a nop in the jal slot); retail sets it once at the
   merge point.  Writing the tail once after the if/else reorders the blocks
   outright (79.80%).
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
        func_8007F8B4();
    }
}
*/

void func_80080D10(void) {
    D_800926DC = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080D20);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080F04);

typedef struct {
    /* 0x00 */ u_long *pData;
    /* 0x04 */ SPRT sprt[2];
    /* 0x2C */ s16 width;
    /* 0x2E */ s16 x;
} Misc8Text; // Size: 0x30

extern Misc8Text D_80095510[2];

void func_80081094(Misc8Text *p, s32 entry, s32 flag) {
    s32 len;

    len = SystemRenderStringEntry(GetStringEntry(D_80092880, entry), p->pData, 0x3F, flag);
    p->width = len;
    p->x = (0x140 - len) / 2;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081100);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_800811AC);

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
