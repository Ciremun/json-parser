#define NDEBUG

#define JMEM_IMPLEMENTATION
#include "../jmem.h"

#define JP_IMPLEMENTATION
#include "../jp.h"

#include "test.h"

size_t total_errors;

void test_errors()
{
    const char* input = "{}";

    JMemory *memory = jmem_init();
    TEST(memory != 0);

    JParser parser = json_init(memory, input);
    JValue json = json_parse(&parser);

    if (TEST(json.type == JSON_OBJECT))
    {
        JValue nested_error = json["deep"][6]["dark"][9]["error"];
        if (TEST(nested_error.type == JSON_ERROR))
            TEST(nested_error.error == JSON_TYPE_ERROR);
    }

    jmem_free(memory);
}

Test tests[] = {
    { "errors", test_errors },
};

int main()
{
    for (size_t i = 0; i < COUNT(tests); ++i)
        tests[i].f();

    TOTAL_ERRORS
}
