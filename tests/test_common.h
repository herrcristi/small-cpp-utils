#pragma once

#if defined(_MSC_VER)
#pragma warning(disable : 4464) // relative include path contains '..'
#endif

#include "../include/impl/impl_common.h"

#include <gtest/gtest.h>

#include <latch> // c++20
#include <thread>