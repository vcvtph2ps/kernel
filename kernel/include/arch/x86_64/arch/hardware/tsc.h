#pragma once

/**
 * @brief Calibrates the TSC frequency and stores it in the per-CPU data structure. Must be called on each CPU during early initialisation.
 */
void arch_tsc_calibrate();
