#include <cstdio>

using namespace std;

#include "wstring.hh"
#include "ctcset.hh"
#include "miscfun.hh"
#include "ctinsert.hh"

static const bool rebuild_dict             = false;
static const bool sort_dictionary          = false;
static const unsigned MaxDictWordLen       = 20;
static const unsigned MaxDictRecursLen     = 3;
static const unsigned DictMaxSpacesPerWord = 4;

void insertor::GeneratePatches()
{
    ROM ROM(4194304);
    
    /*
    {FILE *fp = fopen("chrono-uncompressed.smc", "rb");
    if(fp){fseek(fp,512,SEEK_SET);fread(&ROM[0], 1, ROM.size(), fp);fclose(fp);}}
    */
    
    fprintf(stderr, "Initializing all free space to zero...\n");
    
    set<unsigned> pages = freespace.GetPageList();
    for(set<unsigned>::const_iterator i = pages.begin(); i != pages.end(); ++i)
    {
    	freespaceset list = freespace.GetList(*i);
    	
    	for(freespaceset::const_iterator j = list.begin(); j != list.end(); ++j)
        {
            unsigned offs = (*i << 16) | j->pos;
            for(unsigned a=0; a < j->len; ++a) ROM.Write(offs+a, 0);
        }
    }
    
    WriteDictionary(ROM);
    WriteStrings(ROM);
    Write8pixfont(ROM);
    Write12pixfont(ROM);
    
    const unsigned MaxHunkSize = 20000;
    
    /* Now write the patches */
    FILE *fp = fopen("ctpatch-nohdr.ips", "wb");
    FILE *fp2 = fopen("ctpatch-hdr.ips", "wb");
    fwrite("PATCH", 1, 5, fp);
    fwrite("PATCH", 1, 5, fp2);
    /* Format:   24bit offset, 16-bit size, then data; repeat */
    for(unsigned a=0; a<ROM.size(); ++a)
    {
        if(!ROM.touched(a))continue;
        
        // Offset is "a" in fp, "a+512" in fp2
        putc((a>>16)&255, fp);
        putc((a>> 8)&255, fp);
        putc((a    )&255, fp);
        putc(((a+512)>>16)&255, fp2);
        putc(((a+512)>> 8)&255, fp2);
        putc(((a+512)    )&255, fp2);
        
        unsigned offs=a, c=0;
        while(a < ROM.size() && ROM.touched(a) && c < MaxHunkSize)
            ++c, ++a;
        
        // Size is "c" in both.
        putc((c>> 8)&255, fp);
        putc((c    )&255, fp);
        putc((c>> 8)&255, fp2);
        putc((c    )&255, fp2);
        fwrite(&ROM[offs], 1, c, fp);
        fwrite(&ROM[offs], 1, c, fp2);
    }
    fwrite("EOF",   1, 3, fp);
    fwrite("EOF",   1, 3, fp2);
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

// Used if rebuild_dict
class substringrec
{
    unsigned count;
    double importance;
public:
    substringrec() : count(0), importance(0) {}
    void mark(double imp) { ++count; importance += 1.0/imp; }
    unsigned getcount() const { return count; }
    double getimportance() const { return 1.0/(importance / count);}
};

/* Build the dictionary. */
void insertor::MakeDictionary()
{
    unsigned origsize=0, dictbytes=0;
    for(stringmap::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->second.type != stringdata::zptr16)continue;        
        origsize += i->second.str.size();
    }
    
    if(rebuild_dict)
    {
        map<unsigned, unsigned> spaceperpage;
        
        set<unsigned> pages = freespace.GetPageList();
        for(set<unsigned>::const_iterator i = pages.begin(); i != pages.end(); ++i)
        	spaceperpage[*i] = freespace.Size(*i);

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

int main(void)
{
    fprintf(stderr,
        "Chrono Trigger script insertor version "VERSION"\n"
        "Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)\n");
    
    insertor ins;
    
    FILE *fp = fopen("ct_eng.txt", "rt");
    ins.LoadFile(fp);
    fclose(fp);
    
    ins.MakeDictionary();
    
    ins.freespace.Report();

    ins.GeneratePatches();
    
    ins.freespace.Report();
    
    return 0;
}
