#define NDEBUG

#include <stdio.h>
#include <stdlib.h>

#define COUNT(a) (sizeof(a) / sizeof(*a))
#define TEST(cond) test(cond, #cond)

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

char *read_file_as_str(const char *path)
{
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    char *str = (char *)malloc(size + 1);
    fseek(f, 0, SEEK_SET);
    fread(str, 1, size, f);
    str[size] = '\0';
    fclose(f);
    return str;
}

int test(int cond, const char *test)
{
    if (!cond)
    {
        total_errors++;
        fprintf(stdout, "%s FAILED\n", test);
    }
    return cond;
}
