#ifndef __MM_H__
#define __MM_H__

#include <stdint.h>
#include "list.h"

#define PAGE_SHIFT      12
#define PAGE_SIZE       (1 << PAGE_SHIFT) // 4096
#define MAX_ORDER       10

struct page {
    struct list_head list;
    int order;
    int allocated; // 0: Free, 1: Allocated, 2: Reserved
    uint64_t pfn;

    void *freelist;
    int pool_size;   // 0 for buddy system page
    int free_count;
};

void mm_init(void *dtb);

struct page *alloc_pages(int order);
void free_pages(struct page *page, int order);
uint64_t page_to_phys(struct page *page);

void *kmalloc(size_t size);
void kfree(void *ptr);

void memory_reserve(uint64_t start, uint64_t end);

void dump_buddy_info();

void mm_test();

void test_alloc_1();

#endif