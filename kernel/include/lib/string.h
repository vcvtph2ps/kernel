#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * @brief Computes the length of a null terminated string
 * @param s The input string
 * @return The number of characters in the string, excluding the null terminator
 */
size_t strlen(const char* s); // NOLINT

/**
 * @brief Copies n bytes from memory area src to memory area dest
 * @param dest The destination memory area
 * @param src The source memory area
 * @param n The number of bytes to copy
 * @return A pointer to the destination memory area (dest)
 */
void* memcpy(void* restrict dest, const void* restrict src, size_t n); // NOLINT

/**
 * @brief Fills the first n bytes of the memory area pointed to by s with the constant byte c
 * @param s The memory area to fill
 * @param c The byte value to set (passed as int, but converted to unsigned char)
 * @param n The number of bytes to set
 * @return A pointer to the memory area s
 */
void* memset(void* s, int c, size_t n); // NOLINT

/**
 * @brief Copies n bytes from src to dest, handling overlapping memory areas safely
 * @param dest The destination memory area
 * @param src The source memory area
 * @param n The number of bytes to copy
 * @return A pointer to the destination memory area (dest)
 */
void* memmove(void* dest, const void* src, size_t n); // NOLINT

/**
 * @brief Compares the first n bytes of two memory areas
 * @param s1 The first memory area
 * @param s2 The second memory area
 * @param n The number of bytes to compare
 * @return An integer less than, equal to, or greater than zero if s1 is found to be less than, equal to, or greater than s2
 */
int memcmp(const void* s1, const void* s2, size_t n); // NOLINT

/**
 * @brief Compares two null terminated strings
 * @param s1 The first string
 * @param s2 The second string
 * @return An integer less than, equal to, or greater than zero if s1 is found to be less than, equal to, or greater than s2
 */
int strcmp(const char* s1, const char* s2); // NOLINT

/**
 * @brief Compares up to n characters of two null terminated strings
 * @param s1 The first string
 * @param s2 The second string
 * @param n The maximum number of characters to compare
 * @return An integer less than, equal to, or greater than zero if s1 is found to be less than, equal to, or greater than s2
 */
int strncmp(const char* s1, const char* s2, size_t n); // NOLINT
