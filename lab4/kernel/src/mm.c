#include "mm.h"
#include "uart.h"
#include "stdio.h"
#include "utils.h"
#include "fdt.h"
#include "string.h"
#include "initrd.h"

extern char _start[]; 
extern char _end[];   
extern char _stack_top[];

struct list_head free_area[MAX_ORDER + 1];
struct page *mem_map;   
uint64_t phys_mem_start;
uint64_t phys_mem_end;
uint64_t total_pages;
static uint64_t heap_ptr;

int print_log = 0;

#define POOL_COUNT 8 
struct list_head pool_list[POOL_COUNT];

struct page *phys_to_page(uint64_t phys) {
    if (phys < phys_mem_start || phys >= phys_mem_end) return NULL;
    uint64_t pfn = (phys - phys_mem_start) >> PAGE_SHIFT;
    return &mem_map[pfn];
}

uint64_t page_to_phys(struct page *page) {
    return phys_mem_start + (page->pfn * PAGE_SIZE);
}

void *startup_alloc(size_t size) {
    if (heap_ptr % PAGE_SIZE != 0) {
        heap_ptr += PAGE_SIZE - (heap_ptr % PAGE_SIZE);
    }
    uint64_t addr = heap_ptr;
    heap_ptr += size;
    return (void *)addr;
}

struct page *alloc_pages(int order) {
    if (order > MAX_ORDER) return NULL;
    for (int i = order; i <= MAX_ORDER; i++) {
        if (list_empty(&free_area[i])) continue;
        struct page *page = list_entry(free_area[i].next, struct page, list);
        list_del(&page->list);
        page->allocated = 1;
        page->order = order;
        while (i > order) {
            i--;
            uint64_t buddy_pfn = page->pfn + (1 << i);
            struct page *buddy = &mem_map[buddy_pfn];
            buddy->allocated = 0;
            buddy->order = i;
            list_add(&buddy->list, &free_area[i]);

            if(print_log) printf("[Buddy] Split: block added to Order %d\n", i);
        }

        if(print_log) printf("[Page] Allocate Page at %lx, order %d\n", page_to_phys(page), order);
        
        return page;
    }
    return NULL; 
}

void free_pages(struct page *page, int order) {
    if (!page) return;
    if(print_log) printf("[Page] Free Page at %lx, order %d\n", page_to_phys(page), order);
    page->allocated = 0;
    while (order < MAX_ORDER) {
        uint64_t buddy_pfn = page->pfn ^ (1 << order);
        if (buddy_pfn >= total_pages) break;
        struct page *buddy = &mem_map[buddy_pfn];
        if (buddy->allocated != 0 || buddy->order != order) break;
        list_del(&buddy->list);
        if (buddy->pfn < page->pfn) page = buddy;
        if(print_log) printf("[Buddy] Merge: Order %d + Order %d -> Order %d\n", order, order, order + 1);
        order++;
    }
    page->order = order;
    list_add(&page->list, &free_area[order]);
}

void dump_buddy_info() {
    printf("--- Buddy Status ---\n");
    for (int i = 0; i <= MAX_ORDER; i++) {
        int count = 0;
        struct list_head *pos;
        list_for_each(pos, &free_area[i]) count++;
        printf("Order %d: %d\n", i, count);
    }
}

void memory_reserve(uint64_t start, uint64_t end) {
    uint64_t start_pfn = (start - phys_mem_start) >> PAGE_SHIFT;
    uint64_t end_pfn = (end - phys_mem_start + PAGE_SIZE - 1) >> PAGE_SHIFT;
    if (start_pfn >= total_pages) return;
    if (end_pfn > total_pages) end_pfn = total_pages;
    for (uint64_t i = start_pfn; i < end_pfn; i++) {
        mem_map[i].allocated = 2; 
    }
}

