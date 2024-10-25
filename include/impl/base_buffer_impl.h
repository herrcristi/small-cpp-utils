#pragma once

#include <stddef.h>
#include <string.h>

#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace small {
    // class for representing a base_buffer that implements
    // all the needed functions and operators
    // it must be supplied with derived class with proper functions
    class base_buffer
    {
    protected:
        // base_buffer (allocates in chunks)
        base_buffer() = default;
        base_buffer(const base_buffer &) = delete;
        base_buffer(base_buffer &&) = delete;
        virtual ~base_buffer() = default;

    public:
        // clang-format off
        // functions for getting size size / length / empty
        inline std::size_t  size            () const    {   return m_buffer_length; }
        inline std::size_t  length          () const    {   return size();          }
        inline bool         empty           () const    {   return size() == 0;     }
        // clang-format on

        // clang-format off
        // functions to supply in the derived class that affect size
        // clear / reserve / resize / shrink_to_fit
        inline void         clear           ()                      { this->clear_impl();           }
        inline void         reserve         (std::size_t new_size)  { this->reserve_impl(new_size); }
        inline void         resize          (std::size_t new_size)  { this->resize_impl(new_size);  }
        inline void         shrink_to_fit   ()                      { this->shrink_impl();          }
        // clang-format on

        // clang-format off
        // data access to buffer
        inline const char * get_buffer      () const    { return m_buffer_data; }
        inline char *       get_buffer      ()          { return m_buffer_data; } // direct access

        inline const char * data            () const    { return m_buffer_data; }
        inline char *       data            ()          { return m_buffer_data; } // direct access
        
        inline const char * begin           () const noexcept { return data(); }
        inline const char * end             () const noexcept { return data() + size(); }
        inline const char * cbegin          () const noexcept { return data(); }
        inline const char * cend            () const noexcept { return data() + size(); }
        
        inline const char * rbegin          () const noexcept { return end(); }
        inline const char * rend            () const noexcept { return begin(); }
        inline const char * crbegin         () const noexcept { return end(); }
        inline const char * crend           () const noexcept { return begin(); }
        // clang-format on

        // clang-format off
        // conversion as c_string
        inline std::string          c_string() const    { return std::string(data(), size()); }
        inline std::string_view     c_view  () const    { return std::string_view{data(), size()}; }
        inline std::vector<char>    c_vector() const
        {
            // vector is not null terminated, handle with care
            std::vector<char> v;
            v.reserve(size() + 1);
            v.resize(size());
            memcpy(v.data(), data(), size());
            return v;
        }
        // clang-format on

        // clang-format off
        // assign
        inline void assign(const base_buffer &b)
        {
            if (this != &b) {
                set(0 /*startfrom*/, b.data(), b.size());
            }
        }

        inline void     assign          (const char c)                                      { set(0 /*startfrom*/, &c, 1); }
        inline void     assign          (const char *s)                                     { set(0 /*startfrom*/, s, strlen(s)); }
        inline void     assign          (const char *s, std::size_t len)                    { set(0 /*startfrom*/, s, len); }
        inline void     assign          (const std::string_view s)                          { set(0 /*startfrom*/, s.data(), s.size()); }
        inline void     assign          (const std::vector<char> &v)                        { set(0 /*startfrom*/, v.data(), v.size()); }
        // clang-format on

        // clang-format off
        // append
        inline void     append          (const base_buffer &b)                              { set(size() /*startfrom*/, b.data(), b.size()); }

        inline void     append          (const char c)                                      { set(size() /*startfrom*/, &c, 1); }
        inline void     append          (const char *s)                                     { set(size() /*startfrom*/, s, strlen(s)); }
        inline void     append          (const char *s, std::size_t len)                    { set(size() /*startfrom*/, s, len); }
        inline void     append          (const std::string_view s)                          { set(size() /*startfrom*/, s.data(), s.size()); }
        inline void     append          (const std::vector<char> &v)                        { set(size() /*startfrom*/, v.data(), v.size()); }
        // clang-format on

        // clang-format off
        // insert
        inline void     insert          (std::size_t from, const base_buffer &b)            { this->insert_impl(from, b.data(), b.size()); }

        inline void     insert          (std::size_t from, const char c)                    { this->insert_impl(from, &c, 1); }
        inline void     insert          (std::size_t from, const char *s)                   { this->insert_impl(from, s, strlen(s)); }
        inline void     insert          (std::size_t from, const char *s, std::size_t len)  { this->insert_impl(from, s, len); }
        inline void     insert          (std::size_t from, const std::string_view s)        { this->insert_impl(from, s.data(), s.size()); }
        inline void     insert          (std::size_t from, const std::vector<char> &v)      { this->insert_impl(from, v.data(), v.size()); }
        // clang-format on

        // clang-format off
        // overwrite
        inline void     overwrite       (std::size_t from, const base_buffer &b)            { set(from, b.data(), b.size()); }

        inline void     overwrite       (std::size_t from, const char c)                    { set(from, &c, 1); }
        inline void     overwrite       (std::size_t from, const char *s)                   { set(from, s, strlen(s)); }
        inline void     overwrite       (std::size_t from, const char *s, std::size_t len)  { set(from, s, len); }
        inline void     overwrite       (std::size_t from, const std::string_view s)        { set(from, s.data(), s.size()); }
        inline void     overwrite       (std::size_t from, const std::vector<char> &v)      { set(from, v.data(), v.size()); }
        // clang-format on

        // clang-format off
        // set
        inline void     set             (std::size_t from, const base_buffer &b)            { this->set_impl(from, b.data(), b.size()); }

        inline void     set             (std::size_t from, const char c)                    { this->set_impl(from, &c, 1); }
        inline void     set             (std::size_t from, const char *s)                   { this->set_impl(from, s, strlen(s)); }
        inline void     set             (std::size_t from, const char *s, std::size_t len)  { this->set_impl(from, s, len); }
        inline void     set             (std::size_t from, const std::string_view s)        { this->set_impl(from, s.data(), s.size()); }
        inline void     set             (std::size_t from, const std::vector<char> &v)      { this->set_impl(from, v.data(), v.size()); }
        // clang-format on

        // erase
        inline void erase(std::size_t from)
        {
            if (from < size()) {
                resize(from);
            }
        }
        inline void erase(std::size_t from, std::size_t length)
        {
            this->erase_impl(from, length);
        }

        // compare
        inline bool is_equal(const char *s, std::size_t s_length) const
        {
            return size() == s_length && compare(s, s_length) == 0;
        }

        inline int compare(const char *s, std::size_t s_length) const
        {
            const std::size_t this_size = size();
            int cmp = memcmp(data(), s, std::min(this_size, s_length));

            if (cmp != 0) {
                // different
                return cmp;
            }

            // equal so far
            if (this_size == s_length) {
                return 0; // true equal
            }

            return this_size < s_length ? -1 : 1;
        }

        //
        // operators
        //

        // clang-format off
        // these specific operators must be implemented in derived classes
        inline base_buffer &operator=   (const base_buffer &o) = delete;
        inline base_buffer &operator=   (base_buffer &&o) = delete;
        // =
        inline base_buffer &operator=   (const char c) noexcept             { set(0 /*from*/, &c, 1);               return *this; }
        inline base_buffer &operator=   (const char *s) noexcept            { set(0 /*from*/, s, strlen(s));        return *this; }
        inline base_buffer &operator=   (const std::string_view s) noexcept { set(0 /*from*/, s.data(), s.size());  return *this; }
        inline base_buffer &operator=   (const std::vector<char> &v) noexcept{set(0 /*from*/, v.data(), v.size());  return *this; }
        // clang-format on

        // clang-format off
        // +=
        inline base_buffer &operator+=  (const base_buffer &b) noexcept     { append(b.data(), b.size());           return *this; }

        inline base_buffer &operator+=  (const char c) noexcept             { append(&c, 1);                        return *this; }
        inline base_buffer &operator+=  (const char *s) noexcept            { append(s, strlen(s));                 return *this; }
        inline base_buffer &operator+=  (const std::string_view s) noexcept { append(s.data(), s.size());           return *this; }
        inline base_buffer &operator+=  (const std::vector<char> &v) noexcept{append(v.data(), v.size());           return *this; }
        // clang-format on

        // clang-format off
        // [] / at
        inline char &       operator[]  (std::size_t index)         { return m_buffer_data[index]; }
        inline char         operator[]  (std::size_t index) const   { return m_buffer_data[index]; }

        inline char &       at          (std::size_t index)         { return m_buffer_data[index]; }
        inline char         at          (std::size_t index) const   { return m_buffer_data[index]; }

        // front / back
        inline char &       front       ()                          { return m_buffer_data[0]; }
        inline char         front       () const                    { return m_buffer_data[0]; }

        inline char &       back        ()                          { return size() > 0 ? m_buffer_data[size() - 1] : m_buffer_data[0]; }
        inline char         back        () const                    { return size() > 0 ? m_buffer_data[size() - 1] : m_buffer_data[0]; }

        // push / pop
        inline void         push_back   (const char c)              { append(c); }
        inline void         pop_back    ()  {
            if (size() > 0) {
                resize(size() - 1);
            }
        }
        // clang-format on

        // operator
        inline operator std::string_view() const { return c_view(); }

    protected:
        // empty buffer
        inline const char *get_empty_buffer() const
        {
            return m_empty_buffer;
        }

        // !! after every function call setup buffer data
        inline void setup_buffer(char *buffer_data, std::size_t buffer_length)
        {
            m_buffer_data = buffer_data;
            m_buffer_length = buffer_length;
        }

        // clang-format off
        // !! functions to override
        virtual void clear_impl     () = 0;
        virtual void reserve_impl   (std::size_t /*size*/) = 0;
        virtual void resize_impl    (std::size_t /*size*/) = 0;
        virtual void shrink_impl    () = 0;

        virtual void set_impl       (std::size_t from, const char *buffer, std::size_t length) { buffer_set_impl(from, buffer, length); }

        virtual void insert_impl    (std::size_t from, const char *buffer, std::size_t length) { buffer_insert_impl(from, buffer, length); }

        virtual void erase_impl     (std::size_t from, std::size_t length) { buffer_erase_impl(from, length); }
        // clang-format on

    protected:
        // buffer set
        inline void buffer_set_impl(std::size_t from, const char *b, std::size_t b_length)
        {
            resize(from + b_length); // make room

            if (b && m_buffer_data && m_buffer_data != m_empty_buffer /*good allocation*/) {
                memcpy(m_buffer_data + from, b, b_length);
            }
        }

        // buffer insert
        inline void buffer_insert_impl(std::size_t from /*= 0*/, const char *b, std::size_t b_length)
        {
            std::size_t initial_length = size();
            resize(from <= initial_length ? initial_length + b_length : from + b_length);

            if (b && m_buffer_data && m_buffer_data != m_empty_buffer /*good allocation*/) {
                if (from <= initial_length) {
                    memmove(m_buffer_data + from + b_length, m_buffer_data + from, initial_length - from);
                } else {
                    memset(m_buffer_data + initial_length, '\0', (from - initial_length));
                }
                memcpy(m_buffer_data + from, b, b_length);
            }
        }

        // erase
        inline void buffer_erase_impl(std::size_t from, std::size_t length)
        {
            if (m_buffer_data && m_buffer_data != m_empty_buffer) {
                if (from < size()) {
                    if (from + length < size()) {
                        std::size_t move_length = size() - (from + length);
                        memmove(m_buffer_data + from, m_buffer_data + from + length, move_length);
                        resize(size() - length);
                    } else {
                        resize(from);
                    }
                }
            }
        }

        // other operators

        // clang-format off
        // + must be defined in derived classes

        // ==
        friend inline bool operator==(const base_buffer &b, const base_buffer &b2)      { return b.is_equal(b2.data(), b2.size()); }

        friend inline bool operator==(const base_buffer &b, const char c)               { return b.is_equal(&c, 1); }
        friend inline bool operator==(const base_buffer &b, const char *s)              { return b.is_equal(s, strlen(s)); }
        friend inline bool operator==(const base_buffer &b, const std::string_view s)   { return b.is_equal(s.data(), s.size()); }
        friend inline bool operator==(const base_buffer &b, const std::vector<char> &v) { return b.is_equal(v.data(), v.size()); }

        friend inline bool operator==(const char c, const base_buffer &b)               { return b.is_equal(&c, 1); }
        friend inline bool operator==(const char *s, const base_buffer &b)              { return b.is_equal(s, strlen(s)); }
        friend inline bool operator==(const std::string_view s, const base_buffer &b)   { return b.is_equal(s.data(), s.size()); }
        friend inline bool operator==(const std::vector<char> &v, const base_buffer &b) { return b.is_equal(v.data(), v.size()); }

        // !=
        friend inline bool operator!=(const base_buffer &b, const base_buffer &b2)      { return !b.is_equal(b2.data(), b2.size()); }

        friend inline bool operator!=(const base_buffer &b, const char c)               { return !b.is_equal(&c, 1); }
        friend inline bool operator!=(const base_buffer &b, const char *s)              { return !b.is_equal(s, strlen(s)); }
        friend inline bool operator!=(const base_buffer &b, const std::string_view s)   { return !b.is_equal(s.data(), s.size()); }
        friend inline bool operator!=(const base_buffer &b, const std::vector<char> &v) { return !b.is_equal(v.data(), v.size()); }

        friend inline bool operator!=(const char c, const base_buffer &b)               { return !b.is_equal(&c, 1); }
        friend inline bool operator!=(const char *s, const base_buffer &b)              { return !b.is_equal(s, strlen(s)); }
        friend inline bool operator!=(const std::string_view s, const base_buffer &b)   { return !b.is_equal(s.data(), s.size()); }
        friend inline bool operator!=(const std::vector<char> &v, const base_buffer &b) { return !b.is_equal(v.data(), v.size()); }

        // <
        friend inline bool operator<(const base_buffer &b, const base_buffer &b2)       { return b.compare(b2.data(), b2.size()) < 0; }

        friend inline bool operator<(const base_buffer &b, const char c)                { return b.compare(&c, 1) < 0; }
        friend inline bool operator<(const base_buffer &b, const char *s)               { return b.compare(s, strlen(s)) < 0; }
        friend inline bool operator<(const base_buffer &b, const std::string_view s)    { return b.compare(s.data(), s.size()) < 0; }
        friend inline bool operator<(const base_buffer &b, const std::vector<char> &v)  { return b.compare(v.data(), v.size()) < 0; }

        friend inline bool operator<(const char c, const base_buffer &b)                { return b.compare(&c, 1) >= 0; }
        friend inline bool operator<(const char *s, const base_buffer &b)               { return b.compare(s, strlen(s)) >= 0; }
        friend inline bool operator<(const std::string_view s, const base_buffer &b)    { return b.compare(s.data(), s.size()) >= 0; }
        friend inline bool operator<(const std::vector<char> &v, const base_buffer &b)  { return b.compare(v.data(), v.size()) >= 0; }

        // <=
        friend inline bool operator<=(const base_buffer &b, const base_buffer &b2)      { return b.compare(b2.data(), b2.size()) <= 0; }

        friend inline bool operator<=(const base_buffer &b, const char c)               { return b.compare(&c, 1) <= 0; }
        friend inline bool operator<=(const base_buffer &b, const char *s)              { return b.compare(s, strlen(s)) <= 0; }
        friend inline bool operator<=(const base_buffer &b, const std::string_view s)   { return b.compare(s.data(), s.size()) <= 0; }
        friend inline bool operator<=(const base_buffer &b, const std::vector<char> &v) { return b.compare(v.data(), v.size()) <= 0; }

        friend inline bool operator<=(const char c, const base_buffer &b)               { return b.compare(&c, 1) > 0; }
        friend inline bool operator<=(const char *s, const base_buffer &b)              { return b.compare(s, strlen(s)) > 0; }
        friend inline bool operator<=(const std::string_view s, const base_buffer &b)   { return b.compare(s.data(), s.size()) > 0; }
        friend inline bool operator<=(const std::vector<char> &v, const base_buffer &b) { return b.compare(v.data(), v.size()) > 0; }

        // >
        friend inline bool operator>(const base_buffer &b, const base_buffer &b2)       { return b.compare(b2.data(), b2.size()) > 0; }

        friend inline bool operator>(const base_buffer &b, const char c)                { return b.compare(&c, 1) > 0; }
        friend inline bool operator>(const base_buffer &b, const char *s)               { return b.compare(s, strlen(s)) > 0; }
        friend inline bool operator>(const base_buffer &b, const std::string_view s)    { return b.compare(s.data(), s.size()) > 0; }
        friend inline bool operator>(const base_buffer &b, const std::vector<char> &v)  { return b.compare(v.data(), v.size()) > 0; }

        friend inline bool operator>(const char c, const base_buffer &b)                { return b.compare(&c, 1) <= 0; }
        friend inline bool operator>(const char *s, const base_buffer &b)               { return b.compare(s, strlen(s)) <= 0; }
        friend inline bool operator>(const std::string_view s, const base_buffer &b)    { return b.compare(s.data(), s.size()) <= 0; }
        friend inline bool operator>(const std::vector<char> &v, const base_buffer &b)  { return b.compare(v.data(), v.size()) <= 0; }

        // >=
        friend inline bool operator>=(const base_buffer &b, const base_buffer &b2)      { return b.compare(b2.data(), b2.size()) >= 0; }

        friend inline bool operator>=(const base_buffer &b, const char c)               { return b.compare(&c, 1) >= 0; }
        friend inline bool operator>=(const base_buffer &b, const char *s)              { return b.compare(s, strlen(s)) >= 0; }
        friend inline bool operator>=(const base_buffer &b, const std::string_view s)   { return b.compare(s.data(), s.size()) >= 0; }
        friend inline bool operator>=(const base_buffer &b, const std::vector<char> &v) { return b.compare(v.data(), v.size()) >= 0; }

        friend inline bool operator>=(const char c, const base_buffer &b)               { return b.compare(&c, 1) < 0; }
        friend inline bool operator>=(const char *s, const base_buffer &b)              { return b.compare(s, strlen(s)) < 0; }
        friend inline bool operator>=(const std::string_view s, const base_buffer &b)   { return b.compare(s.data(), s.size()) < 0; }
        friend inline bool operator>=(const std::vector<char> &v, const base_buffer &b) { return b.compare(v.data(), v.size()) < 0; }

        // out
        friend inline std::ostream &operator<<(std::ostream &out, const base_buffer &b)
        {
            out << b.c_view();
            return out;
        }
        // clang-format on

    private:
        // base_buffer empty
        char m_empty_buffer[1]{'\0'};
        // base_buffer use char* instead of vector<char> because it is much faster
        char *m_buffer_data{nullptr};
        std::size_t m_buffer_length{0};
    };

} // namespace small
