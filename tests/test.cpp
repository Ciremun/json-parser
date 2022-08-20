#define JP_IMPLEMENTATION
#include "../jp.h"

#include "test.h"

void test_errors()
{
    const char *input = "{}";

    JValue json = json_parse(input);

    if (TEST(json.type == JSON_OBJECT))
    {
        TEST(json.object.length == 0);
        JValue nested_error = json["deep"][6]["dark"][9]["error"];
        if (TEST(nested_error.type == JSON_ERROR))
            TEST(nested_error.error == JSON_TYPE_ERROR);
    }
}

void test_values()
{
    const char *input = "{\"test\": 69}";

    JValue json = json_parse(input);

    if (TEST(json.type == JSON_OBJECT))
    {
        JValue object_number = json_get(&json.object, "test");
        if (TEST(object_number.type == JSON_NUMBER))
            TEST(object_number.number == 69);
    }
}

void test_single_values()
{
    const char *input = "1337";

    JValue value = json_parse(input);

    if (TEST(value.type == JSON_NUMBER))
        TEST(value.number == 1337);

    input = "\"string\"";
    value = json_parse(input);

    if (TEST(value.type == JSON_STRING))
        TEST(strcmp(value.string.data, "string") == 0);

    input = "[]";
    value = json_parse(input);

    if (TEST(value.type == JSON_ARRAY))
        TEST(value.array.data == 0);

    input = "{}";
    value = json_parse(input);

    if (TEST(value.type == JSON_OBJECT))
        TEST(value.object.length == 0);

    input = "true";
    value = json_parse(input);

    if (TEST(value.type == JSON_BOOL))
        TEST(value.boolean == 1);

    input = "null";
    value = json_parse(input);

    if (TEST(value.type == JSON_NULL))
        TEST(value.null == 0);
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

    TOTAL_ERRORS();
}
