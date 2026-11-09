#include "common.h"

#include "psyq/libapi.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard", _card_info);

void InitCARD(long val) {
    ChangeClearPAD(0);
    EnterCriticalSection();
    InitCARD2(val);
    func_8004E8D8();
    func_8004E990();
    ExitCriticalSection();
}

void StartCARD(void) {
    EnterCriticalSection();
    StartCARD2();
    ChangeClearPAD(0);
    ExitCriticalSection();
}

long StopCARD(void) {
    StopCARD2();
    func_8004E990();
    func_8004EA20();
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard", InitCARD2);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard", StartCARD2);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libcard", StopCARD2);

/*
Nothing left in this unit is convertible: the four remaining stubs are BIOS call
gates (`li t2,0xA0|0xB0; jr t2; li t1,<bios fn>`) that no C definition emits
(gcc never jumps indirect through t2), and each stub's .s also carries a pad
`nop` after `.size` that C would drop. tools/scripts/convertible.py marks all
four BLOCKED.
*/
