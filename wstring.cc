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
    ucs4string tmp;
    tmp += p;
    return puts(tmp);
}

const string wstringOut::puts(const ucs4string &s) const
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

const ucs4string wstringIn::putc(char p) const
{
    string tmp;
    tmp += p;
    return puts(tmp);
}

const ucs4string wstringIn::puts(const string &s) const
{
    const char *input = (const char *)(s.data());
    size_t left = s.size();
    ucs4string result;
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
        ucs4string tmp((const ucs4 *)(&OutBuf), bytes / (sizeof(ucs4)));
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

ucs4string AscToWstr(const string &s)
{
    ucs4string result;
    for(unsigned a=0; a<s.size(); ++a)
        result += AscToWchar(s[a]);
    return result;
}

string WstrToAsc(const ucs4string &s)
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

int ucs4cmp(const ucs4* s1, const ucs4* s2, size_t n)
{
    for(unsigned c=0; c<n; ++c)
        if(s1[c] != s2[c])
            return s1[c] < s2[c] ? -1 : 1;
    return 0;
}

size_t ucs4len(const ucs4* s)
{
    size_t result=0;
    while(*s) { ++result; ++s; }
    return result;
}

const ucs4* ucs4chr(const ucs4* s, const ucs4 a, size_t n)
{
    for(unsigned c=0; c<n; ++c)if(s[c]==a)return s+c;
    return 0;
}

ucs4* ucs4cpy(ucs4* s1, const ucs4* s2, size_t n)
{
    for(unsigned c=0; c<n; ++c)s1[c] = s2[c];
    return s1;
}

ucs4* ucs4move(ucs4* s1, const ucs4* s2, unsigned n)
{
    if(s1 < s2) return ucs4cpy(s1,s2,n);
    for(unsigned c=n; c-->0; )s1[c] = s2[c];
    return s1;
}

ucs4* ucs4set(ucs4* s, ucs4 a, size_t n)
{
    for(unsigned c=0; c<n; ++c) s[c] = a;
    return s;
}

#if WSTRING_METHOD==3
namespace std
{
  template<> 
  int char_traits<ucs4>::
  compare(const ucs4* s1, const ucs4* s2, size_t n)
  { return ucs4cmp(s1,s2, n); }
  
  template<>
  ucs4* char_traits<ucs4>::
  copy(ucs4* s1, const ucs4* s2, size_t n)
  { return ucs4cpy(s1, s2, n); }
  
  template<>
  ucs4* char_traits<ucs4>::
  move(ucs4* s1, const ucs4* s2, size_t n)
  { return ucs4move(s1, s2, n); }
  
  template<>
  ucs4* char_traits<ucs4>::
  assign(ucs4* s, size_t n, ucs4 a)
  { return ucs4set(s, a, n); }
}
#endif
