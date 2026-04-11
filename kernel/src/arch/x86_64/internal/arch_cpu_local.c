#include <arch/cpu_local.h>
#include <arch/msr.h>
#include <common/cpu_local.h>
#include <common/irql.h>
#include <memory/memory.h>
#include <string.h>

static volatile arch_cpu_local_t g_bsp_cpu_local = {};

arch_cpu_local_t* g_cpu_local_storage;
uint32_t g_cpu_local_count;

void init_cpu_local_data(arch_cpu_local_t* cpu_local, uint32_t core_id, uint32_t lapic_id) {
    cpu_local->core_id = core_id;
    cpu_local->lapic_id = lapic_id;
    cpu_local->current_irql = IRQL_PASSIVE;
    cpu_local->self = cpu_local;
}

void cpu_local_init_bsp() {
    init_cpu_local_data((arch_cpu_local_t*) &g_bsp_cpu_local, 0, 0);
    arch_msr_write_msr(IA32_GS_BASE_MSR, (uint64_t) &g_bsp_cpu_local);
}

void cpu_local_init_storage(uint32_t core_count) {
    (void) core_count;
    // g_cpu_local_count = core_count;
    // g_cpu_local_storage = (arch_cpu_local_t*) vm_map_anon(g_vm_global_address_space, VM_NO_HINT, ALIGN_UP(sizeof(arch_cpu_local_t) * g_cpu_local_count, PAGE_SIZE_DEFAULT), VM_PROT_RW, VM_CACHE_NORMAL, true);
    // g_bsp_cpu_local.lapic_id = arch_lapic_get_id();
    // memcpy(g_cpu_local_storage, (void*) &g_bsp_cpu_local, sizeof(arch_cpu_local_t));
    // arch_msr_write_msr(IA32_GS_BASE_MSR, (uint64_t) &g_cpu_local_storage[0]);
}

void cpu_local_init_ap(uint32_t core_id) {
    (void) core_id;
    // init_cpu_local_data(&g_cpu_local_storage[core_id], core_id, arch_lapic_get_id());
    // arch_msr_write_msr(IA32_GS_BASE_MSR, (uint64_t) &g_cpu_local_storage[core_id]);
}

uint32_t cpu_local_get_core_lapic_id(uint32_t core_id) {
    return g_cpu_local_storage[core_id].lapic_id;
}
