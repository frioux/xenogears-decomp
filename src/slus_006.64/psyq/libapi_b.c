#include "common.h"

extern long D_800593B8;
extern long D_80062640;
extern long D_80062644;

extern int DMACallback(int id, void (*pCallbackFn)());
extern void func_80040B7C();

void func_80040B7C(void) {
    int unused;

    ((void (*)(void))D_800593B8)();
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", func_80040BA4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", func_80040C20);

void func_80040C3C(void) {
    func_80040B7C();
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", func_80040C5C);

long func_80040CBC(long value) {
    long* pHook = &D_80062640;
    long old = *pHook;

    *pHook = value;
    return old;
}

long func_80040CD0(long value) {
    long* pHook = &D_80062644;
    long old = *pHook;

    *pHook = value;
    return old;
}

void* func_80040CE4(void (*pCallbackFn)()) {
    DMACallback(3, pCallbackFn);
}
