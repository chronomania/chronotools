#include <map>
#include <string>

#include "ctcset.hh"

/* These are language-specific settings! */

#include "config.hh"

/* Language-specific settings end here. */
namespace
{
    std::string CharSet = "iso-8859-1";

    class CharacterSet
    {
        wstring cset;
        std::map<ucs4, unsigned char> revmap;
    public:
        CharacterSet()
        {
        }
        
        void Init()
        {
            cset = GetConf("font", "font12_00").SField();
            cset+= GetConf("font", "font12_10").SField();
            cset+= GetConf("font", "font12_20").SField();
            cset+= GetConf("font", "font12_30").SField();
            cset+= GetConf("font", "font12_40").SField();
            cset+= GetConf("font", "font12_50").SField();
            cset+= GetConf("font", "font12_60").SField();
            cset+= GetConf("font", "font12_70").SField();
            cset+= GetConf("font", "font12_80").SField();
            cset+= GetConf("font", "font12_90").SField();
            cset+= GetConf("font", "font12_A0").SField();
            cset+= GetConf("font", "font12_B0").SField();
            cset+= GetConf("font", "font12_C0").SField();
            cset+= GetConf("font", "font12_D0").SField();
            cset+= GetConf("font", "font12_E0").SField();
            cset+= GetConf("font", "font12_F0").SField();
            
            if(cset.size() != 256)
            {
                fprintf(stderr, "ctcset error: Configuration not set properly!\n");
            }
             
            wstring noncharstr = GetConf("font", "nonchar");
            ucs4 nonchar = noncharstr[0];
            
            for(unsigned a = 0; a < cset.size(); ++a)
            {
                ucs4 c = cset[a];
                if(c == nonchar) { cset[a] = c = ilseq; }
                if(c != ilseq)revmap[c] = a;
            }
            fprintf(stderr, "Built charset map\n");
        }
        ucs4 operator[] (unsigned char ind)
        {
            if(!cset.size()) Init();
            return cset[ind];
        }
        unsigned char find(ucs4 p)
        {
            if(p == ilseq)return 0;
            
            if(!cset.size()) Init();
            
            map<ucs4, unsigned char>::const_iterator i;
            i = revmap.find(p);
            if(i == revmap.end())return 0;
            return i->second;
        }
    } characterset;
}

const char *getcharset()
{
    return CharSet.c_str();
}

void setcharset(const char *newcset)
{
    CharSet = newcset;
}

// Note: Only values 0xA0..0xFF are worth using.
ucs4 getucs4(unsigned char chronochar)
{
    return characterset[chronochar];
}

// Note: Returns 0 for nonpresentible chars.
unsigned char getchronochar(ucs4 c)
{
    return characterset.find(c);
}

unsigned get_num_chronochars()
{
    // Standard defines that this'll be initialized upon the first call.
    static const unsigned cache = GetConf("font", "num_characters");
    return cache;
}
