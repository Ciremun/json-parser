#define JP_IMPLEMENTATION
#include "jp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *read_file_as_str(const char *path)
{
    FILE *f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    char *str = (char *)malloc(size + 1);
    memset(str, 0, size);
    fseek(f, 0, SEEK_SET);
    fread(str, 1, size, f);
    str[size] = '\0';
    fclose(f);
    return str;
}

int main()
{
    char *input = read_file_as_str("input.json");
    printf("%s\n", input);

    JParser parser = json_init(input);
    JValue json = json_parse(&parser, input);

    // C++
    printf("value: %s\n", json["owo"]["uwo"]["str"].string);

    // C
    JObject owo = json_get(&json.object, "owo").object;
    JObject uwo = json_get(&owo, "uwo").object;
    char *str = json_get(&uwo, "str").string;
    printf("value: %s\n", str);

    parser.free();
    free(input);

    return 0;
}
