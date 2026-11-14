#include "common.h"
#include "psyq/libgpu.h"

extern u8 D_80092954[];

extern void func_80083C0C(int mode);
extern void SystemStringFree(void* pString);

void func_800707A8(void) {
    func_80083C0C(3);
    SystemStringFree(&D_80092954[0]);
}

s32 func_800707D8(s32 a0, s32 a1, s32 a2) {
    s32 d;

    d = a0 - a1;
    if (d < 0) {
        d++;
        d -= a2;
    } else {
        d--;
        d += a2;
    }
    return d / a2;
}

extern s32 D_8009867C;
extern s32 D_80098680;
extern s32 D_80098684;
extern s32 D_800925F4;
extern s32 D_800925F8;
extern s32 D_800925FC;
extern s32 D_80097058;
extern s32 D_800970E0;
extern s32 D_80098774;
extern s32 D_800987FC;

/* 66.68%: original keeps &D_8009867C in $s0 across the first call and reads and
   stores through it; ours rematerialises the address (lui at / lw / sw). The same
   C without the pointer local scores the same; -G0 65.72, gcc 2.6.0 65.72.
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80070808);

/* 66.68% attempt:
void func_80070808(s32* p, s32 step) {
    s32* pX = &D_8009867C;

    *pX += func_800707D8(p[0], *pX, step);
    D_80098684 += func_800707D8(p[2], D_80098684, step);
    D_80098680 += func_800707D8(p[1], D_80098680 + D_800925F4, step);
}
*/

extern s32 D_8009871C;
extern s32 D_80098720;
extern s32 D_80098724;
extern void func_800828F8(s32* pA, s32* pB, s32 limit);

/* 74.00%: same missing shape as func_80070808, the original keeps &D_8009871C in
   $s2 across all four calls and reaches it through the register; ours re-forms it
   with lui/addiu each time.
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800708C4);

/* 74.00% attempt:
void func_800708C4(s32* p, s32 step) {
    s32 values[3];

    values[0] = func_800707D8(p[0], D_8009871C, step);
    values[1] = func_800707D8(p[1], D_80098720, step);
    values[2] = func_800707D8(p[2], D_80098724, step);
    func_800828F8(&D_8009871C, values, 0x3D00);
    D_8009871C += values[0];
    D_80098720 += values[1];
    D_80098724 += values[2];
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007099C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80070C7C);

void func_80070F80(s32 value) {
    D_800925F8 = value;
    D_80098774 = 0;
    D_80097058 = 0;
    D_800925FC = 0;
    D_800987FC &= 0xFFFF7FFF;
    D_800970E0 &= 0xFFFF7FFF;
}

extern s32 D_80092934;

s32 func_80070FD8(u8* p) {
    VECTOR vec;
    s32 angle;

    vec = *(VECTOR*)p;
    vec.vx -= 0x3F80;
    vec.vz -= 0x3F80;
    angle = ratan2(vec.vx, vec.vz) & 0xFFF;
    if (angle >= 0x201) {
        *(s32*)(p + 0x58) = 0x800 - D_80092934;
    } else {
        *(s32*)(p + 0x58) = 0xC00 - D_80092934;
    }
    *(s32*)(p + 0x48) = 0xFF;
    *(s16*)(p + 0xCE) = 0;
    return 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007107C);

extern s32 D_800925E0;
extern s32 D_800925E4;
extern void* D_80092868;
extern u8 D_800928A0;
extern u8 D_800929E4[];

void func_80071724(void* pOt) {
    u8* p;

    p = D_80092868;
    *(s32*)(p + 0x7C) = D_800925E0 | (D_800925E4 << 16);
    AddPrim(pOt, p + 0x74);
    AddPrim(pOt, &D_800929E4[D_800928A0 * 8]);
}

extern u32 D_800929EC;
extern u32 D_800929F0;
extern u8 D_8009A158;
extern u8 D_8009A159;
extern u16 D_8009A15A;

/* 0x10-byte texture primitive template: tag, colour/code, xy, then UV and CLUT */
typedef struct {
    /* 0x0 */ u_long tag;
    /* 0x4 */ u_char r0, g0, b0, code;
    /* 0x8 */ s16 x0, y0;
    /* 0xC */ u_char u0, v0;
    /* 0xE */ u16 clut;
} BattleTexBlock; // Size: 0x10

