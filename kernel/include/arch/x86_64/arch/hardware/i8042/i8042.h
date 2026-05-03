#pragma once

#include <stddef.h>
#include <stdint.h>

typedef enum {
    ARCH_I8042_PORT_ONE = 0,
    ARCH_I8042_PORT_TWO = 1
} arch_i8042_port_t;

typedef enum {
    ARCH_I8042_PORT_ONE_IRQ = 1,
    ARCH_I8042_PORT_TWO_IRQ = 12,
} arch_i8042_legacy_irq_t;

/**
 * @brief Initializes the i8042 controller. This should be called once during early initialization
 * @return true if the controller was successfully initialized, otherwise false
 */
bool arch_i8042_init();

/**
 * @brief Enables the specified i8042 port. This should be called before any attempts to read or write to the port.
 * @param port The port to enable
 */
void arch_i8042_port_enable(arch_i8042_port_t port);

/**
 * @brief Reads a byte from the specified i8042 port.
 * @param wait Whether to block and wait for the port to be ready before reading. If wait is false, the function will return immediately and may fail if the port is not ready.
 * @return The byte read from the port
 */
uint8_t arch_i8042_port_read(bool wait);

/**
 * @brief Writes a byte to the specified i8042 port. If wait is true, this function will block until the port is ready to be written to.
 * @param port The port to write to
 * @param value The byte to write to the port
 */
bool arch_i8042_port_write(arch_i8042_port_t port, uint8_t value);
