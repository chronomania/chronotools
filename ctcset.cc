#include <map>
#include "ctcset.hh"

/* These are language-specific settings! */
static const char CharSet[] = "iso-8859-1";
static const char Font16[] =
    // This is supposed to be in CharSet.
    "ABCDEFGHIJKLMNOP"   // A0
    "QRSTUVWXYZabcdef"   // B0
    "ghijklmnopqrstuv"   // C0
#if 0
    "wxyz0123456789!?"   // D0
    "/«»:&()'.,=-+%¶ "   // E0  EE=musicsymbol
    "¶¶å#äöéÉÅ¶¶¶¶ÖÄ_";  // F0  F0=heartsymbol, F1=..., F2=infinity
#else
    "wxyz0123456789ÅÄ"   // D0
    "Ö«»:-()'.,åäöé¶ "   // E0  EE=musicsymbol
    "¶¶%É=&+#!?¶¶¶/¶_";  // F0  F0=heartsymbol, F1=..., F2=infinity
#endif

// Note: There is no more space for extra symbols in 8x8 font!
// This is enough for Danish, German and Swedish at least.

const char *getcharset() { return CharSet; }


/* Language-specific settings end here. */

static class CharacterSet
{
    wstring cset;
    map<ucs4, unsigned char> revmap;
public:
    CharacterSet()
    {
        wstringIn conv;
        conv.SetSet(CharSet);
        cset = conv.puts(Font16);
        for(unsigned a = 0; a < cset.size(); ++a)
        {
            ucs4 c = cset[a];
            if(c != ilseq)revmap[c] = 0xA0 + a;
        }
    }
    ucs4 operator[] (unsigned ind) const
    {
        if(ind >= 0x60)return ilseq;
        if(cset[ind] == '¶')return ilseq;
        return cset[ind];
    }
    unsigned char find(ucs4 p) const
    {
        map<ucs4, unsigned char>::const_iterator i;
        if(p == ilseq)return '¶';
        i = revmap.find(p);
        if(i == revmap.end())return '¶';
        return i->second;
    }
} characterset;

// Note: Only values 0xA0..0xFF are worth using.
ucs4 getucs4(unsigned char chronochar)
{
    if(chronochar < 0xA0)return ilseq;
    return characterset[chronochar - 0xA0];
}

// Note: Returns '¶' for nonpresentible chars.
unsigned char getchronochar(ucs4 c)
{
    return characterset.find(c);
}