extern BattleTexBlock D_8009A14C;
extern BattleTexBlock D_8009A244;

/* 79.36%: every statement is right and the size is retail's 300 bytes, but
   gcc re-forms the second load's base (lui v1,%hi(D_800929E4); lw v1,4(v1)) where
   retail reuses the $s0 already holding the address, hoists the GetClut argument
   loads (lh a0/lh a1) above the D_800929EC stores, and sinks the `sh v0,
   D_8009A15A` of GetClut's result below the 16-byte struct copy. Tried: two u32
   loads hoisted into locals, (u32*)pTPage[i] vs pTPage->tag/->code[0] (both
   rematerialise), a DR_TPAGE struct copy for the pair (fixes $s0 but makes the
   second store a %lo(D_800929EC+4) reloc), copy via pointer local, and
   (u_short*)tim.prect instead of RECT fields (gcc still narrows to lbu where
   retail has lhu, so the andi 0x3F follows a halfword load). The unused RECT
   local is load-bearing: it reproduces retail's 0x38 frame.
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071794);

/* 79.36% attempt:
void func_80071794(u8* p) {
    TIM_IMAGE tim;
    u_short tpage;
    s16* pClut;
    DR_TPAGE* pTPage;
    BattleTexBlock* pBlock;
    RECT rect;

    OpenTIM(*(u_long**)(p + 0x60));
    ReadTIM(&tim);
    pClut = (s16*)tim.caddr;
    pClut[0] = 0;
    pClut[2] = -0x8000;
    pClut[3] = -1;
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    tpage = GetTPage(0, 1, tim.prect->x, tim.prect->y);
    pTPage = (DR_TPAGE*)&D_800929E4[0];
    SetDrawTPage(pTPage, 0, 0, tpage);
    D_800929EC = pTPage->tag;
    D_800929F0 = pTPage->code[0];
    D_8009A158 = (((u_short*)tim.prect)[0] & 0x3F) << 2;
    D_8009A159 = ((u_short*)tim.prect)[1];
    D_8009A15A = GetClut(tim.crect->x, tim.crect->y);
    pBlock = &D_8009A244;
    *pBlock = D_8009A14C;
}
*/

extern s32 D_800925D4;
extern s32 D_800925D8;
extern u8 D_8009868C[];
extern s32 func_80032F54(void* pString, s16 a, s16 b, s16 c, s16 d, s16 e, s16 f);

void func_800718C0(void) {
    D_800925D8 = -1;
    D_800925D4 = 0;
    func_80032F54(&D_8009868C[0], 0x140, 0x30, 0x1C, 0x9A, 0x40, 4);
}

extern u8 D_80092608;
extern s16 D_80098698;
extern s16 D_80098692;
extern s32 D_8009105C[];
extern s32 D_800925E8;
extern s32 D_800925E0;
extern s32 D_800925EC;
extern s32 D_800925E4;
extern s16 D_80092600;
extern s16 D_800987E0;
extern u16 D_800987E8;
extern s16 D_800970C4;
extern u16 D_800970CC;
extern void func_8008EB4C(s32 id);

/* 89.15%: reorg steals the else branch's li v0,0x4 into the bnez delay slot
   where the original leaves a nop, which makes our value pseudo land in v1. */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007191C);

