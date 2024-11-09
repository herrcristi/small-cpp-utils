#pragma once

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <thread>

namespace small {
    // to lower
    // clang-format off
    const unsigned char to_lower[] = 
    {
               0,         1,        2,        3,        4,       5,         6,        7,        8,        9,       10,       11,
              12,        13,       14,       15,       16,      17,        18,       19,       20,       21,       22,       23,
              24,        25,       26,       27,       28,      29,        30,       31,  /* */32,  /*!*/33, /*\"*/34,  /*#*/35,
         /*$*/36,   /*%*/37,  /*&*/38, /*\'*/39,  /*(*/40,  /*)*/41,  /***/42,  /*+*/43,  /*,*/44,  /*-*/45,  /*.*/46,  /*/*/47,
         /*0*/48,   /*1*/49,  /*2*/50,  /*3*/51,  /*4*/52,  /*5*/53,  /*6*/54,  /*7*/55,  /*8*/56,  /*9*/57,  /*:*/58,  /*;*/59,
         /*<*/60,   /*=*/61,  /*>*/62,  /*?*/63,  /*@*/64, /*A*/'a', /*B*/'b', /*C*/'c', /*D*/'d', /*E*/'e', /*F*/'f', /*G*/'g',
        /*H*/'h',  /*I*/'i', /*J*/'j', /*K*/'k', /*L*/'l', /*M*/'m', /*N*/'n', /*O*/'o', /*P*/'p', /*Q*/'q', /*R*/'r', /*S*/'s',
        /*T*/'t',  /*U*/'u', /*V*/'v', /*W*/'w', /*X*/'x', /*Y*/'y', /*Z*/'z',  /*[*/91, /*\'*/92,  /*]*/93,  /*^*/94,  /*_*/95,
         /*`*/96,   /*a*/97,  /*b*/98,  /*c*/99, /*d*/100, /*e*/101, /*f*/102, /*g*/103, /*h*/104, /*i*/105, /*j*/106, /*k*/107,
        /*l*/108,  /*m*/109, /*n*/110, /*o*/111, /*p*/112, /*q*/113, /*r*/114, /*s*/115, /*t*/116, /*u*/117, /*v*/118, /*w*/119,
        /*x*/120,  /*y*/121, /*z*/122, /*{*/123, /*|*/124, /*}*/125, /*~*/126,  /**/127,  /**/128,  /**/129,  /**/130,  /**/131,
         /**/132,   /**/133,  /**/134,  /**/135,  /**/136,  /**/137, /*-*/154,  /**/139, /*-*/156,  /**/141, /*-*/158,  /**/143,
         /**/144,   /**/145,  /**/146,  /**/147,  /**/148,  /**/149,  /**/150,  /**/151,  /**/152,  /**/153,  /**/154,  /**/155,
         /**/156,   /**/157,  /**/158, /*-*/255,  /**/160,  /**/161,  /**/162,  /**/163,  /**/164,  /**/165,  /**/166,  /**/167,
         /**/168,   /**/169,  /**/170,  /**/171,  /**/172,  /**/173,  /**/174,  /**/175,  /**/176,  /**/177,  /**/178,  /**/179,
         /**/180,   /**/181,  /**/182,  /**/183,  /**/184,  /**/185,  /**/186,  /**/187,  /**/188,  /**/189,  /**/190,  /**/191,
        /*-*/224,   /**/225,  /**/226,  /**/227,  /**/228,  /**/229,  /**/230,  /**/231,  /**/232,  /**/233,  /**/234, /*-*/235,
        /*-*/236,   /**/237,  /**/238,  /**/239,  /**/240,  /**/241,  /**/242,  /**/243,  /**/244,  /**/245,  /**/246,  /**/215,
        /*-*/248,   /**/249,  /**/250,  /**/251,  /**/252,  /**/253, /*-*/254,  /**/223,  /**/224,  /**/225,  /**/226,  /**/227,
         /**/228,   /**/229,  /**/230,  /**/231,  /**/232,  /**/233,  /**/234,  /**/235,  /**/236,  /**/237,  /**/238,  /**/239,
         /**/240,   /**/241,  /**/242,  /**/243,  /**/244,  /**/245,  /**/246,  /**/247,  /**/248,  /**/249,  /**/250,  /**/251,
         /**/252,   /**/253,  /**/254,  /**/255
    };
    // clang-format on

    //
    // stricmp without locale
    //
    inline int stricmp(const char *dst, const char *src)
    {
        int f = 0;
        int l = 0;
        do {
            if (((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z')) {
                f += 'a' - 'A';
            }
            if (((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z')) {
                l += 'a' - 'A';
            }
        } while (f && (f == l));

        if (f == l) {
            return 0;
        }
        return f < l ? -1 : 1;
    }

    //
    // strnicmp without locale
    //
    inline int strnicmp(const char *dst, const char *src, std::size_t num)
    {
        int f = 0;
        int l = 0;
        for (; num > 0; num--) {
            if (((f = (unsigned char)(*(dst++))) >= 'A') && (f <= 'Z')) {
                f += 'a' - 'A';
            }
            if (((l = (unsigned char)(*(src++))) >= 'A') && (l <= 'Z')) {
                l += 'a' - 'A';
            }

            if (f && (f == l)) {
                continue;
            }

            break;
        }

        if (num == 0 || f == l) {
            return 0;
        }
        return f < l ? -1 : 1;
    }

    //
    // insensitive compare
    //
    struct icasecmp
    {
        inline bool operator()(const std::string_view a, const std::string_view b) const
        {
            return small::stricmp(a.data(), b.data()) < 0;
        }
    };

    //
    // sleep
    //
    inline void sleep(int time_in_ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(time_in_ms));
    }

    //
    // time utils
    //
    inline std::chrono::high_resolution_clock::time_point timeNow()
    {
        return std::chrono::high_resolution_clock::now();
    }

    inline long long timeDiffMs(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now())
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    inline long long timeDiffMicro(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now())
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }
    inline long long timeDiffNano(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now())
    {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    }

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

    //
    // uuid
    //
    inline std::pair<unsigned long long, unsigned long long> uuid128()
    {
        // generate 2 random uint64 numbers
        return {small::rand64(), small::rand64()};
    }

    inline std::string uuid_add_hyphen(std::string &u)
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

    inline std::string uuid_add_braces(std::string &u)
    {
        u.insert(0, 1, '{');
        u.insert(u.size(), 1, '}');
        return u;
    }

    inline std::string uuid_to_uppercase(std::string &u)
    {
        std::transform(u.begin(), u.end(), u.begin(), ::toupper);
        return u;
    }

    struct ConfigUUID
    {
        bool add_hyphen{false};
        bool add_braces{false};
        bool to_uppercase{false};
    };

    inline std::string uuid(const ConfigUUID config = {})
    {
        // generate 2 random uint64 numbers
        auto [r1, r2] = uuid128();

        std::stringstream ss;
        ss << std::hex << std::setfill('0');

        ss << std::setw(16) << r1;
        ss << std::setw(16) << r2;

        auto u = ss.str();

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
