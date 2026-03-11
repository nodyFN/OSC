#include "timer.h"
#include "sbi.h"
#include "fdt.h"
#include "utils.h"
#include "stdio.h"

uint64_t TIMERBASE_FREQ = 0x989680;

extern struct KernelInfo kernel_info;

void set_next_timer(int second) {
    uint64_t current_time = get_time();
    sbi_set_timer(current_time + TIMERBASE_FREQ * second);
}

void timer_init(){
    int offset = fdt_path_offset(kernel_info.dtb_addr, "/cpus");
    if(offset == -1){
        printf("[FAILED] Can not find cpu frequency [1].\n");
    }else{
        int len;
        uint32_t* prop = (uint32_t*)fdt_getprop(kernel_info.dtb_addr, offset, "timebase-frequency", &len);
        if (!prop){
            printf("[FAILED] Can not find cpu frequency [2].\n");
        }else{
            TIMERBASE_FREQ = toLittleEndian(*prop);
            // printf("TIMERBASE_FREQ: %d\n", TIMERBASE_FREQ);
        }
    }

    set_next_timer(0);
    __asm__ volatile("csrs sie, %0" : : "r"(1 << 5));
    __asm__ volatile("csrs sstatus, %0" : : "r"(1 << 1));
}