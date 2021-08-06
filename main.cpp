#include <unordered_map>
#include <variant>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef NDEBUG
#define JP_PANIC(fmt, ...) \
    do                     \
    {                      \
        exit(1);           \
    } while (0)
#else
#define JP_PANIC(fmt, ...)                                                    \
    do                                                                        \
    {                                                                         \
        printf("[ERRO] L%d: " fmt "\n", __LINE__ __VA_OPT__(, ) __VA_ARGS__); \
        exit(1);                                                              \
    } while (0)
#endif

#define JSON_INPUT "{\"test\": \"Hello, World!\"}"

struct JObject;

using JValue = std::variant<JObject, int, bool, char *, std::nullptr_t>;

struct JPair
{
    char *key;
    JValue *value;
};

struct JObject
{
    JPair *pairs;
    size_t count;
};

enum class TokenKind
{
    open_curly,
    close_curly,
    open_quotation,
    close_quotation,
    colon,
    comma,
    key_part,
    value_part,
    none
};

enum class State
{
    key,
    value,
    none
};

void parse_json(const char *input)
{
    TokenKind current = TokenKind::none;
    State state = State::none;
    char *current_key = nullptr;
    JValue *current_value = nullptr;
    for (size_t pos = 0; input[pos] != '\0'; ++pos)
    {
        if (input[pos] == ' ' ||
            input[pos] == '\t' ||
            input[pos] == '\n')
            continue;
        switch (input[pos])
        {
        case '"':
        {
            switch (current)
            {
            case TokenKind::open_curly:
            {
                current = TokenKind::open_quotation;
                state = State::key;
            }
            break;
            case TokenKind::open_quotation:
            {
                if (input[pos - 1] != '\\')
                    current = TokenKind::close_quotation;
                else
                    goto process_char;
            }
            break;
            case TokenKind::colon:
            {
                current = TokenKind::open_quotation;
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
            if (current == TokenKind::close_quotation)
            {
                current = TokenKind::open_quotation;
                state = State::key;
            }
            else
            {
                goto process_char;
            }
        }
        break;
        case ':':
        {
            if (current == TokenKind::close_quotation)
                current = TokenKind::colon;
            else
                goto process_char;
        }
        break;
        case '{':
        {
            if (current == TokenKind::none || current == TokenKind::colon)
                current = TokenKind::open_curly;
            else
                goto process_char;
        }
        break;
        case '}':
        {
            if (current == TokenKind::close_quotation)
                current = TokenKind::close_curly;
            else
                goto process_char;
        }
        break;
        default:
// TokenKind::open_quotation always?
process_char:
        {
            switch (current)
            {
            case TokenKind::open_quotation:
            {
                if (state == State::key)
                {
                    size_t start_pos = pos;
                    do
                    {
                        pos++;
                    } while (input[pos] != '"');
                    size_t key_size = pos - start_pos + 1;
                    current_key = static_cast<char *>(malloc(key_size));
                    memcpy(current_key, input + start_pos, key_size - 1);
                    current_key[key_size - 1] = '\0';
                    current = TokenKind::close_quotation;
                    // is this needed?
                    state = State::none;
                }
                else if (state == State::value)
                {
                    size_t start_pos = pos;
                    do
                    {
                        pos++;
                    } while (input[pos] != '"');
                    size_t value_size = pos - start_pos + 1;
                    char* value_string = static_cast<char *>(malloc(value_size));
                    memcpy(value_string, input + start_pos, value_size - 1);
                    value_string[value_size - 1] = '\0';
                    // current_value = new JValue(value_string);
                    current_value = static_cast<JValue *>(malloc(sizeof(JValue)));
                    new (current_value) JValue(value_string);
                    printf("value: %s\n", std::get<char*>(*current_value));
                    current = TokenKind::close_quotation;
                    // is this needed?
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
}

int main()
{
    parse_json(JSON_INPUT);
    return 0;
}