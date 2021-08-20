#ifndef JP_H_
#define JP_H_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define EXIT(code) ExitProcess(code)
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <errno.h>
#define EXIT(code) exit(code)
#endif // _WIN32

#include <assert.h>
#include <stdint.h>
#include <string.h>

#if (!defined(NDEBUG)) && (defined(JP_DEBUG)) && ((defined(__cplusplus)) || (!defined(__clang__) && defined(__GNUC__)))
#include <stdio.h>
#define JP_PANIC(fmt, ...)                                                    \
    do                                                                        \
    {                                                                         \
        printf("[ERRO] L%d: " fmt "\n", __LINE__ __VA_OPT__(, ) __VA_ARGS__); \
        EXIT(1);                                                              \
    } while (0)
#elif !defined(NDEBUG)
#define JP_PANIC(fmt, ...) \
    do                     \
    {                      \
        assert(0 && fmt);  \
        EXIT(1);           \
    } while (0)
#else
#define JP_PANIC(fmt, ...) EXIT(1)
#endif // JP_PANIC

static size_t system_memory_size = 0;

typedef struct JPair JPair;
typedef struct JValue JValue;

typedef enum
{
    JSON_OBJECT = 0,
    JSON_STRING,
    JSON_BOOL,
    JSON_NULL,
    JSON_NUMBER,
    JSON_ARRAY,
} JType;

typedef enum
{
    TOKEN_KIND_NONE = 0,
    TOKEN_KIND_OPEN_CURLY,
    TOKEN_KIND_CLOSE_CURLY,
    TOKEN_KIND_OPEN_QUOTATION,
    TOKEN_KIND_CLOSE_QUOTATION,
    TOKEN_KIND_COLON,
    TOKEN_KIND_COMMA
} JTokenKind;

typedef enum
{
    STATE_NONE = 0,
    STATE_KEY,
    STATE_VALUE
} JState;

typedef struct
{
    JPair *pairs;
    size_t pairs_count;
} JObject;

#define JVALUEDEF         \
    JType type;           \
    union                 \
    {                     \
        JObject object;   \
        long long number; \
        int boolean;      \
        char *string;     \
        int null;         \
        JValue *array;    \
    };

#ifdef __cplusplus
struct JValue
{
    JVALUEDEF
    JValue operator[](const char *key);
    JValue operator[](size_t idx);
    JValue operator[](int idx);
};
#else
struct JValue
{
    JVALUEDEF
};
#endif // __cplusplus

struct JPair
{
    char *key;
    JValue *value;
};

typedef struct
{
    char *start;
    char *base;
#ifdef _WIN32
    size_t commited;
    size_t allocated;
    size_t commit_size;
#endif // _WIN32
} JMemory;

typedef struct
{
    JMemory memory;
    size_t pairs_total;
    size_t pairs_commited;
} JParser;

#ifndef _WIN32
int GetPhysicallyInstalledSystemMemory(size_t *output);
#endif // _WIN32

void json_memory_init(JMemory *memory);
void *json_memory_alloc(JMemory *memory, size_t size);
void json_memory_free(JMemory *memory);
void json_object_init(JObject *jobject, JPair *pairs);
void json_object_add_pair(JObject *jobject, char *key, JValue *value);
JValue json_get(JObject *jobject, const char *key);
JParser json_init(const char *input);
JValue json_parse(JParser *jparser, const char *input);
void string_to_number(const char *string, size_t *out);

#endif // JP_H_

#ifdef JP_IMPLEMENTATION

#ifdef __cplusplus
JValue JValue::operator[](const char *key)
{
    assert(type == JSON_OBJECT);
    for (size_t i = 0; i < object.pairs_count; ++i)
        if (strcmp(key, object.pairs[i].key) == 0)
            return *(object.pairs[i].value);
    JP_PANIC("key \"%s\" was not found", key);
}
JValue JValue::operator[](size_t idx)
{
    assert(type == JSON_ARRAY);
    return array[idx];
}
JValue JValue::operator[](int idx)
{
    return operator[](static_cast<size_t>(idx));
}
#endif // #ifdef __cplusplus

