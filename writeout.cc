#include <string>

#include "ctinsert.hh"
#include "rom.hh"
#include "ctcset.hh"
#include "settings.hh"
#include "config.hh"
#include "logfiles.hh"
#include "rangeset.hh"
#include "pageptrlist.hh"

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
        unsigned addr = o65addrs[a];
        if(addr == NOWHERE) continue;
        ROM.AddPatch(objects.GetCode(a), addr & 0x3FFFFF, objects.GetName(a));
        //objects.Release(a);
    }
    
    freespace.VerboseDump();
}

void insertor::GenerateCode()
{
    WriteFixedStrings();
    WriteOtherStrings();
    WriteRelocatedStrings();

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
    fprintf(stderr, "Writing dictionary...\n");

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
}

void insertor::WriteFixedStrings()
{
    fprintf(stderr, "Writing fixed-length strings...\n");
    
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->type == stringdata::fixed)
        {
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
    fprintf(stderr, "Writing other strings...\n");
    map<unsigned, PagePtrList> tmp;
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
        if(i->type == stringdata::zptr8
        || i->type == stringdata::zptr12)
        {
            const string s = GetString(i->str);
            vector<unsigned char> data(s.c_str(), s.c_str() + s.size() + 1);
            
            tmp[i->address >> 16].AddItem(data, i->address & 0xFFFF);
        }
    }

    for(map<unsigned, PagePtrList>::iterator
        i = tmp.begin(); i != tmp.end(); ++i)
    {
        fprintf(stderr, "Organizing page %02X...\n", i->first);
        i->second.Create(*this, i->first, "zstring");
    }
}

void insertor::WriteStringTable(stringdata::strtype type,
                                const string& tablename,
                                const string& what)
{
    PagePtrList tmp;
    unsigned index = 0;
    for(stringlist::const_iterator i=strings.begin(); i!=strings.end(); ++i)
    {
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
