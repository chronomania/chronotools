#ifndef bqtCTcsetH
#define bqtCTcsetH

#include "wstring.hh"

extern const char *getcharset();

// Note: Only values 0xA0..0xFF are worth using.
extern ucs4 getucs4(unsigned char chronochar);

// Note: Returns '¶' for nonpresentible chars.
extern unsigned char getchronochar(ucs4 c);

#endif
