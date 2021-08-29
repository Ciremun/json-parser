#include <stdio.h>
#include <stdlib.h>

#define COUNT(a) (sizeof(a) / sizeof(*a))

extern size_t total_errors;

typedef struct
{
    char *start;
    size_t length;
} String;

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

void test(int cond, const char *test)
{
    if (!cond)
    {
        total_errors++;
        fprintf(stdout, "  %s - fail\n", test);
    }
}
