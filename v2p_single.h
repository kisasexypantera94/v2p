#pragma once 

#include <stdint.h>
#include <stdbool.h>

typedef enum paging_mode {
    LEGACY = 2,
    PAE = 3,
} paging_mode_t;

typedef enum error {
    SUCCESS = 0,
    PAGE_FAULT = -1,
    READ_FAULT = -2,
    INVALID_TRANSLATION_TYPE = -3,
} error_t;

typedef enum page_fault {
    NOT_PRESENT = 0,
    RESERVED_BIT_VIOLATION = (1U << 3U),
} page_fault_t;

// TODO: move to legacy.c/pae.c
enum flags {
    // PDE
    P_PDE4KB = 0U,
    P_PDE2MB = 0U,
    P_PDE4MB = 0U,
    PS_PDE4KB = 7U,
    PS_PDE2MB = 7U,
    PS_PDE4MB = 7U,

    // PTE
    P_PTE = 0U,
    PAT_PTE = 7U,
};

// чтение указанного количества байт физической памяти по заданному адресу в указанный буфер
// функция вернет количество прочитанных байт (меньшее или 0 означает ошибку - выход за пределы памяти)
typedef int32_t (*pread_func_t)(void *buf, const uint32_t size, const uint64_t physical_addr);

typedef struct config {
    // paging mode
    paging_mode_t level;

    // cr3
    uint32_t root_addr;

    // function which reads from physical-address
    pread_func_t read_func;

    // page-size extensions for 32-bit paging
    bool pse;

    // page-size extensions with 40-bit physical-address extension
    bool pse36;

    // page-attribute table
    bool pat;

    // execute disable
    bool nxe;

    // supervisor-mode access prevention
    bool smap;

    // override smap
    bool ac;

    // supervisor-mode execution prevention
    bool smep;

    // allows pages to be protected from supervisor-mode writes
    bool wp;

    // true if cpl < 3
    bool supervisor;

    // physical-address width supported by the processor
    uint8_t maxphyaddr;
} config_t;

// функция трансляции виртуального адреса в физический:
//
// virt_addr - виртуальный адрес подлежащий трансляции
// cfg - конфигурация трансляции
// функция возвращает успешность трансляции: 0 - успешно, не 0 - ошибки
// phys_addr - выходной оттранслированный физический адрес
// page_fault - код page_fault исключения
error_t
va2pa(uint32_t virt_addr, const config_t *cfg, uint64_t *phys_addr, uint32_t *page_fault);

#include <stdint.h>

error_t
va2pa_legacy(uint32_t virt_addr,
             const config_t *cfg,
             uint64_t *phys_addr,
             uint32_t *page_fault);

#include <stdint.h>

error_t
va2pa_pae(uint32_t virt_addr,
          const config_t *cfg,
          uint64_t *phys_addr,
          uint32_t *page_fault);

// One of the few situations when magic numbers are not bad IMO
static const uint8_t DIRECTORY_SHIFT = 32;
static const uint8_t TABLE_SHIFT = 22;
static const uint8_t PAGE_SHIFT = 12;

error_t
va2pa(const uint32_t virt_addr,
      const config_t *const cfg,
      uint64_t *const phys_addr,
      uint32_t *page_fault) {
    switch (cfg->level) {
        case LEGACY: {
            return va2pa_legacy(virt_addr, cfg, phys_addr, page_fault);
        }
        case PAE: {
            return va2pa_pae(virt_addr, cfg, phys_addr, page_fault);
        }
        default:
            return INVALID_TRANSLATION_TYPE;
    }
}

#include <stdint.h>
#include <stdbool.h>

inline uint64_t
check_bit(uint64_t x, uint8_t N);

inline uint64_t
comp_mask(uint8_t l, uint8_t r);

inline uint8_t
min(uint8_t a, uint8_t b);

error_t
check_access(bool is_supervisor_addr,
             const config_t *cfg,
             uint32_t *page_fault);

void
get_features(bool *pat, uint8_t *maxphyaddr);

// CR3 -> PDE -> PTE -> PHYS (4KB pages)
// CR3 -> PDE -> PHYS (4MB pages)
error_t
va2pa_legacy(const uint32_t virt_addr,
             const config_t *const cfg,
             uint64_t *const phys_addr,
             uint32_t *page_fault) {
    //---------------------------------------------------------
    // GET PDE
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
        *page_fault = 0;
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
        *page_fault = comp_mask(3, 3);
        return PAGE_FAULT;
    }
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

    //---------------------------------------------------------
    // GET PTE
    //---------------------------------------------------------
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
        *page_fault = 0;
        return PAGE_FAULT;
    }
    if (cfg->pse && !cfg->pat) {
        // If the P flag of a PTE is 1, bit 7 is reserved
        if (check_bit(pte, PAT_PTE)) {
            *page_fault = comp_mask(3, 3);
            return PAGE_FAULT;
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
        *page_fault |= 0U;
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
        *page_fault |= 0U;
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
        *page_fault |= comp_mask(3, 3);
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
        *page_fault |= 0U;
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
        *page_fault |= comp_mask(3, 3);
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
#include <cpuid.h>

uint64_t
check_bit(const uint64_t x, const uint8_t N) {
    return x & (1U << N);
}

// Compute bit-mask with bits[l:r] set to 1
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

uint8_t
min(uint8_t a, uint8_t b) {
    return a < b ? a : b;
}

error_t
check_access(bool is_supervisor_addr,
             const config_t *const cfg,
             uint32_t *page_fault) {
    // if supervisor-mode access
    if (cfg->supervisor) {
        // If CR4.SMAP=1 and EFLAGS.AC=0 or the access is implicit,
        // data may not be read from any user-mode address
        if (!is_supervisor_addr && cfg->smap && !cfg->ac) {
            // TODO: set page_fault
            return PAGE_FAULT;
        }
    } else if (is_supervisor_addr) {
        // TODO: set page_fault
        return PAGE_FAULT;
    }

    return SUCCESS;
}

void
get_features(bool *pat, uint8_t *maxphyaddr) {
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;

    // If CPUID.01H:EDX.PAT [bit 16] = 1, the 8-entry page-attribute table (PAT) is supported.
    if (__get_cpuid(0x1, &eax, &ebx, &ecx, &edx)) {
        *pat = check_bit(edx, 16);
    }

    // CPUID.80000008H:EAX[7:0] reports the physical-address width supported by the processor.
    // This width is referred to as MAXPHYADDR.
    if (__get_cpuid(0x80000008, &eax, &ebx, &ecx, &edx)) {
        *maxphyaddr = eax & comp_mask(7, 0);
    }
}
