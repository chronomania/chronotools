#ifndef bqtCTcsetH
#define bqtCTcsetH

#include "wstring.hh"

typedef unsigned short ctchar;

typedef basic_string<ctchar> ctstring;

extern const char *getcharset();
extern void setcharset(const char *newcsetname);

// Note: Only values 0xA0..0xFF are worth using.
extern ucs4 getucs4(ctchar chronochar);

// Note: Returns 0 for nonpresentible chars.
extern ctchar getchronochar(ucs4 c);

extern unsigned get_num_chronochars();
extern unsigned get_num_extrachars();

unsigned CalcSize(const ctstring &word);
const string GetString(const ctstring &word);

#endif
