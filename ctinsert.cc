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

#ifdef linux
#include <sys/mman.h>
#define USE_MMAP 1
#endif

using namespace std;

#include "wstring.hh"
#include "ctcset.hh"
#include "miscfun.hh"
#include "tgaimage.hh"
#include "ctinsert.hh"

static const bool rebuild_dict             = false;
static const bool sort_dictionary          = false;
static const unsigned MaxDictWordLen       = 20;
static const unsigned MaxDictRecursLen     = 3;
static const unsigned DictMaxSpacesPerWord = 4;
static const char font8fn[]  = "ct8fnFI.tga";
static const char font12fn[] = "ct16fnFI.tga";

struct todo_sort
{
    const vector<string> &tab;
    todo_sort(const vector<string> &t) : tab(t) { }
    bool operator() (unsigned a, unsigned b) const
    {
        return tab[a].size() > tab[b].size();
    }
};

unsigned insertor::WritePPtr
    (vector<unsigned char> &ROM,
     vector<bool> &Touched,
     unsigned pointeraddr,
     const string &string)
{
    unsigned page = pointeraddr >> 16;
    unsigned spaceptr = freespace.Find(page, string.size() + 1);
    if(spaceptr == NOWHERE)
        return spaceptr;
    
    WriteByte(ROM, Touched, pointeraddr,   spaceptr&255);
    WriteByte(ROM, Touched, pointeraddr+1, spaceptr>>8);
    
    //fprintf(stderr, "Wrote %u bytes at %06X->%04X\n", string.size()+1, pointeraddr, spaceptr);
    
    spaceptr += page<<16;
    for(unsigned a=0; a<=string.size(); ++a)
        WriteByte(ROM, Touched, spaceptr+a, a ? string[a-1] : string.size());
    
    return spaceptr & 0xFFFF;
}

unsigned insertor::WriteZPtr
   (vector<unsigned char> &ROM,
    vector<bool> &Touched,
    unsigned pointeraddr,
    const string &string,
    unsigned spaceptr)
{
    unsigned page = pointeraddr >> 16;
    
    if(spaceptr == NOWHERE)
    {
        spaceptr = freespace.Find(page, string.size() + 1);
        if(spaceptr == NOWHERE)
            return NOWHERE;
    }
    
    WriteByte(ROM, Touched, pointeraddr,   spaceptr&255);
    WriteByte(ROM, Touched, pointeraddr+1, spaceptr>>8);
    
    /*
    fprintf(stderr, "Wrote %u bytes at %06X->%04X: ",
        string.size()+1, pointeraddr, spaceptr);
    fprintf(stderr, DispString(string).c_str());
    fprintf(stderr, "\n");
    */
    
    spaceptr += page<<16;
    for(unsigned a=0; a<string.size(); ++a)
        WriteByte(ROM, Touched, spaceptr+a, string[a]);
    WriteByte(ROM, Touched, spaceptr+string.size(), 0);
    return spaceptr & 0xFFFF;
}

void insertor::Write8pixfont
   (vector<unsigned char> &ROM,
    vector<bool> &Touched)
{
    TGAimage font8(font8fn);
    font8.setboxsize(8, 8);
    font8.setboxperline(32);
    
    static const char palette[] = {0,3,1,2,0};
    
    vector<unsigned char> tiletable(256*16);
    
    unsigned to=0;
    for(unsigned a=0; a<256; ++a)
    {
        vector<char> box = font8.getbox(a);
        for(unsigned p=0; p<box.size(); ++p)
            if((unsigned char)box[p] < sizeof(palette))
                box[p] = palette[box[p]];
        
        unsigned po=0;
        for(unsigned y=0; y<8; ++y)
        {
            unsigned char byte1 = 0;
            unsigned char byte2 = 0;
            for(unsigned x=0; x<8; ++x)
            {
                unsigned shift = (7-x);
                byte1 |= ((box[po]&1)  ) << shift;
                byte2 |= ((box[po]&2)/2) << shift;
                ++po;
            }
            tiletable[to++] = byte1;
            tiletable[to++] = byte2;
        }
    }
    for(unsigned a=0; a<tiletable.size(); ++a)
        WriteByte(ROM, Touched, 0x3F8C60+a, tiletable[a]);
}

