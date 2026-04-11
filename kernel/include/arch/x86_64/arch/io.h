#pragma once
#include <stdint.h>

/**
 * @brief Writes a byte to an I/O port.
 * @param port The I/O port to write to.
 * @param value The byte value to write.
 */
void arch_io_port_write_u8(uint16_t port, uint8_t value);

/**
 * @brief Writes a word to an I/O port.
 * @param port The I/O port to write to.
 * @param value The word value to write.
 */
void arch_io_port_write_u16(uint16_t port, uint16_t value);

/**
 * @brief Writes a dword to an I/O port.
 * @param port The I/O port to write to.
 * @param value The dword value to write.
 */
void arch_io_port_write_u32(uint16_t port, uint32_t value);

/**
 * @brief Reads a byte from an I/O port.
 * @param port The I/O port to write to.
 * @return The byte value read from the I/O port.
 */
uint8_t arch_io_port_read_u8(uint16_t port);

/**
 * @brief Reads a byte from an I/O port.
 * @param port The I/O port to write to.
 * @return The byte value read from the I/O port.
 */
uint16_t arch_io_port_read_u16(uint16_t port);

/**
 * @brief Reads a dword from an I/O port.
 * @param port The I/O port to write to.
 * @return The dword value read from the I/O port.
 */
uint32_t arch_io_port_read_u32(uint16_t port);
