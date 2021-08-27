#define JP_IMPLEMENTATION
#include "jp.h"

#include "test.h"

int main()
{
    char *input = read_file_as_str("input.json");
    printf("input: %s\n", input);

    JParser parser = json_init(input);
    if (parser.error.code != JSON_OK)
        VALUE_ERROR(parser);

    JValue json = json_parse(&parser);
    if (json.type == JSON_ERROR)
        VALUE_ERROR(json);

    printf("empty string: %s\n", json["empty"].string);
    JValue aya = json["aya"];
    if (aya.type == JSON_ERROR)
        VALUE_ERROR(aya);
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

    json_memory_free(&parser.memory);
    free(input);

    return 0;
}
