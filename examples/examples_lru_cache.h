#pragma once

#include "examples_common.h"

#include "../include/lru_cache.h"

namespace examples::lru_cache {
    //
    //  example 1
    //
    inline int Example1()
    {
        std::cout << "LRU Cache\n";

        small::lru_cache<int, std::string> cache({.capacity = 2});

        cache.set(1, "A");
        cache.set(2, "B");
        std::cout << "get 1=" << *cache.get(1) << std::endl; // returns A
        cache.set(3, "C");
        std::cout << "get 2=" << cache.get(2) << std::endl; // returns nullptr (not found)
        cache.set(4, "D");
        std::cout << "get 1=" << cache.get(1) << std::endl;  // returns nullptr (not found)
        std::cout << "get 3=" << *cache.get(3) << std::endl; // returns C
        std::cout << "get 4=" << *cache[4] << std::endl;     // returns D
        *cache[4] = "E";
        std::cout << "get 4=" << *cache[4] << std::endl; // returns E
        cache.erase(4);
        std::cout << "get 4=" << cache[4] << std::endl; // returns nullptr (not found)

        std::cout << "LRU Cache finished\n\n";

        return 0;
    }
} // namespace examples::lru_cache