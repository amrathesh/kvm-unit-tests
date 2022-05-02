#include "libcflat.h"
#include "acpi.h"

#ifdef CONFIG_EFI
struct rsdp_descriptor *efi_rsdp = NULL;

void set_efi_rsdp(struct rsdp_descriptor *rsdp)
{
	efi_rsdp = rsdp;
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
	struct rsdt_descriptor_rev1 *rsdt;
	struct acpi_table_xsdt *xsdt = NULL;
	void *end;
	int i;

	/* FACS is special... */
	if (sig == FACS_SIGNATURE) {
		struct acpi_table_fadt *fadt;

		fadt = find_acpi_table_addr(FACP_SIGNATURE);
		if (!fadt)
			return NULL;

		return (void*)(ulong)fadt->firmware_ctrl;
	}

	rsdp = get_rsdp();
	if (rsdp == NULL) {
		printf("Can't find RSDP\n");
		return 0;
	}

	if (sig == RSDP_SIGNATURE)
		return rsdp;

	rsdt = (void *)(ulong)rsdp->rsdt_physical_address;
	if (!rsdt || rsdt->signature != RSDT_SIGNATURE)
		rsdt = NULL;

	if (sig == RSDT_SIGNATURE)
		return rsdt;

	if (rsdp->revision > 1)
		xsdt = (void *)(ulong)rsdp->xsdt_physical_address;
	if (!xsdt || xsdt->signature != XSDT_SIGNATURE)
		xsdt = NULL;

	if (sig == XSDT_SIGNATURE)
		return xsdt;

	// APCI requires that we first try to use XSDT if it's valid,
	//  we use to find other tables, otherwise we use RSDT.
	if (xsdt) {
		end = (void *)(ulong)xsdt + xsdt->length;
		for (i = 0; (void *)&xsdt->table_offset_entry[i] < end; i++) {
			struct acpi_table *t =
				(void *)xsdt->table_offset_entry[i];
			if (t && t->signature == sig)
				return t;
		}
	} else if (rsdt) {
		end = (void *)rsdt + rsdt->length;
		for (i = 0; (void *)&rsdt->table_offset_entry[i] < end; i++) {
			struct acpi_table *t =
				(void *)(ulong)rsdt->table_offset_entry[i];
			if (t && t->signature == sig)
				return t;
		}
	}

	return NULL;
}

void acpi_table_parse_madt(enum acpi_madt_type mtype,
			   acpi_table_handler handler)
{
	struct acpi_table_madt *madt;
	void *end;

	madt = find_acpi_table_addr(MADT_SIGNATURE);
	assert(madt);

	struct acpi_subtable_header *header =
		(void *)(ulong)madt + sizeof(struct acpi_table_madt);
	end = (void *)((ulong)madt + madt->length);

	while ((void *)header < end) {
		if (header->type == mtype)
			handler(header);

		header = (void *)(ulong)header + header->length;
	}
}
