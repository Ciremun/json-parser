#ifndef JP_H_
#define JP_H_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if !defined(NDEBUG)
#include <stdio.h>
#endif // NDEBUG
#else
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif             // _GNU_SOURCE
#include <stdio.h> // system memory size
#include <sys/mman.h>
#if !defined(NDEBUG)
#include <errno.h>
#include <stdlib.h>
#endif // NDEBUG
#endif // _WIN32

#define JP_PANIC(...)

#include <stdint.h> // system memory size
#include <string.h> // memcpy, memcmp

// TODO(#15): JERROR macro
// TODO(#13): CI
// TODO(#14): tests
// TODO(#12): customizable allocator
// TODO(#10): error reporting
#if !defined(NDEBUG)
#define ERROR_MESSAGE_SIZE 1024
#define UNEXPECTED_EOF(pos)                                                    \
    do                                                                         \
    {                                                                          \
        JValue value;                                                          \
        value.type = JSON_ERROR;                                               \
        value.error.code = JSON_UNEXPECTED_EOF;                                \
        value.error.message =                                                  \
            (char *)json_memory_alloc(&parser->memory, ERROR_MESSAGE_SIZE);    \
        sprintf(value.error.message, "unexpected end of file at %zu", pos);    \
        return value;                                                          \
    } while (0)
#else
#define UNEXPECTED_EOF(...)                                                    \
    do                                                                         \
    {                                                                          \
        JValue value;                                                          \
        value.type = JSON_ERROR;                                               \
        value.error.code = JSON_UNEXPECTED_EOF;                                \
        value.error.message = 0;                                               \
        return value;                                                          \
    } while (0)
#endif // NDEBUG

typedef struct JPair JPair;
typedef struct JValue JValue;

typedef enum
{
    JSON_KEY_NOT_FOUND = 10,
    JSON_UNEXPECTED_EOF,
    JSON_PARSE_ERROR,
    JSON_TYPE_ERROR,
    JSON_MEMORY_ERROR
} JCode;

typedef enum
{
    JSON_OBJECT = 0,
    JSON_STRING,
    JSON_BOOL,
    JSON_NULL,
    JSON_NUMBER,
    JSON_ARRAY,
    JSON_ERROR,
} JType;

typedef struct
{
    JCode code;
    char *message;
} JError;

typedef struct
{
    char *start;
    char *base;
    size_t capacity;
#ifdef _WIN32
    size_t commited;
    size_t allocated;
    size_t commit_size;
#endif // _WIN32
} JMemory;

typedef struct
{
    JPair *pairs;
    size_t pairs_count;
#if !defined(NDEBUG)
    JMemory *memory;
#endif // NDEBUG
} JObject;

