#include <iconv.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>

#include "wstring.hh"

namespace
{
    const char *const midset
      = (*(const short *)"\1\0\0\0\0\0\0\0" == 1) ? "UCS-4LE" : "UCS-4BE";
}

wstringOut::wstringOut() : charset(midset)
{
    converter = iconv_open(charset.c_str(), midset);
    tester = iconv_open(charset.c_str(), midset);
}

wstringOut::wstringOut(const char *setname) : charset(midset)
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

const string wstringOut::putc(ucs4 p) const
{
    wstring tmp;
    tmp += p;
    return puts(tmp);
}

const string wstringOut::puts(const wstring &s) const
{
    const char *input = (const char *)(s.data());
    size_t left = s.size() * sizeof(ucs4);
    string result;
    while(left > 0)
    {
        char OutBuf[4096], *outptr = OutBuf;
        unsigned outsize = sizeof OutBuf;
        size_t retval = iconv(converter,
                              const_cast<char **> (&input),
                              &left,
                              &outptr,
                              &outsize);
        
        string tmp(OutBuf, outptr-OutBuf);
        result += tmp;
        
        if(retval == (size_t)-1)
        {
            if(errno == E2BIG)
            {
                continue;
            }
            if(errno == EILSEQ)
            {
                input += sizeof(ucs4);
                left -= sizeof(ucs4);
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

bool wstringOut::isok(ucs4 p) const
{
    char OutBuf[256], *outptr = OutBuf;
    const char *tmp = (const char *)(&p);
    unsigned outsize = sizeof OutBuf;
    unsigned insize = sizeof(p);
    size_t retval = iconv(tester,
                          const_cast<char **> (&tmp),
                          &insize,
                          &outptr,
                          &outsize);
    if(retval == (size_t)-1)return false;
    return true;
}

wstringIn::wstringIn() : charset(midset)
{
    converter = iconv_open(midset, charset.c_str());
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

const wstring wstringIn::putc(char p) const
{
    string tmp;
    tmp += p;
    return puts(tmp);
}

const wstring wstringIn::puts(const string &s) const
{
    const char *input = (const char *)(s.data());
    size_t left = s.size();
    wstring result;
    while(left > 0)
    {
        char OutBuf[4096], *outptr = OutBuf;
        unsigned outsize = sizeof OutBuf;
        size_t retval = iconv(converter,
                              const_cast<char **> (&input),
                              &left,
                              &outptr,
                              &outsize);
        
        //unsigned bytes = (sizeof OutBuf) - outsize;
        unsigned bytes = outptr-OutBuf;
        wstring tmp((const ucs4 *)(&OutBuf), bytes / (sizeof(ucs4)));
        result += tmp;
        
        if(retval == (size_t)-1)
        {
            if(errno == E2BIG)
            {
                continue;
            }
            if(errno == EILSEQ)
            {
                input += sizeof(ucs4);
                left -= sizeof(ucs4);
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

wstring AscToWstr(const string &s)
{
    wstring result;
    for(unsigned a=0; a<s.size(); ++a)
        result += AscToWchar(s[a]);
    return result;
}

string WstrToAsc(const wstring &s)
{
    string result;
    for(unsigned a=0; a<s.size(); ++a)
        result += WcharToAsc(s[a]);
    return result;
}

char WcharToAsc(ucs4 c)
{
    // jis ascii
    if(c >= 0xFF10 && c <= 0xFF19) return c - 0xFF10 + '0';
    if(c >= 0xFF21 && c <= 0xFF26) return c - 0xFF21 + 'A';
    if(c >= 0xFF41 && c <= 0xFF46) return c - 0xFF41 + 'a';
    // default
    return static_cast<char> (c);
}

ucs4 AscToWchar(char c)
{
    return static_cast<ucs4> (static_cast<unsigned char> (c));
}

long atoi(const ucs4 *p, int base)
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

int Whex(ucs4 p)
{
    char c = WcharToAsc(p);
    if(c >= '0' && c <= '9') return (c-'0');
    if(c >= 'A' && c <= 'Z') return (c+10-'A');
    if(c >= 'a' && c <= 'z') return (c+10-'a');
    return -1;
}

