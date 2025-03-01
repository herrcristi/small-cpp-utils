#pragma once

#include <stdlib.h>

#include <array>
#include <clocale>
#include <cwchar>
#include <string>
#include <string_view>
#include <vector>

//
// small::stack_string<256/*on stack*/> s;
// ...
//

namespace small {
    //
    // a string class that allocates on stack
    //
    template <std::size_t StackAllocSize = 256>
    class stack_string
    {
    public:
        // clang-format off
        stack_string() { init(); }

        stack_string(const char c) noexcept                             : stack_string() { operator=(c); }
        stack_string(const char* s, const size_t& s_length) noexcept    : stack_string() { set(0 /*from*/, s, s_length); }
        stack_string(const std::string_view s) noexcept                 : stack_string() { operator=(s); }
        stack_string(const std::vector<char>& v) noexcept               : stack_string() { operator=(v); }
        stack_string(const std::string& s) noexcept                     : stack_string(std::string_view{s}) {}
        
        stack_string(const wchar_t c) noexcept                          : stack_string() { operator=(c); }
        stack_string(const wchar_t *s, const size_t& s_length ) noexcept: stack_string() { set(0 /*from*/, s, s_length); }
        stack_string(const std::wstring_view s) noexcept                : stack_string() { operator=(s); }
        stack_string(const std::wstring& s) noexcept                    : stack_string(std::wstring_view{s}) {}

        stack_string(const stack_string& o) noexcept                    : stack_string() { operator=(o); }
        stack_string(stack_string&& o) noexcept                         : stack_string() { operator=(std::forward<stack_string>(o)); }
        
        // destructor
        ~stack_string() = default;
        // clang-format on

        // clang-format off
        // size / empty / clear
        inline size_t       size            () const { return m_std_string ? m_std_string->size() : m_stack_string_size;  }
        inline size_t       length          () const { return size();  }
        inline bool         empty           () const { return size() == 0; }
        inline void         clear           () { init(); }
        // clang-format on

        // clang-format off
        // data
        inline const char*  c_str           () const { return m_std_string ? m_std_string.get()->c_str() : m_stack_string.data(); }
        inline const char*  data            () const { return m_std_string ? m_std_string.get()->c_str() : m_stack_string.data(); }
        inline char*        data            ()       { return m_std_string ? m_std_string.get()->data()  : m_stack_string.data(); } // direct access

        inline const char*  begin           () const noexcept { return data(); }
        inline const char*  end             () const noexcept { return data() + size(); }
        inline const char*  cbegin          () const noexcept { return data(); }
        inline const char*  cend            () const noexcept { return data() + size(); }
        
        inline const char*  rbegin          () const noexcept { return end(); }
        inline const char*  rend            () const noexcept { return begin(); }
        inline const char*  crbegin         () const noexcept { return end(); }
        inline const char*  crend           () const noexcept { return begin(); }
        // clang-format off
        
        // clang-format off
        // as c_string
        inline std::string      c_string    () const { return m_std_string ? *(m_std_string.get()) : std::string(data(), size()); }
        inline std::string_view c_view      () const { return std::string_view{data(), size()}; }
        inline std::wstring     c_wstring   () const { return get_stringw_impl(); }
        inline std::vector<char>c_vector    () const
        {
            // vector is not null terminated, handle with care
            std::vector<char> v;
            v.reserve(size() + 1);
            v.resize(size());
            memcpy(v.data(), data(), size());
            return v;
        }
        // clang-format on

        // reserve / resize
        inline void reserve(const size_t new_size)
        {
            ensure_allocation(new_size);
            if (m_std_string) {
                m_std_string->reserve(new_size);
            } else {
                /**/
            }
        }

        inline void resize(const size_t new_size)
        {
            ensure_allocation(new_size);
            if (m_std_string) {
                m_std_string->resize(new_size);
            } else {
                // fill with zero from current size to new size
                if (m_stack_string_size < new_size) {
                    memset(m_stack_string.data() + m_stack_string_size, 0, new_size - m_stack_string_size);
                }
                m_stack_string_size                 = new_size;
                m_stack_string[m_stack_string_size] = '\0';
            }
        }

