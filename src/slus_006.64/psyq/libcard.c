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
Nothing left in this unit is convertible. The four remaining stubs are BIOS
vector gates of three instructions -- `addiu t2,$zero,<vector>`, `jr t2`,
`addiu t1,$zero,<function>` -- and retail has no epilogue after them, so a C
definition cannot be their source: gcc ends every function it compiles with
`jr ra; nop`, which is why a C body carrying those three words in an asm block
comes out 20 bytes instead of 12 (measured). `__attribute__((noreturn))` does
suppress the epilogue, but it makes gcc drop the code after InitCARD2,
StartCARD2 and StopCARD2 in this very file, which call them and then keep
working. Defining each symbol at file scope with `__asm__` does reproduce retail
byte for byte (objdiff 100.00%), but that is the shipped assembly moved into a
.c file rather than recovered C, so it is not a match; the honest form for these
is a `hasm` subsegment, as psyq/libapi/DeliverEvent is one.

Each stub's .s also carries one pad `nop` after `.size`: Psy-Q pads every
function to a 16-byte multiple and retail's linker kept the pad, so converting
one deletes that byte and shifts the exe. tools/scripts/convertible.py marks all
four BLOCKED. Sig: LIBCARD/C171.OBJ _card_info, A74/A75/A76 for the other three
(sig40.txt) -- InitCARD2/StartCARD2/StopCARD2 are assembly modules and _card_info,
from a C module, has the same shape, so Psy-Q spelled these without a C function.
*/
