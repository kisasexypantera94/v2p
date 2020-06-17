#include "legacy.h"
#include "utils.h"

// CR3 -> PDE -> PTE -> PHYS (4KB pages)
// CR3 -> PDE -> PHYS (4MB pages)
error_t
va2pa_legacy(const uint32_t virt_addr,
             const config_t *const cfg,
             uint64_t *const phys_addr,
             uint32_t *page_fault) {
    bool is_supervisor_addr = false;
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
        return PAGE_FAULT;
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
    // Check that none of the reserved bits has been set
    if (pde & pde_reserved_mask) {
        // TODO: set page_fault
        return PAGE_FAULT;
    }
    if (!check_bit(pde, 2)) {
        is_supervisor_addr = true;
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

        return check_access(is_supervisor_addr, cfg, page_fault);
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
        // TODO: set page_fault
        return PAGE_FAULT;
    }
    // If the PAT is not supported
    if (!cfg->pat) {
        // If the P flag of a PTE is 1, bit 7 is reserved
        if (check_bit(pte, PAT_PTE)) {
            // TODO: set page_fault
            return PAGE_FAULT;
        }
    }
    if (!check_bit(pte, 2)) {
        is_supervisor_addr = true;
    }
    //---------------------------------------------------------
    *phys_addr = 0;

    // Bits 31:12 are from the PTE
    *phys_addr |= pte & comp_mask(31, 12);

    // Bits 11:0 are from the original linear address
    *phys_addr |= virt_addr & comp_mask(11, 0);

    return check_access(is_supervisor_addr, cfg, page_fault);
}

