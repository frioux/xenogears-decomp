#include "common.h"

/* The file-server entry points are `break` calls into the dev-kit debug stub:
   the stub takes its arguments in a1..a3 (a0 unused) and answers with a status
   in v0 and the result in v1, which is why each wrapper shifts its arguments
   down one register and turns a non-zero status into -1. */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCopen);
/* 83.12%: the branch shape matches (beqz v0 to the merge point), but gcc puts
   the -1 in the `value` register and copies at the return
   (`li v1,-1; jr ra; move v0,v1`) instead of the original
   `move v0,v1; li v0,-1`, and it mis-assigns the two shifted operands.
int PCopen(char *name, int flags, int perms) {
    register char *pName asm("$4") = name;
    register int nFlags asm("$5") = flags;
    register int status asm("$2");
    register int value asm("$3");
    int result;

    __asm__ volatile("move $6, %2\n\tmove $5, %1\n\tbreak 259"
                     : "=r"(status), "=r"(value) : "r"(pName), "r"(nFlags));
    result = value;
    if (status != 0)
        result = -1;
    return result;
}
*/

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
