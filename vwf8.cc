#include "ctinsert.hh"
#include "config.hh"
#include "settings.hh"
#include "o65.hh"

void insertor::GenerateVWF8code()
{
    if(!GetConf("font", "use_vwf8"))
    {
        return;
    }
    
    const vector<unsigned char> &widths  = Font8v.GetWidths();
    const vector<unsigned char> &tiletab = Font8v.GetTiles();
    
    const string codefile = WstrToAsc(GetConf("vwf8", "file").SField());
    
    FILE *fp = fopen(codefile.c_str(), "rb");
    if(!fp) return;
    
    fprintf(stderr, "Loading '%s'...\n", codefile.c_str());
    
    O65 vwf8_code;
    
    vwf8_code.Load(fp);
    fclose(fp);
    
    const unsigned Code_Size = vwf8_code.GetCodeSize();
    
    vector<freespacerec> Organization(3);
    Organization[0].len = Code_Size;
    Organization[1].len = widths.size();
    Organization[2].len = tiletab.size();
    
    freespace.OrganizeToAnyPage(Organization);
    
    const unsigned Code_Address     = Organization[0].pos;
    const unsigned WidthTab_Address = Organization[1].pos;
    const unsigned TileTab_Address  = Organization[2].pos;
    
    vwf8_code.LocateCode(Code_Address | 0xC00000);
    
    fprintf(stderr, "Writing VWF8:"
                    " %u(widths)@ $%06X,"
                    " %u(tiles)@ $%06X,"
                    " %u(code)@ $%06X\n",
        widths.size(),   0xC00000 | WidthTab_Address,
        tiletab.size(),  0xC00000 | TileTab_Address,
        Code_Size,       0xC00000 | Code_Address
           );

    vwf8_code.LinkSym("WIDTH_ADDR",   WidthTab_Address | 0xC00000);
    vwf8_code.LinkSym("TILEDATA_ADDR", TileTab_Address | 0xC00000);
    
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
                
                // Don't initialize, or it will overwrite whatever
                // uses that space!
                //while(skipbytes-- > 0) { PlaceByte(0, address++); --nopcount; }
                nopcount = 0;
            }
        }
        else
        {
            while(nopcount > 2)
            {
                nopcount -= 2;
                unsigned skipbytes = nopcount;
                if(skipbytes > 127) skipbytes = 127;
                PlaceByte(0x80,      address++); // BRA over the space.
                PlaceByte(skipbytes, address++);
                
                freespace.Add((address >> 16) & 0x3F,
                              address & 0xFFFF,
                              skipbytes);
                
                // Don't initialize, or it will overwrite whatever
                // uses that space!
                //while(skipbytes-- > 0) { PlaceByte(0, address++); --nopcount; }
                nopcount -= skipbytes;
            }
            while(nopcount > 0) { PlaceByte(0xEA, address++); --nopcount; }
        }
    }
    
    vwf8_code.Verify();
    
    PlaceData(widths,              WidthTab_Address);
    PlaceData(tiletab,             TileTab_Address);
    PlaceData(vwf8_code.GetCode(), Code_Address);
}
