#include <arch/cpu_local.h>

/**
 * @brief Initializes the CPU local storage for the BSP.
 * This function should be called once during the early init of the kernel on the BSP.
 */
void cpu_local_init_bsp();

/**
 * @brief Initializes the CPU local storage for all APs.
 * This function should be called once during the late init of the kernel on the BSP.
 * @param core_count The total number of CPU cores in the system, including the BSP. This is used to determine how much memory to allocate for the CPU local storage.
 */
void cpu_local_init_storage(uint32_t core_count);


void cpu_local_init_ap(uint32_t core_id);

uint32_t cpu_local_get_core_lapic_id(uint32_t core_id);