/* 89.15% attempt:
void func_8007191C(u32 idx) {
    s32 value;

    D_80092608 = idx < 1;
    if (idx == 0) {
        func_8008EB4C(0x37);
        D_80098698 = 2;
        value = 0xB4;
    } else {
        D_80098698 = 4;
        value = 0x9A;
    }
    D_80098692 = value;
    func_80070F80(D_8009105C[idx]);
    D_800925E8 = 0xA0;
    D_800925E0 = 0xA0;
    D_800925EC = 0x6D;
    D_800925E4 = 0x6D;
    D_80092600 = 0;
    D_800987E0 = D_800987E8;
    D_800970C4 = D_800970CC;
}
*/

extern u8 D_80099D9D;
extern u8 D_80099D9E;
extern s32 D_800928C8;
extern u8 D_80092884;
extern u8* D_80092880;
extern u8 D_800929BC;
extern s32 D_800925DC;
extern u8 D_80092604;
extern s32 D_80092904;
extern s32 D_80092900;
extern u8 D_80090F38[];
extern u8* GetStringEntry(u8* pData, s32 index);
extern void SystemStringPushBuffer(void* pString, u8* pEntry);

void func_800719F0(void) {
    D_80099D9D = 0;
    D_80099D9E = 0;
    func_80083C0C(7);
    D_800928C8 = 5;
    D_80092884 = 0;
    func_80032F54(&D_80092954[0], 0x140, 0x70, 0xA2, 0x2A, 0x1C, 8);
    SystemStringPushBuffer(&D_80092954[0], GetStringEntry(D_80092880, 0x42));
    D_800929BC = 0x1E;
    D_800925DC = 0;
    func_80070F80(&D_80090F38[0]);
    D_80092604 = 0;
    D_80092904 = 0;
    D_80092900 = 0;
    func_8007191C(9);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071AD0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071DA4);

extern s32 D_8009284C;
extern s32 D_80092948;
extern u8 D_8009872C[];
extern u8 D_80097010[];
extern void func_80039C4C(s32 arg);
extern void func_80039FF8(void);
extern void func_80071DA4(u8* p);

/* 91.77%: case 3 is the only difference. retail materialises &D_80098720 into
   $v1 and stores through it twice (lui v1/%hi; addiu v1/%lo; sw v0,0(v1));
   ours keeps both stores as direct absolute references, which maspsx expands to
   lui $at/%hi; sw %lo($at), and drops the load for the compare (the value is
   already in $v0, as in retail). Tried: a pointer local, pTPage-style field
   access, a s32 value local, and ((s32*)&D_8009871C)[1] so the address is a
   symbol plus a constant offset - gcc folds every one of them back into a direct
   symbol store.
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071F8C);

/* 91.77% attempt:
void func_80071F8C(s32 mode) {
    switch (mode) {
    case 0:
        func_80039C4C(D_80092948);
        func_80039FF8();
        break;
    case 1:
        func_80071DA4(&D_8009872C[0]);
        if (D_800970C4 * 0x100 / D_800970CC < D_800987E0 * 0x100 / D_800987E8) {
            D_800925D4 = 0x43;
        } else {
            D_800925D4 = 0x44;
        }
        break;
    case 2:
        func_80071DA4(&D_80097010[0]);
        break;
    case 3:
        D_80098720 = -D_8009284C;
        if (D_80098720 < -0x600) {
            D_80098720 = -0x600;
        }
        break;
    }
}
*/


extern u8 D_8009293C;

void func_800720C4(void) {
    D_8009293C = 0;
}

extern u8 D_800910C4;
extern u8 D_800928D4;
extern u8 D_80099D9D;
extern u8 D_80099D9E;
extern s32 D_80092900;
extern s32 D_80092A00;
extern s32 D_80092A10;
extern s32 D_80092A20;
extern s32 D_800970F8;
extern s32 D_80098814;

