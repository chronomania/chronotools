#include "compiler.hh"
#include "ctinsert.hh"
#include "config.hh"
#include "settings.hh"
#include "o65.hh"

void insertor::GenerateVWF8code()
{
    const vector<unsigned char> &widths  = Font8v.GetWidths();
    const vector<unsigned char> &tiletab = Font8v.GetTiles();
    
    fprintf(stderr, "8-pix VWF will be placed:");

    const unsigned WidthTab_Address = freespace.FindFromAnyPage(widths.size());
    const unsigned TileTab_Address  = freespace.FindFromAnyPage(tiletab.size());
    
    fprintf(stderr, " %u bytes at $%06X,"
                    " %u bytes at $%06X\n",
        widths.size(),   0xC00000 | WidthTab_Address,
        tiletab.size(),  0xC00000 | TileTab_Address
           );

    PlaceData(widths,  WidthTab_Address);
    PlaceData(tiletab, TileTab_Address);

    const string codefile = WstrToAsc(GetConf("vwf8", "file").SField());
    
    FILE *fp = fopen(codefile.c_str(), "rb");
    if(!fp) return;
    
    fprintf(stderr, "Loading '%s'...\n", codefile.c_str());
    
    O65 vwf8_code;
    
    vwf8_code.Load(fp);
    fclose(fp);
    
    unsigned code_size = vwf8_code.GetCodeSize();
    unsigned code_addr = freespace.FindFromAnyPage(code_size);
    vwf8_code.LocateCode(code_addr);
    
    vwf8_code.LinkSym("WIDTH_SEG",   (WidthTab_Address >> 16) | 0xC0);
    vwf8_code.LinkSym("WIDTH_OFFS",  (WidthTab_Address & 0xFFFF));
    vwf8_code.LinkSym("TILEDATA_SEG",  (TileTab_Address >> 16) | 0xC0);
    vwf8_code.LinkSym("TILEDATA_OFFS", (TileTab_Address & 0xFFFF));
    
#if 0
    DrawS_2bit = vwf8_code.GetSymAddress(WstrToAsc(GetConf("vwf8", "b2_draws")));
    DrawS_4bit = vwf8_code.GetSymAddress(WstrToAsc(GetConf("vwf8", "b4_draws")));
#endif

    const ConfParser::ElemVec& elems = GetConf("vwf8", "add_call_of").Fields();
    for(unsigned a=0; a<elems.size(); a += 4)
    {
        const ucs4string& funcname = elems[a];
        unsigned address           = elems[a+1];
        unsigned nopcount          = elems[a+2];
        bool add_rts               = elems[a+3];
        
        SNEScode tmpcode;
        tmpcode.AddCallFrom(address);
        tmpcode.YourAddressIs(vwf8_code.GetSymAddress(WstrToAsc(funcname)));
        codes.push_back(tmpcode);
        
        address += 4;
        if(add_rts)
        {
            PlaceByte(0x60, address++);
            if(nopcount > 0)
            {
                freespace.Add((address >> 16) & 0x3F,
                              address & 0xFFFF,
                              nopcount);
                
                // Mark zero instead of NOP...
                while(nopcount > 0) { PlaceByte(0, address++); --nopcount; }
            }
        }
        else
        {
            while(nopcount > 0) { PlaceByte(0xEA, address++); --nopcount; }
        }
    }
    
    vwf8_code.Verify();
    
    PlaceData(vwf8_code.GetCode(), code_addr);
}