        inline void shrink_to_fit()
        {
            if (m_std_string) {
                m_std_string->shrink_to_fit();
            } else {
                /*nothing to do*/
            }
        }

        // clang-format off
        // assign
        inline void         assign          (const char c)                                      { set(0/*startfrom*/, c); }
        inline void         assign          (const char* s, const size_t& s_length)             { set(0/*startfrom*/, s, s_length); }
        inline void         assign          (const std::string_view s)                          { set(0/*startfrom*/, s); }
        inline void         assign          (const std::vector<char>& v)                        { set(0/*startfrom*/, v); }
        
        inline void         assign          (const wchar_t c)                                   { set(0/*startfrom*/, c); }
        inline void         assign          (const wchar_t* s, const size_t& s_length)          { set(0/*startfrom*/, s, s_length); }
        inline void         assign          (const std::wstring_view s)                         { set(0/*startfrom*/, s); }
        // clang-format on

        // clang-format off
        // append
        inline void         append          (const char c)                                      { set(size()/*startfrom*/, c); }
        inline void         append          (const char* s, size_t s_length)                    { set(size()/*startfrom*/, s, s_length); }
        inline void         append          (const std::string_view s)                          { set(size()/*startfrom*/, s); }
        inline void         append          (const std::vector<char>& v)                        { set(size()/*startfrom*/, v); }
        
        inline void         append          (const wchar_t c)                                   { set(size()/*startfrom*/, c); }
        inline void         append          (const wchar_t* s, size_t s_length)                 { set(size()/*startfrom*/, s, s_length); }
        inline void         append          (const std::wstring_view s)                         { set(size()/*startfrom*/, s); }
        // clang-format on

        // clang-format off
        // insert
        inline void         insert          (std::size_t from, const char c)                    { insert_impl(from, &c, 1); }
        inline void         insert          (std::size_t from, const char* s, size_t s_length)  { insert_impl(from, s, s_length); }
        inline void         insert          (std::size_t from, const std::string_view s)        { insert_impl(from, s.data(), s.size()); }
        inline void         insert          (std::size_t from, const std::vector<char>& v)      { insert_impl(from, v.data(), v.size()); }
        
        inline void         insert          (std::size_t from, const wchar_t c)                 { insert_impl(from, &c, 1); }
        inline void         insert          (std::size_t from, const wchar_t* s, size_t s_length){insert_impl(from, s, s_length); }
        inline void         insert          (std::size_t from, const std::wstring_view s)       { insert_impl(from, s.data(), s.size()); }
        // clang-format on

        // clang-format off
        // overwrite
        inline void         overwrite       (std::size_t from, const char c)                    { set(from, c); }
        inline void         overwrite       (std::size_t from, const char* s, size_t s_length)  { set(from, s, s_length); }
        inline void         overwrite       (std::size_t from, const std::string_view s)        { set(from, s); }
        inline void         overwrite       (std::size_t from, const std::vector<char>& v)      { set(from, v); }
        
        inline void         overwrite       (std::size_t from, const wchar_t c)                 { set(from, c); }
        inline void         overwrite       (std::size_t from, const wchar_t* s, size_t s_length){set(from, s, s_length); }
        inline void         overwrite       (std::size_t from, const std::wstring_view s)       { set(from, s); }
        // clang-format on

        // clang-format off
        // set
        inline void         set             (std::size_t from, const char c)                    { set_impl(from, &c, 1); }
        inline void         set             (std::size_t from, const char* s, size_t s_length)  { set_impl(from, s, s_length); }
        inline void         set             (std::size_t from, const std::string_view s)        { set_impl(from, s.data(), s.size()); }
        inline void         set             (std::size_t from, const std::vector<char>& v)      { set_impl(from, v.data(), v.size()); }
        
