#include "init_stages.h"
#include <common/arch.h>
#include <common/sched/sched.h>
#include <lib/log.h>

ATOMIC uint32_t g_arch_bsp_finished = 0;
ATOMIC uint32_t g_arch_ap_finished = 0;

const char* init_stage_to_str(init_stage_t stage) {
    switch(stage) {
        case INIT_STAGE_BASE_MEM:       return "base_mem";
        case INIT_STAGE_ARCH_CPU:       return "arch_cpu";
        case INIT_STAGE_TIME:           return "time";
        case INIT_STAGE_ACPI_BOOTSTRAP: return "acpi_bootstrap";
        case INIT_STAGE_ARCH_PLATFORM:  return "arch_platform";
        case INIT_STAGE_ARCH_AP_INIT:   return "arch_ap_init";
        case INIT_STAGE_ARCH_USERSPACE: return "arch_userspace";
        case INIT_STAGE_VFS:            return "vfs";
        case INIT_STAGE_USERSPACE:      return "userspace";
    }
    __builtin_unreachable();
}

typedef struct {
    init_stage_t stage;
    void (*handler)(uint32_t core_id);
    bool bsp_only;
} init_stage_handler_t;

#define INIT_STAGE(STAGE, HANDLER, BSP_ONLY) { .stage = (STAGE), .handler = (HANDLER), .bsp_only = (BSP_ONLY) }

init_stage_handler_t g_init_stage_handlers[] = {
    INIT_STAGE(INIT_STAGE_BASE_MEM, fn_init_stage_base_mem_bsp, true),
    INIT_STAGE(INIT_STAGE_BASE_MEM, fn_init_stage_base_mem_ap, false),

    INIT_STAGE(INIT_STAGE_ARCH_CPU, fn_init_stage_arch_cpu_bsp, true),
    INIT_STAGE(INIT_STAGE_ARCH_CPU, fn_init_stage_arch_cpu_ap, false),

    INIT_STAGE(INIT_STAGE_TIME, fn_init_stage_time_bsp, true),
    INIT_STAGE(INIT_STAGE_TIME, fn_init_stage_time_ap, false),

    INIT_STAGE(INIT_STAGE_ACPI_BOOTSTRAP, fn_init_stage_acpi_bootstrap, true),
    INIT_STAGE(INIT_STAGE_ARCH_PLATFORM, fn_init_stage_arch_platform, true),
    INIT_STAGE(INIT_STAGE_ARCH_AP_INIT, fn_init_stage_arch_ap_init, true),
    INIT_STAGE(INIT_STAGE_ARCH_USERSPACE, fn_init_stage_arch_userspace, true),
    INIT_STAGE(INIT_STAGE_VFS, fn_init_stage_vfs, true),
    INIT_STAGE(INIT_STAGE_USERSPACE, fn_init_stage_userspace, true),
};

#undef INIT_STAGE

bool should_run_stage(bool is_bsp, bool bsp_only) {
    return (is_bsp && bsp_only) || (!is_bsp && !bsp_only);
}

void run_stage(init_stage_t stage, uint32_t core_id, bool is_bsp) {
    for(size_t i = 0; i < sizeof(g_init_stage_handlers) / sizeof(init_stage_handler_t); i++) {
        if(g_init_stage_handlers[i].stage == stage) {
            if(!should_run_stage(is_bsp, g_init_stage_handlers[i].bsp_only)) { continue; }

            g_init_stage_handlers[i].handler(core_id);
            LOG_OKAY("finished stage %s (%i) on %s\n", init_stage_to_str(stage), stage, is_bsp ? "bsp" : "ap");
            return;
        }
    }
    LOG_WARN("No handler found for stage %s (%i)\n", init_stage_to_str(stage), stage);
}

void arch_init_bsp() {
    ATOMIC_STORE(&g_arch_bsp_finished, 0, ATOMIC_RELAXED);

    run_stage(INIT_STAGE_BASE_MEM, 0, true);
    run_stage(INIT_STAGE_ARCH_CPU, 0, true);
    run_stage(INIT_STAGE_TIME, 0, true);
    run_stage(INIT_STAGE_ACPI_BOOTSTRAP, 0, true);
    run_stage(INIT_STAGE_ARCH_PLATFORM, 0, true);
    run_stage(INIT_STAGE_ARCH_AP_INIT, 0, true);
    run_stage(INIT_STAGE_ARCH_USERSPACE, 0, true);
    run_stage(INIT_STAGE_VFS, 0, true);
    run_stage(INIT_STAGE_USERSPACE, 0, true);

    ATOMIC_STORE(&g_arch_bsp_finished, 1, ATOMIC_RELAXED);

    sched_arch_handoff();
}

void arch_init_ap(uint32_t core_id) {
    run_stage(INIT_STAGE_BASE_MEM, core_id, false);
    run_stage(INIT_STAGE_ARCH_CPU, core_id, false);
    run_stage(INIT_STAGE_TIME, core_id, false);
    run_stage(INIT_STAGE_ACPI_BOOTSTRAP, core_id, false);
    run_stage(INIT_STAGE_ARCH_PLATFORM, core_id, false);
    run_stage(INIT_STAGE_ARCH_AP_INIT, core_id, false);
    run_stage(INIT_STAGE_ARCH_USERSPACE, core_id, false);
    run_stage(INIT_STAGE_VFS, core_id, false);
    run_stage(INIT_STAGE_USERSPACE, core_id, false);

    ATOMIC_STORE(&g_arch_ap_finished, 1, ATOMIC_RELAXED);
    while(ATOMIC_LOAD(&g_arch_ap_finished, ATOMIC_ACQUIRE) == 0) { arch_relax(); }

    while(1);
}
