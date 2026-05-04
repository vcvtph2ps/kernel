#include <common/cpu_local.h>
#include <common/init.h>
#include <lib/log.h>
#include <memory/heap.h>
#include <memory/pmm.h>
#include <memory/ptm.h>
#include <memory/slab.h>

void init_stage_base_mem(uint32_t core_id) {
    if(INIT_CORE_IS_BSP(core_id)) {
        log_init();
        cpu_local_init_early();
        pmm_init();
        log_init_framebuffer();
        slab_cache_init();
        heap_init();
    } else {
        ptm_init_kernel(core_id);
        cpu_local_init(core_id);
    }
}
