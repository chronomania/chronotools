#include "tgaimage.hh"
#include "ctinsert.hh"
#include "ctcset.hh"
#include "fonts.hh"
#include "typefaces.hh"
#include "conjugate.hh"
#include "config.hh"
#include "hash.hh"

#define NO_GAINLESS_SHUFFLING_AROUND 1
#define FONT12_DEBUG_LOADING         0
#define DISPLAY_REARRANGE_DETAILS    0
#define DUMP_FREE_MAPS               0

namespace
{
    unsigned font12_end   = 0x300;
    unsigned font8v_end   = 0x300;
    unsigned font8v_begin = 0x80;

#if 0
    unsigned get_num_normal_chars() // Get num of characters < 0x100
    {
        unsigned begin = get_font_begin();
        if(begin >= 0x100) return 0;
        return 0x100 - begin;
    }

    unsigned get_num_extra_chars()  // Get num of characters >= 0x100
    {
        unsigned end = font12_end;
        
        if(end < 0x100) return 0;
        
        return end - 0x100;
    }
#endif

}

void Font12data::LoadBoxAs(unsigned boxno, unsigned tileno, class TGAimage &image)
{
    //fprintf(stderr, "%03X -> %03X\t", boxno, tileno + get_font_begin());
    
    static const char palette[] = {0,0,1,2,3,0};
    
    vector<char> box = image.getbox(boxno);
    
    //fprintf(stderr, "Char $%X: dest index $%X\n", boxno, tileno);

    unsigned width=0;
    while(box[width] != 5 && width < 12)++width;
    
    // Typefaces refer to original characters.
    for(unsigned t=0; t<Typefaces.size(); ++t)
    {
        const ctchar chronochar = boxno;
        const unsigned begin   = Typefaces[t].get_begin();
        const unsigned end     = Typefaces[t].get_end();
        const unsigned condense= Typefaces[t].get_condense();
        
        if(chronochar >= begin
        && chronochar < end
        && condense > 0
        && condense < 10
        && width >= condense)
        {
            width -= condense;
        }
    }

    widths[tileno] = width;

#if FONT12_DEBUG_LOADING
    fprintf(stderr, "box %03X: %u(+%u) - width=%u:\n",
        tileno + get_font_begin(),
        tileno, boxstart, width);
    for(unsigned y=0; y<12; ++y)
    {
        fprintf(stderr, "   ");
        for(unsigned x=0; x<12; ++x)
            fprintf(stderr, "%c", "0.\"/#-"[box[y*12+x]]);
        fprintf(stderr, "\n");
    }
#endif

    for(unsigned p=0; p<box.size(); ++p)
        if((unsigned char)box[p] < sizeof(palette))
            box[p] = palette[ static_cast<unsigned> (box[p]) ];
    
    unsigned to  = 24*(tileno);
    unsigned to2 = 24*(tileno/2);
    unsigned po  = 0;
    for(unsigned y=0; y<12; ++y)
    {
        #define gb(n,v) ((box[po+n]&v)/v)
        
        if(true)
        {
            unsigned char
            byte1 = (gb( 0,1) << 7)
                  | (gb( 1,1) << 6)
                  | (gb( 2,1) << 5)
                  | (gb( 3,1) << 4)
                  | (gb( 4,1) << 3)
                  | (gb( 5,1) << 2)
                  | (gb( 6,1) << 1)
                  | (gb( 7,1));
            unsigned char
            byte2 = (gb( 0,2) << 7)
                  | (gb( 1,2) << 6)
                  | (gb( 2,2) << 5)
                  | (gb( 3,2) << 4)
                  | (gb( 4,2) << 3)
                  | (gb( 5,2) << 2)
                  | (gb( 6,2) << 1)
                  | (gb( 7,2));

            tiletab1[to++] = byte1;
            tiletab1[to++] = byte2;

            po += 8;
        }

        if(true)
        {
            unsigned char
            byte3 = (gb( 0,1) << 3)
                  | (gb( 1,1) << 2)
                  | (gb( 2,1) << 1)
                  | (gb( 3,1) << 0);
            unsigned char
            byte4 = (gb( 0,2) << 3)
                  | (gb( 1,2) << 2)
                  | (gb( 2,2) << 1)
                  | (gb( 3,2) << 0);
            if(!(tileno&1)) { byte3 *= 16; byte4 *= 16; }
            
            tiletab2[to2++] |= byte3;
            tiletab2[to2++] |= byte4;

            po += 4;
        }
        
        #undef gb
    }
}

