#include <arch/cpu_local.h>
#include <arch/hardware/lapic.h>
#include <arch/interrupts.h>
#include <assert.h>
#include <common/arch.h>
#include <common/interrupts/dw.h>
#include <common/interrupts/interrupt.h>
#include <common/sched/sched.h>
#include <log.h>

// @todo: add fred interrupts

interrupt_handler_fn_t g_handlers[256] = {};

void interrupt_set_handler(uint8_t vector, interrupt_handler_fn_t handler) {
    assert(g_handlers[vector] == nullptr && "Interrupt handler already registered for vector");
    g_handlers[vector] = handler;
}

extern void idt_init_bsp();
extern void idt_init_ap();
extern void idt_set_usermode_stack(uint64_t stack_pointer);

void interrupt_init_bsp() {
    idt_init_bsp();
}

void interrupt_init_ap() {
    idt_init_ap();
}

void interrupt_set_usermode_stack(uint64_t stack_pointer) {
    idt_set_usermode_stack(stack_pointer);
}

// @todo: soft ints and threaded shit
void x86_64_dispatch_interrupt(arch_interrupts_frame_t* frame) {
    bool is_threaded = CPU_LOCAL_READ(preempt.threaded);
    bool is_outmost_handler = false;

    if(is_threaded) {
        is_outmost_handler = !CPU_LOCAL_GET_CURRENT_THREAD()->common.in_interrupt_handler;
        if(is_outmost_handler) { CPU_LOCAL_GET_CURRENT_THREAD()->common.in_interrupt_handler = true; }

        sched_preempt_disable();
        dw_status_disable();
    }

    if(g_handlers[frame->vector] == nullptr && frame->vector < 0x20) { arch_panic_int(frame); }

    if(g_handlers[frame->vector] != nullptr) g_handlers[frame->vector](frame);
    if(frame->vector >= 32) arch_lapic_eoi();

    if(is_threaded) {
        (void) arch_enable_interupts();
        dw_status_enable();
        (void) arch_disable_interupts();

        sched_preempt_enable();

        if(is_outmost_handler) { CPU_LOCAL_GET_CURRENT_THREAD()->common.in_interrupt_handler = false; }
    }
}
