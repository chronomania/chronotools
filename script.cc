#include <cstdio>
#include <set>
#include <list>
#include <cctype>

#include "ctinsert.hh"
#include "wstring.hh"
#include "ctcset.hh"
#include "compress.hh"
#include "miscfun.hh"
#include "symbols.hh"
#include "config.hh"
#include "conjugate.hh"
#include "typefaces.hh"
#include "msginsert.hh"
#include "pageptrlist.hh"
#include "base62.hh"
#include "rommap.hh"
#include "romaddr.hh"

using namespace std;

namespace
{
    #undef getc
    class ScriptCharGet
    {
        wstringIn conv;
        FILE *fp;
        unsigned cacheptr;
        wstring cache;
        
        wchar_t getc_priv()
        {
            for(;;)
            {
                if(cacheptr < cache.size())
                    return cache[cacheptr++];
                
                cache.clear();
                cacheptr = 0;
                while(cache.empty())
                {
                    char Buf[512];
                    size_t n = fread(Buf, 1, sizeof Buf, fp);
                    if(n == 0) break;
                    
                    cache += conv.puts(std::string(Buf, n));
                }
                // So now cache may be of arbitrary size.
                if(cache.empty()) return (wchar_t)EOF;
            }
        }
    public:
        ScriptCharGet(FILE *f) : conv(getcharset()),
                                 fp(f), cacheptr(0),
                                 cache()
        {
        /* - nobody cares
            fprintf(stderr, "Built script character set converter\n");
        */
        }
        wchar_t getc()
        {
            wchar_t c = getc_priv();
            if(c == '\r') return getc();
            if(c == ';')
            {
                for(;;)
                {
                    c = getc_priv();
                    if(c == '\r') continue;
                    if(c == '\n' || c == (wchar_t)EOF) break;
                }
            }
            return c;
        }
    private:
        const ScriptCharGet& operator=(const ScriptCharGet&);
        ScriptCharGet(const ScriptCharGet& );
    };
    
    bool CumulateBase62(unsigned& label, const string& header, char c)
    {
        if(!::CumulateBase62(label, c))
        {
            string tmp = EncodeBase62(label, 4);
            
            MessageInvalidLabelChar(c, tmp, header);
            return false;
        }
        return true;
    }
    
