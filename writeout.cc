#include <string>

#include "ctinsert.hh"
#include "rom.hh"
#include "ctcset.hh"
#include "settings.hh"
#include "config.hh"
#include "logfiles.hh"
#include "stringoffs.hh"
#include "rangemap.hh"

#define INITIALIZE_FREESPACE_ZERO

namespace
{
    void WritePtr(ROM &ROM, unsigned addr, unsigned short value)
    {
        ROM.Write(addr,   value&255);
        ROM.Write(addr+1, value>>8);
    }
    
    unsigned WritePPtr
    (
        ROM &ROM,
        unsigned pointeraddr, // 24-bit
        const ctstring &str,
        unsigned spaceptr     // 16-bit
    )
    {
        if(spaceptr == NOWHERE) return spaceptr;

        unsigned page = pointeraddr >> 16;
        
        WritePtr(ROM, pointeraddr, spaceptr);
        
        spaceptr += page<<16;
        
        const string data = GetString(str);
        
        // Note: Written "length" must be number of _characters_, NOT number of bytes!
        const unsigned length = str.size();
        
#if 0
        fprintf(stderr, "Wrote %u bytes at %06X->%04X; (%02X)",
            data.size()+1,
            pointeraddr,
            spaceptr,
            length);
        for(unsigned a=0; a<data.size(); ++a)
            fprintf(stderr, " %02X", (unsigned char)data[a]);
        fprintf(stderr, "\n");
#endif
        
        ROM.Write(spaceptr, length);
        for(unsigned a=0; a<data.size(); ++a)
            ROM.Write(spaceptr+a+1, data[a]);
        
        return spaceptr & 0xFFFF;
    }

    unsigned WriteZPtr
    (
        ROM &ROM,
        unsigned pointeraddr, //24-bit
        const ctstring &str,
        unsigned spaceptr     // 16-bit
    )
    {
        if(spaceptr == NOWHERE) return spaceptr;

        unsigned page = pointeraddr >> 16;
        
        WritePtr(ROM, pointeraddr, spaceptr);
        
        const string data = GetString(str);
        
#if 0
        fprintf(stderr, "Wrote %u bytes at %06X->%04X: ",
            data.size()+1, pointeraddr, spaceptr);
        fprintf(stderr, DispString(string).c_str());
        fprintf(stderr, "\n");
#endif
        
        spaceptr += page<<16;
        for(unsigned a=0; a<data.size(); ++a)
            ROM.Write(spaceptr+a, data[a]);
        ROM.Write(spaceptr+data.size(), 0);
        return spaceptr & 0xFFFF;
    }

    void WritePageZ
        (ROM &ROM,
         unsigned page,
         stringoffsmap &pagestrings,
         freespacemap &freespace
        )
    {
        FILE *log = GetLogFile("mem", "log_addrs");

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
                todobytes += CalcSize(pagestrings[todo[a]].str) + 1;

            if(log)
                fprintf(log, "  Page %02X: Writing %u strings (%u bytes) and reusing %u\n",
                    page, todo.size(), todobytes, reusenum);

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
                if(spaceptr == NOWHERE) ++unwritten;
                pagestrings[stringnum].offs = WriteZPtr(ROM, ptroffs, str, spaceptr);
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
                
                WritePtr(ROM, ptroffs, place);
            }
        }
    }
}

void insertor::PatchROM(ROM &ROM)
{
#ifdef INITIALIZE_FREESPACE_ZERO
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
#endif
    
    // Write the strings first because they require certain pages!
    WriteStrings(ROM);
    
    // Then generate various things to write
    GenerateCode();
    
    // Write code.
    WriteCode(ROM);
}

void insertor::GenerateCode()
{
    // Do this first, because it requires two blocks on same page.
    GenerateVWF12code();
    GenerateVWF8code();
    
    PlaceData(Font8.GetTiles(), GetConst(TILETAB_8_ADDRESS), "8x8 tiles");
    
    GenerateConjugatorCode();
    GenerateCrononickCode();
    GenerateSignatureCode();
    
    // Do this after all code has been generated.
    LinkAndLocateCode();
    
    // The dictionary fits almost anywhere.
    WriteDictionary();

    /* Ensure all code has addresses */
    list<SNEScode>::iterator i;
    
    vector<freespacerec> blocks;
    for(i=codes.begin(); i!=codes.end(); ++i)
        if(!i->HasAddress())
        {
            blocks.push_back(i->size());
        }
    
    if(!blocks.empty())
    {
        fprintf(stderr, "Organizing %u undetermined blocks:", blocks.size());
        freespace.OrganizeToAnyPage(blocks);
        
        unsigned a=0;
        for(i=codes.begin(); i!=codes.end(); ++i)
            if(!i->HasAddress())
            {
                fprintf(stderr, " \"%s\"", i->GetName().c_str());
                i->YourAddressIs(blocks[a++].pos);
            }
        fprintf(stderr, ", done\n");
    }

    // Doesn't use space.
    PatchTimeBoxes();
}

