#include "tgaimage.hh"
#include "ctinsert.hh"
#include "ctcset.hh"
#include "fonts.hh"

void Font12data::Load(const string &filename)
{
    fprintf(stderr, "Loading '%s'...\n", filename.c_str());
    TGAimage font12(filename);
    font12.setboxsize(12, 12);
    font12.setboxperline(32);
    
    static const char palette[] = {0,0,1,2,3,0};
    
    unsigned boxcount = font12.getboxcount();
    unsigned boxstart = 0;
    if(boxcount > get_num_chronochars()) boxstart = boxcount - get_num_chronochars();
    
    tiletab1.resize(get_num_chronochars() * 24);
    tiletab2.resize(get_num_chronochars() * 12);
    
    widths.resize(get_num_chronochars());
    
    unsigned to=0;
    for(unsigned a=0; a<get_num_chronochars(); ++a)
    {
        vector<char> box = font12.getbox(a + boxstart);

        unsigned width=0;
        while(box[width] != 5 && width<12)++width;
        
        for(unsigned p=0; p<box.size(); ++p)
            if((unsigned char)box[p] < sizeof(palette))
                box[p] = palette[box[p]];
        
        unsigned po=0;
        for(unsigned y=0; y<12; ++y)
        {
            unsigned char byte1 = 0;
            unsigned char byte2 = 0;
            unsigned char byte3 = 0;
            unsigned char byte4 = 0;
            for(unsigned x=0; x<8; ++x)
            {
                unsigned shift = (7-x)&7;
                byte1 |= ((box[po]&1)  ) << shift;
                byte2 |= ((box[po]&2)/2) << shift;
                ++po;
            }
            for(unsigned x=0; x<4; ++x)
            {
                unsigned shift = (7-x)&7;
                byte3 |= ((box[po]&1)  ) << shift;
                byte4 |= ((box[po]&2)/2) << shift;
                ++po;
            }
            tiletab1[to++] = byte1;
            tiletab1[to++] = byte2;
            
            if(a&1)byte3 <<= 4;
            tiletab2[(a>>1)*24 + y*2  ] |= byte3;
            if(a&1)byte4 <<= 4;
            tiletab2[(a>>1)*24 + y*2+1] |= byte4;
        }
        widths[a] = width;
    }
}


void Font8data::Load(const string &filename)
{
    fprintf(stderr, "Loading '%s'...\n", filename.c_str());

    TGAimage font8(filename);
    font8.setboxsize(8, 8);
    font8.setboxperline(32);
    
    static const char palette[] = {0,0,1,2,3};
    
    tiletable.resize(256 * 16);
    
    unsigned to=0;
    for(unsigned a=0; a<256; ++a)
    {
        vector<char> box = font8.getbox(a);
        for(unsigned p=0; p<box.size(); ++p)
            if((unsigned char)box[p] < sizeof(palette))
                box[p] = palette[box[p]];
        
        unsigned po=0;
        for(unsigned y=0; y<8; ++y)
        {
            unsigned char byte1 = 0;
            unsigned char byte2 = 0;
            for(unsigned x=0; x<8; ++x)
            {
                unsigned shift = (7-x);
                byte1 |= ((box[po]&1)  ) << shift;
                byte2 |= ((box[po]&2)/2) << shift;
                ++po;
            }
            tiletable[to++] = byte1;
            tiletable[to++] = byte2;
        }
    }
}

unsigned insertor::GetFont12width(unsigned char chronoch) const
{
    return Font12.GetWidth(chronoch - (0x100-get_num_chronochars()));
}
