#include "ctinsert.hh"
#include "config.hh"
#include "settings.hh"
#include "o65.hh"

void insertor::GenerateVWF8code()
{
    const vector<unsigned char> &widths  = Font8v.GetWidths();
    const vector<unsigned char> &tiletab = Font8v.GetTiles();
    
    const string codefile = WstrToAsc(GetConf("vwf8", "file").SField());
    
    O65 vwf8_code = LoadObject(codefile, "VWF8");
    if(vwf8_code.Error()) return;
    
    if(!LinkCalls("vwf8"))
    {
        fprintf(stderr, "> > VWF8 won't be used\n");
        return;
    }
    
    objects.AddObject(CreateObject(Font8v.GetWidths(), "WIDTH_ADDR"),    "vwf8 widths");
    objects.AddObject(CreateObject(Font8v.GetTiles(),  "TILEDATA_ADDR"), "vwf8 tiles");
    objects.AddObject(vwf8_code, "vwf8 code");
}
