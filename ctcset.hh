#ifndef bqtCTcsetH
#define bqtCTcsetH

#include "wstring.hh"

extern const char *getcharset();
extern void setcharset(const char *newcsetname);

// Note: Only values 0xA0..0xFF are worth using.
extern ucs4 getucs4(unsigned char chronochar);

// Note: Returns 0 for nonpresentible chars.
extern unsigned char getchronochar(ucs4 c);

extern unsigned get_num_chronochars();

#endif
