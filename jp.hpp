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
#else
#include <sys/mman.h>
#include <errno.h>
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
struct JsonParser;
using JValue = std::variant<JObject, int, bool, char *, std::nullptr_t>;

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

static size_t system_memory_size = 0;

struct Memory
{
    char *start;
    char *base;
#ifdef _WIN32
    size_t commited;
    size_t allocated;
    size_t commit_size;
#endif // _WIN32

    Memory()
#ifdef _WIN32
        : commited(0), allocated(0), commit_size(1024)
#endif // _WIN32
    {
        if (system_memory_size == 0)
        {
            if (!GetPhysicallyInstalledSystemMemory(&system_memory_size))
                system_memory_size = INTPTR_MAX == INT32_MAX ? 4294967295ULL : 16ULL * 1024 * 1024 * 1024;
            else
                system_memory_size *= 1024;
        }
#ifdef _WIN32
        base = static_cast<char *>(VirtualAlloc(nullptr, system_memory_size, MEM_RESERVE, PAGE_READWRITE));
        if (base == nullptr)
            JP_PANIC("VirtualAlloc failed: %lu", GetLastError());
#else
        base = static_cast<char *>(mmap(0, system_memory_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0));
        if (base == MAP_FAILED)
            JP_PANIC("mmap failed: %s", strerror(errno));
#endif // _WIN32
        start = base;
    }

    void *alloc(size_t size)
    {
#ifdef _WIN32
        allocated += size;
        if (allocated * 2 > commited)
        {
            commit_size += size * 2;
            if (VirtualAlloc(start, commit_size, MEM_COMMIT, PAGE_READWRITE) == nullptr)
                JP_PANIC("VirtualAlloc failed: %lu", GetLastError());
            commited += commit_size;
        }
#endif // _WIN32
        start += size;
        return start - size;
    }

    void free()
    {
#ifdef _WIN32
        if (VirtualFree(base, 0, MEM_RELEASE) == 0)
            JP_PANIC("VirtualFree failed: %lu", GetLastError());
#else
        if (munmap(base, system_memory_size) == -1)
            JP_PANIC("munmap failed: %s", strerror(errno));
#endif // _WIN32
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
    size_t pairs_count;

    JObject(JPair *pairs) : pairs(pairs), pairs_count(0) {}

    void add_pair(char *key, JValue *value)
    {
        pairs[pairs_count].key = key;
        pairs[pairs_count].value = value;
        pairs_count++;
    }

    JObject &operator[](const char *key)
    {
        for (size_t i = 0; i < pairs_count; ++i)
        {
            if (strcmp(key, pairs[i].key) == 0)
                return reinterpret_cast<JObject &>(*(pairs[i].value));
        }
        JP_PANIC("key \"%s\" was not found", key);
    }

    char *str(const char *key)
    {
        return reinterpret_cast<char *&>(this->operator[](key));
    }

    JObject &obj(const char *key)
    {
        return this->operator[](key);
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

struct JsonParser
{
    Memory json_memory;
    const char *input;
    size_t pairs_total;
    size_t pairs_commited;

    JsonParser(const char *input) : input(input), pairs_total(0), pairs_commited(0)
    {
        for (size_t i = 0; input[i] != 0; ++i)
            if (input[i] == ':' && input[i - 1] == '"' && input[i - 2] != '\\')
                pairs_total++;
        json_memory.alloc(sizeof(JPair) * pairs_total);
    }

    JObject parse()
    {
        return parse_object(input);
    }

    void free()
    {
        json_memory.free();
    }

    JObject parse_object(const char *input)
    {
        TokenKind current_token_kind = TokenKind::none;
        State state = State::none;

        char *current_key = nullptr;
        JValue *current_value = nullptr;

        size_t pairs_offset = sizeof(JPair) * pairs_commited;
        JPair *pairs_start = reinterpret_cast<JPair *>(json_memory.base + pairs_offset);
        JObject json(pairs_start);

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
                    case TokenKind::colon:
                    {
                        size_t start_pos = pos;
                        do
                        {
                            pos++;
                            if (input[pos] == '\0')
                                JP_PANIC("unexpected end of file at %zu", pos);
                        } while (pos - start_pos != 4);
                        char *bool_value = static_cast<char *>(json_memory.alloc(5));
                        memcpy(bool_value, input + start_pos, 4);
                        bool_value[4] = '\0';
                        if (strcmp(bool_value, "true") == 0)
                        {
                            current_value = static_cast<JValue *>(json_memory.alloc(sizeof(JValue)));
                            new (current_value) JValue(true);
                            current_token_kind = TokenKind::close_quotation;
                            state = State::none;
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
                            pairs_commited++;
                    } while (input[i] != '}');
                    current_token_kind = TokenKind::open_curly;
                }
                break;
                case TokenKind::colon:
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
                    char *nested_object_string = static_cast<char *>(json_memory.alloc(nested_object_string_size));
                    memcpy(nested_object_string, input + start_pos, nested_object_string_size - 1);
                    nested_object_string[nested_object_string_size - 1] = '\0';
                    JObject nested_object = parse_object(nested_object_string);
                    current_value = static_cast<JValue *>(json_memory.alloc(sizeof(JValue)));
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
                            if (input[pos] == '\0')
                                JP_PANIC("unexpected end of file at %zu", pos);
                        } while (input[pos] != '"' && input[pos - 1] != '\\');
                        size_t key_size = pos - start_pos + 1;
                        current_key = static_cast<char *>(json_memory.alloc(key_size));
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
                            if (input[pos] == '\0')
                                JP_PANIC("unexpected end of file at %zu", pos);
                        } while (input[pos] != '"' && input[pos - 1] != '\\');
                        size_t value_size = pos - start_pos + 1;
                        char *value_string = static_cast<char *>(json_memory.alloc(value_size));
                        memcpy(value_string, input + start_pos, value_size - 1);
                        value_string[value_size - 1] = '\0';
                        current_value = static_cast<JValue *>(json_memory.alloc(sizeof(JValue)));
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
};

#endif // JP_IMPLEMENTATION
