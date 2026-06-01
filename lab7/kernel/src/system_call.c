#include "system_call.h"

long getpid(){
    register long a0 asm("a0");
    register long a7 asm("a7") = 0;

    asm volatile(
        "ecall\n"
        : "=r"(a0)
        : "r"(a7)
        : "memory"
    );

    return a0;
}

long uart_read(char *buf, long count){
    register long a0 asm("a0") = (long)buf;
    register long a1 asm("a1") = count;
    register long a7 asm("a7") = 1;

    asm volatile(
        "ecall\n"
        : "+r" (a0)
        : "r" (a1), "r" (a7)
        : "memory"
    );

    return a0;
}

long uart_write(const char *buf, long count) {
    register long a0 asm("a0") = (long)buf;
    register long a1 asm("a1") = count;
    register long a7 asm("a7") = 2;

    asm volatile(
        "ecall\n"
        : "+r" (a0)
        : "r" (a1), "r" (a7)
        : "memory"
    );

    return a0;
}

int exec(const char *path){
    register long a0 asm("a0") = (long)path;
    register long a7 asm("a7") = 3;

    asm volatile(
        "ecall\n"
        : "+r" (a0)
        : "r" (a7)
        : "memory"
    );

    return (int)a0;
}

long fork() {
    register long a0 asm("a0");
    register long a7 asm("a7") = 4;
    asm volatile(
        "ecall\n"
        : "=r" (a0)
        : "r" (a7)
        : "memory"
    );
    return (int)a0;
}

void exit(int status){
    register long a0 asm("a0") = status;
    register long a7 asm("a7") = 5;

    asm volatile(
        "ecall\n"
        : 
        : "r"(a0), "r"(a7)
        : "memory"
    );
}

int stop(long pid) {
    long ret;
    register long a0 asm("a0") = pid;
    register long a7 asm("a7") = 6;

    asm volatile(
        "ecall\n"
        "mv %0, a0\n"
        : "=r" (ret)
        : "r" (a0), "r" (a7)
        : "memory"
    );

    return (int)ret;
}