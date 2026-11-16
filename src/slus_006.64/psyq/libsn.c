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

/* 91.67%, decomp-permuter score 135 (base 185): argument shift and break match. Retail tail is a select, beqz v0 / addu v0,v1,zero / addiu v0,zero,-1; the body below zeroes value to stop gcc folding the test into value and gets beqz v0, but the -1 lands in v1 with the copy after jr (v1,zero then li v1,-1 then jr ra then move v0,v1). Same wall as PCcreate: retail stores -1 into the returned pseudo in v0 and copies v1 in the branch delay slot, which needs a real copy statement whose condition survives copy propagation. Tried: honest select, if/else, ternary, separate ret, -one temp. */
/* int PClseek(int fd, int offset, int mode) {
    int status;
    int value;
    int one;

    one = 1;
    __asm__ volatile("move $7, %4\n\tmove $6, %3\n\tmove $5, %2\n\tbreak 263"
                     : "=r"(status), "=r"(value) : "r"(fd), "r"(offset), "r"(mode));
    value = 0;
    if (status != value) {
        status = -one;
        value = status;
    }
    return value;
}
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PClseek);


/* 99.38%, decomp-permuter score 5 (base 165, then 2023 iters from 5: NONE): retail is a select, beqz v0, addu v0,v1,zero, addiu v0,zero,-1. The body below reproduces retail exactly except that the branch tests value ($v1, the copy source) instead of status ($v0). Retail needs the branch operand and the copy source in different registers, which needs a real copy statement (so the returned pseudo gets $v0) whose condition survives copy propagation - and gcc folds value==status into a branch on $v1. Every honest select form (ret=value then if, if/else, ternary, separate ret, register asm("$2")) is 84.38% or worse with li v1,-1 and the copy after jr. Semantic note: this body branches on value rather than status, which is not what retail does. */
/* int PCcreate(char *pathname, int mode) {
    int value;
    int status;

    __asm__ volatile("move $5, %2\n\tmove $6, $zero\n\tbreak 258"
                     : "=r"(status), "=r"(value) : "r"(pathname));
    value = status;
    status = value != 0;
    if (status)
        value = -1;
    status = value;
    return status;
}
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCcreate);

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
