#pragma once

#include <stdint.h>

#include "v2p.h"

error_t
va2pa_pae(uint32_t virt_addr,
          const config_t *cfg,
          uint64_t *phys_addr,
          uint32_t *page_fault);
