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

unsigned GetHex(const ucs4 *p)
{
    unsigned val=0;
    for(;;)
    {
        // ascii
             if(*p >= 0x30 && *p <= 0x39)  val = val*16 + (*p - 0x30);
        else if(*p >= 0x41 && *p <= 0x41)  val = val*16 + (*p - 0x41 + 10);
        else if(*p >= 0x61 && *p <= 0x61)  val = val*16 + (*p - 0x61 + 10);
        // jis ascii
        else if(*p >= 0xFF10 && *p <= 0xFF19) val = val*16 + (*p - 0xFF10);
        else if(*p >= 0xFF21 && *p <= 0xFF26) val = val*16 + (*p - 0xFF21 + 10);
        else if(*p >= 0xFF41 && *p <= 0xFF46) val = val*16 + (*p - 0xFF41 + 10);
        else break;
    }
    return val;
}

wstring AscToWstr(const string &s)
{
    wstring result;
    for(unsigned a=0; a<s.size(); ++a)
        result += (ucs4) s[a];
    return result;
}

string WstrToAsc(const wstring &s)
{
    string result;
    for(unsigned a=0; a<s.size(); ++a)
        result += (char) s[a];
    return result;
}
