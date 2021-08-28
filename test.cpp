#define JMEM_IMPLEMENTATION
#include "jmem.h"

#define JP_IMPLEMENTATION
#include "jp.h"

#include "test.h"

int main()
{
    char *input = read_file_as_str("input.json");

    JMemory *memory = jmem_init();
    if (memory == 0)
        exit(1);

    // void *custom_malloc(void *struct_ptr, unsigned long long int size)
    // {
    //     return malloc(size);
    // }
    //
    // JMemory memory = {.base = (char *)malloc(1024), .alloc = custom_malloc, .struct_ptr = 0};
    //
    // or
    //
    // typedef struct
    // {
    //     char *base;
    //     char *start;
    // } Memory;
    //
    // void *custom_alloc(void *struct_ptr, unsigned long long int size)
    // {
    //     Memory *memory = (Memory *)struct_ptr;
    //     memory->start += size;
    //     return memory->start - size;
    // }
    //
    // char mem[4096];
    // Memory custom = {.base = mem, .start = custom.base};
    // JMemory memory = {.base = custom.base, .alloc = custom_alloc, .struct_ptr = &custom};

    JParser parser = json_init(memory, input);

    JValue json = json_parse(&parser);
    if (json.type == JSON_ERROR)
        exit(1);

    printf("empty string: %s\n", json["empty"].string);
    JValue aya = json["aya"];
    if (aya.type == JSON_ERROR)
        exit(1);
    else
        printf("positive number: %lld\n", aya.number);
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

    jmem_free(memory);
    free(input);

    return 0;
}