void Font12data::Reload(const Rearrangemap_t& arrange)
{
    fprintf(stderr, "Loading '%s'...\n", fn.c_str());
    TGAimage image(fn);
    image.setboxsize(12, 12);
    image.setboxperline(32);
    
    unsigned boxcount = image.getboxcount();
    unsigned boxstart = get_font_begin();
    
    charcount = font12_end - get_font_begin();
    
    tiletab1.clear();
    tiletab2.clear();
    
    tiletab1.resize(charcount * 24, 0);
    tiletab2.resize(charcount * 12, 0);
    
    widths.clear();
    widths.resize(charcount, 0);
    
    hash_set<unsigned> forbid;
    for(Rearrangemap_t::const_iterator
        i = arrange.begin(); i != arrange.end(); ++i)
    {
        forbid.insert(i->second); // Target: may not be overwritten
        forbid.insert(i->first);  // Source: already written
        
        unsigned newno = i->second - boxstart;
        if(newno >= charcount) continue;
        
        LoadBoxAs(i->first, newno, image);
    }

    for(unsigned b=0; b<boxcount; ++b)
    {
        if(b < boxstart) continue;
        
        if(forbid.find(b) != forbid.end()) continue;
        
        unsigned newno = b - boxstart;
        if(newno >= charcount) continue;
        
        LoadBoxAs(b, newno, image);
    }
    
    //fprintf(stderr, "\n");
}

void Font12data::Load(const string &filename)
{
    fn = filename;
    
    Rearrangemap_t dummy;
    Reload(dummy);
}

unsigned Font12data::GetCount() const
{
    return charcount;
}

void Font8data::LoadBoxAs(unsigned boxno, unsigned tileno, class TGAimage &image)
{
    static const char palette[] = {0,0,1,2,3,0};

    vector<char> box = image.getbox(boxno);
    
    unsigned width=0;
    while(box[width] != 1 && width < 8)++width;

    widths[tileno] = width;

    for(unsigned p=0; p<box.size(); ++p)
        if((unsigned char)box[p] < sizeof(palette))
            box[p] = palette[ static_cast<unsigned> (box[p]) ];
    
    unsigned to = tileno * 16;
    unsigned po = 0;
    for(unsigned y=0; y<8; ++y)
    {
        unsigned char byte1 = 0;
        unsigned char byte2 = 0;
        for(unsigned x=0; x<8; ++x)
        {
            unsigned shift = (7-x);
            byte1 |= ((box[po]&1)  ) << shift;
            byte2 |= ((box[po]&2)/2) << shift;
            ++po;
        }
        tiletable[to++] = byte1;
        tiletable[to++] = byte2;
    }
}

void Font8data::Reload(const Rearrangemap_t& arrange)
{
    fprintf(stderr, "Loading '%s'...\n", fn.c_str());

    TGAimage image(fn);
    image.setboxsize(8, 8);
    image.setboxperline(32);
    
    unsigned boxstart  = GetBegin();
    unsigned boxcount  = image.getboxcount();
    unsigned charcount = GetCount() - boxstart;
    
    tiletable.clear();
    tiletable.resize(charcount * 16, 0);
    
    widths.clear();
    widths.resize(charcount, 0);
    
    hash_set<unsigned> forbid;
    for(Rearrangemap_t::const_iterator
        i = arrange.begin(); i != arrange.end(); ++i)
    {
        unsigned b = i->first;
        unsigned c = i->second;
        
        if(c < boxstart) continue;
        
        forbid.insert(c); // Target: may not be overwritten
        forbid.insert(b); // Source: already written
        
        unsigned newno = c - boxstart;
        if(newno >= charcount) continue;
        
        LoadBoxAs(i->first, newno, image);
    }

    for(unsigned b=0; b<boxcount; ++b)
    {
        if(b < boxstart) continue;
        
        if(forbid.find(b) != forbid.end()) continue;
        
        unsigned newno = b - boxstart;
        if(newno >= charcount) continue;
        
        LoadBoxAs(b, newno, image);
    }
}

