#include <cstdio>
#include <set>
#include <list>

#include "ctinsert.hh"
#include "wstring.hh"
#include "ctcset.hh"
#include "miscfun.hh"
#include "symbols.hh"

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
        
        ucs4string Comment;

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
                    int c = fgetc(fp);
                    if(c == EOF)break;
                    // conv.putc may generate any amount of wchars, including 0
                    cache = conv.putc(c);
                }
                // So now cache may be of arbitrary size.
                if(cache.empty()) return (ucs4)EOF;
            }
        }
    public:
        ScriptCharGet(FILE *f) : fp(f), cacheptr(0)
        {
            conv.SetSet(getcharset());
            fprintf(stderr, "Built script character set converter\n");
        }
        ucs4 getc()
        {
            ucs4 c = getc_priv();
            if(c == ';')
            {
                CLEARSTR(Comment);
                for(;;)
                {
                    c = getc_priv();
                    if(c == '\n' || c == (ucs4)EOF) break;
                    Comment += c;
                }
            }
            return c;
        }
        const ucs4string &getcomment() const { return Comment; }
    };
}

namespace
{
    set<ucs4string> rawcodes;
}

const ctstring insertor::ParseScriptEntry(const ucs4string &input, const stringdata &model) const
{
    ucs4string content = input;
    
    const bool is_dialog = model.type == stringdata::zptr12;
    const bool is_8pix   = model.type == stringdata::zptr8;
    const bool is_fixed  = model.type == stringdata::fixed;

    const Symbols::type &symbols
        = Symbols.GetMap(is_dialog ? 16
                       : is_8pix   ? 8
                       : is_fixed  ? 2
                       : 0);

    if(is_dialog)
    {
        content = str_replace
        (
          AscToWstr(" [pause]"),
          AscToWstr("[pause]"),
          content
        );
        content = str_replace
        (
          AscToWstr("[nl]   "),
          AscToWstr("[nl3]"),
          content
        );
        content = str_replace
        (
          AscToWstr("[pause]   "),
          AscToWstr("[pause3]"),
          content
        );
        content = str_replace
        (
          AscToWstr("[pausenl]   "),
          AscToWstr("[pausenl3]"),
          content
        );
        content = str_replace
        (
          AscToWstr("[cls]   "),
          AscToWstr("[cls3]"),
          content
        );
    }

    ctstring result;
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
                if(content.compare(a, i->first.size(), i->first) == 0)
                {
                    unsigned testlen = i->first.size();
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
            ctchar chronoc = getchronochar(c);
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
        else if(is_dialog && code.compare(0, delay.size(), delay) == 0)
        {
            result += (ctchar)3;
            result += (ctchar)atoi(code.c_str() + delay.size(), 10);
        }
        else if(is_dialog && code.compare(0, tech.size(), tech) == 0)
        {
            result += (ctchar)0x12;
            result += (ctchar)0x00;
        }
        else if(is_dialog && code.compare(0, monster.size(), monster) == 0)
        {
            result += (ctchar)0x12;
            result += (ctchar)0x01;
        }
        else if(is_8pix && code.compare(0, code0.size(), code0) == 0)
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
        else if(is_8pix && code.compare(0, code1.size(), code1) == 0) { result += 1; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code2.size(), code2) == 0) { result += 2; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code3.size(), code3) == 0) { result += 3; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code4.size(), code4) == 0) { result += 4; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code5.size(), code5) == 0) { result += 5; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code6.size(), code6) == 0) { result += 6; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code7.size(), code7) == 0) { result += 7; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code8.size(), code8) == 0) { result += 8; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code9.size(), code9) == 0) { result += 9; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code10.size(), code10) == 0) { result += 10; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code11.size(), code11) == 0) { result += 11; goto Handle8Code; }
        else if(is_8pix && code.compare(0, code12.size(), code12) == 0) { result += 12; goto Handle8Code; }
        else
        {
            result += atoi(code.c_str()+1, 16);
            rawcodes.insert(code);
        }
    }
    
    if(is_dialog)
        result = WrapDialogLines(result);

    return result;
}

