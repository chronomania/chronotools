#include <string>

#include "ctinsert.hh"
#include "rom.hh"
#include "ctcset.hh"
#include "settings.hh"
#include "config.hh"
#include "logfiles.hh"
#include "stringoffs.hh"
#include "rangeset.hh"

namespace
{
    class StringReceipt
    {
        vector<unsigned char> data;
        rangeset<unsigned> ptrbytes, databytes;
        list<unsigned> pointers;
        
        typedef multimap<unsigned, pair<ReferMethod, string> > refermap;
        refermap referers;
        
    public:
        StringReceipt(): data(65536) { }
        void WritePtr(unsigned short addr, unsigned short value)
        {
            data[addr]   = value&255;
            data[addr+1] = value>>8;
            ptrbytes.set(addr, addr+2);
            pointers.push_back(addr);
        }
        void WriteData(unsigned short spaceaddr, const vector<unsigned char>& buf)
        {
            for(unsigned a=0; a<buf.size(); ++a)
                data[spaceaddr+a] = buf[a];
            databytes.set(spaceaddr, spaceaddr+buf.size());
        }
        void WriteDataPtr(unsigned short ptraddr, unsigned short spaceaddr,
                          const vector<unsigned char>& buf)
        {
/*
            fprintf(stderr, "Writing %u bytes to %04X (ptr at %04X)\n",
                            buf.size(), spaceaddr, ptraddr);
*/
            WritePtr(ptraddr, spaceaddr);
            WriteData(spaceaddr, buf);
        }
        void WriteZPtr(unsigned short ptraddr, unsigned short spaceaddr, const ctstring& s)
        {
            const string StringBuf = GetString(s);
            vector<unsigned char> DataBuf(StringBuf.size() + 1);
            for(unsigned a=0; a<StringBuf.size(); ++a) DataBuf[a] = StringBuf[a];
            DataBuf[StringBuf.size()] = 0;
            WriteDataPtr(ptraddr, spaceaddr, DataBuf);
        }
        void WritePPtr(unsigned short ptraddr, unsigned short spaceaddr, const ctstring& s)
        {
            const string StringBuf = GetString(s);
            vector<unsigned char> DataBuf(StringBuf.size() + 1);
            for(unsigned a=0; a<StringBuf.size(); ++a) DataBuf[a+1] = StringBuf[a];
            DataBuf[0] = StringBuf.size();
            WriteDataPtr(ptraddr, spaceaddr, DataBuf);
        }
        void WriteLString(unsigned short spaceaddr, const ctstring& s, unsigned width)
        {
            vector<unsigned char> Buf(width, 255);
            
            // Fixed strings shouldn't contain extrachars.
            // Thus s.size() can be safely used.
            unsigned size = s.size();
            if(size > width) size = width;
            
            std::copy(s.begin(), s.begin()+size, Buf.begin());
            
            WriteData(spaceaddr, Buf);
        }
        void Apply(insertor& ins,
                   const string& what, unsigned strpage)
        {
            Apply(ins, what + " table", ptrbytes, strpage);
            Apply(ins, what + " data", databytes, strpage);
            
            if(!referers.empty())
            {
                fprintf(stderr, "Script Writer: %u leftover referers\n", referers.size());
            }
        }
        void AddReference(const ReferMethod& reference, unsigned short target, const string& what="")
        {
            referers.insert(make_pair(target, make_pair(reference, what)));
        }
    private:
        void Apply(insertor& ins, const string& what,
                   rangeset<unsigned>& ranges, unsigned page)
        {
            ranges.compact();
            list<rangeset<unsigned>::const_iterator> rangelist;
            ranges.find_all_coinciding(0,0x10000, rangelist);
            for(list<rangeset<unsigned>::const_iterator>::const_iterator
                j = rangelist.begin();
                j != rangelist.end();
                ++j)
            {
                unsigned begin  = (*j)->lower;
                unsigned end    = (*j)->upper;
                unsigned absptr = begin | (page << 16);
                
                vector<unsigned char> Buf(data.begin() + begin, data.begin() + end);
                
                O65 tmp;
                tmp.LoadCodeFrom(Buf);
                tmp.LocateCode(absptr);
                
                O65linker::LinkageWish wish;
                wish.SetAddress(absptr);
                
                list<pair<string, ReferMethod> > refs;
                for(refermap::iterator j,i = referers.begin(); i != referers.end(); i=j)
                {
                    j = i; ++j;
                    if(i->first < begin || i->first >= end) continue;
                    
                    string refname = what + ":" + i->second.second;
                    
                    tmp.DeclareCodeGlobal(refname, absptr + (i->first - begin));
                    refs.push_back(make_pair(refname, i->second.first));
                    referers.erase(i);
                }
                
                ins.objects.AddObject(tmp, what, wish);
                for(list<pair<string, ReferMethod> >::const_iterator
                    i = refs.begin(); i != refs.end(); ++i)
                {
                    ins.objects.AddReference(i->first, i->second);
                }
            }
        }
    };

