#pragma once

#include <stdlib.h>

#include <algorithm>
#include <iomanip>
#include <string>

#include "impl/util_str_impl.h"

namespace small {
    // to lower
    // clang-format off
    const unsigned char to_lower_table[] = 
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
    inline int stricmp(const char* dst, const char* src)
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
    inline int strnicmp(const char* dst, const char* src, std::size_t num)
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
    // conversion to_lower_case, to_upper_case
    //
    inline std::string to_lower_case(std::string& u)
    {
        std::transform(u.begin(), u.end(), u.begin(), ::tolower);
        return u;
    }

    inline std::string to_upper_case(std::string& u)
    {
        std::transform(u.begin(), u.end(), u.begin(), ::toupper);
        return u;
    }

    inline std::string to_capitalize_case(std::string& u)
    {
        std::transform(u.begin(), u.end(), u.begin(), ::tolower);
        if (u.begin() != u.end()) {
            *u.begin() = static_cast<char>(::toupper(*u.begin()));
        }
        return u;
    }

    //
    // convert to hex representation
    //
    struct to_hex_config
    {
        bool fill{}; // fill with zeroes up to the size
    };

    template <typename T>
    inline std::string to_hex(const T number, const to_hex_config config = {})
    {
        std::stringstream ss;
        ss << std::hex;
        if (config.fill) {
            ss << std::setfill('0');
            ss << std::setw(2 * sizeof(T));
        }

        ss << number;
        return ss.str();
    }

    template <typename T>
    inline std::string to_hex_fill(const T number)
    {
        return to_hex(number, {.fill = true});
    }

    //
    // string conversions
    //

    // from utf8 -> utf16
    inline std::wstring to_utf16(const char* str8, const std::size_t& mbsize)
    {
        std::wstring wstr;
        if (str8 == nullptr || mbsize == 0) {
            return wstr;
        }

        // check if it is null terminated and create a string if it is not
        const char* mbstr = str8;
        std::string str_null_terminated;
        if (mbstr[mbsize] != '\0') {
            str_null_terminated.append(mbstr, mbsize);
            mbstr = str_null_terminated.c_str();
        }

        std::size_t wsize = small::strimpl::to_utf16_needed_length(mbstr, mbsize);
        if (wsize == static_cast<std::size_t>(-1)) {
            return wstr;
        }

        wstr.resize(wsize);
        small::strimpl::to_utf16(mbstr, mbsize, wstr.data(), wstr.size());
        return wstr;
    }

    inline std::wstring to_utf16(const std::string_view sv)
    {
        return to_utf16(sv.data(), sv.size());
    }

    // from utf16 -> utf8
    inline std::string to_utf8(const wchar_t* wstr16, const std::size_t& wsize)
    {
        std::string str;
        if (wstr16 == nullptr || wsize == 0) {
            return str;
        }

        // check if it is null terminated and create a string if it is not
        const wchar_t* wstr = wstr16;
        std::wstring   wstr_null_terminated;
        if (wstr[wsize] != L'\0') {
            wstr_null_terminated.append(wstr16, wsize);
            wstr = wstr_null_terminated.c_str();
        }

        std::size_t mbsize = small::strimpl::to_utf8_needed_length(wstr, wsize);
        if (mbsize == static_cast<std::size_t>(-1)) {
            return str;
        }

        str.resize(mbsize);
        small::strimpl::to_utf8(wstr, wsize, str.data(), str.size());
        return str;
    }

    inline std::string to_utf8(const std::wstring_view wstr16)
    {
        return to_utf8(wstr16.data(), wstr16.size());
    }
} // namespace small
