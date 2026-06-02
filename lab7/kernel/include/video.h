#ifndef __VIDEO_H__
#define __VIDEO_H__

#ifdef QEMU
    #define FB_BASE_PA 0xfe000000UL
#else
    #define FB_BASE_PA 0x7f700000UL
#endif

// #define FB_BASE   0xfe000000
#ifdef QEMU
    #define FB_BASE 0xfe000000
#else
    #define FB_BASE 0x7f700000
#endif

#define FB_WIDTH  1920
#define FB_HEIGHT 1080
#define FB_BPP    4
#define XRGB8888  875713112

#define QEMU_PACKED __attribute__((packed))
// #define bswap64(x)  __builtin_bswap64(x)
// #define bswap32(x)  __builtin_bswap32(x)
// #define bswap16(x)  __builtin_bswap16(x)
#define bswap16(x) \
    ((uint16_t)((((uint16_t)(x) & 0x00ff) << 8) | \
                (((uint16_t)(x) & 0xff00) >> 8)))

#define bswap32(x) \
    ((uint32_t)((((uint32_t)(x) & 0x000000ff) << 24) | \
                (((uint32_t)(x) & 0x0000ff00) <<  8) | \
                (((uint32_t)(x) & 0x00ff0000) >>  8) | \
                (((uint32_t)(x) & 0xff000000) >> 24)))

#define bswap64(x) \
    ((uint64_t)((((uint64_t)(x) & 0x00000000000000ffULL) << 56) | \
                (((uint64_t)(x) & 0x000000000000ff00ULL) << 40) | \
                (((uint64_t)(x) & 0x0000000000ff0000ULL) << 24) | \
                (((uint64_t)(x) & 0x00000000ff000000ULL) <<  8) | \
                (((uint64_t)(x) & 0x000000ff00000000ULL) >>  8) | \
                (((uint64_t)(x) & 0x0000ff0000000000ULL) >> 24) | \
                (((uint64_t)(x) & 0x00ff000000000000ULL) >> 40) | \
                (((uint64_t)(x) & 0xff00000000000000ULL) >> 56)))

// #define FW_CFG_BASE   0x10100000UL
// #define FW_CFG_SELECT (uint16_t*)(FW_CFG_BASE + 0x08)
// #define FW_CFG_DATA   (uint64_t*)(FW_CFG_BASE + 0x00)
// #define FW_CFG_DMA    (uint64_t*)(FW_CFG_BASE + 0x10)
#define FW_CFG_BASE   PA_TO_VA(0x10100000UL)
#define FW_CFG_SELECT (uint16_t*)(FW_CFG_BASE + 0x08)
#define FW_CFG_DATA   (uint64_t*)(FW_CFG_BASE + 0x00)
#define FW_CFG_DMA    (uint64_t*)(FW_CFG_BASE + 0x10)

#define FW_CFG_DMA_CTL_ERROR  0x01
#define FW_CFG_DMA_CTL_READ   0x02
#define FW_CFG_DMA_CTL_SKIP   0x04
#define FW_CFG_DMA_CTL_SELECT 0x08
#define FW_CFG_DMA_CTL_WRITE  0x10

#define FW_CFG_FILE_DIR 0x19

#define CACHE_BLOCK_SIZE 64
#define cbo_flush(start)                            \
    ({                                              \
        unsigned long __v = (unsigned long)(start); \
        __asm__ __volatile__(                       \
            "cbo.flush"                             \
            " 0(%0)"                                \
            :                                       \
            : "rK"(__v)                             \
            : "memory");                            \
    })

void video_init();
void video_bmp_display(unsigned int* bmp_image, int width, int height);

#endif