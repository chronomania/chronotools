#include <cstdio>
#include <string>

/* I am sorry for the spaghettiness of this small program... */

using namespace std;

#include "rommap.hh"
#include "romaddr.hh"
#include "settings.hh"
#include "eventdata.hh"
#include "dumpevent.hh"
#include "scriptfile.hh"

#include "dumptext.hh"
#include "dumpfont.hh"
#include "dumpgfx.hh"
#include "msgdump.hh"
#include "miscfun.hh"
#include "config.hh"

static const char scriptoutfile[] = "ctdump.out";

/* static due to namespace conflict */
static void LoadDict()
{
    unsigned DictPtr = ROM[GetConst(DICT_OFFSET)+0]
                    + (ROM[GetConst(DICT_OFFSET)+1] << 8)
                    + (ROM[GetConst(DICT_SEGMENT1)] << 16);
    DictPtr = SNES2ROMaddr(DictPtr);

    unsigned char UpperLimit = ROM[GetConst(CSET_BEGINBYTE)];
    
    fprintf(stderr, "Dictionary end byte for this ROM is $%02X...\n", UpperLimit);

    unsigned dictsize = UpperLimit-0x21;  // For A0, that is 127.
    
    /*
     * Note: In the English ROM, dict item 0xA0 is "...", which is never used.
     *       It is not loaded here, and not marked freed.
     */
    LoadDict(DictPtr, dictsize);

    // unprotect the dictionary because it will be relocated.
    UnProt(DictPtr, dictsize*2);
    
    MarkFree(DictPtr, dictsize*2, L"dictionary pointers");
}

/* static due to namespace conflict */
static void Dump12Font()
{
    unsigned char A0 = ROM[GetConst(CSET_BEGINBYTE)];
    
    unsigned Offset = ROM[GetConst(VWF12_WIDTH_INDEX)];
    if(Offset == 0x20) Offset = 0; // ctfin puts $20 here (sep $20 instead of sbc $A0)
    
    unsigned WidthPtr = ROM[GetConst(VWF12_WIDTH_OFFSET)+0]
                     + (ROM[GetConst(VWF12_WIDTH_OFFSET)+1]<<8)
                     + (ROM[GetConst(VWF12_WIDTH_SEGMENT)] << 16)
                     - Offset;
    WidthPtr = SNES2ROMaddr(WidthPtr);
    
    unsigned FontSeg = SNES2ROMpage(ROM[GetConst(VWF12_SEGMENT)]);
    unsigned FontPtr1 = ROM[GetConst(VWF12_TAB1_OFFSET)+0]
                     + (ROM[GetConst(VWF12_TAB1_OFFSET)+1] << 8)
                     + (FontSeg << 16);
    unsigned FontPtr2 = ROM[GetConst(VWF12_TAB2_OFFSET)+0]
                     + (ROM[GetConst(VWF12_TAB2_OFFSET)+1] << 8)
                     + (FontSeg << 16);
    
#if 0
    /* Nobody understands this message anyway. */
    if(FontPtr2 != FontPtr1 + 0x1800)
    {
        fprintf(stderr,
            "Warning: This isn't the original ROM.\n"
            "The high part of 12pix font isn't immediately after the low part:\n"
            "%06X != %06X+1800\n", FontPtr2, FontPtr1);
    }
#endif
    
    const std::wstring what = L"12pix font";
    
    Dump12Font(0,0x2FF, FontPtr1, FontPtr2, WidthPtr);
    
    // FIXME: Is it all really free??
    MarkFree(FontPtr1, 256*24, what + L" part 1");
    MarkFree(FontPtr2, 256*12, what + L" part 2");
    
    // FIXME: 0x100 is not the upper limit in all roms!
    MarkFree(WidthPtr+A0, 0x100-A0, what + L" width table");
}

/* static due to namespace conflict */
static void Dump8x8sprites()
{
    Dump8x8sprites(GetConst(TILETAB_8_ADDRESS), 256);
}

namespace
{
    void LoadROM(const char *fn)
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
    