    void insertor::WritePageZ(unsigned page, stringoffsmap& pagestrings)
    {
        FILE *log = GetLogFile("mem", "log_addrs");

        pagestrings.GenerateNeederList();
        
        const stringoffsmap::neederlist_t &neederlist = pagestrings.neederlist;
        
        StringReceipt receipt;
        
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
                todobytes += CalcSize(pagestrings[todo[a]].str) + 1;

            vector<freespacerec> Organization(todo.size());
            for(unsigned a=0; a<todo.size(); ++a)
                Organization[a].len = CalcSize(pagestrings[todo[a]].str) + 1;
            
            freespace.Organize(Organization, page);
            
            unsigned unwritten = 0;
            for(unsigned a=0; a<todo.size(); ++a)
            {
                unsigned stringnum  = todo[a];
                unsigned ptroffs    = pagestrings[stringnum].offs;
                const ctstring &str = pagestrings[stringnum].str;

                unsigned spaceptr = Organization[a].pos;
                pagestrings[stringnum].offs = spaceptr;
                
                if(spaceptr == NOWHERE)
                    ++unwritten;
                else
                {
                     receipt.WriteZPtr(ptroffs, spaceptr, str);
                }
            }
            if(unwritten && log)
                fprintf(log, "  %u string%s unwritten\n", unwritten, unwritten==1?"":"s");
        }
        
        if(true) /* Then do parasites */
        {
            for(unsigned stringnum=0; stringnum<pagestrings.size(); ++stringnum)
            {
                stringoffsmap::neederlist_t::const_iterator i = neederlist.find(stringnum);
                if(i == neederlist.end()) continue;
                
                unsigned parasitenum = i->first;
                unsigned hostnum     = i->second;

                unsigned hostoffs   = pagestrings[hostnum].offs;
                        
                unsigned ptroffs    = pagestrings[parasitenum].offs;
                const ctstring &str = pagestrings[parasitenum].str;
                
                if(hostoffs == NOWHERE)
                {
                    if(log)
                        fprintf(log, "  Host %u doesn't exist! ", hostnum);
                    
                    // Try find another host
                    for(hostnum=0; hostnum<pagestrings.size(); ++hostnum)
                    {
                        // Skip parasites
                        if(neederlist.find(hostnum) != neederlist.end()) continue;
                        // Skip unwritten ones
                        if(pagestrings[hostnum].offs == NOWHERE) continue;
                        const ctstring &host = pagestrings[hostnum].str;
                        // Impossible host?
                        if(host.size() < str.size()) continue;
                        unsigned extralen = host.size() - str.size();
                        if(str != host.substr(extralen)) continue;
                        hostoffs = pagestrings[hostnum].offs;
                        break;
                    }
                    if(hostoffs == NOWHERE)
                    {
                        if(log)
                            fprintf(log, "  String %u wasn't written.\n", parasitenum);
                        continue;
                    }
                    if(log)
                        fprintf(log, "  Substitute %u assigned for %u.\n",
                            hostnum, parasitenum);
                }

                const ctstring &host = pagestrings[hostnum].str;
                
                unsigned place = hostoffs + CalcSize(host) - CalcSize(str);
                
                receipt.WritePtr(ptroffs, place);
            }
        }
        receipt.Apply(*this, "strings", page);
    }
}

