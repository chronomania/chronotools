#include <cstring>

using std::memcmp;

#include "miscfun.hh"

const char *mempos(const char *haystack,unsigned haysize,
                   const char *needle,  unsigned needlesize)
{
    if(haysize >= needlesize)
    {
        unsigned poscountminus1 = haysize - needlesize;
        for(unsigned a=0; a<=poscountminus1; ++a)
            if(!memcmp(haystack+a, needle, needlesize))
                return haystack+a;
    }
    return NULL;
}

/* This is DarkForce's hashing code */
unsigned hashstr(const char *s, unsigned len)
{
    unsigned h = 0;
    for(unsigned a=0; a<len; ++a)
    {
        unsigned char c = s[a];
        c = h ^ c;
        h ^= (c * 707106);
    }
    return (h&0x0000FF00) | ((h>>16)&0xFF);
    // Paluuarvo on siis 16-bittinen hash!
}
