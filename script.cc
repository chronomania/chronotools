#include <cstdio>
#include <set>
#include <list>
#include <cctype>

#include "ctinsert.hh"
#include "wstring.hh"
#include "ctcset.hh"
#include "miscfun.hh"
#include "symbols.hh"
#include "config.hh"
#include "conjugate.hh"
#include "typefaces.hh"
#include "msginsert.hh"
#include "pageptrlist.hh"

using namespace std;

namespace
{
    #undef getc
    class ScriptCharGet
    {
        wstringIn conv;
        FILE *fp;
        unsigned cacheptr;
        ucs4string cache;
        
        ucs4 getc_priv()
        {
            for(;;)
            {
                if(cacheptr < cache.size())
                    return cache[cacheptr++];
                
                CLEARSTR(cache);
                cacheptr = 0;
                while(cache.empty())
                {
                    char Buf[512];
                    size_t n = fread(Buf, 1, sizeof Buf, fp);
                    if(n == 0) break;
                    
                    cache += conv.puts(std::string(Buf, n));
                }
                // So now cache may be of arbitrary size.
                if(cache.empty()) return (ucs4)EOF;
            }
        }
    public:
        ScriptCharGet(FILE *f) : fp(f), cacheptr(0)
        {
            conv.SetSet(getcharset());
        /* - nobody cares
            fprintf(stderr, "Built script character set converter\n");
        */
        }
        ucs4 getc()
        {
            ucs4 c = getc_priv();
            if(c == '\r') return getc();
            if(c == ';')
            {
                for(;;)
                {
                    c = getc_priv();
                    if(c == '\r') continue;
                    if(c == '\n' || c == (ucs4)EOF) break;
                }
            }
            return c;
        }
    };
    
    bool CumulateBase62(unsigned& label, const string& header, int c)
    {
        if(isdigit(c))
            label = label * 62 + c - '0';
        else if(c >= 'A' && c <= 'Z')
            label = label * 62 + (c - 'A' + 10);
        else if(c >= 'a' && c <= 'z')
            label = label * 62 + (c - 'a' + 36);
        else
        {
            string tmp;
            for(unsigned tmpnum=label; tmpnum!=0; )
            {
                unsigned digit = tmpnum%62;
                tmpnum/=62;
                string tmp2;
                tmp2 += (digit<10)?(digit+'0')
                    :(digit<36)?(digit+'A'-10)
                    :(digit+'a'-36);
                tmp = tmp2 + tmp;
            }
            while(tmp.size() < 4)tmp = "0" + tmp;
            
            MessageInvalidLabelChar(c, tmp, header);
            return false;
        }
        return true;
    }
    
    bool CumulateBase16(unsigned& label, const string& header, int c)
    {
        if(isdigit(c))
            label = label * 16 + c - '0';
        else if(c >= 'A' && c <= 'z')
            label = label * 16 + c + 10 - 'A';
        else if(c >= 'A' && c <= 'z')
            label = label * 16 + c + 10 - 'a';
        else
        {
            MessageInvalidLabelChar(c, label, header);
            return false;
        }
        return true;
    }
}

namespace
{
    set<ucs4string> rawcodes;
}

