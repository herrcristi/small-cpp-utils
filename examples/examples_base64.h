#pragma once

#include "examples_common.h"

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

        // frombase64 now returns std::optional<T>
        auto decoded_opt = small::frombase64(vb64);
        if (decoded_opt) {
            std::cout << "decoded base64 is \"" << *decoded_opt << "\"\n";
        } else {
            std::cout << "failed to decode base64\n";
        }

        auto decodedvd64_opt = small::frombase64<std::vector<char>>(b64);
        if (decodedvd64_opt) {
            std::cout << "decoded vector is valid\n";
        } else {
            std::cout << "failed to decode base64 to vector\n";
        }

        std::cout << "Base64 finished\n\n";

        return 0;
    }
} // namespace examples::base64