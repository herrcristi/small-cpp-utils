#pragma once

#include <cstddef>
#include <string_view>

namespace small {
    //
    // quick hash function for buffer
    // may produce more collisions than standard hash functions, but is much faster
    //
    inline unsigned long long qhash131(const char* buffer, const std::size_t length, unsigned long long start_hash = 0)
    {
        if (buffer == nullptr) {
            return start_hash;
        }

        // h = h * 131 + char
        for (std::size_t i = 0; i < length; ++i, ++buffer) {
            // overflow is by design
            start_hash = (start_hash << 7) + (start_hash << 1) + start_hash + (unsigned char)*buffer;
        }
        return start_hash;
    }

    inline unsigned long long qhash131(const std::string_view v, unsigned long long start_hash = 0)
    {
        // h = h * 131 + char
        for (auto& ch : v) {
            start_hash = (start_hash << 7) + (start_hash << 1) + start_hash + (unsigned char)ch;
        }
        return start_hash;
    }

    //
    // quick hash function for null terminating string
    //
    inline unsigned long long qhash131z(const char* buffer, unsigned long long start_hash = 0)
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

    //
    // quick hash function for buffer (FNV-1a)
    //
    inline unsigned long long qhash1a(const char* buffer, const std::size_t length, unsigned long long start_hash = 0)
    {
        if (buffer == nullptr) {
            return start_hash;
        }

        // Use FNV-1a algorithm (standard, collision-resistant)
        const unsigned long long FNV_PRIME = 1099511628211ULL;
        unsigned long long       hash      = start_hash ? start_hash : 14695981039346656037ULL;

        for (std::size_t i = 0; i < length; ++i) {
            hash ^= (unsigned char)buffer[i];
            hash *= FNV_PRIME; // Will overflow naturally, providing avalanche effect
        }
        return hash;
    }

    inline unsigned long long qhash1a(const std::string_view v, unsigned long long start_hash = 0)
    {
        // Use FNV-1a algorithm (standard, collision-resistant)
        const unsigned long long FNV_PRIME = 1099511628211ULL;
        unsigned long long       hash      = start_hash ? start_hash : 14695981039346656037ULL;

        for (auto& ch : v) {
            hash ^= (unsigned char)ch;
            hash *= FNV_PRIME; // Will overflow naturally, providing avalanche effect
        }
        return hash;
    }

    inline unsigned long long qhash1az(const char* buffer, unsigned long long start_hash = 0)
    {
        if (buffer == nullptr) {
            return start_hash;
        }

        // Use FNV-1a algorithm (standard, collision-resistant)
        const unsigned long long FNV_PRIME = 1099511628211ULL;
        unsigned long long       hash      = start_hash ? start_hash : 14695981039346656037ULL;

        for (; *buffer != '\0'; ++buffer) {
            hash ^= (unsigned char)*buffer;
            hash *= FNV_PRIME; // Will overflow naturally, providing avalanche effect
        }
        return hash;
    }

    // Backward-compatible aliases (prefer FNV-1a for security)
    // ========================================================================

    // Original qhash name now points to FNV-1a (more secure than 131x)
    inline unsigned long long qhash(const char* buffer, const std::size_t length, unsigned long long start_hash = 0)
    {
        return qhash1a(buffer, length, start_hash);
    }

    inline unsigned long long qhash(const std::string_view v, unsigned long long start_hash = 0)
    {
        return qhash1a(v, start_hash);
    }

    // Original qhashz name now points to FNV-1a (more secure than 131x)
    inline unsigned long long qhashz(const char* buffer, unsigned long long start_hash = 0)
    {
        return qhash1az(buffer, start_hash);
    }

    // Note: For performance-critical code, use qhash131/qhash131z for original 131x algorithm
    // which is faster but more collision-prone
} // namespace small