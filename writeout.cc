#include <string>

#include "ctinsert.hh"
#include "rom.hh"
#include "ctcset.hh"
#include "settings.hh"
#include "config.hh"
#include "logfiles.hh"
#include "rangeset.hh"
#include "pageptrlist.hh"
#include "msginsert.hh"

void insertor::ClearROM(ROM &ROM) const
{
    const bool ClearSpace = GetConf("patch", "clear_free_space");
    if(!ClearSpace) return;

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
            
            ROM.SetZero(offs, reclen, "clear free space");
        }
    }
}

void insertor::PatchROM(ROM &ROM) const
{
    ClearROM(ROM);
    
    fprintf(stderr, "Inhabitating the ROM image...\n");
    
    /* Then write everything. */

    const vector<unsigned> o65addrs = objects.GetAddrList();
    for(unsigned a=0; a<o65addrs.size(); ++a)
    {
        unsigned addr = o65addrs[a];
        if(addr == NOWHERE) continue;
        ROM.AddPatch(objects.GetCode(a), addr & 0x3FFFFF, objects.GetName(a));
        //objects.Release(a);
    }
    
    freespace.VerboseDump();
}

void insertor::WriteEverything()
{
    // These don't have to be in any particular order.
    WriteImages();
    WriteStrings();
    WriteFont8();
    WriteVWF12();
    WriteConjugator();
    WriteUserCode();
    WriteDictionary();
    
    const bool UseThinNumbers = GetConf("font", "use_thin_numbers");
    
    // Patch the name entry function
    //  FIXME: Make this read the pointers from "strings" instead
    
    if(UseThinNumbers)
    {
        PlaceByte(0x73, 0x02F21B, "thin '0'");
    }
    else
    {
        //PlaceByte(0xD4, 0x02F21B, "thick '0'");
    }

    /* Everything has been written. */
    
    /* Now organize them. */
    freespace.OrganizeO65linker(objects);
    
    /* Resolve all references. */
    objects.Link();
}

void insertor::WriteFixedStrings()
{
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->type == stringdata::fixed)
        {
            MessageWorking();
            
            unsigned pos = i->address;
            const ctstring &s = i->str;
            
            // Fixed strings don't contain extrachars.
            // Thus s.size() is safe.
            unsigned size = s.size();
            
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

            vector<unsigned char> Buf(i->width, 255);
            if(size > i->width) size = i->width;
            
            std::copy(s.begin(), s.begin()+size, Buf.begin());
            
            objects.AddLump(Buf, pos & 0x3FFFFF, "lstring");
        }
    }
}

void insertor::WriteOtherStrings()
{
    map<unsigned, PagePtrList> tmp;
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->type == stringdata::zptr8
        || i->type == stringdata::zptr12)
        {
            MessageWorking();
            
            const string s = GetString(i->str);
            vector<unsigned char> data(s.c_str(), s.c_str() + s.size() + 1);
            
#if 0
            fprintf(stderr, "String: '%s'", DispString(i->str).c_str());
            fprintf(stderr, "\n");
            for(unsigned a=0; a<data.size(); ++a)
                fprintf(stderr, " %02X", data[a]);
            fprintf(stderr, "\n");
#endif
            
            tmp[i->address >> 16].AddItem(data, i->address & 0xFFFF);
        }
    }

    for(map<unsigned, PagePtrList>::iterator
        i = tmp.begin(); i != tmp.end(); ++i)
    {
        char Buf[64]; sprintf(Buf, "page $%02X", i->first);
        MessageLoadingItem(Buf);
        
        MessageWorking();
        
        i->second.Create(*this, i->first, "zstring");
    }
}

void insertor::WriteDictionary()
{
    MessageWritingDict();

    PagePtrList tmp;
    
    for(unsigned a=0; a<dict.size(); ++a)
    {
        const string s = GetString(dict[a]);
        vector<unsigned char> Buf(s.size() + 1);
        std::copy(s.begin(), s.begin()+s.size(), Buf.begin()+1);
        Buf[0] = s.size();
        
        tmp.AddItem(Buf, a*2);
    }
    
    tmp.Create(*this, "dict", "DICT_TABLE");
    
    objects.AddReference("DICT_TABLE", OffsPtrFrom(GetConst(DICT_OFFSET)));
    objects.AddReference("DICT_TABLE", PagePtrFrom(GetConst(DICT_SEGMENT1)));
    objects.AddReference("DICT_TABLE", PagePtrFrom(GetConst(DICT_SEGMENT2)));
    
    MessageDone();
}

void insertor::WriteStringTable(stringdata::strtype type,
                                const string& tablename,
                                const string& what)
{
    MessageLoadingItem(what);
    PagePtrList tmp;
    unsigned index = 0;
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        MessageWorking();
        if(i->type == type)
        {
            const string s = GetString(i->str);
            vector<unsigned char> data(s.c_str(), s.c_str() + s.size() + 1);
            tmp.AddItem(data, index);
            index += 2;
        }
    }
    if(index) tmp.Create(*this, what, tablename);
}

void insertor::WriteRelocatedStrings()
{
    WriteStringTable(stringdata::item,    "ITEMTABLE", "Items");
    WriteStringTable(stringdata::tech,    "TECHTABLE", "Techs");
    WriteStringTable(stringdata::monster, "MONSTERTABLE", "Monsters");
}

void insertor::WriteStrings()
{
    MessageWritingStrings();
    WriteFixedStrings();
    WriteOtherStrings();
    WriteRelocatedStrings();
    MessageDone();
}

void insertor::WriteFont8()
{
    objects.AddLump(Font8.GetTiles(), GetConst(TILETAB_8_ADDRESS), "8x8 tiles");
}

void insertor::WriteVWF12()
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
