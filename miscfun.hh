#include <string>
using std::string;

extern string str_replace(const string &search, const char *with, const string &where);
extern const char *mempos(const char *haystack,unsigned haysize,
                          const char *needle,  unsigned needlesize);