void mm_init(void *dtb) {
    uint64_t ram_base = 0, ram_size = 0;
    if (fdt_get_memory_info(dtb, &ram_base, &ram_size) != 0) {
        ram_base = 0x80000000;
        ram_size = 0x10000000; 
    }

    phys_mem_start = ram_base;
    phys_mem_end = ram_base + ram_size;
    total_pages = ram_size / PAGE_SIZE;

    uint64_t kernel_start = (uint64_t)_start;
    uint64_t kernel_end   = (uint64_t)_stack_top;

    uint64_t dtb_start = (uint64_t)dtb;
    uint32_t dtb_totalsize = fdt_total_size(dtb);
    uint64_t dtb_end = dtb_start + dtb_totalsize;

    uint64_t initrd_start = 0, initrd_end = 0;
    int has_initrd = (get_initrd_info(dtb, &initrd_start, &initrd_end) != -1);

    uint64_t mem_map_size = total_pages * sizeof(struct page);

    uint64_t safe_heap = kernel_end;
    safe_heap = (safe_heap + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);

    while (1) {
        int collision = 0;
        uint64_t safe_heap_end = safe_heap + mem_map_size;

        if (has_initrd && safe_heap < initrd_end && safe_heap_end > initrd_start) {
            safe_heap = initrd_end;
            safe_heap = (safe_heap + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            collision = 1;
        }

        if (safe_heap < dtb_end && safe_heap_end > dtb_start) {
            safe_heap = dtb_end;
            safe_heap = (safe_heap + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
            collision = 1;
        }

        if (!collision) {
            break;
        }
    }

    if (safe_heap + mem_map_size > phys_mem_end) {
        printf("[Error] Out of memory when allocating mem_map!\n");
        while(1); 
    }
    heap_ptr = safe_heap;

    mem_map = (struct page *)startup_alloc(mem_map_size);

    for (int i = 0; i <= MAX_ORDER; i++) {
        INIT_LIST_HEAD(&free_area[i]);
    }

    for (uint64_t i = 0; i < total_pages; i++) {
        INIT_LIST_HEAD(&mem_map[i].list);
        mem_map[i].pfn = i;
        mem_map[i].allocated = 1;
        mem_map[i].order = 0;
    }
    
    memory_reserve(phys_mem_start, kernel_end); 
    printf("[Reserve] OpenSBI & Kernel Image: %lx - %lx\n", phys_mem_start, kernel_end);

    memory_reserve(dtb_start, dtb_end); 
    printf("[Reserve] DTB region: %lx - %lx\n", dtb_start, dtb_end);

    if (has_initrd) {
        memory_reserve(initrd_start, initrd_end);
        printf("[Reserve] Initrd region: %lx - %lx\n", initrd_start, initrd_end);
    }

    memory_reserve(safe_heap, heap_ptr); 
    printf("[Reserve] Startup Allocator (mem_map): %lx - %lx\n", safe_heap, heap_ptr);

    if (fdt_reserve_memory(dtb) == -1) {
       printf("[Warning] No /reserved-memory is found in DTB\n");
    }

    for (uint64_t i = 0; i < total_pages; i++) {
        if (mem_map[i].allocated == 1) { 
            free_pages(&mem_map[i], 0);
        }
    }
    
    for (int i = 0; i < POOL_COUNT; i++) {
        INIT_LIST_HEAD(&pool_list[i]);
    }

    print_log = 0;
}

int get_pool_index(size_t size) {
    if (size <= 16) return 0;
    if (size <= 32) return 1;
    if (size <= 64) return 2;
    if (size <= 128) return 3;
    if (size <= 256) return 4;
    if (size <= 512) return 5;
    if (size <= 1024) return 6;
    if (size <= 2048) return 7;
    return -1;
}

int get_pool_size(int index) {
    return 16 << index;
}

void init_pool_page(struct page *p, int chunk_size) {
    p->pool_size = chunk_size;
    p->free_count = PAGE_SIZE / chunk_size;
    p->freelist = (void *)page_to_phys(p);

    uint64_t base = page_to_phys(p);
    for (int i = 0; i < p->free_count - 1; i++) {
        uint64_t curr = base + i * chunk_size;
        uint64_t next = base + (i + 1) * chunk_size;
        *(uint64_t *)curr = next;
    }

    *(uint64_t *)(base + (p->free_count - 1) * chunk_size) = 0;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    
    int idx = get_pool_index(size);
    if (idx < 0) {
        int order = 0;
        while ((PAGE_SIZE << order) < size) order++;
        struct page *p = alloc_pages(order);
        if (!p) return NULL;
        p->pool_size = 0;
        return (void *)page_to_phys(p);
    }

    int chunk_size = get_pool_size(idx);
    
    if (list_empty(&pool_list[idx])) {
        struct page *new_page = alloc_pages(0);
        if (!new_page) return NULL;
        
        init_pool_page(new_page, chunk_size);
        list_add(&new_page->list, &pool_list[idx]);
    }

    struct page *p = list_entry(pool_list[idx].next, struct page, list);
    void *ret = p->freelist;
    
    if (ret) {
        p->freelist = (void *)(*(uint64_t *)ret);
        p->free_count--;
    } else {
        return NULL;
    }

    if (p->free_count == 0) {
        list_del(&p->list); 
    }
    if(print_log) printf("[Chunk] Allocate %d bytes at %lx\n", chunk_size, (uint64_t)ret);

    return ret;
}

void kfree(void *ptr) {
    if (!ptr) return;

    struct page *p = phys_to_page((uint64_t)ptr);
    if (!p) return;

    if (p->pool_size == 0) {
        free_pages(p, p->order);
        return;
    }

    if(print_log) printf("[Chunk] Free chunk at %lx (pool size: %d)\n", (uint64_t)ptr, p->pool_size);

    *(uint64_t *)ptr = (uint64_t)p->freelist;
    p->freelist = ptr;
    p->free_count++;

    int max_count = PAGE_SIZE / p->pool_size;
    int idx = get_pool_index(p->pool_size);

    if (p->free_count == 1) {
        list_add(&p->list, &pool_list[idx]);
    }

    if (p->free_count == max_count) {
        list_del(&p->list);
        
        p->pool_size = 0;
        free_pages(p, 0);
    }
}

void mm_test() {
    printf("\n=== [Test] Memory Allocator Tests ===\n");

    // 1. Buddy System Test
    printf("\n--- Buddy System Test ---\n");
    struct page *p_test = alloc_pages(0);
    printf("Allocated Order 0: 0x%lx\n", page_to_phys(p_test));
    free_pages(p_test, 0);
    printf("Freed Order 0 page.\n");

    // 2. Dynamic Allocator (Pool) Test
    printf("\n--- Dynamic Allocator (Pool) Test ---\n");
    
    // 16-byte Pool
    printf("[Test] Allocating 3x 16 bytes...\n");
    void *ptr1 = kmalloc(16);
    void *ptr2 = kmalloc(16);
    void *ptr3 = kmalloc(16);
    
    printf("kmalloc(16) -> 0x%lx\n", (uint64_t)ptr1);
    printf("kmalloc(16) -> 0x%lx\n", (uint64_t)ptr2);
    printf("kmalloc(16) -> 0x%lx\n", (uint64_t)ptr3);

    // Check whether continuous
    if ((uint64_t)ptr2 - (uint64_t)ptr1 == 16) {
        printf("[Pass] 16-byte chunks are contiguous.\n");
    } else {
        printf("[Info] 16-byte chunks distance: %ld\n", (uint64_t)ptr2 - (uint64_t)ptr1);
    }
    
    // 32-bytes pool test
    printf("\n[Test] Allocating 2x 32 bytes...\n");
    void *ptr4 = kmalloc(32);
    void *ptr5 = kmalloc(32);
    printf("kmalloc(32) -> 0x%lx\n", (uint64_t)ptr4);
    printf("kmalloc(32) -> 0x%lx\n", (uint64_t)ptr5);

    // reuse test
    printf("\n--- Reuse Test ---\n");
    printf("Freeing ptr2 (16-byte) at 0x%lx...\n", (uint64_t)ptr2);
    kfree(ptr2); 
    
    printf("Allocating 16 bytes again...\n");
    void *ptr6 = kmalloc(16);
    printf("kmalloc(16) -> 0x%lx\n", (uint64_t)ptr6);
    
    if (ptr6 == ptr2) {
        printf("[Pass] Memory reused successfully (LIFO).\n");
    } else {
        printf("[Info] Got new chunk (might be FIFO or new page).\n");
    }

    // large allocation test
    printf("\n--- Large Allocation Test ---\n");
    printf("Allocating 4096 bytes (Page Size)...\n");
    void *large_ptr = kmalloc(4096); 
    printf("kmalloc(4096) -> 0x%lx\n", (uint64_t)large_ptr);
    
    if ((uint64_t)large_ptr % 4096 == 0) {
        printf("[Pass] Large allocation is page-aligned (from Buddy).\n");
    } else {
        printf("[Fail] Large allocation NOT page-aligned!\n");
    }
    
    kfree(large_ptr);

    kfree(ptr1);
    kfree(ptr3);
    kfree(ptr4);
    kfree(ptr5);
    kfree(ptr6);
    
    printf("=== Tests Complete ===\n");
}

void test_alloc_1() {
    uart_puts("Testing memory allocation...\n");
    char *ptr1 = (char *)kmalloc(4000);
    char *ptr2 = (char *)kmalloc(8000);
    char *ptr3 = (char *)kmalloc(4000);
    char *ptr4 = (char *)kmalloc(4000);

    kfree(ptr1);
    kfree(ptr2);
    kfree(ptr3);
    kfree(ptr4);

    /* Test kmalloc */
    uart_puts("Testing dynamic allocator...\n");
    char *kmem_ptr1 = (char *)kmalloc(16);
    char *kmem_ptr2 = (char *)kmalloc(32);
    char *kmem_ptr3 = (char *)kmalloc(64);
    char *kmem_ptr4 = (char *)kmalloc(128);

    kfree(kmem_ptr1);
    kfree(kmem_ptr2);
    kfree(kmem_ptr3);
    kfree(kmem_ptr4);

    char *kmem_ptr5 = (char *)kmalloc(16);
    char *kmem_ptr6 = (char *)kmalloc(32);

    kfree(kmem_ptr5);
    kfree(kmem_ptr6);

    // Test allocate new page if the cache is not enough
    void *kmem_ptr[102];
    for (int i=0; i<100; i++) {
        kmem_ptr[i] = (char *)kmalloc(128);
    }
    for (int i=0; i<100; i++) {
        kfree(kmem_ptr[i]);
    }

    #define MAX_ALLOC_SIZE 1ULL << 31
    // Test exceeding the maximum size
    char *kmem_ptr7 = (char *)kmalloc(MAX_ALLOC_SIZE + 1);
    if (kmem_ptr7 == NULL) {
        uart_puts("Allocation failed as expected for size > MAX_ALLOC_SIZE\n");
    }
    else {
        uart_puts("Unexpected allocation success for size > MAX_ALLOC_SIZE\n");
        kfree(kmem_ptr7);
    }
}