void insertor::LoadFile(FILE *fp)
{
    if(!fp)return;
    
    string header;
    
    ucs4 c;

    ScriptCharGet getter(fp);
#define cget(c) (c=getter.getc())
    
    cget(c);
    
    stringdata model;

    rawcodes.clear();
    
    for(;;)
    {
        if(c == (ucs4)EOF)break;
        if(c == '\n')
        {
            cget(c);
            continue;
        }

        if(c == '*')
        {
            header = "";
            for(;;)
            {
                cget(c);
                if(c == (ucs4)EOF || isspace(c))break;
                header += WcharToAsc(c);
            }
            while(c != (ucs4)EOF && c != '\n') { cget(c); }
            
            if(header == "z")
            {
                model.type = stringdata::zptr12;
                fprintf(stderr, "Loading strings for %s (%s)", header.c_str(),
                    WstrToAsc(getter.getcomment()).c_str());
            }
            else if(header == "r")
            {
                model.type = stringdata::zptr8;
                fprintf(stderr, "Loading strings for %s (%s)", header.c_str(),
                    WstrToAsc(getter.getcomment()).c_str());
            }
            else if(header.size() > 1 && header[0] == 'l')
            {
                model.type = stringdata::fixed;
                model.width = atoi(header.c_str() + 1);
                fprintf(stderr, "Loading strings for %s (%s)", header.c_str(),
                    WstrToAsc(getter.getcomment()).c_str());
            }
            else if(header.size() >= 1 && header[0] == 'd')
            {
                // ok
            }
            else if(header.size() > 1 && header[0] == 's')
            {
                // ok
            }
            else
            {
                fprintf(stderr, "Unknown header '%s'\n", header.c_str());
            }
            
            continue;
        }
        
        if(c == '$')
        {
            unsigned label = 0;
            for(;;)
            {
                cget(c);
                if(c == (ucs4)EOF || c == ':')break;
                
                if(header.size() > 0 && header[0] == 'd')
                {
                    // dictionary labels are decimal
                    if(isdigit(c))
                        label = label * 10 + c - '0';
                    else
                    {
                        fprintf(stderr, "$%u: Got char '%c', invalid is (in label)!\n", label, c);
                    }
                }
                else if(header.size() > 0 && header[0] == 's')
                {
                    // freespace labels are hex
                    if(isdigit(c))
                        label = label * 16 + c - '0';
                    else if(c >= 'A' && c <= 'z')
                        label = label * 16 + c + 10 - 'A';
                    else if(c >= 'A' && c <= 'z')
                        label = label * 16 + c + 10 - 'a';
                    else
                    {
                        fprintf(stderr, "$%X: Got char '%c', invalid is (in label)!\n", label, c);
                    }
                }
                else
                {
                    // other labels are 62-base (10+26+26) numbers
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
                        fprintf(stderr, "$%s", tmp.c_str());
                        fprintf(stderr, ": Got char '%c', expected ':' (in label)!\n", c);
                        break;
                    }
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
                
                ctstring dictword;
                for(unsigned a=0; a<content.size(); ++a)
                    dictword += getchronochar(content[a]);
                dict.push_back(dictword);
                continue;
            }
            
            // Either 'z' (dialog), 'l' (fixed) or 'r' (8pix)
            
            model.str = ParseScriptEntry(content, model);
            model.address = label;
            
            strings.push_back(model);
            
            static char cursbuf[]="-/|\\",curspos=0;
            if(!(curspos%4)) fprintf(stderr,"%c\010",cursbuf[curspos/4]);
            curspos=(curspos+1)%(4*4);
            if(c == '*')fputs(" \n", stderr);
           
            continue;
        }
        fprintf(stderr, "Unexpected char '%c'\n", c);
        cget(c);
    }
    
    if(!rawcodes.empty())
    {
        fprintf(stderr, "Warning: Raw codes encountered:");
        for(set<ucs4string>::const_iterator i=rawcodes.begin(); i!=rawcodes.end(); ++i)
            fprintf(stderr, " %s", WstrToAsc(*i).c_str());
        fprintf(stderr, "\n");
    }
}
