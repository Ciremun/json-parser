#define JMEM_IMPLEMENTATION
#include "jmem.h"

#define JP_IMPLEMENTATION
#include "jp.h"

#include "test.h"

int main(void)
{
    char *input = read_file_as_str("input.json");

    JMemory *memory = jmem_init();
    if (memory == 0)
        exit(1);

    JParser parser = json_init(memory, input);

    JValue json = json_parse(&parser);
    if (json.type == JSON_ERROR)
        exit(1);

    JValue value = json_get(&json.object, "error");
    JObject owo = json_get(&json.object, "owo").object;
    JObject uwo = json_get(&owo, "uwo").object;
    char *str = json_get(&uwo, "str").string;
    JValue *arr = json_get(&uwo, "arr").array;
    JValue *test = json_get(&json.object, "test").array;
    printf("  C:\n");
    if (value.type == JSON_ERROR)
        printf("key \"error\" was not found\n");
    printf("    string: %s\n", str);
    printf("    array string: %s\n", arr[0].string);
    printf("    negative number: %lld\n", test[0].number);

    jmem_free(memory);
    free(input);

    return 0;
}
