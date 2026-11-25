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

// BEST ATTEMPT Sprintf -- 98.87% (531 insns compiled vs 535 retail, 2124 vs 2140
// bytes; the diff prints 561 because the retail side counts reloc and label lines).
// Structure is retail's: PSY-Q 2.7.2 -O3, frame 0x248, s0-s6+ra in retail's slots
// (s3=pDst, s2=count, s1=p, s4=' ', s5='-', s6='+'), fmt at 0x210, width 0x214,
// prec 0x218, ap at 0x220, va area at 0x250, pFormat's home 0x24C. Every
// instruction of the body, both digit loops, all three pad loops and the copyOut
// tail are identical modulo branch targets; exactly FOUR instructions are missing.
// THE RESIDUAL, named: in the four arms whose vararg lands in `value` ('d'/'i',
// 'u', 'o', and the 'X'/'x' hex arm) retail emits the va_arg fetch BEFORE the
// flags load and therefore pays a load-delay nop, where this compile loads flags
// first and fills that slot. Retail .L8003FF04: lw v1,544(sp) / addiu v0,v1,4 /
// sw v0,544(sp) / lw a0,0(v1) / lw v1,528(sp) / nop / andi v0,v1,0x20; this
// compile: ... sw v1,544(sp) / lw v1,528(sp) / lw a0,0(v0) / andi v0,v1,0x20.
// The 'n' and 's' arms load flags first in BOTH builds, so it is not a rule.
// MEASURED DEAD (five scored runs, all byte-identical, so this is neither source
// order nor a flag): fmt.u.flags copied to a u32 local after the fetch; the same
// local read BEFORE the fetch; va_arg split into pArg = ap; ap = pArg + 4;
// value = *(s32*)pArg; a (u32) cast around va_arg; declaring `value` before
// c/count/len.
// THE MOVER IS sched1 (-fschedule-insns), NOT expand/local_alloc/reorg. -da
// dumps: f.i.rtl/cse/cse2/loop/flow/combine all hold retail's order (value load
// 503 before flags load 506); f.i.sched (schedule_insns, which toplev.c runs
// BEFORE local_alloc) holds the swap, and it survives lreg/greg/sched2/dbr.
// sched.c rank_for_schedule: INSN_PRIORITY first, then dependence class, then
// LUID. priority = max over in-block successors of (prio + insn_cost - 1), and
// with the mips 2-cycle load latency the flags load scores 2 (it feeds and->
// branch, both in this block) while the value load scores 1 (its only consumer,
// the sll that does the (s16) cast, sits in the beqz delay slot, i.e. the next
// block). Retail therefore needs an RTL in which the value load has an in-block
// successor, which none of the five spellings produce.
// NOTE -fno-schedule-insns CANNOT be used to test this: toplev.c sets
// flag_schedule_insns = 1 in its `if (optimize >= 2)` block AFTER option
// processing, so -O3 overrides it. Every 'scheduler off' measurement in earlier
// notes of this park was therefore taken with sched1 still running.
// permute.sh --minutes 4: PERMUTER_BETTER base=674 best=522, and the candidate is
// wrong-shaped code (it wraps the fetch in a redundant if (fmt.u.flags) and
// duplicates it), so it was not ported.
// FIXED THIS ROUND: p = &work[0x200] moved ABOVE the LEFT-flag clear (98.46 ->
// 98.87). Retail forms addiu s1,sp,0x210 in the beqz delay slot of that if, which
// only happens when the statement precedes it.
// WHAT MADE THE EARLIER JUMPS, in order (each measured): 75.75 -> 87.64 came from
// the TU split already in config/slus_006.64.yaml (memmove/strlen/memchr no longer
// inlined, the three jals appear); 87.64 -> 90.12 from s32 c instead of char c
// (kills the andi 0xff on every format-character compare); 90.12 -> 96.60 from
// reading varargs through an ADDRESS-TAKEN va_list -- va_arg(*((va_list*)&ap), T)
// -- which puts ap in memory at 0x220 with sp-relative accesses and lets
// count/pDst/p stay in s2/s3/s1 instead of needing a ninth saved register;
// 96.60 -> 96.83 from SPRINTF_WORK_SIZE 0x200, not 0x210: the 0x10 at the bottom of
// retail's frame is the o32 outgoing-args block, so work starts at sp+0x10 and
// &work[0x200] == &fmt == sp+0x210; 96.83 -> 98.46 from writing the '%s' arms as
// "else if (!PRECSET) strlen else memchr" so strlen is the fall-through block, as
// in retail (memchr taken instead gave the mirrored layout).
// TRIED AND REJECTED: char c (87.64), volatile va_list (88.86, too many insns),
// gcc 2.6.0 -O3 (95.62), gcc 2.7.2-cdk (compile-failed), -O2 (90.12, measured on
// the 90.12 body), u32 local for flags, va_arg split into statements, (u32) cast
// around va_arg, value declared first. twin_for.py: no twin.
// parked
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
}


