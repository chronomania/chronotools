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
        vector<unsigned char> revmapfirst;
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
            
            revmapfirst.clear();
            revmapfirst.resize(GetConf("font", "charcachesize"), 0);
             
            wstring noncharstr = GetConf("font", "nonchar");
            ucs4 nonchar = noncharstr[0];
            
            for(unsigned a = 0; a < cset.size(); ++a)
            {
                ucs4 c = cset[a];
                if(c == nonchar) { cset[a] = c = ilseq; }
                if((unsigned)c < revmapfirst.size()) revmapfirst[(unsigned)c] = a;
                else if(c != ilseq)revmap[c] = a;
            }
            fprintf(stderr, "Built charset map (%u noncached)\n", revmap.size());
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
            
            if((unsigned)p < revmapfirst.size()) return revmapfirst[(unsigned)p];
            
            map<ucs4, unsigned char>::const_iterator i;
            i = revmap.find(p);
            if(i == revmap.end())return 0;
            return i->second;
        }
    } characterset;
}

const char *getcharset()
{
#if 0
    fprintf(stderr, "Character set asked - returning '%s'\n", CharSet.c_str());
#endif
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
    unsigned char result = characterset.find(c);
    if(result == 0)
    {
        fprintf(stderr, "Error: Irrepresentible character '%c' (%X)\n", c, c);
    }
    else if(result < 0x100 - get_num_chronochars())
    {
        fprintf(stderr, "Error: Character %02X ('%c', %X) is outside visible character range\n",
            result, c, c);
    }
    return result;
}

unsigned get_num_chronochars()
{
    // Standard defines that this'll be initialized upon the first call.
    static const unsigned cache = GetConf("font", "num_characters");
    return cache;
}
