#pragma once

#if defined(_MSC_VER)
#pragma warning(disable : 4464) // relative include path contains '..'
#pragma warning(disable : 4711) // function selected for automatic inline expansion
#pragma warning(disable : 4710) // function not inlined
#pragma warning(disable : 4514) // unreferenced inline function has been removed
#pragma warning(disable : 4577) // 'noexcept' used with no exception handling mode specified; termination on exception is not guaranteed. Specify /EHsc
#pragma warning(disable : 4530) // C++ exception handler used, but unwind semantics are not enabled.Specify / EHsc
#pragma warning(disable : 5045) // Compiler will insert Spectre mitigation for memory load if /Qspectre switch specified
#endif

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <clocale>
#include <cstdio>
#include <cstring>
#include <deque>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <locale.h>
#include <map>
#include <queue>
#include <set>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>