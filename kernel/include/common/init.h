#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <lib/helpers.h>

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

extern ATOMIC uint32_t g_arch_bsp_finished;
extern ATOMIC uint32_t g_arch_ap_finished;

void fn_init_stage_base_mem_bsp(uint32_t core_id);
void fn_init_stage_base_mem_ap(uint32_t core_id);
void fn_init_stage_arch_cpu_bsp(uint32_t core_id);
void fn_init_stage_arch_cpu_ap(uint32_t core_id);
void fn_init_stage_time_bsp(uint32_t core_id);
void fn_init_stage_time_ap(uint32_t core_id);
void fn_init_stage_acpi_bootstrap(uint32_t core_id);
void fn_init_stage_arch_platform(uint32_t core_id);
void fn_init_stage_arch_ap_init(uint32_t core_id);
void fn_init_stage_arch_userspace(uint32_t core_id);
void fn_init_stage_vfs(uint32_t core_id);
void fn_init_stage_userspace(uint32_t core_id);
