#include <cstdio>
#include <cctype>
#include <string>
#include <cstdlib>
#include <utility>
#include <ctime>
#include <cmath>
#include <list>
#include <set>
#include <map>

using namespace std;

#include "ctcset.hh"
#include "miscfun.hh"

class insertor
{
    char revcharset[256];

    map<string, char> symbols2, symbols8, symbols16;
    
    typedef struct
    {
        string str;
        enum { zptr8, zptr16, fixed } type;
        unsigned width; // used if type==fixed;
    } stringdata;

    map<unsigned/*page*/, set<pair<unsigned/*pos*/,unsigned/*len*/> > > freespace;
    map<unsigned, stringdata> strings;

    list<string> dict;
    unsigned dictaddr, dictsize;

public:
    void LoadCharSet();
    void LoadSymbols();
    void LoadFile(FILE *fp);

    void DispString(const string &s)
    {
        string result;
        for(unsigned a=0; a<s.size(); ++a)
        {
            unsigned char c = characterset[(unsigned char)s[a]];
            if((char)c != '¶')
            {
                result += c;
            }
            else
            {
                c = (unsigned char)s[a];
                char Buf[64];
                if(c == 5) result += "[nl]";
                else if(c == 6) result += "[nl3]";
                else if(c == 11) result += "[pause]";
                else if(c == 12) result += "[pause3]";
                //else if(c == 0) result += "[end]";
                else { sprintf(Buf, "[%d]", c); result += Buf; }
            }
        }
        fprintf(stderr, "%s", result.c_str());
    }
    
    void Dump()
    {
        map<string, unsigned> substringtable;
        
        list<string> stringlist;
        
        map<unsigned, stringdata>::const_iterator i;
        for(i=strings.begin(); i!=strings.end(); ++i)
        {
        	if(i->second.type != stringdata::zptr16)
        	    continue;
            stringlist.push_back(i->second.str);
        }
        
        unsigned origsize=0;
        for(list<string>::iterator l = stringlist.begin(); l != stringlist.end(); ++l)
        	origsize += l->size();
        
        time_t begin = time(NULL);
        list<string> substrlist;
        #if 0
        for(unsigned substrcount=0; substrcount<dictsize; ++substrcount)
        {
            map<string, unsigned>::const_iterator bestj = substringtable.end();
            
            fprintf(stderr, "Finding substrings... %u/%u", substrcount+1, dictsize);
            
            substringtable.clear();

			unsigned tickpos=0;

            list<string>::iterator l;
            unsigned stringcounter=0;
            for(l = stringlist.begin(); l != stringlist.end(); ++l, ++stringcounter)
            {
            	if(tickpos)
            		--tickpos;
            	else
            	{
            		tickpos=256;
            		double totalpos = substrcount;
            		totalpos += stringcounter / (double)stringlist.size();
            		totalpos /= (double)dictsize;
            		if(totalpos==0.0)totalpos=1e-10;
            		//totalpos = pow(totalpos, 0.3);
            		
            		time_t now = time(NULL);
            		double diff = difftime(now, begin);
            		double total = diff/totalpos;
            		double left = total-diff;
            		char Buf[64];
            		sprintf(Buf, " - about %.2f minutes left...", left/60.0);
            		fputs(Buf, stderr);
            		for(unsigned a=0; Buf[a]; ++a)putc(8, stderr);
            	}
            	
                const string &s = *l;
                for(unsigned a=0; a<s.size(); ++a)
                {
                    unsigned c=0;
                    for(unsigned b=a; b<s.size(); ++b)
                    {
                        unsigned char ch = s[b];
                        if(ch < 0xA0) // || ch == 0xEF)
                            break;
                        if(++c >= 2)
                        {
                        	const string substr = s.substr(a, c);
                            substringtable[substr] += c;
                        }
                        if(c >= 16)break;
                    }
                }
            }
            
            fprintf(stderr, "\r%8u substrings; ", substringtable.size());
            
            int bestscore=0;
            for(map<string, unsigned>::const_iterator
                j = substringtable.begin();
                j != substringtable.end();
                ++j)
            {
            	int realscore = j->second - (j->second / j->first.size());
                if(realscore > bestscore)
                {
                    bestj = j;
                    bestscore= realscore;
                }
            }
            
            const string bestword = bestj->first;

            fprintf(stderr, "%4u: '", bestscore);
            DispString(bestword);
            fprintf(stderr, "'%30c", '\n');
            
            substrlist.push_back(bestword);
            
            for(l = stringlist.begin(); l != stringlist.end(); ++l)
            	*l = str_replace(bestword, "?", *l);
        }
        #else
        list<string>::iterator d;
        for(d=dict.begin(); d!=dict.end(); ++d)
        {
        	const string &bestword = *d;

            fprintf(stderr, "%4u: '", 0);
            DispString(bestword);
            fprintf(stderr, "'%30c", '\n');
            
        	list<string>::iterator l;
            for(l = stringlist.begin(); l != stringlist.end(); ++l)
            	*l = str_replace(bestword, "?", *l);
        }
        #endif

        unsigned resultsize=0;
        for(list<string>::iterator l = stringlist.begin(); l != stringlist.end(); ++l)
        	resultsize += l->size();
        
        fprintf(stderr, "Original script size: %u bytes; new script size: %u bytes\nSaved: %u bytes\n",
        	origsize, resultsize, origsize-resultsize);
        
        /* FIXME: Write the dictionary (dictsize pointers) at dictaddr. */
    }
};