void Font8data::Load(const string &filename)
{
    fn = filename;
    
    Rearrangemap_t dummy;
    Reload(dummy);
}

unsigned Font8data::GetBegin() const
{
    return 0;
}
unsigned Font8data::GetCount() const
{
    return 256;
}

unsigned Font8vdata::GetBegin() const
{
    return font8v_begin;
}
unsigned Font8vdata::GetCount() const
{
    return font8v_end;
}

unsigned insertor::GetFont12width(ctchar chronoch) const
{
    unsigned start = get_font_begin();
    if(chronoch < start)
    {
        fprintf(stderr, "Error: requested chronoch=$%04X, smallest allowed=$%02X\n",
            chronoch, start);
    }
    try
    {
        return Font12.GetWidth(chronoch-start);
    }
    catch(...)
    {
        fprintf(stderr,
            "Error: Character index $%04X is too big, says the font\n",
            chronoch);
        return 0;
    }
}

namespace
{
    typedef hash_set<ctchar> charset_t;

    const charset_t LoadFixedMap(const char *ConfigKey, cset_class cl)
    {
        const ConfParser::ElemVec& elems = GetConf("font", ConfigKey).Fields();
        charset_t result;
        for(unsigned a=0; a<elems.size(); ++a)
        {
            const ucs4string &str = elems[a];
            
            if(!str.size())
            {
                unsigned value = elems[a];
                result.insert(value);
            }
            else
            {
                for(unsigned b=0; b<str.size(); ++b)
                {
                    ctchar c = getchronochar(str[b], cl);
                    result.insert(c);
                }
            }
        }
        return result;
    }
    
    struct UsageData
    {
        ctchar   ch;
        unsigned count;
        
        bool operator< (const UsageData& b) const
        {
            return count < b.count;
        }
    };
    
    typedef vector<UsageData> usagemap_t;
    
    typedef hash_map<ctchar, unsigned> usagemap_by_char_t;
    
    const usagemap_t LoadUsageMap(const usagemap_by_char_t& src)
    {
        usagemap_t result;
        
        for(usagemap_by_char_t::const_iterator i=src.begin(); i!=src.end(); ++i)
        {
            UsageData tmp;
            tmp.ch = i->first;
            tmp.count = i->second;
            result.push_back(tmp);
        }
        sort(result.begin(), result.end());
        return result;
    }

    /* Rearrangemap_t is defined in fonts.hh */
    
    void Arrange(Rearrangemap_t& result,
                 set<ctchar>& patients,
                 const charset_t& slots,
                 const set<ctchar>& ControlCodes)
    {
        if(patients.size() > slots.size())
        {
            fprintf(stderr, "Error: Arrange(): More patients than doctors\n");
        }
        
        set<ctchar> slots2;
        for(charset_t::const_iterator i = slots.begin(); i != slots.end(); ++i)
            slots2.insert(*i);
        
        // Save first <n> slots, erase rest (the excess)
        unsigned n = 0;
        for(set<ctchar>::iterator b, a = slots2.begin(); a != slots2.end(); a = b)
        {
            b = a; ++b;
            if(n++ >= patients.size()) slots2.erase(a);
        }
        
        // Place the control codes at end of the slot list
        for(set<ctchar>::const_iterator b, a = patients.begin(); a != patients.end(); a = b)
        {
            b = a; ++b;
            
            ctchar orig = *a;

            if(ControlCodes.find(orig) != ControlCodes.end())
            {
                // Pick the last slot
                
                set<ctchar>::iterator dest = slots2.end(); --dest;
                result[orig] = *dest;
                slots2.erase(dest);
                
                patients.erase(a);
            }
        }

#if NO_GAINLESS_SHUFFLING_AROUND /* This prevents gainless shuffling around */

        // If the patient's home slot is one of the patient's
        // target choices, don't move that patient anywhere
        for(set<ctchar>::iterator b, a = slots2.begin(); a != slots2.end(); a = b)
        {
            b = a; ++b;
            
            set<ctchar>::iterator c = patients.find(*a);
            if(c == patients.end()) continue;
            
            result[*c] = *a;
            
            patients.erase(c);
            slots2.erase(a);
        }
#endif
        
        // Assign everything else straightforwardly
        set<ctchar>::const_iterator dest = slots2.begin();
        for(set<ctchar>::const_iterator j = patients.begin(); j != patients.end(); ++j)
        {
            ctchar orig = *j;
            result[orig] = *dest++;
        }
    }

