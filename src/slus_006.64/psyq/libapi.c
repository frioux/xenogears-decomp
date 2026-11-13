#include "common.h"
#include "psyq/kernel.h"

typedef struct {
    u16 rootCounter;
    s16 unk2;
    s16 mode;
    s16 : 16;
    s16 target;
    s32 : 32;
} Counter;

extern volatile long* g_pInterruptStatusRegister;
extern volatile Counter* g_pRCounters;
extern volatile long g_InterruptStatusMasks[4];

extern long D_80056414;
extern u_char *D_80056418;
extern u_long *D_8005641C;
extern u_long D_8005A200;
extern u_long D_8005A204;
extern u_long D_8005A208;
extern u_long D_8005A20C;

extern int func_8004092C(void);
extern int func_800409AC(void);
extern int func_800409E4(void);
extern int func_80040A4C(void);
extern void func_80040ABC(char *buf1, long len1, char *buf2, long len2);
extern void func_80040B00(void);
extern void func_80040BA4(void);
extern void _remove_ChgclrPAD(char *buf1, long len1, char *buf2, long len2);
extern void _patch_pad(void);
extern void InitPAD2(char *buf1, long len1, char *buf2, long len2);
extern void StartPAD2(void);
extern void StopPAD2(void);
extern int SysEnqIntRP(int prio, u_long *pEntry);
extern int SysDeqIntRP(int prio, u_long *pEntry);

/* The A0/C0 BIOS vector stubs of this unit (ChangeClearPAD, InitPAD2, StartPAD2,
   StopPAD2, func_80040ABC, SysEnqIntRP, SysDeqIntRP) cannot be C: retail leaves
   an instruction after .size (convertible.py: BLOCKED), and the body is
   `jr $t2` through vector 0xB0/0xC0 with the function index in $t1, which gcc
   never emits -- it passes argument 0 in $a0. They belong in a hasm segment. */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", ChangeClearPAD);

long SetRCnt(long spec, short target, long mode) {
    int i = spec & 0xFFFF;
    int final_mode = 0x48;

    if (i >= 3)
        return 0;

    g_pRCounters[i].mode = 0;
    g_pRCounters[i].target = target;

    if (i < 2U) {
        if (mode & RCntMdGATE) final_mode = 0x49;
        if (!(mode & RCntMdSC)) final_mode |= 0x100;
    } else if (i == 2U) {
        if (!(mode & RCntMdSC)) final_mode = 0x248;
    }

    if ((mode & RCntMdINTR) != 0)
        final_mode |= 0x10;

    g_pRCounters[i].mode = final_mode;
    return 1;
}

long GetRCnt(long spec) {
    int i = spec & 0xFFFF;
    if (i >= 3)
        return 0;
    return g_pRCounters[i].rootCounter;
}

long StartRCnt(long spec) {
    int i = spec & 0xFFFF;
    long* mask = g_InterruptStatusMasks;

    g_pInterruptStatusRegister[1] |= mask[i];
    return i < 3;
}

long StopRCnt(long spec) {
    int i = spec & 0xFFFF;
    long* mask = g_InterruptStatusMasks;
    g_pInterruptStatusRegister[1] &= ~mask[i];
    return 1;
}

long ResetRCnt(long spec) {
    int i = spec & 0xFFFF;
    if (i >= 3)
        return 0;
    g_pRCounters[i].rootCounter = 0;
    return 1;
}

void func_8004076C(long value) {
    D_80056414 = value;
}

long func_8004077C(void) {
    return D_80056414;
}

long func_8004078C(char *buf1, long len1, char *buf2, long len2) {
    _remove_ChgclrPAD(buf1, len1, buf2, len2);
    EnterCriticalSection();
    _patch_pad();
    ExitCriticalSection();
    ChangeClearPAD(0);
    func_8004092C();
    func_80040ABC(buf1, len1, buf2, len2);
    func_80040BA4();
    D_80056414 = 1;
    return 1;
}

long InitPAD(char *buf1, long len1, char *buf2, long len2) {
    _remove_ChgclrPAD(buf1, len1, buf2, len2);
    EnterCriticalSection();
    _patch_pad();
    ExitCriticalSection();
    ChangeClearPAD(0);
    func_8004092C();
    InitPAD2(buf1, len1, buf2, len2);
    func_80040BA4();
    D_80056414 = 1;
    return 1;
}

long StartPAD(void) {
    StartPAD2();
    ChangeClearPAD(0);
    EnablePAD();
}

void func_800408F4(void) {
    func_80040B00();
    StopPAD2();
    func_800409AC();
    D_80056414 = 0;
}

int func_8004092C(void) {
    u_long *pEntry;

    EnterCriticalSection();
    // the entry handed to the interrupt handler starts 4 bytes before the handlers
    pEntry = &D_8005A204;
    pEntry[0] = (u_long)func_800409E4;
    D_8005A208 = (u_long)func_80040A4C;
    D_8005A200 = 0;
    D_8005A20C = 0;
    SysDeqIntRP(1, pEntry - 1);
    SysEnqIntRP(1, pEntry - 1);
    ExitCriticalSection();
    return 1;
}

int func_800409AC(void) {
    EnterCriticalSection();
    SysDeqIntRP(1, &D_8005A200);
    ExitCriticalSection();
    return 1;
}

int func_800409E4(void) {
    volatile int nDelay;
    s32 pad[2]; // unused: reproduces the original's 0x8-byte frame

    *(u_short*)(D_80056418 + 0xA) = 0;
    for (nDelay = 10; --nDelay != -1;)
        ;
    return 0;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", func_80040A4C);
/*
81.25%: the first test already matches (beqz to the epilogue with `move v0,zero`
in its delay slot). Only the tail differs: the original materialises the 0/1 by
branch (bnez past `li v0,1`, then a dead `move v0,zero`), while gcc returns the
masked value itself (`andi v0,v0,1`), because the condition is known to be 0..1.
Tried, all identical: nested if, if/else-if/else, goto, %2, `int ret` plus one
return, do/while, `&&` bare and via a local, bitfield struct, inverted (two
`return 0` then `return 1`), long/1L return type; also gcc-2.6.0 and -O2.
int func_80040A4C(void) {
    u_long *pEntry = D_8005641C;

    if ((pEntry[1] & 1) == 0)
        return 0;
    if (pEntry[0] & 1)
        return 1;
    return 0;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", InitPAD2);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", StartPAD2);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", StopPAD2);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", func_80040ABC);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", SysEnqIntRP);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", SysDeqIntRP);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", EnablePAD);
/*
Best 0%: a hand-written tail jump through the patch hook in .bss
(lui/lw into t1, jr t1, no $ra save). gcc 2.7.2 will not sibcall through a
function pointer -- every C spelling tested gives a full frame plus jalr.
Re-confirmed: `((void (*)(void))jtbl_800593AC)();` gives lui/lw v0, addiu sp,-24,
sw ra, jalr v0, lw ra, addiu sp,24, jr ra -- no sibling call is even attempted
for an indirect callee, so no C spelling can reach this. Target is retail asm.
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", func_80040B00);
/* Same blocker as EnablePAD: hand-written tail jump through D_800593B0; gcc makes
   a 24-byte frame plus jalr (0%). */

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", _patch_pad);
