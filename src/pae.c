#include "pae.h"
#include "utils.h"

// CR3 -> PDPTE -> PDE -> PTE -> PHYS (4KB pages)
// CR3 -> PDPTE -> PDE -> PHYS (2MB pages)
error_t
va2pa_pae(const uint32_t virt_addr,
          const config_t *const cfg,
          uint64_t *const phys_addr,
          uint32_t *page_fault) {
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
    if (cfg->read_func(&pde, sizeof(uint64_t), pde_addr) <= 0) {
        return READ_FAULT;
    }
    if (!check_bit(pde, 0)) {
        return PAGE_FAULT;
    }
    uint64_t pde_reserved_mask = 0;
    // If the P flag (bit 0) of a PDE bits 62:MAXPHYADDR are reserved.
    pde_reserved_mask |= comp_mask(62, cfg->maxphyaddr);
    if (check_bit(pde, 7)) {
        // If the P flag and the PS flag (bit 7) of a PDE are both 1, bits 20:13 are reserved
        pde_reserved_mask |= comp_mask(20, 13);

        // If the PAT is not supported
        if (!cfg->pat) {
            // If the P flag and the PS flag of a PDE are both 1, bit 12 is reserved
            pde_reserved_mask |= comp_mask(12, 12);
        }
    }
    if (!cfg->nxe) {
        // If IA32_EFER.NXE = 0 and the P flag of a PDE, the XD flag (bit 63) is reserved
        pde_reserved_mask |= comp_mask(63, 63);
    }
    // check that none of the reserved bits has been set
    if (pde & pde_reserved_mask) {
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

        return SUCCESS;
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
        return PAGE_FAULT;
    }
    uint64_t pte_reserved_mask = 0;
    // If the P flag (bit 0) of a PTE is 1, bits 62:MAXPHYADDR are reserved
    pte_reserved_mask |= comp_mask(62, cfg->maxphyaddr);
    if (!cfg->nxe) {
        // If IA32_EFER.NXE = 0 and the P flag of a PTE is 1, the XD flag (bit 63) is reserved
        pte_reserved_mask |= comp_mask(63, 63);
    }
    if (!cfg->pat) {
        pte_reserved_mask |= comp_mask(7, 7);
    }
    if (pte & pte_reserved_mask) {
        return PAGE_FAULT;
    }
    //---------------------------------------------------------
    *phys_addr = 0;

    // Bits 51:12 are from the PTE
    *phys_addr |= pte & comp_mask(51, 12);

    //Bits 11:0 are from the original linear address
    *phys_addr |= virt_addr & comp_mask(11, 0);

    return SUCCESS;
}
