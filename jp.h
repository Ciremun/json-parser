#ifndef JP_H_
#define JP_H_

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#define _GNU_SOURCE
#include <sys/mman.h>
#include <errno.h>
#endif

#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef NDEBUG
#define JP_PANIC(fmt, ...) exit(1);
#else
#include <stdio.h>
#define JP_PANIC(fmt, ...)                                                    \
    do                                                                        \
    {                                                                         \
        printf("[ERRO] L%d: " fmt "\n", __LINE__ __VA_OPT__(, ) __VA_ARGS__); \
        exit(1);                                                              \
    } while (0)
#endif

static size_t system_memory_size = 0;

typedef struct JPair JPair;

typedef enum
{
    JSON_OBJECT = 0,
    JSON_STRING,
    JSON_BOOL,
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
} TokenKind;

typedef enum
{
    STATE_NONE = 0,
    STATE_KEY,
    STATE_VALUE
} State;

typedef struct
{
    JPair *pairs;
    size_t pairs_count;
} JObject;

typedef struct
{
    JType type;
    union
    {
        JObject object;
        int number;
        int boolean;
        char *string;
        int null;
    };
} JValue;

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
} Memory;

typedef struct
{
    Memory memory;
    size_t pairs_total;
    size_t pairs_commited;
} JParser;

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

void json_memory_init(Memory *memory)
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

void *json_memory_alloc(Memory *memory, size_t size)
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

void json_memory_free(Memory *memory)
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
    {
        if (strcmp(key, jobject->pairs[i].key) == 0)
            return *(jobject->pairs[i].value);
    }
    JP_PANIC("key \"%s\" was not found", key);
}

#endif // JP_H_

#ifdef JP_IMPLEMENTATION

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

JObject json_parse(JParser *jparser, const char *input)
{
    TokenKind current_token_kind = TOKEN_KIND_NONE;
    State state = STATE_NONE;

    char *current_key = NULL;
    JValue *current_value = NULL;

    size_t pairs_offset = sizeof(JPair) * jparser->pairs_commited;
    JPair *pairs_start = (JPair *)(jparser->memory.base + pairs_offset);
    JObject json;
    json_object_init(&json, pairs_start);

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
                char *bool_value = (char *)json_memory_alloc(&jparser->memory, 5);
                memcpy(bool_value, input + start_pos, 4);
                bool_value[4] = '\0';
                if (strcmp(bool_value, "true") == 0)
                {
                    current_value = (JValue *)json_memory_alloc(&jparser->memory, sizeof(JValue));
                    current_value->type = JSON_BOOL;
                    current_value->boolean = 1;
                    current_token_kind = TOKEN_KIND_CLOSE_QUOTATION;
                    state = STATE_NONE;
                    pos--;
                    break;
                }
                JP_PANIC("failed to parse 'true'");
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
                json_object_add_pair(&json, current_key, current_value);
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
                                JP_PANIC("unexpected end of file at %zu", i);
                            else if (input[i] == '{')
                                open_curly_count++;
                            else if (input[i] == '}')
                                open_curly_count--;
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
                        JP_PANIC("unexpected end of file at %zu", pos);
                    else if (input[pos] == '{')
                        open_curly_count++;
                    else if (input[pos] == '}')
                        open_curly_count--;
                } while (open_curly_count != 0);
                size_t nested_object_string_size = pos - start_pos + 2;
                char *nested_object_string = (char *)json_memory_alloc(&jparser->memory, nested_object_string_size);
                memcpy(nested_object_string, input + start_pos, nested_object_string_size - 1);
                nested_object_string[nested_object_string_size - 1] = '\0';
                JObject nested_object = json_parse(jparser, nested_object_string);
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
                json_object_add_pair(&json, current_key, current_value);
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

#endif // JP_IMPLEMENTATION