    void DumpGFX()
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
        DumpGFX_Compressed_4bit(0x3E6002, 16, L"title screen gfx", "titlegfx.tga", pal);
    }

        DumpGFX_Compressed_4bit(0x05DA88, 16, L"worldmap gfx 1", "pontpo.tga");

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
        DumpGFX_Compressed_4bit(0x038000, 19, L"epoch control gfx", "eraes.tga", pal);
    }
        DumpGFX_2bit(0x3FF488,  6, 2, L"batlmode prompt gfx 2", "active2.tga"); // "Active Time Battle ver. 2"

    {   static const unsigned pal[16] = {
    0x000010,0x000010,0x101818,0x202031,
    0x29314A,0x41415A,0x525262,0x5A5A73,
    0x5A6283,0x737394,0x838B9C,0x8B94AC,
    0xA4A4B4,0xB4B4CD,0xCDCDDE,0xF6F6FF };
        // in japanese it is FFE83C
        DumpGFX_4bit(0x3FF008, 12, 3, L"batlmode prompt gfx 1", "active1.tga", pal); // "Battle Mode"
    }

        DumpGFX_2bit(0x114C80,12, 1, L"symbol gfx 1", "misc1.tga");
        DumpGFX_2bit(0x3FC9FC,13, 2, L"symbol gfx 2", "misc2.tga");
        DumpGFX_4bit(0x3FE4CC, 2,16, L"symbol gfx 3", "misc3.tga");

    {   static const unsigned pal[16] = {
    0x000000,0x392010,0x394139,0x6A5A41,
    0x837B62,0x623108,0xA44100,0xC5B47B,
    0xCD8B39,0xCDAC52,0xDE9410,0xF6CD41,
    0xFFFF6A,0xFFFFC5,0xFFFFFF,0x291018 };
        DumpGFX_4bit(0x3FD5E4,  6, 2, L"elemental symbol 1", "elem1.tga", pal); //lightning
    }

    {   static const unsigned pal[16] = {
    0x000000,0x000010,0x000031,0x100052,
    0x20086A,0x202031,0x313152,0x39208B,
    0x4A4162,0x524183,0x524A94,0x5A41B4,
    0x6A629C,0x7B62CD,0x8B7BD5,0x000000 };
        DumpGFX_4bit(0x3FD764,  6, 2, L"elemental symbol 2", "elem2.tga", pal); //shadow
    }

    {   static const unsigned pal[16] = {
    0x000000,0x0029CD,0x0041FF,0x005AFF,
    0x007BFF,0x00A4FF,0x00C5FF,0x1073FF,
    0x20BDFF,0x5A73FF,0x6AA4FF,0x6AD5FF,
    0xB4C5FF,0xB4EEFF,0x00299C,0xFFFFFF };
        DumpGFX_4bit(0x3FD8E4,  6, 2, L"elemental symbol 3", "elem3.tga", pal); //water
    }
        
    {   static const unsigned pal[16] =
    {
    0x000000,0x080000,0x180808,0x181010,
    0x291810,0x312020,0x413129,0x4A2918,
    0x521808,0x523129,0x5A4141,0x6A3110,
    0x942900,0xA44100,0xB42900,0xBD4A00 };
        DumpGFX_4bit(0x3FDA64,  6, 2, L"elemental symbol 4", "elem4.tga", pal); //fire
    }