    const Rearrangemap_t Rearrange
      (const usagemap_t& usages,
       const charset_t& free,
       charset_t& fixed,
       const set<ctchar>& ControlCodes)
    {
        Rearrangemap_t result;
        
        charset_t OneByteSlots;
        charset_t TwoByteSlots;
        
        /* Collect slots */
        for(charset_t::const_iterator i = free.begin(); i != free.end(); ++i)
        {
            if(*i < 0x100) OneByteSlots.insert(*i);
            else TwoByteSlots.insert(*i);
        }
        
        /* Slots come from moved characters too */
        for(usagemap_t::const_iterator i = usages.end(); i-- != usages.begin(); )
        {
            ctchar ch = i->ch;
            
            if(ch < 0x100) OneByteSlots.insert(ch);
            else TwoByteSlots.insert(ch);
        }
        
        unsigned left1 = OneByteSlots.size();
        unsigned left2 = TwoByteSlots.size();
        
        set<ctchar> OneBytePatients;
        set<ctchar> TwoBytePatients;
        
        /* Allocate slots */
        for(usagemap_t::const_iterator i = usages.end(); i-- != usages.begin(); )
        {
            ctchar ch = i->ch;
            if(left1 > 0 /* && ControlCodes.find(ch)==ControlCodes.end() */ )
            {
                // First allocate onebyte slots
                OneBytePatients.insert(ch);
                --left1;
            }
            else if(left2 > 0)
            {
                // Then twobyte.
                TwoBytePatients.insert(ch);
                --left2;
            }
            else
                break;
        }
        
#if 0
        if(left1 || left2)
            fprintf(stderr, "%u 1byte and %u 2byte slots left free\n", left1, left2);
#endif

        /* Assign slots */
        Arrange(result, OneBytePatients, OneByteSlots, ControlCodes);
        Arrange(result, TwoBytePatients, TwoByteSlots, ControlCodes);
        
        for(Rearrangemap_t::iterator b,a = result.begin(); a != result.end(); a=b)
        {
            b=a; ++b;
            
            if(ControlCodes.find(a->first) == ControlCodes.end())
            {
                fixed.insert(a->second);
            }
            if(a->first == a->second) result.erase(a);
        }

#if DISPLAY_REARRANGE_DETAILS
        for(Rearrangemap_t::const_iterator i=result.begin(); i!=result.end(); ++i)
        {
            unsigned usetimes = 0;
            
            for(unsigned a=0; a<usages.size(); ++a)
            {
                if(usages[a].ch == i->first) { usetimes = usages[a].count; break; }
            }
        
            fprintf(stderr, "  Arranged %X(used %u times) to %X\n",
                i->first,
                usetimes,
                i->second);
        }
        fprintf(stderr, "--\n");
#endif
        
        return result;
    }
}

#include "extras.hh"

