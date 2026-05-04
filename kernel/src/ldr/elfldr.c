#include <ldr/elfldr.h>
#include <memory/heap.h>
#include <memory/memory.h>
#include <stdint.h>
#include <string.h>

#include "log.h"

typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} elf64_elf_header_t;

#define ELF_CLASS_IDX 4
#define ELF_CLASS_64_BIT 2

#define ELF_DATA_IDX 5
#define ELF_DATA_2LSB 1

#define ETYPE_REL 1
#define ETYPE_EXEC 2
#define ETYPE_DYN 3

#define EMACHINE_X86_64 62

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} elf64_program_header_t;

#define PTYPE_LOAD 1
#define PTYPE_INTERP 3
#define PTYPE_PHDR 6

#define PFLAGS_EXECUTE (1 << 0)
#define PFLAGS_WRITE (1 << 1)
#define PFLAGS_READ (1 << 2)


bool elf_supported(const elf64_elf_header_t* elf_header) {
    if(!elf_header) {
        return false;
    }
    if(elf_header->e_ident[0] != 0x7f || elf_header->e_ident[1] != 'E' || elf_header->e_ident[2] != 'L' || elf_header->e_ident[3] != 'F') {
        return false;
    }
    if(elf_header->e_ident[ELF_CLASS_IDX] != ELF_CLASS_64_BIT) {
        return false;
    }
    if(elf_header->e_ident[ELF_DATA_IDX] != ELF_DATA_2LSB) {
        return false;
    }
    if(elf_header->e_machine != EMACHINE_X86_64) {
        return false;
    }
    if(elf_header->e_type != ETYPE_EXEC && elf_header->e_type != ETYPE_DYN) {
        return false;
    }
    return true;
}

typedef struct {
    uintptr_t image_start_address;
    uintptr_t image_end_address;
    uintptr_t image_offset;
} elf_image_allocation;

bool internal_allocate_for_image(vm_address_space_t* address_space, const elf64_elf_header_t* elf_header, const elf64_program_header_t* phdr_cache, elf_image_allocation* out_allocation) {
    elf_image_allocation allocation = { 0 };
    allocation.image_start_address = UINTPTR_MAX;
    allocation.image_end_address = 0;

    for(size_t i = 0; i < elf_header->e_phnum; i++) {
        if(phdr_cache[i].p_type == PTYPE_LOAD) {
            if(phdr_cache[i].p_vaddr < allocation.image_start_address) {
                allocation.image_start_address = phdr_cache[i].p_vaddr;
            }
            if(phdr_cache[i].p_vaddr + phdr_cache[i].p_memsz > allocation.image_end_address) {
                allocation.image_end_address = phdr_cache[i].p_vaddr + phdr_cache[i].p_memsz;
            }
        }
    }

    LOG_STRC("lowest_address = 0x%lx, highest_address = 0x%lx\n", allocation.image_start_address, allocation.image_end_address);

    uintptr_t target_allocation = allocation.image_start_address + allocation.image_offset;
    size_t image_size = ALIGN_UP((allocation.image_end_address - allocation.image_start_address), PAGE_SIZE_DEFAULT);
    LOG_STRC("target_allocation = 0x%lx, image_size = 0x%lx\n", target_allocation, image_size);

    if(elf_header->e_type == ETYPE_DYN) {
        uintptr_t allocated = (virt_addr_t) vm_map_anon(address_space, VM_NO_HINT, image_size, VM_PROT_RW, VM_CACHE_NORMAL, VM_FLAG_ZERO);
        assert(allocated != 0 && "Failed to allocate memory for elf image");
        allocation.image_offset = allocated;
    } else {
        LOG_STRC("target_allocation = 0x%lx\n", target_allocation);
        virt_addr_t result_vaddr = (virt_addr_t) vm_map_anon(address_space, (void*) target_allocation, image_size, VM_PROT_RW, VM_CACHE_NORMAL, VM_FLAG_ZERO | VM_FLAG_FIXED);
        assert(result_vaddr != 0 && "Failed to allocate memory for elf image");
    }
    LOG_STRC("image_slide = 0x%lx, image_size = 0x%lx\n", allocation.image_offset, image_size);

    out_allocation->image_start_address = allocation.image_start_address;
    out_allocation->image_end_address = allocation.image_end_address;
    out_allocation->image_offset = allocation.image_offset;
    return true;
}

