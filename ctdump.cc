#include <cstdio>
#include <vector>
#include <string>
#include <map>
#include <unistd.h>

/* I am sorry for the spaghettiness of this small program... */

using namespace std;

#include "ctcset.hh"
#include "settings.hh"
#include "rommap.hh"
#include "strload.hh"
#include "tgaimage.hh"
#include "symbols.hh"
#include "config.hh"
#include "extras.hh"
#include "ctdump.hh"
#include "compress.hh"

static ucs4string dict[256];

static const char scriptoutfile[] = "ctdump.out";

FILE *scriptout;

static const ucs4string Disp8Char(ctchar k)
{
    const Symbols::revtype &symbols = Symbols.GetRev(2);
    
    Symbols::revtype::const_iterator i = symbols.find(k);
    
    if(i != symbols.end())
        return i->second;

    if(k == 0x2D) return AscToWstr(":");
        
    ucs4 tmp = getucs4(k, cset_8pix);
    if(tmp == ilseq)
    {
        char Buf[32];
        sprintf(Buf, "[%02X]", k);
        return AscToWstr(Buf);
    }
    
    ucs4string result;
    result += tmp;
    return result;
}

static const ucs4string Disp12Char(ctchar k)
{
    // Override these special ones to get proper formatting:
    switch(k)
    {
        case 0x05: return AscToWstr("[nl]\n");
        case 0x06: return AscToWstr("[nl]\n   ");
        case 0x07: return AscToWstr("[pausenl]\n");
        case 0x08: return AscToWstr("[pausenl]\n   ");
        case 0x09: return AscToWstr("\n[cls]\n");
        case 0x0A: return AscToWstr("\n[cls]\n   ");
        case 0x0B: return AscToWstr("\n[pause]\n");
        case 0x0C: return AscToWstr("\n[pause]\n   ");
    }
    
    const Symbols::revtype &symbols = Symbols.GetRev(16);
    
    Symbols::revtype::const_iterator i = symbols.find(k);
    
    if(i != symbols.end())
        return i->second;

    // Note:
    // The character names
    // (Crono,Marle,Lucca,Robo,Frog,Ayla,Magus,Nadia,Epoch)
    // are quite obfuscated in the ROM. We use hardcoded
    // symbol map instead of trying to decipher the ROM.
    
    if(k < 256 && !dict[k].empty())
        return dict[k];

    ucs4 tmp = getucs4(k, cset_12pix);
    if(tmp == ilseq)
    {
        char Buf[32];
        sprintf(Buf, "[%02X]", k);
        return AscToWstr(Buf);
    }

    ucs4string result;
    result += tmp;
    return result;
}

static void DumpZStrings(const unsigned offs, unsigned len, bool dolf=true)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    
    vector<ctstring> strings = LoadZStrings(offs, len, Extras_12);

    fprintf(scriptout, "*z;%u pointerstrings (12pix font)\n", strings.size());

    wstringOut conv(getcharset());    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        string line = conv.putc('$');
        const unsigned noffs = offs + a*2;
        for(unsigned k=62*62*62; ; k/=62)
        {
            unsigned dig = (noffs/k)%62;
            if(dig < 10) line += conv.putc('0' + dig);
            else if(dig < 36) line += conv.putc('A' + (dig-10));
            else line += conv.putc('a' + (dig-36));
            if(k==1)break;
        }
        line += conv.putc(':');
        if(dolf) line += conv.putc('\n');
        
        const ctstring &s = strings[a];

        for(unsigned b=0; b<s.size(); ++b)
        {
            switch(s[b])
            {
                case 0x03:
                {
                    char Buf[64];
                    sprintf(Buf, "[delay %02X]", (unsigned char)s[++b]);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 0x12:
                {
                    char Buf[64];
                    ++b;
                    if(s[b] == 0x00)
                        strcpy(Buf, "[tech]");
                    else if(s[b] == 0x01)
                        strcpy(Buf, "[monster]");
                    else
                        sprintf(Buf, "[12][%02X]", (unsigned char)s[b]);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                default:
                {
                    line += conv.puts(Disp12Char(s[b]));
                }
            }
        }
        fprintf(scriptout, "%s\n", line.c_str());
    }
    fprintf(scriptout, "\n\n");
    fprintf(stderr, " done\n");
}

