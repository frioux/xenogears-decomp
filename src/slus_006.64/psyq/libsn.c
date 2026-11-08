#include "common.h"

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCopen);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCclose);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PClseek);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCcreate);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCinit);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", PCread);
/*
int PCread(int fd, char *buff, int len) {
    int nChunk;
    int nRead;
    int nTotal;

    nTotal = 0;
    while (len != 0) {
        nChunk = len;
        if ((u_int)len > 0x8000)
            nChunk = 0x8000;
        nRead = func_8004C458(0, fd, nChunk, buff);
        nTotal += nRead;
        if (nRead == -1)
            return -1;
        buff += nRead;
        len -= nRead;
        if (nRead < nChunk)
            break;
    }
    return nTotal;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libsn", func_8004C458);

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
