#include "ctinsert.hh"
#include "rom.hh"

namespace
{
    const unsigned Font8_Address  = 0x3F8C60;
    const unsigned Font12_Address = 0x3F2F60;
    const unsigned WidthTab_Address = 0x260E6;
}

namespace
{
    unsigned WritePPtr
    (
        ROM &ROM,
        unsigned pointeraddr,
        const string &string,
        unsigned spaceptr
    )
    {
        if(spaceptr == NOWHERE) return spaceptr;

        unsigned page = pointeraddr >> 16;
        
        ROM.Write(pointeraddr,   spaceptr&255);
        ROM.Write(pointeraddr+1, spaceptr>>8);
        
        /*
        fprintf(stderr, "Wrote %u bytes at %06X->%04X\n",
            string.size()+1, pointeraddr, spaceptr);
        */
        
        spaceptr += page<<16;
        ROM.Write(spaceptr, string.size());
        for(unsigned a=0; a<string.size(); ++a)
            ROM.Write(spaceptr+a+1, string[a]);
        
        return spaceptr & 0xFFFF;
    }

    unsigned WriteZPtr
    (
        ROM &ROM,
        unsigned pointeraddr,
        const string &string,
        unsigned spaceptr
    )
    {
        if(spaceptr == NOWHERE) return spaceptr;

        unsigned page = pointeraddr >> 16;
        
        ROM.Write(pointeraddr,   spaceptr&255);
        ROM.Write(pointeraddr+1, spaceptr>>8);
        
        /*
        fprintf(stderr, "Wrote %u bytes at %06X->%04X: ",
            string.size()+1, pointeraddr, spaceptr);
        fprintf(stderr, DispString(string).c_str());
        fprintf(stderr, "\n");
        */
        
        spaceptr += page<<16;
        for(unsigned a=0; a<string.size(); ++a)
            ROM.Write(spaceptr+a, string[a]);
        ROM.Write(spaceptr+string.size(), 0);
        return spaceptr & 0xFFFF;
    }

    void WritePageZ
        (ROM &ROM,
         unsigned page,
         stringoffsmap &pagestrings,
         freespacemap &freespace
        )
    {
        pagestrings.GenerateNeederList();

        const stringoffsmap::neederlist_t &neederlist = pagestrings.neederlist;
        
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

            unsigned todobytes=0;
            for(unsigned a=0; a<todo.size(); ++a)
                todobytes += pagestrings[todo[a]].str.size() + 1;

            fprintf(stderr, "Page %02X: Writing %u strings (%u bytes) and reusing %u\n",
                page, todo.size(), todobytes, reusenum);

            vector<freespacerec> Organization(todo.size());
            for(unsigned a=0; a<todo.size(); ++a)
                Organization[a].len = pagestrings[todo[a]].str.size() + 1;
            
            freespace.Organize(Organization, page);
            
            unsigned unwritten = 0;
            for(unsigned a=0; a<todo.size(); ++a)
            {
                unsigned stringnum = todo[a];
                unsigned ptroffs = pagestrings[stringnum].offs;
                const string &str = pagestrings[stringnum].str;

                unsigned spaceptr = Organization[a].pos;
                if(spaceptr == NOWHERE) ++unwritten;
                pagestrings[stringnum].offs = WriteZPtr(ROM, ptroffs, str, spaceptr);
            }
            if(unwritten)
                fprintf(stderr, "%u string%s unwritten\n", unwritten, unwritten==1?"":"s");
        }
        if(true) /* Then do parasites */
        {
            for(unsigned stringnum=0; stringnum<pagestrings.size(); ++stringnum)
            {
                stringoffsmap::neederlist_t::const_iterator i = neederlist.find(stringnum);
                if(i == neederlist.end()) continue;
                
                unsigned parasitenum = i->first;
                unsigned hostnum     = i->second;

                unsigned hostoffs = pagestrings[hostnum].offs;
                        
                unsigned ptroffs = pagestrings[parasitenum].offs;
                const string &str = pagestrings[parasitenum].str;
                
                if(hostoffs == NOWHERE)
                {
                    fprintf(stderr, "Host %u doesn't exist! ", hostnum);
                    
                    // Try find another host
                    for(hostnum=0; hostnum<pagestrings.size(); ++hostnum)
                    {
                        // Skip parasites
                        if(neederlist.find(hostnum) != neederlist.end()) continue;
                        // Skip unwritten ones
                        if(pagestrings[hostnum].offs == NOWHERE) continue;
                        const string &host = pagestrings[hostnum].str;
                        // Impossible host?
                        if(host.size() < str.size()) continue;
                        unsigned extralen = host.size() - str.size();
                        if(str != host.substr(extralen)) continue;
                        hostoffs = pagestrings[hostnum].offs;
                        break;
                    }
                    if(hostoffs == NOWHERE)
                    {
                        fprintf(stderr, "String %u wasn't written.\n", parasitenum);
                        continue;
                    }
                    fprintf(stderr, "Substitute %u assigned for %u.\n",
                        hostnum, parasitenum);
                }

                const string &host = pagestrings[hostnum].str;
                
                unsigned place = hostoffs + host.size()-str.size();
                
                ROM.Write(ptroffs,   place&255);
                ROM.Write(ptroffs+1, place>>8 );
            }
        }
    }
}