void insertor::LoadCharSet()
{
    for(unsigned a=0; a<256; ++a)
    {
        const char *c = strchr(characterset, (char)a);
        if(!c) revcharset[a] = '?';
        else revcharset[a] = (char)(c - characterset);
    }
}

void insertor::LoadSymbols()
{
    int targets=0;
    #define defsym(sym, c) if(targets&2)symbols2[#sym]=(char)(c);\
                           if(targets&8)symbols8[#sym]=(char)(c);\
                           if(targets&16)symbols16[#sym]=(char)(c);\
                           ;
                           
    #define defbsym(sym, c) defsym([sym], c)
    
    // 8pix symbols;  *xx:xxxx:Lxx
    // 16pix symbols; *xx:xxxx:Zxx
    
    targets=2+8+16;
    defbsym(end,         0x00)
    targets=16;
    defbsym(nl,          0x05)
    defbsym(nl3,         0x06)
    defbsym(cls1,        0x09)
    defbsym(cls2,        0x0A)
    defbsym(pause,       0x0B)
    defbsym(pause3,      0x0C)
    defbsym(num8,        0x0D)
    defbsym(num16,       0x0E)
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
    defbsym(starsymbol,  0x2F)
    targets=16;
    defbsym(musicsymbol, 0xEE)
    defbsym(heartsymbol, 0xF0)
    
    #undef defsym

    fprintf(stderr, "%u 8pix symbols loaded\n", symbols8.size());
    fprintf(stderr, "%u 16pix symbols loaded\n", symbols16.size());
}

void insertor::LoadFile(FILE *fp)
{
    if(!fp)return;
    
    string header;
    
    int c;

    #define cget(c) c=fgetc(fp);if(c==';'){do{c=fgetc(fp);}while(c != '\n' && c != EOF);}
    cget(c);
    
    stringdata model;
    unsigned stringaddr;
    const map<string, char> *symbols = NULL;
    
    for(;;)
    {
        if(c == EOF)break;
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
                if(c == EOF || isspace(c))break;
                header += (char)c;
            }
            while(c != EOF && c != '\n') { cget(c); }
            
            if(header.size() > 9 && header[8] == 'L')
            {
                model.type = stringdata::fixed;
                unsigned seg, ofs;
                sscanf(header.c_str(), "%X:%X:L%u", &seg, &ofs, &model.width);
                stringaddr = (seg << 16) + ofs;
                symbols = &symbols8;
            }
            else if(header.size() > 9 && header[8] == 'Z')
            {
                model.type = stringdata::zptr16;
                unsigned seg, ofs, stringcount;
                sscanf(header.c_str(), "%X:%X:Z%u", &seg, &ofs, &stringcount);
                stringaddr = (seg << 16) + ofs;
                symbols = &symbols16;
            }
            else if(header.size() > 9 && header[8] == 'R')
            {
                model.type = stringdata::zptr8;
                unsigned seg, ofs, stringcount;
                sscanf(header.c_str(), "%X:%X:R%u", &seg, &ofs, &stringcount);
                stringaddr = (seg << 16) + ofs;
                symbols = &symbols2;
            }
            else if(header.size() > 9 && header[8] == 'D')
            {
                unsigned seg, ofs, count;
                sscanf(header.c_str(), "%X:%X:D%u", &seg, &ofs, &count);
                dictaddr = (seg << 16) + ofs;
                dictsize = count;
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
                if(c == EOF || c == '\n' || c == ':')break;
                if(isdigit(c))
                    label = label * 10 + c - '0';
                else
                {
                    fprintf(stderr, "$%u: Got char '%c', invalid is!\n", label, c);
                }
            }
            if(c == ':')
            {
                cget(c);
                if(c != '\t')
                    fprintf(stderr, "$%u: Got char '%c', tab expected!\n", label, c);
            }
            string content;
            string spaces;
            for(;;)
            {
                const bool beginning = (c == '\n');
                cget(c);
                if(c == EOF)break;
                if(beginning && (c == '*' || c == '$'))break;
                if(c == '\n') { spaces = ""; continue; }
                if(c == ' ') { spaces += c; continue; }
                if(spaces.size())
                {
                    content += spaces;
                    spaces="";
                }
                if(c == '[')
                {
                    content += c;
                    for(;;)
                    {
                        cget(c);
                        content += c;
                        if(c == ']' || c == EOF)break;
                    }
                    continue;
                }
                content += c;
            }
            
            if(header.size() == 4 && header[3] == 'S')
            {
                unsigned page=0, begin=0;
                sscanf(header.c_str(), "%X:S", &page);
                sscanf(content.c_str(), "%X", &begin);
                
                //fprintf(stderr, "Adding %u bytes of free space at %02X:%04X\n", label, page, begin);
                freespace[page].insert(pair<unsigned,unsigned> (begin, label));
                
                continue;
            }

            if(header.size() >= 9 && header[8] == 'D')
            {
            	string newcontent;
            	for(unsigned a=0; a<content.size(); ++a)
            	    newcontent += revcharset[(unsigned char)content[a]];
                dict.push_back(newcontent);
                continue;
            }

            content = str_replace("[nl]   ",    "[nl3]", content);
            content = str_replace("[pause]   ", "[pause3]", content);
            
            string newcontent, code;
            
            for(unsigned a=0; a<content.size(); ++a)
            {
                map<string, char>::const_iterator i;
                char c = content[a];
                for(unsigned testlen=4; testlen<=5; ++testlen)
                    if(symbols != NULL && (i = symbols->find(code = content.substr(a, testlen))) != symbols->end())
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
                        code += c;
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
                    else if(isdigit(code[1]))
                    {
                        newcontent += (char)atoi(code.c_str()+1);
                        fprintf(stderr, "Warning: Raw code: %s\n", code.c_str());
                    }
                    else
                    {
                        fprintf(stderr, "Unknown code: %s\n", code.c_str());
                    }
                }
                else
                    newcontent += revcharset[(unsigned char)c];
            }
            
            model.str = newcontent;
            strings[stringaddr] = model;
            
            if(model.type == stringdata::zptr8
            || model.type == stringdata::zptr16
              )
                stringaddr += 2;
            else if(model.type == stringdata::fixed)
                stringaddr += model.width;
            
            if(!label)
                fprintf(stderr, "Loading strings for %s", header.c_str());
            static char cursbuf[4]="-/|\\",curspos=0;
            fprintf(stderr,"%c\010",cursbuf[++curspos&3]);
            if(c == '*')fputs(" \n", stderr);
           
            continue;
        }
        fprintf(stderr, "Unexpected char '%c'\n", c);
        cget(c);
    }
}

int main(void)
{
    insertor ins;
    
    ins.LoadCharSet();
    ins.LoadSymbols();
    
    FILE *fp = fopen("ct_eng.txt", "rt");
    ins.LoadFile(fp);
    fclose(fp);
    
    ins.Dump();
    
    return 0;
}