        inline void         set             (std::size_t from, const wchar_t c)                 { set_impl(from, &c, 1); }
        inline void         set             (std::size_t from, const wchar_t* s, size_t s_length){set_impl(from, s, s_length); }
        inline void         set             (std::size_t from, const std::wstring_view s)       { set_impl(from, s.data(), s.size()); }
        // clang-format on

        // clang-format off
        // swap
        inline void         swap            ( stack_string& o ) noexcept                        { swap_impl( o ); }
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
            erase_impl(from, length);
        }

        // compare
        inline bool is_equal(const char* s, std::size_t s_length) const
        {
            return size() == s_length && compare(s, s_length) == 0;
        }

        inline int compare(const char* s, std::size_t s_length) const
        {
            const std::size_t this_size = size();
            int               cmp       = memcmp(data(), s, std::min(this_size, s_length));

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

        // =
        inline stack_string& operator=(const stack_string& o) noexcept
        {
            if (this != &o) {
                m_stack_string_size = o.m_stack_string_size;
                m_stack_string      = o.m_stack_string;
                if (o.m_std_string) {
                    if (!m_std_string) {
                        m_std_string = std::make_unique<std::string>();
                    }
                    *m_std_string = *o.m_std_string.get();
                } else {
                    m_std_string.reset();
                }
            }
            return *this;
        }

        inline stack_string& operator=(stack_string&& o) noexcept
        {
            if (this != &o) {
                m_stack_string_size = o.m_stack_string_size;
                m_stack_string      = o.m_stack_string;
                m_std_string        = std::move(o.m_std_string);
                o.init();
            }
            return *this;
        }

        // clang-format off
        inline stack_string& operator=      (const char c) noexcept                             { assign(c); return *this; }
        inline stack_string& operator=      (const std::string_view s) noexcept                 { assign(s); return *this; }
        inline stack_string& operator=      (const std::vector<char>& v) noexcept               { assign(v); return *this; }
        inline stack_string& operator=      (const std::string& s) noexcept                     { assign(std::string_view{s}); return *this; }
        
        inline stack_string& operator=      (const wchar_t c) noexcept                          { assign(c); return *this; }
        inline stack_string& operator=      (const std::wstring_view s) noexcept                { assign(s); return *this; }
        inline stack_string& operator=      (const std::wstring& s) noexcept                    { assign(std::wstring_view{s}); return *this; }
        // clang-format on

        // clang-format off
        // +=
        inline stack_string& operator+=     (const char c) noexcept                             { append(c); return *this; }
        inline stack_string& operator+=     (const std::string_view s) noexcept                 { append(s); return *this; }
        inline stack_string& operator+=     (const std::vector<char>& v) noexcept               { append(v); return *this; }
        
        inline stack_string& operator+=     (const wchar_t c) noexcept                          { append(c); return *this; }
        inline stack_string& operator+=     (const std::wstring_view s) noexcept                { append(s); return *this; }
        // clang-format on

        // clang-format off
        // [] / at
        inline char&        operator[]      (std::size_t index)                                 { return data()[index]; }
        inline char         operator[]      (std::size_t index) const                           { return data()[index]; }
        
        inline char&        at              (std::size_t index)                                 { return data()[index]; }
        inline char         at              (std::size_t index) const                           { return data()[index]; }

         // front / back
         inline char&        front          ()                                                  { return size() > 0 ? data()[0] : m_stack_string[0]; }
         inline char         front          () const                                            { return size() > 0 ? data()[0] : '\0'; }
 
         inline char&        back           ()                                                  { return size() > 0 ? data()[size() - 1] : m_stack_string[0]; }
         inline char         back           () const                                            { return size() > 0 ? data()[size() - 1] : '\0'; }

        // clang-format on

        // clang-format off
        inline void         push_back       (const char c) { append( c ); }
        inline void         pop_back        () { 
            if (size() > 0) { 
                resize(size() - 1); 
            } 
        }
        // clang-format off

        // operator
        inline              operator std::string_view() const { return c_view(); }

        // clang-format off
        //
        // substr
        //
        inline std::string_view substr(std::size_t __pos = 0, std::size_t __n = std::string::npos) const noexcept(false)
        {
            return c_view().substr(__pos, __n);
        }

        // starts_with
        inline bool         starts_with     (std::string_view __x) const noexcept               { return substr(0, __x.size()) == __x; }
        inline bool         starts_with     (char __x) const noexcept                           { return !empty() && (front() == __x); }
        inline bool         starts_with     (const char* __x) const noexcept                    { return starts_with(std::string_view(__x)); }

        // ends_with
        inline bool         ends_with       (std::string_view __x) const noexcept 
        {
            const auto __len = this->size();
            const auto __xlen = __x.size();
            return __len >= __xlen && memcmp(data() + size() - __xlen, __x.data(), __xlen) == 0;
        }
        inline bool         ends_with       (char __x) const noexcept                           { return !empty() && (back() == __x); }
        inline bool         ends_with       (const char* __x) const noexcept                    { return ends_with(std::string_view(__x)); }

        // contains
        inline bool         contains        (std::string_view __x) const noexcept               { return find(__x) != std::string::npos; }
        inline bool         contains        (char __x) const noexcept                           { return find(__x) != std::string::npos; }
        inline bool         contains        (const char* __x) const noexcept                    { return find(__x) != std::string::npos; }

        // find
        inline std::size_t  find            (std::string_view __str, std::size_t __pos = 0) const noexcept          { return c_view().find(__str, __pos); }
        inline std::size_t  find            (char __c, std::size_t __pos = 0) const noexcept                        { return c_view().find(__c, __pos); }
        inline std::size_t  find            (const char* __str, std::size_t __pos, std::size_t __n) const noexcept  { return c_view().find(__str, __pos, __n); }
        inline std::size_t  find            (const char* __str, std::size_t __pos = 0) const noexcept               { return c_view().find(__str, __pos); }

        // rfind
        inline std::size_t  rfind           (std::string_view __str, std::size_t __pos = std::string::npos) const noexcept { return c_view().rfind(__str, __pos); }
        inline std::size_t  rfind           (char __c, std::size_t __pos = std::string::npos) const noexcept        { return c_view().rfind(__c, __pos); }
        inline std::size_t  rfind           (const char* __str, std::size_t __pos, std::size_t __n) const noexcept  { return c_view().rfind(__str, __pos, __n); }
        inline std::size_t  rfind           (const char* __str, std::size_t __pos = std::string::npos) const noexcept{return c_view().rfind(__str, __pos); }

        // find_first_of
        inline std::size_t  find_first_of   (std::string_view __str, std::size_t __pos = 0) const noexcept          { return c_view().find_first_of(__str, __pos); }
        inline std::size_t  find_first_of   (char __c, std::size_t __pos = 0) const noexcept                        { return c_view().find_first_of(__c, __pos); }
        inline std::size_t  find_first_of   (const char* __str, std::size_t __pos, std::size_t __n) const noexcept  { return c_view().find_first_of(__str, __pos, __n); }
        inline std::size_t  find_first_of   (const char* __str, std::size_t __pos = 0) const noexcept               { return c_view().find_first_of(__str, __pos); }

        // find_last_of
        inline std::size_t  find_last_of    (std::string_view __str, std::size_t __pos = std::string::npos) const noexcept { return c_view().find_last_of(__str, __pos); }
        inline std::size_t  find_last_of    (char __c, std::size_t __pos = std::string::npos) const noexcept        { return c_view().find_last_of(__c, __pos); }
        inline std::size_t  find_last_of    (const char* __str, std::size_t __pos, std::size_t __n) const noexcept  { return c_view().find_last_of(__str, __pos, __n); }
        inline std::size_t  find_last_of    (const char* __str, std::size_t __pos = std::string::npos) const noexcept{return c_view().find_last_of(__str, __pos); }

        // find_first_not_of
        inline std::size_t  find_first_not_of(std::string_view __str, std::size_t __pos = 0) const noexcept      { return c_view().find_first_not_of(__str, __pos); }
        inline std::size_t  find_first_not_of(char __c, std::size_t __pos = 0) const noexcept                    { return c_view().find_first_not_of(__c, __pos); }
        inline std::size_t  find_first_not_of(const char* __str, std::size_t __pos, std::size_t __n) const noexcept{return c_view().find_first_not_of(__str, __pos, __n); }
        inline std::size_t  find_first_not_of(const char* __str, std::size_t __pos = 0) const noexcept           { return c_view().find_first_not_of(__str, __pos); }

        // find_last_not_of
        inline std::size_t  find_last_not_of(std::string_view __str, std::size_t __pos = std::string::npos) const noexcept { return c_view().find_last_not_of(__str, __pos); }
        inline std::size_t  find_last_not_of(char __c, std::size_t __pos = std::string::npos) const noexcept     { return c_view().find_last_not_of(__c, __pos); }
        inline std::size_t  find_last_not_of(const char* __str, std::size_t __pos, std::size_t __n) const noexcept{return c_view().find_last_not_of(__str, __pos, __n); }
        inline std::size_t  find_last_not_of(const char* __str, std::size_t __pos = std::string::npos) const noexcept{return c_view().find_last_not_of(__str, __pos); }
        // clang-format on

    private:
        // init
        inline void init()
        {
            m_stack_string_size = 0;
            m_stack_string.fill('\0');
            m_std_string.reset();
        }

        // ensure allocation, stack or new std::string
        inline void ensure_allocation(size_t new_size)
        {
            // once std::string is allocated use it
            if (m_std_string) {
                return;
            }

            if (new_size < m_stack_string.max_size()) {
                /*still on stack including null terminator*/
            } else {
                if (!m_std_string) {
                    m_std_string = std::make_unique<std::string>(m_stack_string.data(), m_stack_string_size);
                }
            }
        }

        // set impl
        inline void set_impl(const size_t& start_from, const char* b, const size_t& b_length)
        {
            resize(start_from + b_length);
            memcpy(data() + start_from, b, b_length);
        }

        // set impl
        inline void set_impl(const size_t& start_from, const wchar_t* wstr, const size_t& wstr_length)
        {
            std::setlocale(LC_ALL, "en_US.utf8");
            std::mbstate_t state = std::mbstate_t();

            // determine size

#ifdef _WIN32
            size_t new_length = 0;
            int    ret        = wcsrtombs_s(&new_length, nullptr, 0, &wstr, 0, &state);
            if (ret != 0 || new_length == 0)
                return;
            --new_length; // because it adds the null terminator in length
            resize(start_from + new_length);

            size_t converted = 0;
            ret              = wcsrtombs_s(&converted, data() + start_from, new_length + 1, &wstr, new_length, &state);
#else
            size_t new_length = std::wcsrtombs(nullptr, &wstr, 0, &state);
            if (new_length == static_cast<std::size_t>(-1))
                return;

            resize(start_from + new_length);
            /*size_t converted =*/std::wcsrtombs(data() + start_from, &wstr, new_length, &state);
#endif
        }

        // get the wstring
        std::wstring get_stringw_impl() const
        {
            std::setlocale(LC_ALL, "en_US.utf8");
            std::mbstate_t state = std::mbstate_t();

            // determine size
            std::wstring wstr;
            const char*  mbstr = data();

#ifdef _WIN32
            size_t new_length = 0;
            int    ret        = mbsrtowcs_s(&new_length, nullptr, 0, &mbstr, 0, &state);
            if (ret != 0 || new_length == 0)
                return wstr;
            --new_length; // because it adds the null terminator in length

            wstr.resize(new_length);
            size_t converted = 0;
            ret              = mbsrtowcs_s(&converted, wstr.data(), new_length + 1, &mbstr, new_length, &state);
#else
            size_t new_length = std::mbsrtowcs(nullptr, &mbstr, 0, &state);
            if (new_length == static_cast<std::size_t>(-1))
                return wstr;

            wstr.resize(new_length);
            std::mbsrtowcs(wstr.data(), &mbstr, wstr.size(), &state);
#endif
            return wstr;
        }

        // insert impl
        inline void insert_impl(const size_t& insert_from, const char* b, const size_t& b_length)
        {
            size_t initial_length = size();
            reserve(insert_from <= initial_length ? initial_length + b_length : insert_from + b_length);

            if (m_std_string) {
                m_std_string->insert(insert_from, b, b_length);
            } else {
                resize(insert_from <= initial_length ? initial_length + b_length : insert_from + b_length);
                if (insert_from <= initial_length) {
                    memmove(m_stack_string.data() + insert_from + b_length,
                            m_stack_string.data() + insert_from,
                            initial_length - insert_from);
                } else {
                    memset(m_stack_string.data() + initial_length, '\0', (insert_from - initial_length));
                }
                memcpy(m_stack_string.data() + insert_from, b, b_length);
            }
        }

        // erase
        inline void erase_impl(const size_t& start_from, const size_t& length)
        {
            if (m_std_string) {
                m_std_string->erase(start_from, length);
            } else {
                if (start_from < size()) {
                    if (start_from + length < size()) {
                        size_t move_length = size() - (start_from + length);
                        memmove(m_stack_string + start_from, m_stack_string + start_from + length, move_length);
                        resize(size() - length);
                    } else {
                        resize(start_from);
                    }
                }
            }
        }

        // swap
        inline void swap_impl(stack_string& o)
        {
            std::swap(m_stack_string_size, o.m_stack_string_size);
            std::swap(m_stack_string, o.m_stack_string);
            std::swap(m_std_string, o.m_std_string);
        }

        // clang-format off
        // +        
        friend inline stack_string operator+(const stack_string& s, const char c)               { stack_string sr = s; sr.append(c); return sr; }
        friend inline stack_string operator+(const stack_string& s, const std::string_view v)   { stack_string sr = s; sr.append(v); return sr; }
        friend inline stack_string operator+(const stack_string& s, const std::vector<char>& v) { stack_string sr = s; sr.append(v); return sr; }        
        friend inline stack_string operator+(const stack_string& s, const wchar_t c)            { stack_string sr = s; sr.append(c); return sr; }        
        friend inline stack_string operator+(const stack_string& s, const std::wstring_view w)  { stack_string sr = s; sr.append(w); return sr; }
        
        friend inline stack_string operator+(const char c, const stack_string& s2)              { stack_string sr(c); sr.append(s2); return sr; }
        friend inline stack_string operator+(const std::vector<char>& v, const stack_string& s2){ stack_string sr(v); sr.append(s2); return sr; }
        friend inline stack_string operator+(const wchar_t c, const stack_string& s2)           { stack_string sr(c); sr.append(s2); return sr; }
        friend inline stack_string operator+(const std::wstring_view s, const stack_string& s2) { stack_string sr(s); sr.append(s2); return sr; }
        // clang-format on

        // clang-format off
        // ==
        friend inline bool         operator==(const stack_string& s, const char c)              { return s.is_equal( &c, sizeof(c)); }
        friend inline bool         operator==(const stack_string& s, const std::string_view v)  { return s.is_equal(v.data(), v.size()); }
        friend inline bool         operator==(const stack_string& s, const std::vector<char>& v){ return s.is_equal(v.data(), v.size()); }
         
        friend inline bool         operator==(const char c, const stack_string& s2)             { return s2.is_equal(&c, sizeof(c)); }
        friend inline bool         operator==(const std::vector<char>& v, const stack_string& s2){return s2.is_equal(v.data(), v.size()); }
        
        // !=
        friend inline bool         operator!=(const stack_string& s, const char c)              { return !s.is_equal(&c, sizeof(c)); }
        friend inline bool         operator!=(const stack_string& s, const std::string_view v)  { return !s.is_equal(v.data(), v.size()); }
        friend inline bool         operator!=(const stack_string& s, const std::vector<char>& v){ return !s.is_equal(v.data(), v.size()); }

        friend inline bool         operator!=(const char c, const stack_string& s2)             { return !s2.is_equal(&c, sizeof(c)); }
        friend inline bool         operator!=(const std::vector<char>& v, const stack_string& s2){return !s2.is_equal(v.data(), v.size()); }
        // clang-format on

        // clang-format off
        // < 
        friend inline bool         operator< (const stack_string& s, const char c)              { return s.compare(&c, sizeof(c)) < 0; }
        friend inline bool         operator< (const stack_string& s, const std::string_view v)  { return s.compare(v.data(), v.size()) < 0; }
        friend inline bool         operator< (const stack_string& s, const std::vector<char>& v){ return s.compare(v.data(), v.size()) < 0; }

        friend inline bool         operator< (const char c, const stack_string& s2)             { return s2.compare(&c, sizeof(c)) >= 0; }
        friend inline bool         operator< (const std::vector<char>& v, const stack_string& s2){return s2.compare(v.data(), v.size()) >= 0; }
        // clang-format on

        // clang-format off
        // <= 
        friend inline bool         operator<=(const stack_string& s, const char c)              { return s.compare(&c, sizeof(c)) <= 0; }
        friend inline bool         operator<=(const stack_string& s, const std::string_view v)  { return s.compare(v.data(), v.size()) <= 0; }
        friend inline bool         operator<=(const stack_string& s, const std::vector<char>& v){ return s.compare(v.data(), v.size()) <= 0; }

        friend inline bool         operator<=(const char c, const stack_string& s2)             { return s2.compare(&c, sizeof(c)) > 0; }
        friend inline bool         operator<=(const std::vector<char>& v, const stack_string& s2){return s2.compare(v.data(), v.size()) > 0; }
        // clang-format on

        // clang-format off
        // >        
        friend inline bool         operator> (const stack_string& s, const char c)              { return s.compare(&c, sizeof(c)) > 0; }
        friend inline bool         operator> (const stack_string& s, const std::string_view v)  { return s.compare(v.data(), v.size()) > 0; }
        friend inline bool         operator> (const stack_string& s, const std::vector<char>& v){ return s.compare(v.data(), v.size()) > 0; }

        friend inline bool         operator> (const char c, const stack_string& s2)             { return s2.compare(&c, sizeof(c)) <= 0; }
        friend inline bool         operator> (const std::vector<char>& v, const stack_string& s2){return s2.compare(v.data(), v.size()) <= 0; }
        // clang-format on

        // clang-format off
        // >= 
        friend inline bool         operator>=(const stack_string& s, const char c)              { return s.compare(&c, sizeof(c)) >= 0; }
        friend inline bool         operator>=(const stack_string& s, const std::string_view v)  { return s.compare(v.data(), v.size()) >= 0; }
        friend inline bool         operator>=(const stack_string& s, const std::vector<char>& v){ return s.compare(v.data(), v.size()) >= 0; }
                                                                            
        friend inline bool         operator>=(const char c, const stack_string& s2)             { return s2.compare(&c, sizeof(c)) < 0; }        
        friend inline bool         operator>=(const std::vector<char>& v, const stack_string& s2){return s2.compare(v.data(), v.size()) < 0; }
        // clang-format on

        // out
        friend inline std::ostream& operator<<(std::ostream& out, const stack_string& s)
        {
            out << s.c_view();
            return out;
        }

    private:
        // string by default uses the array and if it goes beyond makes use of the std_string
        std::array<char, StackAllocSize> m_stack_string;
        size_t                           m_stack_string_size;

        // for larger string
        std::unique_ptr<std::string> m_std_string;
    };
} // namespace small
