#include "../../include/kernel/pmm.h"

mem_region_state_t* free_regions;
size_t free_regions_size;

free_chunk_t free_bin = (free_chunk_t){
    .size = 0,
    .front = &free_bin,
    .back = &free_bin
};

int printf(const char* restrict, ...);

void PMM_init(){
    size_t mmap_entries = multiboot_info->mmap_length / sizeof(mmap_entry_t);

    mmap_entry_t* mmap = (mmap_entry_t*)(long)multiboot_info->mmap_addr;

    printf("MB ADDRESS: %p\n", multiboot_info->mmap_addr);
    if (multiboot_info->flags >> 9 & 0x1)
    printf("MB string: %s\n", (char*)multiboot_info->boot_loader_name);

    /* stack-allocated since no dynamic allocation ... yet >:) */
    mmap_entry_t* free_mem_regions[mmap_entries];

    /* save all the free memory regions */
    size_t free_region_count = 0;
    for (size_t i = 0; i < mmap_entries; ++i)
        if (mmap[i].type == MULTIBOOT_MEMORY_AVAILABLE){
            printf("hey");
            free_mem_regions[free_region_count++] = mmap + i;
        }

    free_regions_size = free_region_count * sizeof(mem_region_state_t);

    for (size_t region_num = 0; region_num < free_region_count; ++region_num){
        mmap_entry_t* entry = free_mem_regions[region_num];
        if (entry->length_low > free_regions_size){
            
            free_regions = (mem_region_state_t*)(long)entry->address_low;
            entry->address_low += free_regions_size;

            for (uint32_t region = 0; region <= free_region_count; ++region){
                free_regions[region] = (mem_region_state_t){
                    .addr = entry->address_low,
                    .len = entry->length_low,
                    .used = PMM_REGION_FREE
                };
            }
        }
        break;
    }
}

void* kmalloc(size_t size){
    if (!size)
        return NULL;
    
    /* round to nearest multiple of 8 */
    size += sizeof(allocated_chunk_t) + ((size + sizeof(allocated_chunk_t)) % 8);

    if (size < sizeof(free_chunk_t))
        size = sizeof(free_chunk_t);

    free_chunk_t* free_chunk = free_bin.front;

    while (free_chunk != &free_bin){
        if (free_chunk->size == size){
            /* unlink the chunk from the list */
            free_chunk->back->front = free_chunk->front;
            free_chunk->front->back = free_chunk->back;

            return free_chunk + sizeof(allocated_chunk_t);
        }
        /* we can split the chunk and have enough space for a free chunk */
        if (free_chunk->size > size && free_chunk->size - size - sizeof(allocated_chunk_t) > sizeof(free_chunk_t)){
            free_chunk->size -= size + sizeof(allocated_chunk_t);
            allocated_chunk_t* alloc_chunk = (allocated_chunk_t*)(free_chunk + free_chunk->size);
            alloc_chunk->size = size;
        }
        free_chunk = free_chunk->front;
    }

    /* we cant find a free chunk, make one */
    for (size_t i = 0; i < free_regions_size; ++i){
        mem_region_state_t* region = free_regions + i;

        if (region->len - region->used > size){
            allocated_chunk_t* new_chunk = (allocated_chunk_t*)(region->addr + region->used);
            new_chunk->size = size;
            region->used += size;
            return &new_chunk->data;
        }
    }

    /* lmfao */
    return NULL;
}

void kfree(void* ptr) {
    /* add the chunk back into the list */
    free_chunk_t* free_chunk = ptr - sizeof(allocated_chunk_t);
    free_chunk->back = free_bin.front->back;
    free_bin.front->back = free_chunk;
    free_chunk->front = free_bin.front;
    free_bin.front = free_chunk;
}