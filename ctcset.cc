#include <map>
#include "ctcset.hh"

/* These are language-specific settings! */

namespace
{
    // Document character set
    const char CharSet[] = "iso-8859-1";

    // Map of chrono symbols to document set
    const char Font16[] =
     // This is supposed to be encoded in CharSet.
     "ABCDEFGH" "IJKLMNOP"   // A0
     "QRSTUVWX" "YZabcdef"   // B0
     "ghijklmn" "opqrstuv"   // C0
     "wxyz0123" "456789ÅÄ"   // D0
     "Ö«»:-()'" ".,åäöé¶ "   // E0  EE=musicsymbol
     "¶¶%É=&+#" "!?¶¶¶/¶_";  // F0  F0=heartsymbol, F1=..., F2=originally infinity
}

// Note: There is no more space for extra symbols in 8x8 font!
// This is enough for Danish, German and Swedish at least.

const char *getcharset() { return CharSet; }


/* Language-specific settings end here. */
namespace
{
    class CharacterSet
    {
        wstring cset;
        std::map<ucs4, unsigned char> revmap;
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
            fprintf(stderr, "Built charset map\n");
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
}

// Note: Only values 0xA0..0xFF are worth using.
ucs4 getucs4(unsigned char chronochar)
{
    if(chronochar < 0xA0)return ilseq;
    if(Font16[chronochar - 0xA0] == '¶') return ilseq;
    return characterset[chronochar - 0xA0];
}

// Note: Returns '¶' for nonpresentible chars.
unsigned char getchronochar(ucs4 c)
{
    return characterset.find(c);
}
