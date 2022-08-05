/*
 * Copyright (C) 2014, Red Hat Inc, Andrew Jones <drjones@redhat.com>
 *
 * This work is licensed under the terms of the GNU LGPL, version 2.
 *
 * This is a simple allocator that provides contiguous physical addresses
 * with byte granularity.
 */
#include "alloc.h"
#include "asm/spinlock.h"
#include "asm/io.h"
#include "alloc_phys.h"

/*
 * used is the end address of the currently allocated memory, non-inclusive.
 * used equals top means that all memory has been allocated.
 */
static phys_addr_t base, used, top;

#define DEFAULT_MINIMUM_ALIGNMENT	32
static size_t align_min = DEFAULT_MINIMUM_ALIGNMENT;

static void *memalign_early(size_t alignment, size_t sz);
static struct alloc_ops early_alloc_ops = {
	.memalign = memalign_early,
};
struct alloc_ops *alloc_ops = &early_alloc_ops;

void phys_alloc_show(void)
{
	printf("phys_alloc minimum alignment: %#" PRIx64 "\n", (u64)align_min);
	printf("%016" PRIx64 "-%016" PRIx64 " [USED]\n", (u64)base, (u64)used);
	printf("%016" PRIx64 "-%016" PRIx64 " [FREE]\n", (u64)used, (u64)top);
}

void phys_alloc_init(phys_addr_t base_addr, phys_addr_t size)
{
	used = base = base_addr;
	top = base + size;
}

void phys_alloc_set_minimum_alignment(phys_addr_t align)
{
	assert(align && !(align & (align - 1)));
	align_min = align;
}

void phys_alloc_perform_cache_maintenance(cache_maint_fn maint_fn)
{
	maint_fn((unsigned long)&base);
	maint_fn((unsigned long)&used);
	maint_fn((unsigned long)&top);
	maint_fn((unsigned long)&align_min);
}

static void *memalign_early(size_t alignment, size_t sz)
{
	phys_addr_t align = (phys_addr_t)alignment;
	phys_addr_t size = (phys_addr_t)sz;
	phys_addr_t size_orig = size;
	phys_addr_t addr, top_safe;

	assert(align && !(align & (align - 1)));

	top_safe = top;
	if (sizeof(long) == 4)
		top_safe = MIN(top_safe, 1ULL << 32);
	assert(base < top_safe);

	if (align < align_min)
		align = align_min;

	addr = ALIGN(used, align);
	size += addr - used;

	if (size > top_safe - used) {
		printf("phys_alloc: requested=%#" PRIx64
		       " (align=%#" PRIx64 "), "
		       "need=%#" PRIx64 ", but free=%#" PRIx64 ". "
		       "top=%#" PRIx64 ", top_safe=%#" PRIx64 "\n",
		       (u64)size_orig, (u64)align, (u64)size,
		       (u64)(top_safe - used), (u64)top, (u64)top_safe);
		return NULL;
	}

	used += size;

	return phys_to_virt(addr);
}

void phys_alloc_get_unused(phys_addr_t *p_base, phys_addr_t *p_top)
{
	*p_base = used;
	*p_top = top;

	/* Empty allocator. */
	used = top;
}