void insertor::PatchROM(ROM &ROM) const
{
    const bool ClearSpace = GetConf("patch", "clear_free_space");
    
    if(ClearSpace)
    {
        fprintf(stderr, "Initializing all free space to zero...\n");

        set<unsigned> pages = freespace.GetPageList();
        for(set<unsigned>::const_iterator i = pages.begin(); i != pages.end(); ++i)
        {
            freespaceset list = freespace.GetList(*i);
            
            for(freespaceset::const_iterator j = list.begin(); j != list.end(); ++j)
            {
                const unsigned recpos = j->lower;
                const unsigned reclen = j->upper - recpos;
                unsigned offs = (*i << 16) | recpos;
                for(unsigned a=0; a < reclen; ++a) ROM.Write(offs+a, 0);
            }
        }
    }
    
    fprintf(stderr, "Inhabitating the ROM image...\n");
    
    /* Then write everything. */

    const vector<unsigned> o65addrs = objects.GetAddrList();
    for(unsigned a=0; a<o65addrs.size(); ++a)
    {
        ROM.AddPatch(objects.GetCode(a),
                     o65addrs[a] & 0x3FFFFF,
                     objects.GetName(a));
        //objects.Release(a);
    }
    
    freespace.VerboseDump();
}

void insertor::GenerateCode()
{
    // Write the strings first because they require certain pages!
    WriteStrings();
    
    // Then write items, techs, monsters.
    WriteRelocatedStrings();

    // Do this first, because it requires two blocks on same page.
    GenerateVWF12code();
    
    GenerateConjugatorCode();
    
    LoadAllUserCode();
    
    objects.AddLump(Font8.GetTiles(), GetConst(TILETAB_8_ADDRESS), "8x8 tiles");
    
    // The dictionary fits almost anywhere.
    WriteDictionary();

    // Do this after all code has been generated.
    LinkAndLocateCode();
    
    const bool UseThinNumbers = GetConf("font", "use_thin_numbers");
    
    // Patch the name entry function
    //  FIXME: Make this read the pointers from "strings" instead
    
#if 0
    // - disabled. Now requires the entry HXVk instead of HXBj and $HXBl.
    // $HXVk
    if(ROM.touched(0x3FC4D3))
    {
        // Write the address of name input strings.
        ROM.Write(0x02E553, ROM[0x3FC4D3]);
        ROM.Write(0x02E554, ROM[0x3FC4D4]);
        ROM.Write(0x02E555, 0xFF);
    }
    if(ROM.touched(0x3FC4D5))
    {
        ROM.Write(0x02E331, ROM[0x3FC4D5]);
        ROM.Write(0x02E332, ROM[0x3FC4D6]);
        ROM.Write(0x02E333, 0xFF);
    }
#endif

    // Testataan moottoria
    // ROM.Write(0x0058DE, 0xA9); //lda A, $08
    // ROM.Write(0x0058DF, 0x07);
    // ROM.Write(0x0058E0, 0xEA); //nop
    // ROM.Write(0x0058E1, 0xEA);

    if(UseThinNumbers)
    {
        PlaceByte(0x73, 0x02F21B, "thin '0'");
    }
    else
    {
        //PlaceByte(0xD4, 0x02F21B, "thick '0'");
    }

    objects.SortByAddress();
}

void insertor::LinkAndLocateCode()
{
    /* Organize the code blobs */    
    freespace.OrganizeO65linker(objects);
    
    /* Link the code blobs. */
    objects.Link();
}

