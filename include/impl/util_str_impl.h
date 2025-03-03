#pragma once

#include <locale.h>
#include <stdlib.h>

#include <cerrno>
#include <clocale>
#include <cstring>
#include <iomanip>
#include <string>

#if defined(_WIN32) || defined(_WIN64)
#include <stringapiset.h>
#endif

namespace small::strimpl {

    //
    // string conversions
    //

    //
    // from utf8 -> utf16 needed length
    //
    inline std::size_t to_utf16_needed_length(const char* mbstr /*src*/, [[maybe_unused]] const std::size_t& mbsize)
    {
        std::setlocale(LC_ALL, "");
        std::mbstate_t state{};

        // determine size
        std::size_t new_length = 0;
#if defined(_WIN32) || defined(_WIN64)
        int wsize = MultiByteToWideChar(CP_UTF8, 0, mbstr, (int)mbsize, NULL, 0);
        if (wsize <= 0) {
            return static_cast<std::size_t>(-1);
        }
        new_length = static_cast<std::size_t>(wsize);
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
    inline void to_utf16(const char* mbstr /*src*/, [[maybe_unused]] const std::size_t& mbsize, wchar_t* wstr /*dest*/, const std::size_t& wsize /*dest lengt*/)
    {
        std::setlocale(LC_ALL, "");
        std::mbstate_t state{};

#if defined(_WIN32) || defined(_WIN64)
        size_t converted = 0;
        int    converted = MultiByteToWideChar(CP_UTF8, 0, mbstr, (int)mbsize, wstr, wsize);
        wstr[converted]  = L'\0';
#else
        /*size_t converted =*/std::mbsrtowcs(wstr, &mbstr, wsize, &state);
#endif
    }

    //
    // from utf16 -> utf8 needed length
    //
    inline std::size_t to_utf8_needed_length(const wchar_t* wstr /*src*/, [[maybe_unused]] const std::size_t& wsize)
    {
        std::setlocale(LC_ALL, "");
        std::mbstate_t state{};

        // determine size
        std::size_t new_length = 0;

#if defined(_WIN32) || defined(_WIN64)
        int wsize = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)wsize, NULL, 0, NULL, NULL);
        if (wsize <= 0) {
            return static_cast<std::size_t>(-1);
        }
        new_length = static_cast<std::size_t>(wsize);
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
    inline void to_utf8(const wchar_t* wstr /*src*/, [[maybe_unused]] const std::size_t& wsize, char* mbstr /*dest*/, const std::size_t& mbsize /*dest length*/)
    {
        std::setlocale(LC_ALL, "");
        std::mbstate_t state{};

#if defined(_WIN32) || defined(_WIN64)
        int converted    = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)wsize, mbstr, (int)mbsize, NULL, NULL);
        mbstr[converted] = '\0';
#else
        /*size_t converted =*/std::wcsrtombs(mbstr, &wstr, mbsize, &state);
#endif
    }

} // namespace small::strimpl
