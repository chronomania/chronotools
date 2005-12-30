#include <cstdio>
#include <iostream>
#include <vector>

#include "fonts.hh"
typedef unsigned char byte;
typedef unsigned short word;

//dummy, used by fonts.o
void MessageReorganizingFonts() {}
void MessageDone() {}
void MessageLinkingModules(unsigned) {}
void MessageLoadingItem(const std::string&) {}
void MessageWorking() {}

static struct VWFdata
{
    Font8data data;
    VWFdata()
    {
        data.Load("ct8fnFIv.tga");
        //data.Load("ct8fn.tga");
    }
public:
    byte operator[] (unsigned offs) const
    {
        // Simulate the memory access in SNES
        return data.GetTiles()[offs];
    }
} Data;

static struct Width
{
public:
    byte operator[] (unsigned offs) const
    {
        // Simulate the memory access in SNES
        return Data.data.GetWidth(offs);
    }
} Width;

static struct Device
{
    std::vector<byte> table;
public:
    Device(): table(8192)
    {
        static unsigned char tmp[]=
        {

0xC0,0x00,0x00,0x00,0xE0,0x40,0x00,0x00,0xB0,0x60,0x00,0x00,0x5A,0x30,0x00,0x00,0x2F,0x1A,0x00,0x00,0x12,0x0C,0x00,0x00,0x3D,0x16,0x00,0x00,0x17,0x02,0x00,0x00,0x78,0x70,0x00,0x00,0xF0,0x80,0x00,0x00,0xC3,0x87,0x00,0x00,0x63,0xF0,0x00,0x00,0x7B,0x17,0x00,0x00,0x1F,0x14,0x00,0x00,0xE3,0xF3,0x00,0x00,0xE3,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x30,0x00,0x00,0x3D,0xBB,0x00,0x00,0xFB,0x92,0x00,0x00,0xDB,0x93,0x00,0x00,0xDB,0x92,0x00,0x00,0xCD,0x8B,0x00,0x00,0xC9,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x8C,0xDE,0x00,0x00,0xFF,0x52,0x00,0x00,0xFF,0xDE,0x00,0x00,0x9C,0x10,0x00,0x00,0x8C,0xDE,0x00,0x00,0x8C,0x00,0x00,0x00,0x06,0x04,0x00,0x00,0x06,0x04,0x00,0x00,0xE6,0xF4,0x00,0x00,0xFF,0x95,0x00,0x00,0xDF,0x96,0x00,0x00,0xDF,0x95,0x00,0x00,0xDE,0x95,0x00,0x00,0x94,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xD8,0xBD,0x00,0x00,0x9E,0x04,0x00,0x00,0x1E,0x3D,0x00,0x00,0xBF,0x25,0x00,0x00,0xDE,0x9C,0x00,0x00,0x9E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCC,0xEE,0x00,0x00,0xFF,0x2A,0x00,0x00,0xFE,0xE8,0x00,0x00,0xFC,0x28,0x00,0x00,0xFC,0xE8,0x00,0x00,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x00,0x00,0x00,0x5A,0x3C,0x00,0x00,0x3C,0x7E,0x00,0x00,0xDB,0x7E,0x00,0x00,0x5A,0xFF,0x00,0x00,0xE7,0x99,0x00,0x00,0xFF,0x81,0x00,0x00,0x81,0x00,0x00,0x00,0xDF,0x95,0x00,0x00,0xDD,0x90,0x00,0x00,0xF7,0xA5,0x00,0x00,0xE7,0xC5,0x00,0x00,0xF7,0xA5,0x00,0x00,0xDF,0x95,0x00,0x00,0xDF,0x95,0x00,0x00,0x95,0x00,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xE3,0x77,0x00,0x00,0xFF,0x54,0x00,0x00,0xF7,0x47,0x00,0x00,0xE7,0x44,0x00,0x00,0xE3,0x47,0x00,0x00,0x43,0x00,0x00,0x00,0x30,0x20,0x00,0x00,0x30,0x20,0x00,0x00,0x37,0xA5,0x00,0x00,0xFD,0xA9,0x00,0x00,0xF9,0xB0,0x00,0x00,0x3C,0x28,0x00,0x00,0x36,0xAC,0x00,0x00,0x25,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xBE,0x2F,0x00,0x00,0xBF,0x29,0x00,0x00,0xFD,0xA9,0x00,0x00,0xFD,0x69,0x00,0x00,0x7E,0x2F,0x00,0x00,0xEE,0xC8,0x00,0x00,0x3C,0x28,0x00,0x00,0x28,0x10,0x00,0x00,0x33,0x7B,0x00,0x00,0xBF,0x0A,0x00,0x00,0xBF,0x7A,0x00,0x00,0xFF,0x4A,0x00,0x00,0x3F,0x7A,0x00,0x00,0x3E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0xC0,0x80,0x00,0x00,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xDB,0x00,0x00,0x00,0x66,0xDB,0x00,0x00,0xDB,0xFF,0x00,0x00,0xA5,0x7E,0x00,0x00,0xBD,0x7E,0x00,0x00,0x5A,0x3C,0x00,0x00,0x66,0x3C,0x00,0x00,0x66,0x00,0x00,0x00,0xCC,0x88,0x00,0x00,0xCC,0x88,0x00,0x00,0xED,0xCB,0x00,0x00,0xFF,0xAA,0x00,0x00,0xFF,0x9A,0x00,0x00,0xDF,0x8A,0x00,0x00,0xCD,0x8B,0x00,0x00,0x89,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x9B,0xD2,0x00,0x00,0xFB,0x52,0x00,0x00,0x7B,0x52,0x00,0x00,0x7F,0x4A,0x00,0x00,0x8C,0xC6,0x00,0x00,0x84,0x00,0x00,0x00,0x06,0x04,0x00,0x00,0x06,0x04,0x00,0x00,0x66,0xF4,0x00,0x00,0x7F,0x17,0x00,0x00,0x7E,0xF4,0x00,0x00,0xFE,0x94,0x00,0x00,0x7E,0x74,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x3D,0x00,0x00,0x1E,0x84,0x00,0x00,0xDE,0xBD,0x00,0x00,0xFF,0xA5,0x00,0x00,0xDE,0x9C,0x00,0x00,0x9E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCC,0xEE,0x00,0x00,0xFF,0x2A,0x00,0x00,0xFE,0xE8,0x00,0x00,0xFC,0x28,0x00,0x00,0xFC,0xE8,0x00,0x00,0xF8,0x00,0x00,0x00,0x06,0x04,0x00,0x00,0x04,0x00,0x00,0x00,0xE6,0xF4,0x00,0x00,0xFE,0x94,0x00,0x00,0xDE,0x94,0x00,0x00,0xDE,0x94,0x00,0x00,0xDE,0x94,0x00,0x00,0x94,0x00,0x00,0x00,0x54,0x38,0x00,0x00,0x7C,0x38,0x00,0x00,0x96,0x7C,0x00,0x00,0xFB,0x46,0x00,0x00,0xE7,0x42,0x00,0x00,0xBD,0x66,0x00,0x00,0x5A,0x3C,0x00,0x00,0x3C,0x00,0x00,0x00,0xD8,0x90,0x00,0x00,0xD8,0x90,0x00,0x00,0xF6,0xA4,0x00,0x00,0xE6,0xC4,0x00,0x00,0xF6,0xA4,0x00,0x00,0xDE,0x94,0x00,0x00,0xDB,0x93,0x00,0x00,0x93,0x00,0x00,0x00,0x30,0x20,0x00,0x00,0x36,0x2C,0x00,0x00,0xFF,0xAE,0x00,0x00,0xFE,0xA4,0x00,0x00,0xF6,0xA4,0x00,0x00,0xF6,0xA4,0x00,0x00,0xF3,0xA2,0x00,0x00,0xA2,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x67,0xF7,0x00,0x00,0x7F,0x14,0x00,0x00,0x7E,0xF4,0x00,0x00,0xFE,0x94,0x00,0x00,0x7E,0x74,0x00,0x00,0x7C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x19,0xBD,0x00,0x00,0xDF,0x85,0x00,0x00,0xDF,0xBD,0x00,0x00,0xFF,0xA5,0x00,0x00,0xDF,0x9D,0x00,0x00,0x9F,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xCE,0xEF,0x00,0x00,0xFF,0x29,0x00,0x00,0xBD,0x29,0x00,0x00,0xBD,0x29,0x00,0x00,0xCE,0xEF,0x00,0x00,0xCE,0x08,0x00,0x00,0x60,0x40,0x00,0x00,0x40,0x00,0x00,0x00,0x60,0x40,0x00,0x00,0xE0,0x40,0x00,0x00,0xE0,0x40,0x00,0x00,0xE0,0x40,0x00,0x00,0x60,0x40,0x00,0x00,0x40,0x00,0x00,0x00,

        };

        memcpy(&table[0], tmp, sizeof(tmp));
    
        /*
        std::FILE *fp=std::fopen("00700008192.sd2", "rb");
        std::fread(&table[0],1,8192,fp);
        std::fclose(fp);
        */
    }
    byte& operator[] (unsigned offs) { return table[offs]; }
} Device;

