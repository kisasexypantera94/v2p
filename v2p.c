#include "v2p.h"

static const uint8_t DIRECTORY_SHIFT = 32;
static const uint8_t TABLE_SHIFT = 22;
static const uint8_t PAGE_SHIFT = 12;

uint32_t
get_mask(const uint8_t l, const uint8_t r) {
    uint32_t a = r != 0 ? ~((1UL << r) - 1) : 0xffffffff;
    uint32_t b = l != 0 ? (1UL << (l + 1U)) - 1 : 1;
    return a & b;
}

void
va2pa_legacy(const uint32_t virt_addr,
             const uint32_t root_addr,
             uint32_t *phys_addr) {
    //---------------------------------------------------------
    uint32_t pde = 0;

    // Bits 31:12 are from CR3
    pde |= get_mask(DIRECTORY_SHIFT - 1, PAGE_SHIFT) & root_addr;

    // Bits 11:2 are bits 31:22 of the linear address
    uint32_t virt_for_pde = virt_addr & get_mask(DIRECTORY_SHIFT - 1, TABLE_SHIFT);
    pde |= (virt_for_pde >> 20U) & get_mask(PAGE_SHIFT - 1, 2);
    //---------------------------------------------------------
    uint32_t pte = 0;

    // Bits 31:12 are from the PDE
    pte |= pde & get_mask(DIRECTORY_SHIFT - 1, PAGE_SHIFT);

    // Bits 11:2 are bits 21:12 of the linear address
    uint32_t virt_for_pte = virt_addr & get_mask(TABLE_SHIFT - 1, PAGE_SHIFT);
    pte |= (virt_for_pte >> 10U) & get_mask(PAGE_SHIFT - 1, 2);
    //---------------------------------------------------------

    // Bits 31:12 are from the PTE
    *phys_addr |= pte & get_mask(DIRECTORY_SHIFT - 1, PAGE_SHIFT);

    // Bits 11:0 are from the original linear address
    *phys_addr |= virt_addr & get_mask(PAGE_SHIFT - 1, 0);
}

int
va2pa(const uint32_t virt_addr,
      const uint32_t level,
      const uint32_t root_addr,
      const PREAD_FUNC read_func,
      uint32_t *phys_addr) {
    switch (level) {
        case LEGACY:
            va2pa_legacy(virt_addr, root_addr, phys_addr);
            break;
        case PAE:
            break;
        default:
            break;
    }
}

