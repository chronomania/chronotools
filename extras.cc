#include "extras.hh"

extrasizemap_t Extras_2, Extras_8, Extras_12;

namespace
{
    void Load_8()
    {
        extrasizemap_t& extrasizes = Extras_8;
        
        extrasizes[2] = 2;
        extrasizes[3] = 2+2;
        extrasizes[4] = 3;
        extrasizes[5] = 2;
        extrasizes[6] = 2;
        extrasizes[7] = 2;
        extrasizes[8] = 1;
        extrasizes[9] = 1;
        extrasizes[10] = 1;
        extrasizes[11] = 2+2;
        extrasizes[12] = 1+2;
    }

    void Load_12()
    {
        extrasizemap_t& extrasizes = Extras_12;
        
        extrasizes[0x12] = 1;
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
