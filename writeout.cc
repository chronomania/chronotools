#include <string>

#include "ctinsert.hh"
#include "rom.hh"
#include "ctcset.hh"
#include "config.hh"
#include "logfiles.hh"

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
    WriteFonts();
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
