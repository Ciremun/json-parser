#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define COUNT(a) (sizeof(a) / sizeof(*a))
#define STRINGIFY(x) #x
#define TEST(cond) test(cond, #cond, __LINE__)
#define TOTAL_ERRORS()                                                         \
    do                                                                         \
    {                                                                          \
        if (total_errors == 0)                                                 \
        {                                                                      \
            fprintf(stdout, "all tests passed\n");                             \
            return 0;                                                          \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            fprintf(stdout, "total errors: %zu\n", total_errors);              \
            return 1;                                                          \
        }                                                                      \
    } while (0)

size_t total_errors;

typedef struct
{
    char *start;
    size_t length;
} String;

typedef struct
{
    const char *name;
    void (*f)(void);
} Test;


char *test_memory_buffer_base = 0;

void *custom_alloc(unsigned long long int size)
{
    test_memory_buffer_base += size;
    return test_memory_buffer_base - size;
}

void *custom_malloc(unsigned long long int size)
{
    return malloc(size);
}

void *returns_null(unsigned long long int size)
{
    (void)size;
    return 0;
}

size_t write_to_string(const void *buffer, size_t size, size_t count, void *stream)
{
    String* str = (String *)stream;
    memcpy(str->start + str->length, buffer, count * size);
    str->length += count;
    return count;
}

int test(int cond, const char *test, jsize_t line_number)
{
    if (!cond)
    {
        total_errors++;
        fprintf(stdout, "%llu: %s FAILED\n", line_number, test);
    }
    return cond;
}

char *read_file_as_str(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!TEST(f != 0))
        exit(1);
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    char *str = (char *)malloc(size + 1);
    fseek(f, 0, SEEK_SET);
    fread(str, 1, size, f);
    str[size] = '\0';
    fclose(f);
    return str;
}
