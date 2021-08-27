#ifndef JMEM_H_
#define JMEM_H_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif // _WIN32

#include "jp.h"

#ifndef _WIN32
int GetPhysicallyInstalledSystemMemory(jsize_t *output);
#endif // _WIN32

typedef struct
{
    char *start;
    char *base;
    jsize_t capacity;
#ifdef _WIN32
    jsize_t commited;
    jsize_t allocated;
    jsize_t commit_size;
#endif // _WIN32
} JMem;

JMemory *jmem_init();
int jmem_free(JMemory *memory);
void *jmem_alloc(void *struct_ptr, unsigned long long int size);

#endif // JMEM_H_

#ifdef JMEM_IMPLEMENTATION

#ifndef _WIN32
int GetPhysicallyInstalledSystemMemory(jsize_t *output)
{
    int fd = open("/proc/meminfo", O_RDONLY);
    if (fd == -1)
        return 0;

    char buff[64];
    int count = 0;
    if ((count = read(fd, buff, 64)) <= 0)
        goto error;

    if (json_memcmp(buff, "MemTotal:", 9) != 0)
        goto error;

    jsize_t pos;
    pos = 9;
    if (!json_skip_whitespaces(buff, &pos))
        goto error;

    jsize_t i;
    i = 0;

    jsize_t digit;

    while ((buff + pos)[i] != ' ')
    {
        digit = (buff + pos)[i] - 48;
        if (digit > 9)
            goto error;
        *output = *output * 10 + digit;
        i++;
    }

    close(fd);
    return 1;
error:
    close(fd);
    return 0;
}
#endif // _WIN32

JMemory *jmem_init()
{
    JMem memory;
    memory.capacity = 0;
    if (!GetPhysicallyInstalledSystemMemory(&memory.capacity))
        memory.capacity = (jsize_t)4294967295;
    else
        memory.capacity *= 1024;
#ifdef _WIN32
    memory.commited = 0;
    memory.allocated = 0;
    memory.commit_size = 1024;
    memory.base =
        (char *)(VirtualAlloc(0, memory.capacity, MEM_RESERVE, PAGE_READWRITE));
    if (memory.base == 0)
        return 0;
#else
    memory.base = (char *)(mmap(0, memory.capacity, PROT_READ | PROT_WRITE,
                                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (memory.base == MAP_FAILED)
        return 0;
#endif // _WIN32
    memory.start = memory.base;

    JMem *memory_ptr = (JMem *)jmem_alloc(&memory, sizeof(JMem));
#ifdef _WIN32
    memory_ptr->commited = memory.commited;
    memory_ptr->allocated = memory.allocated;
    memory_ptr->commit_size = memory.commit_size;
#endif // _WIN32
    memory_ptr->capacity = memory.capacity;
    memory_ptr->base = memory.base;
    memory_ptr->start = memory.start;

    JMemory *mem = (JMemory *)jmem_alloc(memory_ptr, sizeof(JMemory));
    mem->alloc = jmem_alloc;
    mem->base = memory_ptr->start;
    mem->struct_ptr = (void *)memory_ptr;
    return mem;
}

int jmem_free(JMemory *memory)
{
    JMem *mem = (JMem *)memory->struct_ptr;
#ifdef _WIN32
    if (VirtualFree(mem->base, 0, MEM_RELEASE) == 0)
        return 0;
#else
    if (munmap(mem->base, mem->capacity) == -1)
        return 0;
#endif // _WIN32
    return 1;
}

void *jmem_alloc(void *struct_ptr, unsigned long long int size)
{
    JMem *memory = (JMem *)struct_ptr;
#ifdef _WIN32
    memory->allocated += size;
    if (memory->allocated * 2 > memory->commited)
    {
        memory->commit_size += size * 2;
        if (VirtualAlloc(memory->start, memory->commit_size, MEM_COMMIT,
                         PAGE_READWRITE) == 0)
            return 0;
        memory->commited += memory->commit_size;
    }
#endif // _WIN32
    memory->start += size;
    return memory->start - size;
}

#endif // JMEM_IMPLEMENTATION