void insertor::Write12pixfont
   (vector<unsigned char> &ROM,
    vector<bool> &Touched)
{
    TGAimage font12(font12fn);
    font12.setboxsize(12, 12);
    font12.setboxperline(32);
    
    static const char palette[] = {0,0,1,2,3,0};
    
    vector<unsigned char> tiletable(96 * 24 + 96 * 12);
    
    unsigned to=0;
    for(unsigned a=0; a<96; ++a)
    {
        vector<char> box = font12.getbox(a);

        unsigned width=0;
        while(box[width] != 5 && width<12)++width;
        
        for(unsigned p=0; p<box.size(); ++p)
            if((unsigned char)box[p] < sizeof(palette))
                box[p] = palette[box[p]];
        
        unsigned po=0;
        for(unsigned y=0; y<12; ++y)
        {
            unsigned char byte1 = 0;
            unsigned char byte2 = 0;
            unsigned char byte3 = 0;
            unsigned char byte4 = 0;
            for(unsigned x=0; x<8; ++x)
            {
                unsigned shift = (7-x)&7;
                byte1 |= ((box[po]&1)  ) << shift;
                byte2 |= ((box[po]&2)/2) << shift;
                ++po;
            }
            for(unsigned x=0; x<4; ++x)
            {
                unsigned shift = (7-x)&7;
                byte3 |= ((box[po]&1)  ) << shift;
                byte4 |= ((box[po]&2)/2) << shift;
                ++po;
            }
            tiletable[to++] = byte1;
            tiletable[to++] = byte2;
            
            if(a&1)byte3 <<= 4;
            tiletable[96*24 + (a>>1)*24 + y*2  ] |= byte3;
            if(a&1)byte4 <<= 4;
            tiletable[96*24 + (a>>1)*24 + y*2+1] |= byte4;
        }
        
        WriteByte(ROM, Touched, 0x260E6+a, width);
    }
    for(unsigned a=0; a<tiletable.size(); ++a)
        WriteByte(ROM, Touched, 0x3F2F60+a, tiletable[a]);
}

