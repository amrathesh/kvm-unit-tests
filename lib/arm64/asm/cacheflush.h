#ifndef _ASMARM64_CACHEFLUSH_H_
#define _ASMARM64_CACHEFLUSH_H_
/*
 * Based on arch/arm64/asm/include/cacheflush.h
 *
 * Copyright (C) 1999-2002 Russell King.
 * Copyright (C) 2012,2022 ARM Ltd.
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include <asm/page.h>

extern void dcache_clean_addr_poc(unsigned long addr);
/*
 * Invalidating a specific address is dangerous, because it means invalidating
 * everything that shares the same cache line. Do clean and invalidate instead,
 * as the clean is harmless.
 */
extern void dcache_clean_inval_addr_poc(unsigned long addr);

extern void dcache_inval_poc(unsigned long start, unsigned long end);
extern void dcache_clean_poc(unsigned long start, unsigned long end);

static inline void dcache_inval_page_poc(unsigned long page_addr)
{
	assert(PAGE_ALIGN(page_addr) == page_addr);
	dcache_inval_poc(page_addr, page_addr + PAGE_SIZE);
}

static inline void dcache_clean_page_poc(unsigned long page_addr)
{
	assert(PAGE_ALIGN(page_addr) == page_addr);
	dcache_clean_poc(page_addr, page_addr + PAGE_SIZE);
}

#endif /* _ASMARM64_CACHEFLUSH_H_ */
