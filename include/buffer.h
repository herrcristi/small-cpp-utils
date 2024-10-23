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
// b.set( "ana", 3 );
// b.set( "b", 1, 2 );
//
// char* e = b.extract(); // extract "anb"
// free( e );
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
// free( e1 );
//
// b.append( "world", 5 );
// b.clear();
//
// std::string s64 = small::tobase64_s( "hello world", 11 );
// b.clear();
// small::frombase64( s64.c_str(), (int)s64.size(), &b );
// b = small::frombase64_b( s64 );
//
namespace small {
    const size_t default_buffer_chunk_size = 4096;

    // class for representing a buffer
    class buffer : public base_buffer
    {
    public:
        // buffer (allocates in chunks)
        buffer(size_t chunk_size = default_buffer_chunk_size) { init(chunk_size); }

        // from buffer
        buffer(const buffer &o) noexcept : buffer()
        {
            init(o.chunk_size_);
            operator=(o);
        }
        buffer(buffer &&o) noexcept : buffer()
        {
            init(o.chunk_size_);
            operator=(std::forward<buffer>(o));
        }

        // from char*
        buffer(const char c) noexcept : buffer() { base_buffer::operator=(c); }
        buffer(const char *s) noexcept : buffer() { base_buffer::operator=(s); }
        buffer(const char *s, size_t s_length) noexcept : buffer() { set(0 /*from*/, s, s_length); }
        buffer(const std::string &s) noexcept : buffer() { base_buffer::operator=(s); }
        buffer(const std::string_view s) noexcept : buffer() { base_buffer::operator=(s); }
        buffer(const std::vector<char> &v) noexcept : buffer() { base_buffer::operator=(v); }

        // from wchar_t*
        buffer(const wchar_t c) noexcept : buffer() { base_buffer::operator=(c); }
        buffer(const wchar_t *s) noexcept : buffer() { base_buffer::operator=(s); }
        buffer(const wchar_t *s, size_t s_length) noexcept : buffer() { set(0 /*from*/, s, s_length); }
        buffer(const std::wstring &s) noexcept : buffer() { base_buffer::operator=(s); }
        buffer(const std::wstring_view s) noexcept : buffer() { base_buffer::operator=(s); }
        buffer(const std::vector<wchar_t> &v) noexcept : buffer() { base_buffer::operator=(v); }

        // from char*
        buffer(size_t chunk_size, const char c) noexcept : buffer(chunk_size) { operator=(c); }
        buffer(size_t chunk_size, const char *s) noexcept : buffer(chunk_size) { operator=(s); }
        buffer(size_t chunk_size, const char *s, size_t s_length) noexcept : buffer(chunk_size) { set(0 /*from*/, s, s_length); }
        buffer(size_t chunk_size, const std::string &s) noexcept : buffer(chunk_size) { operator=(s); }
        buffer(size_t chunk_size, const std::string_view s) noexcept : buffer(chunk_size) { operator=(s); }
        buffer(size_t chunk_size, const std::vector<char> &v) noexcept : buffer(chunk_size) { operator=(v); }

        // from wchar_t*
        buffer(size_t chunk_size, const wchar_t c) noexcept : buffer(chunk_size) { operator=(c); }
        buffer(size_t chunk_size, const wchar_t *s) noexcept : buffer(chunk_size) { operator=(s); }
        buffer(size_t chunk_size, const wchar_t *s, size_t s_length) noexcept : buffer(chunk_size) { set(0 /*from*/, s, s_length); }
        buffer(size_t chunk_size, const std::wstring &s) noexcept : buffer(chunk_size) { operator=(s); }
        buffer(size_t chunk_size, const std::wstring_view s) noexcept : buffer(chunk_size) { operator=(s); }
        buffer(size_t chunk_size, const std::vector<wchar_t> &v) noexcept : buffer(chunk_size) { operator=(v); }

        // destructor
        ~buffer() { free_chunk_buffer(); }

