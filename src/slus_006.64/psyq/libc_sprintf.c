#include "common.h"

#include "psyq/stdarg.h"

// Declarations only: these are DEFINED in the libc TU, so having them in this
// TU as definitions is what used to get them inlined away.
extern int strlen(char* pString);
extern void* memchr(u_char* pSrc, u_char ch, int size);
extern void* memmove(u_char* pDst, u_char* pSrc, int size);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libc_sprintf", Sprintf);

/* BEST ATTEMPT Sprintf -- 75.75% (545 insns compiled vs 561 retail, 2180 bytes).
   BLOCKER, measured, not a guess: memmove, strlen and memchr are DEFINED above
   Sprintf in this TU (address order), so gcc 2.7.2 -O3 inlines all three and the
   compiled body has ZERO jal where retail has three (jal memchr 0x80040300,
   jal strlen 0x800402EC, jal memmove 0x800403C0); with them inlined gcc also
   keeps pDst/count/p/len in t-registers instead of s3/s2/s1/s0, so the frame is
   0x220 instead of 0x248 and no s-register or ra save appears at all.
   PROOF the body itself is right: compiling the same text with only prototypes
   (tools/scripts/cc1_probe.sh slus_006.64/psyq/libc <snippet with extern
   strlen/memchr/memmove>) emits the three jal's, saves ra and s0-s6, and puts
   fmt at sp+0x210 with width/prec at +0x214/+0x218, exactly like retail; its
   frame is 0x258, 0x10 too big (the pFormat home went inside the frame instead
   of the incoming-arg area at sp+0x24C, which is what va_start(&pFormat) gives
   retail). A block-scope `extern void* memmove();` inside the body does NOT stop
   the inlining (tried; still 545 insns).
   FIX (needs gears.toml, so not done here): split Sprintf into its own TU --
   config/slus_006.64.yaml [0x300E8, c, psyq/libc_sprintf] + src/.../libc_sprintf.c
   with the same PSY-Q preset keyed per file -- then un-park this body verbatim
   and declare strlen/memchr/memmove extern. Only then can it be scored.
   Structure facts that the body already encodes (all read off the .s): frame
   0x248, va area at sp+0x250 (so &pFormat lands at 0x24C), fmt spec struct
   0xC bytes at 0x210 (flags word + sign byte 1 sharing the word, width 0x214,
   prec 0x218), ap at 0x220, digit scratch is the 0x210 bytes below the spec
   filled downward from &work[0x210]; switch (c) over 'L'..'x' is the 45-word
   jtbl at 0x9418 (the FIRST word of this unit's rodata range, so table offset is
   reproducible); conversions are L X c d i h l n o p s u x, no float, flags
   1 - 2 + 4 # 8 0 0x10 precset 0x20 h 0x40 l 0x80 L, '-' clears 0x8,
   %p = prec 8 + flags 0x50 then the 'X' arm, '#' on %o gives a leading '0',
   '#' on %x/%X gives the 0<case-char> prefix, '#' on %s reads a length byte,
   an unknown conversion ends the output (goto done), %n stores count and skips
   the copy-out.

0x210 bytes of scratch: digits and padding are written downward from the
   byte just below the format spec, then moved out with memmove.
#define SPRINTF_WORK_SIZE 0x210

#define SPRINTF_FLAG_LEFT    0x01
#define SPRINTF_FLAG_PLUS    0x02
#define SPRINTF_FLAG_ALT     0x04
#define SPRINTF_FLAG_ZERO    0x08
#define SPRINTF_FLAG_PRECSET 0x10
#define SPRINTF_FLAG_SHORT   0x20
#define SPRINTF_FLAG_LONG    0x40
#define SPRINTF_FLAG_LONGDBL 0x80

The flags and the sign/pad character share the spec's first word: the flags
   are read and rewritten whole, the character is byte 1 of the same word.
typedef struct {
    // 0x0 union {
        u32 flags;
        struct {
            u8 bits;
            char sign;
            u8 pad2[2];
        } byte;
    } u;
    // 0x4 s32 width;
    // 0x8 s32 prec;
} SprintfFormat; // Size: 0xC

extern SprintfFormat g_SprintfFormatDefault;
extern const char D_80018BF0[]; // .rodata, "0123456789ABCDEF"
extern const char D_80018C04[]; // .rodata, "0123456789abcdef"

int Sprintf(char* pDst, char* pFormat, ...) {
    char work[SPRINTF_WORK_SIZE];
    SprintfFormat fmt;
    va_list ap;
    char* p;
    const char* pDigit;
    char c;
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
            fmt.width = va_arg(ap, s32);
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
                fmt.prec = va_arg(ap, s32);
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
        if (fmt.u.flags & SPRINTF_FLAG_LEFT)
            fmt.u.flags &= ~SPRINTF_FLAG_ZERO;
        p = &work[SPRINTF_WORK_SIZE];

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
                value = va_arg(ap, s32);
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
                value = va_arg(ap, s32);
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
                value = va_arg(ap, s32);
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
                fall through

            case 'X':
                pDigit = D_80018BF0;
                goto hex;

            case 'x':
                pDigit = D_80018C04;

            hex:
                value = va_arg(ap, s32);
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
                *--p = (char) va_arg(ap, s32);
                len = 1;
                goto copyOut;

            case 's':
                p = va_arg(ap, char*);
                if (fmt.u.flags & SPRINTF_FLAG_ALT) {
                    len = *(u8*) p;
                    p++;
                    if ((fmt.u.flags & SPRINTF_FLAG_PRECSET) && fmt.prec < len)
                        len = fmt.prec;
                } else if (fmt.u.flags & SPRINTF_FLAG_PRECSET) {
                    char* pEnd;

                    pEnd = memchr(p, 0, fmt.prec);
                    if (pEnd != NULL)
                        len = pEnd - p;
                    else
                        len = fmt.prec;
                } else {
                    len = strlen(p);
                }
                goto copyOut;

            case 'n':
                p = va_arg(ap, char*);
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
*/

