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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#define EXIT(code) exit(code)
#endif // _WIN32

#include <assert.h>
#include <stdint.h>
#include <string.h>

// TODO(#12): customizable allocator
// TODO(#9): do not allocate value strings, just use memcmp
// TODO(#10): error reporting
#if (!defined(NDEBUG)) && (defined(JP_DEBUG)) &&                               \
    ((defined(__cplusplus)) || (!defined(__clang__) && defined(__GNUC__)))
#include <stdio.h>
#define JP_PANIC(fmt, ...)                                                     \
    do                                                                         \
    {                                                                          \
        printf("[ERRO] L%d: " fmt "\n", __LINE__ __VA_OPT__(, ) __VA_ARGS__);  \
        EXIT(1);                                                               \
    } while (0)
#elif !defined(NDEBUG)
#define JP_PANIC(fmt, ...)                                                     \
    do                                                                         \
    {                                                                          \
        assert(0 && fmt);                                                      \
        EXIT(1);                                                               \
    } while (0)
#else
#define JP_PANIC(fmt, ...) EXIT(1)
#endif // JP_PANIC

#define UNEXPECTED_EOF(chr, pos)                                               \
    if (chr == '\0')                                                           \
    JP_PANIC("unexpected end of file at %zu", pos)

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

typedef struct
{
    JPair *pairs;
    size_t pairs_count;
} JObject;

struct JValue
{
    JType type;
    union
    {
        JObject object;
        long long number;
        int boolean;
        char *string;
        int null;
        JValue *array;
    };
#ifdef __cplusplus
    JValue operator[](const char *key);
    JValue operator[](size_t idx);
    JValue operator[](int idx);
#endif // __cplusplus
};

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

int json_whitespace_char(char c);
int json_match_char(char c, const char *input, size_t *pos);
void json_skip_whitespaces(const char *input, size_t *pos);
void json_memory_init(JMemory *memory);
void json_memory_free(JMemory *memory);
void *json_memory_alloc(JMemory *memory, size_t size);
void json_object_init(JObject *object, JPair *pairs);
void json_object_add_pair(JObject *object, char *key, JValue *value);
JParser json_init(const char *input);
JValue json_get(JObject *object, const char *key);
JValue json_parse(JParser *parser, const char *input);
JValue *json_parse_object(JParser *parser, const char *input, size_t *pos);
JValue *json_parse_value(JParser *parser, const char *input, size_t *pos);
JValue *json_parse_string(JParser *parser, const char *input, size_t *pos);
JValue *json_parse_number(JParser *parser, const char *input, size_t *pos,
                          int negative);
JValue *json_parse_boolean(JParser *parser, const char *input, size_t *pos,
                           int bool_value, const char *bool_string,
                           size_t bool_string_length);
JValue *json_parse_null(JParser *parser, const char *input, size_t *pos);
JValue *json_parse_array(JParser *parser, const char *input, size_t *pos);

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
#endif // __cplusplus

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

void json_skip_whitespaces(const char *input, size_t *pos)
{
    while (json_whitespace_char(input[*pos]))
        (*pos)++;
    UNEXPECTED_EOF(input[*pos], *pos);
}

int json_match_char(char c, const char *input, size_t *pos)
{
    do
    {
        UNEXPECTED_EOF(input[*pos], *pos);
    } while (json_whitespace_char(input[(*pos)++]));
    int match = input[*pos - 1] == c;
    if (!match)
        JP_PANIC("expected '%c' found '%c' at %zu", c, input[*pos - 1],
                 *pos - 1);
    return match;
}

