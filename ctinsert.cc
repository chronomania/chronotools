#include <cstdio>
#include <cctype>
#include <string>
#include <cstdlib>
#include <utility>
#include <ctime>
#include <cmath>
#include <vector>
#include <algorithm>
#include <list>
#include <set>
#include <map>

using namespace std;

#include "ctcset.hh"
#include "miscfun.hh"

static const bool rebuild_dict = false;

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

    typedef pair<unsigned/*pos*/,unsigned/*len*/> freespacerec;
    typedef set<freespacerec> freespaceset;
    typedef map<unsigned/*page*/, freespaceset> freespacemap;
    freespacemap freespace;
    typedef map<unsigned, stringdata> stringmap;
    stringmap strings;

    vector<string> dict;
    unsigned dictaddr, dictsize;

public:
    void LoadCharSet();
    void LoadSymbols();
    void LoadFile(FILE *fp);

    string DispString(const string &s);
    void MakeDictionary();
    
    unsigned FindFreeSpace(unsigned page, unsigned length)
    {
        freespacemap::iterator mapi = freespace.find(page);
        if(mapi == freespace.end())
        {
            fprintf(stderr,
                "Can't find %u bytes of free space in page %02X - no space there at all!\n",
                length, page);
            return 0x10000;
        }
        freespaceset &spaceset = mapi->second;
        freespaceset::const_iterator reci;

        unsigned biggest = 0;
        freespaceset::const_iterator best = spaceset.end();
        
        for(reci = spaceset.begin(); reci != spaceset.end(); ++reci)
        {
            if(reci->second == length)
            {
                unsigned pos = reci->first;
                // Found exact match!
                spaceset.erase(reci);
                return pos;
            }
            if(reci->second < length)
                continue;
            
            if(reci->second > biggest)
            {
                biggest = reci->second;
                best = reci;
                //break;
            }
        }
        if(biggest < length)
        {
            fprintf(stderr,
                "Can't find %u bytes of free space in page %02X - biggest continuous space is only %u bytes!\n",
                length, page, biggest);
            return 0x10000;
        }
        unsigned bestpos = best->first;
        freespacerec tmp(bestpos + length, biggest - length);
        spaceset.erase(best);
        spaceset.insert(tmp);
        return bestpos;
    }
    
    void WritePPtr(vector<unsigned char> &ROM,
                   vector<bool> &Touched,
                   unsigned pointeraddr,
                   const string &string)
    {
        unsigned page = pointeraddr >> 16;
        unsigned spaceptr = FindFreeSpace(page, string.size() + 1);
        if(spaceptr == 0x10000)
            return;
        
        Touched[pointeraddr] = true;
        Touched[pointeraddr+1] = true;
        
        ROM[pointeraddr  ] = spaceptr & 255;
        ROM[pointeraddr+1] = spaceptr >> 8;
        
        fprintf(stderr, "Wrote %u bytes at %06X->%04X\n", string.size()+1, pointeraddr, spaceptr);
        
        spaceptr += page<<16;
        for(unsigned a=0; a<=string.size(); ++a)
        {
            if(!a)ROM[spaceptr] = string.size();
            else ROM[spaceptr+a] = string[a-1];
            Touched[spaceptr+a] = true;
        }
    }

    unsigned WriteZPtr(vector<unsigned char> &ROM,
                       vector<bool> &Touched,
                       unsigned pointeraddr,
                       const string &string,
                       unsigned spaceptr=0x10000)
    {
        unsigned page = pointeraddr >> 16;
        
        if(spaceptr == 0x10000)
        {
            spaceptr = FindFreeSpace(page, string.size() + 1);
            if(spaceptr == 1)
                return 0x10000;
        }
        
        Touched[pointeraddr] = true;
        Touched[pointeraddr+1] = true;
        
        ROM[pointeraddr  ] = spaceptr & 255;
        ROM[pointeraddr+1] = spaceptr >> 8;
        
        fprintf(stderr, "Wrote %u bytes at %06X->%04X: ", string.size()+1, pointeraddr, spaceptr);
        //fprintf(stderr, DispString(string).c_str());
        fprintf(stderr, "\n");
        
        spaceptr += page<<16;
        for(unsigned a=0; a<=string.size(); ++a)
        {
            if(a==string.size()) ROM[spaceptr+a]=0;
            else ROM[spaceptr+a] = string[a];
            Touched[spaceptr+a] = true;
        }
        return spaceptr & 0xFFFF;
    }
    
    void WriteROM()
    {
        vector<unsigned char> ROM(4194304,        0);
        vector<bool>      Touched(ROM.size(), false);
        
        fprintf(stderr, "Writing dictionary...\n");
        for(unsigned a=0; a<dictsize; ++a)
            WritePPtr(ROM, Touched, dictaddr + a*2, dict[a]);
        
        stringmap::const_iterator i;

        vector<string>   pagestrings;
        vector<unsigned> pageoffs;
        unsigned prevpage = 0xFFFF;
        for(i=strings.begin(); ; ++i)
        {
            if(i == strings.end()) goto Flush16;

            if(i->second.type != stringdata::zptr16
            && i->second.type != stringdata::zptr8)continue;
            
            if((i->first >> 16) != prevpage)
            {
        Flush16:;
                // terminology: potilas=needle, tohtori=haystack

                multimap<unsigned/*potilas*/, unsigned/*tohtori*/> belong1;
                multimap<unsigned/*tohtori*/, unsigned/*potilas*/> belong2;
                
                fprintf(stderr, "Writing %u strings...\n", pagestrings.size());
                for(unsigned potilasnum=0; potilasnum<pagestrings.size(); ++potilasnum)
                {
                    const string &potilas = pagestrings[potilasnum];
                    unsigned longest=0, tohtoribest=0;
                    for(unsigned tohtorinum=potilasnum+1;
                                 tohtorinum<pagestrings.size();
                                 ++tohtorinum)
                    {
                        const string &tohtori = pagestrings[tohtorinum];
                        if(tohtori.size() < potilas.size())
                            continue;
                        
                        unsigned extralen = tohtori.size()-potilas.size();
                        if(potilas == tohtori.substr(extralen))
                            if(extralen > longest)
                            {
                                longest = extralen;
                                tohtoribest = tohtorinum;
                            }
                    }
                    if(longest)
                    {
                        fprintf(stderr, "String %u(",potilasnum);
                        fprintf(stderr, DispString(pagestrings[potilasnum]).c_str());
                        fprintf(stderr, ") depends on string %u(",tohtoribest);
                        fprintf(stderr, DispString(pagestrings[tohtoribest]).c_str());
                        fprintf(stderr, ")\n");
                        
                        belong1.insert(pair<unsigned,unsigned> (potilasnum, tohtoribest));
                        belong2.insert(pair<unsigned,unsigned> (tohtoribest, potilasnum));
                    }
                }
                /* Nyt noissa mapeissa on listattu, kuka
                 * tarvitsee ket‰kin. Tarvitsemisketjut
                 * eiv‰t voi muodostaa ympyr‰‰, joten nyt
                 * pit‰‰ vain asettaa elementit siihen
                 * j‰rjestykseen, miss‰ ensinn‰ ovat ne,
                 * ketk‰ eiv‰t tarvitse mit‰‰n. Sen j‰lkeen
                 * ne, jotka tarvitsevat, lis‰t‰‰n erikseen
                 * muokaten pointtereita.
                 */
                
                set<unsigned> done;
                while(done.size() != pagestrings.size())
                    for(unsigned a=0; a<pagestrings.size(); ++a)
                    {
                        if(done.find(a) != done.end())continue;
                        multimap<unsigned,unsigned>::const_iterator i = belong1.find(a);
                        // If this string does not require any other string
                        if(i == belong1.end())
                        {
                            pageoffs[a] = WriteZPtr(ROM, Touched, pageoffs[a], pagestrings[a]);
                            done.insert(a);
                        }
                        else
                        {
                            set<unsigned>::const_iterator p = done.find(i->second);
                            if(p != done.end()) /* The depending string has already been dumped */
                            {
                                unsigned b = i->second;
                                //fprintf(stderr, "Reusing pointer!\n");
                                pageoffs[a] = WriteZPtr(ROM, Touched, pageoffs[a], pagestrings[a],
                                                        pageoffs[b] + pagestrings[b].size()-pagestrings[a].size());
                                done.insert(a);
                            }
                        }
                    }
                
                if(i == strings.end())break;
                pagestrings.clear();
                pageoffs.clear();
            }
            prevpage = i->first >> 16;
            
            string s = i->second.str;
            
            if(i->second.type == stringdata::zptr16)
            {
                unsigned char Buf[2];
                Buf[0] = 0x21;
                Buf[1] = 0;

                vector<string>::const_iterator l;
                for(l = dict.begin(); l != dict.end(); ++l)
                {
                    s = str_replace(*l, (const char *)Buf, s);
                    ++Buf[0];
                    if(Buf[0] == 0xA0)Buf[0] = 0xF1;
                }
            }
            
            pagestrings.push_back(s);
            pageoffs.push_back(i->first);
        }
        
        FILE *fp = fopen("ctpatch-nohdr.ips", "wb");
        FILE *fp2 = fopen("ctpatch-hdr.ips", "wb");
        fwrite("PATCH", 1, 5, fp);
        fwrite("PATCH", 1, 5, fp2);
        /* Format:   24bit offset, 16-bit size, then data; repeat */
        for(unsigned a=0; a<Touched.size(); ++a)
        {
            if(!Touched[a])continue;
            putc((a>>16)&255, fp);
            putc((a>> 8)&255, fp);
            putc((a    )&255, fp);
            putc(((a+512)>>16)&255, fp2);
            putc(((a+512)>> 8)&255, fp2);
            putc(((a+512)    )&255, fp2);
            unsigned offs=a, c=0;
            while(a < Touched.size() && Touched[a])
                ++c, ++a;
            putc((c>> 8)&255, fp);
            putc((c    )&255, fp);
            putc((c>> 8)&255, fp2);
            putc((c    )&255, fp2);
            fwrite(&ROM[offs], 1, c, fp);
            fwrite(&ROM[offs], 1, c, fp2);
        }
        fwrite("EOF",   1, 5, fp);
        fwrite("EOF",   1, 5, fp2);
        fclose(fp);
        fclose(fp2);
    }
};

