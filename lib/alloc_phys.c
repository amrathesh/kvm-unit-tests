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

#define PHYS_ALLOC_NR_REGIONS	256

struct phys_alloc_region {
	phys_addr_t base;
	phys_addr_t size;
};

static struct phys_alloc_region regions[PHYS_ALLOC_NR_REGIONS];
static int nr_regions;

static phys_addr_t base, top;

#define DEFAULT_MINIMUM_ALIGNMENT	32
static size_t align_min = DEFAULT_MINIMUM_ALIGNMENT;

static void *memalign_early(size_t alignment, size_t sz);
static struct alloc_ops early_alloc_ops = {
	.memalign = memalign_early,
};
struct alloc_ops *alloc_ops = &early_alloc_ops;

void phys_alloc_show(void)
{
	int i;

	printf("phys_alloc minimum alignment: %#" PRIx64 "\n", (u64)align_min);
	for (i = 0; i < nr_regions; ++i)
		printf("%016" PRIx64 "-%016" PRIx64 " [%s]\n",
			(u64)regions[i].base,
			(u64)(regions[i].base + regions[i].size - 1),
			"USED");
	printf("%016" PRIx64 "-%016" PRIx64 " [%s]\n",
		(u64)base, (u64)(top - 1), "FREE");
}

void phys_alloc_init(phys_addr_t base_addr, phys_addr_t size)
{
	base = base_addr;
	top = base + size;
	nr_regions = 0;
}

void phys_alloc_set_minimum_alignment(phys_addr_t align)
{
	assert(align && !(align & (align - 1)));
	align_min = align;
}

static void *memalign_early(size_t alignment, size_t sz)
{
	static bool warned = false;
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

	addr = ALIGN(base, align);
	size += addr - base;

	if ((top_safe - base) < size) {
		printf("phys_alloc: requested=%#" PRIx64
		       " (align=%#" PRIx64 "), "
		       "need=%#" PRIx64 ", but free=%#" PRIx64 ". "
		       "top=%#" PRIx64 ", top_safe=%#" PRIx64 "\n",
		       (u64)size_orig, (u64)align, (u64)size,
		       (u64)(top_safe - base), (u64)top, (u64)top_safe);
		return NULL;
	}

	base += size;

	if (nr_regions < PHYS_ALLOC_NR_REGIONS) {
		regions[nr_regions].base = addr;
		regions[nr_regions].size = size_orig;
		++nr_regions;
	} else if (!warned) {
		printf("WARNING: phys_alloc: No free log entries, "
		       "can no longer log allocations...\n");
		warned = true;
	}

	return phys_to_virt(addr);
}

void phys_alloc_get_unused(phys_addr_t *p_base, phys_addr_t *p_top)
{
	*p_base = base;
	*p_top = top;
	if (base == top)
		return;
	regions[nr_regions].base = base;
	regions[nr_regions].size = top - base;
	++nr_regions;
	base = top;
}
