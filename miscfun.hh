#ifndef ctMiscFunHH
#define ctMiscFunHH

#include "wstring.hh"

extern string str_replace(const string &search, unsigned char with, const string &where);
extern wstring str_replace(const wstring &search, const wstring &with, const wstring &where);

extern const char *mempos(const char *haystack,unsigned haysize,
                          const char *needle,  unsigned needlesize);
extern unsigned hashstr(const char *s, unsigned len);

#endif
