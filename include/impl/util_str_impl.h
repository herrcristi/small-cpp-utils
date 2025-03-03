#pragma once

#include <locale.h>
#include <stdlib.h>

#include <cerrno>
#include <clocale>
#include <cstring>
#include <iomanip>
#include <string>

namespace small::strimpl {

    //
    // string conversions
    //

    //
    // from utf8 -> utf16 needed length
    //
    inline std::size_t to_utf16_needed_length(const char* mbstr /*src*/)
    {
        std::setlocale(LC_ALL, "");
        std::mbstate_t state{};

        // determine size
        std::size_t new_length = 0;
#if defined(_WIN32) || defined(_WIN64)
        int ret = ::mbsrtowcs_s(&new_length, nullptr, 0, &mbstr, 0, &state);
        if (ret != 0 || new_length == 0) {
            return static_cast<std::size_t>(-1);
        }
        --new_length; // because it adds the null terminator in length
#else
        new_length = std::mbsrtowcs(nullptr, &mbstr, 0, &state);
        if (new_length == static_cast<std::size_t>(-1)) {
            return static_cast<std::size_t>(-1);
        }
#endif
        return new_length;
    }

    //
    // from utf8 -> utf16 convert
    //
    inline void to_utf16(const char* mbstr /*src*/, wchar_t* wstr /*dest*/, const std::size_t& new_length /*dest lengt*/)
    {
        std::setlocale(LC_ALL, "");
        std::mbstate_t state{};

#if defined(_WIN32) || defined(_WIN64)
        size_t converted = 0;
        /* ret              = */ ::mbsrtowcs_s(&converted, wstr, new_length + 1, &mbstr, new_length, &state);
#else
        /*size_t converted =*/std::mbsrtowcs(wstr, &mbstr, new_length, &state);
#endif
    }

    //
    // from utf16 -> utf8 needed length
    //
    inline std::size_t to_utf8_needed_length(const wchar_t* wstr /*src*/)
    {
        std::setlocale(LC_ALL, "");
        std::mbstate_t state{};

        // determine size
        std::size_t new_length = 0;

#if defined(_WIN32) || defined(_WIN64)
        int ret = ::wcsrtombs_s(&new_length, nullptr, 0, &wstr, 0, &state);
        if (ret != 0 || new_length == 0) {
            return static_cast<std::size_t>(-1);
        }
        --new_length; // because it adds the null terminator in length
#else
        new_length = std::wcsrtombs(nullptr, &wstr, 0, &state);
        if (new_length == static_cast<std::size_t>(-1)) {
            return static_cast<std::size_t>(-1);
        }
#endif
        return new_length;
    }

    //
    // from utf16 -> utf8 convert
    //
    inline void to_utf8(const wchar_t* wstr /*src*/, char* mbstr /*dest*/, const std::size_t& new_length /*dest length*/)
    {
        std::setlocale(LC_ALL, "");
        std::mbstate_t state{};

#if defined(_WIN32) || defined(_WIN64)
        size_t converted = 0;
        /* ret              = */ ::wcsrtombs_s(&converted, mbstr, new_length + 1, &wstr, new_length, &state);
#else
        /*size_t converted =*/std::wcsrtombs(mbstr, &wstr, new_length, &state);
#endif
    }

} // namespace small::strimpl
