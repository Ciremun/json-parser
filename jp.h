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
#include <string.h>
#endif // NDEBUG
#endif // _WIN32

#if !defined(NDEBUG)
#include <stdlib.h>
#endif // NDEBUG

// TODO(#17): docs
// TODO(#16): map and parse /proc/meminfo file
// TODO(#13): CI
// TODO(#14): tests
// TODO(#12): customizable allocator
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
        sprintf(value.error.message, "unexpected end of file at %llu", pos);   \
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
typedef unsigned long long int jsize_t;

typedef enum
{
    JSON_OK = 10,
    JSON_KEY_NOT_FOUND,
    JSON_UNEXPECTED_EOF,
    JSON_PARSE_ERROR,
    JSON_TYPE_ERROR,
    JSON_MEMORY_ERROR,
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
    jsize_t capacity;
#ifdef _WIN32
    jsize_t commited;
    jsize_t allocated;
    jsize_t commit_size;
#endif // _WIN32
} JMemory;

typedef struct
{
    JPair *pairs;
    jsize_t pairs_count;
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
    JValue operator[](jsize_t idx);
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
    JError error;
    jsize_t pairs_total;
    jsize_t pairs_commited;
    const char *input;
} JParser;

#ifndef _WIN32
int GetPhysicallyInstalledSystemMemory(jsize_t *output);
#endif // _WIN32

int json_whitespace_char(char c);
int json_match_char(char c, const char *input, jsize_t *pos);
int json_skip_whitespaces(const char *input, jsize_t *pos);
int json_memcmp(const void *str1, const void *str2, jsize_t count);
int json_strcmp(const char *p1, const char *p2);
int json_memory_init(JMemory *memory);
int json_memory_free(JMemory *memory);
void json_object_init(JObject *object, JPair *pairs, JMemory *memory);
void json_object_add_pair(JObject *object, char *key, JValue value);
void *json_memcpy(void *dst, void const *src, jsize_t size);
void *json_memory_alloc(JMemory *memory, jsize_t size);
JParser json_init(const char *input);
JValue json_get(JObject *object, const char *key);
JValue json_parse(JParser *parser);
JValue json_parse_object(JParser *parser, jsize_t *pos);
JValue json_parse_value(JParser *parser, jsize_t *pos);
JValue json_parse_string(JParser *parser, jsize_t *pos);
JValue json_parse_number(JParser *parser, jsize_t *pos, int negative);
JValue json_parse_boolean(JParser *parser, jsize_t *pos, int bool_value,
                          const char *bool_string, jsize_t bool_string_length);
JValue json_parse_null(JParser *parser, jsize_t *pos);
JValue json_parse_array(JParser *parser, jsize_t *pos);

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
JValue JValue::operator[](jsize_t idx)
{
    if (type != JSON_ARRAY)
    {
        JValue value;
        value.type = JSON_ERROR;
        value.error.code = JSON_TYPE_ERROR;
#if !defined(NDEBUG)
        value.error.message = (char *)malloc(ERROR_MESSAGE_SIZE);
        sprintf(value.error.message, "value is not an array at [%llu]", idx);
#else
        value.error.message = 0;
#endif // NDEBUG
        return value;
    }
    return array[idx];
}
JValue JValue::operator[](int idx)
{
    return operator[](static_cast<jsize_t>(idx));
}
#endif // __cplusplus

#ifndef _WIN32
int GetPhysicallyInstalledSystemMemory(jsize_t *output)
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
        if (sscanf(line, "MemTotal: %llu kB", output) == 1)
        {
            fclose(meminfo);
            return 1;
        }
    }

    fclose(meminfo);
    return 0;
}
#endif // _WIN32

