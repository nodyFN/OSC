#include "string.h"

int strcmp(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return *str1 - *str2;
        }
        str1++;
        str2++;
    }

    return *str1 - *str2;
}

unsigned long strlen(const char *str) {
    const char *s;
    for (s = str; *s; ++s)
        ;
    return (s - str);
}

int strncmp(const char* str1, const char* str2, const int n){
    for(int i = 0; i < n; i++){
        if(str1[i] != str2[i]){
            return str1[i] - str2[i];
        }
        if(str1[i] == '\0'){
            return 0;
        }
    }
    return 0;
}

void* memcpy(void* dest, const void* src, unsigned long n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dest;
}

void memset(void* dest, int val, size_t len){
    unsigned char *ptr = (unsigned char *)dest;

    for (size_t i = 0; i < len; i++) {
        ptr[i] = (unsigned char)val;
    }
}