#if 0
    {   static const unsigned pal[16] =
    {
    0x8B8B8B,0xD5D5D5,0xB4BDC5,
    0xFFC5AC,0xF69C7B,0xC5735A,0x9C5A41,
    0x734A29,0xDE9410,0xC56A00,0x41B483,
    0x204131,0xA44110,0x6A1000,0x391008 };
        DumpGFX_4bit(0x3F0000+0*6*6*32,  6, 6, L"character 1 portrait", "face1.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0xA429D5,0xFFEEE6,0xEEBDA4,
    0xE6A48B,0xC59473,0xB48352,0x946A4A,
    0x734139,0x41C55A,0xD5A462,0xD58331,
    0xC56210,0xC53900,0x318341,0x412008, 0x000000 };
        DumpGFX_4bit(0x3F0000+1*6*6*32,  6, 6, L"character 2 portrait", "face2.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0x8B8B8B,0xF6F6EE,0xF6D5CD,
    0xF6B4B4,0xD5948B,0xB47329,0xB46A4A,
    0x7B5A20,0x5A3908,0x392008,0x8B3900,
    0xE65241,0x107B8B,0xC55200,0x7B4A18, 0x000000 };
        DumpGFX_4bit(0x3F0000+2*6*6*32,  6, 6, L"character 3 portrait", "face3.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0x949494,0xF6E6E6,0xE6D5A4,
    0xDEB49C,0xDE9C62,0xAC7B52,0x834A10,
    0x5A3100,0x412010,0x9C949C,0x7B737B,
    0x62FFB4,0x299C73,0x7B5A39,0x5A4129, 0x000000 };
        DumpGFX_4bit(0x3F0000+3*6*6*32,  6, 6, L"character 4 portrait", "face4.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0x949494,0x000000,0xFFFFF6,
    0xEEEEBD,0xD5949C,0xD5DE83,0x9CBD31,
    0xE6A420,0xBD5A41,0x837373,0x9C6A31,
    0x629400,0x622918,0x414120,0x313110, 0x000000 };
        DumpGFX_4bit(0x3F0000+4*6*6*32,  6, 6, L"character 5 portrait", "face5.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0x8B8B8B,0xFFF6E6,0xFFD5C5,
    0xE6DE7B,0xC5B462,0xFFB494,0xEE947B,
    0xA49462,0xB48352,0x946241,0x735231,
    0x734141,0x5A2920,0x627B94,0xEE5A7B, 0x000000 };
        DumpGFX_4bit(0x3F0000+5*6*6*32,  6, 6, L"character 6 portrait", "face6.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0x207362,0x390810,0xE6E6E6,
    0xD5C5B4,0xB4A494,0x8B7B6A,0x625A41,
    0xB494C5,0x6283A4,0x7373C5,0x525294,
    0x203194,0xB4296A,0x5A1020,0x4A3929, 0x000000 };
        DumpGFX_4bit(0x3F0000+6*6*6*32,  6, 6, L"character 7 portrait", "face7.tga", pal);
    };

        DumpGFX_4bit(0x3FEB88,  6, 6, L"character 8 portrait (b/w)", "face8.tga");
