#include <map>
#include "ctcset.hh"

/* These are language-specific settings! */

#include "settings.hh"

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
                if(c != ilseq)revmap[c] = a;
            }
            fprintf(stderr, "Built charset map\n");
        }
        ucs4 operator[] (unsigned ind) const
        {
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
	// FIXME: This isn't multibyte safe
    if(Font16[chronochar] == '¶') return ilseq;
    
    return characterset[chronochar];
}

// Note: Returns '¶' for nonpresentible chars.
unsigned char getchronochar(ucs4 c)
{
    return characterset.find(c);
}
