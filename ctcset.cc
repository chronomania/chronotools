#include <string>
#include <algorithm>

using std::string;

#include "ctcset.hh"
#include "config.hh"
#include "fonts.hh"
#include "hash.hh"

/* Language-specific settings end here. */
namespace
{
#if USE_ICONV
    std::string CharSet = "iso-8859-1";
#endif

    class CharacterSet
    {
        hash_map<wchar_t, ctchar> revmap;
        std::vector<ctchar> revmapfirst;

    private:
        void RebuildRevmap()
        {
            revmapfirst.clear();
            revmapfirst.resize( (unsigned) GetConf("font", "charcachesize"), 0);

            std::wstring noncharstr = GetConf("font", "nonchar");
            wchar_t nonchar = noncharstr[0];

            for(unsigned a = 0; a < cset.size(); ++a)
            {
                wchar_t c = cset[a];
                if(c == nonchar) { cset[a] = c = ilseq; }
                if((unsigned)c < revmapfirst.size()) revmapfirst[(unsigned)c] = a;
                else if(c != ilseq)revmap[c] = a;
            }
#if 0 /* nobody needs this message */
            fprintf(stderr, "Built charset map (%u noncached)\n", revmap.size());
#endif
        }

    protected:
        std::wstring cset;
        virtual void GetCSet() = 0;

    public:
        CharacterSet() : revmap(), revmapfirst(), cset()
        {
        }
        virtual ~CharacterSet()
        {
        }

        void Rearrange(const Rearrangemap_t& rearrange)
        {
            std::wstring newcset(cset.size(), ilseq);

            hash_set<ctchar> forbid;

            for(Rearrangemap_t::const_iterator
                i = rearrange.begin(); i != rearrange.end(); ++i)
            {
                if(i->second < newcset.size()
                && i->first  < cset.size())
                {
                    newcset[i->second] = cset[i->first];
                    forbid.insert(i->second); // Target may not be overwritten
                    forbid.insert(i->first);  // Source has already been written
                }
            }

            for(unsigned a=0; a<cset.size(); ++a)
            {
                if(forbid.find(a) != forbid.end()) continue;
                newcset[a] = cset[a];
            }

            cset = newcset;

            RebuildRevmap();
        }

        void Init()
        {
            GetCSet();
            /* "cset" now contains the oneway map. */

            RebuildRevmap();
        }
        wchar_t operator[] (ctchar ind)
        {
            if(cset.empty()) Init();
            return cset[ind];
        }
        ctchar find(wchar_t p)
        {
            if(p == ilseq)return 0;

            if(cset.empty()) Init();

            if((unsigned)p < revmapfirst.size()) return revmapfirst[(unsigned)p];

            hash_map<wchar_t, ctchar>::const_iterator i;
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

#if USE_ICONV
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
#endif

// Note: Only values 0xA0..0xFF are worth using.
wchar_t getwchar_t(ctchar chronochar, cset_class cl)
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
ctchar getctchar(wchar_t ch, cset_class cl)
{
    ctchar result = 0;
    switch(cl)
    {
        case cset_8pix: result = cset8.find(ch); break;
        case cset_12pix: result = cset12.find(ch); break;
    }
    if(!result
    || (cl == cset_12pix && result < get_font_begin())
      )
    {
        fprintf(stderr, "Error: Irrepresentible character '%c' ($%X) = $%X (%d)", ch, ch, result, result);
        switch(cl)
        {
            case cset_8pix: fprintf(stderr, " in 8x8 font\n"); break;
            case cset_12pix: fprintf(stderr, " in 12pix font\n"); break;
            default: fprintf(stderr, " in unknown font class\n");
        }
    }

    return result;
}

unsigned get_font_begin()
{
    // Standard defines that this'll be initialized upon the first call.
    static const unsigned cache = GetConf("font", "begin");
    return cache;
}

#if 0
namespace std/*__gnu_cxx*/
{
  template<>
  int char_traits<ctchar>::
  compare(const ctchar* s1, const ctchar* s2, std::size_t n)
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
#endif

unsigned CalcSize(const ctstring &word)
{
    unsigned result = word.size();
    for(unsigned a=0; a<word.size(); ++a)
        if(word[a] >= 0x100) ++result;
    return result;
}

const std::string GetString(const ctstring &word)
{
    std::string result;

    for(unsigned a=0; a<word.size(); ++a)
    {
        unsigned c = word[a];
        if(c >= 0x100) result += (char)(c >> 8);
        result += (char)(c & 0xFF);
    }
    return result;
}

const ctstring getctstring(const std::wstring& s, cset_class cl)
{
    ctstring result(s.size(), 0);
    for(unsigned a=0; a<s.size(); ++a)
        result[a] = getctchar(s[a], cl);
    return result;
}

const ctstring getctstring(const std::vector<unsigned char>& blob)
{
    ctstring result(blob.size(), 0);
    std::copy(blob.begin(), blob.end(), &result[0]);
    return result;
}


void RearrangeCharset(cset_class cl, const Rearrangemap_t& rearrange)
{
    switch(cl)
    {
        case cset_8pix: cset8.Rearrange(rearrange); break;
        case cset_12pix: cset12.Rearrange(rearrange); break;
    }
}
