#include "ctinsert.hh"
#include "tgaimage.hh"
#include "rom.hh"

void insertor::LoadImage(const string &fn, unsigned address)
{
    const TGAimage image(fn);
    imagedata result;
    result.address = address;
    
    unsigned tiles_x = image.GetXdim() / 8;
    unsigned tiles_y = image.GetYdim() / 8;
    
    const unsigned tilecount = tiles_x * tiles_y;
    const unsigned bitness   = image.GetPalSize()==4 ? 2 : 4;
    
    if(!tilecount) return;
    
    fprintf(stderr, "Loading '%s'... (%u bits, %u by %u tiles)\n",
        fn.c_str(), bitness, tiles_x, tiles_y);
    
    result.data.resize(tilecount * 8 * bitness);
    
    const unsigned slice = (bitness==1 ? 1 : 2);
    
    for(unsigned a=0; a<tilecount; ++a)
    {
        const unsigned tilex = 8 * (a % tiles_x);
        const unsigned tiley = 8 * (a / tiles_x);
        const unsigned tileoffs = a * 8 * bitness;
        
        for(unsigned ty=tiley,y=0; y<8; ++y,++ty)
        {
            unsigned char byte[8] = {0,0,0,0};
            for(unsigned tx=tilex,x=0; x<8; ++x,++tx)
            {
                unsigned pix = image.Point(tx,ty);
                for(unsigned bit = 0; bit < bitness; ++bit)
                {
                    byte[bit] |= (pix&1) << (7-x);
                    pix >>= 1;
                }
            }
            
            for(unsigned bit=0; bit<bitness; ++bit)
            {
                result.data[
                   tileoffs
                 + y * slice
                 + (bit&1)
                 + (bit/2)*16
                           ] = byte[bit];
            }
        }
    }
    images.push_back(result);
}

void insertor::WriteImages(ROM &ROM) const
{
    fprintf(stderr, "Writing images...\n");
    for(imagelist::const_iterator i = images.begin(); i != images.end(); ++i)
    {
        const imagedata& image = *i;
        
        for(unsigned a=0; a<image.data.size(); ++a)
            ROM.Write(image.address + a, image.data[a]);
    }
}
