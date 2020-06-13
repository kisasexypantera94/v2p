#include <stdbool.h>

#include "v2p.h"


// One of the few situations when magic numbers are not bad IMO
static const uint8_t DIRECTORY_SHIFT = 32;
static const uint8_t TABLE_SHIFT = 22;
static const uint8_t PAGE_SHIFT = 12;


bool
check_bit(const uint32_t x, const uint8_t N) {
    return x & (1U << N);
}


uint64_t
comp_mask(const uint8_t l, const uint8_t r) {
    uint64_t a = r != 0 ? ~((1UL << r) - 1) : 0xffffffffffffffff;
    uint64_t b;
    if (l == 0) {
        b = 0x1;
    } else if (l == 63) {
        b = 0xffffffffffffffff;
    } else {
        b = (1UL << (l + 1U)) - 1;
    }
    return a & b;
}

// CR3 -> PDE -> PTE -> PHYS (4KB pages)
int
va2pa_legacy(const uint32_t virt_addr,
             const uint32_t root_addr,
             const PREAD_FUNC read_func,
             uint64_t *phys_addr) {
    //---------------------------------------------------------
    uint32_t pde_addr = 0;

    // Bits 31:12 are from CR3
    pde_addr |= comp_mask(31, 12) & root_addr;

    // Bits 11:2 are bits 31:22 of the linear address
    uint32_t virt_for_pde = virt_addr & comp_mask(31, 22);
    pde_addr |= (virt_for_pde >> 20U) & comp_mask(11, 2);

    uint32_t pde;
    if (read_func(&pde, sizeof(uint32_t), pde_addr) <= 0) {
        return READ_FAULT;
    }
    if (!check_bit(pde, 0)) {
        return PAGE_FAULT;
    }
    //---------------------------------------------------------
    uint32_t pte_addr = 0;

    // Bits 31:12 are from the PDE
    pte_addr |= pde & comp_mask(31, 12);

    // Bits 11:2 are bits 21:12 of the linear address
    uint32_t virt_for_pte = virt_addr & comp_mask(21, 12);
    pte_addr |= (virt_for_pte >> 10U) & comp_mask(11, 2);

    uint32_t pte;
    if (read_func(&pte, sizeof(uint32_t), pte_addr) <= 0) {
        return READ_FAULT;
    }
    if (!check_bit(pte, 0)) {
        return PAGE_FAULT;
    }
    //---------------------------------------------------------
    *phys_addr = 0;

    // Bits 31:12 are from the PTE
    *phys_addr |= pte & comp_mask(31, 12);

    // Bits 11:0 are from the original linear address
    *phys_addr |= virt_addr & comp_mask(11, 0);

    return 0;
}

// CR3 -> PDPTE -> PDE -> PTE -> PHYS (4KB pages)
// CR3 -> PDPTE -> PDE -> PHYS (2MB pages)
int
va2pa_pae(const uint32_t virt_addr,
          const uint32_t root_addr,
          const PREAD_FUNC read_func,
          uint64_t *phys_addr) {
    //---------------------------------------------------------
    uint32_t pdpte_addr = 0;
    // TODO: not sure, maybe I should add it with cr3
    // Bits 31:30 of the linear address select a PDPTE register
    pdpte_addr |= virt_addr & comp_mask(31, 30);

    uint64_t pdpte;
    if (read_func(&pdpte, sizeof(uint64_t), pdpte_addr) <= 0) {
        return READ_FAULT;
    }
    // If the P flag (bit 0) of PDPTEi is 0, the processor ignores bits 63:1,
    // and there is no mapping for the 1-GByte region controlled by PDPTEi.
    // A reference using a linear address in this region causes a page-fault exception
    if (!check_bit(pdpte, 0)) {
        return PAGE_FAULT;
    }
    //---------------------------------------------------------
    // If the P flag of PDPTEi is 1, 4-KByte naturally aligned page directory
    // is located at the physical address specified in bits 51:12 of PDPTEi
    uint64_t pde_addr = 0;

    // Bits 51:12 are from PDPTEi
    pde_addr |= pdpte & comp_mask(51, 12);

    // Bits 11:3 are bits 29:21 of the linear address
    uint64_t virt_for_pde = virt_addr & comp_mask(29, 21);
    pde_addr |= (virt_for_pde >> 18U) & comp_mask(11, 3);

    uint64_t pde;
    if (read_func(&pde, sizeof(uint64_t), pde_addr) <= 0) {
        return READ_FAULT;
    }
    if (!check_bit(pde, 0)) {
        return PAGE_FAULT;
    }
    //---------------------------------------------------------
    // If the PDE’s PS flag is 1, the PDE maps a 2-MByte page
    if (check_bit(pde, 7)) {
        *phys_addr = 0;

        // Bits 51:21 are from the PDE
        *phys_addr |= pde & comp_mask(51, 21);

        // Bits 20:0 are from the original linear address
        *phys_addr |= virt_addr & comp_mask(20, 0);

        return 0;
    }

    // If the PDE’s PS flag is 0, a 4-KByte naturally aligned page table
    // is located at the physical address specified in bits 51:12 of the PDE.
    // A page table comprises 512 64-bit entries (PTEs).
    uint64_t pte_addr = 0;
    // Bits 51:12 are from the PDE
    pte_addr |= pde & comp_mask(51, 12);
    // Bits 11:3 are bits 20:12 of the linear address
    uint64_t virt_for_pte = virt_addr & comp_mask(20, 12);
    pte_addr |= (virt_for_pte >> 9U) & comp_mask(11, 3);

    uint64_t pte;
    if (read_func(&pte, sizeof(uint64_t), pte_addr) <= 0) {
        return READ_FAULT;
    }
    if (!check_bit(pte, 0)) {
        return PAGE_FAULT;
    }
    //---------------------------------------------------------
    *phys_addr = 0;

    // Bits 51:12 are from the PTE
    *phys_addr |= pte & comp_mask(51, 12);

    //Bits 11:0 are from the original linear address
    *phys_addr |= virt_addr & comp_mask(11, 0);

    return 0;
}

int
va2pa(const uint32_t virt_addr,
      const uint32_t level,
      const uint32_t root_addr,
      const PREAD_FUNC read_func,
      uint64_t *phys_addr) {
    switch (level) {
        case LEGACY:
            return va2pa_legacy(virt_addr, root_addr, read_func, phys_addr);
        case PAE:
            return va2pa_pae(virt_addr, root_addr, read_func, phys_addr);
        default:
            break;
    }

    return 0;
}

