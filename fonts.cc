#include "tgaimage.hh"
#include "ctinsert.hh"
#include "ctcset.hh"
#include "fonts.hh"
#include "typefaces.hh"
#include "conjugate.hh"
#include "logfiles.hh"
#include "msginsert.hh"
#include "settings.hh"
#include "config.hh"
#include "hash.hh"

#include <utility>

#define NO_GAINLESS_SHUFFLING_AROUND 1
#define FONT12_DEBUG_LOADING         0

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

    std::vector<unsigned char> box = image.getbox(boxno);

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
    if(image.Error())
    {
        return;
    }

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

    std::vector<unsigned char> box = image.getbox(boxno);

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
    if(image.Error())
    {
        return;
    }

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

void Font8data::Load(const std::string &filename)
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
            const std::wstring &str = elems[a];

            if(!str.size())
            {
                unsigned value = elems[a];
                result.insert(value);
            }
            else
            {
                for(unsigned b=0; b<str.size(); ++b)
                {
                    ctchar c = getctchar(str[b], cl);
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

    typedef std::vector<UsageData> usagemap_t;

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
                 std::set<ctchar>& patients,
                 const charset_t& slots,
                 const std::set<ctchar>& ControlCodes)
    {
        if(patients.size() > slots.size())
        {
            fprintf(stderr, "Error: Arrange(): More patients than doctors\n");
        }

        std::set<ctchar> slots2;
        for(charset_t::const_iterator i = slots.begin(); i != slots.end(); ++i)
            slots2.insert(*i);

        // Save first <n> slots, erase rest (the excess)
        unsigned n = 0;
        for(std::set<ctchar>::iterator b, a = slots2.begin(); a != slots2.end(); a = b)
        {
            b = a; ++b;
            if(n++ >= patients.size()) slots2.erase(a);
        }

        // Place the control codes at end of the slot list
        for(std::set<ctchar>::const_iterator b, a = patients.begin(); a != patients.end(); a = b)
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
        for(std::set<ctchar>::iterator b, a = slots2.begin(); a != slots2.end(); a = b)
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
        std::set<ctchar>::const_iterator dest = slots2.begin();
        for(std::set<ctchar>::const_iterator j = patients.begin(); j != patients.end(); ++j)
        {
            ctchar orig = *j;
            result[orig] = *dest++;
        }
    }

    const Rearrangemap_t Rearrange(const usagemap_t& usages,
                                   const charset_t& free,
                                   charset_t& fixed,
                                   const std::set<ctchar>& ControlCodes)
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

        std::set<ctchar> OneBytePatients;
        std::set<ctchar> TwoBytePatients;

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
                // Mark this new location so that it won't be erased in the font
                fixed.insert(a->second);
            }

            /* Delete useless orders (attempts to not move char) */
            if(a->first == a->second) result.erase(a);
        }

        return result;
    }

    void DumpFreeMap(const std::string& label, const std::string& map)
    {
        static const char MapChars[8+1] = "-LMm" "cKCy";
        static const char *const MapDescs[8] =
        {"free slot", "locked",
         "movable",   "locked+movable=?",
         "mystery control char", "locked control char",
         "movable control char", "locked+movable+ctrl=?"};
        const unsigned width = 16;
        bool used[8] = {false};

        FILE *log = GetLogFile("font", "log_rearrange");
        if(!log) return;

        unsigned ndec = map.size() > 256 ? 3 : 2;

        std::vector<std::string> labels;
        std::vector<std::string> lines;
        std::vector<std::string> legend;

        // Build the text lines
        std::string cur_line, cur_label;
        unsigned col=0;
        for(unsigned c=0; c<map.size(); ++c)
        {
            if(col == 0)
            {
                char Buf[32];
                sprintf(Buf, "%0*X: ", ndec, c);
                cur_label = Buf;
                cur_line  = "";
            }
            cur_line += MapChars[(unsigned char)map[c]];
            used[(unsigned char)map[c]] = true;
            if(++col >= width)
            {
                col=0;
                labels.push_back(cur_label);
                lines.push_back(cur_line);
            }
        }
        if(col > 0)
        {
            labels.push_back(cur_label);
            lines.push_back(cur_line);
        }

        // Shorten the lines
        for(unsigned a=0; a<lines.size(); ++a)
        {
            if(a+1 < lines.size() && lines[a+1] != lines[a]) continue;

            unsigned hog = 0;
            while(a+2+hog < lines.size() && lines[a+2+hog] == lines[a]) ++hog;

            if(hog > 0)
            {
                labels[++a] = "...  ";
                lines[a] = "";
                while(--hog > 0)
                {
                    lines.erase(lines.begin() + a+1);
                    labels.erase(labels.begin() + a+1);
                }
            }
        }

        // Build the legend (include only used chars)
        for(unsigned a=0; a<8; ++a)
            if(used[a])
            {
                string s;
                s += MapChars[a];
                s += '=';
                s += MapDescs[a];
                legend.push_back(s);
            }

        fprintf(log, "%s:\n", label.c_str());
        for(unsigned a=0; a<lines.size(); ++a)
        {
            fprintf(log, "%6s%-*s", labels[a].c_str(), width, lines[a].c_str());
            if(a < legend.size()) fprintf(log, " %s", legend[a].c_str());
            fprintf(log, "\n");
        }
        fprintf(log, "\n");
    }

    void DumpMovableMaps(const usagemap_t& Usages, cset_class type, const std::string& what)
    {
        FILE *log = GetLogFile("font", "log_rearrange");
        if(!log) return;

        const unsigned width = 3;

        fprintf(log, "%s:\n", what.c_str());
        for(usagemap_t::const_iterator i = Usages.begin(); i != Usages.end(); ++i)
        {
            wchar_t c = getwchar_t(i->ch, type);
            if(c == ilseq)
                fprintf(log,  "     (%0*X): %u\n",                width, i->ch, i->count);
            else
                fprintf(log, "  '%c'(%0*X): %u\n", WcharToAsc(c), width, i->ch, i->count);
        }
        fprintf(log, "\n");
    }

    void DumpRearranges(const Rearrangemap_t& Rearranges,
                        const usagemap_by_char_t& usages,
                        const std::string& what)
    {
        FILE *log = GetLogFile("font", "log_rearrange");
        if(!log) return;

        fprintf(log, "Rearranged %u %s symbols:\n", (unsigned) Rearranges.size(), what.c_str());

        for(Rearrangemap_t::const_iterator i=Rearranges.begin(); i!=Rearranges.end(); ++i)
        {
            unsigned usetimes = 0;

            usagemap_by_char_t::const_iterator j = usages.find(i->first);
            if(j != usages.end()) usetimes = j->second;

            fprintf(log, "  %X(used %u times) moved to %X\n",
                i->first,
                usetimes,
                i->second);
        }
        fprintf(log, "\n");
    }
}

