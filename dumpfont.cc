#include <string>

#include "tgaimage.hh"
#include "dumpfont.hh"
#include "settings.hh"
#include "config.hh"
#include "msgdump.hh"
#include "rommap.hh"

void Dump8x8sprites(unsigned spriteoffs, unsigned count)
{
    const string filename = WstrToAsc(GetConf("dumper", "font8fn"));
    const string what     = "8pix font";
    
    MessageBeginDumpingImage(filename, what);

    const unsigned xdim = 32;
    const unsigned ydim = (count+xdim-1)/xdim;
    
    const unsigned xpixdim = xdim*8 + (xdim+1);
    const unsigned ypixdim = ydim*8 + (ydim+1);
    
    const char palette[] = {1,2,3,4};
    const char bordercolor=0;
    
    TGAimage image(xpixdim, ypixdim, bordercolor);
    
    MarkProt(spriteoffs, count*2, what);
    
    unsigned offs = spriteoffs;
    for(unsigned a=0; a<count; ++a)
    {
        for(unsigned y=0; y<8; ++y)
        {
            unsigned xpos = (a%xdim) * (8+1) + 1;
            unsigned ypos = (a/xdim) * (8+1) + 1+y;
            
            unsigned char byte1 = ROM[offs];
            unsigned char byte2 = ROM[offs+1];
            offs += 2;
            for(unsigned x=0; x<8; ++x)
                image.PSet(xpos+x, ypos, palette
                    [((byte1 >> (7-x))&1)
                  | (((byte2 >> (7-x))&1) << 1)]);
        }
    }

    image.Save(filename, TGAimage::pal_6color);

    MessageDone();
}

void Dump12Font(unsigned begin,unsigned end,
                unsigned offs1, unsigned offs2,
                unsigned sizeoffs)
{
    const string filename = WstrToAsc(GetConf("dumper", "font12fn"));
    const string what = "12pix font";
    
    MessageBeginDumpingImage(filename, what);

    const unsigned count = (end+1) - begin;
    
    unsigned maxwidth = 12;
    
    const unsigned xdim = 32;
    const unsigned ydim = (count+xdim-1)/xdim;
    
    const unsigned xpixdim = xdim*maxwidth + (xdim+1);
    const unsigned ypixdim = ydim*maxwidth + (ydim+1);
    
    const char palette[] = {1,2,3,4};
    const char bordercolor=0;
    const char fillercolor=5;
    
    TGAimage image(xpixdim, ypixdim, bordercolor);
    
    for(unsigned a=begin; a<=end; ++a)
    {
        unsigned hioffs = offs1 + 24 * a;
        unsigned looffs = offs2 + 24 * (a >> 1);
        
        unsigned width = ROM[sizeoffs + a];
        
        if(!GetConst(VWF12_WIDTH_USE))
            width = 12;
        
        if(width > maxwidth)width = maxwidth;
        for(unsigned y=0; y<12; ++y)
        {
            unsigned xpos = ((a-begin)%xdim) * (maxwidth+1) + 1;
            unsigned ypos = ((a-begin)/xdim) * (maxwidth+1) + 1+y;
            
            unsigned char byte1 = ROM[hioffs];
            unsigned char byte2 = ROM[hioffs+1];
            unsigned char byte3 = ROM[looffs];
            unsigned char byte4 = ROM[looffs+1];
            
            hioffs += 2;
            looffs += 2;
            
            for(unsigned x=0; x<width; ++x)
            {
                if(x == 8)
                {
                    byte1 = byte3;
                    byte2 = byte4;
                    if(a&1) { byte1 = (byte1 & 15) << 4;
                              byte2 = (byte2 & 15) << 4;
                            }
                }
                
                image.PSet(xpos+x, ypos, palette
                     [((byte1 >> (7-(x&7)))&1)
                   | (((byte2 >> (7-(x&7)))&1) << 1)]);
            }
            for(unsigned x=width; x<12; ++x)
                image.PSet(xpos+x, ypos, fillercolor);
        }
    }
    
    image.Save(filename, TGAimage::pal_6color);

    MessageDone();
}
