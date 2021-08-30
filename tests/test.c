#define JMEM_IMPLEMENTATION
#include "../jmem.h"

#define JP_IMPLEMENTATION
#include "../jp.h"

#define JIM_IMPLEMENTATION
#include "jim.h"

#include "test.h"

#include <assert.h>
#include <string.h>

// TODO(#25): TEST macro

size_t total_errors;

size_t write_to_string(const void *buffer, size_t size, size_t count, void *stream)
{
    String* str = (String *)stream;
    memcpy(str->start + str->length, buffer, count * size);
    str->length += count;
    return count;
}

void test_values(void)
{
    char buffer[512] = {0};
    String input = { .start = buffer, .length = 0, };
    Jim jim = { .sink = &input, .write = (Jim_Write)write_to_string, };
    jim_object_begin(&jim);
        jim_member_key(&jim, "number");
        jim_integer(&jim, 12345);
        jim_member_key(&jim, "empty_string");
        jim_string(&jim, "");
        jim_member_key(&jim, "string");
        jim_string(&jim, "test string");
        jim_member_key(&jim, "bool");
        jim_bool(&jim, 1);
        jim_member_key(&jim, "null");
        jim_null(&jim);
        jim_member_key(&jim, "array");
        jim_array_begin(&jim);
            jim_integer(&jim, 69);
            jim_string(&jim, "another test string");
            jim_bool(&jim, 0);
            jim_null(&jim);
            jim_array_begin(&jim);
                jim_string(&jim, "yet another test string");
            jim_array_end(&jim);
            jim_object_begin(&jim);
                jim_member_key(&jim, "test");
                jim_array_begin(&jim);
                    jim_object_begin(&jim);
                        jim_member_key(&jim, "nested");
                        jim_string(&jim, "nested string");
                    jim_object_end(&jim);
                jim_array_end(&jim);
            jim_object_end(&jim);
            jim_object_begin(&jim);
                jim_member_key(&jim, "nested_array_object");
                jim_integer(&jim, 69);
            jim_object_end(&jim);
            jim_array_end(&jim);
        jim_member_key(&jim, "empty_array");
        jim_array_begin(&jim);
        jim_array_end(&jim);
        jim_member_key(&jim, "empty_object");
        jim_object_begin(&jim);
        jim_object_end(&jim);
        jim_member_key(&jim, "object");
        jim_object_begin(&jim);
            jim_member_key(&jim, "array_object");
            jim_string(&jim, "array object string");
        jim_object_end(&jim);
    jim_object_end(&jim);

    JMemory *memory = jmem_init();
    test(memory != 0, "memory != 0");

    JParser parser = json_init(memory, buffer);

    JValue json = json_parse(&parser);
    test(json.type != JSON_ERROR, "json.type != JSON_ERROR");

    JValue number = json_get(&json.object, "number");
    test(number.type == JSON_NUMBER, "number.type == JSON_NUMBER");
    test(number.number == 12345, "number.number == 12345");

    JValue empty_string = json_get(&json.object, "empty_string");
    test(empty_string.type == JSON_STRING, "empty_string.type == JSON_STRING");
    test(empty_string.string == 0, "empty_string.string == 0");

    JValue string = json_get(&json.object, "string");
    test(string.type == JSON_STRING, "string.type == JSON_STRING");
    test(strcmp(string.string, "test string") == 0, "strcmp(string.string, \"test string\") == 0");

    JValue boolean = json_get(&json.object, "bool");
    test(boolean.type == JSON_BOOL, "boolean.type == JSON_BOOL");
    test(boolean.boolean == 1, "boolean.boolean == 1");

    JValue null = json_get(&json.object, "null");
    test(null.type == JSON_NULL, "null.type == JSON_NULL");
    test(null.null == 0, "null.null == 0");

    JValue array = json_get(&json.object, "array");
    test(array.type == JSON_ARRAY, "array.type == JSON_ARRAY");
    JValue array_number = array.array[0];
    test(array_number.type == JSON_NUMBER, "array_number.type == JSON_NUMBER");
    test(array_number.number == 69, "array_number == 69");

    jmem_free(memory);
}

void (*tests[])(void) = {
    test_values,
};

int main(void)
{

    for (size_t i = 0; i < COUNT(tests); ++i)
        tests[i]();

    if (total_errors == 0)
    {
        fprintf(stdout, "all tests passed\n");
        return 0;
    }
    else
    {
        fprintf(stdout, "total errors: %zu\n", total_errors);
        return 1;
    }
}
