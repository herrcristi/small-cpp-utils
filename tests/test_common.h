#pragma once

#if defined(_MSC_VER)
#pragma warning(disable : 4464) // relative include path contains '..'

// for tests only
#pragma warning(disable : 4625) // '`anonymous-namespace'::...Test': copy constructor was implicitly defined as deleted
#pragma warning(disable : 5026) // '`anonymous-namespace'::...Test': move constructor was implicitly defined as deleted
#pragma warning(disable : 4626) // '`anonymous-namespace'::...Test': assignment operator was implicitly defined as deleted
#pragma warning(disable : 5027) // '`anonymous-namespace'::...Test': move assignment operator was implicitly defined as deleted

#endif

#include "../include/impl/impl_common.h"

#include <gtest/gtest.h>

#include <latch> // c++20
#include <thread>