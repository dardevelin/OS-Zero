#ifndef __UNIT_IA32_BOOT_H__
#define __UNIT_IA32_BOOT_H__

#if !defined(__ASSEMBLY__)
#include <stdint.h>
#endif
#include <kern/conf.h>

#if !defined(__ASSEMBLY__)
/* RAM-size in bytes */
#define grubmemsz(hdr) ((1024 + (hdr)->himem) << 10)
#endif

#define MBMAGIC     0x1BADB002
#define MBPAGEALIGN (1 << 0)
#define MBMEMINFO   (1 << 1)
#if (VBE2)
#define MBVIDEOMODE (1 << 2)
#define MBFLAGS     (MBPAGEALIGN | MBMEMINFO | MBVIDEOMODE)
#else
#define MBFLAGS     (MBPAGEALIGN | MBMEMINFO)
#endif
#define MBCHKSUM    (-(MBMAGIC + MBFLAGS))

/* flags to select fields to fill */
#define GRUBMEM  (1 << 0)     /* lomem, himem */
#define GRUBDEV  (1 << 1)     /* bootdev */
#define GRUBCMD  (1 << 2)     /* cmdline */
#define GRUBMOD  (1 << 3)     /* modcnt, modadr */
#define GRUBSYM1 (1 << 4)     /* symbols */
#define GRUBSYM2 (1 << 5)
#define GRUBMAP  (1 << 6)     /* maplen, mapadr */
#define GRUBDRV  (1 << 7)     /* drvlen, drvadr */
#define GRUBCONF (1 << 8)     /* conftab */
#define GRUBLDR  (1 << 9)     /* bootldr */
#define GRUBAPM  (1 << 10)    /* apmtab */
#define GRUBVBE  (1 << 11)    /* VBE video extensions */

#if !defined(__ASSEMBLY__)
/* header structure to use in C code */
struct mboothdr {
    uint32_t flags;
    uint32_t lomem;
    uint32_t himem;
    uint32_t bootdev;
    uint32_t cmdline;
    uint32_t modcnt;
    uint32_t modadr;
    uint32_t syms[4];
    uint32_t maplen;
    uint32_t mapadr;
    uint32_t drvlen;
    uint32_t drvadr;
    uint32_t conftab;
    uint32_t bootldr;
    uint32_t apmtab;
    uint32_t vbectlinfo;
    uint32_t vbemodeinfo;
    uint32_t vbemode;
    uint32_t vbeseg;
    uint32_t vbeofs;
    uint32_t vbelen;
};
#endif

#define KERNSTKTOP   0x00080000
#define KERNSTKSIZE  8192

/* segment IDs */
#define NULLSEG      0
#define TEXTSEG      1
#define DATASEG      2
#define TSSSEG       3
#define UTEXTSEG     4
#define UDATASEG     5
#define CPUSEG       6
#define NGDT         7

/* segment selectors */
#define NULLSEL      0x0000
#define TEXTSEL      0x0008
#define DATASEL      0x0010
#define TSSSEL       0x0018
#define UTEXTSEL     0x0020
#define UDATASEL     0x0028
#define CPUSEL       0x0030

/* page size in bytes */
#define NBPG         4096

/* CR0 control register bits */
#define CR0PE        0x00000001
#define CR0WP        0x00010000
#define CR0PG        0x80000000

#if defined(__ASSEMBLY__)
#define SEG_EXEC     0x8
//#define SEG_GROWDOWN 0x4
#define SEG_CONFORM  0x4
#define SEG_WRITE    0x2
#define SEG_READ     0x2
#define SEG_ACCESS   0x1
#define SEG_ASM_NULL                                                    \
    .word 0, 0;                                                         \
    .byte 0, 0, 0, 0
#define SEG_ASM(type, base, lim)                                        \
    .word (((lim) >> 12) & 0xffff), ((base) & 0xffff);                  \
    .byte (((base) >> 16) & 0xff), (0x90 | (type)),                     \
        (0xc0 | (((lim) >> 28) & 0xf)), (((base) >> 24) & 0xff)
#endif

#endif /* __UNIT_IA32_BOOT_H__ */

