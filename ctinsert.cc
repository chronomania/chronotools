#include <cstdio>

using namespace std;

#include "ctinsert.hh"
#include "ctcset.hh"
#include "rom.hh"

namespace
{
    const char font8fn[]                = "ct8fn.tga";
    const char font12fn[]               = "ct16fn.tga";
    const char scriptfn[]               = "ct.txt";
    
    const char patchfile_hdr[]          = "ctpatch-hdr.ips";
    const char patchfile_nohdr[]        = "ctpatch-nohdr.ips";

    void GeneratePatches(ROM &ROM)
    {
        /*
        {FILE *fp = fopen("chrono-uncompressed.smc", "rb");
        if(fp){fseek(fp,512,SEEK_SET);fread(&ROM[0], 1, ROM.size(), fp);fclose(fp);}}
        */
        
        const unsigned MaxHunkSize = 20000;
        
        /* Now write the patches */
        FILE *fp = fopen(patchfile_nohdr, "wb");
        FILE *fp2 = fopen(patchfile_hdr, "wb");
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
        fclose(fp); fprintf(stderr, "Created %s\n", patchfile_nohdr);
        fclose(fp2); fprintf(stderr, "Created %s\n", patchfile_hdr);
    }
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
            case 0xEE: result += conv.puts(AscToWstr("[musicsymbol]")); break;
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

void stringoffsmap::GenerateNeederList()
{
    neederlist.clear();
    
    for(unsigned parasitenum=0; parasitenum < size(); ++parasitenum)
    {
        const string &parasite = (*this)[parasitenum].str;
        for(unsigned hostnum = 0; hostnum < size(); ++hostnum)
        {
            // Can't depend on self
            if(hostnum == parasitenum) { Continue: continue; }
            // If the host depends on this "parasite", skip it
            neederlist_t::iterator i;
            for(unsigned tmp=hostnum;;)
            {
                i = neederlist.find(tmp);
                if(i == neederlist.end()) break;
                tmp = i->second;
                // Host depends on "parasite", skip this
                if(tmp == parasitenum) { goto Continue; }
            }
            
            const string &host = (*this)[hostnum].str;
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
    
    /* Nyt mapissa on listattu, kuka tarvitsee ketäkin. */

#if 0
    typedef map<string, vector<unsigned> > needertmp_t;
    needertmp_t needertmp;
    for(neederlist_t::const_iterator j = neederlist.begin(); j != neederlist.end(); ++j)
    {
        needertmp[(*this)[j->first].str].push_back(j->first);
    }
    for(needertmp_t::const_iterator j = needertmp.begin(); j != needertmp.end(); ++j)
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
            DispString((*this)[hostnum].str).c_str());
    }
#endif
}

const set<unsigned> insertor::GetZStringPageList() const
{
    set<unsigned> result;
    for(stringmap::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        unsigned page = i->first >> 16;
        switch(i->second.type)
        {
            case stringdata::fixed:
                // This is not a pointer
                continue;
            case stringdata::zptr8:
            case stringdata::zptr12:
                // These are ok
                break;
            // If we omitted something, compiler should warn
        }
        result.insert(page);
    }
    return result;
}

const stringoffsmap insertor::GetZStringList(unsigned pagenum) const
{
    stringoffsmap result;
    for(stringmap::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        unsigned page = i->first >> 16;
        if(page != pagenum) continue;
        switch(i->second.type)
        {
            case stringdata::fixed:
                // This is not a pointer
                continue;
            case stringdata::zptr8:
            case stringdata::zptr12:
                // These are ok
                break;
            // If we omitted something, compiler should warn
        }
        stringoffsdata tmp;
        tmp.str  = i->second.str;
        tmp.offs = i->first;
        result.push_back(tmp);
    }
    return result;
}


int main(void)
{
    fprintf(stderr,
        "Chrono Trigger script insertor version "VERSION"\n"
        "Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)\n");
    
    insertor *ins = new insertor;
    
    // Font loading must happen before script loading,
    // or script won't be properly paragraph-wrapped.
    ins->LoadFont8(font8fn);
    ins->LoadFont12(font12fn);
    
    FILE *fp = fopen(scriptfn, "rt");
    ins->LoadFile(fp);
    fclose(fp);
    
    ins->DictionaryCompress();
    
    ins->GenerateCode();
    
    ins->freespace.Report();

    fprintf(stderr, "Creating a virtual ROM...\n");
    ROM ROM(4194304);
    
    ins->PatchROM(ROM);

    ins->freespace.Report();
    
    fprintf(stderr, "Unallocating insertor data...\n");
    delete ins; ins = NULL;
    
    GeneratePatches(ROM);
    
    return 0;
}
