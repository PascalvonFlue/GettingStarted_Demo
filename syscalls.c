#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

static inline int semihosting_call(int reason, void *arg)
{
    int value;
    __asm__ volatile (
        "mov r0, %1      \n"
        "mov r1, %2      \n"
        "bkpt 0xAB       \n"
        "mov %0, r0      \n"
        : "=r" (value)
        : "r" (reason), "r" (arg)
        : "r0", "r1", "memory"
    );
    return value;
}

int _write(int file, const char *ptr, int len)
{
    (void)file;

    struct {
        int handle;
        const char *buf;
        int len;
    } args;

    args.handle = 1;
    args.buf    = ptr;
    args.len    = len;


    int not_written = semihosting_call(0x05, &args);

    if (not_written < 0)
        return -1;

    return len - not_written;
}


void *_sbrk(ptrdiff_t incr)
{
    (void)incr;
    errno = ENOMEM;
    return (void *)-1;
}


int _close(int file)
{
    (void)file;
    return -1;
}

int _fstat(int file, struct stat *st)
{
    (void)file;
    if (st)
        st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file)
{
    (void)file;
    return 1;
}

int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

int _read(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return 0; // no stdin
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}