        // chunk size
        inline size_t get_chunk_size() const { return chunk_size_; }
        inline void set_chunk_size(size_t chunk_size) { chunk_size_ = chunk_size; }

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
            if (chunk_buffer_data_ == nullptr || chunk_buffer_data_ == get_empty_buffer()) {
                b = (char *)malloc(sizeof(char));
                if (b)
                    *b = '\0';
            } else {
                b = chunk_buffer_data_;
                init(chunk_size_);
            }
            return b;
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
                chunk_size_ = o.chunk_size_;
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
                chunk_size_ = o.chunk_size_;
                chunk_buffer_data_ = o.chunk_buffer_data_;
                chunk_buffer_length_ = o.chunk_buffer_length_;
                chunk_buffer_alloc_size_ = o.chunk_buffer_alloc_size_;
                o.init(this->chunk_size_);
            }
            return *this;
        }

        // swap
        inline void swap(buffer &o)
        {
            std::swap(chunk_size_, o.chunk_size_);
            std::swap(chunk_buffer_length_, o.chunk_buffer_length_);
            std::swap(chunk_buffer_alloc_size_, o.chunk_buffer_alloc_size_);
            // swap buffer has 4 cases
            if (chunk_buffer_data_ != get_empty_buffer() && o.chunk_buffer_data_ != o.get_empty_buffer()) {
                std::swap(chunk_buffer_data_, o.chunk_buffer_data_);
            } else if (chunk_buffer_data_ == get_empty_buffer() && o.chunk_buffer_data_ == o.get_empty_buffer()) { /*do nothing*/
            } else if (chunk_buffer_data_ != get_empty_buffer() && o.chunk_buffer_data_ == o.get_empty_buffer()) {
                o.chunk_buffer_data_ = chunk_buffer_data_;
                chunk_buffer_data_ = (char *)get_empty_buffer();
            } else if (chunk_buffer_data_ == get_empty_buffer() && o.chunk_buffer_data_ != o.get_empty_buffer()) {
                chunk_buffer_data_ = o.chunk_buffer_data_;
                o.chunk_buffer_data_ = (char *)o.get_empty_buffer();
            }
        }

    private:
        // init
        inline void init(size_t chunk_size)
        {
            chunk_size_ = chunk_size;
            chunk_buffer_data_ = (char *)get_empty_buffer();
            chunk_buffer_length_ = 0;
            chunk_buffer_alloc_size_ = 0;
            setup_buffer(chunk_buffer_data_, chunk_buffer_length_);
        }

        // free_chunk_buffer
        inline void free_chunk_buffer()
        {
            chunk_buffer_length_ = 0;
            chunk_buffer_alloc_size_ = 0;
            if (chunk_buffer_data_ && (chunk_buffer_data_ != get_empty_buffer())) {
                free(chunk_buffer_data_);
                chunk_buffer_data_ = (char *)get_empty_buffer();
            }
        }

        // ensure size returns new_length
        inline size_t ensure_size(size_t new_size, const bool shrink = false)
        {
            // we always append a '\0' to the end so we can use as string
            size_t new_alloc_size = ((new_size + sizeof(char) /*for '\0'*/ + (chunk_size_ - 1)) / chunk_size_) * chunk_size_;
            bool reallocate = false;
            if (shrink) {
                reallocate = (chunk_buffer_alloc_size_ == 0) || (new_alloc_size != chunk_buffer_alloc_size_); // we need another size
            } else {
                reallocate = (chunk_buffer_alloc_size_ == 0) || (new_alloc_size > chunk_buffer_alloc_size_); // we need bigger size
            }

            // (re)allocate
            if (reallocate) {
                chunk_buffer_data_ = (chunk_buffer_alloc_size_ == 0) ? (char *)malloc(new_alloc_size) : (char *)realloc(chunk_buffer_data_, new_alloc_size);
                chunk_buffer_alloc_size_ = new_alloc_size;
            }

            // failure
            if (chunk_buffer_data_ == nullptr) {
                init(chunk_size_);
                return 0;
            }
            // return new_length of the buffer
            chunk_buffer_data_[new_size] = '\0';
            return new_size;
        }

        // !! override functions
        void clear_impl() override
        {
            chunk_buffer_length_ = 0;
            chunk_buffer_data_[0] = '\0';
            setup_buffer(chunk_buffer_data_, chunk_buffer_length_);
        }

        void reserve_impl(size_t new_size) override
        {
            ensure_size(new_size);
            setup_buffer(chunk_buffer_data_, chunk_buffer_length_);
        }

        void resize_impl(size_t new_size) override
        {
            chunk_buffer_length_ = ensure_size(new_size);
            setup_buffer(chunk_buffer_data_, chunk_buffer_length_);
        }

        void shrink_impl() override
        {
            chunk_buffer_length_ = ensure_size(size(), true /*shrink*/);
            setup_buffer(chunk_buffer_data_, chunk_buffer_length_);
        }

    private:
        // chunk size
        size_t chunk_size_;
        // buffer use char* instead of vector<char> because it is much faster
        char *chunk_buffer_data_;
        size_t chunk_buffer_length_;
        size_t chunk_buffer_alloc_size_;
    };

    // other operators

    // +
    inline buffer operator+(const buffer &b, const base_buffer &b2)
    {
        buffer br = b;
        br.append(b2.data(), b2.size());
        return br;
    }
    inline buffer operator+(buffer &&b, const buffer &b2)
    {
        buffer br(std::forward<buffer>(b));
        br.append(b2.data(), b2.size());
        return br;
    }
    inline buffer operator+(const base_buffer &b2, const buffer &b)
    {
        buffer br = b;
        br.append(b2.data(), b2.size());
        return br;
    }

    inline buffer operator+(const buffer &b, const char c)
    {
        buffer br = b;
        br.append(&c, 1);
        return br;
    }
    inline buffer operator+(const buffer &b, const char *s)
    {
        buffer br = b;
        br.append(s, strlen(s));
        return br;
    }
    inline buffer operator+(const buffer &b, const std::string &s)
    {
        buffer br = b;
        br.append(s.c_str(), s.size());
        return br;
    }
    inline buffer operator+(const buffer &b, const std::string_view s)
    {
        buffer br = b;
        br.append(s.data(), s.size());
        return br;
    }
    inline buffer operator+(const buffer &b, const std::vector<char> &v)
    {
        buffer br = b;
        br.append(v.data(), v.size());
        return br;
    }

    inline buffer operator+(const buffer &b, const wchar_t c)
    {
        buffer br = b;
        br.append(&c, 1);
        return br;
    }
    inline buffer operator+(const buffer &b, const wchar_t *s)
    {
        buffer br = b;
        br.append(s, wcslen(s));
        return br;
    }
    inline buffer operator+(const buffer &b, const std::wstring &s)
    {
        buffer br = b;
        br.append(s.c_str(), s.size());
        return br;
    }
    inline buffer operator+(const buffer &b, const std::wstring_view s)
    {
        buffer br = b;
        br.append(s.data(), s.size());
        return br;
    }
    inline buffer operator+(const buffer &b, const std::vector<wchar_t> &v)
    {
        buffer br = b;
        br.append(v.data(), v.size());
        return br;
    }

    // +
    inline buffer operator+(const char c, const buffer &b)
    {
        buffer br(b.get_chunk_size());
        br.append(&c, 1);
        br += b;
        return br;
    }
    inline buffer operator+(const char *s, const buffer &b)
    {
        buffer br(b.get_chunk_size());
        br.append(s, strlen(s));
        br += b;
        return br;
    }
    inline buffer operator+(const std::string &s, const buffer &b)
    {
        buffer br(b.get_chunk_size());
        br.append(s.c_str(), s.size());
        br += b;
        return br;
    }
    inline buffer operator+(const std::string_view s, const buffer &b)
    {
        buffer br(b.get_chunk_size());
        br.append(s.data(), s.size());
        br += b;
        return br;
    }
    inline buffer operator+(const std::vector<char> &v, const buffer &b)
    {
        buffer br(b.get_chunk_size());
        br.append(v.data(), v.size());
        br += b;
        return br;
    }

    inline buffer operator+(const wchar_t c, const buffer &b)
    {
        buffer br(b.get_chunk_size());
        br.append(&c, 1);
        br += b;
        return br;
    }
    inline buffer operator+(const wchar_t *s, const buffer &b)
    {
        buffer br(b.get_chunk_size());
        br.append(s, wcslen(s));
        br += b;
        return br;
    }
    inline buffer operator+(const std::wstring &s, const buffer &b)
    {
        buffer br(b.get_chunk_size());
        br.append(s.c_str(), s.size());
        br += b;
        return br;
    }
    inline buffer operator+(const std::wstring_view s, const buffer &b)
    {
        buffer br(b.get_chunk_size());
        br.append(s.data(), s.size());
        br += b;
        return br;
    }
    inline buffer operator+(const std::vector<wchar_t> &v, const buffer &b)
    {
        buffer br(b.get_chunk_size());
        br.append(v.data(), v.size());
        br += b;
        return br;
    }

} // namespace small