static byte Translate(char c)
{
    byte base = 0xA0;
    if(c >= 'A' && c <= 'Z') return c-'A'+base;
    if(c >= 'a' && c <= 'z') return c-'a'+base+26;
    if(c >= '0' && c <= '9') return c-'0'+base+26+26;
    return 0xFF;
}

// 76543210 76543210
// ********
//  ******* *
//   ****** **
//    ***** ***

static void Draw(byte c)
{
    static word memaddr = 0;
    static byte pixoffs = 0;
    
    word tilebase = c*16;

    unsigned memtmp = memaddr;
    
    for(unsigned y=0; y<16; ++y)
    {
        word data = Data[tilebase++];
        // The next switch reads: data <<= (8-pixoffs);
        if(!y) std::cout << ' ' << (8-pixoffs) << " shifts:";
        std::cout << ' ' << std::hex << data;
        switch(pixoffs)
        {
            case 0: data <<= 1;
            case 1: data <<= 1;
            case 2: data <<= 1;
            case 3: data <<= 1;
            case 4: data <<= 1;
            case 5: data <<= 1;
            case 6: data <<= 1;
            case 7: data <<= 1;
        }
        Device[memtmp+16] |= data & 255;
        Device[memtmp++]  |= data >> 8;
    }
    std::cout << "; ";
    pixoffs += Width[c];
    std::cout << "pos(+" << (unsigned)Width[c] << ") = " << (unsigned)pixoffs;
    if(pixoffs >= 8)
    {
        pixoffs -= 8;
        memaddr += 16;
        std::cout << " -> " << (unsigned)pixoffs;
    }
    std::cout << "\n";
}

static void Draw(const char *s)
{
    memset(&Device[0], 0, 16*90*3);
    
    while(*s)
        Draw(Translate(*s++));
}

static void DumpDevice()
{
    // This won't be asm, so write as you wish
    
    unsigned bitness = 4;
    
    unsigned wid = 90, maxy = 3;
    
    for(unsigned l=0; l<maxy; ++l)
    for(unsigned y=0; y<8; ++y)
    {
        unsigned xpos = l*wid;
        for(unsigned x=0; x<wid; ++x,++xpos)
        {
            unsigned merkki = xpos/8;
            
            byte shift = (7-(x&7));
            byte value = 0;
            
            unsigned ptr = merkki*8*bitness + y*bitness;
            for(unsigned b=0; b<bitness; ++b)
            {
                byte b1 = Device[ptr++];
                byte bit = (b1 >> shift) & 1;
                
                value += bit << b;
            }
            
            switch(bitness)
            {
                case 2:
                    std::cout << (" .c#"[value]);
                    break;
                case 4:
                    std::cout << (" .c#456789ABCDEF"[value]);
                    break;
            }
        }
        std::cout << "\n";
    }
}

int main(void)
{
    DumpDevice();
    Draw("Riimumiekka");
    DumpDevice();
}
