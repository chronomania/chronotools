#ifndef bqtCTcsetH
#define bqtCTcsetH

#include "wstring.hh"

typedef unsigned short ctchar;

typedef basic_string<ctchar> ctstring;

extern const char *getcharset();
extern void setcharset(const char *newcsetname);

enum cset_class { cset_8pix, cset_12pix };

// Note: Only values 0xA0..0xFF are worth using.
extern ucs4 getucs4(ctchar, cset_class=cset_12pix);

// Note: Returns 0 for nonpresentible chars.
extern ctchar getchronochar(ucs4, cset_class=cset_12pix);

extern unsigned get_font_begin();
extern unsigned get_font_end();

unsigned CalcSize(const ctstring &word);
const string GetString(const ctstring &word);

#endif