int json_whitespace_char(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void json_memory_init(JMemory *memory)
{
    if (system_memory_size == 0)
    {
        if (!GetPhysicallyInstalledSystemMemory(&system_memory_size))
            system_memory_size = INTPTR_MAX == INT32_MAX
                                     ? 4294967295ULL
                                     : 16ULL * 1024 * 1024 * 1024;
        else
            system_memory_size *= 1024;
    }
#ifdef _WIN32
    memory->commited = 0;
    memory->allocated = 0;
    memory->commit_size = 1024;
    memory->base = (char *)(VirtualAlloc(NULL, system_memory_size, MEM_RESERVE,
                                         PAGE_READWRITE));
    if (memory->base == NULL)
        JP_PANIC("VirtualAlloc failed: %lu", GetLastError());
#else
    memory->base = (char *)(mmap(0, system_memory_size, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
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
        if (VirtualAlloc(memory->start, memory->commit_size, MEM_COMMIT,
                         PAGE_READWRITE) == NULL)
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

void json_object_init(JObject *object, JPair *pairs)
{
    object->pairs = pairs;
    object->pairs_count = 0;
}

void json_object_add_pair(JObject *object, char *key, JValue *value)
{
    object->pairs[object->pairs_count].key = key;
    object->pairs[object->pairs_count].value = value;
    object->pairs_count++;
}

JValue json_get(JObject *object, const char *key)
{
    for (size_t i = 0; i < object->pairs_count; ++i)
        if (strcmp(key, object->pairs[i].key) == 0)
            return *(object->pairs[i].value);
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

JValue json_parse(JParser *parser, const char *input)
{
    size_t pos = 0;
    return *json_parse_object(parser, input, &pos);
}

JValue *json_parse_string(JParser *parser, const char *input, size_t *pos)
{
    (*pos)++;
    size_t start = *pos;
    while (input[*pos] != '"' && input[*pos - 1] != '\\')
        UNEXPECTED_EOF(input[(*pos)++], *pos - 1);
    char *value_string = 0;
    if (*pos - start != 0)
    {
        size_t string_size = *pos - start + 1;
        value_string = (char *)json_memory_alloc(&parser->memory, string_size);
        memcpy(value_string, input + start, string_size - 1);
        value_string[string_size - 1] = '\0';
    }
    JValue *value =
        (JValue *)json_memory_alloc(&parser->memory, sizeof(JValue));
    value->type = JSON_STRING;
    value->string = value_string;
    (*pos)++;
    return value;
}

JValue *json_parse_number(JParser *parser, const char *input, size_t *pos,
                          int negative)
{
    if (negative)
        (*pos)++;
    size_t start_pos = *pos;
    while (!json_whitespace_char(input[*pos]) && input[*pos] != ',' &&
           input[*pos] != '}' && input[*pos] != ']')
        UNEXPECTED_EOF(input[(*pos)++], *pos - 1);
    size_t number_string_length = *pos - start_pos;
    size_t number = 0;
    size_t i = 0;
    while (i < number_string_length)
    {
        size_t digit = (input + start_pos)[i] - 48;
        number = number * 10 + digit;
        i++;
    }
    if (negative)
        number = -(long long)number;
    JValue *value =
        (JValue *)json_memory_alloc(&parser->memory, sizeof(JValue));
    value->type = JSON_NUMBER;
    value->number = number;
    return value;
}

JValue *json_parse_boolean(JParser *parser, const char *input, size_t *pos,
                           int bool_value, const char *bool_string,
                           size_t bool_string_length)
{
    if (memcmp(input + *pos, bool_string, bool_string_length) == 0)
    {
        JValue *value =
            (JValue *)json_memory_alloc(&parser->memory, sizeof(JValue));
        value->type = JSON_BOOL;
        value->boolean = bool_value;
        *pos += bool_string_length;
        return value;
    }
    JP_PANIC("failed to parse %s", bool_string);
}

JValue *json_parse_null(JParser *parser, const char *input, size_t *pos)
{
    if (memcmp(input + *pos, "null", 4) == 0)
    {
        JValue *value =
            (JValue *)json_memory_alloc(&parser->memory, sizeof(JValue));
        value->type = JSON_NULL;
        value->null = 0;
        *pos += 4;
        return value;
    }
    JP_PANIC("failed to parse null");
}

JValue *json_parse_array(JParser *parser, const char *input, size_t *pos)
{
    (*pos)++;
    json_skip_whitespaces(input, pos);
    JValue *value =
        (JValue *)json_memory_alloc(&parser->memory, sizeof(JValue));
    value->type = JSON_ARRAY;
    size_t start_pos = *pos;
    if (input[start_pos] == ']')
    {
        value->array = 0;
        (*pos)++;
        return value;
    }
    size_t array_values_count = 1;
    int inside_string = 0;
    do
    {
        UNEXPECTED_EOF(input[start_pos], start_pos);
        if (input[start_pos] == '"')
            inside_string = !inside_string;
        if (!inside_string && input[start_pos] == ',')
            array_values_count++;
    } while (input[++start_pos] != ']');
    JValue *array_values = (JValue *)json_memory_alloc(
        &parser->memory, sizeof(JValue) * array_values_count);
    for (size_t i = 0; i < array_values_count; ++i)
    {
        array_values[i] = *json_parse_value(parser, input, pos);
        (*pos)++;
        json_skip_whitespaces(input, pos);
    }
    value->array = array_values;
    return value;
}

JValue *json_parse_value(JParser *parser, const char *input, size_t *pos)
{
    switch (input[*pos])
    {
    case '{':
        return json_parse_object(parser, input, pos);
    case '[':
        return json_parse_array(parser, input, pos);
    case '"':
        return json_parse_string(parser, input, pos);
    case '-':
        return json_parse_number(parser, input, pos, 1);
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
        return json_parse_number(parser, input, pos, 0);
    case 't':
        return json_parse_boolean(parser, input, pos, 1, "true", 4);
    case 'f':
        return json_parse_boolean(parser, input, pos, 0, "false", 5);
    case 'n':
        return json_parse_null(parser, input, pos);
    default:
        JP_PANIC("unknown char %c at %zu", input[*pos], *pos);
    }
}

JValue *json_parse_object(JParser *parser, const char *input, size_t *pos)
{
    json_match_char('{', input, pos);

    JPair *pairs_start =
        (JPair *)(parser->memory.base + sizeof(JPair) * parser->pairs_commited);
    JValue *object =
        (JValue *)json_memory_alloc(&parser->memory, sizeof(JValue));
    object->type = JSON_OBJECT;
    json_object_init(&object->object, pairs_start);

    size_t i = *pos;
    do
    {
        UNEXPECTED_EOF(input[i], i);
        if (input[i] == '{')
        {
            size_t open_curly_count = 2;
            do
            {
                i++;
                UNEXPECTED_EOF(input[i], i);
                if (input[i] == '{')
                    open_curly_count++;
                if (input[i] == '}')
                    open_curly_count--;
            } while (open_curly_count != 1);
            i++;
        }
        if (input[i] == ':' && input[i - 1] == '"' && input[i - 2] != '\\')
            parser->pairs_commited++;
        i++;
    } while (input[i] != '}');

parse_pair:

    json_match_char('"', input, pos);
    (*pos)--;
    char *key = json_parse_string(parser, input, pos)->string;

    json_match_char(':', input, pos);
    json_skip_whitespaces(input, pos);

    JValue *value = json_parse_value(parser, input, pos);

    json_object_add_pair(&object->object, key, value);
    json_skip_whitespaces(input, pos);

    if (input[*pos] == ',')
    {
        (*pos)++;
        goto parse_pair;
    }

    json_match_char('}', input, pos);
    return object;
}

#endif // JP_IMPLEMENTATION