void insertor::WriteROM()
{
    vector<unsigned char> ROM(4194304,        0);
    vector<bool>      Touched(ROM.size(), false);
    
    /*
    {FILE *fp = fopen("chrono-uncompressed.smc", "rb");
    if(fp){fseek(fp,512,SEEK_SET);fread(&ROM[0], 1, ROM.size(), fp);fclose(fp);}}
    */
    
    fprintf(stderr, "Initializing all free space to zero...\n");
    for(freespacemap::const_iterator i=freespace.begin(); i!=freespace.end(); ++i)
        for(freespaceset::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
        {
            unsigned offs = (i->first << 16) | j->first;
            for(unsigned a=0; a<j->second; ++a)
                WriteByte(ROM, Touched, offs+a, 0);
        }
    
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
        
        if(i->second.type == stringdata::fixed)
        {
            const string &s = i->second.str;
            unsigned pos = i->first;
            
            if(s.size() != i->second.width)
                fprintf(stderr, "Warning: Fixed string at %06X: len(%u) != space(%u)...\n",
                pos, s.size(), i->second.width);

            unsigned a;
            for(a=0; a<s.size(); ++a)      WriteByte(ROM, Touched, pos++, s[a]);
            for(; a < i->second.width; ++a)WriteByte(ROM, Touched, pos++, 0);
            continue;
        }

        if(i->second.type != stringdata::zptr16
        && i->second.type != stringdata::zptr8)continue;
        
        if((i->first >> 16) != prevpage)
        {
    Flush16:;
            fprintf(stderr, "Writing %u strings... (page %02X)\n",
                pagestrings.size(),
                i->first >> 16);
            
            // parasite -> host
            map<unsigned, unsigned> neederlist;
            for(unsigned parasitenum=0;
                         parasitenum < pagestrings.size();
                         ++parasitenum)
            {
                const string &parasite = pagestrings[parasitenum];
                for(unsigned hostnum = 0;
                             hostnum < pagestrings.size();
                             ++hostnum)
                {
                    // Can't depend on self
                    if(hostnum == parasitenum) { Continue: continue; }
                    // If the host depends on this "parasite", skip it
                    map<unsigned, unsigned>::iterator i;
                    for(unsigned tmp=hostnum;;)
                    {
                        i = neederlist.find(tmp);
                        if(i == neederlist.end()) break;
                        tmp = i->second;
                        // Host depends on "parasite", skip this
                        if(tmp == parasitenum) { goto Continue; }
                    }
                    
                    const string &host = pagestrings[hostnum];
                    if(host.size() < parasite.size())
                        continue;
                    
                    unsigned extralen = host.size()-parasite.size();
                    if(parasite == host.substr(extralen))
                    {
                        for(;;)
                        {
                            i = neederlist.find(hostnum);
                            if(i == neederlist.end()) break;
                            // Our "host" depends on someone else.
                            // Take his host instead.
                            hostnum = i->second;
                        }
                        
                        neederlist[parasitenum] = hostnum;
                        
                        // Now if there are parasites referring to this one
                        for(i=neederlist.begin(); i!=neederlist.end(); ++i)
                            if(i->second == parasitenum)
                            {
                                // rerefer them to this one's host
                                i->second = hostnum;
                            }

                        break;
                    }
                }
            }
            
#if 0
            map<string, vector<unsigned> > needertmp;
            /* Nyt mapissa on listattu, kuka tarvitsee ketäkin. */
            for(map<unsigned,unsigned>::const_iterator
                j = neederlist.begin();
                j != neederlist.end();
                ++j)
            {
                needertmp[pagestrings[j->first]].push_back(j->first);
            }
            for(map<string, vector<unsigned> >::const_iterator
                j = needertmp.begin();
                j != needertmp.end();
                ++j)
            {
                unsigned hostnum = 0;
                const vector<unsigned> &tmp = j->second;

                unsigned c=0;
                fprintf(stderr, "String%s", tmp.size()==1 ? "" : "s");
                for(unsigned a=0; a<tmp.size(); ++a)
                {
                    hostnum = neederlist[tmp[a]];
                    if(++c > 15) { fprintf(stderr, "\n   "); c=0; }
                    fprintf(stderr, " %u", tmp[a]);
                }
                
                fprintf(stderr, " (%s) depend%s on string %u(%s)\n",
                    DispString(j->first).c_str(),
                    tmp.size()==1 ? "s" : "", hostnum,
                    DispString(pagestrings[hostnum]).c_str());
            }
#endif
            
            // pageoffs, pagestrings have been defined before this "if" was met.
            
            if(true) /* First do hosts */
            {
                vector<unsigned> todo;
                todo.reserve(pagestrings.size() - neederlist.size());
                unsigned reusenum = 0;
                for(unsigned stringnum=0; stringnum<pagestrings.size(); ++stringnum)
                {
                    if(neederlist.find(stringnum) == neederlist.end())
                        todo.push_back(stringnum);
                    else
                        ++reusenum;
                }

                fprintf(stderr, "Writing %u strings and reusing %u\n", todo.size(), reusenum);
                sort(todo.begin(), todo.end(), todo_sort(pagestrings));
                
                for(unsigned a=0; a<todo.size(); ++a)
                {
                    unsigned stringnum = todo[a];
                    unsigned ptroffs = pageoffs[stringnum];
                    const string &str = pagestrings[stringnum];

                    pageoffs[stringnum] = WriteZPtr(ROM, Touched, ptroffs, str);
                }
            }
            if(true) /* Then do parasites */
            {
                for(unsigned stringnum=0; stringnum<pagestrings.size(); ++stringnum)
                {
                    map<unsigned, unsigned>::const_iterator i;
                    i = neederlist.find(stringnum);
                    if(i == neederlist.end()) continue;
                    
                    unsigned parasitenum = i->first;
                    unsigned hostnum     = i->second;

                    unsigned hostoffs = pageoffs[hostnum];
                            
                    unsigned ptroffs = pageoffs[parasitenum];
                    const string &str = pagestrings[parasitenum];
                    
                    if(hostoffs == NOWHERE)
                    {
                        fprintf(stderr, "Host %u doesn't exist! ", hostnum);
                        
                        // Try find another host
                        for(hostnum=0; hostnum<pagestrings.size(); ++hostnum)
                        {
                            // Skip parasites
                            if(neederlist.find(hostnum) != neederlist.end()) continue;
                            // Skip unwritten ones
                            if(pageoffs[hostnum] == NOWHERE) continue;
                            const string &host = pagestrings[hostnum];
                            // Impossible host?
                            if(host.size() < str.size()) continue;
                            unsigned extralen = host.size() - str.size();
                            if(str != host.substr(extralen)) continue;
                            hostoffs = pageoffs[hostnum];
                            break;
                        }
                        if(hostoffs == NOWHERE)
                        {
                            fprintf(stderr, "String %u not written!\n", parasitenum);
                            continue;
                        }
                        fprintf(stderr, "Substitute %u assigned for %u.\n",
                            hostnum, parasitenum);
                    }

                    const string &host = pagestrings[hostnum];
                    
                    unsigned place = hostoffs + host.size()-str.size();
                    
                    WriteZPtr(ROM, Touched, ptroffs, str, place);
                }
            }
            
            if(i == strings.end())break;
            pagestrings.clear();
            pageoffs.clear();
        }
        prevpage = i->first >> 16;
        
        string s = i->second.str;
        
        pagestrings.push_back(s);
        pageoffs.push_back(i->first);
    }
    
    Write8pixfont(ROM, Touched);
    Write12pixfont(ROM, Touched);
    
    /* Now write the patches */
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
        while(a < Touched.size() && Touched[a] && c < 20000)
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
    fclose(fp); fprintf(stderr, "Created %s\n", "ctpatch-nohdr.ips");
    fclose(fp2); fprintf(stderr, "Created %s\n", "ctpatch-hdr.ips");
}

