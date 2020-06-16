#include "v2p.h"
#include "legacy.h"
#include "pae.h"

// One of the few situations when magic numbers are not bad IMO
static const uint8_t DIRECTORY_SHIFT = 32;
static const uint8_t TABLE_SHIFT = 22;
static const uint8_t PAGE_SHIFT = 12;


error_t
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