const ctstring insertor::ParseScriptEntry(const ucs4string &input, const stringdata &model)
{
    ucs4string content = input;
    
    const bool is_dialog = model.type == stringdata::zptr12;
    const bool is_8pix   = model.type == stringdata::zptr8;
    const bool is_fixed  = !is_dialog && !is_8pix;

    const Symbols::type &symbols
        = Symbols.GetMap(is_dialog ? 16
                       : is_8pix   ? 8
                       : is_fixed  ? 2
                       : 0);

    int current_typeface = -1;
    
    if(is_dialog)
    {
        str_replace_inplace
        (
          content,
          AscToWstr(" [pause]"),
          AscToWstr("[pause]")
        );
        str_replace_inplace
        (
          content,
          AscToWstr("[nl]   "),
          AscToWstr("[nl3]")
        );
        str_replace_inplace
        (
          content,
          AscToWstr("[pause]   "),
          AscToWstr("[pause3]")
        );
        str_replace_inplace
        (
          content,
          AscToWstr("[pausenl]   "),
          AscToWstr("[pausenl3]")
        );
        str_replace_inplace
        (
          content,
          AscToWstr("[cls]   "),
          AscToWstr("[cls3]")
        );
    }

    ctstring result;
    for(unsigned a=0; a<content.size(); ++a)
    {
        // First test for defined symbols.
        //if(current_typeface == -1)
        {
            bool foundsym = false;
            
            ucs4string bs; bs += content[a];
            ucs4string us = content.substr(a);
            
            Symbols::type::const_iterator
                i,
                b = symbols.upper_bound(bs),
                e = symbols.upper_bound(us);
            
            for(i=b; i!=e; ++i)
            {
                if(!content.compare(a, i->first.size(), i->first))
                {
                    unsigned testlen = i->first.size();
                    
                    if(current_typeface >= 0
                    //&& i->second >= get_font_begin()
                      )
                    {
                        MessageSymbolIgnored(i->first);
                        continue;
                    }
                    
                    a += testlen-1;
                    result += i->second;
                    foundsym = true;
                    break;
                }
            }
            if(foundsym) continue;
        }
        
        // Next test for [codes].
        ucs4 c = content[a];
        if(c != AscToWchar('['))
        {
            // No code, translate byte.
            ctchar chronoc = 0;
            
            switch(model.type)
            {
                case stringdata::zptr12:
                    chronoc = getchronochar(c, cset_12pix);
                    break;
                case stringdata::zptr8:
                case stringdata::fixed:
                case stringdata::item:
                case stringdata::tech:
                case stringdata::monster:
                    chronoc = getchronochar(c, cset_8pix);
                    break;
            }
            
            /* A slight optimization: We're not typeface-changing spaces! */
            if(current_typeface >= 0 && c != AscToWchar(' '))
            {
                unsigned offset = Typefaces[current_typeface].get_offset();
                chronoc += offset;
            }
            
            result += chronoc;
            continue;
        }
        
        ucs4string code;
        for(code += c; ++a < content.size(); )
        {
            c = content[a];
            code += c;
            if(WcharToAsc(c) == ']')break;
        }
        
        // Codes used by dialog engine
        static const ucs4string delay = AscToWstr("[delay ");
        static const ucs4string tech  = AscToWstr("[tech]");
        static const ucs4string monster=AscToWstr("[monster]");
        
        // Codes used by status screen engine
        static const ucs4string code1 = AscToWstr("[next");
        static const ucs4string code2 = AscToWstr("[goto,");
        static const ucs4string code3 = AscToWstr("[func1,");
        static const ucs4string code4 = AscToWstr("[substr,");
        static const ucs4string code5 = AscToWstr("[member,");
        static const ucs4string code6 = AscToWstr("[attrs,");
        static const ucs4string code7 = AscToWstr("[out,");
        static const ucs4string code8 = AscToWstr("[spc,");
        static const ucs4string code9 = AscToWstr("[len,");
        static const ucs4string code10= AscToWstr("[attr,");
        static const ucs4string code11= AscToWstr("[func2,");
        static const ucs4string code12= AscToWstr("[stat,");
        static const ucs4string code0 = AscToWstr("[gfx");
        
        if(false) {} // for indentation...
        else if(is_dialog && !code.compare(0, delay.size(), delay))
        {
            result += (ctchar)3;
            result += (ctchar)atoi(code.c_str() + delay.size(), 10);
        }
        else if(is_dialog && code == tech)
        {
            result += (ctchar)0x12;
            result += (ctchar)0x00;
        }
        else if(is_dialog && code == monster)
        {
            result += (ctchar)0x12;
            result += (ctchar)0x01;
        }
        else if(is_dialog)
        {
            bool found = false;
            for(unsigned a=0; a<Typefaces.size(); ++a)
            {
                const ucs4string& begin = Typefaces[a].get_begin_marker();
                const ucs4string& end   = Typefaces[a].get_end_marker();
                
                if(code == begin)
                {
                    if(current_typeface >= 0)
                    {
                        MessageTFStartError(begin);
                    }
                    current_typeface = a;
                    found = true;
                }
                if(code == end)
                {
                    if(current_typeface < 0)
                    {
                        MessageTFEndError(end);
                    }
                    current_typeface = -1;
                    found = true;
                }
            }
            if(!found) goto Continue1;
        }
        else Continue1:
            if(is_8pix && !code.compare(0, code0.size(), code0))
        {
        Handle8Code:
            unsigned a=0, b=code.size();
            
            while(a<b && code[a] != ',') ++a;
            while(a<b && code[a] == ',')
            {
                unsigned value = 0, digits = 0;
                for(; ++a<b && code[a]!=',' && code[a]!=']'; ++digits)
                    value = value*16 + Whex(code[a]);
                while(digits >= 2)
                {
                    result += (ctchar)(value & 255);
                    value >>= 8;
                    digits -= 2;
                }
            }
        }
        else if(is_8pix && !code.compare(0, code1.size(), code1)) { result += 1; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code2.size(), code2)) { result += 2; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code3.size(), code3)) { result += 3; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code4.size(), code4)) { result += 4; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code5.size(), code5)) { result += 5; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code6.size(), code6)) { result += 6; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code7.size(), code7)) { result += 7; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code8.size(), code8)) { result += 8; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code9.size(), code9)) { result += 9; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code10.size(), code10)) { result += 10; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code11.size(), code11)) { result += 11; goto Handle8Code; }
        else if(is_8pix && !code.compare(0, code12.size(), code12)) { result += 12; goto Handle8Code; }
        else
        {
            result += (ctchar)atoi(code.c_str()+1, 16);
            rawcodes.insert(code);
        }
    }
    
    if(is_dialog)
    {
        Conjugater->Work(result);
        result = WrapDialogLines(result);
    }

    return result;
}

