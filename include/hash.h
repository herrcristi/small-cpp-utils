#pragma once

#include <cstddef>
#include <string_view>

namespace small {
    //
    // quick hash function for buffer
    //
    inline unsigned long long qhash(const char* buffer, const std::size_t length, unsigned long long start_hash = 0)
    {
        if (buffer == nullptr) {
            return start_hash;
        }

        // h = h * 131 + char
        for (std::size_t i = 0; i < length; ++i, ++buffer) {
            start_hash = (start_hash << 7) + (start_hash << 1) + start_hash + (unsigned char)*buffer;
        }
        return start_hash;
    }

    inline unsigned long long qhash(const std::string_view v, unsigned long long start_hash = 0)
    {
        // h = h * 131 + char
        for (auto& ch : v)
            start_hash = (start_hash << 7) + (start_hash << 1) + start_hash + (unsigned char)ch;
        return start_hash;
    }

    //
    // quick hash function for null terminating string
    //
    inline unsigned long long qhashz(const char* buffer, unsigned long long start_hash = 0)
    {
        if (buffer == nullptr) {
            return start_hash;
        }

        // h = h * 131 + char
        for (; *buffer != '\0'; ++buffer) {
            start_hash = (start_hash << 7) + (start_hash << 1) + start_hash + (unsigned char)*buffer;
        }
        return start_hash;
    }
} // namespace small