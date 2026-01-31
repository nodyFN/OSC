#include "string.h"

int strcmp(const char* str1, const char* str2) {
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            // return 0; // 不相等
            return *str1 - *str2; // 返回差值
        }
        str1++;
        str2++;
    }
    // return (*str1 == '\0' && *str2 == '\0'); // 都到結尾才算相等
    return *str1 - *str2; // 返回差值
}

// 注意：回傳型別通常用 size_t (unsigned long)，這取決於你的定義
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
        if(str1[i] == '\0'){ // 提前結束
            return 0;
        }
    }
    return 0; // 前 n 個字元相等
}

void* memcpy(void* dest, const void* src, unsigned long n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;
    while (n--) *d++ = *s++;
    return dest;
}