static void Dump8Strings(const unsigned offs, unsigned len=0)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    vector<ctstring> strings = LoadZStrings(offs, len, Extras_8);

    fprintf(scriptout, "*r;%u pointerstrings (8pix font)\n", strings.size());

    wstringOut conv(getcharset());    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        string line = conv.putc('$');
        const unsigned noffs = offs + a*2;
        for(unsigned k=62*62*62; ; k/=62)
        {
            unsigned dig = (noffs/k)%62;
            if(dig < 10) line += conv.putc('0' + dig);
            else if(dig < 36) line += conv.putc('A' + (dig-10));
            else line += conv.putc('a' + (dig-36));
            if(k==1)break;
        }
        line += conv.putc(':');
        
        ctstring s = strings[a];

        unsigned attr=0;
        for(unsigned b=0; b<s.size(); ++b)
        {
            char Buf[64];
Retry:
            switch(s[b])
            {
                case 1:
                {
                    line += conv.puts(AscToWstr("[next]"));
                    break;
                }
                case 2:
                {
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[goto,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 3:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[func1,%04X,%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 4:
                {
                    unsigned c1, c2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c1 = (unsigned char)s[++b];
                    
                    if(c1 >= 0xC0)
                    {
                        unsigned addr = ((c1&0x3F) << 16) + c2;
                        
                        ctstring str = LoadZString(addr, Extras_8);
                        
                        //line += conv.putc(AscToWchar('{')); //}
                        
                        b -= 3;
                        s.erase(b, 4);
                        s.insert(b, str);
                        goto Retry;
                    }
                    
                    sprintf(Buf, "[substr,%02X%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 5:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[member,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 6:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[attrs,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 7:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[out,%04X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 8:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[spc,%02X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 9:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[len,%02X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 10:
                {
                    char Buf[64];
                    unsigned c;
                    c = (unsigned char)s[++b];
                    sprintf(Buf, "[attr,%02X]", c);
                    line += conv.puts(AscToWstr(Buf));
                    attr = c;
                    break;
                }
                case 11:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[func2,%04X,%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                case 12:
                {
                    unsigned c1, c2;
                    c1 = (unsigned char)s[++b];
                    c2 = (unsigned char)s[b+1] + 256*(unsigned char)s[b+2]; b+=2;
                    sprintf(Buf, "[stat,%02X,%04X]", c1,c2);
                    line += conv.puts(AscToWstr(Buf));
                    break;
                }
                default:
                    if(attr & 0x03)
                    {
                        line += conv.puts(AscToWstr("[gfx"));
                        for(;;)
                        {
                            unsigned char byte = s[b];
                            if(byte <= 12) { --b; break; }

                            char Buf[32];
                            sprintf(Buf, ",%02X", byte);
                            line += conv.puts(AscToWstr(Buf));
                            ++b;
                        }
                        line += conv.putc(AscToWchar(']'));
                    }
                    else
                    {
                        line += conv.puts(Disp8Char(s[b]));
                    }
            }
        }
        fprintf(scriptout, "%s\n", line.c_str());
    }

    fprintf(scriptout, "\n\n");
    fprintf(stderr, " done\n");
}

static void DumpFStrings(unsigned offs, unsigned len, unsigned maxcount=0)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    vector<ctstring> strings = LoadFStrings(offs, len, maxcount);
    fprintf(scriptout,
        "*l%u;%u fixed length strings (length: %u bytes)\n",
        len, strings.size(), len);

    wstringOut conv(getcharset());

    for(unsigned a=0; a<strings.size(); ++a)
    {
        string line = conv.putc('$');
        const unsigned noffs = offs + a*len;
        for(unsigned k=62*62*62; ; k/=62)
        {
            unsigned dig = (noffs/k)%62;
            if(dig < 10) line += conv.putc('0' + dig);
            else if(dig < 36) line += conv.putc('A' + (dig-10));
            else line += conv.putc('a' + (dig-36));
            if(k==1)break;
        }
        line += conv.putc(':');
        
        const ctstring &s = strings[a];

        for(unsigned b=0; b<s.size(); ++b)
            line += conv.puts(Disp8Char(s[b]));

        fprintf(scriptout, "%s\n", line.c_str());
    }
    fprintf(scriptout, "\n\n");
    fprintf(stderr, " done\n");
}

static void LoadDict(unsigned offs, unsigned len)
{
    vector<ctstring> strings = LoadPStrings(offs, len);
    fprintf(scriptout,
        "*d;dictionary (%u substrings)\n",
        strings.size());

    wstringOut conv(getcharset());

    for(unsigned a=0; a<strings.size(); ++a)
    {
        char Buf[64];
        sprintf(Buf, "$%u:", a);
        
        string line = conv.puts(AscToWstr(Buf));
        
        const ctstring &s = strings[a];

        for(unsigned b=0; b<s.size(); ++b)
            line += conv.puts(Disp8Char(s[b]));

        fprintf(scriptout, "%s\n", line.c_str());
    }
    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        const ctstring &s = strings[a];
        ucs4string tmp;
        for(unsigned b=0; b<s.size(); ++b)tmp += getucs4(s[b], cset_12pix);
        dict[a + 0x21] = tmp;
    }

    fprintf(scriptout, "\n\n");
}

static void Dump8x8sprites(unsigned spriteoffs, unsigned count)
{
    fprintf(stderr, "Dumping 8pix font...");
    const unsigned xdim = 32;
    const unsigned ydim = (count+xdim-1)/xdim;
    
    const unsigned xpixdim = xdim*8 + (xdim+1);
    const unsigned ypixdim = ydim*8 + (ydim+1);
    
    const char palette[] = {1,2,3,4};
    const char bordercolor=0;
    
    TGAimage image(xpixdim, ypixdim, bordercolor);
    
    MarkProt(spriteoffs, count*2);
    
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

    image.Save(WstrToAsc(GetConf("dumper", "font8fn")), TGAimage::pal_6color);

    fprintf(stderr, " done\n");
}

static void DumpFont(unsigned begin,unsigned end, unsigned offs1, unsigned offs2, unsigned sizeoffs)
{
    fprintf(stderr, "Dumping 12pix font...");
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
    
    image.Save(WstrToAsc(GetConf("dumper", "font12fn")), TGAimage::pal_6color);

    fprintf(stderr, " done\n");
}

static void Dump12Font()
{
    unsigned char A0 = ROM[FirstChar_Address];
    
    unsigned Offset = ROM[WidthTab_Offset_Addr];
    if(Offset == 0x20) Offset = 0; // ctfin puts $20 here (sep $20 instead of sbc $A0)
    
    unsigned WidthPtr = ROM[WidthTab_Address_Ofs+0]
                     + (ROM[WidthTab_Address_Ofs+1]<<8)
                     + ((ROM[WidthTab_Address_Seg] & 0x3F) << 16)
                     - Offset;
    
    unsigned FontSeg = ROM[Font12_Address_Seg] & 0x3F;
    unsigned FontPtr1 = ROM[Font12a_Address_Ofs+0]
                     + (ROM[Font12a_Address_Ofs+1] << 8)
                     + (FontSeg << 16);
    unsigned FontPtr2 = ROM[Font12b_Address_Ofs+0]
                     + (ROM[Font12b_Address_Ofs+1] << 8)
                     + (FontSeg << 16);
    
    if(FontPtr2 != FontPtr1 + 0x1800)
    {
        fprintf(stderr,
            "Warning: This isn't the original ROM.\n"
            "The high part of 12pix font isn't immediately after the low part:\n"
            "%06X != %06X+1800\n", FontPtr2, FontPtr1);
    }
    
    DumpFont(0,0x2FF, FontPtr1, FontPtr2, WidthPtr);
    
    MarkFree(FontPtr1, 256*24);
    MarkFree(FontPtr2, 256*12);
    
    MarkFree(WidthPtr+A0, 0x100-A0);
}

static void DoLoadDict()
{
    unsigned DictPtr = ROM[DictAddr_Ofs+0]
                    + (ROM[DictAddr_Ofs+1] << 8)
                   + ((ROM[DictAddr_Seg_1] & 0x3F) << 16);

    unsigned char A0 = ROM[FirstChar_Address];
    
    fprintf(stderr, "Dictionary end byte for this ROM is $%02X...\n", A0);

    unsigned dictsize = A0-0x21;  // For A0, that is 127.
    
    LoadDict(DictPtr, dictsize);

    // unprotect the dictionary because it will be relocated.
    UnProt(DictPtr, dictsize*2);
    
    MarkFree(DictPtr, dictsize*2);
}

static void LoadROM(const char *fn)
{
    FILE *fp = fopen(fn, "rb");
    if(!fp)
    {
        perror(fn);
        return;
    }
    LoadROM(fp);
    fclose(fp);
}

static void DumpGFX_2bit(unsigned addr, unsigned xtile, unsigned ytile, const string &fn)
{
    TGAimage result(xtile * 8, ytile * 8, 0);
    
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
}

static void DumpGFX_4bit(unsigned addr, unsigned xtile, unsigned ytile, const string &fn,
                         const unsigned *palette = NULL)
{
    TGAimage result(xtile * 8, ytile * 8, 0);
    
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
}

static void DumpGFX_Compressed_4bit
    (unsigned addr, unsigned xtile,
     const string &fn,
     const unsigned *palette = NULL)
{
    vector<unsigned char> Target;
    
    unsigned origsize = Uncompress(ROM + (addr&0x3FFFFF), Target);
    unsigned size = Target.size();
    
    fprintf(stderr, "Created %s: Uncompressed %u bytes from %u bytes...\n",
        fn.c_str(), size, origsize);
    
    unsigned char *SavedROM = ROM;
    ROM = &Target[0];
    
    unsigned ytile = (size+xtile*32-1) / (xtile*32);

    DumpGFX_4bit(0, xtile,ytile, fn, palette);
    
    ROM = SavedROM;
}

static void DumpGFX()
{
{   static const unsigned pal[16] = {
0x000000, //000000 - 50
0xFFFFFF, //0A0A0A - 51
0x8C8A8C, //020201 - 52
0x52515A, //050505 - 54
0x212021, //060502 - 55
0xA5A2AD, //070707 - 56
0xFF0000, //0B0804 - 57
0x735229, //0E0A05 - 58
0x836231, //100C06 - 59
0x8B7B62, //110F0C - 5A
0x949494, //121212 - 5B
0x9C7B41, //130F08 - 5C
0xFF00FF, //1F001F - 5D
0xFF00FF, //1F001F - 5E
0xFF00FF, //1F001F - 5F
};
    DumpGFX_Compressed_4bit(0xFE6002, 16, "titlegfx.tga", pal);
}

    DumpGFX_Compressed_4bit(0xC5DA88, 16, "pontpo.tga");

{   static const unsigned pal[16] = {
0x000000, //000000 - 80
0x293931, //050706 - 81
0x293931, //050706 - 82
0x293931, //050706 - 83
0x293931, //050706 - 84
0x293931, //050706 - 85
0x293931, //050706 - 86
0xC5C5C5, //181818 - 87
0x83838B, //101011 - 88
0x52525A, //0A0A0B - 89
0x313139, //060607 - 8A
0x080810, //010102 - 8B
0x293931, //050706 - 8C
0x293931, //050706 - 8D
0x293931, //050706 - 8E
0x293931, //050706 - 8F
};
    /* FIXME: duplicate tiles */
    DumpGFX_Compressed_4bit(0xC38000, 19, "eraes.tga", pal);
}
    DumpGFX_2bit(0x3FF488,  6, 2, "active2.tga"); // "Active Time Battle ver. 2"

{   static const unsigned pal[16] = {
0x000010,0x000010,0x101818,0x202031,
0x29314A,0x41415A,0x525262,0x5A5A73,
0x5A6283,0x737394,0x838B9C,0x8B94AC,
0xA4A4B4,0xB4B4CD,0xCDCDDE,0xF6F6FF };
    DumpGFX_4bit(0x3FF008, 12, 3, "active1.tga", pal); // "Battle Mode"
}

    DumpGFX_2bit(0x114C80,12, 1, "misc1.tga");
    DumpGFX_2bit(0x3FC9FC,13, 2, "misc2.tga");
    DumpGFX_4bit(0x3FE4CC, 2,32, "misc3.tga");

{   static const unsigned pal[16] = {
0x000000,0x392010,0x394139,0x6A5A41,
0x837B62,0x623108,0xA44100,0xC5B47B,
0xCD8B39,0xCDAC52,0xDE9410,0xF6CD41,
0xFFFF6A,0xFFFFC5,0xFFFFFF,0x291018 };
    DumpGFX_4bit(0x3FD5E4,  6, 2, "elem1.tga", pal); //lightning
}

{   static const unsigned pal[16] = {
0x000000,0x000010,0x000031,0x100052,
0x20086A,0x202031,0x313152,0x39208B,
0x4A4162,0x524183,0x524A94,0x5A41B4,
0x6A629C,0x7B62CD,0x8B7BD5,0x000000 };
    DumpGFX_4bit(0x3FD764,  6, 2, "elem2.tga", pal); //shadow
}

{   static const unsigned pal[16] = {
0x000000,0x0029CD,0x0041FF,0x005AFF,
0x007BFF,0x00A4FF,0x00C5FF,0x1073FF,
0x20BDFF,0x5A73FF,0x6AA4FF,0x6AD5FF,
0xB4C5FF,0xB4EEFF,0x00299C,0xFFFFFF };
    DumpGFX_4bit(0x3FD8E4,  6, 2, "elem3.tga", pal); //water
}
    
{   static const unsigned pal[16] =
{
0x000000,0x080000,0x180808,0x181010,
0x291810,0x312020,0x413129,0x4A2918,
0x521808,0x523129,0x5A4141,0x6A3110,
0x942900,0xA44100,0xB42900,0xBD4A00 };
    DumpGFX_4bit(0x3FDA64,  6, 2, "elem4.tga", pal); //fire
}

{   static const unsigned pal[16] =
{
0x8B8B8B,0xD5D5D5,0xB4BDC5,
0xFFC5AC,0xF69C7B,0xC5735A,0x9C5A41,
0x734A29,0xDE9410,0xC56A00,0x41B483,
0x204131,0xA44110,0x6A1000,0x391008 };
    DumpGFX_4bit(0x3F0000+0*6*6*32,  6, 6, "face1.tga", pal);
};

{   static const unsigned pal[16] =
{
0xA429D5,0xFFEEE6,0xEEBDA4,
0xE6A48B,0xC59473,0xB48352,0x946A4A,
0x734139,0x41C55A,0xD5A462,0xD58331,
0xC56210,0xC53900,0x318341,0x412008, 0x000000 };
    DumpGFX_4bit(0x3F0000+1*6*6*32,  6, 6, "face2.tga", pal);
};

{   static const unsigned pal[16] =
{
0x8B8B8B,0xF6F6EE,0xF6D5CD,
0xF6B4B4,0xD5948B,0xB47329,0xB46A4A,
0x7B5A20,0x5A3908,0x392008,0x8B3900,
0xE65241,0x107B8B,0xC55200,0x7B4A18, 0x000000 };
    DumpGFX_4bit(0x3F0000+2*6*6*32,  6, 6, "face3.tga", pal);
};

{   static const unsigned pal[16] =
{
0x949494,0xF6E6E6,0xE6D5A4,
0xDEB49C,0xDE9C62,0xAC7B52,0x834A10,
0x5A3100,0x412010,0x9C949C,0x7B737B,
0x62FFB4,0x299C73,0x7B5A39,0x5A4129, 0x000000 };
    DumpGFX_4bit(0x3F0000+3*6*6*32,  6, 6, "face4.tga", pal);
};

{   static const unsigned pal[16] =
{
0x949494,0x000000,0xFFFFF6,
0xEEEEBD,0xD5949C,0xD5DE83,0x9CBD31,
0xE6A420,0xBD5A41,0x837373,0x9C6A31,
0x629400,0x622918,0x414120,0x313110, 0x000000 };
    DumpGFX_4bit(0x3F0000+4*6*6*32,  6, 6, "face5.tga", pal);
};

{   static const unsigned pal[16] =
{
0x8B8B8B,0xFFF6E6,0xFFD5C5,
0xE6DE7B,0xC5B462,0xFFB494,0xEE947B,
0xA49462,0xB48352,0x946241,0x735231,
0x734141,0x5A2920,0x627B94,0xEE5A7B, 0x000000 };
    DumpGFX_4bit(0x3F0000+5*6*6*32,  6, 6, "face6.tga", pal);
};

{   static const unsigned pal[16] =
{
0x207362,0x390810,0xE6E6E6,
0xD5C5B4,0xB4A494,0x8B7B6A,0x625A41,
0xB494C5,0x6283A4,0x7373C5,0x525294,
0x203194,0xB4296A,0x5A1020,0x4A3929, 0x000000 };
    DumpGFX_4bit(0x3F0000+6*6*6*32,  6, 6, "face7.tga", pal);
};

    DumpGFX_4bit(0x3FEB88,  6, 6, "face8.tga");
}

int main(int argc, const char* const* argv)
{
    fprintf(stderr,
        "Chrono Trigger script dumper version "VERSION"\n"
        "Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)\n");
    
    if(argc != 2)
    {
        fprintf(stderr,
            "Usage: ctdump romfilename > scriptfilename\n");
        return -1;
    }
    
    LoadROM(argv[1]);
    
    DumpGFX();
    
    scriptout = fopen(scriptoutfile, "wt");
    
    fprintf(scriptout,
        "; Note: There is a one byte sequence for [nl] and three spaces.\n"
        ";       Don't attempt to save space by removing those spaces,\n"
        ";       you will only make things worse...\n"
        ";       Similar for [pause]s, [pausenl]s and [cls]s.\n"
        ";\n"
          );
    
    DoLoadDict();
    
    // 
    fprintf(scriptout, ";items\n");
    DumpFStrings(0x0C0B5E, 11, 242);
    fprintf(scriptout, ";item descriptions\n");
    DumpZStrings(0x0C2EB1, 242, false);
    
    fprintf(scriptout, ";techs\n");
    DumpFStrings(0x0C15C4, 11);
    fprintf(scriptout, ";tech descs\n");
    DumpZStrings(0x0C3A09, 117, false);
    
    fprintf(scriptout, ";tech related\n");
    DumpZStrings(0x0C3AF3, 4, false);
    
    fprintf(scriptout, ";monsters\n");
    DumpFStrings(0x0C6500, 11); // monsters

    fprintf(scriptout, ";Weapon Helmet Armor Accessory\n");
    DumpFStrings(0x02A3BA, 10, 4);

    fprintf(scriptout, ";Location titles\n");
    DumpZStrings(0x06F400, 112, false);
    
    fprintf(scriptout, ";Battle announcements\n");
    DumpZStrings(0x0EEF11, 14, false);
    
    fprintf(scriptout, ";Battle messages\n");
    DumpZStrings(0x0CCBC9, 227, false);
    
    fprintf(scriptout, ";600ad (castle, masa+mune, naga-ette)\n");
    fprintf(scriptout, ";12kbc daltonstuff\n");
    DumpZStrings(0x18D000, 78);
    
    fprintf(scriptout, ";65Mbc\n");
    DumpZStrings(0x18DD80, 254);
    
    fprintf(scriptout, ";2300ad (factory, sewer, belthasar)\n");
    fprintf(scriptout, ";65Mbc azalastuff\n");
    fprintf(scriptout, ";slideshow-ending\n");
    DumpZStrings(0x1EC000, 187);
    
    fprintf(scriptout, ";1000ad (towns, castle)\n");
    fprintf(scriptout, ";600ad (towns)\n");
    fprintf(scriptout, ";2300ad (factory)\n");
    DumpZStrings(0x1EE300, 145);
    
    fprintf(scriptout, ";Treasure box messages\n");
    DumpZStrings(0x1EFF00, 3, false);

    fprintf(scriptout, ";1000ad (Lucca's home)\n");
    fprintf(scriptout, ";2300ad (factory)\n");
    fprintf(scriptout, ";1000ad (rainbow shell trial)    \n");
    DumpZStrings(0x36A000, 106);

    fprintf(scriptout, ";no Crono -ending\n");
    fprintf(scriptout, ";happy ending (castle)\n");
    DumpZStrings(0x36B230, 144);

    fprintf(scriptout, ";1000ad (various indoors)\n");
    fprintf(scriptout, ";600ad (various indoors)\n");
    fprintf(scriptout, ";2300ad (various indoors)\n");
    DumpZStrings(0x370000, 456);

    fprintf(scriptout, ";2300ad (various indoors)\n");
    fprintf(scriptout, ";end of time (gaspar's stories, Spekkio etc)\n");
    fprintf(scriptout, ";600ad (Ozzie's scenes, Magus battle, castle)\n");
    fprintf(scriptout, ";1999ad Lavos scenes\n");
    fprintf(scriptout, ";12kbc various scenes\n");
    fprintf(scriptout, ";1000ad castle scenes\n");
    DumpZStrings(0x374900, 1203);

    fprintf(scriptout, ";1000ad jail scenes\n");
    fprintf(scriptout, ";600ad bridge battle stuff\n");
    fprintf(scriptout, ";1000ad (melchior, medina village)\n");
    fprintf(scriptout, ";65Mbc\n");
    fprintf(scriptout, ";12kbc\n");
    fprintf(scriptout, ";600ad (Toma stuff, Marco&Fiona stuff)\n");
    fprintf(scriptout, ";1000ad (Cyrus stuff)\n");
    fprintf(scriptout, ";Black Omen\n");
    DumpZStrings(0x384650, 678);

    fprintf(scriptout, ";600ad (Cathedral, other indoors)\n");
    fprintf(scriptout, ";12kbc (out- and indoors)\n");
    DumpZStrings(0x39B000, 444);

    fprintf(scriptout, ";1000ad (fair, the trial, castle)\n");
    fprintf(scriptout, ";600ad (Frog scenes)\n");
    fprintf(scriptout, ";12kbc\n");
    fprintf(scriptout, ";2300ad (death's peak)\n");
    DumpZStrings(0x3CBA00, 399);

    fprintf(scriptout, ";Dreamteam etc\n");
    fprintf(scriptout, ";Forest scene\n");
    DumpZStrings(0x3F4460, 81);
    
    fprintf(scriptout, ";12kbc cities\n");
    DumpZStrings(0x3F5860, 85);

    fprintf(scriptout, ";?\n");
    DumpZStrings(0x3F6B00, 186);

    fprintf(scriptout, ";Battle tutorials, Zeal stuff, party stuff\n");
    DumpZStrings(0x3F8400, 39);
    
    fprintf(scriptout, ";Item types\n");
    Dump8Strings(0x3FB310, 242);

    fprintf(scriptout, ";Status screen strings\n");
    
    Dump8Strings(0x3FC457, 64);
    
    fprintf(scriptout, ";Misc prompts\n");
    DumpZStrings(0x3FCF3B, 7);
    
    fprintf(scriptout, ";Configuration\n");
    DumpZStrings(0x3FD3FE, 50, false);
    
    fprintf(scriptout, ";Era list\n");
    Dump8Strings(0x3FD396, 8);
    
    fprintf(scriptout, ";Episode list\n");
    DumpZStrings(0x3FD03E, 27, false);
    
    Dump8x8sprites(Font8_Address, 256);
    
    Dump12Font();
    
    FindEndSpaces();

    ListSpaces();
    
    fclose(scriptout);
    
    ShowProtMap();
    
    fprintf(stderr, "Done\n");
    
    scriptout = fopen(scriptoutfile, "rt");
    for(;;)
    {
        char Buf[4096];
        if(!fgets(Buf, sizeof Buf, scriptout)) break;
        printf("%s", Buf);
    }
    fclose(scriptout);
    remove(scriptoutfile);
    
    return 0;
}
