#pragma once
#include <stdint.h>

typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
} arch_interrupts_regs_t;

typedef struct {
    uint64_t rip, cs, rflags, rsp, ss;
} arch_interrupts_state_t;

typedef struct {
    uint64_t vector, error, interrupt_data;
    arch_interrupts_regs_t* regs;
    arch_interrupts_state_t* state;
    bool is_user;
} arch_interrupts_frame_t;

// typedef struct fred_frame {
//     arch_interrupts_regs_t regs;
//     uint64_t error;
//     arch_interrupts_state_t state;
//     uint64_t fred_event_data;
//     uint64_t fred_reserved;
// } arch_interrupt_fred_frame_t;
