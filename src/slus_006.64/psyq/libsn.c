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

int PCread(int fd, char *buff, int len) {
    register char *pBuff asm("$19");
    int nLeft;
    int nTotal;
    int nChunk;
    int nWritten;

    pBuff = buff;
    nLeft = len;
    nTotal = 0;
    if (nLeft != 0) {
        do {
            nChunk = nLeft;
            if ((u_int)nLeft > 0x8000)
                nChunk = 0x8000;
            nWritten = _SN_read(0, fd, nChunk, pBuff);
            nTotal += nWritten;
            if (nWritten == -1)
                return -1;
            pBuff += nWritten;
            nLeft -= nWritten;
            if (nWritten < nChunk)
                break;
        } while (nLeft != 0);
    }
    return nTotal;
}

// 99.17%, decomp-permuter score 5 (base 165): retail is a select,
// `beqz v0,L; addu v0,v1,zero; addiu v0,zero,-1`. The body below gets the
// shape right - result in $2, `addu v0,v1,zero` sunk into the branch delay
// slot - by comparing an uninitialised local, and the only remaining
// difference is that the branch tests $a0, the register gcc picks for that
// dead local, instead of $v0 (status). Honest forms are all worse: plain
// `if (status != 0) ret = -1` is 79.17% (`li v1,-1`, copy at the return);
// if/else, ternary and early return 45-57%; `register int ret asm("$2")`
// emits the copy before the branch and needs a nop; letting status double as
// the result gives the mirrored `bnez` layout. Same 79.17% under gcc 2.6.0,
// 2.7.2-cdk, -O1, -O2. Permuter re-run from score 5 (1696 iters): NONE.
/*
int _SN_read(int fd, int cmd, int nbytes, char *buff) {
    register int status asm("$2");
    register int value asm("$3");
    int new_var;
    int ret;

    __asm__ volatile("break 261" : "=r"(status), "=r"(value));
    ret = value;
    if (new_var != 0) {
        new_var = status;
        ret = -1;
    }
    return ret;
}
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", _SN_read);

int PCwrite(int fd, char *buff, int len) {
    register char *pBuff asm("$19");
    int nLeft;
    int nTotal;
    int nChunk;
    int nWritten;

    pBuff = buff;
    nLeft = len;
    nTotal = 0;
    if (nLeft != 0) {
        do {
            nChunk = nLeft;
            if ((u_int)nLeft > 0x8000)
                nChunk = 0x8000;
            nWritten = func_8004C530(0, fd, nChunk, pBuff);
            nTotal += nWritten;
            if (nWritten == -1)
                return -1;
            pBuff += nWritten;
            nLeft -= nWritten;
            if (nWritten < nChunk)
                break;
        } while (nLeft != 0);
    }
    return nTotal;
}
