#include <arch/cpu_local.h>
#include <arch/hardware/fpu.h>
#include <arch/hardware/lapic.h>
#include <arch/interrupts.h>
#include <arch/msr.h>
#include <arch/sched/thread.h>
#include <assert.h>
#include <common/arch.h>
#include <common/interrupts/interrupt.h>
#include <common/sched/sched.h>
#include <common/sched/thread.h>
#include <common/userspace/process.h>
#include <common/userspace/syscalls/sys_futex.h>
#include <helpers.h>
#include <list.h>
#include <log.h>
#include <memory/heap.h>
#include <memory/memory.h>
#include <memory/ptm.h>
#include <memory/vm.h>
#include <spinlock.h>
#include <string.h>

#define LAPIC_TIMER_VECTOR 0x20

typedef struct [[gnu::packed]] {
    uint64_t r12, r13, r14, r15, rbp, rbx;
    virt_addr_t thread_init;
    virt_addr_t entry;
    virt_addr_t thread_exit;

    uint64_t rbp_0;
    uint64_t rip_0;
} init_stack_kernel_t;

typedef struct [[gnu::packed]] {
    uint64_t r12, r13, r14, r15, rbp, rbx;
    virt_addr_t thread_init;
    virt_addr_t thread_init_user;
    virt_addr_t entry;
    virt_addr_t user_stack;
} init_stack_user_t;

extern x86_64_thread_t* x86_64_context_switch(x86_64_thread_t* t_current, x86_64_thread_t* t_next);
extern void x86_64_userspace_init_sysexit();

static void sched_timer_handler(arch_interrupts_frame_t* frame) {
    (void) frame;

    sys_futex_check_timeouts();
    CPU_LOCAL_WRITE(preempt.yield_pending, true);
}

void internal_sched_thread_drop(thread_t* thread) {
    if(thread == thread->sched->idle_thread) return;

    // @todo: reap
    switch(thread->state) {
        case THREAD_STATE_READY:   sched_thread_schedule(thread); break;
        case THREAD_STATE_DEAD:    break;
        case THREAD_STATE_BLOCKED: break;
        default:                   assertf(false, "invalid state on drop %d", thread->state);
    }
}

void internal_thread_init_common(x86_64_thread_t* prev) { // NOLINT
    internal_sched_thread_drop(&prev->common);
    (void) arch_enable_interupts();
    sched_arch_reset_preempt_timer();
}

void sched_arch_reset_preempt_timer() {
    arch_lapic_timer_oneshot_ms(10);
}

void internal_thread_exit_kernel() {
    sched_yield(THREAD_STATE_DEAD);
}


void sched_arch_init_bsp() {
    interrupt_set_handler(LAPIC_TIMER_VECTOR, sched_timer_handler);
}

x86_64_thread_t* sched_arch_create_thread_common(size_t tid, void* process, scheduler_t* sched, virt_addr_t kernel_stack_top, virt_addr_t stack) {
    x86_64_thread_t* thread = heap_alloc(sizeof(x86_64_thread_t));
    thread->common.tid = tid;
    thread->common.state = THREAD_STATE_READY;
    thread->common.sched = sched;
    thread->stack = stack;
    thread->kernel_stack_top = kernel_stack_top;
    thread->common.process = process;
    thread->fpu_area = nullptr;
    thread->fsbase = 0;
    thread->gsbase = 0;
    if(process) { thread->fpu_area = arch_fpu_alloc_area(); }

    LOG_INFO("Created thread with tid %u\n", tid);
    return thread;
}

thread_t* sched_arch_create_thread_kernel(virt_addr_t entry) {
    virt_addr_t kernel_stack_top = (virt_addr_t) vm_map_anon(g_vm_global_address_space, VM_NO_HINT, 16 * PAGE_SIZE_DEFAULT, VM_PROT_RW, VM_CACHE_NORMAL, VM_FLAG_NONE);

    init_stack_kernel_t* init_stack = (init_stack_kernel_t*) (kernel_stack_top - sizeof(init_stack_kernel_t));
    memset(init_stack, 0, sizeof(init_stack_kernel_t));
    init_stack->entry = entry;
    init_stack->thread_init = (virt_addr_t) internal_thread_init_common;
    init_stack->thread_exit = (virt_addr_t) internal_thread_exit_kernel;

    return &sched_arch_create_thread_common(process_allocate_id(), nullptr, &CPU_LOCAL_READ(self)->sched, kernel_stack_top, (uintptr_t) init_stack)->common;
}

thread_t* sched_arch_create_thread_user(process_t* process, virt_addr_t user_stack_top, virt_addr_t entry, bool inherit_pid) {
    virt_addr_t kernel_stack_top = (virt_addr_t) vm_map_anon(g_vm_global_address_space, VM_NO_HINT, 16 * PAGE_SIZE_DEFAULT, VM_PROT_RW, VM_CACHE_NORMAL, true) + (16 * PAGE_SIZE_DEFAULT);

    init_stack_user_t* init_stack = (init_stack_user_t*) (kernel_stack_top - sizeof(init_stack_user_t));
    init_stack->entry = entry;
    init_stack->thread_init = (virt_addr_t) internal_thread_init_common;
    init_stack->thread_init_user = (virt_addr_t) x86_64_userspace_init_sysexit;
    init_stack->entry = entry;
    init_stack->user_stack = user_stack_top;

    uint32_t tid = inherit_pid ? process->pid : process_allocate_id();
    return &sched_arch_create_thread_common(tid, process, &CPU_LOCAL_READ(self)->sched, kernel_stack_top, (uintptr_t) init_stack)->common;
}

thread_t* sched_arch_thread_current() {
    x86_64_thread_t* thread = CPU_LOCAL_GET_CURRENT_THREAD();
    assert(thread != nullptr);
    return &thread->common;
}

void sched_arch_context_switch(thread_t* t_current, thread_t* t_next) {
    LOG_STRC("current=%u, next=%u\n", t_current->tid, t_next->tid);
    x86_64_thread_t* current = CONTAINER_OF(t_current, x86_64_thread_t, common);
    x86_64_thread_t* next = CONTAINER_OF(t_next, x86_64_thread_t, common);

    CPU_LOCAL_WRITE(current_thread, next);
    interrupt_set_usermode_stack(next->stack);
    if(current->common.process) { arch_fpu_save(current->fpu_area); }
    if(next->common.process) { arch_fpu_load(next->fpu_area); }
    if(current->common.process && next->common.process && current->common.process != next->common.process) {
        ptm_load_address_space(next->common.process->address_space);
    } else if(next->common.process) {
        ptm_load_address_space(next->common.process->address_space);
    }

    current->gsbase = arch_msr_read_msr(IA32_KERNEL_GS_BASE_MSR);
    current->fsbase = arch_msr_read_msr(IA32_FS_BASE_MSR);

    arch_msr_write_msr(IA32_KERNEL_GS_BASE_MSR, next->gsbase);
    arch_msr_write_msr(IA32_FS_BASE_MSR, next->fsbase);

    x86_64_thread_t* prev = x86_64_context_switch(current, next);
    internal_sched_thread_drop(&prev->common);
}
