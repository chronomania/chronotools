#include <iconv.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cwchar>

#include "wstring.hh"

#if USE_ICONV
# ifdef WIN32
#  define ICONV_INPUTTYPE const char **
# else
#  define ICONV_INPUTTYPE char **
# endif

namespace
{
    static class midset
    {
        const char* set;
    public:
        midset(): set(NULL)
        {
            wchar_t tmp = 1;
            if(sizeof(wchar_t) == 4)
                set = (*(const char *)&tmp == 1) ? "UCS-4LE" : "UCS-4BE";
            else
                set = (*(const char *)&tmp == 1) ? "UCS-2LE" : "UCS-2BE";
        }
        inline operator const char*() const
        {
            return set;
        }
    } midset;
}
#else
/* not iconv */
#include <clocale>
#include <cstdlib>
static class EnsureLocale
{
public:
    EnsureLocale() { setlocale(LC_CTYPE, "");
                     setlocale(LC_MESSAGES, "");
                   }
} EnsureLocale;

#endif

wstringOut::wstringOut()
#if USE_ICONV
 : converter(), tester(), charset(midset)
#endif
{
#if USE_ICONV
    converter = iconv_open(charset.c_str(), midset);
    tester = iconv_open(charset.c_str(), midset);
#endif
}

#if USE_ICONV
wstringOut::wstringOut(const char *setname) : converter(), tester(), charset(midset)
{
    converter = iconv_open(charset.c_str(), midset);
    tester = iconv_open(charset.c_str(), midset);
    SetSet(setname);
}
void wstringOut::SetSet(const char *setname)
{
    iconv_close(converter);
    iconv_close(tester);
    charset = setname;
    converter = iconv_open(setname, midset);
    if(converter == (iconv_t)(-1))
    {
        perror("wstringOut::SetSet::iconv_open");
        exit(1);
    }
    tester = iconv_open(setname, midset);
}
#endif

wstringOut::~wstringOut()
{
#if USE_ICONV
    iconv_close(converter);
    iconv_close(tester);
#endif
}
    
const std::string wstringOut::putc(wchar_t p) const
{
#if USE_ICONV
    std::wstring tmp;
    tmp += p;
    return puts(tmp);
#else
    char* Buf = (char*)alloca(MB_CUR_MAX);
    int n = wctomb(Buf, p);
    if(n < 0) return "";
    return std::string(Buf, Buf+n);
#endif
}

const std::string wstringOut::puts(const std::wstring &s) const
{
#if USE_ICONV
    const char *input = (const char *)(s.data());
    size_t left = s.size() * sizeof(wchar_t);
    std::string result;
    while(left > 0)
    {
        char OutBuf[4096], *outptr = OutBuf;
        unsigned outsize = sizeof OutBuf;
        size_t retval = iconv(converter,
                              const_cast<ICONV_INPUTTYPE> (&input),
                              &left,
                              &outptr,
                              &outsize);
        
        std::string tmp(OutBuf, outptr-OutBuf);
        result += tmp;
        
        if(retval == (size_t)-1)
        {
            if(errno == E2BIG)
            {
                continue;
            }
            if(errno == EILSEQ)
            {
                input += sizeof(wchar_t);
                left -= sizeof(wchar_t);
                result += '?';
            }
            if(errno == EINVAL)
            {
                /* Got partial byte */
            }
        }
    }
    return result;
#else
    const wchar_t *ptr = s.data(), *end = ptr + s.size();
    std::string result;
    while(ptr < end)
    {
        char Buf[4096];
        const wchar_t *begin = ptr;
        size_t n = std::wcsrtombs(Buf, &ptr, (sizeof Buf) / sizeof(Buf[0]), NULL);
        bool error = n == (size_t)-1;
        if(!error)
        {
            result.insert(result.end(), Buf, Buf+n);
            continue;
        }
        for(unsigned n2 = ptr - begin; n2 > 0; --n2)
        {
            n = std::wctomb(Buf, *ptr++);
            result.insert(result.end(), Buf, Buf + n);
        }
        if(*ptr) { result += '?'; ++ptr; }
    }
    return result;
#endif
}

bool wstringOut::isok(wchar_t p) const
{
#if USE_ICONV
    char OutBuf[256], *outptr = OutBuf;
    const char *tmp = (const char *)(&p);
    unsigned outsize = sizeof OutBuf;
    unsigned insize = sizeof(p);
    size_t retval = iconv(tester,
                          const_cast<ICONV_INPUTTYPE> (&tmp),
                          &insize,
                          &outptr,
                          &outsize);
    if(retval == (size_t)-1)return false;
    return true;
#else
    char* Buf = (char*)alloca(MB_CUR_MAX);
    return wctomb(Buf, p) >= 0;
#endif
}

