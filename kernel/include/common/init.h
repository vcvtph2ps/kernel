#pragma once

#include <lib/helpers.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    INIT_STAGE_BASE_MEM,
    INIT_STAGE_ARCH_CPU,
    INIT_STAGE_TIME,
    INIT_STAGE_ACPI_BOOTSTRAP,
    INIT_STAGE_ARCH_PLATFORM,
    INIT_STAGE_ARCH_AP_INIT,
    INIT_STAGE_ARCH_USERSPACE,
    INIT_STAGE_VFS,
    INIT_STAGE_USERSPACE
} init_stage_t;

ATOMIC extern uint32_t g_init_bsp_finished;
ATOMIC extern uint32_t g_init_ap_finished;

#define INIT_CORE_IS_BSP(CORE_ID) ((CORE_ID) == 0)

void init_stage_base_mem(uint32_t core_id);
void init_stage_arch_cpu(uint32_t core_id);
void init_stage_time(uint32_t core_id);
void init_stage_acpi_bootstrap(uint32_t core_id);
void init_stage_arch_platform(uint32_t core_id);
void init_stage_arch_ap_init(uint32_t core_id);
void init_stage_arch_userspace(uint32_t core_id);
void init_stage_vfs(uint32_t core_id);
void init_stage_userspace(uint32_t core_id);
