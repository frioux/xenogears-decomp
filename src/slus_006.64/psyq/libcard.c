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