int json_whitespace_char(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int json_match_char(char c, const char *input, jsize_t *pos)
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

int json_skip_whitespaces(const char *input, jsize_t *pos)
{
    while (json_whitespace_char(input[*pos]))
        (*pos)++;
    if (input[*pos] == '\0')
        return 0;
    return 1;
}

int json_memcmp(const void *str1, const void *str2, jsize_t count)
{
    const unsigned char *s1 = (const unsigned char *)str1;
    const unsigned char *s2 = (const unsigned char *)str2;
    while (count-- > 0)
    {
        if (*s1++ != *s2++)
            return s1[-1] < s2[-1] ? -1 : 1;
    }
    return 0;
}

int json_strcmp(const char *p1, const char *p2)
{
    const unsigned char *s1 = (const unsigned char *)p1;
    const unsigned char *s2 = (const unsigned char *)p2;
    unsigned char c1, c2;
    do
    {
        c1 = (unsigned char)*s1++;
        c2 = (unsigned char)*s2++;
        if (c1 == 0)
            return c1 - c2;
    } while (c1 == c2);
    return c1 - c2;
}

int json_memory_init(JMemory *memory)
{
    if (memory->capacity == 0)
    {
        if (!GetPhysicallyInstalledSystemMemory(&memory->capacity))
            memory->capacity = (jsize_t)4294967295;
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
        return 0;
#else
    memory->base = (char *)(mmap(0, memory->capacity, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
    if (memory->base == MAP_FAILED)
        return 0;
#endif // _WIN32
    memory->start = memory->base;
    return 1;
}

void *json_memcpy(void *dst, void const *src, jsize_t size)
{
    unsigned char *source = (unsigned char *)src;
    unsigned char *dest = (unsigned char *)dst;
    while (size--)
        *dest++ = *source++;
    return dst;
}

void *json_memory_alloc(JMemory *memory, jsize_t size)
{
#ifdef _WIN32
    memory->allocated += size;
    if (memory->allocated * 2 > memory->commited)
    {
        memory->commit_size += size * 2;
        if (VirtualAlloc(memory->start, memory->commit_size, MEM_COMMIT,
                         PAGE_READWRITE) == 0)
            return 0;
        memory->commited += memory->commit_size;
    }
#endif // _WIN32
    memory->start += size;
    return memory->start - size;
}

int json_memory_free(JMemory *memory)
{
#ifdef _WIN32
    if (VirtualFree(memory->base, 0, MEM_RELEASE) == 0)
        return 0;
#else
    if (munmap(memory->base, memory->capacity) == -1)
        return 0;
#endif // _WIN32
    return 1;
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

JParser json_init(const char *input)
{
    JParser parser;
    parser.error.code = JSON_OK;
    parser.memory.capacity = 0;
    if (!json_memory_init(&parser.memory))
    {
        parser.error.code = JSON_MEMORY_ERROR;
#if !defined(NDEBUG)
        parser.error.message = (char *)malloc(ERROR_MESSAGE_SIZE);
#ifdef _WIN32
        sprintf(parser.error.message, "VirtualAlloc failed: %lu",
                GetLastError());
#else
        sprintf(parser.error.message, "mmap failed: %s", strerror(errno));
#endif // _WIN32
#endif // NDEBUG
        return parser;
    }
    parser.pairs_total = 0;
    parser.pairs_commited = 0;
    parser.input = input;
    for (jsize_t i = 0; input[i] != 0; ++i)
        if (input[i] == ':' && input[i - 1] == '"' && input[i - 2] != '\\')
            parser.pairs_total++;
#ifdef _WIN32
    if (json_memory_alloc(&parser.memory, sizeof(JPair) * parser.pairs_total) ==
        0)
    {
        parser.error.code = JSON_MEMORY_ERROR;
#if !defined(NDEBUG)
        parser.error.message = (char *)malloc(ERROR_MESSAGE_SIZE);
        sprintf(parser.error.message, "VirtualAlloc failed: %lu",
                GetLastError());
#endif // NDEBUG
    }
#else
    json_memory_alloc(&parser.memory, sizeof(JPair) * parser.pairs_total);
#endif // _WIN32
    return parser;
}

JValue json_get(JObject *object, const char *key)
{
    for (jsize_t i = 0; i < object->pairs_count; ++i)
        if (json_strcmp(key, object->pairs[i].key) == 0)
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

JValue json_parse(JParser *parser)
{
    jsize_t pos = 0;
    return json_parse_object(parser, &pos);
}

JValue json_parse_string(JParser *parser, jsize_t *pos)
{
    (*pos)++;
    jsize_t start = *pos;
    while (parser->input[*pos] != '"' && parser->input[*pos - 1] != '\\')
    {
        if (parser->input[*pos] == '\0')
            UNEXPECTED_EOF(*pos);
        (*pos)++;
    }
    char *value_string = 0;
    if (*pos - start != 0)
    {
        jsize_t string_size = *pos - start + 1;
        value_string = (char *)json_memory_alloc(&parser->memory, string_size);
#ifdef _WIN32
        if (value_string == 0)
        {
            JValue value;
            value.type = JSON_ERROR;
            value.error.code = JSON_MEMORY_ERROR;
#if !defined(NDEBUG)
            value.error.message = (char *)malloc(ERROR_MESSAGE_SIZE);
            sprintf(value.error.message, "VirtualAlloc failed: %lu",
                    GetLastError());
#else
            value.error.message = 0;
#endif // NDEBUG
            return value;
        }
#endif // _WIN32
        json_memcpy(value_string, parser->input + start, string_size - 1);
        value_string[string_size - 1] = '\0';
    }
    JValue value;
    value.type = JSON_STRING;
    value.string = value_string;
    (*pos)++;
    return value;
}

JValue json_parse_number(JParser *parser, jsize_t *pos, int negative)
{
    if (negative)
        (*pos)++;
    jsize_t start_pos = *pos;
    while (!json_whitespace_char(parser->input[*pos]) &&
           parser->input[*pos] != ',' && parser->input[*pos] != '}' &&
           parser->input[*pos] != ']')
    {
        if (parser->input[*pos] == '\0')
            UNEXPECTED_EOF(*pos);
        (*pos)++;
    }
    jsize_t number_string_length = *pos - start_pos;
    jsize_t number = 0;
    jsize_t i = 0;
    while (i < number_string_length)
    {
        jsize_t digit = (parser->input + start_pos)[i] - 48;
        if (digit > 9)
        {
            JValue value;
            value.type = JSON_ERROR;
            value.error.code = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
            value.error.message =
                (char *)json_memory_alloc(&parser->memory, ERROR_MESSAGE_SIZE);
            sprintf(value.error.message, "couldn't parse a number at %llu",
                    start_pos + i + 1);
#else
            value.error.message = 0;
#endif // NDEBUG
            return value;
        }
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

JValue json_parse_boolean(JParser *parser, jsize_t *pos, int bool_value,
                          const char *bool_string, jsize_t bool_string_length)
{
    if (json_memcmp(parser->input + *pos, bool_string, bool_string_length) == 0)
    {
        JValue value;
        value.type = JSON_BOOL;
        value.boolean = bool_value;
        *pos += bool_string_length;
        return value;
    }
    JValue value;
    value.type = JSON_ERROR;
    value.error.code = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
    value.error.message =
        (char *)json_memory_alloc(&parser->memory, ERROR_MESSAGE_SIZE);
    sprintf(value.error.message, "failed to parse %s", bool_string);
#else
    value.error.message = 0;
#endif // NDEBUG
    return value;
}

JValue json_parse_null(JParser *parser, jsize_t *pos)
{
    if (json_memcmp(parser->input + *pos, "null", 4) == 0)
    {
        JValue value;
        value.type = JSON_NULL;
        value.null = 0;
        *pos += 4;
        return value;
    }
    JValue value;
    value.type = JSON_ERROR;
    value.error.code = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
    value.error.message =
        (char *)json_memory_alloc(&parser->memory, ERROR_MESSAGE_SIZE);
    memcpy(value.error.message, "failed to parse null", 21);
#else
    value.error.message = 0;
#endif // NDEBUG
    return value;
}

JValue json_parse_array(JParser *parser, jsize_t *pos)
{
    (*pos)++;
    if (!json_skip_whitespaces(parser->input, pos))
        UNEXPECTED_EOF(*pos);
    JValue value;
    value.type = JSON_ARRAY;
    jsize_t start_pos = *pos;
    if (parser->input[start_pos] == ']')
    {
        value.array = 0;
        (*pos)++;
        return value;
    }
    jsize_t array_values_count = 1;
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
#ifdef _WIN32
    if (array_values == 0)
    {
        JValue value;
        value.type = JSON_ERROR;
        value.error.code = JSON_MEMORY_ERROR;
#if !defined(NDEBUG)
        value.error.message = (char *)malloc(ERROR_MESSAGE_SIZE);
        sprintf(value.error.message, "VirtualAlloc failed: %lu",
                GetLastError());
#else
        value.error.message = 0;
#endif // NDEBUG
        return value;
    }
#endif // _WIN32
    for (jsize_t i = 0; i < array_values_count; ++i)
    {
        array_values[i] = json_parse_value(parser, pos);
        (*pos)++;
        if (!json_skip_whitespaces(parser->input, pos))
            UNEXPECTED_EOF(*pos);
    }
    value.array = array_values;
    return value;
}

JValue json_parse_value(JParser *parser, jsize_t *pos)
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
    {
        JValue value;
        value.type = JSON_ERROR;
        value.error.code = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
        value.error.message =
            (char *)json_memory_alloc(&parser->memory, ERROR_MESSAGE_SIZE);
        sprintf(value.error.message, "unknown char %c at %llu",
                parser->input[*pos], *pos);
#else
        value.error.message = 0;
#endif // NDEBUG
        return value;
    }
    }
}

JValue json_parse_object(JParser *parser, jsize_t *pos)
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
        sprintf(value.error.message, "expected '%c' found '%c' at %llu", '{',
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

    jsize_t i = *pos;
    do
    {
        if (parser->input[i] == '\0')
            UNEXPECTED_EOF(i);
        if (parser->input[i] == '{')
        {
            jsize_t open_curly_count = 2;
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
            sprintf(value.error.message, "expected '%c' found '%c' at %llu",
                    '"', parser->input[*pos - 1], *pos - 1);
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
            sprintf(value.error.message, "expected '%c' found '%c' at %llu",
                    ':', parser->input[*pos - 1], *pos - 1);
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
