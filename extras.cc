#include "extras.hh"

extrasizemap_t Extras_2, Extras_8, Extras_12;

namespace
{
    void Load_8()
    {
        extrasizemap_t& extrasizes = Extras_8;
        
      //extrasizes[1] = 0; // next
        extrasizes[2] = 2; // goto
        extrasizes[3] = 2+2;// func1
        extrasizes[4] = 3; // substr
        extrasizes[5] = 2; // member
        extrasizes[6] = 2; // attrs
        extrasizes[7] = 2; // out
        extrasizes[8] = 1; // spc
        extrasizes[9] = 1; // len
        extrasizes[10] = 1; // attr
        extrasizes[11] = 2+2; // func2
        extrasizes[12] = 1+2; // stat
    }

    void Load_12()
    {
        extrasizemap_t& extrasizes = Extras_12;
        
        extrasizes[0x12] = 1; // monster/item
        //extrasizes[0x03] = 1; // delay - for some reason, it doesn't eat space
    }
    
    class Loader
    {
    public:
        Loader()
        {
            Load_8();
            Load_12();
        }
    } Loader;
}
