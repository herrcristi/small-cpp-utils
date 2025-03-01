#pragma once

#include <stdlib.h>

#include <array>
#include <clocale>
#include <cwchar>
#include <string>
#include <string_view>

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

        
        stack_string(const stack_string& o) noexcept                    : stack_string() { operator=(o); }
        stack_string(stack_string&&      o) noexcept                    : stack_string() { operator=( std::forward<stack_string>(o)); }
        
        stack_string(const char c) noexcept                             : stack_string() { operator=(c); }
        stack_string(const char* s, const size_t& s_length) noexcept    : stack_string() { set(s, s_length); }
        stack_string(const std::string& s) noexcept                     : stack_string() { operator=(s); }
        stack_string(const std::vector<char>& v) noexcept               : stack_string() { operator=(v); }
        stack_string(const std::string_view s) noexcept                 : stack_string() { operator=(s); }
        
        stack_string(const wchar_t c) noexcept                          : stack_string() { operator=(c); }
        stack_string(const wchar_t *s, const size_t& s_length ) noexcept: stack_string() { set(s, s_length); }
        stack_string(const std::wstring& s) noexcept                    : stack_string() { operator=(s); }

        // destructor
        ~stack_string() = default;
        // clang-format on

        // clang-format off
        // size / empty / clear
        inline size_t       size            () const { return m_std_string ? m_std_string->size() : m_stack_string_size;  }
        inline size_t       length          () const { return size();  }
        inline bool         empty           () const { return size() == 0; }
        inline void         clear           () { m_stack_string_size = 0; init(); }
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
            ensure_size(new_size);
            if (m_std_string) {
                m_std_string->reserve(new_size);
            } else {
                /**/
            }
        }

        inline void resize(const size_t new_size)
        {
            ensure_size(new_size);
            if (m_std_string) {
                m_std_string->resize(new_size);
            } else {
                m_stack_string_size                 = new_size;
                m_stack_string[m_stack_string_size] = '\0';
            }
        }

        inline void shrink_to_fit()
        {
            if (m_std_string) {
                m_std_string->shrink_to_fit();
            } else { /**/
            }
        }

        // clang-format off
        // assign
        inline void         assign          (const stack_string& s)     { if (this != &s) { set(s.data(), s.size(), 0/*startfrom*/); } }
        inline void         assign          (const char c)              { set(&c, sizeof(c), 0/*startfrom*/); }
        inline void         assign          (const char* s, const size_t& s_length) { set(s, s_length, 0/*startfrom*/); }
        inline void         assign          (const wchar_t c)           { set(&c, sizeof(c)/sizeof(wchar_t), 0/*startfrom*/); }
        inline void         assign          (const wchar_t* s, const size_t& s_length ){ set(s, s_length, 0/*startfrom*/); }
        inline void         assign          (const std::string& s)      { set(s.c_str(), s.size(), 0/*startfrom*/); }
        inline void         assign          (const std::string_view s)  { set(s.data(), s.size(), 0/*startfrom*/); }
        inline void         assign          (const std::wstring& s)     { set(s.c_str(), s.size(),0/*startfrom*/); }
        inline void         assign          (const std::vector<char>& v){ set(v.data(), v.size(), 0/*startfrom*/); }
        // clang-format on

        // clang-format off
        // append
        inline void         append          (const stack_string& s)     { set( s.data(),              s.size(),       size()/*startfrom*/ ); }
        inline void         append          (const char c)              { set( &c,                    sizeof(c),      size()/*startfrom*/ ); }
        inline void         append          (const char* s, size_t s_length) { set( s,         s_length,       size()/*startfrom*/ ); }
        inline void         append          (const wchar_t c)           { set( &c,                    sizeof(c) / sizeof(wchar_t), size()/*startfrom*/ ); }
        inline void         append          (const wchar_t* s, size_t s_length ){ set( s,              s_length,       size()/*startfrom*/ ); }
        inline void         append          (const std::string& s)      { set( s.c_str(),             s.size(),       size()/*startfrom*/ ); }
        inline void         append          (const std::string_view s)  { set( s.data(),              s.size(),       size()/*startfrom*/ ); }
        inline void         append          (const std::wstring& s)     { set( s.c_str(),             s.size(),       size()/*startfrom*/ ); }
        inline void         append          (const std::vector<char>& v){ set( v.data(),              v.size(),       size()/*startfrom*/ ); }
        // clang-format on

        // clang-format off
        // set
        inline void         set             ( const char* s,    const size_t& s_length, const size_t& start_from = 0 ) { set_impl( s, s_length, start_from ); }
        inline void         overwrite       ( const char* s,    const size_t& s_length, const size_t& start_from = 0 ) { set( s, s_length, start_from ); }
        
        inline void         set             ( const wchar_t* s, const size_t& s_length, const size_t& start_from = 0 ) { set_impl( s, s_length, start_from ); }
        // clang-format on

        // clang-format off
        // insert
        inline void         insert          ( const char* b, const size_t& b_length, const size_t& insert_from = 0 ) { insert_impl( b, b_length, insert_from ); }
        // erase
        inline void         erase           ( const size_t& start_from ) { if ( start_from < size() ) { resize( start_from ); } }
        inline void         erase           ( const size_t& start_from, const size_t& length ) { erase_impl( start_from, length ); }
        
        // compare
        inline bool         is_equal        ( const char *s, const size_t& s_length ) const { return size() == s_length && compare( s, s_length ) == 0; }
        inline int          compare         ( const char *s, const size_t& s_length ) const { return strncmp( data(), s, size() < s_length ? size()+1 : s_length+1 ); }

        inline bool         is_equal        ( const wchar_t *s, const size_t& s_length ) const { stack_string ss( s, s_length ); return is_equal( ss.data(), ss.size() ) == 0;  }
        inline int          compare         ( const wchar_t *s, const size_t& s_length ) const { stack_string ss( s, s_length ); return compare ( ss.data(), ss.size() );       }

        // swap
        inline void         swap            ( stack_string& o ) { swap_impl( o ); }
        // clang-format on

        // operators

        // =
        inline stack_string& operator=(const stack_string& o) noexcept
        {
            if (this != &o) {
                m_stack_string_size = o.m_stack_string_size;
                m_stack_string      = std::copy(o.m_stack_string);
                if (o.m_std_string) {
                    if (!m_std_string) {
                        m_std_string = std::make_unique<std::string>();
                    }
                    m_std_string = *o.m_std_string.get();
                } else {
                    m_std_string.reset();
                }
            }
            return *this;
        }

        inline stack_string& operator=(stack_string&& o) noexcept
        {
            if (this != &o) {
                m_stack_string_size = std::move(o.m_stack_string_size);
                m_stack_string      = std::move(o.m_stack_string);
                m_std_string        = std::move(o.m_std_string);
                o.init();
            }
            return *this;
        }

        // clang-format off
        inline stack_string& operator=              ( const char*   s           ) noexcept { set( s,                        strlen(s)       ); return *this; }
        inline stack_string& operator=              ( const char    c           ) noexcept { set( &c,                       sizeof(char)    ); return *this; }
        inline stack_string& operator=              ( const wchar_t*s           ) noexcept { set( s,                        wcslen(s)       ); return *this; }
        inline stack_string& operator=              ( const wchar_t c           ) noexcept { set( &c,                       sizeof(c) / sizeof(wchar_t) ); return *this; }
        inline stack_string& operator=              ( const std::string&  s     ) noexcept { set( s.c_str(),                s.size()        ); return *this; }
        inline stack_string& operator=              ( const std::wstring& s     ) noexcept { set( s.c_str(),                s.size()        ); return *this; }
        inline stack_string& operator=              ( const std::vector<char>& v) noexcept { set( v.data(),                 v.size()        ); return *this; }
        inline stack_string& operator=              ( const std::string_view  s ) noexcept { set( s.data(),                 s.size()        ); return *this; }
        // clang-format on

        // clang-format off
        // +=
        inline stack_string& operator+=             ( const stack_string& s     ) noexcept { append( s.data(),              s.size()        ); return *this; }
        inline stack_string& operator+=             ( const char*   s           ) noexcept { append( s,                     strlen(s)       ); return *this; }
        inline stack_string& operator+=             ( const char    c           ) noexcept { append( &c,                    sizeof(char)    ); return *this; }
        inline stack_string& operator+=             ( const wchar_t*s           ) noexcept { append( s,                     wcslen(s)       ); return *this; }
        inline stack_string& operator+=             ( const wchar_t c           ) noexcept { append( &c,                    1               ); return *this; }
        inline stack_string& operator+=             ( const std::string&  s     ) noexcept { append( s.c_str(),             s.size()        ); return *this; }
        inline stack_string& operator+=             ( const std::wstring& s     ) noexcept { append( s.c_str(),             s.size()        ); return *this; }
        inline stack_string& operator+=             ( const std::vector<char>& v) noexcept { append( v.data(),              v.size()        ); return *this; }
        inline stack_string& operator+=             ( const std::string_view s  ) noexcept { append( s.data(),              s.size()        ); return *this; }
        // clang-format on

        // clang-format off
        // []
        inline char&    operator[]                  ( const size_t& index       ) { return m_stack_string[ index ]; }
        inline const char operator[]                ( const size_t& index       ) const { return m_stack_string[ index ]; }
        
        inline char&    at                          ( const size_t& index       ) { return m_stack_string[ index ]; }
        inline const char at                        ( const size_t& index       ) const { return m_stack_string[ index ]; }
        // clang-format on

        // clang-format off
        inline void     push_back                   ( const char c ) { append( c ); }
        inline void     pop_back                    () { if ( size() > 0 ) { resize( size() - 1 ); } }
        // clang-format off


        // operator
        inline          operator std::string_view   () { return c_view(); }



    
    private:
        // init
        inline void     init                        () { m_stack_string_size  = 0; m_stack_string.fill( '\0' ); m_std_string.reset(); }

        // ensure size
        inline void     ensure_size                 ( size_t new_size )
        {
            if ( new_size < m_stack_string.max_size() )
            {

            }
            else 
            {
                if ( !m_std_string )
                {
                    m_std_string = std::make_unique<std::string>( m_stack_string.data(), m_stack_string_size );
                } 
            }
        }

        // set impl
        inline void     set_impl                    ( const char* b, const size_t& b_length, const size_t& start_from = 0 ) 
        { 
            resize( start_from + b_length ); 
            //if ( m_std_string )
            //{
            //    memcpy( m_std_string.get()->data() + start_from, b, b_length );
            //}
            //else
            //{
            //    memcpy( m_stack_string.data() + start_from, b, b_length );
            //}
            memcpy( data() + start_from, b, b_length );
        }

         // set impl
        inline void     set_impl                    ( const wchar_t* wstr, const size_t& wstr_length, const size_t& start_from = 0 )
        { 
            std::setlocale( LC_ALL, "en_US.utf8" );
            std::mbstate_t state = std::mbstate_t();
            
            // determine size
            
#ifdef _WIN32
            size_t new_length = 0;
            int ret = wcsrtombs_s( &new_length, nullptr, 0, &wstr, 0, &state );
            if ( ret != 0 || new_length == 0 )
                return;
            --new_length; // because it adds the null terminator in length
            resize( start_from + new_length );

            size_t converted = 0;
            ret = wcsrtombs_s( &converted, data() + start_from, new_length + 1, &wstr, new_length, &state );
#else
            size_t new_length = std::wcsrtombs( nullptr, &wstr, 0, &state );
            if ( new_length == static_cast<std::size_t>(-1) )
                return;
            
            resize( start_from + new_length );
            /*size_t converted =*/ std::wcsrtombs( data() + start_from, &wstr, new_length, &state );
#endif 
        }

        // get the wstring
        std::wstring get_stringw_impl() const
        {
            std::setlocale( LC_ALL, "en_US.utf8" );
            std::mbstate_t state = std::mbstate_t();

            // determine size
            std::wstring wstr;
            const char* mbstr = data();

#ifdef _WIN32
            size_t new_length = 0;
            int ret = mbsrtowcs_s( &new_length, nullptr, 0, &mbstr, 0, &state );
            if ( ret != 0 || new_length == 0 )
                return wstr;
            --new_length; // because it adds the null terminator in length
            
            wstr.resize( new_length );
            size_t converted = 0;
            ret = mbsrtowcs_s( &converted, wstr.data(), new_length + 1, &mbstr, new_length, &state );
#else
            size_t new_length = std::mbsrtowcs( nullptr, &mbstr, 0, &state );
            if ( new_length == static_cast<std::size_t>(-1) )
                return wstr;
            
            wstr.resize( new_length );
            std::mbsrtowcs( wstr.data(), &mbstr, wstr.size(), &state );
#endif
            return wstr;
        }



        // insert impl
        inline void     insert_impl                 ( const char* b, const size_t& b_length, const size_t& insert_from = 0 ) 
        { 
            size_t initial_length = size();
            reserve( insert_from <= initial_length ? initial_length + b_length : insert_from + b_length );
            
            if ( m_std_string )
            {
                m_std_string->insert( insert_from, b, b_length );
            }
            else
            {
                resize( insert_from <= initial_length ? initial_length + b_length : insert_from + b_length );
                if ( insert_from <= initial_length )
                    memmove( m_stack_string + insert_from + b_length, m_stack_string + insert_from, initial_length - insert_from );
                else
                    memset( m_stack_string + initial_length, '\0', (insert_from - initial_length) );
                memcpy( m_stack_string + insert_from, b, b_length );
            }
        }


        // erase
        inline void     erase_impl                       ( const size_t& start_from, const size_t& length ) 
        { 
            if ( m_std_string )
            {
                m_std_string->erase( start_from, length );
            }
            else
            {
                if ( start_from < size() )
                {
                    if ( start_from + length < size() )
                    {
                        size_t move_length = size() - (start_from + length);
                        memmove( m_stack_string + start_from, m_stack_string + start_from + length, move_length );
                        resize( size() - length );
                    }
                    else
                    {
                        resize( start_from );
                    }
                }
            }
        }


        // swap
        inline void     swap_impl                   ( stack_string& o )
        {
            std::swap( m_stack_string_size,  o.m_stack_string_size );
            std::swap( m_stack_string,       o.m_stack_string );
            std::swap( m_std_string,         o.m_std_string );
        }
    
    private:
        // string by default uses the array and if it goes beyond makes use of the std_string
        std::array<char, StackAllocSize> m_stack_string;
        size_t          m_stack_string_size;
        
        // for larger string
        std::unique_ptr<std::string> m_std_string;
    };




    // other operators

    // clang-format off
    // +
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( const stack_string<S>& b, const stack_string<S>& b2 ) { stack_string<S> br = b; br.append( b2.data(),     b2.size()       ); return br; }
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( stack_string<S>&& b,      const stack_string<S>& b2 ) { stack_string<S> br( std::forward<stack_string<S>>( b ) ); br.append( b2.data(), b2.size() ); return br; }
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( const stack_string<S>& b, const char*   s           ) { stack_string<S> br = b; br.append( s,             strlen( s )     ); return br; }
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( const stack_string<S>& b, const char    c           ) { stack_string<S> br = b; br.append( &c,            sizeof( c )     ); return br; }
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( const stack_string<S>& b, const wchar_t*s           ) { stack_string<S> br = b; br.append( s,             wcslen( s )     ); return br; }
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( const stack_string<S>& b, const wchar_t c           ) { stack_string<S> br = b; br.append( &c,            sizeof( c ) / sizeof( wchar_t )   ); return br; }
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( const stack_string<S>& b, const std::string&  s     ) { stack_string<S> br = b; br.append( s.c_str(),     s.size()        ); return br; }
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( const stack_string<S>& b, const std::wstring& s     ) { stack_string<S> br = b; br.append( s.c_str(),     s.size()        ); return br; }
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( const stack_string<S>& b, const std::vector<char>& v) { stack_string<S> br = b; br.append( v.data(),      v.size()        ); return br; }
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( const stack_string<S>& b, const std::string_view s  ) { stack_string<S> br = b; br.append( s.data(),      s.size()        ); return br; }
    
    template<std::size_t S = 256>
    inline stack_string<S> operator+                ( const char*   s           , const stack_string<S>& b) { stack_string<S> br( s ); br += b; return br; }
    template<std::size_t S = 256>                                                                                                
    inline stack_string<S> operator+                ( const char    c           , const stack_string<S>& b) { stack_string<S> br( c ); br += b; return br; }
    template<std::size_t S = 256>                                                                                                
    inline stack_string<S> operator+                ( const wchar_t*s           , const stack_string<S>& b) { stack_string<S> br( s ); br += b; return br; }
    template<std::size_t S = 256>                                                                                                
    inline stack_string<S> operator+                ( const wchar_t c           , const stack_string<S>& b) { stack_string<S> br( c ); br += b; return br; }
    template<std::size_t S = 256>                                                                                                
    inline stack_string<S> operator+                ( const std::string&  s     , const stack_string<S>& b) { stack_string<S> br( s ); br += b; return br; }
    template<std::size_t S = 256>                                                                                                
    inline stack_string<S> operator+                ( const std::wstring& s     , const stack_string<S>& b) { stack_string<S> br( s ); br += b; return br; }
    template<std::size_t S = 256>                                                                                                
    inline stack_string<S> operator+                ( const std::vector<char>& v, const stack_string<S>& b) { stack_string<S> br( v ); br += b; return br; }
    template<std::size_t S = 256>                                                                                                
    inline stack_string<S> operator+                ( const std::string_view s  , const stack_string<S>& b) { stack_string<S> br( s ); br += b; return br; }
    // clang-format on

    // clang-format off
    // ==
    template<std::size_t S = 256>
    inline bool         operator==                  ( const stack_string<S>& b, const stack_string<S>& b2 ) { return b.is_equal( b2.data(), b2.size()       ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const stack_string<S>& b, const char*   s           ) { return b.is_equal( s,         strlen( s )     ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const stack_string<S>& b, const char    c           ) { return b.is_equal( &c,        sizeof( c )     ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const stack_string<S>& b, const wchar_t*s           ) { return b.is_equal( s,         wcslen( s )     ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const stack_string<S>& b, const wchar_t c           ) { return b.is_equal( &c,        sizeof( c ) / sizeof( wchar_t ) ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const stack_string<S>& b, const std::string&  s     ) { return b.is_equal( s.c_str(), s.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const stack_string<S>& b, const std::wstring& s     ) { return b.is_equal( s.c_str(), s.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const stack_string<S>& b, const std::vector<char>& v) { return b.is_equal( v.data(),  v.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const stack_string<S>& b, const std::string_view s  ) { return b.is_equal( s.data(),  s.size()        ); }
                                                                                                              
    template<std::size_t S = 256>
    inline bool         operator==                  ( const char*   s           , const stack_string<S>& b) { return b.is_equal( s,         strlen( s )     ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const char    c           , const stack_string<S>& b) { return b.is_equal( &c,        sizeof( c )     ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const wchar_t*s           , const stack_string<S>& b) { return b.is_equal( s,         wcslen( s )     ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const wchar_t c           , const stack_string<S>& b) { return b.is_equal( &c,        sizeof( c ) / sizeof( wchar_t ) ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const std::string&  s     , const stack_string<S>& b) { return b.is_equal( s.c_str(), s.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const std::wstring& s     , const stack_string<S>& b) { return b.is_equal( s.c_str(), s.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const std::vector<char>& v, const stack_string<S>& b) { return b.is_equal( v.data(),  v.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator==                  ( const std::string_view s  , const stack_string<S>& b) { return b.is_equal( s.data(),  s.size()        ); }
    // clang-format on

    // clang-format off
    // !=
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const stack_string<S>& b, const stack_string<S>& b2 ) { return !b.is_equal( b2.data(),b2.size()       ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const stack_string<S>& b, const char*   s           ) { return !b.is_equal( s,        strlen( s )     ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const stack_string<S>& b, const char    c           ) { return !b.is_equal( &c,       sizeof( c )     ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const stack_string<S>& b, const wchar_t*s           ) { return !b.is_equal( s,        wcslen( s )     ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const stack_string<S>& b, const wchar_t c           ) { return !b.is_equal( &c,       sizeof( c ) / sizeof( wchar_t ) ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const stack_string<S>& b, const std::string&  s     ) { return !b.is_equal( s.c_str(),s.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const stack_string<S>& b, const std::wstring& s     ) { return !b.is_equal( s.c_str(),s.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const stack_string<S>& b, const std::vector<char>& v) { return !b.is_equal( v.data(), v.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const stack_string<S>& b, const std::string_view s  ) { return !b.is_equal( s.data(), s.size()        ); }
                                                                                                     
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const char*   s           , const stack_string<S>& b) { return !b.is_equal( s,        strlen( s )     ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const char    c           , const stack_string<S>& b) { return !b.is_equal( &c,       sizeof( c )     ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const wchar_t*s           , const stack_string<S>& b) { return !b.is_equal( s,        wcslen( s )     ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const wchar_t c           , const stack_string<S>& b) { return !b.is_equal( &c,       sizeof( c ) / sizeof( wchar_t ) ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const std::string&  s     , const stack_string<S>& b) { return !b.is_equal( s.c_str(),s.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const std::wstring& s     , const stack_string<S>& b) { return !b.is_equal( s.c_str(),s.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const std::vector<char>& v, const stack_string<S>& b) { return !b.is_equal( v.data(), v.size()        ); }
    template<std::size_t S = 256>
    inline bool         operator!=                  ( const std::string_view s  , const stack_string<S>& b) { return !b.is_equal( s.data(), s.size()        ); }
    // clang-format on

    // clang-format off
     // < 
    template<std::size_t S = 256>
    inline bool         operator<                   ( const stack_string<S>& b, const stack_string<S>& b2 ) { return b.compare( b2.data(),  b2.size()       ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const stack_string<S>& b, const char*   s           ) { return b.compare( s,          strlen( s )     ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const stack_string<S>& b, const char    c           ) { return b.compare( &c,         sizeof( c )     ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const stack_string<S>& b, const wchar_t*s           ) { return b.compare( s,          wcslen( s )     ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const stack_string<S>& b, const wchar_t c           ) { return b.compare( &c,         sizeof( c ) / sizeof( wchar_t ) ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const stack_string<S>& b, const std::string&  s     ) { return b.compare( s.c_str(),  s.size()        ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const stack_string<S>& b, const std::wstring& s     ) { return b.compare( s.c_str(),  s.size()        ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const stack_string<S>& b, const std::vector<char>& v) { return b.compare( v.data(),   v.size()        ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const stack_string<S>& b, const std::string_view s  ) { return b.compare( s.data(),   s.size()        ) < 0; }
                                                                                                               
    template<std::size_t S = 256>
    inline bool         operator<                   ( const char*   s           , const stack_string<S>& b) { return b.compare( s,          strlen( s )     ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const char    c           , const stack_string<S>& b) { return b.compare( &c,         sizeof( c )     ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const wchar_t*s           , const stack_string<S>& b) { return b.compare( s,          wcslen( s )     ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const wchar_t c           , const stack_string<S>& b) { return b.compare( &c,         sizeof( c ) / sizeof( wchar_t ) ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const std::string&  s     , const stack_string<S>& b) { return b.compare( s.c_str(),  s.size()        ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const std::wstring& s     , const stack_string<S>& b) { return b.compare( s.c_str(),  s.size()        ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const std::vector<char>& v, const stack_string<S>& b) { return b.compare( v.data(),   v.size()        ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator<                   ( const std::string_view s  , const stack_string<S>& b) { return b.compare( s.data(),   s.size()        ) >= 0; }
    // clang-format on

    // clang-format off
    // <= 
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const stack_string<S>& b, const stack_string<S>& b2 ) { return b.compare( b2.data(),  b2.size()       ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const stack_string<S>& b, const char*   s           ) { return b.compare( s,          strlen( s )     ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const stack_string<S>& b, const char    c           ) { return b.compare( &c,         sizeof( c )     ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const stack_string<S>& b, const wchar_t*s           ) { return b.compare( s,          wcslen( s )     ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const stack_string<S>& b, const wchar_t c           ) { return b.compare( &c,         sizeof( c ) / sizeof( wchar_t ) ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const stack_string<S>& b, const std::string&  s     ) { return b.compare( s.c_str(),  s.size()        ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const stack_string<S>& b, const std::wstring& s     ) { return b.compare( s.c_str(),  s.size()        ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const stack_string<S>& b, const std::vector<char>& v) { return b.compare( v.data(),   v.size()        ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const stack_string<S>& b, const std::string_view s  ) { return b.compare( s.data(),   s.size()        ) <= 0; }
                                                                          
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const char*   s           , const stack_string<S>& b) { return b.compare( s,          strlen( s )     ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const char    c           , const stack_string<S>& b) { return b.compare( &c,         sizeof( c )     ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const wchar_t*s           , const stack_string<S>& b) { return b.compare( s,          wcslen( s )     ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const wchar_t c           , const stack_string<S>& b) { return b.compare( &c,         sizeof( c ) / sizeof( wchar_t ) ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const std::string&  s     , const stack_string<S>& b) { return b.compare( s.c_str(),  s.size()        ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const std::wstring& s     , const stack_string<S>& b) { return b.compare( s.c_str(),  s.size()        ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const std::vector<char>& v, const stack_string<S>& b) { return b.compare( v.data(),   v.size()        ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator<=                  ( const std::string_view s  , const stack_string<S>& b) { return b.compare( s.data(),   s.size()        ) > 0; }
    // clang-format on

    // clang-format off
    // >
    template<std::size_t S = 256>
    inline bool         operator>                   ( const stack_string<S>& b, const stack_string<S>& b2 ) { return b.compare( b2.data(),  b2.size()       ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const stack_string<S>& b, const char*   s           ) { return b.compare( s,          strlen( s )     ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const stack_string<S>& b, const char    c           ) { return b.compare( &c,         sizeof( c )     ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const stack_string<S>& b, const wchar_t*s           ) { return b.compare( s,          wcslen( s )     ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const stack_string<S>& b, const wchar_t c           ) { return b.compare( &c, sizeof( c ) / sizeof( wchar_t ) ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const stack_string<S>& b, const std::string&  s     ) { return b.compare( s.c_str(),   s.size()       ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const stack_string<S>& b, const std::wstring& s     ) { return b.compare( s.c_str(),   s.size()       ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const stack_string<S>& b, const std::vector<char>& v) { return b.compare( v.data(),    v.size()       ) > 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const stack_string<S>& b, const std::string_view s  ) { return b.compare( s.data(),    s.size()       ) > 0; }
                                                                                                              
    template<std::size_t S = 256>
    inline bool         operator>                   ( const char*   s           , const stack_string<S>& b) { return b.compare( s,          strlen( s )     ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const char    c           , const stack_string<S>& b) { return b.compare( &c,         sizeof( c )     ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const wchar_t*s           , const stack_string<S>& b) { return b.compare( s,          wcslen( s )     ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const wchar_t c           , const stack_string<S>& b) { return b.compare( &c,         sizeof( c ) / sizeof( wchar_t ) ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const std::string&  s     , const stack_string<S>& b) { return b.compare( s.c_str(),  s.size()        ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const std::wstring& s     , const stack_string<S>& b) { return b.compare( s.c_str(),  s.size()        ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const std::vector<char>& v, const stack_string<S>& b) { return b.compare( v.data(),   v.size()        ) <= 0; }
    template<std::size_t S = 256>
    inline bool         operator>                   ( const std::string_view s  , const stack_string<S>& b) { return b.compare( s.data(),   s.size()        ) <= 0; }
    // clang-format on

    // clang-format off
    // >= 
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const stack_string<S>& b, const stack_string<S>& b2 ) { return b.compare( b2.data(),  b2.size()       ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const stack_string<S>& b, const char*   s           ) { return b.compare( s,          strlen( s )     ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const stack_string<S>& b, const char    c           ) { return b.compare( &c,         sizeof( c )     ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const stack_string<S>& b, const wchar_t*s           ) { return b.compare( s,          wcslen( s )     ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const stack_string<S>& b, const wchar_t c           ) { return b.compare( &c,         sizeof( c ) / sizeof( wchar_t ) ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const stack_string<S>& b, const std::string&  s     ) { return b.compare( s.c_str(),  s.size()        ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const stack_string<S>& b, const std::wstring& s     ) { return b.compare( s.c_str(),  s.size()        ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const stack_string<S>& b, const std::vector<char>& v) { return b.compare( v.data(),   v.size()        ) >= 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const stack_string<S>& b, const std::string_view s  ) { return b.compare( s.data(),   s.size()        ) >= 0; }
                                                                        
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const char*   s           , const stack_string<S>& b) { return b.compare( s,          strlen( s )     ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const char    c           , const stack_string<S>& b) { return b.compare( &c,         sizeof( c )     ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const wchar_t*s           , const stack_string<S>& b) { return b.compare( s,          wcslen( s )     ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const wchar_t c           , const stack_string<S>& b) { return b.compare( &c,         sizeof( c ) / sizeof( wchar_t ) ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const std::string&  s     , const stack_string<S>& b) { return b.compare( s.c_str(),  s.size()        ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const std::wstring& s     , const stack_string<S>& b) { return b.compare( s.c_str(),  s.size()        ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const std::vector<char>& v, const stack_string<S>& b) { return b.compare( v.data(),   v.size()        ) < 0; }
    template<std::size_t S = 256>
    inline bool         operator>=                  ( const std::string_view s  , const stack_string<S>& b) { return b.compare( s.data(),   s.size()        ) < 0; }
    // clang-format on

} // namespace small
