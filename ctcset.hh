#ifndef bqtCTcsetH
#define bqtCTcsetH

#include <vector>

#include "wstring.hh"
#include "hash.hh"

typedef unsigned short ctchar;

typedef basic_string<ctchar> ctstring;

#if USE_ICONV
extern const char *getcharset();
extern void setcharset(const char *newcsetname);
#else
#define getcharset()
#define setcharset(x)
#endif

enum cset_class { cset_8pix, cset_12pix };

// Note: Only values 0xA0..0xFF are worth using.
wchar_t getwchar_t(ctchar, cset_class=cset_12pix);

// Note: Returns 0 for nonpresentible chars.
ctchar getctchar(wchar_t, cset_class=cset_12pix);

void RearrangeCharset(cset_class, const hash_map<ctchar, ctchar>& );

// Conversion without symbols-lookup.
const ctstring getctstring(const std::wstring&, cset_class=cset_12pix);

// Get a ctstring from a blob
const ctstring getctstring(const std::vector<unsigned char>& blob);

unsigned get_font_begin();
unsigned get_font_end();

unsigned CalcSize(const ctstring &word);

// Get a ROM-writable string out from a ctstring
const std::string GetString(const ctstring &word);


#endif