void internal_elf_handle_pt_load(vm_address_space_t* address_space, vfs_path_t* path, elf64_program_header_t* phdr, elf_image_allocation* allocation) {
    vm_protection_t flags = VM_PROT_NO_ACCESS;
    if(phdr->p_flags & PFLAGS_READ) {
        flags.read = true;
    }
    if(phdr->p_flags & PFLAGS_WRITE) {
        flags.write = true;
    }
    if(phdr->p_flags & PFLAGS_EXECUTE) {
        flags.execute = true;
    }
    virt_addr_t start_vaddr = MATH_FLOOR(allocation->image_offset + phdr->p_vaddr, PAGE_SIZE_DEFAULT);
    virt_addr_t end_vaddr = ALIGN_UP(allocation->image_offset + phdr->p_vaddr + phdr->p_memsz, PAGE_SIZE_DEFAULT);
    for(virt_addr_t j = start_vaddr; j < end_vaddr; j += PAGE_SIZE_DEFAULT) {
        vm_rewrite_prot(address_space, (void*) j, PAGE_SIZE_DEFAULT, flags);
    }

    virt_addr_t phdr_data = (virt_addr_t) heap_alloc(phdr->p_filesz);
    assert(vfs_read(path, (void*) phdr_data, phdr->p_filesz, phdr->p_offset, nullptr) == VFS_RESULT_OK && "Failed to read program header data");
    vm_copy_to(address_space, allocation->image_offset + phdr->p_vaddr, (void*) phdr_data, phdr->p_filesz);
    heap_free((void*) phdr_data, phdr->p_filesz);
}

bool internal_elf_handle_pt_interp(vm_address_space_t* address_space, vfs_path_t* path, elf64_program_header_t* phdr, elfldr_elf_loader_info_t* loader_info) {
    virt_addr_t phdr_data = (virt_addr_t) heap_alloc(phdr->p_filesz);
    assert(vfs_read(path, (void*) phdr_data, phdr->p_filesz, phdr->p_offset, nullptr) == VFS_RESULT_OK && "Failed to read program header data");
    LOG_STRC("interpreter: %s\n", (const char*) phdr_data);

    elfldr_elf_loader_info_t* interp_loader_info;
    if(!elfldr_load_file(address_space, &VFS_MAKE_ABS_PATH((const char*) phdr_data), &interp_loader_info)) {
        heap_free((void*) phdr_data, phdr->p_filesz);
        return false;
    }

    loader_info->executable_entry_point = interp_loader_info->executable_entry_point;
    loader_info->interp_base = interp_loader_info->image_offset;
    heap_free(interp_loader_info, sizeof(elfldr_elf_loader_info_t));

    heap_free((void*) phdr_data, phdr->p_filesz);
    return true;
}

