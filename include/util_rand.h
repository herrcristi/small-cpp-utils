#pragma once

#include <random>

namespace small {

    //
    // rand utils
    //
    inline unsigned char rand8()
    {
        std::random_device rd;  // a seed source for the random number engine
        std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> dis(0, 255);
        return static_cast<unsigned char>(dis(gen));
    }

    inline unsigned short int rand16()
    {
        std::random_device rd;  // a seed source for the random number engine
        std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
        std::uniform_int_distribution<> dis(0, 65535);
        return static_cast<unsigned short int>(dis(gen));
    }

    inline unsigned long rand32()
    {
        std::random_device rd;  // a seed source for the random number engine
        std::mt19937 gen(rd()); // mersenne_twister_engine seeded with rd()
        return gen();
    }

    inline unsigned long long rand64()
    {
        std::random_device rd;     // a seed source for the random number engine
        std::mt19937_64 gen(rd()); // mersenne_twister_engine seeded with rd()
        return gen();
    }

} // namespace small