void insertor::ReorganizeFonts()
{
    charset_t Fixed_12 = LoadFixedMap("lock12syms", cset_12pix);
    charset_t Fixed_8  = LoadFixedMap("lock8syms", cset_8pix);
    
    usagemap_by_char_t UsagesByChar_12;
    usagemap_by_char_t UsagesByChar_8;
    
    fprintf(stderr, "Analyzing character usage...\n");
    
    const ctchar font_begin = get_font_begin();
    
    /* Dialog font: 0..0x20 are specials, 0x21..font_begin-1 is dictionary */
    for(unsigned c=0; c<font_begin; ++c) Fixed_12.insert(c);
    
    /* Status screen font: 0..15 are specials, 16..159 are reserved */
    for(unsigned c=0; c<0xA0; ++c)       Fixed_8.insert(c);

    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        const ctstring& str = i->str;
        
        switch(i->type)
        {
            case stringdata::zptr8:
            {
                unsigned char attr = 0;
                for(unsigned a=0; a<str.size(); ++a)
                {
                    ctchar c = str[a];
                    
                    if(c == 10)
                    {
                        attr = str[a+1];
                    }
                    
                    extrasizemap_t::const_iterator j = Extras_8.find(c);
                    if(j != Extras_8.end()) a += j->second;
                    
                    if(attr & 0x03)
                    {
                        // Don't count GFX
                        continue;
                    }
                    
                    if(Fixed_8.find(c) != Fixed_8.end()) continue;
                    
                    ++UsagesByChar_8[c];
                }
                break;
            }
            case stringdata::zptr12:
            {
                for(unsigned a=0; a<str.size(); ++a)
                {
                    ctchar c = str[a];
                    
                    extrasizemap_t::const_iterator j = Extras_12.find(c);
                    if(j != Extras_12.end()) a += j->second;
                    
                    if(Fixed_12.find(c) != Fixed_12.end()) continue;
                    
                    ++UsagesByChar_12[c];
                }
                break;
            }
            case stringdata::fixed:
            {
                for(unsigned a=0; a<str.size(); ++a)
                {
                    ctchar c = str[a];
                    
                    if(Fixed_12.find(c) != Fixed_12.end()) continue;
                    
                    ++UsagesByChar_12[c];
                }
                break;
            }
        }
    }
    
    usagemap_t Usages_12 = LoadUsageMap(UsagesByChar_12);
    usagemap_t Usages_8  = LoadUsageMap(UsagesByChar_8);
    
#if 0
    fprintf(stderr, "12pix:\n");
    for(usagemap_t::const_iterator i = Usages_12.begin(); i != Usages_12.end(); ++i)
    {
        ucs4 c = getucs4(i->ch, cset_12pix);
        fprintf(stderr, "  '%c'(%X): %u\n", WcharToAsc(c),i->ch, i->count);
    }
    
    fprintf(stderr, "8pix:\n");
    for(usagemap_t::const_iterator i = Usages_8.begin(); i != Usages_8.end(); ++i)
    {
        ucs4 c = getucs4(i->ch, cset_8pix);
        fprintf(stderr, "  '%c'(%X): %u\n", WcharToAsc(c),i->ch, i->count);
    }
#endif

    charset_t Free_12;
    charset_t Free_8;
    
    for(ctchar c=0; c<0x300; ++c)
    {
        if(Fixed_12.find(c) != Fixed_12.end()) continue;
        
        // All symbols which can't be displayed
        // Or which are never used
        
        if(getucs4(c, cset_12pix) == ilseq
        || UsagesByChar_12.find(c) == UsagesByChar_12.end())
        {
            // Can be redefined.
            
            Free_12.insert(c);
        }
    }
    
    for(ctchar c=0; c<0x100; ++c)
    {
        if(Fixed_8.find(c) != Fixed_8.end()) continue;
        
        if(getucs4(c, cset_8pix) == ilseq
        || UsagesByChar_8.find(c) == UsagesByChar_8.end())
        {
            Free_8.insert(c);
        }
    }
    set<ctchar> ControlCodes;

    ControlCodes.clear();
    for(usagemap_t::const_iterator i = Usages_12.begin(); i != Usages_12.end(); ++i)
    {
        if(Conjugater->IsConjChar(i->ch)) ControlCodes.insert(i->ch);
    }
    Rearrangemap_t Rearrange_12 = Rearrange(Usages_12, Free_12, Fixed_12, ControlCodes);
    fprintf(stderr, "- Rearranged %u dialog symbols\n", Rearrange_12.size());
    
    ControlCodes.clear();
    Rearrangemap_t Rearrange_8  = Rearrange(Usages_8, Free_8, Fixed_8, ControlCodes);
    fprintf(stderr, "- Rearranged %u status screen symbols\n", Rearrange_8.size());
    
