#ifndef bqtWstringH
#define bqtWstringH

#include <string>

#define USE_ICONV 1

#if USE_ICONV
#include <iconv.h>
#endif

#undef putc
#undef puts

/* Converting a bytestream to wstring */
class wstringIn
{
#if USE_ICONV
    iconv_t converter;
    std::string charset;
#endif
public:
    wstringIn();
    ~wstringIn();
    
#if USE_ICONV
    wstringIn(const char *setname);
    void SetSet(const char *setname);
#else
    inline void SetSet() {}
#endif

    const std::wstring putc(char p) const;
    const std::wstring puts(const std::string &s) const;
private:
    wstringIn(const wstringIn&);
    const wstringIn& operator=(const wstringIn&);
};

/* Converting a wstring to bytestream */
class wstringOut
{
#if USE_ICONV
    mutable iconv_t converter;
    mutable iconv_t tester;
    std::string charset;
#endif
public:
    wstringOut();
    ~wstringOut();
    
#if USE_ICONV
    wstringOut(const char *setname);
    void SetSet(const char *setname);
#else
    inline void SetSet() {}
#endif
    
    const std::string putc(wchar_t p) const;
    const std::string puts(const std::wstring &s) const;
    bool isok(wchar_t p) const;
private:
    wstringOut(const wstringOut&);
    const wstringOut& operator=(const wstringOut&);
};

/* char to wchar_t conversions - ASCII to UNICODE conversion */
inline wchar_t AscToWchar(char c)
{ return static_cast<wchar_t> (static_cast<unsigned char> (c)); }
extern const std::wstring AscToWstr(const std::string &s);

/* wchar_t to char conversions - UNICODE to ASCII conversion */
extern const std::string WstrToAsc(const std::wstring &s);
extern char WcharToAsc(wchar_t c);

/* Converts the given hex digit, to integer */
extern int Whex(wchar_t p);

/* atoi() for wchar_t pointers */
extern long atoi(const wchar_t *p, int base=10);

/* swprintf() */
const std::wstring wformat(const wchar_t* fmt, ...);


/* UNICODE illegal character */
static const wchar_t ilseq = 0xFFFD;

/* UNICODE signature character */
static const wchar_t ucsig = 0xFEFF;

#endif