#include "extras.hh"

void insertor::ReorganizeFonts()
{
    charset_t Fixed_12 = LoadFixedMap("lock12syms", cset_12pix);
    charset_t Fixed_8  = LoadFixedMap("lock8syms", cset_8pix);

    usagemap_by_char_t UsagesByChar_12;
    usagemap_by_char_t UsagesByChar_8;

    MessageReorganizingFonts();

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
            case stringdata::item:
            case stringdata::tech:
            case stringdata::monster:
            case stringdata::compressed:
            {
                for(unsigned a=0; a<str.size(); ++a)
                {
                    ctchar c = str[a];

                    if(Fixed_12.find(c) != Fixed_12.end()) continue;

                    ++UsagesByChar_12[c];
                }
                break;
            }
            case stringdata::locationevent:
                ; // ignore, should not occur here.
        }
    }

    usagemap_t Usages_12 = LoadUsageMap(UsagesByChar_12);
    usagemap_t Usages_8  = LoadUsageMap(UsagesByChar_8);

    DumpMovableMaps(Usages_12, cset_12pix, "Movable 12pix chars");
    DumpMovableMaps(Usages_8,  cset_8pix,  "Movable 8x8 chars");

    charset_t Free_12;
    charset_t Free_8;

    for(ctchar c=0; c<0x300; ++c)
    {
        if(Fixed_12.find(c) != Fixed_12.end()) continue;

        // All symbols which can't be displayed
        // Or which are never used

        if(getwchar_t(c, cset_12pix) == ilseq
        || UsagesByChar_12.find(c) == UsagesByChar_12.end())
        {
            // Can be redefined.

            Free_12.insert(c);
        }
    }

    for(ctchar c=0; c<0x100; ++c)
    {
        if(Fixed_8.find(c) != Fixed_8.end()) continue;

        if(getwchar_t(c, cset_8pix) == ilseq
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

    if(true) /* dump free maps */
    {
        string map;
        for(ctchar c=0; c<0x300; ++c)
        {
            bool Fixed = Fixed_12.find(c) != Fixed_12.end();
            bool Usage = UsagesByChar_12.find(c) != UsagesByChar_12.end();
            bool Ctrl  = ControlCodes.find(c) != ControlCodes.end();

            map += (char)(Fixed*1 + Usage*2 + Ctrl*4);
        }
        DumpFreeMap("12pix map", map);

        map = "";
        for(ctchar c=0; c<0x100; ++c)
        {
            bool Fixed = Fixed_8.find(c) != Fixed_8.end();
            bool Usage = UsagesByChar_8.find(c) != UsagesByChar_8.end();
            bool Ctrl  = false;

            map += (char)(Fixed*1 + Usage*2 + Ctrl*4);
        }
        DumpFreeMap("8x8 map", map);
    }

    Rearrangemap_t Rearrange_12 = Rearrange(Usages_12, Free_12, Fixed_12, ControlCodes);
    DumpRearranges(Rearrange_12, UsagesByChar_12, "dialog");

    ControlCodes.clear();
    Rearrangemap_t Rearrange_8  = Rearrange(Usages_8, Free_8, Fixed_8, ControlCodes);
    DumpRearranges(Rearrange_8, UsagesByChar_8, "status screen");

    /* Proceed the rearrangements */
    for(stringlist::iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        ctstring& str = i->str;

        switch(i->type)
        {
            case stringdata::zptr8:
            {
                unsigned char attr = 0;
                for(size_t a=0; a<str.size(); ++a)
                {
                    const size_t aa = a; ctchar c = str[aa];

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
                for(size_t a=0; a<str.size(); ++a)
                {
                    const size_t aa = a; ctchar c = str[a];

                    extrasizemap_t::const_iterator j = Extras_12.find(c);
                    if(j != Extras_12.end()) a += j->second;

                    Rearrangemap_t::const_iterator k = Rearrange_12.find(c);
                    if(k != Rearrange_12.end()) str[aa] = k->second;
                }
                break;
            }
            case stringdata::item:
            case stringdata::tech:
            case stringdata::monster:
            case stringdata::fixed:
            case stringdata::compressed:
            {
                for(size_t a=0; a<str.size(); ++a)
                {
                    const size_t aa = a; ctchar c = str[a];

                    if(Fixed_12.find(c) != Fixed_12.end()) continue;

                    Rearrangemap_t::const_iterator k = Rearrange_12.find(c);
                    if(k != Rearrange_12.end()) str[aa] = k->second;
                }
                break;
            }
            case stringdata::locationevent:
                ; // ignore, should not occur here.
        }
    }

    /* Transform the dictionary characters */
    for(size_t d=0; d<dict.size(); ++d)
    {
        ctstring& dictword = dict[d];
        for(size_t a=0; a<dictword.size(); ++a)
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

    FILE *log = GetLogFile("font", "log_rearrange");
    if(log) fprintf(log, "Font12 end tag set at 0x%X\n\n", font12_end);

    Font12.Reload(Rearrange_12);
    Font8.Reload(Rearrange_8);
    Font8v.Reload(Rearrange_12);

    /* RearrangeCharSet() is found in ctcset.cc */

    RearrangeCharset(cset_12pix, Rearrange_12);
    RearrangeCharset(cset_8pix, Rearrange_8);

    MessageDone();
}

void insertor::WriteFonts()
{
    WriteFont8();
    WriteVWF12();
    WriteVWF8();
}

void insertor::WriteFont8()
{
    objects.AddLump(Font8.GetTiles(), GetConst(TILETAB_8_ADDRESS), "8x8 tiles");
}

void insertor::WriteVWF12()
{
    const unsigned font_begin = get_font_begin();
    const unsigned tilecount  = Font12.GetCount();

    /*
     C2:5E1E:
        0  A9 00          - lda a, $00
        2  EB             - xba
        3  38             - sec
        4  A5 35          - lda [$00:D+$35]
        6  E9 A0          - sbc a, $A0
        8  AA             - tax
        9  18             - clc
       10  BF E6 60 C2    - lda $C2:($60E6+x)
       14

       Will be changed to:

        0  C2 20          - rep $20
        2  EA             - nop
        3  EA             - nop
        4  A5 35          * lda [$00:D+$35]
        6  E2 20          - sep $20
        8  AA             * tax
        9  18             * clc
       10  BF E6 60 C2    * lda $C2:($60E6+x)
       14

       Now widthtab may have more than 256 items.
       We may ignore VWF12_WIDTH_INDEX as it's no longer used.
    */

    // patch dialog engine
    PlaceByte(font_begin,  GetConst(CSET_BEGINBYTE), L"vwf12 beginbyte");

    // patch font engine
    PlaceByte(0xC2, GetConst(VWF12_WIDTH_INDEX)-7, L"vwf12 patch"); // rep $20
    PlaceByte(0x20, GetConst(VWF12_WIDTH_INDEX)-6, L"vwf12 patch");
    PlaceByte(0xEA, GetConst(VWF12_WIDTH_INDEX)-5, L"vwf12 patch"); // nop
    PlaceByte(0xEA, GetConst(VWF12_WIDTH_INDEX)-4, L"vwf12 patch"); // nop
    PlaceByte(0xE2, GetConst(VWF12_WIDTH_INDEX)-1, L"vwf12 patch"); // sep $20
    PlaceByte(0x20, GetConst(VWF12_WIDTH_INDEX)  , L"vwf12 patch");
    /* Done with code patches. */

    /* Create a patch for the width table. */
    vector<unsigned char> widths(tilecount);
    for(unsigned a=0; a<tilecount; ++a) widths[a] = Font12.GetWidth(a);

    O65 widthblock;
    widthblock.LoadSegFrom(CODE, widths);
    widthblock.Locate(CODE, get_font_begin());
    widthblock.DeclareGlobal(CODE, "VWF12_WIDTH_TABLE", 0);
    objects.AddObject(widthblock, "VWF12_WIDTH_TABLE");
    objects.AddReference("VWF12_WIDTH_TABLE", OffsPtrFrom(GetConst(VWF12_WIDTH_OFFSET)));
    objects.AddReference("VWF12_WIDTH_TABLE", PagePtrFrom(GetConst(VWF12_WIDTH_SEGMENT)));

    unsigned pagegroup = objects.CreateLinkageGroup();
    O65 block1, block2;

    /* Create a patch for both tile tables. */
    block1.LoadSegFrom(CODE, Font12.GetTab1());
    block2.LoadSegFrom(CODE, Font12.GetTab2());
    //block1.Locate(CODE, font_begin * 24);
    //block2.Locate(CODE, font_begin * 12);
    block1.DeclareGlobal(CODE, "VWF12_TABLES", 0);
    block1.DeclareGlobal(CODE, "VWF12_TABLE1", font_begin*-24);
    block2.DeclareGlobal(CODE, "VWF12_TABLE2", font_begin*-12);

    LinkageWish wish;
    wish.SetLinkageGroup(pagegroup);

    objects.AddObject(block1, "VWF12_TABLE1", wish);
    objects.AddObject(block2, "VWF12_TABLE2", wish);

    /*
     C2:5DCE:
         0        clc
         1        adc TAB1_OFFSET
         4        sta $76
         6        lda $35
         8        lsr
      Will be changed to:
         0        adc TAB1_OFFSET
         3        sta $76
         5        lda $35
         7        clc
         8        lsr
    */
    PlaceByte(0x69, GetConst(VWF12_TAB1_OFFSET)-2, L"vwf12 patch"); // adc
    PlaceByte(0x85, GetConst(VWF12_TAB1_OFFSET)+1, L"vwf12 patch"); // sta $76
    PlaceByte(0x76, GetConst(VWF12_TAB1_OFFSET)+2, L"vwf12 patch");
    PlaceByte(0xA5, GetConst(VWF12_TAB1_OFFSET)+3, L"vwf12 patch"); // lda $35
    PlaceByte(0x35, GetConst(VWF12_TAB1_OFFSET)+4, L"vwf12 patch");
    PlaceByte(0x18, GetConst(VWF12_TAB1_OFFSET)+5, L"vwf12 patch"); // clc

    objects.AddReference("VWF12_TABLE1", OffsPtrFrom(GetConst(VWF12_TAB1_OFFSET)-1));
    objects.AddReference("VWF12_TABLE2", OffsPtrFrom(GetConst(VWF12_TAB2_OFFSET)));
    objects.AddReference("VWF12_TABLES", PagePtrFrom(GetConst(VWF12_SEGMENT)));
}

void insertor::WriteVWF8()
{
    objects.AddLump(Font8v.GetWidths(), "vwf8 widths",  "WIDTH_ADDR");
    objects.AddLump(Font8v.GetTiles(),  "vwf8 tiles",   "TILEDATA_ADDR");

    const bool DisplayEqCount = GetConf("font", "display_eq_count");

    objects.DefineSymbol("VWF8_DISPLAY_EQ_COUNT", DisplayEqCount ? 2 : 0);
}

void insertor::WriteVWF8_Strings()
{
    MessageRenderingVWF8();

    std::vector<unsigned char> ItemGFX_2bpp;
    std::vector<unsigned char> TechGFX_2bpp;
    std::vector<unsigned char> ItemGFX_4bpp;
    std::vector<unsigned char> TechGFX_4bpp;

    const unsigned n_chars = 0x280;
    const unsigned space   = getctchar(' ') - 0x80;

    const std::vector<unsigned char>& FontData = Font8v.GetTiles();
    /* ^ original font data */

    //std::pair<unsigned/*left*/,unsigned/*right*/> CharacterMargins[n_chars];

    unsigned/*n_pixels*/ KerningTable[n_chars/*left*/][n_chars/*right*/];
    /* ^ How many pixels to add to X coordinate after left-char, when right-char is next. */

#if 0
    for(unsigned c=0; c<n_chars; ++c)
    {
        printf("%02X:\n", c);
        for(unsigned y=0; y<8; ++y)
        {
            unsigned byte1 = FontData[c * 16 + y * 2 + 0];
            unsigned byte2 = FontData[c * 16 + y * 2 + 1];
            if(c == space) { byte1 = byte2 = 0x10; }

            for(unsigned x=0; x<8; ++x)
            {
                unsigned char b1 = (byte1 >> (7-x)) & 1;
                unsigned char b2 = (byte2 >> (7-x)) & 1;
                printf("%c", " .c#"[ b2*2 + b1 ]);
            }
            putchar('\n');
        }
        puts("---");
    }
    // Colors: 0=empty, 1=shadow, 2=smooth curve, 3=solid
#endif

    // Could calculate the margins for the characters
    // But won't do that, because our font is already completely left-aligned
    /*for(unsigned c=0; c<n_chars; ++c)
    {
        unsigned margin_left = 8, margin_right = 8;
        for(unsigned y=0; y<8; ++y)
        {
            unsigned byte1 = FontData[c * 16 + y * 2 + 0];
            unsigned byte2 = FontData[c * 16 + y * 2 + 1];
            if(c == space) { byte1 = byte2 = 0x10; }

            for(unsigned x=0; x<8; ++x)
            {
                unsigned char b1 = (byte1 >> (7-x)) & 1;
                unsigned char b2 = (byte2 >> (7-x)) & 1;
                unsigned char color = b2*2 + b1;

                if(!color) continue;
                if(margin_left > x) margin_left = x;
                unsigned m = 7-x;
                if(margin_right > m) margin_right = m;
            }
        }
        CharacterMargins[c] = std::pair<unsigned,unsigned> (margin_left, margin_right);
    }*/

    // Create a kerning table
    for(unsigned c1=0; c1<n_chars; ++c1)
    for(unsigned c2=0; c2<n_chars; ++c2)
    {
        int best_offset  = 8;
        int best_touches = 99999;
        for(int offset = 8; offset > 0; --offset)
        {
            // Test if the bitmaps collide if offseted this way
            int num_touches = 0;
            for(int y1=0; y1<8; ++y1)
            {
                // For each pixel of the left character, 
                unsigned byte1 = FontData[c1 * 16 + y1 * 2 + 0];
                unsigned byte2 = FontData[c1 * 16 + y1 * 2 + 1];

                for(int x1=0; x1<8; ++x1)
                {
                    unsigned char b1 = (byte1 >> (7-x1)) & 1;
                    unsigned char b2 = (byte2 >> (7-x1)) & 1;
                    unsigned char color = b2*2 + b1;
                    if(color <= 1) continue;

                    // Left character has a pixel here
                    for(int y2 = y1-1; y2 <= y1+1; ++y2)
                    {
                        if(y2 < 0 || y2 >= 8) continue;
                        unsigned byte1b = FontData[c2 * 16 + y2 * 2 + 0];
                        unsigned byte2b = FontData[c2 * 16 + y2 * 2 + 1];

                        // For the purposes of collision checking, space is always a vertical column
                        if(c2 == space) { byte1b = byte2b = 0x18; }

                        int x2 = x1-offset;
                        if(x2 >= 0 && x2 < 8)
                        {
                            unsigned char b1b = (byte1b >> (7-x2)) & 1;
                            unsigned char b2b = (byte2b >> (7-x2)) & 1;
                            unsigned char colorb = b2b*2 + b1b;
                            if(colorb > 1)
                                ++num_touches;
                        }
                    }
                }
            }
            if(num_touches > best_touches) break;
            best_touches = num_touches;
            best_offset  = offset;
        }
        if(c1 == 0x4B0/16 // 'r'
        && c2 == 0x680/16) // '.'
        {
            best_offset = 4; // Add some space between r and .
        }

        // Don't do kerning with the weapon/item icons:
        if(c1 < 0x20) best_offset = 7;

        if(c1 == space) best_offset = 4; // For space, add extra pixel

        KerningTable[c1][c2] = best_offset + 1;
    }

    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        MessageWorking();
        if(i->type == stringdata::item
        || i->type == stringdata::tech)
        {
            const ctstring& str = i->str;
            std::vector<unsigned char>& tgt_2bpp = (i->type == stringdata::item) ? ItemGFX_2bpp : TechGFX_2bpp;
            std::vector<unsigned char>& tgt_4bpp = (i->type == stringdata::item) ? ItemGFX_4bpp : TechGFX_4bpp;

            //printf("<%s>\n", GetString(i->str).c_str());

            std::vector<unsigned char> PixelBuffer_2bpp(16 * 16);
            std::vector<unsigned char> PixelBuffer_4bpp(16 * 32);

            unsigned xpos = 0, prev_c = 0;
            for(size_t a=0; a<str.size(); ++a)
            {
                const unsigned c = str[a] - (str[a] < 0x30 ? 0x20 : 0x80);
                // ^ This does the same as Trans: does in ct-vwf8.a65.

                if(a > 0) xpos += KerningTable[prev_c][c];

                for(unsigned y=0; y<8; ++y)
                {
                    unsigned byte1 = FontData[c * 16 + y * 2 + 0];
                    unsigned byte2 = FontData[c * 16 + y * 2 + 1];

                    int newx = xpos;

                    for(unsigned x=0; x<8; ++x, ++newx)
                    {
                        unsigned char b1 = (byte1 >> (7-x)) & 1;
                        unsigned char b2 = (byte2 >> (7-x)) & 1;
                        unsigned char color = b2*2 + b1;
                        if(!color) continue;

                        unsigned char& newbyte0 = PixelBuffer_2bpp[ (newx/8) * 16 + y * 2 + 0];
                        unsigned char& newbyte1 = PixelBuffer_2bpp[ (newx/8) * 16 + y * 2 + 1];

                        newbyte0 |= b1 << (7 - newx%8);
                        newbyte1 |= b2 << (7 - newx%8);

                        unsigned char& newbyte0b = PixelBuffer_4bpp[ (newx/8) * 32 + y * 2 + 0];
                        unsigned char& newbyte1b = PixelBuffer_4bpp[ (newx/8) * 32 + y * 2 + 1];

                        newbyte0b |= b1 << (7 - newx%8);
                        newbyte1b |= b2 << (7 - newx%8);
                    }
                }

                prev_c = c;
            }

            tgt_2bpp.insert(tgt_2bpp.end(), PixelBuffer_2bpp.begin(), PixelBuffer_2bpp.end());
            tgt_4bpp.insert(tgt_4bpp.end(), PixelBuffer_4bpp.begin(), PixelBuffer_4bpp.end());
        }
    }

    MessageDone();

    std::vector<unsigned char> Item_2bpp_part2;
    std::vector<unsigned char> Item_4bpp_part2( ItemGFX_4bpp.begin() + 65536, ItemGFX_4bpp.end() );
    ItemGFX_4bpp.erase( ItemGFX_4bpp.begin() + 65536, ItemGFX_4bpp.end() );

    objects.AddLump(TechGFX_2bpp, "vwf8 techs 2bpp", "VWF8_TECHS_2BPP");
    objects.AddLump(TechGFX_4bpp, "vwf8 techs 4bpp", "VWF8_TECHS_4BPP");

    objects.AddLump(ItemGFX_2bpp,    "vwf8 items 2bpp part1", "VWF8_ITEMS_2BPP_PART1");
    objects.AddLump(ItemGFX_4bpp,    "vwf8 items 4bpp part1", "VWF8_ITEMS_4BPP_PART1");
    objects.AddLump(Item_2bpp_part2, "vwf8 items 2bpp part2", "VWF8_ITEMS_2BPP_PART2");
    objects.AddLump(Item_4bpp_part2, "vwf8 items 4bpp part2", "VWF8_ITEMS_4BPP_PART2");
}

Font8data::Font8data(): tiletable(), widths(), fn() { }
Font8data::~Font8data() {}
Font8vdata::~Font8vdata() {}
