#include "tgaimage.hh"
#include "ctinsert.hh"

namespace
{
    const char font8fn[]  = "ct8fnFI.tga";
    const char font12fn[] = "ct16fnFI.tga";

    const unsigned Font8_Address  = 0x3F8C60;
    const unsigned Font12_Address = 0x3F2F60;
    const unsigned WidthTab_Address = 0x260E6;
}

void insertor::Write8pixfont(ROM &ROM)
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

    fprintf(stderr, "Writing 8-pix font...\n");

    for(unsigned a=0; a<tiletable.size(); ++a)
        ROM.Write(Font8_Address + a, tiletable[a]);
}

void insertor::Write12pixfont(ROM &ROM)
{
    TGAimage font12(font12fn);
    font12.setboxsize(12, 12);
    font12.setboxperline(32);
    
    static const char palette[] = {0,0,1,2,3,0};
    
    vector<unsigned char> tiletable(96 * 24 + 96 * 12);
    
    fprintf(stderr, "Writing 12-pix font...\n");

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
        
        ROM.Write(WidthTab_Address+a, width);
    }
    for(unsigned a=0; a<tiletable.size(); ++a)
        ROM.Write(Font12_Address+a, tiletable[a]);
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

    struct dictitem
    {
        string item;
        unsigned ind;
        bool operator< (const dictitem &b) const { return item.size() > b.item.size(); }
    };
}

void insertor::WriteDictionary(ROM &ROM)
{
    unsigned dictpage = dictaddr >> 16;
    
    freespace.DumpPageMap(dictpage);
    unsigned size = 0;
    for(unsigned a=0; a<dictsize; ++a)size += dict[a].size() + 1;
    fprintf(stderr, "Writing dictionary (%u bytes)...\n", size);
    
    vector<dictitem> dictionary(dictsize);
    for(unsigned a=0; a<dictsize; ++a)
    {
        dictionary[a].item = dict[a];
        dictionary[a].ind = a;
    }
    sort(dictionary.begin(), dictionary.end());
    
    for(unsigned a=0; a<dictionary.size(); ++a)
    {
        unsigned ind      = dictionary[a].ind;
        const string &str = dictionary[a].item;

        unsigned spaceptr = freespace.Find(dictpage, str.size() + 1);
        WritePPtr(ROM, dictaddr + ind*2, str, spaceptr);
    }
    freespace.DumpPageMap(dictaddr >> 16);
}

namespace
{
	struct stringoffsdata
	{
		string   str;
		unsigned offs;
	};
	
    struct todo_sort
    {
        const vector<stringoffsdata> &tab;
        todo_sort(const vector<stringoffsdata> &t) : tab(t) { }
        bool operator() (unsigned a, unsigned b) const
        {
            return tab[a].str.size() > tab[b].str.size();
        }
    };

    void WritePageZ
        (ROM &ROM,
         unsigned page,
         vector<stringoffsdata> &pagestrings,
         insertor &ins
        )
    {
    	if(!pagestrings.size()) return;
    	
        // parasite -> host
        typedef map<unsigned, unsigned> neederlist_t;
        neederlist_t neederlist;
        
        for(unsigned parasitenum=0;
                     parasitenum < pagestrings.size();
                     ++parasitenum)
        {
            const string &parasite = pagestrings[parasitenum].str;
            for(unsigned hostnum = 0;
                         hostnum < pagestrings.size();
                         ++hostnum)
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
                
                const string &host = pagestrings[hostnum].str;
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
            needertmp[pagestrings[j->first].str].push_back(j->first);
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
                DispString(pagestrings[hostnum].str).c_str());
        }
    #endif
        
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

            fprintf(stderr, "Page %02X: Writing %u strings and reusing %u\n",
                page,
                todo.size(), reusenum);

            sort(todo.begin(), todo.end(), todo_sort(pagestrings));
            
            for(unsigned a=0; a<todo.size(); ++a)
            {
                unsigned stringnum = todo[a];
                unsigned ptroffs = pagestrings[stringnum].offs;
                const string &str = pagestrings[stringnum].str;

                unsigned spaceptr = ins.freespace.Find(page, str.size() + 1);
                if(spaceptr == NOWHERE)
                {
                	fprintf(stderr, "Not written %06X: %s\n",
                		ptroffs, ins.DispString(str).c_str());
                }
                pagestrings[stringnum].offs = WriteZPtr(ROM, ptroffs, str, spaceptr);
            }
        }
        if(true) /* Then do parasites */
        {
            for(unsigned stringnum=0; stringnum<pagestrings.size(); ++stringnum)
            {
                neederlist_t::const_iterator i = neederlist.find(stringnum);
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
                        fprintf(stderr, "String %u not written!\n", parasitenum);
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

void insertor::WriteStrings(ROM &ROM)
{
    vector<stringoffsdata> pagestrings;
    unsigned prevpage = 0xFFFF;
    
    for(stringmap::const_iterator i=strings.begin(); ; ++i)
    {
        if(i == strings.end())
        {
        	WritePageZ(ROM, prevpage, pagestrings, *this);
        	break;
        }
        
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
            case stringdata::zptr16:
            {
                unsigned page = i->first >> 16;
                if(page != prevpage)
                {
                	WritePageZ(ROM, prevpage, pagestrings, *this);

                    pagestrings.clear();
                }
                prevpage = page;
                
                stringoffsdata tmp;
                tmp.str  = i->second.str;
                tmp.offs = i->first;
                pagestrings.push_back(tmp);
                
                break;
            }
        }
    }
}
