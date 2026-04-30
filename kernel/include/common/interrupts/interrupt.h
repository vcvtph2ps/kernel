#include <stdint.h>

typedef void (*interrupt_handler_fn_t)(uint8_t vector);

/**
 * @brief Initialize the interrupt system. This should be called once during the early init of the kernel
 */
void interrupt_init_bsp();

/**
 * @brief Initialize the interrupt system on current AP.
 */
void interrupt_init_ap();

/**
 * @brief Register an interrupt handler for the given interrupt vector.
 * @param vector The interrupt vector to register the handler for.
 * @param handler The function to call when the specified interrupt is triggered.
 * @note handler will run in a hardirq context
 */
void interrupt_set_handler(uint8_t vector, interrupt_handler_fn_t handler);

/**
 * @brief Set the stack pointer to use when handling interrupts in user mode.
 * @param stack_pointer The stack pointer to use. This should point to the top of a valid stack in kernel space.
 */
void interrupt_set_usermode_stack(uint64_t stack_pointer);
