#ifndef bqtWstringH
#define bqtWstringH

#define __ENABLE_WSTRING 1
#include <string>

using namespace std;

////////////// From htmlrecode.hh
//#define wstring ucs4string
typedef wchar_t ucs4;
//typedef unsigned int ucs4;
//typedef basic_string<ucs4> wstring;

// Note: address of ilseq must be available
// Thus don't make this #define
static const ucs4 ilseq = 0xFFFD;
static const ucs4 ucsig = 0xFEFF;

#if 1
/* libstdc++ 2 way */
static const wstring emptywstring;
#define CLEARSTR(x) x=emptywstring
#else
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
    ~wstringOut();
    
    void SetSet(const char *setname);
    
    const string putc(ucs4 p) const;
    const string puts(const wstring &s) const;
    bool isok(ucs4 p) const;
};

/* An input class */
class wstringIn
{
    iconv_t converter;
public:
    string charset;
    
    wstringIn();
    ~wstringIn();
    
    void SetSet(const char *setname);

    const wstring putc(char p) const;
    const wstring puts(const string &s) const;
};

extern unsigned GetHex(const ucs4 *p);

extern wstring AscToWstr(const string &s);
extern string WstrToAsc(const wstring &s);

#endif
