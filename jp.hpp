#ifndef JP_HPP_
#define JP_HPP_

#include <variant>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#ifdef NDEBUG
#define JP_PANIC(fmt, ...) exit(1);
#else
#define JP_PANIC(fmt, ...)                                                    \
    do                                                                        \
    {                                                                         \
        printf("[ERRO] L%d: " fmt "\n", __LINE__ __VA_OPT__(, ) __VA_ARGS__); \
        exit(1);                                                              \
    } while (0)
#endif

struct JObject;
JObject parse_json(const char *input);
using JValue = std::variant<JObject, int, bool, char *, std::nullptr_t>;

struct Memory
{
    static const unsigned long long capacity =
        INTPTR_MAX == INT32_MAX ? 4294967295ULL : 1024ULL * 1024 * 1024 * 1024;
    char *base;
    char *start;

    Memory()
    {
        base = static_cast<char *>(VirtualAlloc(nullptr, capacity, MEM_RESERVE, PAGE_READWRITE));
        if (base == nullptr)
            JP_PANIC("VirtualAlloc failed: %lu", GetLastError());
        start = base;
    }

    void *alloc(size_t size)
    {
        if (VirtualAlloc(start, size, MEM_COMMIT, PAGE_READWRITE) == nullptr)
            JP_PANIC("VirtualAlloc failed: %lu", GetLastError());
        start += size;
        return start - size;
    }

    void free()
    {
        VirtualFree(base, start - base, MEM_RELEASE);
    }
};

struct JPair
{
    char *key;
    JValue *value;
};

struct JObject
{
    JPair *pairs;
    size_t count;
    Memory pairs_mem;
    Memory kv_mem;

    JObject()
    {
        pairs = reinterpret_cast<JPair *>(pairs_mem.start);
        count = 0;
    }

    void add_pair(char *key, JValue *value)
    {
        pairs_mem.alloc(sizeof(JPair));
        pairs[count].key = key;
        pairs[count].value = value;
        count++;
    }

    JValue operator[](const char *key)
    {
        for (size_t i = 0; i < count; ++i)
        {
            if (strcmp(key, pairs[i].key) == 0)
                return *(pairs[i].value);
        }
        JP_PANIC("key \"%s\" was not found", key);
    }

    char *str(const char *key)
    {
        return std::get<char *>(this->operator[](key));
    }

    JObject obj(const char *key)
    {
        return std::get<JObject>(this->operator[](key));
    }
};

enum class TokenKind
{
    none,
    open_curly,
    close_curly,
    open_quotation,
    close_quotation,
    colon,
    comma
};

enum class State
{
    none,
    key,
    value
};

#endif // JP_HPP_

#ifdef JP_IMPLEMENTATION

JObject parse_json(const char *input)
{
    TokenKind current_token_kind = TokenKind::none;
    State state = State::none;
    char *current_key = nullptr;
    JValue *current_value = nullptr;
    JObject json;
    for (size_t pos = 0; input[pos] != '\0'; ++pos)
    {
        if (input[pos] == ' ' ||
            input[pos] == '\t' ||
            input[pos] == '\n' ||
            input[pos] == '\r')
            continue;
        switch (input[pos])
        {
        case '"':
        {
            switch (current_token_kind)
            {
            case TokenKind::comma:
            case TokenKind::open_curly:
            {
                current_token_kind = TokenKind::open_quotation;
                state = State::key;
            }
            break;
            case TokenKind::open_quotation:
            {
                if (input[pos - 1] != '\\')
                    current_token_kind = TokenKind::close_quotation;
                else
                    goto process_char;
            }
            break;
            case TokenKind::colon:
            {
                current_token_kind = TokenKind::open_quotation;
                state = State::value;
            }
            break;
            default:
                JP_PANIC();
                break;
            }
        }
        break;
        case ',':
        {
            if (current_token_kind == TokenKind::close_quotation ||
                current_token_kind == TokenKind::close_curly)
            {
                json.add_pair(current_key, current_value);
                current_token_kind = TokenKind::comma;
            }
            else
            {
                goto process_char;
            }
        }
        break;
        case ':':
        {
            if (current_token_kind == TokenKind::close_quotation)
                current_token_kind = TokenKind::colon;
            else
                goto process_char;
        }
        break;
        case '{':
        {
            switch (current_token_kind)
            {
            case TokenKind::none:
            {
                current_token_kind = TokenKind::open_curly;
            }
            break;
            case TokenKind::colon:
            {
                size_t start_pos = pos;
                size_t open_curly_count = 1;
                size_t close_curly_count = 0;
                do
                {
                    pos++;
                    if (input[pos] == '{')
                        open_curly_count++;
                    else if (input[pos] == '}')
                        close_curly_count++;
                } while (open_curly_count - close_curly_count != 0);
                size_t nested_object_string_size = pos - start_pos + 2;
                char *nested_object_string = static_cast<char *>(json.kv_mem.alloc(nested_object_string_size));
                memcpy(nested_object_string, input + start_pos, nested_object_string_size - 1);
                nested_object_string[nested_object_string_size - 1] = '\0';
                JObject nested_object = parse_json(nested_object_string);
                current_value = static_cast<JValue *>(json.kv_mem.alloc(sizeof(JValue)));
                new (current_value) JValue(nested_object);
                current_token_kind = TokenKind::close_curly;
                state = State::none;
            }
            break;
            default:
                goto process_char;
            }
        }
        break;
        case '}':
        {
            if (current_token_kind == TokenKind::close_quotation ||
                current_token_kind == TokenKind::close_curly)
            {
                json.add_pair(current_key, current_value);
                current_token_kind = TokenKind::close_curly;
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
            case TokenKind::open_quotation:
            {
            process_char:
                if (state == State::key)
                {
                    size_t start_pos = pos;
                    do
                    {
                        pos++;
                    } while (input[pos] != '"' && input[pos - 1] != '\\');
                    size_t key_size = pos - start_pos + 1;
                    current_key = static_cast<char *>(json.kv_mem.alloc(key_size));
                    memcpy(current_key, input + start_pos, key_size - 1);
                    current_key[key_size - 1] = '\0';
                    current_token_kind = TokenKind::close_quotation;
                    state = State::none;
                }
                else if (state == State::value)
                {
                    size_t start_pos = pos;
                    do
                    {
                        pos++;
                    } while (input[pos] != '"' && input[pos - 1] != '\\');
                    size_t value_size = pos - start_pos + 1;
                    char *value_string = static_cast<char *>(json.kv_mem.alloc(value_size));
                    memcpy(value_string, input + start_pos, value_size - 1);
                    value_string[value_size - 1] = '\0';
                    current_value = static_cast<JValue *>(json.kv_mem.alloc(sizeof(JValue)));
                    new (current_value) JValue(value_string);
                    current_token_kind = TokenKind::close_quotation;
                    state = State::none;
                }
            }
            break;
            default:
                JP_PANIC();
            }
        }
        break;
        }
    }
    return json;
}

#endif // JP_IMPLEMENTATION
