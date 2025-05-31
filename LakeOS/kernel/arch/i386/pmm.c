#include "../../include/kernel/pmm.h"

static uint8_t* bitmap;
/* stores the BITS (blocks) */
static size_t bitmap_len;

#define div_round_up(a, b)     \
    ((a) % (b)) ?              \
    ((a) / (b) + 1) :          \
    ((a) / (b))

#define bitmap_getblockstate(num)             \
    ((bitmap[(num) / 8] >> ((num) % 8)) & 0x1)

void bitmap_setblockstate(size_t block_num, uint8_t state){
    if (state)
        bitmap[block_num / 8] |= 0x1 << (block_num % 8);
    else 
        bitmap[block_num / 8] &= ~(0x1 << (block_num % 8));
}

void PMM_init(){
    mmap_entry_t* regions = (mmap_entry_t*)multiboot_info->mmap_addr;
    size_t num_regions = multiboot_info->mmap_length / sizeof(mmap_entry_t);

    /* determine bytes of total memory */
    for (size_t i = 0; i < num_regions; ++i)
        bitmap_len += regions[i].length_low;
    
    /* round the size down and convert to blocks */
    bitmap_len = bitmap_len / MEMORY_BLOCK_SIZE;

    size_t bitmap_size = div_round_up(bitmap_len, 8);

    /* look for a region that can fit the bitmap */
    for (size_t i = 0; i < num_regions; ++i)
        if (regions[i].type == MULTIBOOT_MEMORY_AVAILABLE && regions[i].length_low >= bitmap_size){
            /* dont have to create a new region since they are marked reserved by default */
            bitmap = (uint8_t*)regions[i].address_low;
            regions[i].address_low += bitmap_size;
            regions[i].length_low  -= bitmap_size;
        }

    /* mark all blocks as reserved */
    if (bitmap)
        memset(bitmap, 0xFF, bitmap_size);
    else
        printf("Not enough free memory for bitmap: Abort\n");

    size_t block_num = 0;

    /* mark the free regions */
    for (size_t i = 0; i < num_regions; ++i)
        if (regions[i].type == MULTIBOOT_MEMORY_AVAILABLE){
            /* round the size of free regions down */
            for (; regions[i].length_low >= MEMORY_BLOCK_SIZE; regions[i].length_low -= MEMORY_BLOCK_SIZE, ++block_num)
                bitmap_setblockstate(block_num, BITMAP_BLOCK_FREE);
            
            --block_num;
        }
    
    block_num = 0;
    /* mark the reserved regions */
    for (size_t i = 0; i < num_regions; ++i){
        if (regions[i].type != MULTIBOOT_MEMORY_AVAILABLE){
            /* round the size of reserved regions up */
            for (; regions[i].length_low > 0; ++block_num){
                bitmap_setblockstate(block_num, BITMAP_BLOCK_USED);

                if (regions[i].length_low >= MEMORY_BLOCK_SIZE)
                    regions[i].length_low -= MEMORY_BLOCK_SIZE;
                else
                    regions[i].length_low = MEMORY_BLOCK_SIZE;
            }
        }
    }
}

void* alloc_page(){
    for (size_t block_num = 0; block_num < bitmap_len; ++block_num){
        if (bitmap_getblockstate(block_num) == BITMAP_BLOCK_FREE){
            bitmap_setblockstate(block_num, BITMAP_BLOCK_USED);
            return (void*)(MEMORY_BLOCK_SIZE * block_num);
        }
    }
    return NULL;
}

void free_page(void* page){
    bitmap_setblockstate((uint32_t)page / MEMORY_BLOCK_SIZE, BITMAP_BLOCK_FREE);
}

void* alloc_pages(size_t n){
    for (size_t block_num = 0; block_num < bitmap_len; ++block_num){
        size_t free_block_size = 0;
        for (; bitmap_getblockstate(block_num + free_block_size) == BITMAP_BLOCK_FREE; ++free_block_size){
            if (free_block_size == n){
                for (; free_block_size > 0; --free_block_size)
                    bitmap_setblockstate(block_num + free_block_size, BITMAP_BLOCK_USED);

                bitmap_setblockstate(block_num, BITMAP_BLOCK_USED);

                return (void*)(block_num * MEMORY_BLOCK_SIZE);
            }
        }
        block_num += free_block_size;
    }

    return NULL;
}

void free_pages(void* page, size_t n){
    for (size_t i = 0; i <= n; ++i)
        bitmap_setblockstate((uint32_t)page / MEMORY_BLOCK_SIZE + i, BITMAP_BLOCK_FREE);
}