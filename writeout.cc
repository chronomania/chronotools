#include <string>

#include "ctinsert.hh"
#include "rom.hh"
#include "ctcset.hh"
#include "settings.hh"
#include "config.hh"
#include "logfiles.hh"
#include "stringoffs.hh"

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
        
        const string data = GetString(str);
        
#if 0
        fprintf(stderr, "Wrote %u bytes at %06X->%04X\n",
            data.size()+1, pointeraddr, spaceptr);
#endif
        
        spaceptr += page<<16;
        ROM.Write(spaceptr, data.size());
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
                
                unsigned place = hostoffs + host.size()-str.size();
                
                WritePtr(ROM, ptroffs, place);
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
    
    WriteStrings(ROM);
    Write8pixfont(ROM);
    if(GetConf("font", "use_vwf8"))
    {
        // This function will also generate the code to handle the font.
        Write8vpixfont(ROM);
    }
    Write12pixfont(ROM);
    WriteDictionary(ROM);

    GenerateCode(); 
    WriteCode(ROM);
}

void insertor::GenerateCode()
{
    GenerateConjugatorCode();
}

void insertor::WriteCode(ROM &ROM) const
{
    fprintf(stderr, "Writing code...\n");
    
    list<SNEScode>::const_iterator i;
    for(i=codes.begin(); i!=codes.end(); ++i)
        ROM.AddPatch(*i);

    // SEP+JSR takes 5 bytes. We overwrote it
    // with 4 bytes (see conjugate.cc).
    // Patch with NOP.
    ROM.Write(ConjugatePatchAddress + 4, 0xEA);
    
    // Testataan item-listaa
    //ROM.Write(0x02EFB4, 0xA9);
    //ROM.Write(0x02EFB5, 0x10);
}

void insertor::Write8pixfont(ROM &ROM) const
{
    const vector<unsigned char> &tiletab = Font8.GetTiles();
    
    fprintf(stderr, "Writing 8-pix font... (%u bytes)\n", tiletab.size());

    for(unsigned a=0; a<tiletab.size(); ++a)
        ROM.Write(Font8_Address + a, tiletab[a]);
}

void insertor::Write8vpixfont(ROM &ROM)
{
    const vector<unsigned char> &widths  = Font8v.GetWidths();
    const vector<unsigned char> &tiletab = Font8v.GetTiles();
    
    fprintf(stderr, "Writing 8-pix VWF... ");

    unsigned WidthTab_Address = freespace.FindFromAnyPage(widths.size());
    unsigned TileTab_Address  = freespace.FindFromAnyPage(tiletab.size());
    
    fprintf(stderr, "(%u bytes at %02X:%04X,"
                    " %u bytes at %02X:%04X)\n",
        widths.size(),   0xC0 | (WidthTab_Address >> 16), WidthTab_Address & 0xFFFF,
        tiletab.size(),  0xC0 | (TileTab_Address >> 16), TileTab_Address  & 0xFFFF
           );

    for(unsigned a=0; a<widths.size(); ++a) ROM.Write(WidthTab_Address + a, widths[a]);
    for(unsigned a=0; a<tiletab.size(); ++a) ROM.Write(TileTab_Address + a, tiletab[a]);

    GenerateVWF8code(WidthTab_Address, TileTab_Address);
    
    // Patch equip-left-func
    ROM.Write(0x02A5AA+4, 0xEA); // NOP
    ROM.Write(0x02A5AA+5, 0xEA); // NOP

    // Patch item2func
    ROM.Write(0x02F2DC+4, 0xEA); // NOP
    ROM.Write(0x02F2DC+5, 0x60); // RTS

    // Patch for item3func
    ROM.Write(0x02B053+4, 0xEA); // NOP -..and
    ROM.Write(0x02B053+5, 0xEA); // NOP - asl a
    ROM.Write(0x02B053+6, 0xEA); // NOP - tay
    ROM.Write(0x02B053+7, 0xEA); // NOP - lda
    ROM.Write(0x02B053+8, 0xEA); // NOP
    ROM.Write(0x02B053+9, 0xEA); // NOP
    ROM.Write(0x02B053+10, 0xEA);// NOP - tay
    ROM.Write(0x02B053+11, 0xEA); // NOP - lda
    ROM.Write(0x02B053+12, 0xEA); // NOP
    ROM.Write(0x02B053+13, 0xEA); // NOP
    ROM.Write(0x02B053+14, 0xEA); // NOP - jsr
    ROM.Write(0x02B053+15, 0xEA); // NOP
    ROM.Write(0x02B053+16, 0xEA); // NOP

    // Patch tech1func
    ROM.Write(0x02BDE3+4, 0xEA); // NOP
    ROM.Write(0x02BDE3+5, 0xEA); // NOP

}

