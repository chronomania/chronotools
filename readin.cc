#include <cstdio>

#include "ctinsert.hh"
#include "wstring.hh"
#include "ctcset.hh"
#include "miscfun.hh"

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

namespace
{
    map<string, char> symbols2, symbols8, symbols16;

    void AddSym(const char *sym, char c, int targets)
    {
        if(targets&2)symbols2[sym]=c;
        if(targets&8)symbols8[sym]=c;
        if(targets&16)symbols16[sym]=c;
    }
    void LoadSymbols()
    {
        int targets=0;

        // Define macro
        #define defsym(sym, c) AddSym(#sym,static_cast<char>(c),targets);
                               
        // Define bracketed macro
        #define defbsym(sym, c) defsym([sym], c)
        
        // 8pix symbols;  *xx:xxxx:Lxx
        // 16pix symbols; *xx:xxxx:Zxx
        
        targets=2+8+16;
        defbsym(end,         0x00)
        targets=2;
        defbsym(nl,          0x01)
        targets=16;
        // 0x01 seems to be garbage
        // 0x02 seems to be garbage too
        // 0x03 is delay, handled elseway
        // 0x04 seems to do nothing
        defbsym(nl,          0x05)
        defbsym(nl3,         0x06)
        defbsym(pausenl,     0x07)
        defbsym(pausenl3,    0x08)
        defbsym(cls,         0x09)
        defbsym(cls3,        0x0A)
        defbsym(pause,       0x0B)
        defbsym(pause3,      0x0C)
        defbsym(num8,        0x0D)
        defbsym(num16,       0x0E)
        defbsym(num32,       0x0F)
        // 0x10 seems to do nothing
        defbsym(member,      0x11)
        defbsym(tech,        0x12)
        defsym(Crono,        0x13)
        defsym(Marle,        0x14)
        defsym(Lucca,        0x15)
        defsym(Robo,         0x16)
        defsym(Frog,         0x17)
        defsym(Ayla,         0x18)
        defsym(Magus,        0x19)
        defbsym(crononick,   0x1A)
        defbsym(member1,     0x1B)
        defbsym(member2,     0x1C)
        defbsym(member3,     0x1D)
        defsym(Nadia,        0x1E)
        defbsym(item,        0x1F)
        defsym(Epoch,        0x20)
        targets=8;
        defbsym(bladesymbol, 0x20)
        defbsym(bowsymbol,   0x21)
        defbsym(gunsymbol,   0x22)
        defbsym(armsymbol,   0x23)
        defbsym(swordsymbol, 0x24)
        defbsym(fistsymbol,  0x25)
        defbsym(scythesymbol,0x26)
        defbsym(armorsymbol, 0x28)
        defbsym(helmsymbol,  0x27)
        defbsym(ringsymbol,  0x29)
        defbsym(shieldsymbol,0x2E)
        defbsym(starsymbol,  0x2F)
        targets=16;
        defbsym(musicsymbol, 0xEE)
        defbsym(heartsymbol, 0xF0)
        defsym(...,          0xF1)
        
        #undef defsym
        #undef defbsym

        fprintf(stderr, "%u 8pix symbols loaded\n", symbols8.size());
        fprintf(stderr, "%u 16pix symbols loaded\n", symbols16.size());
    }
}

void insertor::LoadFile(FILE *fp)
{
    if(!fp)return;
    
    LoadSymbols();
    
    string header;
    
    ucs4 c;

    ScriptCharGet getter(fp);
#define cget(c) (c=getter.getc())
    
    cget(c);
    
    stringdata model;
    const map<string, char> *symbols = NULL;
    
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
                model.type = stringdata::zptr16;
                symbols = &symbols16;
                fprintf(stderr, "Loading strings for %s (%s)", header.c_str(),
                    WstrToAsc(getter.getcomment()).c_str());
            }
            else if(header == "r")
            {
                model.type = stringdata::zptr8;
                symbols = &symbols2;
                fprintf(stderr, "Loading strings for %s (%s)", header.c_str(),
                    WstrToAsc(getter.getcomment()).c_str());
            }
            else if(header.size() > 1 && header[0] == 'l')
            {
                model.type = stringdata::fixed;
                model.width = atoi(header.c_str() + 1);
                symbols = &symbols8;
                fprintf(stderr, "Loading strings for %s (%s)", header.c_str(),
                    WstrToAsc(getter.getcomment()).c_str());
            }
            else if(header.size() > 3 && header[0] == 'd')
            {
                sscanf(header.c_str()+1, "%X:%u", &dictaddr, &dictsize);
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
            for(;;)
            {
                const bool beginning = (c == '\n');
                cget(c);
                if(c == (ucs4)EOF)break;
                if(beginning && (c == '*' || c == '$'))break;
                if(c == '\n') { continue; }
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
                GotCode:
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
                        fprintf(stderr, " \nWarning: Raw code: %s", code.c_str());
                    }
                    else
                    {
                        fprintf(stderr, " \nUnknown code: %s", code.c_str());
                    }
                }
                else
                    newcontent += getchronochar(c);
            }
            
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
}
