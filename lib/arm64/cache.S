/*
 * Based on arch/arm64/mm/cache.S
 *
 * Copyright (C) 2001 Deep Blue Solutions Ltd.
 * Copyright (C) 2012, 2022 ARM Ltd.
 *
 * This work is licensed under the terms of the GNU GPL, version 2.
 */

#include <asm/assembler.h>

/*
 *      dcache_inval_poc(start, end)
 *
 *      Ensure that any D-cache lines for the interval [start, end) are
 *      invalidated. Any partial lines at the ends of the interval are cleaned
 *      and invalidated to PoC instead to prevent data loss.
 *
 *      - start   - start address of region
 *      - end     - end address of region
 */
.global dcache_inval_poc
dcache_inval_poc:
	dmb	sy
	raw_dcache_line_size x2, x3
	sub 	x3, x2, #1
	tst	x1, x3			// end cache line aligned?
	bic	x1, x1, x3
	b.eq 	1f
	dc	civac, x1		// clean + invalidate end cache line
1:	tst	x0, x3			// start cache line aligned?
	bic	x0, x0, x3
	b.eq	2f
	dc 	civac, x0		// clean + invalidate start cache line
	b	3f
2:	dc	ivac, x0		// invalidate current cache line
3:	add	x0, x0, x2
	cmp	x0, x1
	b.lo	2b
	dsb	sy
	ret

/*
 *      dcache_clean_poc(start, end)
 *
 *      Ensure that any D-cache lines for the interval [start, end)
 *      are cleaned to the PoC.
 *
 *      - start   - start address of region
 *      - end     - end address of region
 */
.global dcache_clean_poc
dcache_clean_poc:
	dmb	sy
	dcache_by_line_op cvac, sy, x0, x1, x2, x3
	ret

/*
 *      dcache_clean_addr_poc(addr)
 *
 *      Ensure that the D-cache line for address addr is cleaned to the PoC.
 *
 *      - addr    - the address
 */
.global dcache_clean_addr_poc
dcache_clean_addr_poc:
	dmb	sy
	dc	cvac, x0
	dsb	sy
	ret

/*
 *      dcache_clean_inval_addr_poc(addr)
 *
 *      Ensure that the D-cache line for address addr is cleaned and invalidated
 *      to the PoC.
 *
 *      - addr    - the address
 */
.global dcache_clean_inval_addr_poc
dcache_clean_inval_addr_poc:
	dmb	sy
	dc	civac, x0
	dsb	sy
	ret
