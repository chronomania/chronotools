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
                string ascii = WstrToAsc(content);
                
                unsigned page=0, begin=0;
                sscanf(header.c_str(), "s%X", &page);
                sscanf(ascii.c_str(), "%X", &begin);
                
                fprintf(stderr, "Adding %u bytes of free space at %02X:%04X\n", label, page, begin);
                freespace[page].insert(pair<unsigned,unsigned> (begin, label));
                
                continue;
            }

            if(header.size() >= 3 && header[0] == 'd')
            {
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

