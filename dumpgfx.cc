#include "tgaimage.hh"
#include "compress.hh"
#include "msgdump.hh"
#include "rommap.hh"

#include "dumpgfx.hh"

void DumpGFX_2bit(unsigned addr,
                  unsigned xtile, unsigned ytile,
                  const string& what,
                  const string& fn)
{
    MessageBeginDumpingImage(fn, what);
    
    TGAimage result(xtile * 8, ytile * 8, 0);
    
    if(addr > 0)
    {
        MarkProt(addr&0x3FFFFF, xtile*ytile*8*8*2/8, what);
    }
    
    for(unsigned ty=0; ty<ytile; ++ty)
        for(unsigned tx=0; tx<xtile; ++tx)
        {
            for(unsigned ypos=ty*8, y=0; y<8; ++y, ++ypos)
            {
                unsigned char byte1 = ROM[addr+0];
                unsigned char byte2 = ROM[addr+1];
                
                addr += 2;
                
                for(unsigned xpos=tx*8, x=0; x<8; ++x, ++xpos)
                {
                    result.PSet(xpos, ypos,
                         ((byte1 >> (7-(x&7)))&1)
                      | (((byte2 >> (7-(x&7)))&1) << 1)
                               );
                }
            }
        }
    result.Save(fn, TGAimage::pal_4color);
    MessageDone();
}

void DumpGFX_4bit(unsigned addr,
                  unsigned xtile, unsigned ytile,
                  const string& what, 
                  const string& fn,
                  const unsigned *palette /* = NULL */)
{
    MessageBeginDumpingImage(fn, what);

    TGAimage result(xtile * 8, ytile * 8, 0);
    
    if(addr > 0)
    {
        MarkProt(addr&0x3FFFFF, xtile*ytile*8*8*4/8, what);
    }
    
    for(unsigned ty=0; ty<ytile; ++ty)
    {
        for(unsigned tx=0; tx<xtile; ++tx)
        {
            for(unsigned ypos=ty*8, y=0; y<8; ++y, ++ypos)
            {
                unsigned char byte1 = ROM[addr + 0];
                unsigned char byte2 = ROM[addr + 1];
                unsigned char byte3 = ROM[addr + 0 + 16];
                unsigned char byte4 = ROM[addr + 1 + 16];
                
                addr += 2;

                for(unsigned xpos=tx*8, x=0; x<8; ++x, ++xpos)
                {
                    result.PSet(xpos, ypos,
                         ((byte1 >> (7-(x&7)))&1)
                      | (((byte2 >> (7-(x&7)))&1) << 1)
                      | (((byte3 >> (7-(x&7)))&1) << 2)
                      | (((byte4 >> (7-(x&7)))&1) << 3)
                               );
                }
            }
            addr += 16;
        }
    }
    result.Save(fn, TGAimage::pal_16color, palette);
    MessageDone();
}

void DumpGFX_Compressed_4bit(unsigned addr,
                             unsigned xtile,
                             const string& what,
                             const string& fn,
                             const unsigned *palette /* = NULL */)
{
    vector<unsigned char> Target;
    
    unsigned origsize = Uncompress(ROM + (addr&0x3FFFFF), Target);
    unsigned size = Target.size();
    
#if 0
    fprintf(stderr, "Created %s: Uncompressed %u bytes from %u bytes...\n",
        fn.c_str(), size, origsize);
#endif
    
    MarkProt(addr&0x3FFFFF, origsize, what);
    
    unsigned char *SavedROM = ROM;
    ROM = &Target[0];
    
    unsigned ytile = (size+xtile*32-1) / (xtile*32);

    DumpGFX_4bit(0, xtile,ytile, what, fn, palette);
    
    ROM = SavedROM;
}

