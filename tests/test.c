#define NDEBUG

#define JMEM_IMPLEMENTATION
#include "../jmem.h"

#define JP_IMPLEMENTATION
#include "../jp.h"

#define JIM_IMPLEMENTATION
#include "jim.h"

#include <assert.h>
#include <string.h>

#include "test.h"

size_t total_errors;

void *returns_null(void *struct_ptr, unsigned long long int size)
{
    (void)struct_ptr;
    (void)size;
    return 0;
}

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
    if (!TEST(json.type == JSON_OBJECT))
        return;

    JValue number = json_get(&json.object, "number");
    if (TEST(number.type == JSON_NUMBER))
        TEST(number.number == 12345);

    JValue empty_string = json_get(&json.object, "empty_string");
    if (TEST(empty_string.type == JSON_STRING))
        TEST(empty_string.string == 0);

    JValue string = json_get(&json.object, "string");
    if (TEST(string.type == JSON_STRING))
        TEST(strcmp(string.string, "test string") == 0);

    JValue boolean = json_get(&json.object, "bool");
    if (TEST(boolean.type == JSON_BOOL))
        TEST(boolean.boolean == 1);

    JValue null = json_get(&json.object, "null");
    if (TEST(null.type == JSON_NULL))
        TEST(null.null == 0);

    JValue array = json_get(&json.object, "array");
    if (TEST(array.type == JSON_ARRAY))
    {
        TEST(array.length == 7);

        JValue array_number = array.array[0];
        if (TEST(array_number.type == JSON_NUMBER))
            TEST(array_number.number == 69);

        JValue array_string = array.array[1];
        if (TEST(array_string.type == JSON_STRING))
            TEST(strcmp(array_string.string, "another test string") == 0);

        JValue array_boolean = array.array[2];
        if (TEST(array_boolean.type == JSON_BOOL))
            TEST(!array_boolean.boolean);

        JValue array_null= array.array[3];
        if (TEST(array_null.type == JSON_NULL))
            TEST(array_null.null == 0);

        JValue array_array = array.array[4];
        if (TEST(array_array.type == JSON_ARRAY))
        {
            JValue array_array_string = array_array.array[0];
            if (TEST(array_array_string.type == JSON_STRING))
                TEST(strcmp(array_array_string.string, "yet another test string") == 0);
        }

        JValue array_object = array.array[5];
        if (TEST(array_object.type == JSON_OBJECT))
        {
            JValue array_object_array = json_get(&array_object.object, "test");
            if (TEST(array_object_array.type == JSON_ARRAY))
            {
                JValue array_object_array_object = array_object_array.array[0];
                if (TEST(array_object_array_object.type == JSON_OBJECT))
                {
                    JValue array_object_array_object_string = json_get(&array_object_array_object.object, "nested");
                    if (TEST(array_object_array_object_string.type == JSON_STRING))
                        TEST(strcmp(array_object_array_object_string.string, "nested string") == 0);
                }
                
            }
        }

        JValue nested_array_object = array.array[6];
        if (TEST(nested_array_object.type == JSON_OBJECT))
        {
            JValue nested_array_object_number = json_get(&nested_array_object.object, "nested_array_object");
            if (TEST(nested_array_object_number.type == JSON_NUMBER))
                TEST(nested_array_object_number.number == 69);
        }

    }

    JValue empty_array = json_get(&json.object, "empty_array");
    if (TEST(empty_array.type == JSON_ARRAY))
    {
        TEST(empty_array.length == 0);
        TEST(empty_array.array == 0);
    }

    JValue empty_object = json_get(&json.object, "empty_object");
    if (TEST(empty_object.type == JSON_OBJECT))
        TEST(empty_object.object.pairs == 0);

    JValue object = json_get(&json.object, "object");
    if (TEST(object.type == JSON_OBJECT))
    {
        JValue array_object_string = json_get(&object.object, "array_object");
        if (TEST(array_object_string.type == JSON_STRING))
        {
            TEST(array_object_string.length == 19);
            TEST(strcmp(array_object_string.string, "array object string") == 0);
        }
    }

    jmem_free(memory);
}

