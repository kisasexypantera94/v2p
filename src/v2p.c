#include "v2p.h"
#include "utils.h"

// One of the few situations when magic numbers are not bad IMO
static const uint8_t DIRECTORY_SHIFT = 32;
static const uint8_t TABLE_SHIFT = 22;
static const uint8_t PAGE_SHIFT = 12;


// CR3 -> PDE -> PTE -> PHYS (4KB pages)
int
va2pa_legacy(const uint32_t virt_addr,
             const config_t *const cfg,
             uint64_t *const phys_addr) {
    //---------------------------------------------------------
    uint32_t pde_addr = 0;

    // Bits 31:12 are from CR3
    pde_addr |= comp_mask(31, 12) & cfg->root_addr;

    // Bits 11:2 are bits 31:22 of the linear address
    uint32_t virt_for_pde = virt_addr & comp_mask(31, 22);
    pde_addr |= (virt_for_pde >> 20U) & comp_mask(11, 2);

    uint32_t pde;
    if (cfg->read_func(&pde, sizeof(uint32_t), pde_addr) <= 0) {
        return READ_FAULT;
    }
    if (!check_bit(pde, P_PDE4KB)) {
        return NO_TRANSLATION;
    }

    // Form PDE reserved mask
    uint64_t pde_reserved_mask = 0;
    if (cfg->pse) {
        if (cfg->pse36) {
            // If the PSE-36 mechanism is supported, bits 21:(M–19) are reserved,
            // where M is the minimum of 40 and MAXPHYADDR
            uint8_t m = min(40, cfg->maxphyaddr);
            pde_reserved_mask |= comp_mask(21, m - 19);
        } else {
            // If the PSE-36 mechanism is not supported, bits 21:13 are reserved
            pde_reserved_mask |= comp_mask(21, 13);
        }

        // If the PAT is not supported
        if (!cfg->pat) {
            // If the P flag and the PS flag of a PDE are both 1, bit 12 is reserved
            if (check_bit(pde, P_PDE4MB) && check_bit(pde, PS_PDE4MB)) {
                pde_reserved_mask |= comp_mask(12, 12);
            }
        }
    }
    // Check that none of the reserved bits was set
    if (pde & pde_reserved_mask) {
        return NO_TRANSLATION;
    }
    //---------------------------------------------------------
    // If CR4.PSE = 1 and the PDE’s PS flag is 1, the PDE maps a 4-MByte page
    if (cfg->pse && check_bit(pde, PS_PDE4MB)) {
        *phys_addr = 0;

        // Bits 39:32 are bits 20:13 of the PDE
        uint64_t pde_for_phys = pde & comp_mask(20, 13);
        *phys_addr |= (pde_for_phys << 19U) & comp_mask(39, 32);

        // Bits 31:22 are bits 31:22 of the PDE
        *phys_addr |= pde & comp_mask(31, 22);

        // Bits 21:0 are from the original linear address.
        *phys_addr |= virt_addr & comp_mask(21, 0);

        return SUCCESS;
    }

    uint32_t pte_addr = 0;

    // Bits 31:12 are from the PDE
    pte_addr |= pde & comp_mask(31, 12);

    // Bits 11:2 are bits 21:12 of the linear address
    uint32_t virt_for_pte = virt_addr & comp_mask(21, 12);
    pte_addr |= (virt_for_pte >> 10U) & comp_mask(11, 2);

    uint32_t pte;
    if (cfg->read_func(&pte, sizeof(uint32_t), pte_addr) <= 0) {
        return READ_FAULT;
    }
    if (!check_bit(pte, P_PTE)) {
        return NO_TRANSLATION;
    }
    // If the PAT is not supported
    if (!cfg->pat) {
        // If the P flag of a PTE is 1, bit 7 is reserved
        if (check_bit(pte, PAT_PTE)) {
            return NO_TRANSLATION;
        }
    }
    //---------------------------------------------------------
    *phys_addr = 0;

    // Bits 31:12 are from the PTE
    *phys_addr |= pte & comp_mask(31, 12);

    // Bits 11:0 are from the original linear address
    *phys_addr |= virt_addr & comp_mask(11, 0);

    return SUCCESS;
}

// CR3 -> PDPTE -> PDE -> PTE -> PHYS (4KB pages)
// CR3 -> PDPTE -> PDE -> PHYS (2MB pages)
int
va2pa_pae(const uint32_t virt_addr,
          const config_t *const cfg,
          uint64_t *const phys_addr) {
    //---------------------------------------------------------
    uint32_t pdpte_addr = 0;
    // TODO: not sure, maybe I should add it with cr3
    // Bits 31:30 of the linear address select a PDPTE register
    pdpte_addr |= virt_addr & comp_mask(31, 30);

    uint64_t pdpte;
    if (cfg->read_func(&pdpte, sizeof(uint64_t), pdpte_addr) <= 0) {
        return READ_FAULT;
    }
    // If the P flag (bit 0) of PDPTEi is 0, the processor ignores bits 63:1,
    // and there is no mapping for the 1-GByte region controlled by PDPTEi.
    // A reference using a linear address in this region causes a page-fault exception
    if (!check_bit(pdpte, 0)) {
        return NO_TRANSLATION;
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
    if (cfg->read_func(&pde, sizeof(uint64_t), pde_addr) <= 0) {
        return READ_FAULT;
    }
    if (!check_bit(pde, 0)) {
        return NO_TRANSLATION;
    }
    //---------------------------------------------------------
    // If the PDE’s PS flag is 1, the PDE maps a 2-MByte page
    if (check_bit(pde, 7)) {
//        reserved_mask |= comp_mask(20, 13);
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
    if (cfg->read_func(&pte, sizeof(uint64_t), pte_addr) <= 0) {
        return READ_FAULT;
    }
    if (!check_bit(pte, 0)) {
        return NO_TRANSLATION;
    }
    //---------------------------------------------------------
    *phys_addr = 0;

    // Bits 51:12 are from the PTE
    *phys_addr |= pte & comp_mask(51, 12);

    //Bits 11:0 are from the original linear address
    *phys_addr |= virt_addr & comp_mask(11, 0);

    return SUCCESS;
}

int
va2pa(const uint32_t virt_addr,
      const config_t *const cfg,
      uint64_t *const phys_addr) {
    switch (cfg->level) {
        case LEGACY: {
            return va2pa_legacy(virt_addr, cfg, phys_addr);
        }
        case PAE: {
            return va2pa_pae(virt_addr, cfg, phys_addr);
        }
        default:
            return INVALID_TRANSLATION_TYPE;
    }
}

