#include "common.h"

#include "psyq/stdarg.h"

// Declarations only: these are DEFINED in the libc TU, so having them in this
// TU as definitions is what gets them inlined away.
extern int strlen(char* pString);
extern void* memchr(u_char* pSrc, u_char ch, int size);
extern void* memmove(u_char* pDst, u_char* pSrc, int size);

#define SPRINTF_WORK_SIZE 0x200

#define SPRINTF_FLAG_LEFT    0x01
#define SPRINTF_FLAG_PLUS    0x02
#define SPRINTF_FLAG_ALT     0x04
#define SPRINTF_FLAG_ZERO    0x08
#define SPRINTF_FLAG_PRECSET 0x10
#define SPRINTF_FLAG_SHORT   0x20
#define SPRINTF_FLAG_LONG    0x40
#define SPRINTF_FLAG_LONGDBL 0x80

// The flags and the sign/pad character share the spec's first word: the flags
// are read and rewritten whole, the character is byte 1 of the same word.
typedef struct {
    union {
        /* 0x0 */ u32 flags;
        struct {
            /* 0x0 */ u8 bits;
            /* 0x1 */ char sign;
            /* 0x2 */ u8 pad2[2];
        } byte;
    } u;
    /* 0x4 */ s32 width;
    /* 0x8 */ s32 prec;
} SprintfFormat; // Size: 0xC

extern SprintfFormat g_SprintfFormatDefault;
extern const char D_80018BF0[]; // .rodata, "0123456789ABCDEF"
extern const char D_80018C04[]; // .rodata, "0123456789abcdef"