void insertor::LoadFile(FILE *fp)
{
    if(!fp)return;

    if(!Conjugater)
        Conjugater = new Conjugatemap(*this);
    
    string header;
    
    ucs4 c;

    ScriptCharGet getter(fp);
#define cget(c) (c=getter.getc())
    
    cget(c);
    
    stringdata model;

    rawcodes.clear();
    
    ucs4string unexpected;
    #define UNEXPECTED(c) unexpected += (c)
    #define EXPECTED() \
        if(!unexpected.empty()) \
        { \
            MessageUnexpected(unexpected); \
            CLEARSTR(unexpected); \
        }

    MessageLoadingDialog();
    
    for(;;)
    {
        MessageWorking();

        if(c == (ucs4)EOF)break;
        if(c == '\n')
        {
            EXPECTED();
            cget(c);
            continue;
        }

        if(c == '*')
        {
            EXPECTED();
            header = "";
            for(;;)
            {
                cget(c);
                if(c == (ucs4)EOF || isspace(c))break;
                header += WcharToAsc(c);
            }
            while(c != (ucs4)EOF && c != '\n') { cget(c); }
            
            model.ref_id  = 0;
            model.width   = 0;
            model.address = 0;

            if(header.size() >= 1 && header[0] == 'z')
            {
                list<ReferMethod> refs;
                for(unsigned a=1; a < header.size() && header[a] == ':'; )
                {
                    char type = header[++a];
                    unsigned addr = 0;
                    while(++a < header.size() && header[a] != ':')
                        CumulateBase62(addr, header, header[a]); 
                    switch(type)
                    {
                        case '^': refs.push_back(PagePtrFrom(addr)); break;
                        case '!': refs.push_back(OffsPtrFrom(addr)); break;
                        default: fprintf(stderr, "Unknown typeid '%c'\n", type);
                    }
                }
                if(!refs.empty())
                {
                    refers.push_back(refs);
                    model.ref_id = refers.size();
                }
                
                model.type = stringdata::zptr12;
                MessageZSection(header);
            }
            else if(header.size() >= 1 && header[0] == 'r')
            {
                model.type = stringdata::zptr8;
                MessageRSection(header);
            }
            else if(header.size() > 1 && header[0] == 'i')
            {
                model.type = stringdata::item;
                model.width = atoi(header.c_str() + 1);
                if(model.width < 1 || model.width > 65535)
                    MessageUnknownHeader(header);
                else
                    MessageLSection(header);
            }
            else if(header.size() > 1 && header[0] == 't')
            {
                model.type = stringdata::tech;
                model.width = atoi(header.c_str() + 1);
                if(model.width < 1 || model.width > 65535)
                    MessageUnknownHeader(header);
                else
                    MessageLSection(header);
            }
            else if(header.size() > 1 && header[0] == 'm')
            {
                model.type = stringdata::monster;
                model.width = atoi(header.c_str() + 1);
                if(model.width < 1 || model.width > 65535)
                    MessageUnknownHeader(header);
                else
                    MessageLSection(header);
            }
            else if(header.size() > 1 && header[0] == 'l')
            {
                model.type = stringdata::fixed;
                model.width = atoi(header.c_str() + 1);
                if(model.width < 1 || model.width > 65535)
                    MessageUnknownHeader(header);
                else
                    MessageLSection(header);
            }
            else if(header.size() >= 1 && header[0] == 'd')
            {
                MessageDSection(header);
            }
            else if(header.size() > 1 && header[0] == 's')
            {
                MessageSSection(header);
            }
            else
            {
                MessageUnknownHeader(header);
            }
            
            continue;
        }
        
        if(c == '$')
        {
            EXPECTED();

            unsigned label = 0;
            for(;;)
            {
                cget(c);
                if(c == (ucs4)EOF || c == ':')break;
                
                if(header.size() > 0 && (header[0] == 'd' || header[0] == 's'))
                {
                    // freespace and dict labels are hex
                    CumulateBase16(label, header, c);
                }
                else
                {
                    // other labels are 62-base (10+26+26) numbers
                    if(!CumulateBase62(label, header, c))
                        break;
                }
            }
            ucs4string content;
            bool ignore_space=false;
            for(;;)
            {
                const bool beginning = (c == '\n');
#if 1
                bool was_tag = c==']';
#endif
                bool was_lf = c=='\n';
                cget(c);
                if(was_lf && c=='\n') continue;
                if(c == (ucs4)EOF)break;
                if(beginning && (c == '*' || c == '$'))break;
                if(c == '\n')
                {
#if 1
                    ignore_space = !was_tag && content.size();
                    was_tag = false;
#endif
                    continue;
                }
#if 1
                if(ignore_space)
                {
                    if(c == ' ') { continue; }
                    ignore_space = false;
                    content += ' ';
                }
#endif
                if(c == '[')
                {
                    content += c;
                    for(;;)
                    {
                        cget(c);
                        content += c;
                        if(c == ']' || c == (ucs4)EOF)break;
                    }
                    continue;
                }
                content += c;
            }
            
            if(header.size() == 3 && header[0] == 's')
            {
                // Free space record
                
                string ascii = WstrToAsc(content);
                
                unsigned page=0, begin=label, end=0;
                sscanf(header.c_str(), "s%X", &page);
                sscanf(ascii.c_str(), "%X", &end);
                
                unsigned length = end-begin;
                
                freespace.Add(page, begin, length);
                
                continue;
            }

            if(header.size() >= 1 && header[0] == 'd')
            {
                // Dictionary record
                
                const Symbols::type &symbols = Symbols.GetMap(16);
                
                /* FIXME: This symbol parsing is
                 * DUPLICATE from ParseScriptEntry()
                 */

                ctstring dictword;
                for(unsigned a=0; a<content.size(); ++a)
                {
                    // First test for defined symbols.
                    if(true)
                    {
                        bool foundsym = false;
                        
                        ucs4string bs; bs += content[a];
                        ucs4string us = content.substr(a);
                        
                        Symbols::type::const_iterator
                            i,
                            b = symbols.upper_bound(bs),
                            e = symbols.upper_bound(us);
                        
                        for(i=b; i!=e; ++i)
                        {
                            if(!content.compare(a, i->first.size(), i->first))
                            {
                                unsigned testlen = i->first.size();
                                a += testlen-1;
                                dictword += i->second;
                                foundsym = true;
                                break;
                            }
                        }
                        if(foundsym) continue;
                    }
                    
                    // Next test for [codes].
                    ucs4 c = content[a];
                    if(c != AscToWchar('['))
                    {
                        // No code, translate byte.
                        ctchar chronoc = getchronochar(c, cset_12pix);
                        dictword += chronoc;
                        continue;
                    }
                    
                    ucs4string code;
                    for(code += c; ++a < content.size(); )
                    {
                        c = content[a];
                        code += c;
                        if(WcharToAsc(c) == ']')break;
                    }
                    
                    dictword += atoi(code.c_str()+1, 16);
                }

                dict.push_back(dictword);
                continue;
            }
            
            // It was one of these:
            //   'z' (dialog)
            //   'r' (8pix)
            //   'l' (fixed)
            //   'i' (relocatable item)
            //   't' (relocatable tech)
            //   'm' (relocatable monster)

            if(model.type == stringdata::item
            || model.type == stringdata::tech
            || model.type == stringdata::monster
              )
            {
                // Mark the space unused already at this moment
                freespace.Add(label, model.width);
            }
            
            model.str = ParseScriptEntry(content, model);
            model.address = label;
            
            strings.push_back(model);
            
            continue;
        }
        
        UNEXPECTED(c);
    }
    
    MessageDone();
    
    if(!rawcodes.empty())
    {
        fprintf(stderr, "Warning: Raw codes encountered:");
        for(set<ucs4string>::const_iterator i=rawcodes.begin(); i!=rawcodes.end(); ++i)
            fprintf(stderr, " %s", WstrToAsc(*i).c_str());
        fprintf(stderr, "\n");
    }
}