#ifndef _WIN32
int GetPhysicallyInstalledSystemMemory(size_t *output)
{
    FILE *meminfo = fopen("/proc/meminfo", "r");
    if (meminfo == NULL)
        JP_PANIC("Error opening file '/proc/meminfo'");

    char line[256];
    while (fgets(line, sizeof(line), meminfo))
    {
        if (sscanf(line, "MemTotal: %zu kB", output) == 1)
        {
            fclose(meminfo);
            return 1;
        }
    }

    fclose(meminfo);
    return 0;
}
#endif // _WIN32

void json_memory_init(JMemory *memory)
{
    if (system_memory_size == 0)
    {
        if (!GetPhysicallyInstalledSystemMemory(&system_memory_size))
            system_memory_size = INTPTR_MAX == INT32_MAX ? 4294967295ULL : 16ULL * 1024 * 1024 * 1024;
        else
            system_memory_size *= 1024;
    }
#ifdef _WIN32
    memory->commited = 0;
    memory->allocated = 0;
    memory->commit_size = 1024;
    memory->base = (char *)(VirtualAlloc(NULL, system_memory_size, MEM_RESERVE, PAGE_READWRITE));
    if (memory->base == NULL)
        JP_PANIC("VirtualAlloc failed: %lu", GetLastError());
#else
    memory->base = (char *)(mmap(0, system_memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (memory->base == MAP_FAILED)
        JP_PANIC("mmap failed: %s", strerror(errno));
#endif // _WIN32
    memory->start = memory->base;
}

void *json_memory_alloc(JMemory *memory, size_t size)
{
#ifdef _WIN32
    memory->allocated += size;
    if (memory->allocated * 2 > memory->commited)
    {
        memory->commit_size += size * 2;
        if (VirtualAlloc(memory->start, memory->commit_size, MEM_COMMIT, PAGE_READWRITE) == NULL)
            JP_PANIC("VirtualAlloc failed: %lu", GetLastError());
        memory->commited += memory->commit_size;
    }
#endif // _WIN32
    memory->start += size;
    return memory->start - size;
}

void json_memory_free(JMemory *memory)
{
#ifdef _WIN32
    if (VirtualFree(memory->base, 0, MEM_RELEASE) == 0)
        JP_PANIC("VirtualFree failed: %lu", GetLastError());
#else
    if (munmap(memory->base, system_memory_size) == -1)
        JP_PANIC("munmap failed: %s", strerror(errno));
#endif // _WIN32
}

void json_object_init(JObject *jobject, JPair *pairs)
{
    jobject->pairs = pairs;
    jobject->pairs_count = 0;
}

void json_object_add_pair(JObject *jobject, char *key, JValue *value)
{
    jobject->pairs[jobject->pairs_count].key = key;
    jobject->pairs[jobject->pairs_count].value = value;
    jobject->pairs_count++;
}

JValue json_get(JObject *jobject, const char *key)
{
    for (size_t i = 0; i < jobject->pairs_count; ++i)
        if (strcmp(key, jobject->pairs[i].key) == 0)
            return *(jobject->pairs[i].value);
    JP_PANIC("key \"%s\" was not found", key);
}

JParser json_init(const char *input)
{
    JParser parser;
    json_memory_init(&parser.memory);
    parser.pairs_total = 0;
    parser.pairs_commited = 0;
    for (size_t i = 0; input[i] != 0; ++i)
        if (input[i] == ':' && input[i - 1] == '"' && input[i - 2] != '\\')
            parser.pairs_total++;
    json_memory_alloc(&parser.memory, sizeof(JPair) * parser.pairs_total);
    return parser;
}

JValue json_parse(JParser *jparser, const char *input)
{
    JTokenKind current_token_kind = TOKEN_KIND_NONE;
    JState state = STATE_NONE;

    char *current_key = NULL;
    JValue *current_value = NULL;

    size_t pairs_offset = sizeof(JPair) * jparser->pairs_commited;
    JPair *pairs_start = (JPair *)(jparser->memory.base + pairs_offset);
    JValue json;
    json.type = JSON_OBJECT;
    json_object_init(&json.object, pairs_start);

    for (size_t pos = 0; input[pos] != '\0'; ++pos)
    {
        if (input[pos] == ' ' ||
            input[pos] == '\t' ||
            input[pos] == '\n' ||
            input[pos] == '\r')
            continue;
        switch (input[pos])
        {
        case 't':
        {
            switch (current_token_kind)
            {
            case TOKEN_KIND_COLON:
            {
                size_t start_pos = pos;
                do
                {
                    pos++;
                    if (input[pos] == '\0')
                        JP_PANIC("unexpected end of file at %zu", pos);
                } while (pos - start_pos != 4);
                // TODO(#5): free unused strings
                char *bool_string = (char *)json_memory_alloc(&jparser->memory, 5);
                memcpy(bool_string, input + start_pos, 4);
                bool_string[4] = '\0';
                if (strcmp(bool_string, "true") == 0)
                {
                    current_value = (JValue *)json_memory_alloc(&jparser->memory, sizeof(JValue));
                    current_value->type = JSON_BOOL;
                    current_value->boolean = 1;
                    current_token_kind = TOKEN_KIND_CLOSE_QUOTATION;
                    state = STATE_NONE;
                    pos--;
                    break;
                }
                JP_PANIC("failed to parse true");
            }
            break;
            default:
                goto process_char;
            }
        }
        break;
        case 'f':
        {
            switch (current_token_kind)
            {
            case TOKEN_KIND_COLON:
            {
                size_t start_pos = pos;
                do
                {
                    pos++;
                    if (input[pos] == '\0')
                        JP_PANIC("unexpected end of file at %zu", pos);
                } while (pos - start_pos != 5);
                char *bool_string = (char *)json_memory_alloc(&jparser->memory, 6);
                memcpy(bool_string, input + start_pos, 5);
                bool_string[5] = '\0';
                if (strcmp(bool_string, "false") == 0)
                {
                    current_value = (JValue *)json_memory_alloc(&jparser->memory, sizeof(JValue));
                    current_value->type = JSON_BOOL;
                    current_value->boolean = 0;
                    current_token_kind = TOKEN_KIND_CLOSE_QUOTATION;
                    state = STATE_NONE;
                    pos--;
                    break;
                }
                JP_PANIC("failed to parse false");
            }
            break;
            default:
                goto process_char;
            }
        }
        break;
        case 'n':
        {
            switch (current_token_kind)
            {
            case TOKEN_KIND_COLON:
            {
                size_t start_pos = pos;
                do
                {
                    pos++;
                    if (input[pos] == '\0')
                        JP_PANIC("unexpected end of file at %zu", pos);
                } while (pos - start_pos != 4);
                char *null_string = (char *)json_memory_alloc(&jparser->memory, 5);
                memcpy(null_string, input + start_pos, 4);
                null_string[4] = '\0';
                if (strcmp(null_string, "null") == 0)
                {
                    current_value = (JValue *)json_memory_alloc(&jparser->memory, sizeof(JValue));
                    current_value->type = JSON_NULL;
                    current_value->null = 0;
                    current_token_kind = TOKEN_KIND_CLOSE_QUOTATION;
                    state = STATE_NONE;
                    pos--;
                    break;
                }
                JP_PANIC("failed to parse null");
            }
            break;
            default:
                goto process_char;
            }
        }
        break;
        // TODO(#4): negative numbers
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
            switch (current_token_kind)
            {
            case TOKEN_KIND_COLON:
            {
                size_t start_pos = pos;
                do
                {
                    pos++;
                    if (input[pos] == '\0')
                        JP_PANIC("unexpected end of file at %zu", pos);
                } while (input[pos] != ',' && input[pos] != '}');
                size_t number_string_size = pos - start_pos + 1;
                char *number_string = (char *)json_memory_alloc(&jparser->memory, number_string_size);
                memcpy(number_string, input + start_pos, number_string_size - 1);
                number_string[number_string_size] = '\0';
                size_t number = 0;
                string_to_number(number_string, &number);
                current_value = (JValue *)json_memory_alloc(&jparser->memory, sizeof(JValue));
                current_value->type = JSON_NUMBER;
                current_value->number = number;
                current_token_kind = TOKEN_KIND_CLOSE_QUOTATION;
                state = STATE_NONE;
                pos--;
            }
            break;
            default:
                goto process_char;
            }
        }
        break;
        // TODO(#3): nested arrays, object
        case '[':
        {
            switch (current_token_kind)
            {
            case TOKEN_KIND_COLON:
            {
                size_t start_pos = pos;
                size_t array_values_count = 0;
                int inside_string = 0;
                do
                {
                    pos++;
                    if (input[pos] == '\0')
                        JP_PANIC("unexpected end of file at %zu", pos);
                    if (input[pos] == '"') {
                        inside_string = !inside_string;
                    }
                    if (!inside_string && input[pos] == ',')
                    {
                        if (array_values_count == 0)
                        {
                            array_values_count = 2;
                        }
                        else
                        {
                            array_values_count++;
                        }
                    }
                } while (input[pos] != ']');
                if (array_values_count == 0 && pos - start_pos != 1)
                {
                    array_values_count = 1;
                }
                size_t i = pos - 1;
                while (input[i--] == ' ');
                if (input[i + 1] == ',')
                {
                    JP_PANIC("failed to parse array at %zu", i + 1);
                }

                JValue *array_values = (JValue *)json_memory_alloc(&jparser->memory, sizeof(JValue) * array_values_count);

                start_pos++;
                for (size_t j = 0; j < array_values_count; ++j)
                {
                    switch (input[start_pos])
                    {
                    case '"':
                    {
                        size_t i = start_pos + 1;
                        do
                        {
                            start_pos++;
                            if (input[start_pos] == '\0')
                            {
                                JP_PANIC("unexpected end of file at %zu", start_pos);
                            }
                        } while (input[start_pos] != '"' && input[start_pos - 1] != '\\');
                        size_t value_size = start_pos - i + 1;
                        char *value_string = (char *)json_memory_alloc(&jparser->memory, value_size);
                        memcpy(value_string, input + i, value_size - 1);
                        value_string[value_size - 1] = '\0';
                        array_values[j].type = JSON_STRING;
                        array_values[j].string = value_string;
                        if (array_values_count > 1)
                        {
                            do
                            {
                                start_pos++;
                            } while (input[start_pos] == ',' || input[start_pos] == ' ');
                        }
                    }
                    break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    {
                        size_t i = start_pos;
                        do
                        {
                            start_pos++;
                            if (input[start_pos] == '\0')
                                JP_PANIC("unexpected end of file at %zu", start_pos);
                        } while (input[start_pos] != ',' && input[start_pos] != ']');
                        size_t number_string_size = start_pos - i + 1;
                        char *number_string = (char *)json_memory_alloc(&jparser->memory, number_string_size);
                        memcpy(number_string, input + i, number_string_size - 1);
                        number_string[number_string_size] = '\0';
                        size_t number = 0;
                        string_to_number(number_string, &number);
                        array_values[j].type = JSON_NUMBER;
                        array_values[j].number = number;
                        if (array_values_count > 1)
                        {
                            do
                            {
                                start_pos++;
                            } while (input[start_pos] == ',' || input[start_pos] == ' ');
                        }
                    }
                    break;
                    case 't':
                    {
                        size_t i = start_pos;
                        do
                        {
                            start_pos++;
                            if (input[start_pos] == '\0')
                            {
                                JP_PANIC("unexpected end of file at %zu", start_pos);
                            }
                        } while (start_pos - i != 4);
                        char *bool_string = (char *)json_memory_alloc(&jparser->memory, 5);
                        memcpy(bool_string, input + i, 4);
                        bool_string[4] = '\0';
                        if (strcmp(bool_string, "true") == 0)
                        {
                            array_values[j].type = JSON_BOOL;
                            array_values[j].boolean = 1;
                            if (array_values_count > 1)
                            {
                                do
                                {
                                    start_pos++;
                                } while (input[start_pos] == ',' || input[start_pos] == ' ');
                            }
                            break;
                        }
                        JP_PANIC("failed to parse true");
                    }
                    break;
                    case 'f':
                    {
                        size_t i = start_pos;
                        do
                        {
                            start_pos++;
                            if (input[start_pos] == '\0')
                            {
                                JP_PANIC("unexpected end of file at %zu", start_pos);
                            }
                        } while (start_pos - i != 5);
                        char *bool_string = (char *)json_memory_alloc(&jparser->memory, 6);
                        memcpy(bool_string, input + i, 5);
                        bool_string[5] = '\0';
                        if (strcmp(bool_string, "false") == 0)
                        {
                            array_values[j].type = JSON_BOOL;
                            array_values[j].boolean = 0;
                            if (array_values_count > 1)
                            {
                                do
                                {
                                    start_pos++;
                                } while (input[start_pos] == ',' || input[start_pos] == ' ');
                            }
                            break;
                        }
                        JP_PANIC("failed to parse false");
                    }
                    break;
                    case 'n':
                    {
                        size_t i = start_pos;
                        do
                        {
                            start_pos++;
                            if (input[start_pos] == '\0')
                            {
                                JP_PANIC("unexpected end of file at %zu", start_pos);
                            }
                        } while (start_pos - i != 4);
                        char *null_string = (char *)json_memory_alloc(&jparser->memory, 5);
                        memcpy(null_string, input + i, 4);
                        null_string[4] = '\0';
                        if (strcmp(null_string, "null") == 0)
                        {
                            array_values[j].type = JSON_NULL;
                            array_values[j].null = 0;
                            if (array_values_count > 1)
                            {
                                do
                                {
                                    start_pos++;
                                } while (input[start_pos] == ',' || input[start_pos] == ' ');
                            }
                            break;
                        }
                        JP_PANIC("failed to parse null");
                    }
                    break;
                    default:
                        JP_PANIC("unknown char: '%c'", input[start_pos]);
                    }
                }

                current_value = (JValue *)json_memory_alloc(&jparser->memory, sizeof(JValue));
                current_value->type = JSON_ARRAY;
                current_value->array = array_values;
                current_token_kind = TOKEN_KIND_CLOSE_QUOTATION;
                state = STATE_NONE;
            }
            break;
            default:
                goto process_char;
            }
        }
        break;
        case '"':
        {
            switch (current_token_kind)
            {
            case TOKEN_KIND_COMMA:
            case TOKEN_KIND_OPEN_CURLY:
            {
                current_token_kind = TOKEN_KIND_OPEN_QUOTATION;
                state = STATE_KEY;
            }
            break;
            case TOKEN_KIND_OPEN_QUOTATION:
            {
                if (input[pos - 1] != '\\')
                    current_token_kind = TOKEN_KIND_CLOSE_QUOTATION;
                else
                    goto process_char;
            }
            break;
            case TOKEN_KIND_COLON:
            {
                current_token_kind = TOKEN_KIND_OPEN_QUOTATION;
                state = STATE_VALUE;
            }
            break;
            default:
            {
                JP_PANIC("unreachable, unknown token kind: %d", current_token_kind);
            }
            break;
            }
        }
        break;
        case ',':
        {
            if (current_token_kind == TOKEN_KIND_CLOSE_QUOTATION ||
                current_token_kind == TOKEN_KIND_CLOSE_CURLY)
            {
                json_object_add_pair(&json.object, current_key, current_value);
                current_token_kind = TOKEN_KIND_COMMA;
            }
            else
            {
                goto process_char;
            }
        }
        break;
        case ':':
        {
            if (current_token_kind == TOKEN_KIND_CLOSE_QUOTATION)
                current_token_kind = TOKEN_KIND_COLON;
            else
                goto process_char;
        }
        break;
        case '{':
        {
            switch (current_token_kind)
            {
            case TOKEN_KIND_NONE:
            {
                size_t i = pos;
                do
                {
                    i++;
                    if (input[i] == '\0')
                        JP_PANIC("unexpected end of file at %zu", i);
                    if (input[i] == '{')
                    {
                        size_t open_curly_count = 2;
                        do
                        {
                            i++;
                            if (input[i] == '\0')
                            {
                                JP_PANIC("unexpected end of file at %zu", i);
                            }
                            if (input[i] == '{')
                            {
                                open_curly_count++;
                            }
                            else if (input[i] == '}')
                            {
                                open_curly_count--;
                            }
                        } while (open_curly_count != 1);
                        i++;
                    }
                    else if (input[i] == ':' && input[i - 1] == '"' && input[i - 2] != '\\')
                        jparser->pairs_commited++;
                } while (input[i] != '}');
                current_token_kind = TOKEN_KIND_OPEN_CURLY;
            }
            break;
            case TOKEN_KIND_COLON:
            {
                size_t start_pos = pos;
                size_t open_curly_count = 1;
                do
                {
                    pos++;
                    if (input[pos] == '\0')
                    {
                        JP_PANIC("unexpected end of file at %zu", pos);
                    }
                    if (input[pos] == '{')
                    {
                        open_curly_count++;
                    }
                    else if (input[pos] == '}')
                    {
                        open_curly_count--;
                    }
                } while (open_curly_count != 0);
                size_t nested_object_string_size = pos - start_pos + 2;
                char *nested_object_string = (char *)json_memory_alloc(&jparser->memory, nested_object_string_size);
                memcpy(nested_object_string, input + start_pos, nested_object_string_size - 1);
                nested_object_string[nested_object_string_size - 1] = '\0';
                JObject nested_object = json_parse(jparser, nested_object_string).object;
                current_value = (JValue *)json_memory_alloc(&jparser->memory, sizeof(JValue));
                current_value->type = JSON_OBJECT;
                current_value->object = nested_object;
                current_token_kind = TOKEN_KIND_CLOSE_CURLY;
                state = STATE_NONE;
            }
            break;
            default:
                goto process_char;
            }
        }
        break;
        case '}':
        {
            if (current_token_kind == TOKEN_KIND_CLOSE_QUOTATION ||
                current_token_kind == TOKEN_KIND_CLOSE_CURLY)
            {
                json_object_add_pair(&json.object, current_key, current_value);
                current_token_kind = TOKEN_KIND_CLOSE_CURLY;
            }
            else
            {
                goto process_char;
            }
        }
        break;
        default:
        {
            switch (current_token_kind)
            {
            case TOKEN_KIND_OPEN_QUOTATION:
            {
            process_char:
                if (state == STATE_KEY)
                {
                    size_t start_pos = pos;
                    do
                    {
                        pos++;
                        if (input[pos] == '\0')
                            JP_PANIC("unexpected end of file at %zu", pos);
                    } while (input[pos] != '"' && input[pos - 1] != '\\');
                    size_t key_size = pos - start_pos + 1;
                    current_key = (char *)json_memory_alloc(&jparser->memory, key_size);
                    memcpy(current_key, input + start_pos, key_size - 1);
                    current_key[key_size - 1] = '\0';
                    current_token_kind = TOKEN_KIND_CLOSE_QUOTATION;
                    state = STATE_NONE;
                }
                else if (state == STATE_VALUE)
                {
                    size_t start_pos = pos;
                    do
                    {
                        pos++;
                        if (input[pos] == '\0')
                            JP_PANIC("unexpected end of file at %zu", pos);
                    } while (input[pos] != '"' && input[pos - 1] != '\\');
                    size_t value_size = pos - start_pos + 1;
                    char *value_string = (char *)json_memory_alloc(&jparser->memory, value_size);
                    memcpy(value_string, input + start_pos, value_size - 1);
                    value_string[value_size - 1] = '\0';
                    current_value = (JValue *)json_memory_alloc(&jparser->memory, sizeof(JValue));
                    current_value->type = JSON_STRING;
                    current_value->string = value_string;
                    current_token_kind = TOKEN_KIND_CLOSE_QUOTATION;
                    state = STATE_NONE;
                }
            }
            break;
            default:
            {
                JP_PANIC("unknown char: '%c'", input[pos]);
            }
            break;
            }
        }
        break;
        }
    }
    return json;
}

void string_to_number(const char *string, size_t *out)
{
    size_t position = 0;
    size_t string_length = strlen(string);
    while (position < string_length)
    {
        size_t digit = string[position] - 48;
        *out = *out * 10 + digit;
        position++;
    }
}

#endif // JP_IMPLEMENTATION
