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

	LinkCalls("vwf8", vwf8_code);

    vwf8_code.Verify();
    
    PlaceData(widths,              WidthTab_Address);
    PlaceData(tiletab,             TileTab_Address);
    PlaceData(vwf8_code.GetCode(), Code_Address);
}
