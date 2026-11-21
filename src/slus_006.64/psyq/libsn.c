#include "common.h"

/* The file-server entry points are `break` calls into the dev-kit debug stub:
   the stub takes its arguments in a1..a3 (a0 unused) and answers with a status
   in v0 and the result in v1, which is why each wrapper shifts its arguments
   down one register and turns a non-zero status into -1.

   Retail's tail is `beqz v0,L; addu v0,v1,zero; addiu v0,zero,-1; L: jr ra`:
   the select with the value copy in the branch's own delay slot. gcc lays it
   out that way only when the status is compared against a local holding 0 and
   the -1 arm ends in a dead `do { } while` loop; without the loop, jump
   optimisation inverts the branch and the copy becomes the fall-through
   (`bnez v0; li v0,-1; move v0,v1`), the same six words in the other order.
   PCcreate and _SN_read below are matched that way; PCopen and PClseek are the
   same six words and their parked bodies below predate the find. */
/* 84.38%: the argument shifting matches and the branch is the right way round
   (`beqz v0`); only the tail differs. Retail is `beqz v0,L; addu v0,v1,zero;
   addiu v0,zero,-1; L: jr ra; nop` - the value copy sits in the delay slot of a
   branch on the very register it writes, which is sound only because the -1 arm
   overwrites v0. gcc 2.7.2 will not put a clobber of a branch operand into that
   branch's own delay slot, so it emits one of two other shapes instead:
   `beqz v0; nop; li v1,-1; jr ra; move v0,v1` (the copy moves out to the
   return) or `bnez v0; li v0,-1; move v0,v1; jr ra` (the constant becomes the
   unconditional default). Both are 8 words too. About 30 hand shapes measured
   (two returns, conditional expression, if-else both ways, goto join, switch,
   do-while, separate result variable, every combination of hard-binding status,
   value and ret), identical under gcc 2.6.0, 2.7.2-cdk, -O2 and -O3; permuter
   3106 iterations from the honest body: best score 5, and the single differing
   word is always the branch's tested register. */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCopen);
/* int PCopen(char *name, int flags, int perms) {
    register int status asm("$2");
    register int value asm("$3");
    int ret;

    __asm__ volatile("move $6, %3\n\tmove $5, %2\n\tbreak 259"
                     : "=r"(status), "=r"(value) : "r"(name), "r"(flags));
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


int PCcreate(char *pathname, int mode) {
    register int status asm("$2");
    int value;
    int noError;

    __asm__ volatile("move $5, %2\n\tmove $6, $zero\n\tbreak 258"
                     : "=r"(status), "=r"(value) : "r"(pathname));
    noError = 0;
    if (status != noError) {
        return -1;
        do {
        } while (noError); /* dead: keeps the -1 arm as the fall-through */
    } else {
        return value;
    }
}

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
int _SN_read(int fd, int cmd, int nbytes, char *buff) {
    register int status asm("$2");
    int value;
    int noError;

    __asm__ volatile("break 261" : "=r"(status), "=r"(value));
    noError = 0;
    if (status != noError) {
        return -1;
        do {
        } while (noError); /* dead: keeps the -1 arm as the fall-through */
    } else {
        return value;
    }
}
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
