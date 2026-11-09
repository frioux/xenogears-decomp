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

/* best: 83.28% -- only the loop body differs: retail tests equality first
   (beq over the mismatch), keeps the loaded byte at QImode for the zero test and
   zero-extends it with andi for the int compare, and increments p2 in the beq
   delay slot so the mismatch path reads *(p2 - 1). gcc rotates the loop the
   other way round here.
int strcmp(u_char* pString1, u_char* pString2) {
    if (pString1 == NULL || pString2 == NULL) {
        if (pString1 == pString2)
            return 0;
        if (pString1 == NULL)
            return -1;
        return 1;
    }
    while (*pString1 == *pString2) {
        if (*pString1 == 0)
            return 0;
        pString1++;
        pString2++;
    }
    return *pString1 - *pString2;
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libc", strcmp);

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libc", Sprintf);
