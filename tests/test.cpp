#define NDEBUG

#define JMEM_IMPLEMENTATION
#include "../jmem.h"

#define JP_IMPLEMENTATION
#include "../jp.h"

#include "test.h"

size_t total_errors;

void test_errors()
{
    const char *input = "{}";

    JMemory *memory = jmem_init();
    TEST(memory != 0);

    JParser parser = json_init(memory, input);
    JValue json = json_parse(&parser);

    if (TEST(json.type == JSON_OBJECT))
    {
        TEST(json.length == 0);
        JValue nested_error = json["deep"][6]["dark"][9]["error"];
        if (TEST(nested_error.type == JSON_ERROR))
            TEST(nested_error.error == JSON_TYPE_ERROR);
    }

    jmem_free(memory);
}

void test_values()
{
    const char *input = "{\"test\": 69}";

    JMemory *memory = jmem_init();
    TEST(memory != 0);

    JParser parser = json_init(memory, input);
    JValue json = json_parse(&parser);

    if (TEST(json.type == JSON_OBJECT))
    {
        JValue object_number = json_get(&json.object, "test");
        if (TEST(object_number.type == JSON_NUMBER))
            TEST(object_number.number == 69);
    }

    jmem_free(memory);
}

void test_single_values()
{
    const char *input = "1337";

    JMemory *memory = jmem_init();

    JParser parser = json_init(memory, input);
    JValue value = json_parse(&parser);

    if (TEST(value.type == JSON_NUMBER))
        TEST(value.number == 1337);

    jmem_free(memory);

    memory = jmem_init();
    TEST(memory != 0);

    input = "\"string\"";
    parser = json_init(memory, input);

    value = json_parse(&parser);

    if (TEST(value.type == JSON_STRING))
        TEST(strcmp(value.string, "string") == 0);

    jmem_free(memory);

    memory = jmem_init();
    TEST(memory != 0);

    input = "[]";
    parser = json_init(memory, input);

    value = json_parse(&parser);

    if (TEST(value.type == JSON_ARRAY))
        TEST(value.array == 0);

    jmem_free(memory);

    memory = jmem_init();
    TEST(memory != 0);

    input = "{}";
    parser = json_init(memory, input);

    value = json_parse(&parser);

    if (TEST(value.type == JSON_OBJECT))
        TEST(value.length == 0);

    jmem_free(memory);

    memory = jmem_init();
    TEST(memory != 0);

    input = "true";
    parser = json_init(memory, input);

    value = json_parse(&parser);

    if (TEST(value.type == JSON_BOOL))
        TEST(value.boolean == 1);

    jmem_free(memory);

    memory = jmem_init();
    TEST(memory != 0);

    input = "null";
    parser = json_init(memory, input);

    value = json_parse(&parser);

    if (TEST(value.type == JSON_NULL))
        TEST(value.null == 0);

    jmem_free(memory);
}

Test tests[] = {
    {"errors", test_errors},
    {"values", test_values},
    {"single values", test_single_values},
};

int main()
{
    for (size_t i = 0; i < COUNT(tests); ++i)
        tests[i].f();

    TOTAL_ERRORS
}
