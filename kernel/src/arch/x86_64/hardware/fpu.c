#include <arch/internal/cpuid.h>
#include <arch/internal/cr.h>
#include <assert.h>
#include <log.h>
#include <memory/vm.h>
#include <stddef.h>

size_t g_fpu_area_size;
void (*g_fpu_save)(void* ptr);
void (*g_fpu_load)(void* ptr);

void arch_fpu_save(void* ptr) {
    g_fpu_save(ptr);
}

void arch_fpu_load(void* ptr) {
    g_fpu_load(ptr);
}

[[clang::no_stack_protector]] static inline void xsave(void* ptr) {
    uint32_t eax, edx;
    asm volatile("xgetbv\nxsave (%2)" : "=&a"(eax), "=&d"(edx) : "r"(ptr), "c"(0) : "memory");
}

[[clang::no_stack_protector]] static inline void xrstor(void* ptr) {
    uint32_t eax, edx;
    asm volatile("xgetbv\nxrstor (%2)" : "=&a"(eax), "=&d"(edx) : "r"(ptr), "c"(0) : "memory");
}

[[clang::no_stack_protector]] static inline void fxsave(void* ptr) {
    asm volatile("fxsave (%0)" : : "r"(ptr) : "memory");
}

[[clang::no_stack_protector]] static inline void fxrstor(void* ptr) {
    asm volatile("fxrstor (%0)" : : "r"(ptr) : "memory");
}

void internal_fpu_init() {
    if(!arch_cpuid_is_feature_supported(ARCH_CPUID_FEATURE_FXSR)) { arch_panic("FXSR is not supported"); }

    // Enable SSE & FPU
    uint64_t cr0 = arch_cr_read_cr0();
    cr0 &= ~(1 << 2); // CR0.EM
    cr0 |= (1 << 1); // CR0.MP
    arch_cr_write_cr0(cr0);

    uint64_t cr4 = arch_cr_read_cr4();
    cr4 |= 1 << 9; // CR4.OSFXSR
    cr4 |= 1 << 10; // CR4.OSXMMEXCPT

    if(arch_cpuid_is_feature_supported(ARCH_CPUID_FEATURE_XSAVE)) {
        LOG_OKAY("xsave supported!\n");
        cr4 |= 1 << 18; // CR4.OSXSAVE
        arch_cr_write_cr4(cr4);

        uint64_t xcr0 = 0;
        xcr0 |= 1 << 0; // XCR0.X87
        xcr0 |= 1 << 1; // XCR0.SSE

        if(arch_cpuid_is_feature_supported(ARCH_CPUID_FEATURE_AVX)) {
            LOG_OKAY("avx supported\n");
            xcr0 |= 1 << 2; // XCR0.AVX
        }
        if(arch_cpuid_is_feature_supported(ARCH_CPUID_FEATURE_AVX512)) {
            LOG_OKAY("avx512 supported\n");
            xcr0 |= 1 << 5; // XCR0.opmask
            xcr0 |= 1 << 6; // XCR0.ZMM_Hi256
            xcr0 |= 1 << 7; // XCR0.Hi16_ZMM
        }
        asm volatile("xsetbv" : : "a"(xcr0), "d"(xcr0 >> 32), "c"(0) : "memory");
    } else {
        arch_cr_write_cr4(cr4);
    }
}

void arch_fpu_init_bsp() {
    internal_fpu_init();

    if(arch_cpuid_is_feature_supported(ARCH_CPUID_FEATURE_XSAVE)) {
        g_fpu_area_size = arch_cpuid(0x0d, 0, ARCH_CPUID_ECX);
        LOG_OKAY("using xsave with size 0x%llx\n", g_fpu_area_size);
        assert(g_fpu_area_size > 0);
        g_fpu_save = xsave;
        g_fpu_load = xrstor;
    } else {
        LOG_WARN("using legacy fxsave\n");
        g_fpu_area_size = 512;
        g_fpu_save = fxsave;
        g_fpu_load = fxrstor;
    }
}

void arch_fpu_init_ap() {
    internal_fpu_init();
}


void* arch_fpu_alloc_area() {
    return vm_map_anon_aligned(g_vm_global_address_space, VM_NO_HINT, ALIGN_UP(g_fpu_area_size, 4096), 4096, VM_PROT_RW, VM_CACHE_NORMAL, VM_FLAG_NONE);
}

void arch_fpu_free_area(void* area) {
    vm_unmap(g_vm_global_address_space, (void*) area, ALIGN_UP(g_fpu_area_size, 4096));
}
