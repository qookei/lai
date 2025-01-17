/*
 * Lightweight ACPI Implementation
 * Copyright (C) 2018-2019 the lai authors
 */

#include <acpispec/tables.h>
#include <lai/core.h>
#include <lai/helpers/pc-bios.h>

static uint8_t lai_bios_calc_checksum(void *ptr, size_t size) {
    uint8_t sum = 0;
    for (size_t i = 0; i < size; i++)
        sum += ((uint8_t *)ptr)[i];
    return sum;
}

lai_api_error_t lai_bios_detect_rsdp_within(uintptr_t base, size_t length,
        struct lai_rsdp_info *info) {
    int e = LAI_ERROR_END_REACHED;
    uint8_t *window = laihost_map(base, length);
    for (size_t off = 0; off < length; off += 16) {
        acpi_rsdp_t *rsdp = (acpi_rsdp_t *)(window + off);

        if (memcmp(rsdp->signature, "RSD PTR ", 8))
            continue;
        if (lai_bios_calc_checksum(rsdp, sizeof(acpi_rsdp_t)))
            continue;

        // TODO: Handle XSDTs and support ACPI 2 detection.
        info->acpi_version = 1;
        info->rsdp_address = base + off;
        info->rsdt_address = rsdp->rsdt;
        e = LAI_ERROR_NONE;
        goto done;
    }

done:
    // TODO: Unmap the memory range.
    return e;
}

lai_api_error_t lai_bios_detect_rsdp(struct lai_rsdp_info *info) {
    int e;

    // ACPI specifies that we can find the EBDA through 0x40E.
    uint16_t bda_data;
    void *bda_window = laihost_map(0x40E, sizeof(uint16_t));
    memcpy(&bda_data, bda_window, sizeof(uint16_t));

    uintptr_t ebda_base = ((uintptr_t)bda_data) << 4;

    // Regions specified by ACPI: (i) first 1 KiB of EBDA, (ii) 0xE0000 - 0xFFFFF.
    if(!(e = lai_bios_detect_rsdp_within(ebda_base, 0x400, info)))
        return LAI_ERROR_NONE;
    LAI_ENSURE(e == LAI_ERROR_END_REACHED);

    if(!(e = lai_bios_detect_rsdp_within(0xE0000, 0x20000, info)))
        return LAI_ERROR_NONE;
    LAI_ENSURE(e == LAI_ERROR_END_REACHED);

    return LAI_ERROR_END_REACHED;
}