// PARKED Sprintf - BEST 99.78% (535 insns / 2140 bytes, retail's exact size).
// Every instruction of the body matches except ONE register tie, present in each of
// the four arms that fetch a vararg into `value` ('d'/'i', 'u', 'o', and the 'X'/'x'
// hex arm): retail loads ap into $v1 and computes ap+4 into $v0, then reuses $v1 for
// the flags load; this build loads ap into $v0 and ap+4 into $v1. The '*' width arm
// and the 'c', 's', 'n' arms already land on retail's registers, so what is left is
// allocator preference inside those four blocks - same opcodes, same size, 18
// operand rows in the objdiff dump.
// WHAT CLOSED 98.87 -> 99.78: wrapping each of those four fetches in its own
// `do { value = va_arg(*((va_list*)&ap), s32); } while (0);` gives the fetch block
// its own scheduling boundary, so sched1 can no longer hoist the flags load above
// the value load. Retail pays a load-delay nop between the flags load and the
// `andi 0x20` test and this body now pays the same one: those were exactly the four
// missing instructions. The barrier must hold the fetch AND NOTHING ELSE - moving
// the `if (fmt.u.flags & SPRINTF_FLAG_SHORT)` line inside it puts the flags load back
// in the same block and drops the body to 98.87% / 531 insns (measured).
// MEASURED DEAD on this 99.78 body, all byte-identical at 535 insns: `u32 value;`
// declared first among the locals; the fetch split into `s32* pArg` (pArg = (s32*)ap;
// ap = (char*)(pArg + 1); value = *pArg) inside the barrier; the fetch copied through
// a `u32 fetched` local inside the barrier.
// permute.sh --minutes 3 (1194 iterations): PERMUTER_NONE base=110 best=110 - it
// cannot move a local_alloc register tie. A 4-minute run on the earlier 98.87 body
// returned a candidate that duplicated the fetch (wrong code), not ported.
// HISTORY, each step measured; keep these spellings: 75.75 -> 87.64 came from the TU
// split already in config/slus_006.64.yaml (memmove/strlen/memchr stop being inlined);
// 87.64 -> 90.12 from `s32 c` instead of char c; 90.12 -> 96.60 from reading varargs
// through an ADDRESS-TAKEN va_list (`va_arg(*((va_list*)&ap), T)`), which keeps ap in
// memory at 0x220 and keeps count/pDst/p in saved registers; 96.60 -> 96.83 from
// SPRINTF_WORK_SIZE 0x200 (work starts at sp+0x10, so &work[0x200] == &fmt ==
// sp+0x210; the 0x10 at the bottom of the frame is the o32 outgoing-args block);
// 96.83 -> 98.46 from writing the '%s' arms as "else if (!PRECSET) strlen else memchr"
// so strlen is the fall-through block; 98.46 -> 98.87 from `p = &work[SPRINTF_WORK_SIZE];`
// above the LEFT-flag clear (retail forms addiu s1,sp,0x210 in that if's delay slot).
// REJECTED: char c (87.64), volatile va_list (88.86), gcc 2.6.0 -O3 (95.62),
// gcc 2.7.2-cdk (compile failed), -O2 (90.12), u32 local for the flags word, value
// declared before c/count/len. twin_for.py: no twin.
// DATA: this unit owns its .rodata range ([0x9418, .rodata, psyq/libc_sprintf], the
// 45-word switch table), and the stub's .s currently supplies that table, so a
// conversion must also be proven with tools/scripts/check_bin.sh slus_006.64.
/*
int Sprintf(char* pDst, char* pFormat, ...) {
    char work[SPRINTF_WORK_SIZE];
    SprintfFormat fmt;
    va_list ap;
    char* p;
    const char* pDigit;
    s32 c;
    s32 count;
    s32 len;
    u32 value;

    count = 0;
    va_start(ap, pFormat);
    c = *pFormat;
    while (c != 0) {
        if (c != '%') {
            pDst[count++] = c;
            goto next;
        }
        fmt = g_SprintfFormatDefault;
        for (;;) {
            c = *++pFormat;
            if (c == '-') {
                fmt.u.flags |= SPRINTF_FLAG_LEFT;
                continue;
            }
            if (c == '+') {
                fmt.u.flags |= SPRINTF_FLAG_PLUS;
                continue;
            }
            if (c == ' ') {
                fmt.u.byte.sign = ' ';
                continue;
            }
            if (c == '#') {
                fmt.u.flags |= SPRINTF_FLAG_ALT;
                continue;
            }
            if (c == '0') {
                fmt.u.flags |= SPRINTF_FLAG_ZERO;
                continue;
            }
            break;
        }
        if (c == '*') {
            fmt.width = va_arg(*((va_list*)&ap), s32);
            if (fmt.width < 0) {
                fmt.width = -fmt.width;
                fmt.u.flags |= SPRINTF_FLAG_LEFT;
            }
            c = *++pFormat;
        } else {
            while ((u32) (c - '0') < 10) {
                fmt.width = fmt.width * 10 + (c - '0');
                c = *++pFormat;
            }
        }
        if (c == '.') {
            c = *++pFormat;
            if (c == '*') {
                fmt.prec = va_arg(*((va_list*)&ap), s32);
                c = *++pFormat;
            } else {
                while ((u32) (c - '0') < 10) {
                    fmt.prec = fmt.prec * 10 + (c - '0');
                    c = *++pFormat;
                }
            }
            if (fmt.prec >= 0)
                fmt.u.flags |= SPRINTF_FLAG_PRECSET;
        }
        p = &work[SPRINTF_WORK_SIZE];
        if (fmt.u.flags & SPRINTF_FLAG_LEFT)
            fmt.u.flags &= ~SPRINTF_FLAG_ZERO;

    again:
        switch (c) {
            case 'h':
                fmt.u.flags |= SPRINTF_FLAG_SHORT;
                c = *++pFormat;
                goto again;

            case 'l':
                fmt.u.flags |= SPRINTF_FLAG_LONG;
                c = *++pFormat;
                goto again;

            case 'L':
                fmt.u.flags |= SPRINTF_FLAG_LONGDBL;
                c = *++pFormat;
                goto again;

            case 'd':
            case 'i':
                do {
                    value = va_arg(*((va_list*)&ap), s32);
                } while (0);
                if (fmt.u.flags & SPRINTF_FLAG_SHORT)
                    value = (s16) value;
                if ((s32) value < 0) {
                    value = -value;
                    fmt.u.byte.sign = '-';
                } else if (fmt.u.flags & SPRINTF_FLAG_PLUS) {
                    fmt.u.byte.sign = '+';
                }
                goto decimal;

            case 'u':
                do {
                    value = va_arg(*((va_list*)&ap), s32);
                } while (0);
                if (fmt.u.flags & SPRINTF_FLAG_SHORT)
                    value &= 0xFFFF;
                fmt.u.byte.sign = 0;

            decimal:
                if (!(fmt.u.flags & SPRINTF_FLAG_PRECSET)) {
                    if (fmt.u.flags & SPRINTF_FLAG_ZERO) {
                        fmt.prec = fmt.width;
                        if (fmt.u.byte.sign != 0)
                            fmt.prec--;
                    }
                    if (fmt.prec <= 0)
                        fmt.prec = 1;
                }
                len = 0;
                while (value != 0) {
                    *--p = value % 10 + '0';
                    value /= 10;
                    len++;
                }
                while (len < fmt.prec) {
                    *--p = '0';
                    len++;
                }
                if (fmt.u.byte.sign != 0) {
                    *--p = fmt.u.byte.sign;
                    len++;
                }
                goto copyOut;

            case 'o':
                do {
                    value = va_arg(*((va_list*)&ap), s32);
                } while (0);
                if (fmt.u.flags & SPRINTF_FLAG_SHORT)
                    value &= 0xFFFF;
                if (!(fmt.u.flags & SPRINTF_FLAG_PRECSET)) {
                    if (fmt.u.flags & SPRINTF_FLAG_ZERO)
                        fmt.prec = fmt.width;
                    if (fmt.prec <= 0)
                        fmt.prec = 1;
                }
                len = 0;
                while (value != 0) {
                    *--p = (value & 7) + '0';
                    value >>= 3;
                    len++;
                }
                if ((fmt.u.flags & SPRINTF_FLAG_ALT) && len != 0 && *p != '0') {
                    *--p = '0';
                    len++;
                }
                while (len < fmt.prec) {
                    *--p = '0';
                    len++;
                }
                goto copyOut;

            case 'p':
                fmt.prec = 8;
                fmt.u.flags |= SPRINTF_FLAG_PRECSET | SPRINTF_FLAG_LONG;
                // fall through

            case 'X':
                pDigit = D_80018BF0;
                goto hex;

            case 'x':
                pDigit = D_80018C04;

            hex:
                do {
                    value = va_arg(*((va_list*)&ap), s32);
                } while (0);
                if (fmt.u.flags & SPRINTF_FLAG_SHORT)
                    value &= 0xFFFF;
                if (!(fmt.u.flags & SPRINTF_FLAG_PRECSET)) {
                    if (fmt.u.flags & SPRINTF_FLAG_ZERO) {
                        fmt.prec = fmt.width;
                        if (fmt.u.flags & SPRINTF_FLAG_ALT)
                            fmt.prec -= 2;
                    }
                    if (fmt.prec <= 0)
                        fmt.prec = 1;
                }
                len = 0;
                while (value != 0) {
                    *--p = pDigit[value & 0xF];
                    value >>= 4;
                    len++;
                }
                while (len < fmt.prec) {
                    *--p = '0';
                    len++;
                }
                if (fmt.u.flags & SPRINTF_FLAG_ALT) {
                    *--p = c;
                    *--p = '0';
                    len += 2;
                }
                goto copyOut;

            case 'c':
                *--p = (char) va_arg(*((va_list*)&ap), s32);
                len = 1;
                goto copyOut;

            case 's':
                p = va_arg(*((va_list*)&ap), char*);
                if (fmt.u.flags & SPRINTF_FLAG_ALT) {
                    len = *(u8*) p;
                    p++;
                    if ((fmt.u.flags & SPRINTF_FLAG_PRECSET) && fmt.prec < len)
                        len = fmt.prec;
                } else if (!(fmt.u.flags & SPRINTF_FLAG_PRECSET)) {
                    len = strlen(p);
                } else {
                    char* pEnd;

                    pEnd = memchr(p, 0, fmt.prec);
                    if (pEnd != NULL)
                        len = pEnd - p;
                    else
                        len = fmt.prec;
                }
                goto copyOut;

            case 'n':
                p = va_arg(*((va_list*)&ap), char*);
                if (fmt.u.flags & SPRINTF_FLAG_SHORT)
                    *(s16*) p = count;
                else
                    *(s32*) p = count;
                goto next;

            default:
                if (c != '%')
                    goto done;
                pDst[count++] = c;
                goto next;
        }

    copyOut:
        if (len < fmt.width) {
            if (!(fmt.u.flags & SPRINTF_FLAG_LEFT)) {
                do {
                    pDst[count++] = ' ';
                } while (len < --fmt.width);
            }
        }
        memmove(pDst + count, p, len);
        count += len;
        if (len < fmt.width) {
            do {
                pDst[count++] = ' ';
            } while (++len < fmt.width);
        }

    next:
        c = *++pFormat;
    }

done:
    pDst[count] = 0;
    return count;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libc_sprintf", Sprintf);



