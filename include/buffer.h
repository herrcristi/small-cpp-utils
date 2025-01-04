#pragma once

#include <stddef.h>
#include <string.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "impl/base_buffer_impl.h"

//
// small::buffer b;
// b.clear();
//
// b.asign( "ana", 3 );
// b.set( 2/*from*/, "b", 1 );
//
// char* e = b.extract(); // extract "anb"
// small::buffer::free( e );
//
// small::buffer b1 = { 8192/*chunksize*/, "buffer", 6/*length*/ };
// small::buffer b2 = { 8192/*chunksize*/, "buffer" };
// small::buffer b3 = "buffer";
// small::buffer b4 = std::string( "buffer" );
//
// auto d1 = b2.c_string();
// auto v1 = b2.c_vector();
//
// b.append( "hello", 5 );
// b.clear( true );
//
// char* e1 = b.extract(); // extract ""
// small::buffer::free( e1 );
//
// b.append( "world", 5 );
// b.clear();
//
// constexpr std::stringview text{ "hello world" }
// std::string s64 = small::tobase64( text );
// b.clear();
// b = small::frombase64<small::buffer>( s64 );
//
namespace small {

    // for buffer configuration
    struct config_buffer
    {
        std::size_t chunk_size{8192}; // default chunk size, use for allocating
    };

    //
    // class for representing a buffer
    //
    class buffer : public small::bufferimpl::base_buffer
    {
    public:
        // buffer (allocates in chunks)
        buffer(const config_buffer config = {}) : m_config(config)
        {
            init();
        }

        // from buffer
        buffer(const buffer &o) noexcept : buffer()
        {
            init();
            operator=(o);
        }
        buffer(buffer &&o) noexcept : buffer()
        {
            init();
            operator=(std::forward<buffer>(o));
        }

        // clang-format off
        // from char*
        buffer(const char c) noexcept                           : buffer() { base_buffer::operator=(c); }
        buffer(const char *s) noexcept                          : buffer() { base_buffer::operator=(s); }
        buffer(const char *s, std::size_t s_length) noexcept    : buffer() { set(0 /*from*/, s, s_length); }
        buffer(const std::string s) noexcept                    : buffer() { base_buffer::operator=(s); }
        buffer(const std::string_view s) noexcept               : buffer() { base_buffer::operator=(s); }
        buffer(const std::vector<char> &v) noexcept             : buffer() { base_buffer::operator=(v); }

        // from char*
        buffer(const config_buffer config, const char c) noexcept                          : buffer(config) { base_buffer::operator=(c); }
        buffer(const config_buffer config, const char *s) noexcept                         : buffer(config) { base_buffer::operator=(s); }
        buffer(const config_buffer config, const char *s, std::size_t s_length) noexcept   : buffer(config) { set(0 /*from*/, s, s_length); }
        buffer(const config_buffer config, const std::string &s) noexcept                  : buffer(config) { base_buffer::operator=(s); }
        buffer(const config_buffer config, const std::string_view s) noexcept              : buffer(config) { base_buffer::operator=(s); }
        buffer(const config_buffer config, const std::vector<char> &v) noexcept            : buffer(config) { base_buffer::operator=(v); }
        // clang-format on

        // destructor
        ~buffer() override
        {
            free_chunk_buffer();
        }

        // chunk size
        inline std::size_t get_chunk_size() const
        {
            return m_config.chunk_size;
        }
        inline void set_chunk_size(std::size_t chunk_size)
        {
            m_config.chunk_size = chunk_size;
        }

        // clear / reserve / resize / shrink_to_fit fn are in base_buffer

        // extra clear and free buffer
        inline void clear_buffer()
        {
            free_chunk_buffer();
            clear();
        }

