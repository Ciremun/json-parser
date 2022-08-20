#ifndef JP_H_
#define JP_H_

#if !defined(NDEBUG)
#include <stdio.h>
#include <stdlib.h>
#endif // NDEBUG

// TODO(#21): utf8
// TODO(#22): hex
// TODO(#23): escapes
// TODO(#17): examples
// TODO(#14): tests
// TODO(Ciremun): move input pos to parser
// TODO(Ciremun): easy api that doesn't require passing the memory struct
typedef struct JPair JPair;
typedef struct JValue JValue;
typedef unsigned long long int jsize_t;

typedef enum
{
    JSON_KEY_NOT_FOUND = 2,
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
    char *base;
    void *(*alloc)(unsigned long long int size);
} JMemory;

typedef struct
{
    JPair *data;
    jsize_t length;
} JObject;

typedef struct
{
    char *data;
    jsize_t length;
} JString;

typedef struct
{
    JValue *data;
    jsize_t length;
} JArray;

struct JValue
{
    JType type;
    union
    {
        long long number;
        int boolean;
        int null;
        JString string;
        JCode error;
        JObject object;
        JArray array;
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
    JMemory *memory;
    jsize_t pairs_commited;
    const char *input;
} JParser;

int json_whitespace_char(char c);
int json_match_char(char c, const char *input, jsize_t *pos);
int json_skip_whitespaces(const char *input, jsize_t *pos);
int json_memcmp(const void *str1, const void *str2, jsize_t count);
int json_strcmp(const char *p1, const char *p2);
void *json_memcpy(void *dst, void const *src, jsize_t size);
JParser json_init(JMemory *memory, const char *input);
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
JValue json_unexpected_eof(jsize_t pos);

#endif // JP_H_

#ifdef JP_IMPLEMENTATION

#ifdef __cplusplus
JValue JValue::operator[](const char *key)
{
    if (type != JSON_OBJECT)
    {
        JValue value;
        value.type = JSON_ERROR;
        value.error = JSON_TYPE_ERROR;
#if !defined(NDEBUG)
        fprintf(stderr, "value is not an object at '%s'\n", key);
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
        value.error = JSON_TYPE_ERROR;
#if !defined(NDEBUG)
        fprintf(stderr, "value is not an array at [%llu]\n", idx);
#endif // NDEBUG
        return value;
    }
    return array.data[idx];
}
JValue JValue::operator[](int idx)
{
    return operator[](static_cast<jsize_t>(idx));
}
#endif // __cplusplus

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
    if (input[*pos - 1] != c)
        return JSON_PARSE_ERROR;
    return 1;
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

void *json_memcpy(void *dst, void const *src, jsize_t size)
{
    unsigned char *source = (unsigned char *)src;
    unsigned char *dest = (unsigned char *)dst;
    while (size--)
        *dest++ = *source++;
    return dst;
}

JParser json_init(JMemory *memory, const char *input)
{
    JParser parser;
    parser.memory = memory;
    parser.pairs_commited = 0;
    parser.input = input;
    jsize_t pairs_total = 0;
    for (jsize_t i = 0; input[i] != 0; ++i)
        if (input[i] == ':' && input[i - 1] == '"' && input[i - 2] != '\\')
            pairs_total++;
    parser.memory->base = (char *)parser.memory->alloc(sizeof(JPair) * pairs_total);
    return parser;
}

JValue json_get(JObject *object, const char *key)
{
    for (jsize_t i = 0; i < object->length; ++i)
        if (json_strcmp(key, object->data[i].key) == 0)
            return object->data[i].value;
    JValue value;
    value.type = JSON_ERROR;
    value.error = JSON_KEY_NOT_FOUND;
#if !defined(NDEBUG)
    fprintf(stderr, "key \"%s\" was not found\n", key);
#endif // NDEBUG
    return value;
}

JValue json_parse(JParser *parser)
{
    jsize_t pos = 0;
    json_skip_whitespaces(parser->input, &pos);
    return json_parse_value(parser, &pos);
}

JValue json_parse_string(JParser *parser, jsize_t *pos)
{
    (*pos)++;
    jsize_t start = *pos;
    while (parser->input[*pos] != '"' && parser->input[*pos - 1] != '\\')
    {
        if (parser->input[*pos] == '\0')
            return json_unexpected_eof(*pos);
        (*pos)++;
    }
    char *value_string = 0;
    jsize_t string_size = 1;
    if (*pos - start != 0)
    {
        string_size = *pos - start + 1;
        value_string = (char *)parser->memory->alloc(string_size);
        if (value_string == 0)
        {
            JValue value;
            value.type = JSON_ERROR;
            value.error = JSON_MEMORY_ERROR;
            return value;
        }
        json_memcpy(value_string, parser->input + start, string_size - 1);
        value_string[string_size - 1] = '\0';
    }
    JValue value;
    value.type = JSON_STRING;
    value.string.data = value_string;
    value.string.length = string_size - 1;
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
           parser->input[*pos] != ']' && parser->input[*pos] != '\0')
    {
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
            value.error = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
            fprintf(stderr, "couldn't parse a number at %llu\n",
                    start_pos + i + 1);
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
    value.error = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
    fprintf(stderr, "failed to parse %s\n", bool_string);
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
    value.error = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
    fprintf(stderr, "failed to parse null\n");
#endif // NDEBUG
    return value;
}

JValue json_parse_array(JParser *parser, jsize_t *pos)
{
    (*pos)++;
    if (!json_skip_whitespaces(parser->input, pos))
        return json_unexpected_eof(*pos);
    JValue value;
    value.type = JSON_ARRAY;
    jsize_t start_pos = *pos;
    if (parser->input[start_pos] == ']')
    {
        value.array.data = 0;
        value.array.length = 0;
        (*pos)++;
        return value;
    }
    jsize_t array_values_count = 1;
    jsize_t open_bracket_count = 1;
    int inside_string = 0;
    do
    {
        if (parser->input[start_pos] == '\0')
            return json_unexpected_eof(start_pos);
        if (parser->input[start_pos] == '[')
            open_bracket_count++;
        if (parser->input[start_pos] == ']')
            open_bracket_count--;
        if (parser->input[start_pos] == '"')
            inside_string = !inside_string;
        if (!inside_string && parser->input[start_pos] == ',')
            array_values_count++;
        start_pos++;
    } while (open_bracket_count != 0);
    JValue *array_values = (JValue *)parser->memory->alloc(sizeof(JValue) * array_values_count);
    if (array_values == 0)
    {
        JValue value;
        value.type = JSON_ERROR;
        value.error = JSON_MEMORY_ERROR;
        return value;
    }
    for (jsize_t i = 0; i < array_values_count; ++i)
    {
        if (!json_skip_whitespaces(parser->input, pos))
            return json_unexpected_eof(*pos);
        JValue value = json_parse_value(parser, pos);
        if (value.type == JSON_ERROR)
            return value;
        array_values[i] = value;
        if (!json_skip_whitespaces(parser->input, pos))
            return json_unexpected_eof(*pos);
        (*pos)++;
    }
    value.array.data = array_values;
    value.array.length = array_values_count;
    return value;
}

JValue json_unexpected_eof(jsize_t pos)
{
    JValue value;
    value.type = JSON_ERROR;
    value.error = JSON_UNEXPECTED_EOF;
#if !defined(NDEBUG)
    fprintf(stderr, "unexpected end of file at %llu\n", pos);
#endif // NDEBUG
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
        value.error = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
        fprintf(stderr, "unknown char %c at %llu\n", parser->input[*pos], *pos);
#endif // NDEBUG
        return value;
    }
    }
}

