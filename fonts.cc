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
    unsigned max_extra = 0;
    
    if(boxcount > 0x100)
    {
        max_extra = boxcount - 0x100;
        boxcount = 0x100;
    }
    
    unsigned charcount = get_num_chronochars();
    unsigned extracount = get_num_extrachars();
    
    if(boxcount > charcount) boxstart = boxcount - charcount;
    
    if(extracount > max_extra) extracount = max_extra;
    
    charcount += extracount;
    
    charcount = (charcount + 1) &~ 1;
    
    tiletab1.resize(charcount * 24, 0);
    tiletab2.resize(charcount * 12, 0);
    
    widths.resize(charcount);
    
    unsigned to=0;
    for(unsigned a=0; a<charcount; ++a)
    {
        vector<char> box = font12.getbox(a + boxstart);
        unsigned width=0;
        while(box[width] != 5 && width < 12)++width;
        
#if 1
        fprintf(stderr, "box %03X: %u(+%u) - width=%u:\n",
            a + (0x100-get_num_chronochars()),
            a,
            boxstart, width);
        for(unsigned y=0; y<12; ++y)
        {
            fprintf(stderr, "   ");
            for(unsigned x=0; x<12; ++x)
                fprintf(stderr, "%c", "0.\"/#-"[box[y*12+x]]);
            fprintf(stderr, "\n");
        }
#endif

        for(unsigned p=0; p<box.size(); ++p)
            if((unsigned char)box[p] < sizeof(palette))
                box[p] = palette[ static_cast<unsigned> (box[p]) ];
        
        unsigned po=0, to2=(a>>1)*24;
        for(unsigned y=0; y<12; ++y)
        {
            #define gb(n,v) ((box[po+n]&v)/v)
            
            if(true)
            {
                unsigned char
                byte1 = (gb( 0,1) << 7)
                      | (gb( 1,1) << 6)
                      | (gb( 2,1) << 5)
                      | (gb( 3,1) << 4)
                      | (gb( 4,1) << 3)
                      | (gb( 5,1) << 2)
                      | (gb( 6,1) << 1)
                      | (gb( 7,1));
                unsigned char
                byte2 = (gb( 0,2) << 7)
                      | (gb( 1,2) << 6)
                      | (gb( 2,2) << 5)
                      | (gb( 3,2) << 4)
                      | (gb( 4,2) << 3)
                      | (gb( 5,2) << 2)
                      | (gb( 6,2) << 1)
                      | (gb( 7,2));

                tiletab1[to++] = byte1;
                tiletab1[to++] = byte2;

                po += 8;
            }

            if(true)
            {
                unsigned char
                byte3 = (gb( 0,1) << 3)
                      | (gb( 1,1) << 2)
                      | (gb( 2,1) << 1)
                      | (gb( 3,1) << 0);
                unsigned char
                byte4 = (gb( 0,2) << 3)
                      | (gb( 1,2) << 2)
                      | (gb( 2,2) << 1)
                      | (gb( 3,2) << 0);
                if(!(a&1)) { byte3 *= 16; byte4 *= 16; }
                
                tiletab2[to2++] |= byte3;
                tiletab2[to2++] |= byte4;

                po += 4;
            }
            
            #undef gb
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
    
    static const char palette[] = {0,0,1,2,3,0};
    
    tiletable.resize(256 * 16);
    widths.resize(256);
    
    unsigned to=0;
    for(unsigned a=0; a<256; ++a)
    {
        vector<char> box = font8.getbox(a);

        unsigned width=0;
        while(box[width] != 1 && width < 8)++width;

        for(unsigned p=0; p<box.size(); ++p)
            if((unsigned char)box[p] < sizeof(palette))
                box[p] = palette[ static_cast<unsigned> (box[p]) ];
        
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
        
        widths[a] = width;
    }
}

unsigned insertor::GetFont12width(ctchar chronoch) const
{
    unsigned start = (0x100-get_num_chronochars());
    if(chronoch < start)
    {
        fprintf(stderr, "Error: requested chronoch=$%04X, smallest allowed=$%02X\n",
            chronoch, start);
    }
    try
    {
        return Font12.GetWidth(chronoch-start);
    }
    catch(...)
    {
        fprintf(stderr,
            "Error: Character index $%04X is too big, says the font\n",
            chronoch);
        return 0;
    }
}
