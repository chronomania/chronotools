#include <cstdio>
#include <set>
#include <list>

#include "ctinsert.hh"
#include "wstring.hh"
#include "ctcset.hh"
#include "miscfun.hh"
#include "symbols.hh"
#include "conjugate.hh"

using namespace std;

#include "settings.hh"

namespace
{
    // How many pixels at max
    const unsigned MaxTextWidth =
      // 256 is the width of the screen
      256
      // The left border is 8 pixels
      - 8
      // Right border is... 16 pixels?
      - 8;

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
                    cache = conv.putc(c);
                }
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
        unsigned row=0, col=0;
        
        string result;
        
        unsigned space3width = (ins.GetFont12width( getchronochar(' ') ) ) * 3;
        unsigned num8width   = (ins.GetFont12width( getchronochar('8') ) );
        unsigned w5width     = (ins.GetFont12width( getchronochar('W') ) ) * 5;
        
        unsigned wrappos = 0;
        unsigned wrapcol = 0;
        
        bool wraps = false;
        bool wrap_indent = false;
        
        bool linelength_error = false;
        bool linecount_error = false;
        
        for(unsigned a=0; a<dialog.size(); ++a)
        {
            unsigned char c = dialog[a];

            if(c == getchronochar(' '))
            {
                if(!col) wrap_indent = true;
                wrappos = result.size();
                wrapcol = col;
            }
            if(c == getchronochar(':'))
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
                case 0x1F: // item -- assume ""
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
                default:
                    if(c >= (0x100-Num_Characters))
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
                "\n"
                "Error: Too long text\n"
                "In: %s\n",
                    DispString(result).c_str());
        }
        
        if(wraps)
        {
            if(warn_wraps)
            {
                fprintf(stderr,
                  "\n"
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
        
        Conjugatemap.Work(result, DispString(result));
        
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
    const map<string, char> *symbols = NULL;
    
    set<string> rawcodes;
    
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
                header += (char)c;
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
            else
            {
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
                
                if(header.size() > 0 && (header[0] == 'd' || header[0] == 's'))
                {
                    // dictionary labels and free space counts are decimal
                    if(isdigit(c))
                        label = label * 10 + c - '0';
                    else
                    {
                        fprintf(stderr, "$%u: Got char '%c', invalid is!\n", label, c);
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
                        fprintf(stderr, "$%X: Got char '%c', invalid is!\n", label, c);
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
                
                unsigned page=0, begin=0;
                sscanf(header.c_str(), "s%X", &page);
                sscanf(ascii.c_str(), "%X", &begin);
                
                freespace.Add(page, begin, label);
                
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
            
            string newcontent, code;
            for(unsigned a=0; a<content.size(); ++a)
            {
                map<string, char>::const_iterator i;
                unsigned char c = content[a];
                for(unsigned testlen=3; testlen<=5; ++testlen)
                    if(symbols != NULL && (i = symbols->find
                        (code = WstrToAsc(content.substr(a, testlen)))
                                          ) != symbols->end())
                    {
                        a += testlen-1;
                        newcontent += i->second;
                        goto GotCode;
                    }
                if(c == '[')
                {
                    code = "[";
                    for(;;)
                    {
                        c = content[++a];
                        code += (char)c;
                        if(c == ']')break;
                    }
                    
                    if(symbols != NULL && (i = symbols->find(code)) != symbols->end())
                    {
                        newcontent += i->second;
                    }
                    else if(code.substr(0, 7) == "[delay ")
                    {
                        newcontent += (char)3;
                        newcontent += (char)atoi(code.c_str() + 7);
                    }
                    else if(code.substr(0, 6) == "[skip ")
                    {
                        newcontent += (char)8;
                        newcontent += (char)atoi(code.c_str() + 6);
                    }
                    else if(code.substr(0, 5) == "[ptr ")
                    {
                        newcontent += (char)5;
                        unsigned k = strtol(code.c_str() + 5, NULL, 16);
                        newcontent += (char)(k & 255);
                        newcontent += (char)(k >> 8);
                    }
                    else if(isdigit(code[1]))
                    {
                        newcontent += (char)atoi(code.c_str()+1);
                        rawcodes.insert(code);
                    }
                    else
                    {
                        fprintf(stderr, " \nUnknown code: %s", code.c_str());
                    }
                }
                else
                    newcontent += getchronochar(c);
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
        for(set<string>::const_iterator i=rawcodes.begin(); i!=rawcodes.end(); ++i)
            fprintf(stderr, " %s", i->c_str());
        fprintf(stderr, "\n");
    }
}
