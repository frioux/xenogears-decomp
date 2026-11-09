#include "common.h"

/* The file-server entry points are `break` calls into the dev-kit debug stub:
   the stub takes its arguments in a1..a3 (a0 unused) and answers with a status
   in v0 and the result in v1, which is why each wrapper shifts its arguments
   down one register and turns a non-zero status into -1. */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCopen);
/* 84.38%: argument shifting matches (the moves must name $4/$5 directly, since
   "r" operands let gcc swap name/flags), and the branch is the right way round
   (beqz v0). Only the tail differs: the original copies the result first
   (`addu v0,v1,zero`, sunk into the branch delay slot) and then stores -1 into
   v0, so it needs the result variable live in v0 while gcc coalesces it with the
   break's value register (`li v1,-1; jr ra; move v0,v1`). Every shape tried puts
   the -1 arm in the delay slot instead: two returns, ?:, goto/join forms, a
   separate result variable, `register ... asm("$2")` on the result, and
   the same source under gcc 2.6.0 / -O2. */
/* int PCopen(char *name, int flags) {
    register int status asm("$2");
    int value, ret;

    __asm__ volatile("move $6, $5\n\tmove $5, $4\n\tbreak 259"
                     : "=r"(status), "=r"(value));
    ret = value;
    if (status != 0)
        ret = -1;
    return ret;
} */

void PCclose(int fd) {
    __asm__ volatile("move $5, %0\n\tbreak 260" : : "r"(fd));
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PClseek);
/* Same shape as PCopen but with three shifted arguments (84.38%).
int PClseek(int fd, int offset, int mode) {
    int status;
    int value;

    __asm__ volatile("move $5, %2\n\tmove $6, %3\n\tmove $7, %4\n\tbreak 263"
                     : "=r"(status), "=r"(value) : "r"(fd), "r"(offset), "r"(mode));
    if (status != 0)
        value = -1;
    return value;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCcreate);
/* Same shape as PCopen, second argument passed as zero (84.38%).
int PCcreate(char *pathname, int mode) {
    int status;
    int value;

    __asm__ volatile("move $5, %2\n\tmove $6, $zero\n\tbreak 258"
                     : "=r"(status), "=r"(value) : "r"(pathname));
    if (status != 0)
        value = -1;
    return value;
}
*/

int PCinit(void) {
    __asm__ volatile("break 257");
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCread);
/* 98.92%: only the register assignment of `nTotal` and `buff` is swapped
   (original keeps nTotal in s2 and buff in s3; every local order and a
   pBuff copy tried gives the swap). */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", _SN_read);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCwrite);
/*
int PCwrite(int fd, char *buff, int len) {
    int nChunk;
    int nWritten;
    int nTotal;

    nTotal = 0;
    if (len != 0) {
        do {
            nChunk = len;
            if ((u_int)len > 0x8000)
                nChunk = 0x8000;
            nWritten = func_8004C530(0, fd, nChunk, buff);
            nTotal += nWritten;
            if (nWritten == -1)
                return -1;
            buff += nWritten;
            len -= nWritten;
            if (nWritten < nChunk)
                break;
        } while (len != 0);
    }
    return nTotal;
}
*/
