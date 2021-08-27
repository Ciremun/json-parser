#include <stdio.h>
#include <stdlib.h>

#define VALUE_ERROR(value)                                                     \
    do                                                                         \
    {                                                                          \
        printf("error: %s\n", value.error.message);                            \
        exit(1);                                                               \
    } while (0)

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
