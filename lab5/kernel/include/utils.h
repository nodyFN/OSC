#ifndef __UTILS_H__
#define __UTILS_H__

static inline uint32_t toLittleEndian(uint32_t val) {
    return ((val & 0x000000FFU) << 24) |
           ((val & 0x0000FF00U) << 8)  |
           ((val & 0x00FF0000U) >> 8)  |
           ((val & 0xFF000000U) >> 24);
}

static inline uint32_t toBigEndian(uint32_t val) {
    return toLittleEndian(val);
}

struct KernelInfo{
    uint64_t hartid;
    void* dtb_addr;
    uint64_t initrd_start_addr;
    uint64_t initrd_end_addr;
};

#define local_irq_save(flags) \
    do { \
        asm volatile("csrr %0, sstatus" : "=r"(flags)); \
        asm volatile("csrc sstatus, %0" : : "r"(1 << 1)); \
    } while(0)

#define local_irq_restore(flags) \
    do { \
        asm volatile("csrw sstatus, %0" : : "r"(flags)); \
    } while(0)

#endif