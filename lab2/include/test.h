#ifndef __TEST_H__
#define __TEST_H__

void test_lab1();

struct fdt_test_info{
    const void* dtb_addr;
    const char* path;
    const char* prop_name;
    const int list_node;
};
void test_lab2(const struct fdt_test_info* info);

#endif