        // extract buffer - be sure to call free after you use it
        inline char *extract()
        {
            char *b = nullptr;
            if (m_chunk_buffer_data == nullptr || m_chunk_buffer_data == get_empty_buffer()) {
                b = (char *)malloc(sizeof(char));
                if (b) {
                    *b = '\0';
                }
            } else {
                b = m_chunk_buffer_data;
                init();
            }
            return b;
        }

        // add a static functions to be called to safely free this buffer from where it was allocated
        static void free(char *&b)
        {
            ::free((void *)b);
            b = nullptr;
        }

        // append    fn are in base_buffer
        // assign    fn are in base_buffer
        // insert    fn are in base_buffer
        // overwrite fn are in base_buffer
        // set       fn are in base_buffer
        // erase     fn are in base_buffer
        // compare   fn are in base_buffer
        // operators = are in base_buffer
        // operators+= are in base_buffer
        // operator []/at are in base_buffer

        // operators
        // =
        inline buffer &operator=(const buffer &o) noexcept
        {
            if (this != &o) {
                m_config = o.m_config;
                ensure_size(o.size(), true /*shrink*/);
                set(0 /*from*/, o.data(), o.size());
            }
            return *this;
        }
        // move operator
        inline buffer &operator=(buffer &&o) noexcept
        {
            if (this != &o) {
                clear_buffer();
                m_config                  = o.m_config;
                m_chunk_buffer_data       = o.m_chunk_buffer_data;
                m_chunk_buffer_length     = o.m_chunk_buffer_length;
                m_chunk_buffer_alloc_size = o.m_chunk_buffer_alloc_size;
                setup_buffer(m_chunk_buffer_data, m_chunk_buffer_length);
                o.init();
            }
            return *this;
        }

        // swap
        inline void swap(buffer &o) noexcept
        {
            std::swap(m_config, o.m_config);
            std::swap(m_chunk_buffer_length, o.m_chunk_buffer_length);
            std::swap(m_chunk_buffer_alloc_size, o.m_chunk_buffer_alloc_size);
            // swap buffer has 4 cases (due to empty buffer which is not static since there are only headers and no cpp)
            if (m_chunk_buffer_data != get_empty_buffer() && o.m_chunk_buffer_data != o.get_empty_buffer()) {
                std::swap(m_chunk_buffer_data, o.m_chunk_buffer_data);
            } else if (m_chunk_buffer_data == get_empty_buffer() && o.m_chunk_buffer_data == o.get_empty_buffer()) { /*do nothing*/
            } else if (m_chunk_buffer_data != get_empty_buffer() && o.m_chunk_buffer_data == o.get_empty_buffer()) {
                o.m_chunk_buffer_data = m_chunk_buffer_data;
                m_chunk_buffer_data   = (char *)(get_empty_buffer());
            } else if (m_chunk_buffer_data == get_empty_buffer() && o.m_chunk_buffer_data != o.get_empty_buffer()) {
                m_chunk_buffer_data   = o.m_chunk_buffer_data;
                o.m_chunk_buffer_data = (char *)o.get_empty_buffer();
            }
            setup_buffer(m_chunk_buffer_data, m_chunk_buffer_length);
            o.setup_buffer(o.m_chunk_buffer_data, o.m_chunk_buffer_length);
        }

    private:
        // init
        inline void init()
        {
            m_config.chunk_size       = std::max(m_config.chunk_size, std::size_t(1));
            m_chunk_buffer_data       = (char *)get_empty_buffer();
            m_chunk_buffer_length     = 0;
            m_chunk_buffer_alloc_size = 0;
            setup_buffer(m_chunk_buffer_data, m_chunk_buffer_length);
        }

        // free_chunk_buffer
        inline void free_chunk_buffer()
        {
            m_chunk_buffer_length     = 0;
            m_chunk_buffer_alloc_size = 0;
            if (m_chunk_buffer_data && (m_chunk_buffer_data != get_empty_buffer())) {
                buffer::free(m_chunk_buffer_data);
                m_chunk_buffer_data = (char *)get_empty_buffer();
            }
        }

