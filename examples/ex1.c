#include <stdio.h>
#include <string.h>

#include "v2p.h"

int32_t
read_func(void *buf, const uint32_t size, const uint64_t physical_addr) {
    uint64_t val = physical_addr | 1U;
    memcpy(buf, &val, size);
    return sizeof(val);
}


int main() {
    uint32_t virt_addr = 123456789;
    config_t cfg = {.level=LEGACY, .read_func=read_func, .root_addr=777777777};

    uint64_t phys;
    uint32_t page_fault;
    error_t err = va2pa(virt_addr, &cfg, &phys, &page_fault);

    switch (err) {
        case SUCCESS: {
            printf("OK: Physical address: %llu\n", phys);
            break;
        }
        case PAGE_FAULT: {
            switch (page_fault) {
                case NOT_PRESENT:
                    printf("ERROR: page fault occurred: not present\n");
                    break;
                case RESERVED_BIT_VIOLATION:
                    printf("ERROR: page fault occurred: reserved bit has been set\n");
                    break;
            }
            break;
        }
        case READ_FAULT: {
            printf("ERROR: read fault occurred\n");
            break;
        }
        case INVALID_TRANSLATION_TYPE: {
            printf("ERROR: invalid translation type\n");
            break;
        }
    }

    return 0;
}