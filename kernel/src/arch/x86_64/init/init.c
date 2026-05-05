#include <common/arch.h>
#include <common/init.h>
#include <common/sched/sched.h>
#include <lib/log.h>

ATOMIC uint32_t g_init_bsp_finished = 0;
ATOMIC uint32_t g_init_ap_finished = 0;

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
} init_stage_handler_t;

#define INIT_STAGE(STAGE, HANDLER) { .stage = (STAGE), .handler = (HANDLER) }

init_stage_handler_t g_init_stage_handlers[] = {
    INIT_STAGE(INIT_STAGE_BASE_MEM, init_stage_base_mem),
    INIT_STAGE(INIT_STAGE_ARCH_CPU, init_stage_arch_cpu),
    INIT_STAGE(INIT_STAGE_TIME, init_stage_time),

    INIT_STAGE(INIT_STAGE_ACPI_BOOTSTRAP, init_stage_acpi_bootstrap),
    INIT_STAGE(INIT_STAGE_ARCH_PLATFORM, init_stage_arch_platform),
    INIT_STAGE(INIT_STAGE_ARCH_AP_INIT, init_stage_arch_ap_init),
    INIT_STAGE(INIT_STAGE_ARCH_USERSPACE, init_stage_arch_userspace),
    INIT_STAGE(INIT_STAGE_VFS, init_stage_vfs),
    INIT_STAGE(INIT_STAGE_USERSPACE, init_stage_userspace),
};

#undef INIT_STAGE

void run_stage(init_stage_t stage, uint32_t core_id) {
    for(size_t i = 0; i < sizeof(g_init_stage_handlers) / sizeof(init_stage_handler_t); i++) {
        if(g_init_stage_handlers[i].stage == stage) {
            g_init_stage_handlers[i].handler(core_id);
            LOG_OKAY("finished stage %s (%i) on %s\n", init_stage_to_str(stage), stage, INIT_CORE_IS_BSP(core_id) ? "bsp" : "ap");
            return;
        }
    }
    LOG_WARN("No handler found for stage %s (%i)\n", init_stage_to_str(stage), stage);
}

void arch_init_bsp() {
    ATOMIC_STORE(&g_init_bsp_finished, 0, ATOMIC_RELAXED);

    run_stage(INIT_STAGE_BASE_MEM, 0);
    run_stage(INIT_STAGE_ARCH_CPU, 0);
    run_stage(INIT_STAGE_TIME, 0);
    run_stage(INIT_STAGE_ACPI_BOOTSTRAP, 0);
    run_stage(INIT_STAGE_ARCH_PLATFORM, 0);
    run_stage(INIT_STAGE_ARCH_AP_INIT, 0);
    run_stage(INIT_STAGE_ARCH_USERSPACE, 0);
    run_stage(INIT_STAGE_VFS, 0);
    run_stage(INIT_STAGE_USERSPACE, 0);

    ATOMIC_STORE(&g_init_bsp_finished, 1, ATOMIC_RELAXED);

    sched_arch_handoff();
    while(1);
}

void arch_init_ap(uint32_t core_id) {
    run_stage(INIT_STAGE_BASE_MEM, core_id);
    run_stage(INIT_STAGE_ARCH_CPU, core_id);
    run_stage(INIT_STAGE_TIME, core_id);
    run_stage(INIT_STAGE_ACPI_BOOTSTRAP, core_id);
    run_stage(INIT_STAGE_ARCH_PLATFORM, core_id);
    run_stage(INIT_STAGE_ARCH_AP_INIT, core_id);
    run_stage(INIT_STAGE_ARCH_USERSPACE, core_id);
    run_stage(INIT_STAGE_VFS, core_id);
    run_stage(INIT_STAGE_USERSPACE, core_id);

    ATOMIC_STORE(&g_init_ap_finished, 1, ATOMIC_RELAXED);
    while(ATOMIC_LOAD(&g_init_ap_finished, ATOMIC_ACQUIRE) == 0) {
        arch_relax();
    }

    // sched_arch_handoff();
    while(1);
}
