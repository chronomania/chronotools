#include <cstdio>
#include <cstdarg>
#include <list>

#include "ctinsert.hh"
#include "wstring.hh"
#include "ctcset.hh"
#include "miscfun.hh"

using namespace std;

namespace
{
    // Ilmoita missä wrapattiin
    const bool warn_wraps   = false;
    
    // Tarkista, aiheuttiko wrap ongelmia
    const bool verify_wraps = true;

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
    
    class Symbols
    {
        map<string, char> symbols2, symbols8, symbols16;

        void AddSym(const char *sym, char c, int targets)
        {
            // 16 instead of 12 because 12 = 8+4, and 8 is already used :)
            if(targets&2)symbols2[sym]=c;
            if(targets&8)symbols8[sym]=c;
            if(targets&16)symbols16[sym]=c;
        }
        
        void Load()
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
            defsym(Lucca,        0x15) // HUOMIO Lucca -> Lucan/Lucasta/Lucalle
            defsym(Robo,         0x16)
            defsym(Frog,         0x17)
            defsym(Ayla,         0x18)
            defsym(Magus,        0x19) // HUOMIO Magus -> Maguksen
            defbsym(crononick,   0x1A)
            defbsym(member1,     0x1B)
            defbsym(member2,     0x1C)
            defbsym(member3,     0x1D)
            defsym(Nadia,        0x1E)
            defbsym(item,        0x1F)
            defsym(Epoch,        0x20) // HUOMIO Epoch -> Epochin
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

