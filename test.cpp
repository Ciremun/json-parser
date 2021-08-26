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
    if (parser.error.code != JSON_OK)
    {
        printf("error: %s\n", parser.error.message);
        exit(1);
    }

    JValue json = json_parse(&parser);
    if (json.type == JSON_ERROR)
    {
        printf("error: %s\n", json.error.message);
        exit(1);
    }

    // C++
    printf("empty string: %s\n", json["empty"].string);
    printf("positive number: %lld\n", json["aya"].number);
    printf("array:\n");
    printf("  negative number: %lld\n", json["test"][0].number);
    printf("  true: %d\n", json["test"][1].boolean);
    printf("  false: %d\n", json["test"][2].boolean);
    printf("  string: %s\n", json["test"][3].string);
    printf("  null: %d\n", json["test"][4].null);
    printf("  array number: %lld\n", json["test"][5][0].number);

    printf("nested:\n  C++:\n");
    printf("    string: %s\n", json["owo"]["uwo"]["str"].string);
    printf("    array string: %s\n", json["owo"]["uwo"]["arr"][0].string);

    // C
    JValue value = json_get(&json.object, "error");
    JObject owo = json_get(&json.object, "owo").object;
    JObject uwo = json_get(&owo, "uwo").object;
    char *str = json_get(&uwo, "str").string;
    JValue *arr = json_get(&uwo, "arr").array;
    JValue *test = json_get(&json.object, "test").array;
    printf("  C:\n");
    if (value.type == JSON_ERROR)
        printf("    error: %s\n", value.error.message);
    printf("    string: %s\n", str);
    printf("    array string: %s\n", arr[0].string);
    printf("    negative number: %lld\n", test[0].number);

    json_memory_free(&parser.memory);
    free(input);

    return 0;
}