void insertor::WriteDictionary()
{
    unsigned size = 0;
    unsigned dictsize = dict.size();
    for(unsigned a=0; a<dictsize; ++a)size += CalcSize(dict[a]) + 1;

    vector<freespacerec> Organization(dictsize + 1);
    for(unsigned a=0; a<dictsize; ++a)
        Organization[a].len = CalcSize(dict[a]) + 1;
    
    // Allocate a record for the table itself
    Organization[dictsize] = dictsize*2;

    unsigned dictpage = NOWHERE;
    freespace.OrganizeToAnySamePage(Organization, dictpage);
    unsigned dictaddr = Organization[dictsize].pos | (dictpage << 16) | 0xC00000;

    /* Display the dictionary data addresses */
    if(true)
    {
        rangeset<unsigned> ranges;
        for(unsigned a=0; a<dictsize; ++a)
        {
            unsigned spaceptr = Organization[a].pos;
            ranges.set(spaceptr, spaceptr + CalcSize(dict[a]) + 1);
        }
        
        ranges.compact();
        
        list<rangeset<unsigned>::const_iterator> rangelist;
        ranges.find_all_coinciding(0,0x10000, rangelist);
        
        fprintf(stderr,
            "> Dictionary: %u(table)@ $%06X",
            dictsize*2, dictaddr | 0xC00000);
        
        for(list<rangeset<unsigned>::const_iterator>::const_iterator
            j = rangelist.begin();
            j != rangelist.end();
            ++j)
        {
            unsigned size = (*j)->upper - (*j)->lower;
            unsigned ptr  = (*j)->lower | (dictpage << 16) | 0xC00000;

            fprintf(stderr, ", %u(data)@ $%06X", size, ptr);
        }
        fprintf(stderr, "\n");
    }
    
    StringReceipt receipt;
    
    for(unsigned a=0; a<dictsize; ++a)
        receipt.WritePPtr(dictaddr + a*2, Organization[a].pos, dict[a]);
    
    receipt.AddReference(OffsPtrFrom(GetConst(DICT_OFFSET)),   dictaddr, "dict ptr");
    receipt.AddReference(PagePtrFrom(GetConst(DICT_SEGMENT1)), dictaddr, "dict ptr");
    receipt.AddReference(PagePtrFrom(GetConst(DICT_SEGMENT2)), dictaddr, "dict ptr");

    receipt.Apply(*this, "dict", dictpage);
}

void insertor::WriteStrings()
{
    fprintf(stderr, "Writing fixed-length strings...\n");
    map<unsigned, StringReceipt> fixedstrings;
    
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        switch(i->type)
        {
            case stringdata::fixed:
            {
                unsigned pos = i->address;
                const ctstring &s = i->str;
                
                unsigned size = CalcSize(s);
                
                if(size > i->width)
                {
#if 0
                    fprintf(stderr, "  Warning: Fixed string at %06X: len(%u) > space(%u)... '%s'\n",
                        pos, size, i->width, DispString(s).c_str());
#endif
                    size = i->width;
                }
                
                // Filler must be 255, or otherwise following problems occur:
                //     item listing goes zigzag
                //     12pix item/tech/mons text in battle has garbage (char 0 in font).

                unsigned page = (pos >> 16) & 0x3F;
                pos &= 0xFFFF;
                
                fixedstrings[page].WriteLString(pos, s, i->width);
                break;
            }
            case stringdata::item:
            case stringdata::tech:
            case stringdata::monster:
            case stringdata::zptr8:
            case stringdata::zptr12:
            {
                // These are not interesting here, as they're handled below
                break;
            }
            // If we omitted something, the compiler should warn
        }
    }
    
    for(map<unsigned, StringReceipt>::iterator
        i = fixedstrings.begin();
        i != fixedstrings.end();
        ++i)
    {
        i->second.Apply(*this, "lstring", i->first);
    }
    fixedstrings.clear();

    fprintf(stderr, "Writing other strings...\n");
    set<unsigned> zpages = GetZStringPageList();
    for(set<unsigned>::const_iterator i=zpages.begin(); i!=zpages.end(); ++i)
    {
       unsigned pagenum = *i;
       stringoffsmap pagestrings = GetZStringList(pagenum);
       WritePageZ(pagenum, pagestrings);
    }
}

