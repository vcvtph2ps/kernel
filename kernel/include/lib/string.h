#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

size_t strlen(const char* s); // NOLINT
void* memcpy(void* restrict dest, const void* restrict src, size_t n); // NOLINT
void* memset(void* s, int c, size_t n); // NOLINT
void* memmove(void* dest, const void* src, size_t n); // NOLINT
int memcmp(const void* s1, const void* s2, size_t n); // NOLINT
int strcmp(const char* s1, const char* s2); // NOLINT
int strncmp(const char* s1, const char* s2, size_t n); // NOLINT