void insertor::PatchROM(ROM &ROM)
{
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
    WriteCode(ROM);
}

namespace
{
    template<typename T>
    void AddCode(vector<T> &z,int a) { z.push_back(a); }
    template<typename T>
    void AddCode(vector<T> &z,int a,int b) { z.reserve(z.size()+2); AddCode(z,a);
                                                                    AddCode(z,b); }
    template<typename T>
    void AddCode(vector<T> &z,int a,int b,int c) { z.reserve(z.size()+3);
                                                   AddCode(z,a);AddCode(z,b);
                                                   AddCode(z,c); }
    template<typename T>
    void AddCode(vector<T> &z,int a,int b,int c,int d) { z.reserve(z.size()+4);
                                                         AddCode(z,a);AddCode(z,b);
                                                         AddCode(z,c);AddCode(z,d); }
    
    class FarCallCode
    {
        /*
            Use this routine to call near-routines from your routines.
            Usage:
               ... your code here ...
               FarCallCode caller(your_code);
               ... last things before call to the routine ...
               caller.Proceed(function_address);
               ... rest of your code ...
               caller.Finish(address_of_your_code)
            Note: Call to the first function OVERWRITES the following:
                  - Register A (you want to reset it before proceeding)
                  - Stack (don't pop anything before "proceed"!)
                  - M-flag (do rep/sep $20 before proceeding!)
        */
        unsigned ret_addr, rtl_addr, targetpos, segment;
        vector<unsigned char> &code;
        
        static unsigned GetRTLaddr(unsigned segment)
        {
            switch(segment)
            {
                // Add list of RTL offsets in different segmets here.
                case 0xC2: return 0x5841;
                
                default:
                    fprintf(stderr, "Error: RTL location in segment 0x%02X unknown\n",
                        segment);
                    return 0;
            }
        }
        void Patch(unsigned offs, unsigned value)
        {
            code[offs + 0] = (value     ) & 255;
            code[offs + 1] = (value >> 8) & 255;
        }
    public:
        FarCallCode(vector<unsigned char> &codeaddr) : code(codeaddr)
        {
            AddCode(code, 0x4B);       //phk (push pb)
            AddCode(code, 0xC2, 0x20); //rep $20 - we need 16-bit immeds here
            AddCode(code, 0xA9, 0,0);  //lda immed16
            ret_addr = code.size() - 2;
            AddCode(code, 0x48);       //pha

            AddCode(code, 0xA9, 0,0);  //lda immed16
            rtl_addr = code.size() - 2;
            AddCode(code, 0x48);       //pha
        }
        void Proceed(unsigned target)
        {
            segment = (target>>16) & 255;
            AddCode(code, 0x5C, target&255, (target>>8)&255, segment);
            targetpos = code.size();
        }
        void Finish(unsigned address)
        {
            Patch(rtl_addr, GetRTLaddr(segment) - 1);
            Patch(ret_addr, targetpos + address - 1);
        }
    };
};

