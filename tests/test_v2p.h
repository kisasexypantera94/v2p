#pragma once

#include <string.h>

#include "v2p.h"
#include "utils.h"

// Mask for every part of the translation (pde, pte)
static uint64_t mock_read_mask[] = {0, 0};
static int mock_read_part = 0;
static int32_t mock_read_return = sizeof(uint64_t);

int32_t
mock_read_func(void *buf, const uint32_t size, const uint64_t physical_addr) {
    uint64_t val = physical_addr | mock_read_mask[mock_read_part++];
    memcpy(buf, &val, size);
    return mock_read_return;
}

bool
test_va2pa() {
    typedef struct {
        const char *name;
        uint32_t virt_addr;
        config_t cfg;
        uint64_t read_mask[2];
        int32_t read_return;
        uint64_t want_phys;
        error_t want_err;
        error_t want_page_fault;
    } test_case;

    test_case t[] = {
            {
                    "ok",
                    123456789,
                    {
                            .level=LEGACY,
                            .root_addr=777777777,
                            .read_func=mock_read_func,
                            .pat=true,
                            .maxphyaddr=52,
                    },
                    {1U,                     1U}, // set only present flag
                    sizeof(uint64_t),
                    0b101110010110111111110100010101,
                    SUCCESS
            },
            {
                    "pat on, not present",
                    0,
                    {
                            .level = LEGACY,
                            .root_addr=0,
                            .read_func=mock_read_func,
                            .pat = true,
                            .maxphyaddr = 52,
                    },
                    {0,                      0}, // not present
                    sizeof(uint64_t),
                    0,
                    PAGE_FAULT,
                    0
            },
            {
                    "read fault",
                    0,
                    {
                            .level=LEGACY,
                            .root_addr=0,
                            .read_func=mock_read_func,
                            .pat=true,
                            .maxphyaddr=52,
                    },
                    {1U,                     1U},
                    0,
                    0,
                    READ_FAULT
            },
            {
                    "pat off, not present",
                    0,
                    {
                            .level=LEGACY,
                            .root_addr=0,
                            .read_func=mock_read_func,
                            .pat=false,
                            .maxphyaddr=52,
                    },
                    {0,                      0},
                    sizeof(uint64_t),
                    0,
                    PAGE_FAULT,
                    0,
            },
            {
                    "pat off, set reserved, pde, 12",
                    0,
                    {
                            .level=LEGACY,
                            .root_addr=0,
                            .read_func=mock_read_func,
                            .pat=false,
                            .pse=true,
                            .maxphyaddr=52,
                    },
                    {1U
                     | comp_mask(12, 12)
                     | (1 << P_PDE4MB)
                     | (1 << PS_PDE4MB),     1U},
                    sizeof(uint64_t),
                    0,
                    PAGE_FAULT,
                    comp_mask(3, 3),
            },
            {
                    "pat on, set reserved pde",
                    0,
                    {
                            .level=LEGACY,
                            0,
                            .read_func=mock_read_func,
                            .pat=true,
                            .pse=true,
                            .maxphyaddr=52,
                    },
                    {1U | comp_mask(12, 12), 1U},
                    sizeof(uint64_t),
                    4096,
                    SUCCESS
            },
            {
                    "set reserved pte",
                    0,
                    {
                            .level=LEGACY,
                            .root_addr=0,
                            .read_func=mock_read_func,
                            .pat=false,
                            .pse=true,
                            .maxphyaddr=52,
                    },
                    {1U,                     1U | (1 << PAT_PTE)},
                    sizeof(uint64_t),
                    0,
                    PAGE_FAULT,
                    comp_mask(3, 3),
            },
    };
    int n = sizeof(t) / sizeof(test_case);

    bool ok = true;
    for (int i = 0; i < n; ++i) {
        // initialize mock_read_func
        mock_read_part = 0;
        mock_read_mask[0] = t[i].read_mask[0];
        mock_read_mask[1] = t[i].read_mask[1];
        mock_read_return = t[i].read_return;

        uint64_t phys = 0;
        uint32_t page_fault = 0;
        error_t err = va2pa(t[i].virt_addr, &t[i].cfg, &phys, &page_fault);
        if (err != t[i].want_err) {
            printf("wrong return code for test '%s'\ngot:  %d\nwant: %d\n\n",
                   t[i].name, err, t[i].want_err);
        }
        if (err == PAGE_FAULT && page_fault != t[i].want_page_fault) {
            printf("wrong page fault for test '%s'\ngot:  %d\nwant: %d\n\n",
                   t[i].name, page_fault, t[i].want_page_fault);
        }
        if (phys != t[i].want_phys) {
            printf("wrong returned physaddr for test '%s'\ngot:  %llu\nwant: %llu\n\n",
                   t[i].name, phys, t[i].want_phys);
            ok = false;
        }
    }

    return ok;
}