void insertor::Write12pixfont(ROM &ROM)
{
    const vector<unsigned char> &tiletab1 = Font12.GetTab1();
    const vector<unsigned char> &tiletab2 = Font12.GetTab2();
    
    fprintf(stderr, "Writing 12-pix font... ");

    unsigned tilecount  = get_num_chronochars() + get_num_extrachars();
    unsigned font_begin = 0x100 - get_num_chronochars();
    
    vector<freespacerec> Organization(2);
    Organization[0].len = tiletab1.size();
    Organization[1].len = tiletab2.size();
    
    unsigned tmp=NOWHERE;
    freespace.OrganizeToAnySamePage(Organization, tmp);

    unsigned addr1 = Organization[0].pos + (tmp<<16);
    unsigned addr2 = Organization[1].pos + (tmp<<16);

    unsigned WidthTab_Address = freespace.FindFromAnyPage(tilecount);
    
    fprintf(stderr, "(%u bytes at %02X:%04X,"
                    " %u bytes at %02X:%04X,"
                    " %u bytes at %02X:%04X)\n",
        tilecount,        0xC0 | (WidthTab_Address >> 16), WidthTab_Address & 0xFFFF,
        tiletab1.size(),  0xC0 | (addr1 >> 16), addr1  & 0xFFFF,
        tiletab2.size(),  0xC0 | (addr2 >> 16), addr2  & 0xFFFF
           );
    
    // patch font engine
    tmp = WidthTab_Address - font_begin;
    ROM.Write(WidthTab_Address_Ofs+0, tmp & 255);
    ROM.Write(WidthTab_Address_Ofs+1, (tmp >> 8) & 255);
    ROM.Write(WidthTab_Address_Seg, 0xC0 | ((tmp >> 16) & 255));
    
    // patch dialog engine
    ROM.Write(FirstChar_Address, font_begin);
    
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
    */

    // patch font engine
    ROM.Write(WidthTab_Offset_Addr-7, 0xC2); // rep $20
    ROM.Write(WidthTab_Offset_Addr-6, 0x20);
    ROM.Write(WidthTab_Offset_Addr-5, 0xEA); // nop
    ROM.Write(WidthTab_Offset_Addr-4, 0xEA); // nop
    ROM.Write(WidthTab_Offset_Addr-1, 0xE2); // sep $20
    ROM.Write(WidthTab_Offset_Addr  , 0x20);

    tmp = addr1 - font_begin * 24;
    // patch font engine
    ROM.Write(Font12a_Address_Ofs+0, tmp & 255);
    ROM.Write(Font12a_Address_Ofs+1, (tmp >> 8) & 255);
    ROM.Write(Font12_Address_Seg, 0xC0 | ((tmp >> 16) & 255));
    tmp = addr2 - font_begin * 12;
    ROM.Write(Font12b_Address_Ofs+0, tmp & 255);
    ROM.Write(Font12b_Address_Ofs+1, (tmp >> 8) & 255);
    
    for(unsigned a=0; a<tiletab1.size(); ++a) ROM.Write(addr1+a, tiletab1[a]);
    for(unsigned a=0; a<tiletab2.size(); ++a) ROM.Write(addr2+a, tiletab2[a]);

    for(unsigned a=0; a<tilecount; ++a) ROM.Write(WidthTab_Address+a, Font12.GetWidth(a));
}

void insertor::WriteDictionary(ROM &ROM)
{
    unsigned size = 0;
    unsigned dictsize = dict.size();
    for(unsigned a=0; a<dictsize; ++a)size += CalcSize(dict[a]) + 1;

    vector<freespacerec> Organization(dictsize + 1);
    for(unsigned a=0; a<dictsize; ++a)
        Organization[a].len = CalcSize(dict[a]) + 1;
    
    Organization[dictsize] = dictsize*2;

    unsigned dictpage = NOWHERE;
    freespace.OrganizeToAnySamePage(Organization, dictpage);
    unsigned dictaddr = Organization[dictsize].pos + (dictpage << 16);

    fprintf(stderr, "Writing dictionary (%u pointers at %02X:%04X, %u bytes)...\n",
        dictsize, dictpage|0xC0, dictaddr&0xFFFF, size);
    
    for(unsigned a=0; a<dictsize; ++a)
    {
        unsigned spaceptr = Organization[a].pos;
        WritePPtr(ROM, dictaddr + a*2, dict[a], spaceptr);
    }
    ROM.Write(DictAddr_Ofs+0, dictaddr & 255);
    ROM.Write(DictAddr_Ofs+1, (dictaddr >> 8) & 255);
    ROM.Write(DictAddr_Seg_1, 0xC0 | dictpage);
    ROM.Write(DictAddr_Seg_2, 0xC0 | dictpage);
}

void insertor::WriteStrings(ROM &ROM)
{
    fprintf(stderr, "Writing fixed-length strings...\n");
    for(stringmap::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        switch(i->second.type)
        {
            case stringdata::fixed:
            {
                unsigned pos = i->first;
                const ctstring &s = i->second.str;
                
                unsigned size = CalcSize(s);
                
                if(size > i->second.width)
                {
#if 0
                    fprintf(stderr, "  Warning: Fixed string at %06X: len(%u) > space(%u)... '%s'\n",
                        pos, size, i->second.width, DispString(s).c_str());
#endif
                    size = i->second.width;
                }
                
                // Filler must be 255, or otherwise following problems occur:
                //     item listing goes zigzag
                //     12pix item/tech/mons text in battle has garbage (char 0 in font).

                unsigned a;
                // These shouldn't contain extrachars.
                for(a=0; a<size; ++a)          ROM.Write(pos++, s[a]);
                for(; a < i->second.width; ++a)ROM.Write(pos++, 255);
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
