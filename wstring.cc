#include <iconv.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#include "wstring.hh"

#ifdef WIN32
# define ICONV_INPUTTYPE const char **
#else
# define ICONV_INPUTTYPE char **
#endif

namespace
{
    const char *const midset
      = (*(const short *)"\1\0\0\0\0\0\0\0" == 1) ? "UCS-4LE" : "UCS-4BE";
}

wstringOut::wstringOut() : converter(), tester(), charset(midset)
{
    converter = iconv_open(charset.c_str(), midset);
    tester = iconv_open(charset.c_str(), midset);
}

wstringOut::wstringOut(const char *setname) : converter(), tester(), charset(midset)
{
    converter = iconv_open(charset.c_str(), midset);
    tester = iconv_open(charset.c_str(), midset);
    SetSet(setname);
}

wstringOut::~wstringOut()
{
    iconv_close(converter);
    iconv_close(tester);
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

const std::string wstringOut::putc(wchar_t p) const
{
    std::wstring tmp;
    tmp += p;
    return puts(tmp);
}

const std::string wstringOut::puts(const std::wstring &s) const
{
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
}

bool wstringOut::isok(wchar_t p) const
{
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
}

wstringIn::wstringIn()
   : converter(iconv_open(midset, midset)), charset(midset)
{
}

wstringIn::wstringIn(const char *setname)
   : converter(iconv_open(midset, midset)), charset(midset)
{
    SetSet(setname);
}

wstringIn::~wstringIn()
{
    iconv_close(converter);
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

const std::wstring wstringIn::putc(char p) const
{
    std::string tmp;
    tmp += p;
    return puts(tmp);
}

const std::wstring wstringIn::puts(const std::string &s) const
{
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
}

int Whex(wchar_t p)
{
    char c = WcharToAsc(p);
    if(c >= '0' && c <= '9') return (c-'0');
    if(c >= 'A' && c <= 'Z') return (c+10-'A');
    if(c >= 'a' && c <= 'z') return (c+10-'a');
    return -1;
}

const std::wstring wformat(const wchar_t* fmt, ...)
{
    wchar_t Buf[4096];
    va_list ap;
    va_start(ap, fmt);
    vswprintf(Buf, 4096, fmt, ap);
    va_end(ap);
    return Buf;
}

const std::string format(const char* fmt, ...)
{
    char Buf[4096];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(Buf, 4096, fmt, ap);
    va_end(ap);
    return Buf;
}

