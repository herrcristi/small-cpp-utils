#include <condition_variable>
#include <cstdio>
#include <iostream>
#include <map>
#include <thread>

#pragma warning(disable : 4464) // relative include path contains '..'

#include "../include/base64.h"

namespace examples::base64 {
    //
    // example 1
    //
    inline int Example1()
    {
        std::cout << "Base64\n";

        constexpr std::string_view str{"hello world"};

        std::string b64  = small::tobase64(str);
        auto        vb64 = small::tobase64<std::vector<char>>(str.data(), str.size());

        std::cout << "base64(\"" << str << "\") is " << b64 << "\n";
        std::cout << "base64 as vector(\"" << str << "\") is ";
        for (auto ch : vb64) {
            std::cout << ch;
        }
        std::cout << "\n";

        std::string decoded     = small::frombase64(vb64);
        auto        decodedvd64 = small::frombase64<std::vector<char>>(b64);

        std::cout << "decoded base64 is \"" << decoded << "\"\n";

        std::cout << "Base64 finished\n\n";

        return 0;
    }
} // namespace examples::base64