#include <cstdio>
#include <set>
#include <list>

#include "ctinsert.hh"
#include "wstring.hh"
#include "ctcset.hh"
#include "miscfun.hh"
#include "symbols.hh"
#include "conjugate.hh"
#include "config.hh"

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
        
        wstring Comment;

        ucs4 getc_priv()
        {
            for(;;)
            {
                if(cacheptr < cache.size())
                    return cache[cacheptr++];
                
                CLEARSTR(cache);
                cacheptr = 0;
                while(!cache.size())
                {
                    int c = fgetc(fp);
                    if(c == EOF)break;
                    // conv.putc may generate any amount of wchars, including 0
                    cache = conv.putc(c);
                }
                // So now cache may be of arbitrary size.
                if(!cache.size()) return (ucs4)EOF;
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
        const wstring &getcomment() const { return Comment; }
    };

    const string Rivita(const string &dialog, const insertor &ins)
    {
        // Standard defines that this'll be initialized upon the first call.
        static const bool warn_wraps   = GetConf("readin", "warn_wraps");
        static const bool verify_wraps = GetConf("readin", "verify_wraps");
        static unsigned MaxTextWidth   = GetConf("readin", "maxtextwidth");
        
        unsigned row=0, col=0;
        
        string result;
        
        static const unsigned char spacechar = getchronochar(' ');
        static const unsigned char colonchar = getchronochar(':');
        static const unsigned char eightchar = getchronochar('8');
        static const unsigned char dblw_char = getchronochar('W');

        unsigned space3width = (ins.GetFont12width( spacechar ) ) * 3;
        unsigned num8width   = (ins.GetFont12width( eightchar ) );
        unsigned w5width     = (ins.GetFont12width( dblw_char ) ) * 5;
        
        unsigned wrappos = 0;
        unsigned wrapcol = 0;
        
        bool wraps = false;
        bool wrap_indent = false;
        
        bool linelength_error = false;
        bool linecount_error = false;
        
        for(unsigned a=0; a<dialog.size(); ++a)
        {
            unsigned char c = dialog[a];
            
            if(c == spacechar)
            {
                if(!col) wrap_indent = true;
                wrappos = result.size();
                wrapcol = col;
            }
            if(c == colonchar)
                wrap_indent = true;

            result += c;
            switch(c)
            {
                case 0x05: // nl
                    ++row;
                    col = 0;
                    wrap_indent = false;
                    break;
                case 0x06: // nl3
                    ++row;
                    col = space3width;
                    wrap_indent = true;
                    break;
                case 0x0B: // pause
                case 0x09: // cls
                    row = 0;
                    col = 0;
                    wrap_indent = false;
                    break;
                case 0x0C: // pause3
                case 0x0A: // cls3
                    row = 0;
                    col = space3width;
                    wrap_indent = true;
                    break;
                case 0x0D: // num8  -- assume 88
                    col += num8width * 2;
                    break;
                case 0x0E: // num16 -- assume 88888
                    col += num8width * 5;
                    break;
                case 0x0F: // num32 -- assume 8888888888
                    col += num8width * 10;
                    break;
                case 0x1F: // item -- assume "" - FIXME
                {
                    col += 1;
                    break;
                }
                case 0x11: // User definable names
                case 0x13: // member, Crono
                case 0x14: // Marle
                case 0x15: // Lucca
                case 0x16: // Robo
                case 0x17: // Frog
                case 0x18: // Ayla
                case 0x19: // Magus
                case 0x1A: // crononick
                case 0x1B: // member1, member2, member3
                case 0x1C: // and Epoch
                case 0x1D: // Assume all of them are of maximal
                case 0x20: // length, that is 'WWWWW'.
                {
                    col += w5width;
                    break;
                }
                case 0x1E: // Nadia:
                {
                    // This name can't be changed by the player
                    col += ins.GetFont12width(getchronochar('N'));
                    col += ins.GetFont12width(getchronochar('a'));
                    col += ins.GetFont12width(getchronochar('d'));
                    col += ins.GetFont12width(getchronochar('i'));
                    col += ins.GetFont12width(getchronochar('a'));
                    break;
                }
                default:
                    if(c >= (0x100-get_num_chronochars()))
                    {
                        unsigned width = ins.GetFont12width(c);
                        col += width;
                        break;
                    }
            }
            if(col >= MaxTextWidth)
            {
                /*
                fprintf(stderr, "\nWrap (col=%3u,row=%3u), result=%s\n",
                    col,row,DispString(result).c_str());
                */
                wraps = true;
                
                col -= wrapcol;
                if(wrap_indent)
                {
                    result[wrappos] = 0x06; // nl3
                    col += space3width;
                }
                else
                {
                    result[wrappos] = 0x05; // nl
                }
                wrappos = result.size();
                wrapcol = col;
                ++row;
                /*
                fprintf(stderr,  "Now  (col=%3u,row=%3u), result=%s\n",
                    col,row,DispString(result).c_str());
                */
            }
            if(row >= 4) linecount_error = true;
            if(col >= MaxTextWidth) linelength_error = true;
        }
        
        if(linecount_error || linelength_error)
        {
            fprintf(stderr,
                " \n"
                "Error: Too long text\n"
                "In:  %s\n"
                "Out: %s\n",
                    DispString(dialog).c_str(),
                    DispString(result).c_str());
        }
        
        if(wraps)
        {
            if(warn_wraps)
            {
                fprintf(stderr,
                  " \n"
                  "Warning: Done some wrapping\n"
                  "In:  %s\n"
                  "Out: %s\n",
                      DispString(dialog).c_str(),
                      DispString(result).c_str()
                 );
            }
            if(verify_wraps)
            {
                result = Rivita(result, ins);
            }
        }
        
        Conjugatemap.Work(result);
        
#if 0
        fprintf(stdout, "Rivita('%s')\n"
                        "->     '%s'\n",
            DispString(dialog).c_str(),
            DispString(result).c_str());
#endif
        
        return result;
    }
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
    const map<wstring, char> *symbols = NULL;
    
    set<wstring> rawcodes;
    
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
                symbols = &Symbols[16];
                fprintf(stderr, "Loading strings for %s (%s)", header.c_str(),
                    WstrToAsc(getter.getcomment()).c_str());
            }
            else if(header == "r")
            {
                model.type = stringdata::zptr8;
                symbols = &Symbols[2];
                fprintf(stderr, "Loading strings for %s (%s)", header.c_str(),
                    WstrToAsc(getter.getcomment()).c_str());
            }
            else if(header.size() > 1 && header[0] == 'l')
            {
                model.type = stringdata::fixed;
                model.width = atoi(header.c_str() + 1);
                symbols = &Symbols[8];
                fprintf(stderr, "Loading strings for %s (%s)", header.c_str(),
                    WstrToAsc(getter.getcomment()).c_str());
            }
            else if(header.size() > 3 && header[0] == 'd')
            {
                sscanf(header.c_str()+1, "%u", &dictsize);
            }
            else if(header.size() > 1 && header[0] == 's')
            {
                // ok
            }
            else
            {
                fprintf(stderr, "Unknown header '%s'\n", header.c_str());
                symbols = NULL;
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

            if(header.size() >= 3 && header[0] == 'd')
            {
                // Dictionary record
                
                string newcontent;
                for(unsigned a=0; a<content.size(); ++a)
                    newcontent += getchronochar(content[a]);
                dict.push_back(newcontent);
                continue;
            }
            
            if(header.size() >= 1 && header[0] == 'z')
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
            
            string newcontent;
            wstring code;
            for(unsigned a=0; a<content.size(); ++a)
            {
                map<wstring, char>::const_iterator i;
                ucs4 c = content[a];
                for(unsigned testlen=3; testlen<=5; ++testlen)
                {
                    if(symbols == NULL) continue;
                    code = content.substr(a, testlen);
                    i = symbols->find(code);
                    if(i == symbols->end()) continue;
                    
                    a += testlen-1;
                    newcontent += i->second;
                    goto GotCode;
                }
                
                if(c == AscToWchar('['))
                {
                    CLEARSTR(code);
                    for(code += c; ; )
                    {
                        c = content[++a];
                        code += c;
                        if(WcharToAsc(c) == ']')break;
                    }
                    
                    // Codes used by dialog engine
                    static const wstring delay = AscToWstr("[delay ");
                    static const wstring tech  = AscToWstr("[tech]");
                    static const wstring monster=AscToWstr("[monster]");
                    
                    // Codes used by status screen engine
                    static const wstring code1 = AscToWstr("[next");
                    static const wstring code2 = AscToWstr("[goto,");
                    static const wstring code3 = AscToWstr("[func1,");
                    static const wstring code4 = AscToWstr("[substr,");
                    static const wstring code5 = AscToWstr("[member,");
                    static const wstring code6 = AscToWstr("[attrs,");
                    static const wstring code7 = AscToWstr("[out,");
                    static const wstring code8 = AscToWstr("[spc,");
                    static const wstring code9 = AscToWstr("[len,");
                    static const wstring code10= AscToWstr("[attr,");
                    static const wstring code11= AscToWstr("[func2,");
                    static const wstring code12= AscToWstr("[stat,");
                    static const wstring code0 = AscToWstr("[gfx");
                    
                    bool is_12 = header.size() > 0 && header[0] == 'z';
                    bool is_8  = header.size() > 0 && header[0] == 'r';
                    
                    if(symbols != NULL && (i = symbols->find(code)) != symbols->end())
                    {
                        newcontent += i->second;
                    }
                    else if(is_12 && code.substr(0, delay.size()) == delay)
                    {
                        newcontent += (char)3;
                        newcontent += (char)atoi(code.c_str() + delay.size(), 10);
                    }
                    else if(is_12 && code.substr(0, tech.size()) == tech)
                    {
                        newcontent += (char)0x12;
                        newcontent += (char)0x00;
                    }
                    else if(is_12 && code.substr(0, monster.size()) == monster)
                    {
                        newcontent += (char)0x12;
                        newcontent += (char)0x01;
                    }
                    else if(is_8 && code.substr(0, code0.size()) == code0)
                    {
                    Handle8Code:
                        const wchar_t *data = code.c_str();
                        while(*data && *data != ',') ++data;
                        while(*data == ',')
                        {
                            unsigned value = 0, digits = 0;
                            for(++data; *data && *data!=',' && *data!=']'; ++data,++digits)
                                value = value*16 + Whex(*data);
                            while(digits >= 2)
                            {
                                newcontent += (char)(value & 255);
                                value >>= 8;
                                digits -= 2;
                            }
                        }
                    }
                    else if(is_8 && code.substr(0, code1.size()) == code1) { newcontent += (char)1; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code2.size()) == code2) { newcontent += (char)2; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code3.size()) == code3) { newcontent += (char)3; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code4.size()) == code4) { newcontent += (char)4; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code5.size()) == code5) { newcontent += (char)5; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code6.size()) == code6) { newcontent += (char)6; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code7.size()) == code7) { newcontent += (char)7; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code8.size()) == code8) { newcontent += (char)8; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code9.size()) == code9) { newcontent += (char)9; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code10.size()) == code10) { newcontent += (char)10; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code11.size()) == code11) { newcontent += (char)11; goto Handle8Code; }
                    else if(is_8 && code.substr(0, code12.size()) == code12) { newcontent += (char)12; goto Handle8Code; }
                    else
                    {
                        newcontent += (char)atoi(code.c_str()+1, 16);
                        rawcodes.insert(code);
                    }
                }
                else
                {
                    unsigned char chronoc = getchronochar(c);
                    newcontent += chronoc;
                }
            GotCode: ;
            }
            
            if(model.type == stringdata::zptr12)
                newcontent = Rivita(newcontent, *this);
            
            model.str = newcontent;
            strings[label] = model;
            
            static char cursbuf[]="-/|\\",curspos=0;
            if(!(curspos%4)) fprintf(stderr,"%c\010",cursbuf[curspos/4]);
            curspos=(curspos+1)%(4*4);
            if(c == '*')fputs(" \n", stderr);
           
            continue;
        }
        fprintf(stderr, "Unexpected char '%c'\n", c);
        cget(c);
    }
    if(rawcodes.size())
    {
        fprintf(stderr, "Warning: Raw codes encountered:");
        for(set<wstring>::const_iterator i=rawcodes.begin(); i!=rawcodes.end(); ++i)
            fprintf(stderr, " %s", WstrToAsc(*i).c_str());
        fprintf(stderr, "\n");
    }
}
