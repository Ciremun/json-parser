#define JMEM_IMPLEMENTATION
#include "../jmem.h"

#define JP_IMPLEMENTATION
#include "../jp.h"

#define JIM_IMPLEMENTATION
#include "jim.h"

#include "test.h"

#include <assert.h>
#include <string.h>

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
    TEST(memory != 0);

    JParser parser = json_init(memory, buffer);

    JValue json = json_parse(&parser);
    TEST(json.type == JSON_OBJECT);

    JValue number = json_get(&json.object, "number");
    TEST(number.type == JSON_NUMBER);
    TEST(number.number == 12345);

    JValue empty_string = json_get(&json.object, "empty_string");
    TEST(empty_string.type == JSON_STRING);
    TEST(empty_string.string == 0);

    JValue string = json_get(&json.object, "string");
    TEST(string.type == JSON_STRING);
    TEST(strcmp(string.string, "test string") == 0);

    JValue boolean = json_get(&json.object, "bool");
    TEST(boolean.type == JSON_BOOL);
    TEST(boolean.boolean == 1);

    JValue null = json_get(&json.object, "null");
    TEST(null.type == JSON_NULL);
    TEST(null.null == 0);

    JValue array = json_get(&json.object, "array");
    TEST(array.type == JSON_ARRAY);

    JValue array_number = array.array[0];
    TEST(array_number.type == JSON_NUMBER);
    TEST(array_number.number == 69);

    JValue array_string = array.array[1];
    TEST(array_string.type == JSON_STRING);
    TEST(strcmp(array_string.string, "another test string") == 0);

    JValue array_boolean = array.array[2];
    TEST(array_boolean.type == JSON_BOOL);
    TEST(!array_boolean.boolean);

    JValue array_null= array.array[3];
    TEST(array_null.type == JSON_NULL);
    TEST(array_null.null == 0);

    JValue array_array = array.array[4];
    TEST(array_array.type == JSON_ARRAY);
    JValue array_array_string = array_array.array[0];
    TEST(array_array_string.type == JSON_STRING);
    TEST(strcmp(array_array_string.string, "yet another test string") == 0);

    JValue array_object = array.array[5];
    TEST(array_object.type == JSON_OBJECT);

    JValue array_object_array = json_get(&array_object.object, "test");
    TEST(array_object_array.type == JSON_ARRAY);

    JValue array_object_array_object = array_object_array.array[0];
    TEST(array_object_array_object.type == JSON_OBJECT);
    
    JValue array_object_array_object_string = json_get(&array_object_array_object.object, "nested");
    TEST(array_object_array_object_string.type == JSON_STRING);
    TEST(strcmp(array_object_array_object_string.string, "nested string") == 0);

    JValue nested_array_object = array.array[6];
    TEST(nested_array_object.type == JSON_OBJECT);

    JValue nested_array_object_number = json_get(&nested_array_object.object, "nested_array_object");
    TEST(nested_array_object_number.type == JSON_NUMBER);
    TEST(nested_array_object_number.number == 69);

    JValue empty_array = json_get(&json.object, "empty_array");
    TEST(empty_array.type == JSON_ARRAY);
    TEST(empty_array.array == 0);

    JValue empty_object = json_get(&json.object, "empty_object");
    TEST(empty_object.type == JSON_OBJECT);
    TEST(empty_object.object.pairs == 0);

    JValue object = json_get(&json.object, "object");
    TEST(object.type == JSON_OBJECT);
    
    JValue array_object_string = json_get(&object.object, "array_object");
    TEST(array_object_string.type == JSON_STRING);
    TEST(strcmp(array_object_string.string, "array object string") == 0);

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