unsigned insertor::CalculateScriptSize() const
{
    MessageMeasuringScript();
    
    map<unsigned, PagePtrList> tmp;
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->type == stringdata::zptr12)
        {
            MessageWorking();
            
            const string s = GetString(i->str);
            vector<unsigned char> data(s.c_str(), s.c_str() + s.size() + 1);
            
            tmp[i->address >> 16].AddItem(data, i->address & 0xFFFF);
        }
    }

    unsigned size = 0;
    for(map<unsigned, PagePtrList>::iterator
        i = tmp.begin(); i != tmp.end(); ++i)
    {
        MessageWorking();
        i->second.Combine();
        size += i->second.Size();
    }
    
    MessageDone();
    
    return size;
}

const list<pair<unsigned, ctstring> > insertor::GetScriptByPage() const
{
    map<unsigned, PagePtrList> tmp;
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->type == stringdata::zptr12)
        {
            const string s = GetString(i->str);
            vector<unsigned char> data(s.c_str(), s.c_str() + s.size() + 1);
            
            tmp[i->address >> 16].AddItem(data, i->address & 0xFFFF);
        }
    }
    
    list<pair<unsigned, ctstring> > result;

    unsigned size = 0;
    for(map<unsigned, PagePtrList>::iterator
        i = tmp.begin(); i != tmp.end(); ++i)
    {
        i->second.Combine();
        
        const vector<unsigned char> s = i->second.GetS();
        
        ctstring tmp;
        
        for(unsigned a=0; a<s.size(); ++a)
        {
            unsigned int byte = s[a];
            if(byte == 1 || byte == 2)
                byte = byte * 256 + s[++a];
            tmp += (ctchar) byte;
        }
        result.push_back(make_pair(i->first, tmp));
    }
    
    return result;
}

