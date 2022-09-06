void *wasm_alloc(unsigned long long int size);

#define JP_DEFAULT_ALLOC wasm_alloc
#define JP_ALLOC_SIZE_TYPE unsigned long long int
#define JP_IMPLEMENTATION
#include "../jp.h"

void print(double value);
void output_result(const char *result);

#define EXPORT(s) __attribute__((export_name(s)))

extern unsigned char __heap_base;
char *bump_pointer = (char *)&__heap_base;

void* EXPORT("wasm_alloc") wasm_alloc(unsigned long long int size)
{
    char *ptr = bump_pointer;
    bump_pointer += size;
    return (void *)ptr;
}

const char *json_error_to_string(JCode code)
{
    switch (code)
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
        case JSON_OBJECT: return "JSON_OBJECT";
        case JSON_STRING: return "JSON_STRING";
        case JSON_BOOL:   return "JSON_BOOL";
        case JSON_NULL:   return "JSON_NULL";
        case JSON_NUMBER: return "JSON_NUMBER";
        case JSON_ARRAY:  return "JSON_ARRAY";
        case JSON_ERROR:  return "JSON_ERROR";
    }
    return "UNKNOWN";
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
        output_result(json_type_to_string(json.type));
}
