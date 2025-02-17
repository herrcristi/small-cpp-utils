#pragma once

#include <iomanip>
#include <sstream>
#include <string>

#include "util_rand.h"
#include "util_str.h"

namespace small {
    //
    // uuid
    //
    inline std::pair<unsigned long long, unsigned long long> uuidp()
    {
        // generate 2 random uint64 numbers
        return {small::rand64(), small::rand64()};
    }

    inline std::string uuid_add_hyphen(std::string& u)
    {
        // add in reverse order
        if (u.size() > 20) {
            u.insert(20, 1, '-');
        }
        if (u.size() > 16) {
            u.insert(16, "-");
        }
        if (u.size() > 12) {
            u.insert(12, "-");
        }
        if (u.size() > 8) {
            u.insert(8, "-");
        }
        return u;
    }

    inline std::string uuid_add_braces(std::string& u)
    {
        u.insert(0, 1, '{');
        u.insert(u.size(), 1, '}');
        return u;
    }

    inline std::string uuid_to_uppercase(std::string& u)
    {
        return small::toUpperCase(u);
    }

    struct config_uuid
    {
        bool add_hyphen{false};
        bool add_braces{false};
        bool to_uppercase{false};
    };

    inline std::string uuid(const config_uuid config = {})
    {
        // generate 2 random uint64 numbers
        auto [r1, r2] = uuidp();

        auto u = small::toHexF(r1) + small::toHexF(r2);

        if (config.add_hyphen) {
            uuid_add_hyphen(u);
        }

        if (config.add_braces) {
            uuid_add_braces(u);
        }

        if (config.to_uppercase) {
            uuid_to_uppercase(u);
        }

        return u;
    }

    // return the uuid with uppercases
    inline std::string uuidc()
    {
        return uuid({.to_uppercase = true});
    }

} // namespace small
