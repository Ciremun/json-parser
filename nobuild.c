#define NOBUILD_IMPLEMENTATION
#include "nobuild.h"

#include <stdlib.h>
#include <string.h>

#define CXXFLAGS "-Wall", "-Wextra", "-pedantic", "-std=c++2a", "-ggdb"
#define MSVC_CXXFLAGS "/std:c++latest"
#define OUTPUT "jp"
#define OUTPUT_FLAGS "-o" OUTPUT
#define MSVC_OUTPUT_FLAGS "/Fe:" OUTPUT

#ifdef _WIN32
#define DEFAULT_CXX "cl"
#define RUN ".\\" OUTPUT ".exe"
#else
#define DEFAULT_CXX "g++"
#define RUN "./" OUTPUT
#endif

char cxx[32];

void set_cxx()
{
    char *c = getenv("cxx");
    if (c == NULL)
        memcpy(cxx, DEFAULT_CXX, sizeof(DEFAULT_CXX));
    else
        strcpy(cxx, c);
}

void build()
{
    set_cxx();
    if (strcmp(cxx, "cl") == 0)
        CMD(cxx, MSVC_CXXFLAGS, "test.cpp", MSVC_OUTPUT_FLAGS);
    else
        CMD(cxx, CXXFLAGS, "test.cpp", OUTPUT_FLAGS);
}

void run()
{
    build();
    CMD(RUN);
}

void process_args(int argc, char **argv)
{
    int run_flag = 0;
    for (int i = 0; i < argc; ++i)
    {
        if (!run_flag && strcmp(argv[i], "run") == 0)
        {
            run_flag = 1;
            run();
        }
    }
}

int main(int argc, char **argv)
{
    GO_REBUILD_URSELF(argc, argv);

    if (argc > 1)
        process_args(argc, argv);
    else
        build();

    return 0;
}
