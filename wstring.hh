#ifndef bqtWstringH
#define bqtWstringH

#include <string>

// Note: address of ilseq must be available
// Thus don't make this #define
static const wchar_t ilseq = 0xFFFD;
static const wchar_t ucsig = 0xFEFF;

#define CLEARSTR(x) x.clear()


#include <iconv.h>

#undef putc
#undef puts

/* An output class */
class wstringOut
{
    mutable iconv_t converter;
    mutable iconv_t tester;
public:
    std::string charset;
    
    wstringOut();
    wstringOut(const char *setname);
    ~wstringOut();
    
    void SetSet(const char *setname);
    
    const std::string putc(wchar_t p) const;
    const std::string puts(const std::wstring &s) const;
    bool isok(wchar_t p) const;
private:
    wstringOut(const wstringOut&);
    const wstringOut& operator=(const wstringOut&);
};

/* An input class */
class wstringIn
{
    iconv_t converter;
public:
    std::string charset;
    
    wstringIn();
    wstringIn(const char *setname);
    ~wstringIn();
    
    void SetSet(const char *setname);

    const std::wstring putc(char p) const;
    const std::wstring puts(const std::string &s) const;
private:
    wstringIn(const wstringIn&);
    const wstringIn& operator=(const wstringIn&);
};

extern const std::wstring AscToWstr(const std::string &s);
extern const std::string WstrToAsc(const std::wstring &s);
extern char WcharToAsc(wchar_t c);
inline wchar_t AscToWchar(char c)
{ return static_cast<wchar_t> (static_cast<unsigned char> (c)); }
extern long atoi(const wchar_t *p, int base=10);
extern int Whex(wchar_t p);

const std::wstring wformat(const wchar_t* fmt, ...);
const std::string   format(const char* fmt, ...);

#endif

