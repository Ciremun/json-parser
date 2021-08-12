#include <cstdio>

#define JP_IMPLEMENTATION
#include "jp.hpp"

char *read_file_as_str(const char *path)
{
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    char* str = static_cast<char *>(calloc(1, size));
    rewind(f);
    fread(str, sizeof(char), size, f);
    return str;
}

int main()
{
    char* input = read_file_as_str("input.json");
    printf("%s\n", input);
    JsonParser parser(input);
    JObject json = parser.parse();
    printf("%s\n", json.str("test"));
    printf("%s\n", json.obj("owo").str("uwu"));
    printf("%s\n", json.obj("owo").obj("deep").obj("dark").str("dungeon"));
    parser.free();
    free(input);
    return 0;
}