void insertor::WriteFixedStrings()
{
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->type == stringdata::fixed)
        {
            MessageWorking();
            
            unsigned pos = i->address;
            const ctstring &s = i->str;
            
            // Fixed strings don't contain extrachars.
            // Thus s.size() is safe.
            unsigned size = s.size();
            
            if(size > i->width)
            {
#if 0
                fprintf(stderr, "  Warning: Fixed string at %06X: len(%u) > space(%u)... '%s'\n",
                    pos, size, i->width, DispString(s).c_str());
#endif
                size = i->width;
            }
            
            // Filler must be 255, or otherwise following problems occur:
            //     item listing goes zigzag
            //     12pix item/tech/mons text in battle has garbage (char 0 in font).

            vector<unsigned char> Buf(i->width, 255);
            if(size > i->width) size = i->width;
            
            std::copy(s.begin(), s.begin()+size, Buf.begin());
            
            objects.AddLump(Buf, pos & 0x3FFFFF, "lstring");
        }
    }
}

void insertor::WriteOtherStrings()
{
    map<unsigned, PagePtrList> pagemap;
    map<unsigned, PagePtrList> refmap;
    
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->type == stringdata::zptr8
        || i->type == stringdata::zptr12)
        {
            MessageWorking();
            
            const string s = GetString(i->str);
            vector<unsigned char> data(s.c_str(), s.c_str() + s.size() + 1);
            
#if 0
            fprintf(stderr, "String: '%s'", DispString(i->str).c_str());
            fprintf(stderr, "\n");
            for(unsigned a=0; a<data.size(); ++a)
                fprintf(stderr, " %02X", data[a]);
            fprintf(stderr, "\n");
#endif
            
            if(i->ref_id)
            {
                refmap[i->ref_id].AddItem(data, i->address & 0xFFFF);
                freespace.Add(i->address & 0x3FFFFF, 2);
            }
            else
                pagemap[i->address >> 16].AddItem(data, i->address & 0xFFFF);
        }
    }

    for(map<unsigned, PagePtrList>::iterator
        i = pagemap.begin(); i != pagemap.end(); ++i)
    {
        char Buf[64]; sprintf(Buf, "page $%02X", i->first);
        MessageLoadingItem(Buf);
        
        MessageWorking();
        
        i->second.Create(*this, i->first, "zstring");
    }

    for(map<unsigned, PagePtrList>::iterator
        i = refmap.begin(); i != refmap.end(); ++i)
    {
        unsigned ref_id = i->first - 1;
        
        char Buf[64]; sprintf(Buf, "reloc_%u_zstring", ref_id);
        MessageLoadingItem(Buf);
        
        MessageWorking();
        i->second.Create(*this, Buf, Buf);
        
        const list<ReferMethod>& refs = refers[ref_id];
        for(list<ReferMethod>::const_iterator
            j = refs.begin(); j != refs.end(); ++j)
        {
            objects.AddReference(Buf, *j);
        }
    }
}

void insertor::WriteStringTable(stringdata::strtype type,
                                const string& tablename,
                                const string& what)
{
    MessageLoadingItem(what);
    PagePtrList tmp;
    unsigned index = 0;
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        MessageWorking();
        if(i->type == type)
        {
            const string s = GetString(i->str);
            vector<unsigned char> data(s.c_str(), s.c_str() + s.size() + 1);
            tmp.AddItem(data, index);
            index += 2;
        }
    }
    if(index) tmp.Create(*this, what, tablename);
}

void insertor::WriteRelocatedStrings()
{
    WriteStringTable(stringdata::item,    "ITEMTABLE", "Items");
    WriteStringTable(stringdata::tech,    "TECHTABLE", "Techs");
    WriteStringTable(stringdata::monster, "MONSTERTABLE", "Monsters");
}

void insertor::WriteStrings()
{
    MessageWritingStrings();
    WriteFixedStrings();
    WriteOtherStrings();
    WriteRelocatedStrings();
    MessageDone();
}