#endif
    }

    void DumpText()
    {
        // 
        BlockComment(L";242 items (note: max length = 11 chars, [symbol] takes 1)\n");
        DumpFStrings(0x0C0B5E, L"items", 11, 242);
        BlockComment(L";242 item descriptions - remember to check for wrapping\n");
        DumpZStrings(0x0C2EB1, L"item descs", 242, false);
        BlockComment(L";242 item types (only 232 are used though)\n");
        Dump8Strings(0x3FB310, L"item types", 242);

        BlockComment(L";item classes (weapon, helmet, armor, accessory)\n");
        DumpFStrings(0x02A3BA, L"item classes", 10, 4);

        BlockComment(L";117 techniques (note: max length = 11 chars, [symbol] takes 1)\n");
        DumpFStrings(0x0C15C4, L"techs", 11, 117);
        BlockComment(L";117 technique descriptions - remember to check for wrapping\n");
        DumpZStrings(0x0C3A09, L"tech descs", 117, false);

        BlockComment(L";tech/battle related strings\n");
        DumpZStrings(0x0C3AF3, L"bat misc", 4, false);
        
        BlockComment(L";treasure box messages (are found elsewhere too)\n");
        DumpZStrings(0x1EFF00, L"tres msg", 3, false);
        
        BlockComment(L";battle menu label: double member techniques\n");
        DumpFStrings(0x0CFB4C, L"bat", 16, 1);
        BlockComment(L";battle menu label: triple member techniques\n");
        DumpFStrings(0x0CFB5E, L"bat", 16, 1);
        BlockComment(L";battle menu labels. each label is 2 lines.\n");
        DumpFStrings(0x0CFA41, L"bat", 7, 12);
        
        BlockComment(L";252 monster names\n");
        DumpFStrings(0x0C6500, L"mons", 11, 252); // monsters

        BlockComment(L";place names\n");
        // REFERRED FROM:
        // $C2:567A A2 00 F4    LDX #$F400
        // $C2:5680 A9 C6       LDA #$C6
        
        //DumpZStrings(0x06F400, L"places", 112, false);
        DumpRZStrings(L"places", 112, false,
                      '^', 0x025681,
                      '!', 0x02567B,
                      0);
        
        BlockComment(L";era list\n");
        // REFERRED FROM:
        // $C2:D3D4 A2 96 D3    LDX #$D396
        // $C2:D3DA A9 FF       LDA #$FF
        
        //DumpZStrings(0x3FD396, L"eraes", 8, false);
        DumpRZStrings(L"eraes", 8, false,
                      '^', 0x02D3DB,
                      '!', 0x02D3D5,
                      0);
        
        BlockComment(L";episode list\n");
        DumpZStrings(0x3FD03E, L"eps", 27, false);
        
        BlockComment(L";battle messages, part 1 (remember to check for wrapping)\n");
        // REFERRED FROM:
        // $CD:01B6 A2 11 EF    LDX #$EF11
        // $CD:01BC A9 CE       LDA #$CE
        // AND:
        // $CD:02F0 A2 11 EF    LDX #$EF11
        // $CD:02F9 A9 CE       LDA #$CE
        DumpRZStrings(L"bat", 14, false,
                      '^', 0x0D01BD,
                      '!', 0x0D01B7,
                      '^', 0x0D02FA,
                      '!', 0x0D02F1,
                      0);
        
        BlockComment(L";battle messages, part 2 (remember to check for wrapping)\n");
        DumpZStrings(0x0CCBC9, L"bat", 227, false);
        
        BlockComment(L";600ad (castle, masa+mune, naga-ette)\n"
                     L";12kbc daltonstuff\n");
        DumpMZStrings(0x18D000, L"dialog", 78); //ok!
        
        BlockComment(L";65Mbc\n");
        DumpMZStrings(0x18DD80, L"dialog", 254); //ok!

        BlockComment(L";2300ad (factory, sewer, belthasar)\n"
                     L";65Mbc azalastuff\n"
                     L";slideshow-ending\n");
        DumpMZStrings(0x1EC000, L"dialog", 187); //ok!
        
        BlockComment(L";1000ad (towns, castle)\n"
                     L";600ad (towns)\n"
                     L";2300ad (factory)\n");
        DumpMZStrings(0x1EE300, L"dialog", 145); //ok!

        BlockComment(L";1000ad (Lucca's home)\n" 
                     L";2300ad (factory)\n"
                     L";1000ad (rainbow shell trial)\n");
        DumpMZStrings(0x36A000, L"dialog", 106); //ok!

        BlockComment(L";no Crono -ending\n"
                     L";happy ending (castle)\n");
        DumpMZStrings(0x36B230, L"dialog", 144); //ok!

        BlockComment(L";1000ad (various indoors)\n"
                     L";600ad (various indoors)\n"
                     L";2300ad (various indoors)\n");
        DumpMZStrings(0x370000, L"dialog", 217); //ok!

        BlockComment(L";1000ad (various indoors)\n"
                     L";600ad (various indoors)\n");
        DumpMZStrings(0x3701B2, L"dialog", 111); //ok!

        BlockComment(L";2300ad (various indoors)\n");
        DumpMZStrings(0x370290, L"dialog", 128); //ok!

        BlockComment(L";2300ad (various indoors)\n");
        DumpMZStrings(0x374900, L"dialog", 240); //ok!

        BlockComment(L";end of time (gaspar's stories, Spekkio etc)\n");
        DumpMZStrings(0x374AE0, L"dialog", 178); //ok!
        
        BlockComment(L";1999ad Lavos scenes\n");
        DumpMZStrings(0x374C44, L"dialog", 179); //ok!
        
        BlockComment(L";12kbc (blackbird scenes)\n");
        DumpMZStrings(0x374DAA, L"dialog", 187); //ok!

        BlockComment(L";600ad castle scenes\n");
        DumpMZStrings(0x374F20, L"dialog", 244); //ok!
        
        BlockComment(L";1000ad castle scenes\n");
        DumpMZStrings(0x375108, L"dialog", 175); //ok!

        BlockComment(L";1000ad jail scenes\n"
                     L";600ad bridge battle stuff\n"
                     L";1000ad (melchior, medina village)\n");
        DumpMZStrings(0x384650, L"dialog", 256); //ok!

        BlockComment(L";65Mbc\n"
                     L";12kbc\n");
        DumpMZStrings(0x384850, L"dialog", 256); //ok!

        BlockComment(L";600ad (Toma stuff, Marco&Fiona stuff)\n"
                     L";1000ad (Cyrus stuff)\n"
                     L";Black Omen\n");
        DumpMZStrings(0x384A50, L"dialog", 166); //ok!
        
        BlockComment(L";600ad (Cathedral, other indoors)\n"
                     L";12kbc (out- and indoors)\n");
        DumpMZStrings(0x39B000, L"dialog", 197); //ok!

        BlockComment(L";600ad (Cathedral, other indoors)\n"
                     L";12kbc (out- and indoors)\n");
        DumpMZStrings(0x39B18A, L"dialog", 247); //ok!

        BlockComment(L";1000ad (fair, the trial, castle)\n"
                     L";600ad (Frog scenes)\n"
                     L";12kbc\n");
        DumpMZStrings(0x3CBA00, L"dialog", 203); //ok!

        BlockComment(L";1000ad (fair, the trial, castle)\n"
                     L";600ad (Frog scenes)\n"
                     L";12kbc\n"
                     L";2300ad (death's peak)\n");
        DumpMZStrings(0x3CBB96, L"dialog", 196); //ok!
        
//        BlockComment(L";reptite ending"); // ASCII TEXT.
//        DumpMZStrings(0x3DA000, L"dialog", 256);

        BlockComment(L";Dreamteam etc\n"
                     L";Forest scene\n");
        DumpMZStrings(0x3F4460, L"dialog", 81); //ok!
        
        BlockComment(L";12kbc cities\n");
        DumpMZStrings(0x3F5860, L"dialog", 85); //ok!

        BlockComment(L";Ayla's home (after the defeat of Magus)\n"
                     L";earthbound islands\n");
        DumpMZStrings(0x3F6B00, L"dialog", 186); //ok!

        BlockComment(L";battle tutorials, Zeal stuff, fair stuff\n");
        DumpMZStrings(0x3F8400, L"dialog", 39); //ok!
        
        BlockComment(L";all kind of screens - be careful when editing.\n" 
                     L";These are the special symbols used here:\n"
                     L";  [next]      = jumps to the next column\n"
                     L";  [goto,w]    = jumps to the specified display address\n"
                     L";  [func1,w,w] = displays a number\n"
                     L";  [substr,p]  = inserts a substring from given address\n"
                     L";  [member,w]  = displays a member name from given address\n"
                     L";  [attrs,w]   = loads display attributes from given addr\n"
                     L";  [out,w]     = displays a symbol from given address\n"
                     L";  [spc,b]     = outputs given amount of spaces\n"
                     L";  [len,b]     = configures the column width\n"
                     L";  [attr,b]    = sets new display attributes\n"
                     L";  [func2,w,w] = unused? not sure\n"
                     L";  [stat,b,w]  = displays a stat from address\n"
                     L";  [gfx,b,...] = raw bytes\n");
        Dump8Strings(0x3FC457, L"screens", 62);

        BlockComment(L";name input character set\n"
                     L";(note: this string ends with one space - don't erase it)\n"
                    );
        DumpFStrings(0x3FC9AC, L"cset", 80, 1);

        BlockComment(L";some misc prompts\n");
        DumpZStrings(0x3FCF3B, L"prompts", 7);
        
        BlockComment(L";configuration screen strings\n");
        DumpZStrings(0x3FD3FE, L"cfg", 50, false);
        
        BlockComment(L";This block initializes some of the game RAM.\n"
                     L";It is compressed as whole, and thus requires the whole\n"
                     L";data... The important bit you may want to edit is the\n"
                     L";character names. If you edit them, please keep them\n"
                     L";exactly 6 bytes long, last byte being [0].\n"
                     L";Imaginary examples:\n"
                     L";   Robo[0][0]    ->  Macho[0]\n"
                     L";   Magus[0]      ->  Ma[0][0][0][0]\n"
                    );
        DumpC8String(0x3CFA35, L"statusram");
    }
    
    void DumpEvents()
    {
        const ConfParser::ElemVec& elems = GetConf("dumper", "dump_events").Fields();
        for(unsigned a=0; a<elems.size(); a += 1)
        {
            unsigned evno = elems[a];
            
            /* Ignore non-existing events */
            if(evno >= 0x66 && evno <= 0x6E) continue;
            if(evno >= 0xE7 && evno <= 0xEB) continue;
            if(evno >=0x140 && evno <=0x142) continue;
            /* Ignore broken event (can not be dumped) */
            if(evno == 0x176) continue;
            
            std::string comment = format(";*** Event %s\n", LocationEventNames[evno]);
            std::string evname  = format("event_%03X", evno);
            
            BlockComment(AscToWstr(comment));
            MessageBeginDumpingEvent(evno);
            DumpEvent(GetConst(EVENT_TABLE_ADDR) + evno*3, AscToWstr(evname));
            MessageDone();
        }
    }

    void DumpFonts()
    {
        Dump8x8sprites();
        Dump12Font();
    }
}

