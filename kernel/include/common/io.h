#pragma once
#include <arch/io.h>
#include <common/arch.h>
#include <memory/memory.h>

/**
 * @brief Writes a value to an memory address.
 * @param addr The virtual address to write to. Will fault if the address is invalid/not present/not writeable.
 * @param value The value to write.
 */
void io_mem_write_u8(virt_addr_t addr, uint8_t value);

/**
 * @brief Writes a value to an memory address.
 * @param addr The virtual address to write to. Will fault if the address is invalid/not present/not writeable.
 * @param value The value to write.
 */
void io_mem_write_u16(virt_addr_t addr, uint16_t value);

/**
 * @brief Writes a value to an memory address.
 * @param addr The virtual address to write to. Will fault if the address is invalid/not present/not writeable.
 * @param value The value to write.
 */
void io_mem_write_u32(virt_addr_t addr, uint32_t value);

/**
 * @brief Writes a value to an memory address.
 * @param addr The virtual address to write to. Will fault if the address is invalid/not present/not writeable.
 * @param value The value to write.
 */
void io_mem_write_u64(virt_addr_t addr, uint64_t value);

/**
 * @brief Reads a value from a memory address.
 * @param addr The virtual address to read from. Will fault if the address is invalid/not present.
 * @return The value read from the specified address.
 */
uint8_t io_mem_read_u8(virt_addr_t addr);

/**
 * @brief Reads a value from a memory address.
 * @param addr The virtual address to read from. Will fault if the address is invalid/not present.
 * @return The value read from the specified address.
 */
uint16_t io_mem_read_u16(virt_addr_t addr);

/**
 * @brief Reads a value from a memory address.
 * @param addr The virtual address to read from. Will fault if the address is invalid/not present.
 * @return The value read from the specified address.
 */
uint32_t io_mem_read_u32(virt_addr_t addr);

/**
 * @brief Reads a value from a memory address.
 * @param addr The virtual address to read from. Will fault if the address is invalid/not present.
 * @return The value read from the specified address.
 */
uint64_t io_mem_read_u64(virt_addr_t addr);
