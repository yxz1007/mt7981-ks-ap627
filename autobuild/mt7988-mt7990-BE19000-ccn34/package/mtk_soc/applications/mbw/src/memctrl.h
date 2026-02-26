#ifndef __MEMORY_CONTROL__
#define __MEMORY_CONTROL__

#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <time.h>

#define TRUE 1
#define FALSE 0

unsigned long EMI_IO_BASE;

extern long EMI_REG_BASE;

#define REG_EMI_BMEN	  (EMI_REG_BASE + 0x400)
#define REG_EMI_MSEL	  (EMI_REG_BASE + 0x440)
#define REG_EMI_MSEL2	  (EMI_REG_BASE + 0x468)
#define REG_EMI_MSEL3	  (EMI_REG_BASE + 0x470)
#define REG_EMI_MSEL4	  (EMI_REG_BASE + 0x478)

#define REG_EMI_BSTP	  (EMI_REG_BASE + 0x404)

#define REG_EMI_CONM      (EMI_REG_BASE + 0x060)
#define REG_EMI_CONN      (EMI_REG_BASE + 0x068)

#define REG_EMI_BMEN1	  (EMI_REG_BASE + 0x4e0)
#define REG_EMI_BMEN2	  (EMI_REG_BASE + 0x4e8)

#define REG_EMI_EMITYPE1  (EMI_REG_BASE + 0x500)
#define REG_EMI_EMITYPE2  (EMI_REG_BASE + 0x508)
#define REG_EMI_EMITYPE3  (EMI_REG_BASE + 0x510)
#define REG_EMI_EMITYPE4  (EMI_REG_BASE + 0x518)
#define REG_EMI_EMITYPE5  (EMI_REG_BASE + 0x520)
#define REG_EMI_EMITYPE6  (EMI_REG_BASE + 0x528)
#define REG_EMI_EMITYPE7  (EMI_REG_BASE + 0x530)
#define REG_EMI_EMITYPE8  (EMI_REG_BASE + 0x538)

#define REG_EMI_EMITYPE9  (EMI_REG_BASE + 0x540)
#define REG_EMI_EMITYPE10 (EMI_REG_BASE + 0x548)
#define REG_EMI_EMITYPE11 (EMI_REG_BASE + 0x550)
#define REG_EMI_EMITYPE12 (EMI_REG_BASE + 0x558)
#define REG_EMI_EMITYPE13 (EMI_REG_BASE + 0x560)
#define REG_EMI_EMITYPE14 (EMI_REG_BASE + 0x568)
#define REG_EMI_EMITYPE15 (EMI_REG_BASE + 0x570)
#define REG_EMI_EMITYPE16 (EMI_REG_BASE + 0x578)

#define REG_EMI_EMITYPE17 (EMI_REG_BASE + 0x580)
#define REG_EMI_EMITYPE18 (EMI_REG_BASE + 0x588)
#define REG_EMI_EMITYPE19 (EMI_REG_BASE + 0x590)
#define REG_EMI_EMITYPE20 (EMI_REG_BASE + 0x598)
#define REG_EMI_EMITYPE21 (EMI_REG_BASE + 0x5a0)

#define REG_EMI_WACT      (EMI_REG_BASE + 0x420)

#define REG_EMI_TSCT      (EMI_REG_BASE + 0x418)
#define REG_EMI_TSCT2     (EMI_REG_BASE + 0x448)
#define REG_EMI_TSCT3     (EMI_REG_BASE + 0x450)


#define REG_EMI_WSCT      (EMI_REG_BASE + 0x428)
#define REG_EMI_WSCT2     (EMI_REG_BASE + 0x458)
#define REG_EMI_WSCT3     (EMI_REG_BASE + 0x460)
#define REG_EMI_WSCT4     (EMI_REG_BASE + 0x464)


#define REG_EMI_BMID0     (EMI_REG_BASE + 0x4b0)
#define REG_EMI_BMID1     (EMI_REG_BASE + 0x4b4)
#define REG_EMI_BMID2     (EMI_REG_BASE + 0x4b8)
#define REG_EMI_BMID3     (EMI_REG_BASE + 0x4bc)
#define REG_EMI_BMID4     (EMI_REG_BASE + 0x4c0)
#define REG_EMI_BMID5     (EMI_REG_BASE + 0x4c4)
#define REG_EMI_BMID6     (EMI_REG_BASE + 0x5c8)
#define REG_EMI_BMID7     (EMI_REG_BASE + 0x5cc)
#define REG_EMI_BMID8     (EMI_REG_BASE + 0x5d0)
#define REG_EMI_BMID9     (EMI_REG_BASE + 0x5d4)
#define REG_EMI_BMID10    (EMI_REG_BASE + 0x5d8)

#define REG_EMI_DBWA    (EMI_REG_BASE + 0xF00)
#define REG_EMI_DBWB    (EMI_REG_BASE + 0xF04)
#define REG_EMI_DBWC    (EMI_REG_BASE + 0xF08)
#define REG_EMI_DBWD    (EMI_REG_BASE + 0xF0C)
#define REG_EMI_DBWE    (EMI_REG_BASE + 0xF10)
#define REG_EMI_DBWF    (EMI_REG_BASE + 0xF14)

#define REG_EMI_DBWG    (EMI_REG_BASE + 0xF18)
#define REG_EMI_DBWH    (EMI_REG_BASE + 0xF1C)

#define REG_EMI_DBWA_2ND    (EMI_REG_BASE + 0xF2C)
#define REG_EMI_DBWB_2ND    (EMI_REG_BASE + 0xF30)
#define REG_EMI_DBWC_2ND    (EMI_REG_BASE + 0xF34)
#define REG_EMI_DBWD_2ND    (EMI_REG_BASE + 0xF38)
#define REG_EMI_DBWE_2ND    (EMI_REG_BASE + 0xF3C)
#define REG_EMI_DBWF_2ND    (EMI_REG_BASE + 0xF40)
#endif