unsigned insertor::WriteStringTable(stringoffsmap& data, const string& what)
{
    if(data.empty()) return NOWHERE;
    
    FILE *log = GetLogFile("mem", "log_addrs");

    data.GenerateNeederList();

    const stringoffsmap::neederlist_t &neederlist = data.neederlist;
    
    unsigned tableptr = NOWHERE;
    unsigned datapage = NOWHERE;
        
    StringReceipt receipt;

    if(true) /* First do hosts */
    {
        vector<unsigned> todo;
        todo.reserve(data.size() - neederlist.size());
        unsigned reusenum = 0;
        for(unsigned stringnum=0; stringnum<data.size(); ++stringnum)
        {
            if(neederlist.find(stringnum) == neederlist.end())
                todo.push_back(stringnum);
            else
                ++reusenum;
        }

        unsigned todobytes=0;
        for(unsigned a=0; a<todo.size(); ++a)
            todobytes += CalcSize(data[todo[a]].str) + 1;

        vector<freespacerec> Organization(todo.size()+1);
        
        Organization[0].len = data.size() * 2;
        for(unsigned a=0; a<todo.size(); ++a)
            Organization[a+1].len = CalcSize(data[todo[a]].str) + 1;
        
        freespace.OrganizeToAnySamePage(Organization, datapage);
        
        tableptr = Organization[0].pos | (datapage << 16);
        
        /*
           datapage     - page
           todo.size()  - number of strings
           todobytes    - bytes to write
           reusenum     - number of strings reused
           tableptr     - address of the table
        */

        /* Create the table of pointers */
        for(unsigned a=0; a<data.size(); ++a)
        {
            data[a].offs = tableptr + a*2;
        }

        unsigned unwritten = 0;
        for(unsigned a=0; a<todo.size(); ++a)
        {
            unsigned stringnum  = todo[a];
            unsigned ptroffs    = data[stringnum].offs;
            const ctstring &str = data[stringnum].str;

            unsigned spaceptr = Organization[a+1].pos;
            
            data[stringnum].offs = spaceptr;
            
            if(spaceptr == NOWHERE)
                ++unwritten;
            else
                receipt.WriteZPtr(ptroffs, spaceptr, str);
        }
        if(unwritten && log)
            fprintf(log, "  %u string%s unwritten\n", unwritten, unwritten==1?"":"s");
    }

    if(true) /* Then do parasites */
    {
        for(unsigned stringnum=0; stringnum<data.size(); ++stringnum)
        {
            stringoffsmap::neederlist_t::const_iterator i = neederlist.find(stringnum);
            if(i == neederlist.end()) continue;
            
            unsigned parasitenum = i->first;
            unsigned hostnum     = i->second;

            unsigned hostoffs   = data[hostnum].offs;
                    
            unsigned ptroffs    = data[parasitenum].offs;
            const ctstring &str = data[parasitenum].str;
            
            if(hostoffs == NOWHERE)
            {
                if(log)
                    fprintf(log, "  Host %u doesn't exist! ", hostnum);
                
                // Try find another host
                for(hostnum=0; hostnum<data.size(); ++hostnum)
                {
                    // Skip parasites
                    if(neederlist.find(hostnum) != neederlist.end()) continue;
                    // Skip unwritten ones
                    if(data[hostnum].offs == NOWHERE) continue;
                    const ctstring &host = data[hostnum].str;
                    // Impossible host?
                    if(host.size() < str.size()) continue;
                    unsigned extralen = host.size() - str.size();
                    if(str != host.substr(extralen)) continue;
                    hostoffs = data[hostnum].offs;
                    break;
                }
                if(hostoffs == NOWHERE)
                {
                    if(log)
                        fprintf(log, "  String %u wasn't written.\n", parasitenum);
                    continue;
                }
                if(log)
                    fprintf(log, "  Substitute %u assigned for %u.\n",
                        hostnum, parasitenum);
            }

            const ctstring &host = data[hostnum].str;
            
            unsigned place = hostoffs + CalcSize(host) - CalcSize(str);
            
            receipt.WritePtr(ptroffs, place);
        }
    }
    
    receipt.Apply(*this, what, datapage);

    return tableptr;
}

void insertor::WriteRelocatedStrings()
{
    stringoffsmap data;
    data = GetStringList(stringdata::item);
    if(!data.empty())
    {
        unsigned itemtable = WriteStringTable(data, "Items");
        objects.DefineSymbol("ITEMTABLE", itemtable | 0xC00000);
    }
    data = GetStringList(stringdata::tech);
    if(!data.empty())
    {
        unsigned techtable = WriteStringTable(data, "Techs");
        objects.DefineSymbol("TECHTABLE", techtable | 0xC00000);
    }
    data = GetStringList(stringdata::monster);
    if(!data.empty())
    {
        unsigned monstertable = WriteStringTable(data, "Monsters");
        objects.DefineSymbol("MONSTERTABLE", monstertable | 0xC00000);
    }
}

