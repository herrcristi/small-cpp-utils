#if defined(_MSC_VER)
#pragma warning(disable : 4464) // relative include path contains '..'
#pragma warning(disable : 4577) // 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed
#pragma warning(disable : 4711) // function selected for automatic inline expansion
#pragma warning(disable : 4710) // function not inlined
#endif

#include <gtest/gtest.h>

#include <latch> // c++20
#include <thread>