bool internal_elf_load_image(vm_address_space_t* address_space, const elf64_elf_header_t* elf_header, vfs_path_t* path, elfldr_elf_loader_info_t* loader_info) {
    elf_image_allocation allocation = { 0 };

    // cache phdrs so we don't have to read them multiple times
    elf64_program_header_t* phdr_cache = heap_alloc(sizeof(elf64_program_header_t) * elf_header->e_phnum);
    for(size_t i = 0; i < elf_header->e_phnum; i++) {
        assert(vfs_read(path, &phdr_cache[i], sizeof(elf64_program_header_t), elf_header->e_phoff + i * elf_header->e_phentsize, nullptr) == VFS_RESULT_OK && "Failed to read program header");
        LOG_STRC("phdr[%d].p_type = 0x%lx\n", i, phdr_cache[i].p_type);
        LOG_STRC("phdr[%d].p_vaddr = 0x%lx, p_memsz = 0x%lx, p_filesz = 0x%lx\n", i, phdr_cache[i].p_vaddr, phdr_cache[i].p_memsz, phdr_cache[i].p_filesz);
    }

    // allocate image memory
    assert(internal_allocate_for_image(address_space, elf_header, phdr_cache, &allocation) && "Failed to allocate memory for elf image");

    // fill out loader info
    loader_info->image_offset = allocation.image_offset;
    loader_info->executable_entry_point = allocation.image_offset + elf_header->e_entry;
    loader_info->image_entry_point = allocation.image_offset + elf_header->e_entry;

    loader_info->phnum = elf_header->e_phnum;
    loader_info->phentsize = elf_header->e_phentsize;
    for(size_t i = 0; i < elf_header->e_phnum; i++) {
        if(phdr_cache[i].p_type == PTYPE_PHDR) {
            loader_info->phdr_table = allocation.image_offset + phdr_cache[i].p_vaddr;
            break;
        }
    }

    if(loader_info->phdr_table == 0) {
        loader_info->phdr_table = allocation.image_offset + elf_header->e_phoff;
    }

    // load phdrs
    for(size_t i = 0; i < elf_header->e_phnum; i++) {
        if(phdr_cache[i].p_type != PTYPE_LOAD && phdr_cache[i].p_type != PTYPE_INTERP) {
            continue;
        }
        LOG_STRC("Loading segment %zu: vaddr=0x%lx, size=%zu\n", i, allocation.image_offset + phdr_cache[i].p_vaddr, phdr_cache[i].p_filesz);
        if(phdr_cache[i].p_type == PTYPE_LOAD) internal_elf_handle_pt_load(address_space, path, &phdr_cache[i], &allocation);
        if(phdr_cache[i].p_type == PTYPE_INTERP) {
            if(!internal_elf_handle_pt_interp(address_space, path, &phdr_cache[i], loader_info)) {
                heap_free(phdr_cache, sizeof(elf64_program_header_t) * elf_header->e_phnum);
                return false;
            }
        }
    }

    heap_free(phdr_cache, sizeof(elf64_program_header_t) * elf_header->e_phnum);
    return true;
}

bool elfldr_load_file(vm_address_space_t* address_space, vfs_path_t* path, elfldr_elf_loader_info_t** out_loader_info) {
    vfs_node_attr_t attr;
    if(vfs_attr(path, &attr) != VFS_RESULT_OK) {
        LOG_FAIL("Failed to get size of elf file\n");
        return false;
    }

    assert(attr.type == VFS_NODE_TYPE_FILE && attr.size >= sizeof(elf64_elf_header_t));

    elf64_elf_header_t* elf_data = heap_alloc(sizeof(elf64_elf_header_t));
    if(vfs_read(path, elf_data, sizeof(elf64_elf_header_t), 0, nullptr) != VFS_RESULT_OK) {
        LOG_FAIL("Failed to read elf header\n");
        heap_free(elf_data, sizeof(elf64_elf_header_t));
        return false;
    }
    if(!elf_supported(elf_data)) {
        LOG_FAIL("Unsupported elf file\n");
        heap_free(elf_data, sizeof(elf64_elf_header_t));
        return false;
    }

    elfldr_elf_loader_info_t* loader_info = heap_alloc(sizeof(elfldr_elf_loader_info_t));
    if(!loader_info) {
        LOG_FAIL("Failed to allocate loader info\n");
        heap_free(elf_data, sizeof(elf64_elf_header_t));
        return false;
    }

    memset(loader_info, 0, sizeof(elfldr_elf_loader_info_t));

    if(!internal_elf_load_image(address_space, elf_data, path, loader_info)) {
        LOG_FAIL("Failed to load elf image\n");
        heap_free(elf_data, sizeof(elf64_elf_header_t));
        heap_free(loader_info, sizeof(elfldr_elf_loader_info_t));
        return false;
    }

    *out_loader_info = loader_info;
    return true;
}
