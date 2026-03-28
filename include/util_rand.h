#pragma once

#include <random>

namespace small {

    namespace {
        // mersenne_twister_engine seeded with rd()
        thread_local std::mt19937 g_rng(std::random_device{}());
    } // namespace

    //
    // rand utils
    //
    inline unsigned char rand8()
    {
        std::uniform_int_distribution<> dis(0, 255);
        return static_cast<unsigned char>(dis(g_rng));
    }

    inline unsigned short int rand16()
    {
        std::uniform_int_distribution<> dis(0, 65535);
        return static_cast<unsigned short int>(dis(g_rng));
    }

    inline unsigned long rand32()
    {
        return g_rng();
    }

    inline unsigned long long rand64()
    {
        return g_rng();
    }

} // namespace small