void func_800720D4(void) {
    if (D_8009293C != 0) {
        return;
    }
    func_80070F80((s32)&D_800910C4);
    D_8009293C = 1;
    D_800928D4 = 0;
    D_80099D9D = 0;
    D_80099D9E = 0;
    D_80092900 = 0;
    D_800925D4 = 0;
    D_800925D8 = 0;
    D_80092A00 = 1;
    D_80092A10 = 1;
    D_80092A20 = 1;
    D_800970F8 = 0;
    D_80098814 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80072170);

void func_800725A8(void) {
}

extern void* BattleCreateOrderingTablePair(s32 a0);
extern void* BattleCreateLightGroup(void* p);
extern void* func_8008C2C0(s32 a0);
extern void func_80078F00(void* p);
extern s16 D_800928D0;
extern void* D_80092614;
extern s32 D_80096FB0;
extern s32 D_80096FAC;
extern s32 D_80096FA8;
extern u8 D_80092618;
extern s32 D_8009261C;
extern s32 D_80092620;
extern s32 D_80092624;
extern s32 D_80092628;
extern s32 D_8009262C;
extern s16 D_80092632;
extern void* D_800910F0;
extern void* D_80092610;

void func_800725B0(u8* p) {
    u8* pSrc;
    void* pAnim;
    s32 id;
    s32 x;
    s32 y;
    s32 z;
    s32 w;
    s32 u;
    u16 v;

    func_80030988(5, 4, 0x40, 0x40);
    pAnim = BattleCreateLightGroup(BattleCreateOrderingTablePair(0x10));
    id = *(s32*)(p + 0x5C);
    D_800910F0 = pAnim;
    D_80092610 = func_8008C2C0(id);
    func_8008976C(0x280, 0xDA);
    SetGeomScreen(0x400);
    D_800928D0 = 0;
    D_80092614 = p;
    func_80078F00(p);
    D_80096FB0 = 0;
    D_80096FAC = 0;
    D_80096FA8 = 0;
    pSrc = *(u8**)(p + 0x8FC);
    x = *(s16*)(pSrc + 0x14);
    y = *(s16*)(pSrc + 0x16);
    z = *(s16*)(pSrc + 0x18);
    w = *(s16*)(pSrc + 0x1A);
    u = *(s16*)(pSrc + 0x1C);
    v = *(u16*)(pSrc + 0x1E);
    D_80092618 = 1;
    D_8009261C = x;
    D_80092620 = y;
    D_80092624 = z;
    D_80092628 = w;
    D_8009262C = u;
    D_80092632 = v;
}

extern void func_8008BC04(void);
extern void func_8007F834(void);
extern void func_80030988(s32 a0, s32 a1, s32 a2, s32 a3);
extern void BattleFreeLightGroup(void* p);
extern void BattleFreeNodeTree(void* p);
extern void func_8008976C(s32 a0, s32 a1);
extern void func_80083C0C(s32 mode);
extern void func_8007E954(s32 value);
extern void func_80080D10(void);
extern void* D_800910F0;
extern void* D_80092610;

void func_800726B4(void) {
    func_8008BC04();
    func_8007F834();
    func_80030988(1, 1, 0x40, 0x40);
    BattleFreeLightGroup(D_800910F0);
    BattleFreeNodeTree(D_80092610);
    func_8008976C(0x140, 0xDA);
    SetGeomScreen(0xC0);
    func_80083C0C(3);
    func_8007E954(0x100);
    func_80080D10();
}

extern MATRIX D_80091C0C;

void func_8007273C(u8* p, MATRIX* pIn, MATRIX* pOut) {
    MATRIX mtx;

    *pOut = *pIn;
    mtx = D_80091C0C;
    mtx.t[0] = *(s32*)(p + 0x34) - D_80096FA8;
    mtx.t[1] = -D_80096FAC;
    mtx.t[2] = *(s32*)(p + 0x3C) - D_80096FB0;
    CompMatrix(pIn, &mtx, &mtx);
    pOut->t[0] = mtx.t[0];
    pOut->t[1] = mtx.t[1];
    pOut->t[2] = mtx.t[2];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80072858);