void test_errors(void)
{
    const char* input = "{\"key\":}";

    JMemory *memory = jmem_init();
    TEST(memory != 0);

    JParser parser = json_init(memory, input);

    JValue json = json_parse(&parser);
    TEST(json.type == JSON_ERROR);
    TEST(json.error == JSON_PARSE_ERROR);

    jmem_free(memory);

    const char* input_2 = "{\"key\":69}";

    memory = jmem_init();
    TEST(memory != 0);

    parser = json_init(memory, input_2);
    json = json_parse(&parser);
    TEST(json.type == JSON_OBJECT);
    
    JValue key_not_found = json_get(&json.object, "error_key");
    TEST(key_not_found.type == JSON_ERROR);
    TEST(key_not_found.error == JSON_KEY_NOT_FOUND);

    jmem_free(memory);
}

void test_single_array(void)
{
    const char* input = "[69]";

    JMemory *memory = jmem_init();
    TEST(memory != 0);

    JParser parser = json_init(memory, input);

    JValue array = json_parse(&parser);
    if (TEST(array.type == JSON_ARRAY))
    {
        JValue array_number = array.array[0];
        if (TEST(array_number.type == JSON_NUMBER))
            TEST(array_number.number == 69);
    }

    jmem_free(memory);
}

void test_memory_error(void)
{
    const char* input = "{\"k\":\"v\"}";

    JMemory *memory = jmem_init();
    TEST(memory != 0);

    JMemory fake_memory = { .base = memory->base, .alloc = returns_null, .struct_ptr = 0 };
    JParser parser = json_init(&fake_memory, input);
    JValue json = json_parse(&parser);

    if (TEST(json.type == JSON_ERROR))
        TEST(json.error == JSON_MEMORY_ERROR);

    jmem_free(memory);
}

void test_input(void)
{
    {
        const char* string_literal_input = "{\"k\":\"v\"}";

        JMemory *memory = jmem_init();
        TEST(memory != 0);

        JParser parser = json_init(memory, string_literal_input);
        JValue json = json_parse(&parser);

        if (TEST(json.type == JSON_OBJECT))
        {
            JValue object_string = json_get(&json.object, "k");
            if (TEST(object_string.type == JSON_STRING))
            {
                TEST(object_string.length == 1);
                TEST(object_string.string[0] == 'v');
            }
        }

        jmem_free(memory);
    }

    {
        JMemory *memory = jmem_init();
        TEST(memory != 0);

        char *file_input = read_file_as_str("tests/test_input.json");

        JParser parser = json_init(memory, file_input);
        JValue json = json_parse(&parser);
        if (TEST(json.type == JSON_OBJECT))
        {
            JValue object_array = json_get(&json.object, "test key");
            if (TEST(object_array.type == JSON_ARRAY))
            {
                TEST(object_array.length == 1);
                JValue array_string = object_array.array[0];
                if (TEST(array_string.type == JSON_STRING))
                {
                    TEST(array_string.length == 10);
                    TEST(strcmp(array_string.string, "test value") == 0);
                }
            }
        }

        jmem_free(memory);
        free(file_input);
    }

    {
        JMemory *memory = jmem_init();
        TEST(memory != 0);

        char stack_input[] = "[\"hello, stack!\"]";

        JParser parser = json_init(memory, stack_input);
        JValue array = json_parse(&parser);
        if (TEST(array.type == JSON_ARRAY))
        {
            TEST(array.length == 1);
            JValue array_string = array.array[0];
            if (TEST(array_string.type == JSON_STRING))
            {
                TEST(array_string.length == 13);
                TEST(strcmp(array_string.string, "hello, stack!") == 0);
            }
        }

        jmem_free(memory);
    }
}

Test tests[] = {
    { .name = "values", .f = test_values },
    { .name = "errors", .f = test_errors },
    { .name = "single array", .f = test_single_array },
    { .name = "memory error", .f = test_memory_error },
    { .name = "input", .f = test_input },
};

int main(void)
{
    for (size_t i = 0; i < COUNT(tests); ++i)
        tests[i].f();

    TOTAL_ERRORS
}
