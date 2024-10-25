#pragma once

#include "impl/base64_impl.h"

#include "buffer.h"

//
// std::string b64 = small::tobase64_s( "hello world" );
// std::vector<char> vb64 = small::tobase64_v( "hello world", 11 );
//
// std::string decoded = small::frombase64_s( b64 );
// std::vector<char> vd64 = small::frombase64_v( b64 );
//
namespace small {
    //
    // implementations for common data
    //

    //
    // tobase64 return the type that is requested
    //
    template <typename T = std::string>
    inline T tobase64(const char *src, const std::size_t src_length)
    {
        T base64;
        std::size_t base64_size = base64impl::get_base64_size(src_length);
        base64.resize(base64_size);

        // encode
        base64impl::tobase64(reinterpret_cast<char *>(base64.data()), src, src_length);

        // compiler should do move
        return base64;
    }

    template <typename T = std::string>
    inline T tobase64(const unsigned char *src, const std::size_t src_length)
    {
        return tobase64<T>(reinterpret_cast<const char *>(src), src_length);
    }

    // to base64 with different arguments
    template <typename T = std::string>
    inline T tobase64(const std::string_view v)
    {
        return tobase64<T>(reinterpret_cast<const char *>(v.data()), v.size());
    }

    template <typename T = std::string>
    inline T tobase64(const std::vector<char> &v)
    {
        return tobase64<T>(reinterpret_cast<const char *>(v.data()), v.size());
    }

    template <typename T = std::string>
    inline T tobase64(const std::vector<unsigned char> &v)
    {
        return tobase64<T>(reinterpret_cast<const char *>(v.data()), v.size());
    }

    //
    // frombase64
    //
    template <typename T = std::string /*decoded may not be always string*/>
    inline T frombase64(const char *base64, const std::size_t base64_length)
    {
        T decoded;
        std::size_t decoded_size = base64impl::get_decodedbase64_size(base64_length);
        decoded.resize(decoded_size);

        // decode
        std::size_t decoded_length = base64impl::frombase64(reinterpret_cast<char *>(decoded.data()), base64, base64_length);
        decoded.resize(decoded_length);

        // compiler should do move
        return decoded;
    }

    template <typename T = std::string>
    inline T frombase64(const unsigned char *base64, const std::size_t base64_length)
    {
        return frombase64<T>(reinterpret_cast<const char *>(base64), base64_length);
    }

    // frombase64
    template <typename T = std::string>
    inline T frombase64(const std::string_view base64)
    {
        return frombase64<T>(reinterpret_cast<const char *>(base64.data()), base64.size());
    }

    template <typename T = std::string>
    inline T frombase64(const std::vector<char> &base64)
    {
        return frombase64<T>(reinterpret_cast<const char *>(base64.data()), base64.size());
    }

    template <typename T = std::string>
    inline T frombase64(const std::vector<unsigned char> &base64)
    {
        return frombase64<T>(reinterpret_cast<const char *>(base64.data()), base64.size());
    }

} // namespace small