void insertor::LinkAndLocateCode()
{
    /* Link code objects */
    
    /* Organize the code blobs */
    vector<unsigned> o65sizes = objects.GetSizeList();
    vector<freespacerec> Organization(o65sizes.size());
    for(unsigned a=0; a<o65sizes.size(); ++a)
        Organization[a].len = o65sizes[a];
    freespace.OrganizeToAnyPage(Organization);
    for(unsigned a=0; a<o65sizes.size(); ++a)
        o65sizes[a] = Organization[a].pos;
    objects.PutAddrList(o65sizes);
    
    /* Link the code blobs */
    objects.Link();
    
    /* Send them into the ROM */
    const vector<unsigned> o65addrs = objects.GetAddrList();
    for(unsigned a=0; a<o65addrs.size(); ++a)
    {
        PlaceData(objects.GetCode(a), o65addrs[a], objects.GetName(a));
        objects.Release(a);
    }
    
    /* Load and remember the references */
    vector<pair<ReferMethod, pair<string,unsigned> > > references = objects.GetReferences();
    for(unsigned a=0; a<references.size(); ++a)
    {
        AddReference(references[a].first,
                     references[a].second.second,
                     references[a].second.first);
    }
}

void insertor::WriteCode(ROM &ROM) const
{
    fprintf(stderr, "Writing code and images...\n");
    
    // Patch the name entry function
    //  FIXME: Make this read the pointers from "strings" instead
    
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

    // Testataan moottoria
    // ROM.Write(0x0058DE, 0xA9); //lda A, $08
    // ROM.Write(0x0058DF, 0x07);
    // ROM.Write(0x0058E0, 0xEA); //nop
    // ROM.Write(0x0058E1, 0xEA);

    list<SNEScode>::const_iterator i;
    for(i=codes.begin(); i!=codes.end(); ++i)
    {
        if(!i->HasAddress())
        {
            fprintf(stderr, "Internal ERROR - codeblob still without address\n");
        }
        ROM.AddPatch(*i);
    }
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
        rangemap<unsigned, bool> ranges;
        for(unsigned a=0; a<dictsize; ++a)
        {
            unsigned spaceptr = Organization[a].pos;
            ranges.set(spaceptr, spaceptr + CalcSize(dict[a]) + 1, true);
        }
        
        ranges.compact();
        
        list<rangemap<unsigned,bool>::const_iterator> rangelist;
        ranges.find_all_coinciding(0,0x10000, rangelist);
        
        fprintf(stderr,
            "> Dictionary: %u(table)@ $%06X",
            dictsize*2, dictaddr | 0xC00000);
        
        for(list<rangemap<unsigned,bool>::const_iterator>::const_iterator
            j = rangelist.begin();
            j != rangelist.end();
            ++j)
        {
            unsigned size = (*j)->first.upper - (*j)->first.lower;
            unsigned ptr  = (*j)->first.lower | (dictpage << 16) | 0xC00000;

            fprintf(stderr, ", %u(data)@ $%06X", size, ptr);
        }
        fprintf(stderr, "\n");
    }
    
    for(unsigned a=0; a<dictsize; ++a)
    {
        vector<unsigned char> str;
        str.push_back(dict[a].size());
        str.insert(str.end(), dict[a].begin(), dict[a].end());
        
        unsigned spaceptr = Organization[a].pos | (dictpage << 16);
        
        PlaceData(str, spaceptr, "dict");
        AddReference(OffsPtrFrom(dictaddr + a*2), spaceptr, "dict");
    }
    
    AddReference(OffsPtrFrom(GetConst(DICT_OFFSET)),   dictaddr, "dict ptr");
    AddReference(PagePtrFrom(GetConst(DICT_SEGMENT1)), dictaddr, "dict ptr");
    AddReference(PagePtrFrom(GetConst(DICT_SEGMENT2)), dictaddr, "dict ptr");
}

void insertor::WriteStrings(ROM &ROM)
{
    fprintf(stderr, "Writing fixed-length strings...\n");
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

                unsigned a;
                // These shouldn't contain extrachars.
                for(a=0; a<size; ++a)   ROM.Write(pos++, s[a]);
                for(; a < i->width; ++a)ROM.Write(pos++, 255);
                break;
            }
            case stringdata::zptr8:
            case stringdata::zptr12:
            {
                // These are not interesting here, as they're handled below
                break;
            }
            // If we omitted something, the compiler should warn
        }
    }
   
    fprintf(stderr, "Writing other strings...\n");
    set<unsigned> zpages = GetZStringPageList();
    for(set<unsigned>::const_iterator i=zpages.begin(); i!=zpages.end(); ++i)
    {
       stringoffsmap pagestrings = GetZStringList(*i);
       WritePageZ(ROM, *i, pagestrings, freespace);
    }
}