struct JValue
{
    JType type;
    union
    {
        long long number;
        int boolean;
        int null;
        char *string;
        JError error;
        JObject object;
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
    JValue value;
};

typedef struct
{
    JMemory memory;
    size_t pairs_total;
    size_t pairs_commited;
    const char *input;
} JParser;

#ifndef _WIN32
int GetPhysicallyInstalledSystemMemory(size_t *output);
#endif // _WIN32

int json_whitespace_char(char c);
int json_match_char(char c, const char *input, size_t *pos);
int json_skip_whitespaces(const char *input, size_t *pos);
void json_memory_init(JMemory *memory);
void json_memory_free(JMemory *memory);
void *json_memory_alloc(JMemory *memory, size_t size);
void json_object_init(JObject *object, JPair *pairs, JMemory *memory);
void json_object_add_pair(JObject *object, char *key, JValue value);
JParser json_init(const char *input);
JValue json_get(JObject *object, const char *key);
JValue json_parse(JParser *parser);
JValue json_parse_object(JParser *parser, size_t *pos);
JValue json_parse_value(JParser *parser, size_t *pos);
JValue json_parse_string(JParser *parser, size_t *pos);
JValue json_parse_number(JParser *parser, size_t *pos, int negative);
JValue json_parse_boolean(JParser *parser, size_t *pos, int bool_value,
                          const char *bool_string, size_t bool_string_length);
JValue json_parse_null(JParser *parser, size_t *pos);
JValue json_parse_array(JParser *parser, size_t *pos);

#endif // JP_H_

#ifdef JP_IMPLEMENTATION

#ifdef __cplusplus
JValue JValue::operator[](const char *key)
{
    if (type != JSON_OBJECT)
    {
        JValue value;
        value.type = JSON_ERROR;
        value.error.code = JSON_TYPE_ERROR;
#if !defined(NDEBUG)
        value.error.message = (char *)malloc(ERROR_MESSAGE_SIZE);
        sprintf(value.error.message, "value is not an object at '%s'", key);
#else
        value.error.message = 0;
#endif // NDEBUG
        return value;
    }
    return json_get(&object, key);
}
JValue JValue::operator[](size_t idx)
{
    if (type != JSON_ARRAY)
    {
        JValue value;
        value.type = JSON_ERROR;
        value.error.code = JSON_TYPE_ERROR;
#if !defined(NDEBUG)
        value.error.message = (char *)malloc(ERROR_MESSAGE_SIZE);
        sprintf(value.error.message, "value is not an array at [%zu]", idx);
#else
        value.error.message = 0;
#endif // NDEBUG
        return value;
    }
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
    if (meminfo == 0)
    {
        fprintf(stderr, "Error opening file '/proc/meminfo'");
        return 0;
    }

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

int json_skip_whitespaces(const char *input, size_t *pos)
{
    while (json_whitespace_char(input[*pos]))
        (*pos)++;
    if (input[*pos] == '\0')
        return 0;
    return 1;
}

int json_match_char(char c, const char *input, size_t *pos)
{
    do
    {
        if (input[*pos] == '\0')
            return JSON_UNEXPECTED_EOF;
    } while (json_whitespace_char(input[(*pos)++]));
    int match = input[*pos - 1] == c;
    if (!match)
        return JSON_PARSE_ERROR;
    return match;
}

int json_whitespace_char(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

void json_memory_init(JMemory *memory)
{
    if (memory->capacity == 0)
    {
        if (!GetPhysicallyInstalledSystemMemory(&memory->capacity))
            memory->capacity = INTPTR_MAX == INT32_MAX
                                   ? 4294967295ULL
                                   : 16ULL * 1024 * 1024 * 1024;
        else
            memory->capacity *= 1024;
    }
#ifdef _WIN32
    memory->commited = 0;
    memory->allocated = 0;
    memory->commit_size = 1024;
    memory->base = (char *)(VirtualAlloc(0, memory->capacity, MEM_RESERVE,
                                         PAGE_READWRITE));
    if (memory->base == 0)
        JP_PANIC("VirtualAlloc failed: %lu", GetLastError());
#else
    memory->base = (char *)(mmap(0, memory->capacity, PROT_READ | PROT_WRITE,
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
                         PAGE_READWRITE) == 0)
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
    if (munmap(memory->base, memory->capacity) == -1)
        JP_PANIC("munmap failed: %s", strerror(errno));
#endif // _WIN32
}

void json_object_init(JObject *object, JPair *pairs, JMemory *memory)
{
    object->pairs = pairs;
    object->pairs_count = 0;
#if !defined(NDEBUG)
    object->memory = memory;
#else
    (void)memory;
#endif // NDEBUG
}

void json_object_add_pair(JObject *object, char *key, JValue value)
{
    object->pairs[object->pairs_count].key = key;
    object->pairs[object->pairs_count].value = value;
    object->pairs_count++;
}

JValue json_get(JObject *object, const char *key)
{
    for (size_t i = 0; i < object->pairs_count; ++i)
        if (strcmp(key, object->pairs[i].key) == 0)
            return object->pairs[i].value;
    JValue value;
    value.type = JSON_ERROR;
    value.error.code = JSON_KEY_NOT_FOUND;
#if !defined(NDEBUG)
    value.error.message =
        (char *)json_memory_alloc(object->memory, ERROR_MESSAGE_SIZE);
    sprintf(value.error.message, "key \"%s\" was not found", key);
#else
    value.error.message = 0;
#endif // NDEBUG
    return value;
}

JParser json_init(const char *input)
{
    JParser parser;
    parser.memory.capacity = 0;
    json_memory_init(&parser.memory);
    parser.pairs_total = 0;
    parser.pairs_commited = 0;
    parser.input = input;
    for (size_t i = 0; input[i] != 0; ++i)
        if (input[i] == ':' && input[i - 1] == '"' && input[i - 2] != '\\')
            parser.pairs_total++;
    json_memory_alloc(&parser.memory, sizeof(JPair) * parser.pairs_total);
    return parser;
}

JValue json_parse(JParser *parser)
{
    size_t pos = 0;
    return json_parse_object(parser, &pos);
}

JValue json_parse_string(JParser *parser, size_t *pos)
{
    (*pos)++;
    size_t start = *pos;
    while (parser->input[*pos] != '"' && parser->input[*pos - 1] != '\\')
    {
        if (parser->input[*pos] == '\0')
            UNEXPECTED_EOF(*pos);
        (*pos)++;
    }
    char *value_string = 0;
    if (*pos - start != 0)
    {
        size_t string_size = *pos - start + 1;
        value_string = (char *)json_memory_alloc(&parser->memory, string_size);
        memcpy(value_string, parser->input + start, string_size - 1);
        value_string[string_size - 1] = '\0';
    }
    JValue value;
    value.type = JSON_STRING;
    value.string = value_string;
    (*pos)++;
    return value;
}

JValue json_parse_number(JParser *parser, size_t *pos, int negative)
{
    if (negative)
        (*pos)++;
    size_t start_pos = *pos;
    while (!json_whitespace_char(parser->input[*pos]) &&
           parser->input[*pos] != ',' && parser->input[*pos] != '}' &&
           parser->input[*pos] != ']')
    {
        if (parser->input[*pos] == '\0')
            UNEXPECTED_EOF(*pos);
        (*pos)++;
    }
    size_t number_string_length = *pos - start_pos;
    size_t number = 0;
    size_t i = 0;
    while (i < number_string_length)
    {
        size_t digit = (parser->input + start_pos)[i] - 48;
        number = number * 10 + digit;
        i++;
    }
    if (negative)
        number = -(long long)number;
    JValue value;
    value.type = JSON_NUMBER;
    value.number = number;
    return value;
}

JValue json_parse_boolean(JParser *parser, size_t *pos, int bool_value,
                          const char *bool_string, size_t bool_string_length)
{
    if (memcmp(parser->input + *pos, bool_string, bool_string_length) == 0)
    {
        JValue value;
        value.type = JSON_BOOL;
        value.boolean = bool_value;
        *pos += bool_string_length;
        return value;
    }
    JP_PANIC("failed to parse %s", bool_string);
}

JValue json_parse_null(JParser *parser, size_t *pos)
{
    if (memcmp(parser->input + *pos, "null", 4) == 0)
    {
        JValue value;
        value.type = JSON_NULL;
        value.null = 0;
        *pos += 4;
        return value;
    }
    JP_PANIC("failed to parse null");
}

JValue json_parse_array(JParser *parser, size_t *pos)
{
    (*pos)++;
    if (!json_skip_whitespaces(parser->input, pos))
        UNEXPECTED_EOF(*pos);
    JValue value;
    value.type = JSON_ARRAY;
    size_t start_pos = *pos;
    if (parser->input[start_pos] == ']')
    {
        value.array = 0;
        (*pos)++;
        return value;
    }
    size_t array_values_count = 1;
    int inside_string = 0;
    do
    {
        if (parser->input[start_pos] == '\0')
            UNEXPECTED_EOF(start_pos);
        if (parser->input[start_pos] == '"')
            inside_string = !inside_string;
        if (!inside_string && parser->input[start_pos] == ',')
            array_values_count++;
    } while (parser->input[++start_pos] != ']');
    JValue *array_values = (JValue *)json_memory_alloc(
        &parser->memory, sizeof(JValue) * array_values_count);
    for (size_t i = 0; i < array_values_count; ++i)
    {
        array_values[i] = json_parse_value(parser, pos);
        (*pos)++;
        if (!json_skip_whitespaces(parser->input, pos))
            UNEXPECTED_EOF(*pos);
    }
    value.array = array_values;
    return value;
}

JValue json_parse_value(JParser *parser, size_t *pos)
{
    switch (parser->input[*pos])
    {
    case '{':
        return json_parse_object(parser, pos);
    case '[':
        return json_parse_array(parser, pos);
    case '"':
        return json_parse_string(parser, pos);
    case '-':
        return json_parse_number(parser, pos, 1);
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
        return json_parse_number(parser, pos, 0);
    case 't':
        return json_parse_boolean(parser, pos, 1, "true", 4);
    case 'f':
        return json_parse_boolean(parser, pos, 0, "false", 5);
    case 'n':
        return json_parse_null(parser, pos);
    default:
        JP_PANIC("unknown char %c at %zu", parser->input[*pos], *pos);
    }
}

JValue json_parse_object(JParser *parser, size_t *pos)
{
    int match = json_match_char('{', parser->input, pos);
    if (match == JSON_UNEXPECTED_EOF)
        UNEXPECTED_EOF(*pos);
    if (match == JSON_PARSE_ERROR)
    {
        JValue value;
        value.type = JSON_ERROR;
        value.error.code = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
        value.error.message =
            (char *)json_memory_alloc(&parser->memory, ERROR_MESSAGE_SIZE);
        sprintf(value.error.message, "expected '%c' found '%c' at %zu", '{',
                parser->input[*pos - 1], *pos - 1);
#else
        value.error.message = 0;
#endif // NDEBUG
        return value;
    }

    JPair *pairs_start =
        (JPair *)(parser->memory.base + sizeof(JPair) * parser->pairs_commited);
    JValue object;
    object.type = JSON_OBJECT;
    json_object_init(&object.object, pairs_start, &parser->memory);

    size_t i = *pos;
    do
    {
        if (parser->input[i] == '\0')
            UNEXPECTED_EOF(i);
        if (parser->input[i] == '{')
        {
            size_t open_curly_count = 2;
            do
            {
                i++;
                if (parser->input[i] == '\0')
                    UNEXPECTED_EOF(i);
                if (parser->input[i] == '{')
                    open_curly_count++;
                if (parser->input[i] == '}')
                    open_curly_count--;
            } while (open_curly_count != 1);
            i++;
        }
        if (parser->input[i] == ':' && parser->input[i - 1] == '"' &&
            parser->input[i - 2] != '\\')
            parser->pairs_commited++;
        i++;
    } while (parser->input[i] != '}');

parse_pair:
{
    {
        int match = json_match_char('"', parser->input, pos);
        if (match == JSON_UNEXPECTED_EOF)
            UNEXPECTED_EOF(*pos);
        if (match == JSON_PARSE_ERROR)
        {
            JValue value;
            value.type = JSON_ERROR;
            value.error.code = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
            value.error.message =
                (char *)json_memory_alloc(&parser->memory, ERROR_MESSAGE_SIZE);
            sprintf(value.error.message, "expected '%c' found '%c' at %zu", '"',
                    parser->input[*pos - 1], *pos - 1);
#else
            value.error.message = 0;
#endif // NDEBUG
            return value;
        }
    }

    (*pos)--;
    char *key = json_parse_string(parser, pos).string;

    {
        int match = json_match_char(':', parser->input, pos);
        if (match == JSON_UNEXPECTED_EOF)
            UNEXPECTED_EOF(*pos);
        if (match == JSON_PARSE_ERROR)
        {
            JValue value;
            value.type = JSON_ERROR;
            value.error.code = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
            value.error.message =
                (char *)json_memory_alloc(&parser->memory, ERROR_MESSAGE_SIZE);
            sprintf(value.error.message, "expected '%c' found '%c' at %zu", ':',
                    parser->input[*pos - 1], *pos - 1);
#else
            value.error.message = 0;
#endif // NDEBUG
            return value;
        }
    }
    if (!json_skip_whitespaces(parser->input, pos))
        UNEXPECTED_EOF(*pos);

    JValue value = json_parse_value(parser, pos);

    json_object_add_pair(&object.object, key, value);
    if (!json_skip_whitespaces(parser->input, pos))
        UNEXPECTED_EOF(*pos);

    if (parser->input[*pos] == ',')
    {
        (*pos)++;
        goto parse_pair;
    }
}
    if (json_match_char('}', parser->input, pos) == JSON_UNEXPECTED_EOF)
        UNEXPECTED_EOF(*pos);
    return object;
}

#endif // JP_IMPLEMENTATION
