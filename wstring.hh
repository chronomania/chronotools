#ifndef bqtWstringH
#define bqtWstringH

// wstring methods:
//     0 = use wchar_t and wstring
//     1 = define ucs4 and use basic_string<ucs4>
//     2 = as 1, overload char_traits
//     3 = as 1, implement char_traits

#define WSTRING_METHOD 2

// clearstr methods:
//     0 = use static empty constant, works always
//     1 = use clear() method, present in libstdc++-3
#define CLEARSTR_METHOD 1


#if WSTRING_METHOD==0
#define __ENABLE_WSTRING 1
#else
#undef __ENABLE_WSTRING
#endif
#include <string>

using namespace std;

#if WSTRING_METHOD == 0
typedef wchar_t ucs4;
#define ucs4string wstring
#endif
#if WSTRING_METHOD >= 1
typedef unsigned int ucs4;
typedef basic_string<ucs4> ucs4string;
#endif

// Note: address of ilseq must be available
// Thus don't make this #define
static const ucs4 ilseq = 0xFFFD;
static const ucs4 ucsig = 0xFEFF;

#if CLEARSTR_METHOD==0
/* libstdc++ 2 way */
static const ucs4string emptyucs4string;
#define CLEARSTR(x) x=emptyucs4string
#endif
#if CLEARSTR_METHOD==1
/* libstdc++ 3 way */
#define CLEARSTR(x) x.clear()
#endif
////////////

#include <iconv.h>

#undef putc
#undef puts

/* An output class */
class wstringOut
{
    mutable iconv_t converter;
    mutable iconv_t tester;
public:
    string charset;
    
    wstringOut();
    wstringOut(const char *setname);
    ~wstringOut();
    
    void SetSet(const char *setname);
    
    const string putc(ucs4 p) const;
    const string puts(const ucs4string &s) const;
    bool isok(ucs4 p) const;
};

/* An input class */
class wstringIn
{
    iconv_t converter;
public:
    string charset;
    
    wstringIn();
    wstringIn(const char *setname);
    ~wstringIn();
    
    void SetSet(const char *setname);

    const ucs4string putc(char p) const;
    const ucs4string puts(const string &s) const;
};

extern ucs4string AscToWstr(const string &s);
extern string WstrToAsc(const ucs4string &s);
extern char WcharToAsc(ucs4 c);
extern ucs4 AscToWchar(char c);
extern long atoi(const ucs4 *p, int base=10);
extern int Whex(ucs4 p);

extern int ucs4cmp(const ucs4* s1, const ucs4* s2, size_t n);
extern size_t ucs4len(const ucs4* s);
extern const ucs4* ucs4chr(const ucs4* s, const ucs4 a, size_t n);
extern ucs4* ucs4move(ucs4* s1, const ucs4* s2, unsigned n);
extern ucs4* ucs4set(ucs4* s, size_t n, ucs4 a);
extern ucs4* ucs4cpy(ucs4* s1, const ucs4* s2, size_t n);

#if WSTRING_METHOD == 2

namespace std
{
  template<>
    struct char_traits<ucs4>
    {
      typedef ucs4          char_type;
      typedef int           int_type; 
      typedef streamoff     off_type; 
      typedef unsigned      pos_type; 
      typedef mbstate_t     state_type;
      
      static void
      assign(char_type& c1, const char_type& c2)
      { c1 = c2; }

      static bool
      eq(const char_type& c1, const char_type& c2)
      { return c1 == c2; }

      static bool
      lt(const char_type& c1, const char_type& c2)
      { return c1 < c2; }

      static int
      compare(const char_type* s1, const char_type* s2, size_t n)
      { return ucs4cmp(s1, s2, n); }

      static size_t
      length(const char_type* s)
      { return ucs4len(s); }

      static const char_type*
      find(const char_type* s, size_t n, const char_type& a)
      { return ucs4chr(s, a, n); }

      static char_type*
      move(char_type* s1, const char_type* s2, int_type n)
      { return ucs4move(s1, s2, n); }

      static char_type*
      copy(char_type* s1, const char_type* s2, size_t n)
      { return ucs4cpy(s1, s2, n); }

      static char_type*
      assign(char_type* s, size_t n, char_type a)
      { return ucs4set(s, a, n); }

      static char_type
      to_char_type(const int_type& c) { return char_type(c); }

      static int_type
      to_int_type(const char_type& c) { return int_type(c); }

      static bool
      eq_int_type(const int_type& c1, const int_type& c2)
      { return c1 == c2; }

      static int_type
      eof() { return -1; }

      static int_type
      not_eof(const int_type& c)
      { return eq_int_type(c, eof()) ? 0 : c; }
  };
}
#endif

#undef wstring
#define wstring USE_ucs4string_NOT_wstring
#undef _GLIBCPP_USE_WCHAR_T

#endif

