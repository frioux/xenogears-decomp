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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libc", strcat);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libc", strcmp);

/* best: 56.12% -- loop body and block layout differ (original keeps the loaded
   byte in a2 and zero-extends it with andi for the compare; the mismatch path
   reloads *(p2 - 1) because p2++ lands in the beq delay slot)
int strcmp(u_char* pString1, u_char* pString2) {
    if (pString1 == NULL || pString2 == NULL) {
        if (pString1 != pString2) {
            if (pString1 != NULL)
                return 1;
            return -1;
        }
        return 0;
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
