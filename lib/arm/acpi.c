#include <asm/setup.h>

#include "libcflat.h"
#include "acpi.h"

#ifdef CONFIG_EFI
struct rsdp_descriptor *efi_rsdp = NULL;

void set_efi_rsdp(struct rsdp_descriptor *rsdp)
{
	efi_rsdp = rsdp;
	assert(rsdp->revision == 2);
}

static struct rsdp_descriptor *get_rsdp(void)
{
	if (efi_rsdp == NULL) {
		printf("Can't find RSDP from UEFI, maybe set_efi_rsdp() was not called\n");
	}
	return efi_rsdp;
}
#else
static struct rsdp_descriptor *get_rsdp(void)
{
	struct rsdp_descriptor *rsdp;
	unsigned long addr;

	for (addr = 0xe0000; addr < 0x100000; addr += 16) {
		rsdp = (void *)addr;
		if (rsdp->signature == RSDP_SIGNATURE_8BYTE)
			break;
	}

	if (addr == 0x100000) {
		return NULL;
	}

	return rsdp;
}
#endif /* CONFIG_EFI */

void* find_acpi_table_addr(u32 sig)
{
    struct rsdp_descriptor *rsdp;
    struct xsdt_descriptor *xsdt;
    void *end;
    int i;

    /* FACS is special... */
    if (sig == FACS_SIGNATURE) {
        struct fadt_descriptor_rev1 *fadt;
        fadt = find_acpi_table_addr(FACP_SIGNATURE);
        if (!fadt) {
            return NULL;
        }
        return (void*)(ulong)fadt->firmware_ctrl;
    }

    rsdp = get_rsdp();
    if (rsdp == NULL) {
        printf("Can't find RSDP\n");
        return 0;
    }

    if (sig == RSDP_SIGNATURE) {
        return rsdp;
    }

    xsdt = (void*)(ulong)rsdp->xsdt_physical_address;
    if (!xsdt || xsdt->signature != XSDT_SIGNATURE)
        return 0;

    if (sig == XSDT_SIGNATURE) {
        return xsdt;
    }

    end = (void*)(ulong)xsdt + xsdt->length;
    for (i=0; (void*)&xsdt->table_offset_entry[i] < end; i++) {
	struct acpi_table *t = (void *)xsdt->table_offset_entry[i];
        if (t && t->signature == sig) {
            return t;
        }
    }
   return NULL;
}

#if 0
void
acpi_parse_gic_cpu_interface(union acpi_subtable_headers *header,
			     void (*func)(int fdtnode, u64 regval, void *info))
{
    struct acpi_madt_generic_interrupt *processor;
    processor = (struct acpi_madt_generic_interrupt *)header;
    u64 hwid = processor->arm_mpidr;

    if (!(processor->flags & ACPI_MADT_ENABLED)) {
	printf("skipping disabled CPU entry with 0x%lx MPIDR\n", hwid);
	return;
    }
    
    if (hwid & ~MPIDR_HWID_BITMASK || hwid == INVALID_HWID) {
        printf("skipping CPU entry with invalid MPIDR 0x%llx\n", hwid);
	return;
    }

    if (is_mpidr_duplicate(cpu_count, hwid)) {
        print("duplicate CPU MPIDR 0x%llx in MADT\n", hwid);
	return;
    }

    /* Check if GICC structure of boot CPU is available in the MADT */
    if (cpu_logical_map(0) == hwid) {
        if (bootcpu_valid) {
	    pr_err("duplicate boot CPU MPIDR: 0x%llx in MADT\n",
		   hwid);
	    return;
	}
	bootcpu_valid = true;
	cpu_madt_gicc[0] = *processor;
	return;
    }
    func(0, hwid, 0);

    return 0;
}
#endif

void acpi_for_each_cpu(void (*func)(int fdtnode, u64 regval, void *info))
{
    struct acpi_table_madt *madt;
    void *end;

    madt = find_acpi_table_addr(MADT_SIGNATURE);
    assert(madt);
	
    struct acpi_subtable_header *header =
	    (void*)(ulong)madt + sizeof(struct acpi_table_madt);
    end = (void*)((ulong)madt + madt->length);

    while ((void *)header < end) {
	    if (header->type == ACPI_MADT_TYPE_GENERIC_INTERRUPT) {
		    struct acpi_madt_generic_interrupt *gicc =
			    (struct acpi_madt_generic_interrupt *)header;
		    func(0, gicc->arm_mpidr, 0);
	    }  
	    header = (void*)(ulong)header + header->length;
    }

}