    bool CumulateBase16(unsigned& label, const string& header, char c)
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
    bool CumulateAlnumString(std::string& label, const string& header, char c)
    {
        if(isalnum(c))
            label += c;
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
    set<wstring> rawcodes;
}

const ctstring insertor::ParseScriptEntry(const wstring &input, const stringdata &model)
{
    wstring content = input;
    
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
          L" [pause]",
          L"[pause]"
        );
        str_replace_inplace
        (
          content,
          L"[nl]   ",
          L"[nl3]"
        );
        str_replace_inplace
        (
          content,
          L"[pause]   ",
          L"[pause3]"
        );
        str_replace_inplace
        (
          content,
          L"[pausenl]   ",
          L"[pausenl3]"
        );
        str_replace_inplace
        (
          content,
          L"[cls]   ",
          L"[cls3]"
        );
    }

    ctstring result;
    for(unsigned a=0; a<content.size(); ++a)
    {
        // First test for defined symbols.
        //if(current_typeface == -1)
        {
            bool foundsym = false;
            
            wstring bs; bs += content[a];
            wstring us = content.substr(a);
            
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
        wchar_t c = content[a];
        if(c != L'[')
        {
            // No code, translate byte.
            ctchar chronoc = 0;
            
            switch(model.type)
            {
                case stringdata::zptr12:
                    chronoc = getctchar(c, cset_12pix);
                    break;
                case stringdata::zptr8:
                case stringdata::fixed:
                case stringdata::item:
                case stringdata::tech:
                case stringdata::monster:
                case stringdata::compressed7E:
                case stringdata::compressed7F:
                    chronoc = getctchar(c, cset_8pix);
                    break;
                case stringdata::locationevent:
                    ; // ignore, should not occur here
            }
            
            /* A slight optimization: We're not typeface-changing spaces! */
            if(current_typeface >= 0 && c != L' ')
            {
                unsigned offset = Typefaces[current_typeface].get_offset();
                chronoc += offset;
            }
            
            result += chronoc;
            continue;
        }
        
        wstring code;
        for(code += c; ++a < content.size(); )
        {
            c = content[a];
            code += c;
            if(c == L']')break;
        }
        
        // Codes used by dialog engine
        static const wstring delay = L"[delay ";
        static const wstring tech  = L"[tech]";
        static const wstring monster=L"[monster]";
        
        // Codes used by status screen engine
        static const wstring code0 = L"[gfx";
        static const wstring code1 = L"[next";
        static const wstring code2 = L"[goto,";
        static const wstring code3 = L"[func1,";
        static const wstring code4 = L"[substr,";
        static const wstring code5 = L"[member,";
        static const wstring code6 = L"[attrs,";
        static const wstring code7 = L"[out,";
        static const wstring code8 = L"[spc,";
        static const wstring code9 = L"[len,";
        static const wstring code10= L"[attr,";
        static const wstring code11= L"[func2,";
        static const wstring code12= L"[stat,";
        
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
                const wstring& begin = Typefaces[a].get_begin_marker();
                const wstring& end   = Typefaces[a].get_end_marker();
                
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
            
            if(model.type != stringdata::compressed7E
            && model.type != stringdata::compressed7F)
            {
                /* raw codes are only allowed in compressed data. */
                /* for others, remember the warning. */
                rawcodes.insert(code);
            }
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
    
    wchar_t c;

    ScriptCharGet getter(fp);
#define cget(c) (c=getter.getc())
    
    cget(c);
    
    stringdata model;

    rawcodes.clear();
    
    std::wstring unexpected;
    #define UNEXPECTED(c) unexpected += (c)
    #define EXPECTED() \
        if(!unexpected.empty()) \
        { \
            MessageUnexpected(unexpected); \
            unexpected.clear(); \
        }

    MessageLoadingDialog();
    
    for(;;)
    {
        MessageWorking();

        if(c == (wchar_t)EOF)break;
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
                if(c == (wchar_t)EOF || isspace(c))break;
                header += WcharToAsc(c);
            }
            while(c != (wchar_t)EOF && c != '\n') { cget(c); }
            
            model.ref_id  = 0;
            model.tab_id  = 0;
            model.width   = 0;
            model.address = 0;

            if(header.size() >= 1
            && (header[0] == 'z' || header[0] == 'Z'))
            {
                list<ReferMethod> refs;
                for(unsigned a=1; a < header.size() && header[a] == ':'; )
                {
                    char type = header[++a];
                    unsigned addr = 0;
                    while(++a < header.size() && header[a] != ':')
                        CumulateBase62(addr, header, header[a]); 
                    
                    if(IsSNESbased(addr))
                    {
                        fprintf(stderr,
                            "\nWarning: '%s' contains a SNES-based address.\n",
                            header.c_str());
                        addr = SNES2ROMaddr(addr);
                    }
                    
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
                    
                    if(header[0] == 'Z')
                    {
                        fprintf(stderr, "\nWarning: *Z ineffective (*z assumed) when pointers used\n");
                        header[0] = 'z';
                    }
                }
                
                if(header[0] == 'Z')
                {
                    model.tab_id = ++table_counter;
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
            else if(header.size() >= 1 && header[0] == 'c')
            {
                model.type = stringdata::compressed7E;
                MessageC8Section(header);
            }
            else if(header.size() >= 5 && header[0] == 'e')
            {
                model.type    = stringdata::locationevent;
                model.address = 0;
                bool ok=true;
                for(unsigned a=1; a<header.size(); ++a)
                {
                    if(!std::isalnum(header[a]) && a<5) {ok=false; break; }
                    if(!::CumulateBase62(model.address, header[a]))
                        { ok=false; break; }
                }
                if(ok)
                    MessageESection(header);
                else
                    MessageUnknownHeader(header);
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
            
            std::string slabel;
            unsigned label = 0;
            for(;;)
            {
                cget(c);
                if(c == (wchar_t)EOF || c == ':')break;
                
                if(!header.empty() && (header[0] == 'd' || header[0] == 's'))
                {
                    // freespace and dict labels are hex
                    if(!CumulateBase16(label, header, c))
                        break;
                }
                else if(!header.empty() && header[0] == 'e')
                {
                    // location event labels are just strings.
                    if(!CumulateAlnumString(slabel, header, c))
                        break;
                }
                else
                {
                    // other labels are 62-base (10+26+26) numbers
                    if(!CumulateBase62(label, header, c))
                        break;
                }
            }
            wstring content;
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
                if(c == (wchar_t)EOF)break;
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
                        if(c == ']' || c == (wchar_t)EOF)break;
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
                        
                        wstring bs; bs += content[a];
                        wstring us = content.substr(a);
                        
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
                    wchar_t c = content[a];
                    if(c != L'[')
                    {
                        // No code, translate byte.
                        ctchar chronoc = getctchar(c, cset_12pix);
                        dictword += chronoc;
                        continue;
                    }
                    
                    wstring code;
                    for(code += c; ++a < content.size(); )
                    {
                        c = content[a];
                        code += c;
                        if(c != L']')break;
                    }
                    
                    dictword += atoi(code.c_str()+1, 16);
                }

                dict.push_back(dictword);
                continue;
            }
            
            // It was one of these:
            //   'z' (dialog)
            //   'Z' (dialog but moved)
            //   'r' (8pix)
            //   'l' (fixed)
            //   'i' (relocatable item)
            //   't' (relocatable tech)
            //   'm' (relocatable monster)
            //   'c' (compressed data)

            if(model.type == stringdata::item
            || model.type == stringdata::tech
            || model.type == stringdata::monster
              )
            {
                // Mark the space unused already
                freespace.Add(label, model.width);
            }
            
            if(model.type == stringdata::locationevent)
            {
                // handle content.
                // label: slabel
            }
            else
            {
                model.str = ParseScriptEntry(content, model);
                model.address = label;

                strings.push_back(model);
            }
            continue;
        }
        
        UNEXPECTED(c);
    }
    
    MessageDone();
    
    if(!rawcodes.empty())
    {
        fprintf(stderr, "Warning: Raw codes encountered:");
        for(set<wstring>::const_iterator i=rawcodes.begin(); i!=rawcodes.end(); ++i)
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
            
            /* & 0x3FFFFF removed from here */
            objects.AddLump(Buf, pos, "lstring");
        }
    }
}

