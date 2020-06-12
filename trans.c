#include "trans.h"

uint32_t
get_mask(uint8_t l, uint8_t r) {
    uint32_t a = r != 0 ? ~((1UL << r) - 1) : 0xffffffff;
    uint32_t b = l != 0 ? (1UL << (l + 1U)) - 1 : 1;
    return a & b;
}

void
va2pa_legacy(uint32_t virt_addr,
             uint32_t root_addr,
             uint32_t *phys_addr) {
    //---------------------------------------------------------
    uint32_t pde = 0;

    // Bits 31:12 are from CR3
    pde |= get_mask(31, 12) & root_addr;

    // Bits 11:2 are bits 31:22 of the linear address
    uint32_t virt_for_pde = virt_addr & get_mask(31, 22);
    pde |= (virt_for_pde >> 20U) & get_mask(11, 2);
    //---------------------------------------------------------
    uint32_t pte = 0;
    // Bits 31:12 are from the PDE
    pte |= pde & get_mask(31, 12);

    // Bits 11:2 are bits 21:12 of the linear address
    uint32_t virt_for_pte = virt_addr & get_mask(21, 12);
    pte |= (virt_for_pte >> 10U) & get_mask(11, 2);
    //---------------------------------------------------------

    *phys_addr |= pte & get_mask(31, 12);
    *phys_addr |= virt_addr & get_mask(11, 0);
}

int
va2pa(uint32_t virt_addr,
      uint32_t level,
      uint32_t root_addr,
      PREAD_FUNC read_func,
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