string insertor::DispString(const string &s)
{
    string result;
    for(unsigned a=0; a<s.size(); ++a)
    {
        unsigned char c = characterset[(unsigned char)s[a]];
        if((char)c != '∂')
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
            else { sprintf(Buf, "[%02X]", c); result += Buf; }
        }
    }
    return result;
}

static bool dictsorter(const string &a, const string &b)
{
    if(a.size() > b.size())return true;
    if(a.size() < b.size())return false;
    if(a < b)return true;
    return false;
}

/* This is DarkForce's hashing code */
static unsigned hashstr(const char *s, unsigned len)
{
    unsigned h = 0;
    for(unsigned a=0; a<len; ++a)
    {
        unsigned char c = s[a];
        c = h ^ c;
        h ^= (c * 707106);
    }
    return (h&0x0000FF00) | ((h>>16)&0xFF);
}

/* Build the dictionary. */
void insertor::MakeDictionary()
{
    list<string> stringlist;
    
    map<unsigned, stringdata>::const_iterator i;
    for(i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->second.type != stringdata::zptr16)
            continue;
        stringlist.push_back(i->second.str);
    }
    
    unsigned origsize=0, dictbytes=0;
    for(list<string>::iterator l = stringlist.begin(); l != stringlist.end(); ++l)
        origsize += l->size();
    
    if(rebuild_dict)
    {
        dict.clear();
        time_t begin = time(NULL);
        for(unsigned substrcount=0; substrcount<dictsize; ++substrcount)
        {
            //map<unsigned, unsigned> substringtable;
            //map<unsigned, string>   hashtable;
            map<string, unsigned> substringtable;
            
            fprintf(stderr, "Finding substrings... %u/%u", substrcount+1, dictsize);

            unsigned tickpos=0;
            
            /* For each string */
            list<string>::iterator l;
            unsigned stringcounter=0;
            for(l = stringlist.begin(); l != stringlist.end(); ++l, ++stringcounter)
            {
                if(tickpos)
                    --tickpos;
                else
                {
                    /* Just tell where we are */
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
                /* Substrings start from each position in the string */
                for(unsigned a=0; a<s.size(); ++a)
                {
                    unsigned c=0;
                    /* And are each length */
                    for(unsigned b=a; b<s.size(); ++b)
                    {
                        unsigned char ch = s[b];
                        /* But can't contain all possible bytes */
                        if(ch < 0xA0) // || ch == 0xEF)
                            break;
                        if(++c >= 2)
                        {
                            /* Cumulate the substring usage counter by its length */
                            substringtable[s.substr(a,c)] += c; /*
                            unsigned hash = hashstr(s.c_str()+a, c);
                            substringtable[hash] += c;
                            if(hashtable.find(hash) == hashtable.end())
                            {
                                const string substr = s.substr(a, c);
                                hashtable.insert(pair<unsigned,string> (hash,substr));
                            } */
                        }
                        if(c >= 16)break;
                    }
                }
            }
            
            fprintf(stderr, "\r%8u substrings; ", substringtable.size());
            
            //map<unsigned, unsigned>::const_iterator bestj = substringtable.end();
            map<string, unsigned>::const_iterator j,bestj = substringtable.end();
            
            /* Now find the substring that has the biggest score */
            int bestscore=0;
            for(j = substringtable.begin();
                j != substringtable.end();
                ++j)
            {
                const string &word = j->first;//hashtable[j->first];
                int realscore = j->second - (j->second / word.size());
                if(realscore > bestscore)
                {
                    bestj = j;
                    bestscore= realscore;
                }
            }
            
            /* Add it to dictionary */
            const string &bestword = bestj->first; //hashtable[bestj->first];
            
            printf("$%u:\t%s; score: %u\n", dict.size(), DispString(bestword).c_str(), bestscore);
            /*fprintf(stderr, "%4u: '", bestscore);
            fprintf(stderr, DispString(bestword).c_str());
            fprintf(stderr, "'%30c", '\n');*/
            fflush(stdout);
            
            dictbytes += bestword.size()+1;
            dict.push_back(bestword);
            
            /* Remove the selected substring from the source strings,
             * so that it won't be used in any other substrings.
             * It would mess up the statistics otherwise.
             */
            for(l = stringlist.begin(); l != stringlist.end(); ++l)
                *l = str_replace(bestword, "?", *l);
        }
    }
    else
    {
        //sort(dict.begin(), dict.end(), dictsorter);
        unsigned col=0;
        vector<string>::iterator d;
        for(d=dict.begin(); d!=dict.end(); ++d)
        {
            const string &bestword = *d;

            fprintf(stderr, "'%s%*c",
                DispString(bestword).c_str(),
                bestword.size()-12, '\'');
            if(++col==6){col=0;putc('\n',stderr);}
            
            dictbytes += bestword.size()+1;
            
            list<string>::iterator l;
            for(l = stringlist.begin(); l != stringlist.end(); ++l)
                *l = str_replace(bestword, "?", *l);
        }
        if(col)putc('\n', stderr);
    }

    unsigned resultsize=0;
    for(list<string>::iterator l = stringlist.begin(); l != stringlist.end(); ++l)
        resultsize += l->size();
    
    fprintf(stderr, "Original script size: %u bytes; new script size: %u bytes\n"
                    "Saved: %u bytes; dictionary size: %u bytes\n",
        origsize, resultsize, origsize-resultsize, dictbytes);
}

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
            /*string spaces;*/
            for(;;)
            {
                const bool beginning = (c == '\n');
                cget(c);
                if(c == EOF)break;
                if(beginning && (c == '*' || c == '$'))break;
                if(c == '\n') { /*spaces = "";*/ continue; }
                /*if(c == ' ') { spaces += c; continue; }
                if(spaces.size())
                {
                    content += spaces;
                    spaces="";
                }*/
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
    
    ins.MakeDictionary();
    
    ins.WriteROM();
    
    return 0;
}
