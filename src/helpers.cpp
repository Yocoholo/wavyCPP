#include "helpers.h"
#include <cstdarg>
#include <stdio.h>
#include <iostream>

void debug(const char* format, ...)
{
    // Create a va_list to hold the variable arguments
    va_list args;
    va_start(args, format);

    // Use vprintf to print the formatted string to std::cout
    vprintf(format, args);

    std::cout << std::flush;
    // End the va_list
    va_end(args);
}