namespace
{
    struct ScriptStat
    {
        unsigned begin;
        unsigned size;
    public:
        ScriptStat() : begin(0), size(0) {}
        void Update(unsigned addr)
        {
            if(!size || begin > addr) begin = addr;
            size += 2;
        }
    };
}

void insertor::WriteOtherStrings()
{
    /* Identify relocated string tables */
    /* This value can't exist in tables naturally. */
    objects.DefineSymbol("RELOCATED_STRING_SIGNATURE", 0xFFFF);

    map<unsigned, PagePtrList> pagemap;
    map<unsigned, PagePtrList> refmap;
    map<unsigned, PagePtrList> tabmap;
    
    map<unsigned, ScriptStat> refstats;
    vector<ScriptStat> tables(table_counter);
    
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
                freespace.Add(i->address, 2);
                
                refstats[i->ref_id-1].Update(i->address);
            }
            else if(i->tab_id)
            {
                tabmap[i->tab_id].AddItem(data, i->address & 0xFFFF);
                tables[i->tab_id-1].Update(i->address);
            }
            else
                pagemap[i->address >> 16].AddItem(data, i->address & 0xFFFF);
        }
    }

    for(map<unsigned, PagePtrList>::iterator
        i = pagemap.begin(); i != pagemap.end(); ++i)
    {
        MessageLoadingItem(format("page $%02X", i->first));
        
        MessageWorking();
        
        i->second.Create(*this, i->first, "zstring");
    }

    for(map<unsigned, PagePtrList>::iterator
        i = refmap.begin(); i != refmap.end(); ++i)
    {
        unsigned ref_id = i->first - 1;
        
        //unsigned table_bytes = refstats[ref_id].size;
        unsigned table_start = refstats[ref_id].begin;
        
        const std::string Symbol = format("reloc_ref_%u_zstring", ref_id);
        MessageLoadingItem(Symbol);
        
        MessageWorking();
        i->second.Create(*this,
                         Symbol/*description*/,
                         table_start,
                         Symbol/*tablename*/);
        
        const list<ReferMethod>& refs = refers[ref_id];
        for(list<ReferMethod>::const_iterator
            j = refs.begin(); j != refs.end(); ++j)
        {
            /* *j = the referer, Symbol = symbol */
            objects.AddReference(Symbol, *j);
        }
    }

    for(map<unsigned, PagePtrList>::iterator
        i = tabmap.begin(); i != tabmap.end(); ++i)
    {
        unsigned tab_id = i->first - 1;
        
        const std::string Symbol = format("reloc_tab_%u_zstring", tab_id);
        MessageLoadingItem(Symbol);
        
        unsigned table_bytes = tables[tab_id].size;
        unsigned table_start = tables[tab_id].begin;
        
        MessageWorking();
        i->second.Create(*this,
                         Symbol/*description*/,
                         table_start,
                         Symbol/*tablename*/);
        
        if(table_bytes < 5)
        {
            fprintf(stderr, "Error: Table %u (at $%06X) is too small (%u), should be >=5!\n",
                tab_id, table_start, table_bytes);
        }
        
        freespace.Add(table_start + 5, table_bytes - 5);
        
        O65 newtab;
        newtab.Resize(CODE, 5);
        newtab.DeclareWordRelocation(CODE, "RELOCATED_STRING_SIGNATURE", 0);
        newtab.DeclareLongRelocation(CODE, Symbol, 2);
        
        string tmp = Symbol;
        objects.AddObject(newtab, tmp+" referer", table_start);
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

void insertor::WriteCompressedStrings()
{
    unsigned counter = 0;
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        MessageWorking();
        if(i->type == stringdata::compressed7E)
        {
            const string s = GetString(i->str);
            const unsigned char* ptr = (const unsigned char*)s.data();
            vector<unsigned char> data = Compress(ptr, s.size(), 0x7E);
            
            std::string name = format("compr_data7E_%u", counter);
            objects.AddLump(data, name, name);
            objects.AddReference(name, LongPtrFrom(i->address));
        }
        else if(i->type == stringdata::compressed7F)
        {
            const string s = GetString(i->str);
            const unsigned char* ptr = (const unsigned char*)s.data();
            vector<unsigned char> data = Compress(ptr, s.size(), 0x7F);
            
            std::string name = format("compr_data7F_%u", counter);
            objects.AddLump(data, name, name);
            objects.AddReference(name, LongPtrFrom(i->address));
        }
    }
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
    WriteCompressedStrings();//anywhere in the world.
    MessageDone();
}