int main(int argc, const char* const* argv)
{
    SelectENGconst();
    
    fprintf(stderr,
        "Chrono Trigger script dumper version "VERSION"\n"
        "Copyright (C) 1992,2004 Bisqwit (http://iki.fi/bisqwit/)\n");
    
    if(argc != 2)
    {
        fprintf(stderr,
            "Usage: ctdump romfilename\n"
            "Will place output (the script) in %s\n", scriptoutfile
        );
        return -1;
    }
    
    LoadROM(argv[1]);
    
    OpenScriptFile(scriptoutfile);

    PutAscii(
        L"; Note: There is a one byte sequence for [nl] and three spaces.\n"
        L";       Don't attempt to save space by removing those spaces,\n"
        L";       you will only make things worse...\n"
        L";       Similar for [pause]s, [pausenl]s and [cls]s.\n"
        L";\n"
        L"; Note: The character names used in this script are:\n"
        L";       Crono Marle Lucca Frog Robos Ayla Magus Epoch\n"
        L";       \"Robos\" is used instead of \"Robo\" to make it possible\n"
        L";       to use words starting with \"Robo\" (like \"Robot\") without\n"
        L";       them breaking when the player changes the character names.\n"
        L";       This has no effect to the game. Do not try changing all\n"
        L";       instances of \"Robos\" to \"Robo\". It will break the game.\n"
        L";\n"
        L"; Note: The order of strings (each starts with $) inside a block\n"
        L";       (each starts with *) can be changed, but they can't be moved\n"
        L";       from block to another.\n"
        L";\n"
        L";\n"
        L"; All supported label types:\n"
        L";  *d    = dict\n"
        L";          entries are: $hexnumber:value\n"
        L";          hexnumber is ignored; only the order matters\n"
        L";  *sNN  = free space record (at page NN)\n"
        L";          entries are: $hexnum1:hexnum2\n"
        L";          hexnum1 marks beginning, hexnum2 marks ending\n"
        L";  *z    = dialog string (12pix zstring)\n"
        L";          entries are: $base62num:value\n"
        L";          base62num refers to the address of the pointer\n"
        L";  *r    = status string (8pix zstring)\n"
        L";          entries are: $base62num:value\n"
        L";          base62num refers to the address of the pointer\n"
        L";  *lNN  = fixed width = NN\n"
        L";          entries are: $base62num:value\n"
        L";          base62num refers to the address of the value\n"
        L";  *iNN  = relocatable item table (original width = NN)\n"
        L";          same meaning as *l                          \n"
        L";  *tNN  = relocatable tech table (original width = NN)\n"
        L";          same meaning as *l\n"
        L";  *mNN  = relocatable monster table (original width = NN)\n"
        L";          same meaning as *l\n"
        L";  *c    = compressed blob, may contain 8pix text\n"
        L";  *eNN  = location event - pointed from NN (base62)\n"
        L"\n"
          );
    
    LoadDict();
    
    fprintf(stderr, "Creating %s (all text content)...\n", scriptoutfile);
    
    BlockComment(L";dictionary, used for compression. don't try to translate it.\n");

    DumpDict();
    DumpFonts();
    DumpGFX();
    DumpText();
    DumpEvents();
    
    FindEndSpaces();

    BlockComment(L";Next comes the map of free space in the ROM.\n"
                 L";It is automatically generated by ctdump.\n"
                 L";You shouldn't edit it unless you know what you're doing.\n"
                 L";It is required by the insertor. The insertor uses this information\n"
                 L";to know where to put the data.\n");
    ListSpaces();

    PutAscii(L";end of free space list\n");
    
    CloseScriptFile();
    
    ShowProtMap();
    
    fprintf(stderr, "%-60s\n", "All done");
    
#if 0
    scriptout = fopen(scriptoutfile, "rt");
    for(;;)
    {
        char Buf[4096];
        if(!fgets(Buf, sizeof Buf, scriptout)) break;
        printf("%s", Buf);
    }
    fclose(scriptout);
    remove(scriptoutfile);
#endif
    
    return 0;
}
