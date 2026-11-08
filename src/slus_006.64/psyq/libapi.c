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
extern void func_80040C5C(char *buf1, long len1, char *buf2, long len2);
extern void _patch_pad(void);
extern void InitPAD2(char *buf1, long len1, char *buf2, long len2);
extern void StartPAD2(void);
extern void StopPAD2(void);
extern int SysEnqIntRP(int prio, u_long *pEntry);
extern int SysDeqIntRP(int prio, u_long *pEntry);

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
    func_80040C5C(buf1, len1, buf2, len2);
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
    func_80040C5C(buf1, len1, buf2, len2);
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", func_800409E4);
/*
int func_800409E4(void) {
    volatile int nDelay;

    *(u_short*)(D_80056418 + 0xA) = 0;
    for (nDelay = 10; --nDelay != -1; )
        ;
    return 0;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", func_80040A4C);
/*
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", func_80040B00);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi", _patch_pad);
