#include <string>

using std::string;

#include "ctcset.hh"
#include "config.hh"
#include "hash.hh"

/* Language-specific settings end here. */
namespace
{
    string CharSet = "iso-8859-1";

    class CharacterSet
    {
        hash_map<ucs4, ctchar> revmap;
        vector<ctchar> revmapfirst;

	protected:
        ucs4string cset;
        virtual void GetCSet() = 0;
        
    public:
        CharacterSet()
        {
        }
        virtual ~CharacterSet()
        {
        }
       
        void Init()
        {
            GetCSet();
            
            revmapfirst.clear();
            revmapfirst.resize(GetConf("font", "charcachesize"), 0);
             
            ucs4string noncharstr = GetConf("font", "nonchar");
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
        ucs4 operator[] (ctchar ind)
        {
            if(cset.empty()) Init();
            return cset[ind];
        }
        ctchar find(ucs4 p)
        {
            if(p == ilseq)return 0;
            
            if(cset.empty()) Init();
            
            if((unsigned)p < revmapfirst.size()) return revmapfirst[(unsigned)p];
            
            hash_map<ucs4, ctchar>::const_iterator i;
            i = revmap.find(p);
            if(i == revmap.end())return 0;
            return i->second;
        }
    };

    class CharacterSet12: public CharacterSet
    {
    private:
        virtual void GetCSet()
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
            cset+= GetConf("font", "font12_100").SField();
            cset+= GetConf("font", "font12_110").SField();
            cset+= GetConf("font", "font12_120").SField();
            cset+= GetConf("font", "font12_130").SField();
            cset+= GetConf("font", "font12_140").SField();
            cset+= GetConf("font", "font12_150").SField();
            cset+= GetConf("font", "font12_160").SField();
            cset+= GetConf("font", "font12_170").SField();
            cset+= GetConf("font", "font12_180").SField();
            cset+= GetConf("font", "font12_190").SField();
            cset+= GetConf("font", "font12_1A0").SField();
            cset+= GetConf("font", "font12_1B0").SField();
            cset+= GetConf("font", "font12_1C0").SField();
            cset+= GetConf("font", "font12_1D0").SField();
            cset+= GetConf("font", "font12_1E0").SField();
            cset+= GetConf("font", "font12_1F0").SField();
            cset+= GetConf("font", "font12_200").SField();
            cset+= GetConf("font", "font12_210").SField();
            cset+= GetConf("font", "font12_220").SField();
            cset+= GetConf("font", "font12_230").SField();
            cset+= GetConf("font", "font12_240").SField();
            cset+= GetConf("font", "font12_250").SField();
            cset+= GetConf("font", "font12_260").SField();
            cset+= GetConf("font", "font12_270").SField();
            cset+= GetConf("font", "font12_280").SField();
            cset+= GetConf("font", "font12_290").SField();
            cset+= GetConf("font", "font12_2A0").SField();
            cset+= GetConf("font", "font12_2B0").SField();
            cset+= GetConf("font", "font12_2C0").SField();
            cset+= GetConf("font", "font12_2D0").SField();
            cset+= GetConf("font", "font12_2E0").SField();
            cset+= GetConf("font", "font12_2F0").SField();
            
            if(cset.size() != 0x300)
            {
                fprintf(stderr, "ctcset error: Configuration not set properly!\n");
            }
        }
    } cset12;
    
    class CharacterSet8: public CharacterSet
    {
    private:
        virtual void GetCSet()
        {
            cset = GetConf("font", "font8_00").SField();
            cset+= GetConf("font", "font8_10").SField();
            cset+= GetConf("font", "font8_20").SField();
            cset+= GetConf("font", "font8_30").SField();
            cset+= GetConf("font", "font8_40").SField();
            cset+= GetConf("font", "font8_50").SField();
            cset+= GetConf("font", "font8_60").SField();
            cset+= GetConf("font", "font8_70").SField();
            cset+= GetConf("font", "font8_80").SField();
            cset+= GetConf("font", "font8_90").SField();
            cset+= GetConf("font", "font8_A0").SField();
            cset+= GetConf("font", "font8_B0").SField();
            cset+= GetConf("font", "font8_C0").SField();
            cset+= GetConf("font", "font8_D0").SField();
            cset+= GetConf("font", "font8_E0").SField();
            cset+= GetConf("font", "font8_F0").SField();

            if(cset.size() != 0x100)
            {
                fprintf(stderr, "ctcset error: Configuration not set properly!\n");
            }
        }
    } cset8;
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
ucs4 getucs4(ctchar chronochar, cset_class cl)
{
	switch(cl)
	{
		case cset_8pix: return cset8[chronochar];
		case cset_12pix: return cset12[chronochar];
	}
	/* Should be unreached */
	return cset12[chronochar];
}

// Note: Returns 0 for nonpresentible chars.
ctchar getchronochar(ucs4 ch, cset_class cl)
{
    ctchar result = 0;
    switch(cl)
    {
    	case cset_8pix: result = cset8.find(ch); break;
    	case cset_12pix: result = cset12.find(ch); break;
    }
    if(result < get_font_begin())
    {
        fprintf(stderr, "Error: Irrepresentible character '%c' (%X)\n", ch, ch);
    }
    return result;
}

unsigned get_font_begin()
{
    // Standard defines that this'll be initialized upon the first call.
    static const unsigned cache = GetConf("font", "begin");
    return cache;
}

namespace std
{
  template<> 
  int char_traits<ctchar>::
  compare(const ctchar* s1, const ctchar* s2, size_t n)
  {
    for(unsigned c=0; c<n; ++c) if(s1[c] != s2[c]) return s1[c] < s2[c] ? -1 : 1;
    return 0;
  }
  
  template<>
  ctchar* char_traits<ctchar>::
  copy(ctchar* s1, const ctchar* s2, size_t n)
  {
    for(unsigned c=0; c<n; ++c)s1[c] = s2[c];
    return s1;
  }
  
  template<>
  ctchar* char_traits<ctchar>::
  move(ctchar* s1, const ctchar* s2, size_t n)
  {
    if(s1 < s2) return copy(s1,s2,n);
    for(unsigned c=n; c-->0; )s1[c] = s2[c];
    return s1;
  }
  
  template<>
  ctchar* char_traits<ctchar>::
  assign(ctchar* s, size_t n, ctchar a)
  {
    for(unsigned c=0; c<n; ++c) s[c] = a;
    return s;
  }
}

unsigned CalcSize(const ctstring &word)
{
    unsigned result = word.size();
    for(unsigned a=0; a<word.size(); ++a)
        if(word[a] >= 0x100) ++result;
    return result;
}

const string GetString(const ctstring &word)
{
    string result;

    for(unsigned a=0; a<word.size(); ++a)
    {
        unsigned c = word[a];
        if(c >= 0x100) result += (char)(c >> 8);
        result += (char)(c & 255);
    }
    return result;
}
