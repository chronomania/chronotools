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

static ucs4string dict[256];

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

    printf("*z;%u pointerstrings (12pix font)\n", strings.size());

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
        puts(line.c_str());
    }
    printf("\n\n");
    fprintf(stderr, " done\n");
}

static void Dump8Strings(const unsigned offs, unsigned len=0)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    vector<ctstring> strings = LoadZStrings(offs, len, Extras_8);

    printf("*r;%u pointerstrings (8pix font)\n", strings.size());

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
        puts(line.c_str());
    }

    printf("\n\n");
    fprintf(stderr, " done\n");
}

static void DumpFStrings(unsigned offs, unsigned len, unsigned maxcount=0)
{
    fprintf(stderr, "Dumping strings at %06X...", offs);
    vector<ctstring> strings = LoadFStrings(offs, len, maxcount);
    printf("*l%u;%u fixed length strings (length: %u bytes)\n",
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
        puts(line.c_str());
    }
    printf("\n\n");
    fprintf(stderr, " done\n");
}

static void LoadDict(unsigned offs, unsigned len)
{
    vector<ctstring> strings = LoadPStrings(offs, len);
    printf("*d;dictionary (%u substrings)\n",
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
        puts(line.c_str());
    }
    
    for(unsigned a=0; a<strings.size(); ++a)
    {
        const ctstring &s = strings[a];
        ucs4string tmp;
        for(unsigned b=0; b<s.size(); ++b)tmp += getucs4(s[b], cset_12pix);
        dict[a + 0x21] = tmp;
    }

    printf("\n\n");
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

    image.Save(WstrToAsc(GetConf("dumper", "font8fn")));

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
    
    image.Save(WstrToAsc(GetConf("dumper", "font12fn")));

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
            "    %06X != %06X+1800\n", FontPtr2, FontPtr1);
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


int main(int argc, const char* const* argv)
{
    fprintf(stderr,
        "Chrono Trigger script dumper version "VERSION"\n"
        "Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)\n");
    
    if(argc != 2)
    {
        fprintf(stderr, "Usage: ctdump romfilename > scriptfilename\n");
        return -1;
    }
    
    LoadROM(argv[1]);

    printf("; Note: There is a one byte sequence for [nl] and three spaces.\n"
           ";       Don't attempt to save space by removing those spaces,\n"
           ";       you will only make things worse...\n"
           ";       Similar for [pause]s, [pausenl]s and [cls]s.\n"
           ";\n"
          );
    
    DoLoadDict();
    
    // 
    puts(";items");
    DumpFStrings(0x0C0B5E, 11, 242);
    puts(";item descriptions");
    DumpZStrings(0x0C2EB1, 242, false);
    
    puts(";techs");
    DumpFStrings(0x0C15C4, 11);
    puts(";tech descs");
    DumpZStrings(0x0C3A09, 117, false);
    
    puts(";tech related");
    DumpZStrings(0x0C3AF3, 4, false);
    
    puts(";monsters");
    DumpFStrings(0x0C6500, 11); // monsters

    puts(";Weapon Helmet Armor Accessory");
    DumpFStrings(0x02A3BA, 10, 4);

    puts(";Location titles");
    DumpZStrings(0x06F400, 112, false);
    
    puts(";Battle announcements");
    DumpZStrings(0x0EEF11, 14, false);
    
    puts(";Battle messages");
    DumpZStrings(0x0CCBC9, 227, false);
    
    puts(";600ad (castle, masa+mune, naga-ette)");
    puts(";12kbc daltonstuff");
    DumpZStrings(0x18D000, 78);
    
    puts(";65Mbc");
    DumpZStrings(0x18DD80, 254);
    
    puts(";2300ad (factory, sewer, belthasar)");
    puts(";65Mbc azalastuff");
    puts(";slideshow-ending");
    DumpZStrings(0x1EC000, 187);
    
    puts(";1000ad (towns, castle)");
    puts(";600ad (towns)");
    puts(";2300ad (factory)");
    DumpZStrings(0x1EE300, 145);
    
    puts(";Treasure box messages");
    DumpZStrings(0x1EFF00, 3, false);

    puts(";1000ad (Lucca's home)");
    puts(";2300ad (factory)");
    puts(";1000ad (rainbow shell trial)    ");
    DumpZStrings(0x36A000, 106);

    puts(";no Crono -ending");
    puts(";happy ending (castle)");
    DumpZStrings(0x36B230, 144);

    puts(";1000ad (various indoors)");
    puts(";600ad (various indoors)");
    puts(";2300ad (various indoors)");
    DumpZStrings(0x370000, 456);

    puts(";2300ad (various indoors)");
    puts(";end of time (gaspar's stories, Spekkio etc)");
    puts(";600ad (Ozzie's scenes, Magus battle, castle)");
    puts(";1999ad Lavos scenes");
    puts(";12kbc various scenes");
    puts(";1000ad castle scenes");
    DumpZStrings(0x374900, 1203);

    puts(";1000ad jail scenes");
    puts(";600ad bridge battle stuff");
    puts(";1000ad (melchior, medina village)");
    puts(";65Mbc");
    puts(";12kbc");
    puts(";600ad (Toma stuff, Marco&Fiona stuff)");
    puts(";1000ad (Cyrus stuff)");
    puts(";Black Omen");
    DumpZStrings(0x384650, 678);

    puts(";600ad (Cathedral, other indoors)");
    puts(";12kbc (out- and indoors)");
    DumpZStrings(0x39B000, 444);

    puts(";1000ad (fair, the trial, castle)");
    puts(";600ad (Frog scenes)");
    puts(";12kbc");
    puts(";2300ad (death's peak)");
    DumpZStrings(0x3CBA00, 399);

    puts(";Dreamteam etc");
    puts(";Forest scene");
    DumpZStrings(0x3F4460, 81);
    
    puts(";12kbc cities");
    DumpZStrings(0x3F5860, 85);

    puts(";?");
    DumpZStrings(0x3F6B00, 186);

    puts(";Battle tutorials, Zeal stuff, party stuff");
    DumpZStrings(0x3F8400, 39);
    
    puts(";Item types");
    Dump8Strings(0x3FB310, 242);

    puts(";Status screen strings");
    
    Dump8Strings(0x3FC457, 61);
    
    Dump8Strings(0x3FC4D3, 2);
    
    puts(";Misc prompts");
    DumpZStrings(0x3FCF3B, 7);
    
    puts(";Configuration");
    DumpZStrings(0x3FD3FE, 50, false);
    
    puts(";Era list");
    Dump8Strings(0x3FD396, 8);
    
    puts(";Episode list");
    DumpZStrings(0x3FD03E, 27, false);
    
    Dump8x8sprites(Font8_Address, 256);
    
    Dump12Font();
    
    FindEndSpaces();

    ListSpaces();
    
    ShowProtMap();
    
    fprintf(stderr, "Done\n");
}
