#pragma once

#include <cstddef>

namespace small {
    //
    // quick hash function for buffer
    //
    inline unsigned long long quick_hash_b(const char *buffer, const std::size_t length, unsigned long long start_hash = 0)
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

    //
    // quick hash function for null terminating string
    //
    inline unsigned long long quick_hash_z(const char *buffer, unsigned long long start_hash = 0)
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