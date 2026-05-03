#include "init_stages.h"
#include <common/cpu_local.h>
#include <memory/pmm.h>
#include <memory/ptm.h>
#include <memory/heap.h>
#include <memory/slab.h>
#include <lib/log.h>

void fn_init_stage_base_mem_bsp(uint32_t core_id) {
    (void) core_id;
    cpu_local_init_bsp();
    pmm_init();
    ptm_init_kernel_bsp();
    log_init();
    slab_cache_init();
    heap_init();
}

void fn_init_stage_base_mem_ap(uint32_t core_id) {
    ptm_init_kernel_ap();
    cpu_local_init_ap(core_id);

    LOG_OKAY("core %u early init\n", core_id);
}