void insertor::WriteCode(ROM &ROM)
{
    fprintf(stderr, "Writing code...\n");
    
    vector<unsigned char> code;
    
    // C2:5DC4 is referenced from:
    //      C2:5C12 
    //      C2:58C4
    
    // save A
    AddCode(code, 0x48);       //pha

#if 0
    // modify the character
    AddCode(code, 0xE2, 0x20); //sep $20
    AddCode(code, 0xA9, 0xA8); //lda a, $A8
    AddCode(code, 0x85, 0x35); //sta [$00:D+$35]
    
    // prepare the framework for a farcall to a near routine
    FarCallCode farcall(code); //overwrites a and pushes stuff

    // call back the routine
    AddCode(code, 0xC2, 0x20); //rep $20
    AddCode(code, 0xA5, 0x35); //lda [$00:D+$35]
    
    farcall.Proceed(0xC25DC8);
#endif 
    // restore A
    AddCode(code, 0x68);       //pla
    // restore character
    AddCode(code, 0xE2, 0x20); //sep $20
    AddCode(code, 0x85, 0x35); //sta [$00:D+$35]
    // be ready to return to the routine
    AddCode(code, 0xC2, 0x20); //rep $20
    AddCode(code, 0xA5, 0x35); //lda [$00:D+$35]
    AddCode(code, 0x6B);       //rtl
    
    unsigned address  = freespace.FindFromAnyPage(code.size());
#if 0    
    farcall.Finish(address);
#endif
    
    ROM.AddSubRoutine(address, code);
    ROM.AddCall(0xC25DC4, address);
}


void insertor::Write8pixfont(ROM &ROM)
{
    fprintf(stderr, "Writing 8-pix font...\n");

    const vector<unsigned char> &tiletab = Font8.GetTiles();
    
    for(unsigned a=0; a<tiletab.size(); ++a)
        ROM.Write(Font8_Address + a, tiletab[a]);
}

void insertor::Write12pixfont(ROM &ROM)
{
    fprintf(stderr, "Writing 12-pix font...\n");

    for(unsigned a=0; a<Font12.GetCharCount(); ++a)
        ROM.Write(WidthTab_Address+a, Font12.GetWidth(a));
    
    const vector<unsigned char> &tiletab = Font12.GetTiles();
    
    for(unsigned a=0; a<tiletab.size(); ++a)
        ROM.Write(Font12_Address+a, tiletab[a]);
}

void insertor::WriteDictionary(ROM &ROM)
{
    unsigned dictpage = dictaddr >> 16;
    
    unsigned size = 0;
    for(unsigned a=0; a<dictsize; ++a)size += dict[a].size() + 1;
    fprintf(stderr, "Writing dictionary (page=%02X, %u bytes)...\n", dictpage, size);
    
    vector<freespacerec> Organization(dictsize);
    for(unsigned a=0; a<dictsize; ++a)
        Organization[a].len = dict[a].size() + 1;
    
    freespace.Organize(Organization, dictpage);
    for(unsigned a=0; a<dictsize; ++a)
    {
        unsigned spaceptr = Organization[a].pos;
        WritePPtr(ROM, dictaddr + a*2, dict[a], spaceptr);
    }
}

void insertor::WriteStrings(ROM &ROM)
{
    for(stringmap::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        switch(i->second.type)
        {
            case stringdata::fixed:
            {
                unsigned pos = i->first;
                const string &s = i->second.str;
                
                if(s.size() != i->second.width)
                    fprintf(stderr, "Warning: Fixed string at %06X: len(%u) != space(%u)...\n",
                    pos, s.size(), i->second.width);

                unsigned a;
                for(a=0; a<s.size(); ++a)      ROM.Write(pos++, s[a]);
                for(; a < i->second.width; ++a)ROM.Write(pos++, 0);
                break;
            }
            case stringdata::zptr8:
            case stringdata::zptr12:
            {
                // These are not interesting here, as they're handled below
                break;
            }
            // If we omitted something, compiler should warn
       }
   }
   
   set<unsigned> zpages = GetZStringPageList();
   for(set<unsigned>::const_iterator i=zpages.begin(); i!=zpages.end(); ++i)
   {
       stringoffsmap pagestrings = GetZStringList(*i);
       WritePageZ(ROM, *i, pagestrings, freespace);
    }
}