string insertor::DispString(const string &s) const
{
    string result;
    wstringOut conv;
    conv.SetSet(getcharset());
    for(unsigned a=0; a<s.size(); ++a)
    {
        unsigned char c = s[a];
        ucs4 u = getucs4(c);
        if(u != ilseq)
        {
            result += conv.putc(u);
            continue;
        }
        switch(c)
        {
            case 0x05: result += conv.puts(AscToWstr("[nl]")); break;
            case 0x06: result += conv.puts(AscToWstr("[nl3]")); break;
            case 0x0B: result += conv.puts(AscToWstr("[pause]")); break;
            case 0x0C: result += conv.puts(AscToWstr("[pause3]")); break;
            case 0x00: result += conv.puts(AscToWstr("[end]")); break; // Uuh?
            case 0xF1: result += conv.puts(AscToWstr("...")); break;
            default:
            {
                char Buf[8];
                sprintf(Buf, "[%02X]", c);
                result += conv.puts(AscToWstr(Buf));
            }
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

/* Build the dictionary. */
void insertor::MakeDictionary()
{
    map<unsigned, unsigned> spaceperpage;
    
    for(freespacemap::const_iterator i=freespace.begin(); i!=freespace.end(); ++i)
    {
        unsigned thisfree = 0, hunkcount = 0;
        for(freespaceset::const_iterator j=i->second.begin(); j!=i->second.end(); ++j)
        {
            thisfree += j->second;
            ++hunkcount;
        }
        spaceperpage[i->first] = thisfree;
    }
    
    unsigned origsize=0, dictbytes=0;
    for(stringmap::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->second.type != stringdata::zptr16)continue;        
        origsize += i->second.str.size();
    }
    
    if(rebuild_dict)
    {
        replacedict.clear();
        dict.clear();
        
        time_t begin = time(NULL);
        fprintf(stderr,
            "Rebuilding the dictionary. This will take probably a long time!\n"
            "You should take a lunch break or something now.\n"
        );
        for(unsigned dictcount=0; dictcount<dictsize; ++dictcount)
        {
            fprintf(stderr, "Finding substrings... %u/%u", dictcount+1, dictsize);

            unsigned tickpos=0;

            /* Calculate the ROM usage for the strings per page */
            map<unsigned, unsigned> usageperpage;

            /* Note: This code is really copypaste from WriteROM() */
            vector<string> pagestrings;
            unsigned prevpage = 0xFFFF;
            for(stringmap::const_iterator i=strings.begin(); ; ++i)
            {
                if(i == strings.end()) goto Flush16;
                
                if(i->second.type != stringdata::zptr16
                && i->second.type != stringdata::zptr8)continue;
                
                if((i->first >> 16) != prevpage)
                {
            Flush16:;
                    // terminology: potilas=needle, tohtori=haystack
                    set<unsigned> needer;
                    for(unsigned potilasnum=0; potilasnum<pagestrings.size(); ++potilasnum)
                    {
                        const string &potilas = pagestrings[potilasnum];
                        for(unsigned tohtorinum=potilasnum+1;
                                     tohtorinum<pagestrings.size();
                                     ++tohtorinum)
                        {
                            const string &tohtori = pagestrings[tohtorinum];
                            if(tohtori.size() < potilas.size())
                                continue;
                            
                            unsigned extralen = tohtori.size()-potilas.size();
                            if(potilas == tohtori.substr(extralen))
                            {
                                needer.insert(potilasnum);
                                break;
                            }
                        }
                    }
                    for(unsigned stringnum=0; stringnum<pagestrings.size(); ++stringnum)
                    {
                        if(needer.find(stringnum) == needer.end())
                            usageperpage[prevpage] += pagestrings[stringnum].size() + 1;
                    }
                    if(i == strings.end())break;
                    pagestrings.clear();
                }
                prevpage = i->first >> 16;                
                pagestrings.push_back(i->second.str);
            }
            
            /* Find substrings from each string */
            unsigned stringcounter=0;
            
            map<string, substringrec> substringtable;
            
            for(stringmap::const_iterator i=strings.begin(); i!=strings.end(); ++i, ++stringcounter)
            {
                if(i->second.type != stringdata::zptr16)
                    continue;
                const unsigned page = i->first >> 16;
                
                /* Weight value for this string is how urgent the compression is */
                double importance = usageperpage[page] / (double)spaceperpage[page];
                const string &s = i->second.str;
                
                importance *= importance;

                if(tickpos)
                    --tickpos;
                else
                {
                    /* Just tell where we are */
                    tickpos = 256;
                    double totalpos = dictcount;
                    totalpos += stringcounter / (double)strings.size();
                    totalpos /= (double)dictsize;
                    if(totalpos==0.0)totalpos=1e-10;
                    //totalpos = pow(totalpos, 0.3);
                    
                    for(unsigned a=fprintf(stderr,
                          " - about %.2f minutes left...", (difftime(time(NULL),begin)*(1.0/totalpos - 1)) / 60.0);
                        a-->0; )putc(8, stderr);
                }
                
                /* Substrings start from each position in the string */
                for(unsigned a=0; a<s.size(); ++a)
                {
                    /* And are each length */
                    unsigned speco=0, spaco=0, nospaco=0;
                    for(unsigned c=0, b=a; b<s.size(); ++b)
                    {
                        if(++c > MaxDictWordLen)break;
                        
                        unsigned char ch = s[b];
                        /* But can't contain all possible bytes      */
                        /* (Dictionary words can't contain [nl] etc) */
                        if(ch <= 0x20)break;
                        if(ch >= 0x21 && ch < 0xA0)
                        {
                            if(speco >= MaxDictRecursLen)break;
                            ++speco;
                        }
                        else if(ch == 0xEF)
                        {
                            if(nospaco > 0
                            && (spaco+1) >= DictMaxSpacesPerWord)break;
                            ++spaco;
                        }
                        else
                        {
                            if(spaco > DictMaxSpacesPerWord)break;
                            ++nospaco;
                        }
                        
                        if(c >= 2)
                        {
                            /* Cumulate the substring usage counter */
                            substringtable[s.substr(a,c)].mark(importance);
                        }
                    }
                }
            }
            
            fprintf(stderr, "\r%8u substrings; ", substringtable.size());
            
            //map<unsigned, substringrec>::const_iterator bestj = substringtable.end();
            map<string, substringrec>::const_iterator j,bestj;
            
            /* Now find the substring that has the biggest score */
            double bestscore=0;
            for(j = substringtable.begin();
                j != substringtable.end();
                ++j)
            {
                const string &word = j->first;
                unsigned oldbytes = j->second.getcount() * word.size();
                unsigned newbytes = j->second.getcount();
                double realscore = (oldbytes - newbytes) * j->second.getimportance();
                if(realscore > bestscore)
                {
                    bestj = j;
                    bestscore= realscore;
                }
            }
            
            /* Add it to dictionary */
            
            /* bestword = the substring to be replaced with one byte */
            const string &bestword = bestj->first;
            
            /* dictword = the string to be displayed when the byte is met */
            string dictword;
            for(unsigned a=0; a<bestword.size(); ++a)
            {
                unsigned char c = bestword[a];
                if(c >= 0x21 && c < 0xA0)
                    dictword += dict[c-0x21];
                else
                    dictword += (char)c;
            }

            printf("$%u:%s; score: %.1f\n", dict.size(), DispString(dictword).c_str(), bestscore);
            fflush(stdout);
            
            /* Remove the selected substring from the source strings,
             * so that it won't be used in any other substrings.
             * It would mess up the statistics otherwise.
             */
            unsigned char replacement = 0x21 + dict.size();

            for(stringmap::iterator i=strings.begin(); i!=strings.end(); ++i)
            {
                if(i->second.type != stringdata::zptr16) continue;
                i->second.str = str_replace(bestword, replacement, i->second.str);
            }

            dictbytes += bestword.size()+1;
            dict.push_back(dictword);
            replacedict.push_back(bestword);
        }
    }
    else
    {
        fprintf(stderr,
            "Applying dictionary. This will take some seconds at most.\n"
        );
        if(sort_dictionary)
            sort(dict.begin(), dict.end(), dictsorter);
        unsigned col=0;
        vector<string> replacements;
        for(unsigned d=0; d<dict.size(); ++d)
        {
            const string &dictword = dict[d];
            
            string bestword = dictword;
            for(unsigned c=0; c<d; ++c)
            //for(unsigned c=d; c-->0; )
            {
                unsigned char replacement = 0x21 + c;
                bestword = str_replace(replacements[c], replacement, bestword);
            }
            
            fprintf(stderr, "'%s%*c",
                DispString(dictword).c_str(),
                dictword.size()-12, '\'');
            if(++col==6){col=0;putc('\n',stderr);}
            
            dictbytes += bestword.size()+1;
            
            unsigned char replacement = 0x21 + d;
            
            for(stringmap::iterator i=strings.begin(); i!=strings.end(); ++i)
            {
                if(i->second.type != stringdata::zptr16) continue;
                i->second.str = str_replace(bestword, replacement, i->second.str);
            }
            replacements.push_back(bestword);
        }
        if(col)putc('\n', stderr);
        replacedict = dict;
    }


    unsigned resultsize=0;
    for(stringmap::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->second.type != stringdata::zptr16)continue;        
        resultsize += i->second.str.size();
    }
    
    fprintf(stderr, "Original script size: %u bytes; new script size: %u bytes\n"
                    "Saved: %u bytes; dictionary size: %u bytes\n",
        origsize, resultsize, origsize-resultsize, dictbytes);
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

    fprintf(stderr, "%u 8pix symbols loaded\n", symbols8.size());
    fprintf(stderr, "%u 16pix symbols loaded\n", symbols16.size());
}

int main(void)
{
    fprintf(stderr,
        "Chrono Trigger script insertor version "VERSION"\n"
        "Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)\n");
    
    insertor ins;
    
    ins.LoadSymbols();
    
    FILE *fp = fopen("ct_eng.txt", "rt");
    ins.LoadFile(fp);
    fclose(fp);
    
    ins.MakeDictionary();
    
    ins.freespace.Report();

    ins.WriteROM();
    
    ins.freespace.Report();
    
    return 0;
}