wstringIn::wstringIn()
#if USE_ICONV
   : converter(iconv_open(midset, midset)), charset(midset)
#endif
{
#if !USE_ICONV
    std::mbtowc(NULL, NULL, 0);
#endif
}

#if USE_ICONV
wstringIn::wstringIn(const char *setname)
   : converter(iconv_open(midset, midset)), charset(midset)
{
    SetSet(setname);
}
void wstringIn::SetSet(const char *setname)
{
    iconv_close(converter);
    charset = setname;
    converter = iconv_open(midset, setname);
    if(converter == (iconv_t)(-1))
    {
        perror("wstringIn::SetSet::iconv_open");
        exit(1);
    }
}
#endif

wstringIn::~wstringIn()
{
#if USE_ICONV
    iconv_close(converter);
#endif
}

const std::wstring wstringIn::putc(char p) const
{
    std::string tmp;
    tmp += p;
    return puts(tmp);
}

const std::wstring wstringIn::puts(const std::string &s) const
{
#if USE_ICONV
    const char *input = (const char *)(s.data());
    size_t left = s.size();
    std::wstring result;
    while(left > 0)
    {
        char OutBuf[4096], *outptr = OutBuf;
        unsigned outsize = sizeof OutBuf;
        size_t retval = iconv(converter,
                              const_cast<ICONV_INPUTTYPE> (&input),
                              &left,
                              &outptr,
                              &outsize);
        
        //unsigned bytes = (sizeof OutBuf) - outsize;
        unsigned bytes = outptr-OutBuf;
        std::wstring tmp((const wchar_t *)(&OutBuf), bytes / (sizeof(wchar_t)));
        result += tmp;
        
        if(retval == (size_t)-1)
        {
            if(errno == E2BIG)
            {
                continue;
            }
            if(errno == EILSEQ)
            {
                input += sizeof(wchar_t);
                left -= sizeof(wchar_t);
                result += ilseq;
            }
            if(errno == EINVAL)
            {
                /* Got partial byte */
            }
        }
    }
    return result;
#else
    std::wstring result;
    for(unsigned offs=0, left = s.size(); left > 0; )
    {
        wchar_t c;
        int n = std::mbtowc(&c, s.data()+offs, left);
        if(n < 0)
        {
            unsigned char byte = s.data()[offs];
            fprintf(stderr, "Ignoring %02X (%c)\n", byte, byte);
            ++offs; --left;
            result += ilseq;
            continue;
        }
        result += c;
        offs += n; left -= n;
    }
    return result;
#endif
}

const std::wstring AscToWstr(const std::string &s)
{
    std::wstring result;
    result.reserve(s.size());
    for(unsigned a=0; a<s.size(); ++a)
        result += AscToWchar(s[a]);
    return result;
}

const std::string WstrToAsc(const std::wstring &s)
{
    std::string result;
    result.reserve(s.size());
    for(unsigned a=0; a<s.size(); ++a)
        result += WcharToAsc(s[a]);
    return result;
}

char WcharToAsc(wchar_t c)
{
    // jis ascii
    if(c >= 0xFF10 && c <= 0xFF19) return c - 0xFF10 + '0';
    if(c >= 0xFF21 && c <= 0xFF26) return c - 0xFF21 + 'A';
    if(c >= 0xFF41 && c <= 0xFF46) return c - 0xFF41 + 'a';
    // default
    return static_cast<char> (c);
}

long atoi(const wchar_t *p, int base)
{
#ifndef WIN32
    return std::wcstol(p, 0, base);
#else
    return wcstol(p, 0, base);
#endif
#if 0
    long ret=0, sign=1;
    while(*p == '-') { sign=-sign; ++p; }
    for(; *p; ++p)
    {
        char c = WcharToAsc(*p);
        
        int p = Whex(c);
        if(p == -1)break;
        ret = ret*base + p;
    }
    return ret * sign;
#endif
}

const std::wstring wformat(const wchar_t* fmt, ...)
{
    wchar_t Buf[4096];
    va_list ap;
    va_start(ap, fmt);
#ifndef WIN32
    std::vswprintf(Buf, 4096, fmt, ap);
#else
    vswprintf(Buf, fmt, ap);
#endif
    va_end(ap);
    return Buf;
}

int Whex(wchar_t p)
{
    char c = WcharToAsc(p);
    if(c >= '0' && c <= '9') return (c-'0');
    if(c >= 'A' && c <= 'Z') return (c+10-'A');
    if(c >= 'a' && c <= 'z') return (c+10-'a');
    return -1;
}
