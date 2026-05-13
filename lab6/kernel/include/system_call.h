#ifndef __SYSTEM_CALL_H__
#define __SYSTEM_CALL_H__

long getpid();
long uart_read(char *buf, long count);
long uart_write(const char *buf, long count);
int exec(const char *path);
long fork();
void exit(int status);

#endif