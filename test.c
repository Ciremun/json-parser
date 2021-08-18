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
    JObject json = json_parse(&parser, input);
    JValue value = json_get(&json, "test");

    printf("value: %d\n", value.boolean);

    // JsonParser parser(input);
    // JObject json = parser.parse();

    // printf("%d\n", (bool &&)json["test"]);
    // printf("%d\n", json.boolean("test"));

    // char *string = (char *&&)json["owo"]["deep"]["dark"]["dungeon"];
    // printf("%s\n", string);

    // printf("%s\n", json.obj("owo").str("uwu"));

    // parser.free();
    // free(input);

    return 0;
}
