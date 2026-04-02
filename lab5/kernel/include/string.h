#ifndef __STRING_H__
#define __STRING_H__


int strcmp(const char* str1, const char* str2);
unsigned long strlen(const char *str);
int strncmp(const char* str1, const char* str2, const int n);
void* memcpy(void* dest, const void* src, unsigned long n);


#endif