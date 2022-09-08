void *wasm_alloc(unsigned long long int size);

#define JP_DEFAULT_ALLOC wasm_alloc
#define JP_ALLOC_SIZE_TYPE unsigned long long int
#define JP_IMPLEMENTATION
#include "../jp.h"

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

void print(double value);
void output_result(const char *result);

#define EXPORT(s) __attribute__((export_name(s)))

extern unsigned char __heap_base;
char *bump_pointer = (char *)&__heap_base;

void json_value_to_string(JValue value);

void* EXPORT("wasm_alloc") wasm_alloc(unsigned long long int size)
{
    char *ptr = bump_pointer;
    bump_pointer += size;
    return (void *)ptr;
}

const char *json_error_to_string(JCode error)
{
    switch (error)
    {
        case JSON_KEY_NOT_FOUND:  return "JSON_KEY_NOT_FOUND";
        case JSON_UNEXPECTED_EOF: return "JSON_UNEXPECTED_EOF";
        case JSON_PARSE_ERROR:    return "JSON_PARSE_ERROR";
        case JSON_TYPE_ERROR:     return "JSON_TYPE_ERROR";
        case JSON_MEMORY_ERROR:   return "JSON_MEMORY_ERROR";
    }
    return "UNKNOWN";
}

const char *json_type_to_string(JType type)
{
    switch (type)
    {
        case JSON_OBJECT: return "object";
        case JSON_STRING: return "string";
        case JSON_BOOL:   return "boolean";
        case JSON_NULL:   return "null";
        case JSON_NUMBER: return "number";
        case JSON_ARRAY:  return "array";
        case JSON_ERROR:  return "error";
    }
    return "UNKNOWN";
}

void json_object_to_string(JObject object)
{
    output_result("{");
    for (jsize_t i = 0; i < object.length; ++i)
    {
        JPair pair = object.data[i];
        output_result(pair.key);
        output_result(".");
        output_result(json_type_to_string(pair.value.type));
        output_result(": ");
        json_value_to_string(pair.value);
        if (i < object.length - 1)
            output_result(",\n");
    }
    output_result("}");
}

void json_bool_to_string(int boolean)
{
    boolean ? output_result("true") : output_result("false");
}

void json_null_to_string(int null)
{
    output_result("null");
}

void json_number_to_string(long long number)
{
    char string[512];
    stbsp_snprintf(string, 512, "%lld", number);
    output_result(string);
}

void json_array_to_string(JArray array)
{
    output_result("[");
    for (jsize_t i = 0; i < array.length; ++i)
    {
        json_value_to_string(array.data[i]);
        if (i < array.length - 1)
            output_result(", ");
    }
    output_result("]");
}

void json_string_to_string(JString string)
{
    output_result("\"");
    output_result(string.data);
    output_result("\"");
}

void json_value_to_string(JValue value)
{
    switch (value.type)
    {
        case JSON_OBJECT: {
            json_object_to_string(value.object);
        } break;
        case JSON_BOOL:   {
            json_bool_to_string(value.boolean);
        } break;
        case JSON_NULL:   {
            json_null_to_string(value.null);
        } break;
        case JSON_NUMBER: {
            json_number_to_string(value.number);
        } break;
        case JSON_ARRAY:  {
            json_array_to_string(value.array);
        } break;
        case JSON_ERROR:  {
            json_error_to_string(value.error);
        } break;
        case JSON_STRING: {
            json_string_to_string(value.string);
        } break;
    }
}

void EXPORT("parse_json") parse_json(const char* input)
{
    JValue json = json_parse(input);
    if (json.type == JSON_ERROR)
    {
        char result[64];
        const char *error = json_error_to_string(json.error);
        unsigned long long int size = 0;
        while (error[size] != 0)
            size++;
        json_memcpy(result, "JSON_ERROR: ", 12);
        json_memcpy(result + 12, error, size);
        output_result(result);
    }
    else
    {
        json_value_to_string(json);
    }
}
