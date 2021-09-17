#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COUNT(a) (sizeof(a) / sizeof(*a))
#define STRINGIFY(x) #x
#define TEST(cond) test(cond, #cond, __LINE__)
#define TOTAL_ERRORS                                                           \
    if (total_errors == 0)                                                     \
    {                                                                          \
        fprintf(stdout, "all tests passed\n");                                 \
        return 0;                                                              \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        fprintf(stdout, "total errors: %zu\n", total_errors);                  \
        return 1;                                                              \
    }

extern size_t total_errors;

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