        // ensure size returns new_length
        inline std::size_t ensure_size(std::size_t new_size, const bool shrink = false)
        {
            const auto chunk_size = m_config.chunk_size;
            // we always append a '\0' to the end so we can use as string
            std::size_t new_alloc_size = ((new_size + sizeof(char) /*for '\0'*/ + (chunk_size - 1)) / chunk_size) * chunk_size;
            bool        reallocate     = false;
            if (shrink) {
                reallocate = (m_chunk_buffer_alloc_size == 0) || (new_alloc_size != m_chunk_buffer_alloc_size); // we need another size
            } else {
                reallocate = (m_chunk_buffer_alloc_size == 0) || (new_alloc_size > m_chunk_buffer_alloc_size); // we need bigger size
            }

            // (re)allocate
            if (reallocate) {
                m_chunk_buffer_data       = (m_chunk_buffer_alloc_size == 0) ? (char *)malloc(new_alloc_size) : (char *)realloc(m_chunk_buffer_data, new_alloc_size);
                m_chunk_buffer_alloc_size = new_alloc_size;
            }

            // failure
            if (m_chunk_buffer_data == nullptr) {
                init();
                return 0;
            }
            // return new_length of the buffer
            m_chunk_buffer_data[new_size] = '\0';
            return new_size;
        }

        // !! override functions
        void clear_impl() override
        {
            m_chunk_buffer_length  = 0;
            m_chunk_buffer_data[0] = '\0';
            setup_buffer(m_chunk_buffer_data, m_chunk_buffer_length);
        }

        void reserve_impl(std::size_t new_size) override
        {
            ensure_size(new_size);
            setup_buffer(m_chunk_buffer_data, m_chunk_buffer_length);
        }

        void resize_impl(std::size_t new_size) override
        {
            m_chunk_buffer_length = ensure_size(new_size);
            setup_buffer(m_chunk_buffer_data, m_chunk_buffer_length);
        }

        void shrink_impl() override
        {
            m_chunk_buffer_length = ensure_size(size(), true /*shrink*/);
            setup_buffer(m_chunk_buffer_data, m_chunk_buffer_length);
        }

    protected:
        // other operators

        // clang-format off
        // +
        friend inline buffer operator+  (const buffer &b, const base_buffer &b2)    { buffer br = b; br.append(b2); return br; }
        friend inline buffer operator+  (const base_buffer &b2, const buffer &b)    { buffer br = b; br.append(b2); return br; }
        friend inline buffer operator+  (const buffer &b, const char c)             { buffer br = b; br.append(c);  return br; }
        friend inline buffer operator+  (const buffer &b, const char *s)            { buffer br = b; br.append(s);  return br; }
        friend inline buffer operator+  (const buffer &b, const std::string_view s) { buffer br = b; br.append(s);  return br; }
        friend inline buffer operator+  (const buffer &b, const std::vector<char> &v){buffer br = b; br.append(v);  return br; }

        // +
        friend inline buffer operator+  (const char c, const buffer &b)             { buffer br(b.get_chunk_size()); br.append(c); br += b; return br; }
        friend inline buffer operator+  (const char *s, const buffer &b)            { buffer br(b.get_chunk_size()); br.append(s); br += b; return br; }
        friend inline buffer operator+  (const std::string_view s, const buffer &b) { buffer br(b.get_chunk_size()); br.append(s); br += b; return br; }
        friend inline buffer operator+  (const std::vector<char> &v, const buffer &b){buffer br(b.get_chunk_size()); br.append(v); br += b; return br; }
        // clang-format on

    private:
        // chunk size
        config_buffer m_config{};
        // buffer use char* instead of vector<char> because it is much faster
        char       *m_chunk_buffer_data{};
        std::size_t m_chunk_buffer_length{};
        std::size_t m_chunk_buffer_alloc_size{};
    };

} // namespace small
