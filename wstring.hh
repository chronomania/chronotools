#ifndef bqtWstringH
#define bqtWstringH

#include <string>

using namespace std;

#define wstring ucs4string
typedef unsigned int ucs4;

/* Define the wstring type */
typedef basic_string<ucs4> wstring;

/* Define illegal character */
#define ilseq ((ucs4)(0xFFFD))

/* Define CLEARWSTR() -macro */
#if 1
/* libstdc++ 2 way */
static const wstring emptywstring;
#define CLEARWSTR(x) x=emptywstring
#else
/* libstdc++ 3 way */
#define CLEARWSTR(x) x.clear()
#endif
#define CLEARSTR(x) CLEARWSTR(x)

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