            //fprintf(stderr, "%u 8pix symbols loaded\n", symbols8.size());
            //fprintf(stderr, "%u 16pix symbols loaded\n", symbols16.size());
        }
    public:
        Symbols()
        {
            Load();
            fprintf(stderr, "Built symbol converter\n");
        }
        const map<string, char> &operator[] (unsigned ind) const
        {
            switch(ind)
            {
                case 2: return symbols2;
                case 8: return symbols8;
                case 16:
                default: return symbols16;
            }
        }
    } Symbols;
    
    class Conjugatemap
    {
        typedef map<string, unsigned char> datamap_t;
        struct form
        {
            datamap_t   data;
            const char *explanation;
        };
        
        list<form> forms;
        
        void AddForm(const form &form)
        {
            forms.push_back(form);
        }
        
        void AddData(datamap_t &target, const string &s) const
        {
            const map<string, char> &symbols16 = Symbols[16];
            const char *name = "...";
            /* Simple method to see which character are we talking about.      */
            /* Modify it if the first character isn't enough in your language. */
            switch(s[0])
            {
                case 'C': name = "Crono"; break;
                case 'L': name = "Lucca"; break;
                case 'M': name = s[2]=='g' ? "Magus" : "Marle"; break;
                case 'R': name = "Robo"; break;
                case 'F': name = "Frog"; break;
                case 'A': name = "Ayla"; break;
                case 'E': name = "Epoch"; break;
                
                // Nadia can't be renamed, so it
                // does not need to be taken care of.
            }
            unsigned char person = symbols16.find(name)->second;
            string key = str_replace(name, person, s);
            for(unsigned a=0; a<key.size(); ++a)
                if(key[a] >= 'a' && key[a] <= 'z')
                    key[a] = getchronochar(key[a]);
            
            target[key] = person;
        }
        datamap_t CreateMap(const char *word, ...) const
        {
            datamap_t result;
            va_list ap;
            va_start(ap, word);
            while(word)
            {
                AddData(result, word);
                word = va_arg(ap, const char *);
            }
            va_end(ap);
            return result;
        }
        void Verify(const string &s,
                    const string &plaintext,
                    const form &form) const
        {
            const datamap_t &tool   = form.data;
            const char *explanation = form.explanation;
            
            datamap_t::const_iterator i;
            for(i=tool.begin(); i!=tool.end(); ++i)
            {
                for(unsigned a=0; a < s.size(); )
                {
                    unsigned b = s.find(i->first, a);
                    if(b == s.npos) break;
                    
                    const char *what = "???";
                    for(map<string, char>::const_iterator
                        j=Symbols[16].begin();
                        j!=Symbols[16].end(); ++j)
                    {
                        if(j->second == i->second) { what = j->first.c_str(); break; }
                    }
                    
                    fprintf(stderr, "\nWarning: %s%s in '%s'",
                        what, explanation, plaintext.c_str());
                    
                    a = b + i->first.size();
                }
            }
        }
        void Load()
        {
            form tmp;
            tmp.data = CreateMap
                ( "Cronon", "Marlen", "Luccan", "Lucan",
                  "Robon", "Frogin", "Aylan", "Maguksen",
                  "Magusin", "Epochin", 0 );
            tmp.explanation = "-n";
            AddForm(tmp);
            
            tmp.data = CreateMap
                ( "Cronoa", "Marlea", "Luccaa",
                  "Roboa", "Frogia", "Froggia",
                  "Aylaa", "Magusta", "Epochia", 0 );
            tmp.explanation = "-a";
            AddForm(tmp);
            
            tmp.data = CreateMap
                ( "Cronolla", "Marlella", "Luccalla", "Lucalla",
                  "Robolla", "Frogilla", "Aylalla", "Maguksella",
                  "Magusilla", "Epochilla", 0 );
            tmp.explanation = "-lla";
            AddForm(tmp);
            
            tmp.data = CreateMap
                ( "Cronolle", "Marlelle", "Luccalle", "Lucalle",
                  "Robolle", "Frogille", "Aylalle", "Magukselle",
                  "Magusille", "Epochille", 0 );
            tmp.explanation = "-lle";
            AddForm(tmp);
            
            tmp.data = CreateMap
                ( "Cronosta", "Marlesta", "Luccasta", "Lucasta",
                  "Robosta", "Frogista", "Aylasta", "Maguksesta",
                  "Magusista", "Epochista", 0 );
            tmp.explanation = "-sta";
            AddForm(tmp);
        }
    public:
        Conjugatemap()
        {
            Load();
            fprintf(stderr, "Built conjugater-map\n");
        }
        void Verify(const string &s, const string &plaintext) const
        {
            for(list<form>::const_iterator
                i = forms.begin();
                i != forms.end();
                ++i)
            {
                Verify(s, plaintext, *i);
            }
        }
    } Conjugatemap;

    const string Rivita(const string &dialog, const insertor &ins)
    {
        //fprintf(stdout, "Rivita('%s')\n", dialog.c_str());
        
        unsigned row=0, col=0;
        
        string result;
        
        unsigned space3width = (ins.GetFont12width( getchronochar(' ') ) ) * 3;
        unsigned num8width   = (ins.GetFont12width( getchronochar('8') ) );
        unsigned w5width     = (ins.GetFont12width( getchronochar('W') ) ) * 5;
        
        unsigned wrappos = 0;
        unsigned wrapcol = 0;
        
        bool wraps = false;
        bool wrap_indent = false;
        
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
                case 0xA0 ... 0xFF:
                {
                    unsigned width = ins.GetFont12width(c);
                    col += width;
                    break;
                }
            }
            if(col >= 256 - 16)
            {
                /*
                fprintf(stderr, "\nWrap (col=%3u,row=%3u), result=%s\n",
                    col,row,ins.DispString(result).c_str());
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
                    col,row,ins.DispString(result).c_str());
                */
            }
            bool errors = false;
            if(row >= 4)
            {
                if(!errors){errors=true;fprintf(stderr,"\n");}
                fprintf(stderr, "Error: Four lines is maximum\n");
            }
            if(col >= 256-16)
            {
                if(!errors){errors=true;fprintf(stderr,"\n");}
                fprintf(stderr, "Error: Too long line\n");
            }
            if(errors)
            {
                fprintf(stderr, "In %s\n",
                    ins.DispString(dialog).c_str());
            }
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
                      ins.DispString(dialog).c_str(),
                      ins.DispString(result).c_str()
                 );
            }
            if(verify_wraps)
            {
                result = Rivita(result, ins);
            }
        }
        
        Conjugatemap.Verify(result, ins.DispString(result));
        
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
                        fprintf(stderr, " \nWarning: Raw code: %s", code.c_str());
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
}