void insertor::GenerateVWF12code()
{
    const unsigned font_begin = get_font_begin();
    const unsigned tilecount  = Font12.GetCount();
    
    /*
     C2:5E1E:
        0  A9 00          - lda a, $00
        2  EB             - xba
        3  38             - sec
        4  A5 35          - lda [$00:D+$35]
        6  E9 A0          - sbc a, $A0
        8  AA             - tax
        9  18             - clc
       10  BF E6 60 C2    - lda $C2:($60E6+x)
       14
       
       Will be changed to:
           
        0  C2 20          - rep $20
        2  EA             - nop
        3  EA             - nop
        4  A5 35          * lda [$00:D+$35]
        6  E2 20          - sep $20
        8  AA             * tax
        9  18             * clc
       10  BF E6 60 C2    * lda $C2:($60E6+x)
       14
       
       Now widthtab may have more than 256 items.
       We may ignore VWF12_WIDTH_INDEX as it's no longer used.
    */

    // patch dialog engine
    PlaceByte(font_begin,  GetConst(CSET_BEGINBYTE), "vwf12 beginbyte");
    
    // patch font engine
    PlaceByte(0xC2, GetConst(VWF12_WIDTH_INDEX)-7, "vwf12 patch"); // rep $20
    PlaceByte(0x20, GetConst(VWF12_WIDTH_INDEX)-6, "vwf12 patch");
    PlaceByte(0xEA, GetConst(VWF12_WIDTH_INDEX)-5, "vwf12 patch"); // nop
    PlaceByte(0xEA, GetConst(VWF12_WIDTH_INDEX)-4, "vwf12 patch"); // nop
    PlaceByte(0xE2, GetConst(VWF12_WIDTH_INDEX)-1, "vwf12 patch"); // sep $20
    PlaceByte(0x20, GetConst(VWF12_WIDTH_INDEX)  , "vwf12 patch");
    /* Done with code patches. */

    /* Create a patch for the width table. */
    vector<unsigned char> widths(tilecount);
    for(unsigned a=0; a<tilecount; ++a) widths[a] = Font12.GetWidth(a);

    O65 widthblock;
    widthblock.LoadCodeFrom(widths);
    widthblock.LocateCode(get_font_begin());
    widthblock.DeclareCodeGlobal("VWF12_WIDTH_TABLE", 0);
    objects.AddObject(widthblock, "VWF12_WIDTH_TABLE");
    objects.AddReference("VWF12_WIDTH_TABLE", OffsPtrFrom(GetConst(VWF12_WIDTH_OFFSET)));
    objects.AddReference("VWF12_WIDTH_TABLE", PagePtrFrom(GetConst(VWF12_WIDTH_SEGMENT)));
    
    unsigned pagegroup = objects.CreateLinkageGroup();
    O65 block1, block2;
    
    /* Create a patch for both tile tables. */
    block1.LoadCodeFrom(Font12.GetTab1());
    block2.LoadCodeFrom(Font12.GetTab2());
    block1.LocateCode(font_begin * 24);
    block2.LocateCode(font_begin * 12);
    block1.DeclareCodeGlobal("VWF12_TABLE1", 0);
    block2.DeclareCodeGlobal("VWF12_TABLE2", 0);
    
    O65linker::LinkageWish wish;
    wish.SetLinkageGroup(pagegroup);
    
    objects.AddObject(block1, "VWF12_TABLE1", wish);
    objects.AddObject(block2, "VWF12_TABLE2", wish);
    
    objects.AddReference("VWF12_TABLE1", PagePtrFrom(GetConst(VWF12_SEGMENT)));
    objects.AddReference("VWF12_TABLE1", OffsPtrFrom(GetConst(VWF12_TAB1_OFFSET)));
    objects.AddReference("VWF12_TABLE2", OffsPtrFrom(GetConst(VWF12_TAB2_OFFSET)));
}
