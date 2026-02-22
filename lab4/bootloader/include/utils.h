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

#endif