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
	dcache_line_size r2, r3
	sub 	r3, r2, #1
	tst	r1, r3			// end cache line aligned?
	bic	r1, r1, r3
	beq 	1f
	// DCCIMVAC
	mcr	p15, 0, r1, c7, c14, 1	// clean + invalidate end cache line
1:	tst	r0, r3			// start cache line aligned?
	bic	r0, r0, r3
	beq	2f
	mcr	p15, 0, r0, c7, c14, 1	// clean + invalidate start cache line
	b	3f
	// DCIMVAC
2:	mcr	p15, 0, r0, c7, c6, 1	// invalidate current cache line
3:	add	r0, r0, r2
	cmp	r0, r1
	blo	2b
	dsb	sy
	mov	pc, lr

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
	dcache_by_line_op dccmvac, sy, r0, r1, r2, r3
	mov	pc, lr

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
	// DCCMVAC
	mcr 	p15, 0, r0, c7, c10, 1
	dsb	sy
	mov	pc, lr

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
	// DCCIMVAC
	mcr 	p15, 0, r0, c7, c14, 1
	dsb	sy
	mov	pc, lr