#if DUMP_FREE_MAPS
    fprintf(stderr, "12pix map:\n");
    for(ctchar c=0; c<0x300; ++c)
    {
        if( (c&31) == 0)  fprintf(stderr, " %03X: ", c);
        fputc( (Fixed_12.find(c) == Fixed_12.end()) ? 'F' : 'U' , stderr);
        if( (c&31) == 31) fputc('\n', stderr);
    }
    
    fprintf(stderr, "8x8 map:\n");
    for(ctchar c=0; c<0x100; ++c)
    {
        if( (c&31) == 0)  fprintf(stderr, " %02X: ", c);
        fputc( (Fixed_8.find(c) == Fixed_8.end()) ? 'F' : 'U' , stderr);
        if( (c&31) == 31) fputc('\n', stderr);
    }
#endif

    /* Proceed the rearrangements */
    for(stringlist::iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        ctstring& str = i->str;
        
        switch(i->type)
        {
            case stringdata::zptr8:
            {
                unsigned char attr = 0;
                for(unsigned a=0; a<str.size(); ++a)
                {
                    const unsigned aa = a; ctchar c = str[aa];
                    
                    if(c == 10)
                    {
                        attr = str[a+1];
                    }
                    
                    extrasizemap_t::const_iterator j = Extras_8.find(c);
                    if(j != Extras_8.end()) a += j->second;
                    
                    if(attr & 0x03)
                    {
                        // Don't modify GFX
                        continue;
                    }
                    
                    Rearrangemap_t::const_iterator k = Rearrange_8.find(c);
                    if(k != Rearrange_8.end()) str[aa] = k->second;
                }
                break;
            }
            case stringdata::zptr12:
            {
                for(unsigned a=0; a<str.size(); ++a)
                {
                    const unsigned aa = a; ctchar c = str[a];
                    
                    extrasizemap_t::const_iterator j = Extras_12.find(c);
                    if(j != Extras_12.end()) a += j->second;
                    
                    Rearrangemap_t::const_iterator k = Rearrange_12.find(c);
                    if(k != Rearrange_12.end()) str[aa] = k->second;
                }
                break;
            }
            case stringdata::fixed:
            {
                for(unsigned a=0; a<str.size(); ++a)
                {
                    const unsigned aa = a; ctchar c = str[a];
                    
                    if(Fixed_12.find(c) != Fixed_12.end()) continue;

                    Rearrangemap_t::const_iterator k = Rearrange_12.find(c);
                    if(k != Rearrange_12.end()) str[aa] = k->second;
                }
                break;
            }
        }
    }
    
    /* Transform the dictionary characters */
    for(unsigned d=0; d<dict.size(); ++d)
    {
        ctstring& dictword = dict[d];
        for(unsigned a=0; a<dictword.size(); ++a)
        {
            ctchar c = dictword[a];
            Rearrangemap_t::const_iterator k = Rearrange_12.find(c);
            if(k != Rearrange_12.end()) dictword[a] = k->second;
        }
    }
    
    for(Rearrangemap_t::const_iterator
        i = Rearrange_12.begin(); i != Rearrange_12.end(); ++i)
    {
        // FIXME: What happens if the conj-chars are swapped?
        
        if(Conjugater->IsConjChar(i->first))
        {
            Conjugater->RedefineConjChar(i->first, i->second);
            continue;
        }
    }
    
    font12_end = 0;
    for(charset_t::const_iterator i = Fixed_12.begin(); i != Fixed_12.end(); ++i)
        if(*i >= font12_end)
        {
            font12_end = *i + 1;
            font8v_end = *i + 1;
        }
    
    fprintf(stderr, "Font12 end tag set at 0x%X\n", font12_end);
    
    Font12.Reload(Rearrange_12);
    Font8.Reload(Rearrange_8);
    Font8v.Reload(Rearrange_12);
    
    RearrangeCharset(cset_12pix, Rearrange_12);
    RearrangeCharset(cset_8pix, Rearrange_8);
    
    /* FIXME: ctcset must be rearranged too!
     * Otherwise conjugate-asm fails */
}