void insertor::GenerateVWF12code()
{
    // O65 linker can't be used here because of the same-page restriction.
    const vector<unsigned char> &tiletab1 = Font12.GetTab1();
    const vector<unsigned char> &tiletab2 = Font12.GetTab2();
    
    const unsigned font_begin = get_font_begin();
    const unsigned tilecount  = Font12.GetCount();
    
    vector<freespacerec> Organization(2);
    Organization[0].len = tiletab1.size();
    Organization[1].len = tiletab2.size();
    
    unsigned page=NOWHERE;
    freespace.OrganizeToAnySamePage(Organization, page);

    unsigned addr1 = Organization[0].pos + (page<<16);
    unsigned addr2 = Organization[1].pos + (page<<16);

    const unsigned WidthTab_Address = freespace.FindFromAnyPage(tilecount);
    
    fprintf(stderr,
        "> VWF12:"
            " %u(widths)@ $%06X,"
            " %u(tab1)@ $%06X,"
            " %u(tab2)@ $%06X\n",
        tilecount,        0xC00000 | WidthTab_Address,
        tiletab1.size(),  0xC00000 | addr1,
        tiletab2.size(),  0xC00000 | addr2
           );
    
    /* As this pointer doesn't point directly to the data,
     * it can't be set with AddOffsPtrFrom/AddSegPtrFrom
     */
    // patch dialog engine
    PlaceByte(font_begin,  GetConst(CSET_BEGINBYTE), "vwf12 beginbyte");
    
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

    // patch font engine
    PlaceByte(0xC2, GetConst(VWF12_WIDTH_INDEX)-7, "vwf12 patch"); // rep $20
    PlaceByte(0x20, GetConst(VWF12_WIDTH_INDEX)-6, "vwf12 patch");
    PlaceByte(0xEA, GetConst(VWF12_WIDTH_INDEX)-5, "vwf12 patch"); // nop
    PlaceByte(0xEA, GetConst(VWF12_WIDTH_INDEX)-4, "vwf12 patch"); // nop
    PlaceByte(0xE2, GetConst(VWF12_WIDTH_INDEX)-1, "vwf12 patch"); // sep $20
    PlaceByte(0x20, GetConst(VWF12_WIDTH_INDEX)  , "vwf12 patch");

    // patch font engine
    unsigned tmp = (WidthTab_Address - font_begin) | 0xC00000;
    
    AddReference(OffsPtrFrom(GetConst(VWF12_WIDTH_OFFSET)),  tmp, "vwf12 width ptr");
    AddReference(PagePtrFrom(GetConst(VWF12_WIDTH_SEGMENT)), tmp, "vwf12 width ptr");
    
    tmp = (addr1 - font_begin * 24) | 0xC00000;
    // patch font engine
    AddReference(OffsPtrFrom(GetConst(VWF12_TAB1_OFFSET)), tmp, "vwf12 ptr1");
    AddReference(PagePtrFrom(GetConst(VWF12_SEGMENT)),     tmp, "vwf12 ptr1,2");
    tmp = addr2 - font_begin * 12;
    AddReference(OffsPtrFrom(GetConst(VWF12_TAB2_OFFSET)), tmp, "vwf12 ptr2");
    
    vector<unsigned char> widths(tilecount);
    for(unsigned a=0; a<tilecount; ++a) widths[a] = Font12.GetWidth(a);
    
    PlaceData(tiletab1, addr1, "font12 tab1");
    PlaceData(tiletab2, addr2, "font12 tab2");
    PlaceData(widths, WidthTab_Address, "font12 widths");
}

void insertor::PatchTimeBoxes()
{
    fprintf(stderr, "Patching map time boxes...\n");

    // Patch tile indices in -65M textbox
    {
        static const unsigned char Patch[16] =
        { 0xFC, 0x00, 0x88, 0x01,
          0x0C, 0x00, 0x8C, 0x01,
          0x1C, 0x00, 0x8E, 0x01,
          0x2C, 0x00, 0x8A, 0x01 };
        PlaceData(vector<unsigned char>(Patch,Patch+16), 0x03F87F, "timebox patch -65M");
    }

    // Patch tile indices in -12k textbox
    {
        static const unsigned char Patch[12] =
        { 0xFE, 0x00, 0xA0, 0x01,
          0x0E, 0x00, 0xA2, 0x01,
          0x1E, 0x00, 0x8A, 0x01 };
        PlaceData(vector<unsigned char>(Patch,Patch+12), 0x03F890, "timebox patch -12k");
    }
}
