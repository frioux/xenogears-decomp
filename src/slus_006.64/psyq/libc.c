#include "common.h"

extern u_long g_RandomSeed;

void* bzero(u_char* pDst, int size) {
    void* pRet;

    if (pDst == NULL)
        return NULL;
    if (size <= 0)
        return NULL;
    pRet = pDst;
    for (; size > 0; size--) {
        *pDst++ = 0;
    }
    return pRet;
}

void* memchr(u_char* pSrc, u_char ch, int size) {
    if (pSrc == NULL)
        return NULL;
    if (size <= 0)
        return NULL;
    while (--size >= 0) {
        if (*pSrc++ == ch)
            return pSrc - 1;
    }
    return NULL;
}

void* memcpy(u_char* pDst, u_char* pSrc, int size) {
    void* pRet;

    if (pDst == NULL)
        return NULL;
    pRet = pDst;
    for (; size > 0; size--) {
        *pDst++ = *pSrc++;
    }
    return pRet;
}

void* memmove(u_char* pDst, u_char* pSrc, int size) {
    if (pDst >= pSrc) {
        while (size-- > 0) {
            pDst[size] = pSrc[size];
        }
    } else {
        while (size-- > 0) {
            *pDst++ = *pSrc++;
        }
    }

    return pDst;
}

void* memset(u_char* pDst, int ch, int size) {
    void* pRet;

    if (pDst == NULL)
        return NULL;
    if (size <= 0)
        return NULL;
    pRet = pDst;
    for (; size > 0; size--) {
        *pDst++ = ch;
    }
    return pRet;
}

int rand(void) {
    u_long nNext;

    nNext = (g_RandomSeed * 0x41C64E6D) + 0x3039;
    g_RandomSeed = nNext;
    return (nNext >> 0x10) & 0x7FFF;
}

void srand(u_long seed) {
    g_RandomSeed = seed;
}

char* strcat(char* pDst, char* pSrc) {
    char* pRet;

    if (pDst == NULL || pSrc == NULL)
        return NULL;
    if (pDst + strlen(pDst) != pSrc + strlen(pSrc)) {
        pRet = pDst;
        while (*pDst++)
            ;
        pDst--;
        while ((*pDst++ = *pSrc++))
            ;
        return pRet;
    }
    return NULL;
}


int strcmp(u_char* pString1, u_char* pString2) {
    if (pString1 == NULL || pString2 == NULL) {
        if (pString1 == pString2)
            return 0;
        if (pString1 == NULL)
            return -1;
        return 1;
    }
    while (*pString1 == *pString2++) {
        if (*pString1 == 0)
            return 0;
        pString1++;
    }
    return *pString1 - pString2[-1];
}


void* strcpy(u_char* pDst, u_char* pSrc) {
    void* pRet;

    if (pDst == NULL)
        return NULL;
    if (pSrc == NULL)
        return NULL;
    pRet = pDst;
    while ((*pDst++ = *pSrc++) != 0) {
        ;
    }
    return pRet;
}

int strlen(char* pString) {
    int nLen;
    char chCur;

    nLen = 0;
    if (pString == NULL)
        return 0;

    while (chCur = *pString, pString++, chCur != NULL)
        nLen++;
    return nLen;
}

/* PARKED Sprintf -- structure decoded, NO C body written. 535 insns, 67 blocks,
   and it owns a 45-word jump table; a partial body is worthless because the
   0x248 frame and the s0-s6 assignment depend on the whole body. Facts for a
   full-session attempt, all read off the .s:
   - convertible.py: OK (580 insns), but this stub's .s OWNS the unit rodata:
     jtbl_80018C18 is the FIRST word of the [0x9418, .rodata, psyq/libc] range
     in config/slus_006.64.yaml, 45 words spanning 0x9418-0x94CB. A C switch
     has to reproduce that table at that exact offset, and the rest of the
     unit rodata (0x94CC-0x969B) must not move.
   - frame addiu sp,-0x248; ra and s0-s6 saved at 0x244/0x240/0x23C/0x238/
     0x230/0x22C/0x228; a1/a2/a3 are spilled to 0x4/0x8/0xC(sp) BEFORE the
     frame is taken, so the vararg save area is sp+0x250 once the frame exists.
   - locals: 0x210 = flags (always read-modify-written as a WORD: lw/ori/sw),
     0x211 = the sign character (plain sb writes, lbu reads), 0x214 = width,
     0x218 = precision, 0x220 = the va_arg walk pointer (starts at sp+0x250,
     +4 per argument), 0x24C = the current format pointer, s3 = destination
     buffer, s2 = output count, s0/s1 = length and source pointer of the item
     just converted, s5 = '-', s6 = '+', s4 = ' '. The 12 bytes at 0x210 are
     re-initialised at every % by a 12-byte copy from the all-zero global
     D_800563F0 (file offset 0x46BF0, in 3F290.sdata.s), i.e. the source
     assigns a template spec struct; the flags word and the byte sign slot
     overlap, so the struct layout needs settling when the body is written.
   - flag bits: 1 '-' 2 '+' 4 '#' 8 '0' 0x10 = precision came from a negative
     '*' (i.e. no precision) 0x20 'h' 0x40 'l' 0x80 'L'. A '-' clears 0x8.
   - main loop is while (*fmt): a non- % byte is stored and counted; a %  parses
     the spec, formats into a scratch buffer, memmoves it to buf+count, pads
     left with spaces unless the '-' bit is set and right up to width, then
     advances fmt. End: buf[count] = 0; return count.
   - conversion dispatch is switch (c - 'L') with sltiu 0x2D, so 45 entries and
     every unlisted index goes to the unknown-conversion arm. Non-default case
     labels, in table order: 'L' (flags or 0x80), 'X', 'c', 'd', 'h' (flags or
     0x20), 'i' (shares the 'd' body), 'l' (flags or 0x40), 'n', 'o', 'p',
     's', 'u', 'x'. No float conversions at all. The whole body calls exactly
     three things: strlen (0x800402EC), memchr (0x80040300), memmove
     (0x800403C0).
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libc", Sprintf);
