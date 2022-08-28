#define NOBUILD_IMPLEMENTATION
#include "nobuild.h"

#include <stdlib.h>
#include <string.h>

#define CFLAGS "-Wall", "-Wextra", "-pedantic", "-std=c11", "-O0", "-ggdb"
#define MSVC_CFLAGS "/nologo", "/W3", "/std:c11"
#define CXXFLAGS "-Wall", "-Wextra", "-pedantic", "-std=c++11", "-O0", "-ggdb"
#define MSVC_CXXFLAGS "/nologo", "/W3", "/std:c++11"

#ifdef _WIN32
#define DEFAULT_CC "cl"
#define DEFAULT_CXX "cl"
#define RUN(executable) CMD(".\\" executable ".exe")
#else
#define DEFAULT_CC "gcc"
#define DEFAULT_CXX "g++"
#define RUN(executable) CMD("./" executable)
#endif

#define SET_COMPILER_EXECUTABLE(env_var, runtime_var, default_executable)      \
    do                                                                         \
    {                                                                          \
        char *c = getenv(env_var);                                             \
        if (c == NULL)                                                         \
            memcpy(runtime_var, default_executable,                            \
                   sizeof(default_executable));                                \
        else                                                                   \
            strcpy(runtime_var, c);                                            \
    } while (0)

char cc[32];
char cxx[32];

void run_tests()
{
    if (strcmp(cc, "cl") == 0)
        CMD(cc, MSVC_CFLAGS, "tests/test.c", "/Fe:", "c-tests");
    else
        CMD(cc, CFLAGS, "tests/test.c", "-o", "c-tests");
    if (strcmp(cxx, "cl") == 0)
        CMD(cxx, MSVC_CXXFLAGS, "tests/test.cpp", "/Fe:", "cxx-tests");
    else
        CMD(cxx, CXXFLAGS, "tests/test.cpp", "-o", "cxx-tests");
    RUN("c-tests");
    RUN("cxx-tests");
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);

    SET_COMPILER_EXECUTABLE("cc", cc, DEFAULT_CC);
    SET_COMPILER_EXECUTABLE("cxx", cxx, DEFAULT_CXX);

    if (argc > 1)
    {
        if (strcmp(argv[1], "examples") == 0)
        {
            int cl = strcmp(cc, "cl") == 0;
            if (cl)
                CMD(cc, MSVC_CFLAGS, "examples/twitch-payload.c", "/Fe:", "twitch-payload-c");
            else
                CMD(cc, CFLAGS, "examples/twitch-payload.c", "-o", "twitch-payload-c");
            RUN("twitch-payload-c");
            if (cl)
                CMD(cc, MSVC_CXXFLAGS, "examples/twitch-payload.cpp", "/Fe:", "twitch-payload-cpp");
            else
                CMD(cc, CXXFLAGS, "examples/twitch-payload.cpp", "-o", "twitch-payload-cpp");
            RUN("twitch-payload-cpp");
            return 0;
        }
    }

    run_tests();
    return 0;
}