JValue json_parse_object(JParser *parser, jsize_t *pos)
{
    int match = json_match_char('{', parser->input, pos);
    if (match == JSON_UNEXPECTED_EOF)
        return json_unexpected_eof(*pos);
    if (match == JSON_PARSE_ERROR)
    {
        JValue value;
        value.type = JSON_ERROR;
        value.error = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
        fprintf(stderr, "expected '%c' found '%c' at %llu\n", '{',
                parser->input[*pos - 1], *pos - 1);
#endif // NDEBUG
        return value;
    }

    JPair *pairs_start = (JPair *)(parser->memory->base +
                                   sizeof(JPair) * parser->pairs_commited);
    JValue value;
    value.type = JSON_OBJECT;
    value.object.data = pairs_start;
    value.object.length = 0;

    jsize_t i = *pos;
    do
    {
        if (parser->input[i] == '\0')
            return json_unexpected_eof(i);
        if (parser->input[i] == '{')
        {
            jsize_t open_curly_count = 1;
            do
            {
                i++;
                if (parser->input[i] == '\0')
                    return json_unexpected_eof(i);
                if (parser->input[i] == '{')
                    open_curly_count++;
                if (parser->input[i] == '}')
                    open_curly_count--;
            } while (open_curly_count != 0);
            i++;
        }
        if (parser->input[i] == ':' && parser->input[i - 1] == '"' &&
            parser->input[i - 2] != '\\')
            parser->pairs_commited++;
    } while (parser->input[i++] != '}');

parse_pair:
{
    {
        int match = json_match_char('"', parser->input, pos);
        if (match == JSON_UNEXPECTED_EOF)
            return json_unexpected_eof(*pos);
        if (match == JSON_PARSE_ERROR)
        {
            if (parser->input[*pos - 1] == '}')
            {
                value.object.data = 0;
                return value;
            }
            JValue value;
            value.type = JSON_ERROR;
            value.error = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
            fprintf(stderr, "expected '%c' found '%c' at %llu\n", '"',
                    parser->input[*pos - 1], *pos - 1);
#endif // NDEBUG
            return value;
        }
    }

    (*pos)--;

    JValue key_value = json_parse_string(parser, pos);
    if (key_value.type == JSON_ERROR)
        return key_value;

    char *key = key_value.string.data;

    {
        int match = json_match_char(':', parser->input, pos);
        if (match == JSON_UNEXPECTED_EOF)
            return json_unexpected_eof(*pos);
        if (match == JSON_PARSE_ERROR)
        {
            JValue error_value;
            error_value.type = JSON_ERROR;
            error_value.error = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
            fprintf(stderr, "expected '%c' found '%c' at %llu\n", ':',
                    parser->input[*pos - 1], *pos - 1);
#endif // NDEBUG
            return error_value;
        }
    }
    if (!json_skip_whitespaces(parser->input, pos))
        return json_unexpected_eof(*pos);

    JValue object_value = json_parse_value(parser, pos);
    if (object_value.type == JSON_ERROR)
        return object_value;

    value.object.data[value.object.length].key = key;
    value.object.data[value.object.length].value = object_value;
    value.object.length++;

    if (!json_skip_whitespaces(parser->input, pos))
        return json_unexpected_eof(*pos);

    if (parser->input[*pos] == ',')
    {
        (*pos)++;
        goto parse_pair;
    }
}
    {
        int match = json_match_char('}', parser->input, pos);
        if (match == JSON_UNEXPECTED_EOF)
            return json_unexpected_eof(*pos);
        if (match == JSON_PARSE_ERROR)
        {
            JValue error_value;
            error_value.type = JSON_ERROR;
            error_value.error = JSON_PARSE_ERROR;
#if !defined(NDEBUG)
            fprintf(stderr, "expected '%c' found '%c' at %llu\n", '}',
                    parser->input[*pos - 1], *pos - 1);
#endif // NDEBUG
            return error_value;
        }
    }
    return value;
}

#endif // JP_IMPLEMENTATION
