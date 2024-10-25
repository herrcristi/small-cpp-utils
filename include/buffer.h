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
// b.append( "b", 1 );
//
// char* e = b.extract(); // extract "anb"
// Buffer::free( e );
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
// Buffer::free( e1 );
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
    const std::size_t DEFAULT_BUFFER_CHUNK_SIZE = 8192;

    // class for representing a buffer
    class buffer : public base_buffer
    {
    public:
        // buffer (allocates in chunks)
        explicit buffer(std::size_t chunk_size = DEFAULT_BUFFER_CHUNK_SIZE)
        {
            init(chunk_size);
        }

        // from buffer
        buffer(const buffer &o) noexcept : buffer()
        {
            init(o.m_chunk_size);
            operator=(o);
        }
        buffer(buffer &&o) noexcept : buffer()
        {
            init(o.m_chunk_size);
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
        buffer(std::size_t chunk_size, const char c) noexcept                          : buffer(chunk_size) { base_buffer::operator=(c); }
        buffer(std::size_t chunk_size, const char *s) noexcept                         : buffer(chunk_size) { base_buffer::operator=(s); }
        buffer(std::size_t chunk_size, const char *s, std::size_t s_length) noexcept   : buffer(chunk_size) { set(0 /*from*/, s, s_length); }
        buffer(std::size_t chunk_size, const std::string &s) noexcept                  : buffer(chunk_size) { base_buffer::operator=(s); }
        buffer(std::size_t chunk_size, const std::string_view s) noexcept              : buffer(chunk_size) { base_buffer::operator=(s); }
        buffer(std::size_t chunk_size, const std::vector<char> &v) noexcept            : buffer(chunk_size) { base_buffer::operator=(v); }
        // clang-format on

        // destructor
        ~buffer() override
        {
            free_chunk_buffer();
        }

        // chunk size
        inline std::size_t get_chunk_size() const
        {
            return m_chunk_size;
        }
        inline void set_chunk_size(std::size_t chunk_size)
        {
            m_chunk_size = chunk_size;
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
                init(m_chunk_size);
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
                m_chunk_size = o.m_chunk_size;
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
                m_chunk_size = o.m_chunk_size;
                m_chunk_buffer_data = o.m_chunk_buffer_data;
                m_chunk_buffer_length = o.m_chunk_buffer_length;
                m_chunk_buffer_alloc_size = o.m_chunk_buffer_alloc_size;
                o.init(this->m_chunk_size);
            }
            return *this;
        }

        // swap
        inline void swap(buffer &o) noexcept
        {
            std::swap(m_chunk_size, o.m_chunk_size);
            std::swap(m_chunk_buffer_length, o.m_chunk_buffer_length);
            std::swap(m_chunk_buffer_alloc_size, o.m_chunk_buffer_alloc_size);
            // swap buffer has 4 cases (due to empty buffer which is not static since there are only headers and no cpp)
            if (m_chunk_buffer_data != get_empty_buffer() && o.m_chunk_buffer_data != o.get_empty_buffer()) {
                std::swap(m_chunk_buffer_data, o.m_chunk_buffer_data);
            } else if (m_chunk_buffer_data == get_empty_buffer() && o.m_chunk_buffer_data == o.get_empty_buffer()) { /*do nothing*/
            } else if (m_chunk_buffer_data != get_empty_buffer() && o.m_chunk_buffer_data == o.get_empty_buffer()) {
                o.m_chunk_buffer_data = m_chunk_buffer_data;
                m_chunk_buffer_data = (char *)(get_empty_buffer());
            } else if (m_chunk_buffer_data == get_empty_buffer() && o.m_chunk_buffer_data != o.get_empty_buffer()) {
                m_chunk_buffer_data = o.m_chunk_buffer_data;
                o.m_chunk_buffer_data = (char *)o.get_empty_buffer();
            }
        }

        //
        // substr
        //
        inline buffer substr(std::size_t __pos = 0, std::size_t __n = std::string::npos) const noexcept(false)
        {
            return buffer(c_view().substr(__pos, __n));
        }

        // starts_with
        inline bool starts_with(std::string_view __x) const noexcept
        {
            return this->substr(0, __x.size()) == __x;
        }

        inline bool starts_with(char __x) const noexcept
        {
            return !this->empty() && (this->front() == __x);
        }

        inline bool starts_with(const char *__x) const noexcept
        {
            return this->starts_with(std::string_view(__x));
        }

        // ends_with
        inline bool ends_with(std::string_view __x) const noexcept
        {
            const auto __len = this->size();
            const auto __xlen = __x.size();
            return __len >= __xlen && memcmp(this->data() + this->size() - __xlen, __x.data(), __xlen) == 0;
        }

        inline bool ends_with(char __x) const noexcept
        {
            return !this->empty() && (this->back() == __x);
        }

        inline bool ends_with(const char *__x) const noexcept
        {
            return this->ends_with(std::string_view(__x));
        }

        // contains
        inline bool contains(std::string_view __x) const noexcept
        {
            return this->find(__x) != std::string::npos;
        }

        inline bool contains(char __x) const noexcept
        {
            return this->find(__x) != std::string::npos;
        }

        inline bool contains(const char *__x) const noexcept
        {
            return this->find(__x) != std::string::npos;
        }

        // find
        inline std::size_t find(std::string_view __str, std::size_t __pos = 0) const noexcept
        {
            return c_view().find(__str, __pos);
        }

        inline std::size_t find(char __c, std::size_t __pos = 0) const noexcept
        {
            return c_view().find(__c, __pos);
        }

        inline std::size_t find(const char *__str, std::size_t __pos, std::size_t __n) const noexcept
        {
            return c_view().find(__str, __pos, __n);
        }

        inline std::size_t find(const char *__str, std::size_t __pos = 0) const noexcept
        {
            return c_view().find(__str, __pos);
        }

        // rfind
        inline std::size_t rfind(std::string_view __str, std::size_t __pos = std::string::npos) const noexcept
        {
            return c_view().rfind(__str, __pos);
        }

        inline std::size_t rfind(char __c, std::size_t __pos = std::string::npos) const noexcept
        {
            return c_view().rfind(__c, __pos);
        }

        inline std::size_t rfind(const char *__str, std::size_t __pos, std::size_t __n) const noexcept
        {
            return c_view().rfind(__str, __pos, __n);
        }

        inline std::size_t rfind(const char *__str, std::size_t __pos = std::string::npos) const noexcept
        {
            return c_view().rfind(__str, __pos);
        }

        // find_first_of
        inline std::size_t find_first_of(std::string_view __str, std::size_t __pos = 0) const noexcept
        {
            return c_view().find_first_of(__str, __pos);
        }

        inline std::size_t find_first_of(char __c, std::size_t __pos = 0) const noexcept
        {
            return c_view().find_first_of(__c, __pos);
        }

        inline std::size_t find_first_of(const char *__str, std::size_t __pos, std::size_t __n) const noexcept
        {
            return c_view().find_first_of(__str, __pos, __n);
        }

        inline std::size_t find_first_of(const char *__str, std::size_t __pos = 0) const noexcept
        {
            return c_view().find_first_of(__str, __pos);
        }

        // find_last_of
        inline std::size_t find_last_of(std::string_view __str, std::size_t __pos = std::string::npos) const noexcept
        {
            return c_view().find_last_of(__str, __pos);
        }

        inline std::size_t find_last_of(char __c, std::size_t __pos = std::string::npos) const noexcept
        {
            return c_view().find_last_of(__c, __pos);
        }

        inline std::size_t find_last_of(const char *__str, std::size_t __pos, std::size_t __n) const noexcept
        {
            return c_view().find_last_of(__str, __pos, __n);
        }

        inline std::size_t find_last_of(const char *__str, std::size_t __pos = std::string::npos) const noexcept
        {
            return c_view().find_last_of(__str, __pos);
        }

        // find_first_not_of
        inline std::size_t find_first_not_of(std::string_view __str, std::size_t __pos = 0) const noexcept
        {
            return c_view().find_first_not_of(__str, __pos);
        }

        inline std::size_t find_first_not_of(char __c, std::size_t __pos = 0) const noexcept
        {
            return c_view().find_first_not_of(__c, __pos);
        }

        inline std::size_t find_first_not_of(const char *__str, std::size_t __pos, std::size_t __n) const noexcept
        {
            return c_view().find_first_not_of(__str, __pos, __n);
        }

        inline std::size_t find_first_not_of(const char *__str, std::size_t __pos = 0) const noexcept
        {
            return c_view().find_first_not_of(__str, __pos);
        }

        // find_last_not_of
        inline std::size_t find_last_not_of(std::string_view __str, std::size_t __pos = std::string::npos) const noexcept
        {
            return c_view().find_last_not_of(__str, __pos);
        }

        inline std::size_t find_last_not_of(char __c, std::size_t __pos = std::string::npos) const noexcept
        {
            return c_view().find_last_not_of(__c, __pos);
        }

        inline std::size_t find_last_not_of(const char *__str, std::size_t __pos, std::size_t __n) const noexcept
        {
            return c_view().find_last_not_of(__str, __pos, __n);
        }

        inline std::size_t find_last_not_of(const char *__str, std::size_t __pos = std::string::npos) const noexcept
        {
            return c_view().find_last_not_of(__str, __pos);
        }

    private:
        // init
        inline void init(std::size_t chunk_size)
        {
            m_chunk_size = chunk_size;
            m_chunk_buffer_data = (char *)get_empty_buffer();
            m_chunk_buffer_length = 0;
            m_chunk_buffer_alloc_size = 0;
            setup_buffer(m_chunk_buffer_data, m_chunk_buffer_length);
        }

        // free_chunk_buffer
        inline void free_chunk_buffer()
        {
            m_chunk_buffer_length = 0;
            m_chunk_buffer_alloc_size = 0;
            if (m_chunk_buffer_data && (m_chunk_buffer_data != get_empty_buffer())) {
                buffer::free(m_chunk_buffer_data);
                m_chunk_buffer_data = (char *)get_empty_buffer();
            }
        }

        // ensure size returns new_length
        inline std::size_t ensure_size(std::size_t new_size, const bool shrink = false)
        {
            // we always append a '\0' to the end so we can use as string
            std::size_t new_alloc_size = ((new_size + sizeof(char) /*for '\0'*/ + (m_chunk_size - 1)) / m_chunk_size) * m_chunk_size;
            bool reallocate = false;
            if (shrink) {
                reallocate = (m_chunk_buffer_alloc_size == 0) || (new_alloc_size != m_chunk_buffer_alloc_size); // we need another size
            } else {
                reallocate = (m_chunk_buffer_alloc_size == 0) || (new_alloc_size > m_chunk_buffer_alloc_size); // we need bigger size
            }

            // (re)allocate
            if (reallocate) {
                m_chunk_buffer_data = (m_chunk_buffer_alloc_size == 0) ? (char *)malloc(new_alloc_size) : (char *)realloc(m_chunk_buffer_data, new_alloc_size);
                m_chunk_buffer_alloc_size = new_alloc_size;
            }

            // failure
            if (m_chunk_buffer_data == nullptr) {
                init(m_chunk_size);
                return 0;
            }
            // return new_length of the buffer
            m_chunk_buffer_data[new_size] = '\0';
            return new_size;
        }

        // !! override functions
        void clear_impl() override
        {
            m_chunk_buffer_length = 0;
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

        // +
        friend inline buffer operator+(const buffer &b, const base_buffer &b2)
        {
            buffer br = b;
            br.append(b2.data(), b2.size());
            return br;
        }
        friend inline buffer operator+(buffer &&b, const base_buffer &b2)
        {
            buffer br(std::forward<buffer>(b));
            br.append(b2.data(), b2.size());
            return br;
        }
        friend inline buffer operator+(const base_buffer &b2, const buffer &b)
        {
            buffer br = b;
            br.append(b2.data(), b2.size());
            return br;
        }

        friend inline buffer operator+(const buffer &b, const char c)
        {
            buffer br = b;
            br.append(&c, 1);
            return br;
        }
        friend inline buffer operator+(const buffer &b, const char *s)
        {
            buffer br = b;
            br.append(s, strlen(s));
            return br;
        }
        friend inline buffer operator+(const buffer &b, const std::string_view s)
        {
            buffer br = b;
            br.append(s.data(), s.size());
            return br;
        }
        friend inline buffer operator+(const buffer &b, const std::vector<char> &v)
        {
            buffer br = b;
            br.append(v.data(), v.size());
            return br;
        }

        // +
        friend inline buffer operator+(const char c, const buffer &b)
        {
            buffer br(b.get_chunk_size());
            br.append(&c, 1);
            br += b;
            return br;
        }
        friend inline buffer operator+(const char *s, const buffer &b)
        {
            buffer br(b.get_chunk_size());
            br.append(s, strlen(s));
            br += b;
            return br;
        }
        friend inline buffer operator+(const std::string_view s, const buffer &b)
        {
            buffer br(b.get_chunk_size());
            br.append(s.data(), s.size());
            br += b;
            return br;
        }
        friend inline buffer operator+(const std::vector<char> &v, const buffer &b)
        {
            buffer br(b.get_chunk_size());
            br.append(v.data(), v.size());
            br += b;
            return br;
        }

    private:
        // chunk size
        std::size_t m_chunk_size;
        // buffer use char* instead of vector<char> because it is much faster
        char *m_chunk_buffer_data;
        std::size_t m_chunk_buffer_length;
        std::size_t m_chunk_buffer_alloc_size;
    };

} // namespace small
