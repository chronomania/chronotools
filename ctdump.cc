#include <cstdio>
#include <string>

/* I am sorry for the spaghettiness of this small program... */

using namespace std;

#include "rommap.hh"
#include "settings.hh"
#include "scriptfile.hh"

#include "dumptext.hh"
#include "dumpfont.hh"
#include "dumpgfx.hh"
#include "msgdump.hh"

static const char scriptoutfile[] = "ctdump.out";

/* static due to namespace conflict */
static void LoadDict()
{
    unsigned DictPtr = ROM[GetConst(DICT_OFFSET)+0]
                    + (ROM[GetConst(DICT_OFFSET)+1] << 8)
                   + ((ROM[GetConst(DICT_SEGMENT1)] & 0x3F) << 16);

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
    
    MarkFree(DictPtr, dictsize*2, "dictionary pointers");
}

/* static due to namespace conflict */
static void Dump12Font()
{
    unsigned char A0 = ROM[GetConst(CSET_BEGINBYTE)];
    
    unsigned Offset = ROM[GetConst(VWF12_WIDTH_INDEX)];
    if(Offset == 0x20) Offset = 0; // ctfin puts $20 here (sep $20 instead of sbc $A0)
    
    unsigned WidthPtr = ROM[GetConst(VWF12_WIDTH_OFFSET)+0]
                     + (ROM[GetConst(VWF12_WIDTH_OFFSET)+1]<<8)
                     + ((ROM[GetConst(VWF12_WIDTH_SEGMENT)] & 0x3F) << 16)
                     - Offset;
    
    unsigned FontSeg = ROM[GetConst(VWF12_SEGMENT)] & 0x3F;
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
    
    const string what = "12pix font";
    
    Dump12Font(0,0x2FF, FontPtr1, FontPtr2, WidthPtr);
    
    // FIXME: Is it all really free??
    MarkFree(FontPtr1, 256*24, what+" part 1");
    MarkFree(FontPtr2, 256*12, what+" part 2");
    
    // FIXME: 0x100 is not the upper limit in all roms!
    MarkFree(WidthPtr+A0, 0x100-A0, what+" width table");
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
        DumpGFX_Compressed_4bit(0xFE6002, 16, "title screen gfx", "titlegfx.tga", pal);
    }

        DumpGFX_Compressed_4bit(0xC5DA88, 16, "worldmap gfx 1", "pontpo.tga");

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
        DumpGFX_Compressed_4bit(0xC38000, 19, "epoch control gfx", "eraes.tga", pal);
    }
        DumpGFX_2bit(0x3FF488,  6, 2, "batlmode prompt gfx 2", "active2.tga"); // "Active Time Battle ver. 2"

    {   static const unsigned pal[16] = {
    0x000010,0x000010,0x101818,0x202031,
    0x29314A,0x41415A,0x525262,0x5A5A73,
    0x5A6283,0x737394,0x838B9C,0x8B94AC,
    0xA4A4B4,0xB4B4CD,0xCDCDDE,0xF6F6FF };
        // in japanese it is FFE83C
        DumpGFX_4bit(0x3FF008, 12, 3, "batlmode prompt gfx 1", "active1.tga", pal); // "Battle Mode"
    }

        DumpGFX_2bit(0x114C80,12, 1, "symbol gfx 1", "misc1.tga");
        DumpGFX_2bit(0x3FC9FC,13, 2, "symbol gfx 2", "misc2.tga");
        DumpGFX_4bit(0x3FE4CC, 2,16, "symbol gfx 3", "misc3.tga");

    {   static const unsigned pal[16] = {
    0x000000,0x392010,0x394139,0x6A5A41,
    0x837B62,0x623108,0xA44100,0xC5B47B,
    0xCD8B39,0xCDAC52,0xDE9410,0xF6CD41,
    0xFFFF6A,0xFFFFC5,0xFFFFFF,0x291018 };
        DumpGFX_4bit(0x3FD5E4,  6, 2, "elemental symbol 1", "elem1.tga", pal); //lightning
    }

    {   static const unsigned pal[16] = {
    0x000000,0x000010,0x000031,0x100052,
    0x20086A,0x202031,0x313152,0x39208B,
    0x4A4162,0x524183,0x524A94,0x5A41B4,
    0x6A629C,0x7B62CD,0x8B7BD5,0x000000 };
        DumpGFX_4bit(0x3FD764,  6, 2, "elemental symbol 2", "elem2.tga", pal); //shadow
    }

    {   static const unsigned pal[16] = {
    0x000000,0x0029CD,0x0041FF,0x005AFF,
    0x007BFF,0x00A4FF,0x00C5FF,0x1073FF,
    0x20BDFF,0x5A73FF,0x6AA4FF,0x6AD5FF,
    0xB4C5FF,0xB4EEFF,0x00299C,0xFFFFFF };
        DumpGFX_4bit(0x3FD8E4,  6, 2, "elemental symbol 3", "elem3.tga", pal); //water
    }
        
    {   static const unsigned pal[16] =
    {
    0x000000,0x080000,0x180808,0x181010,
    0x291810,0x312020,0x413129,0x4A2918,
    0x521808,0x523129,0x5A4141,0x6A3110,
    0x942900,0xA44100,0xB42900,0xBD4A00 };
        DumpGFX_4bit(0x3FDA64,  6, 2, "elemental symbol 4", "elem4.tga", pal); //fire
    }

#if 0
    {   static const unsigned pal[16] =
    {
    0x8B8B8B,0xD5D5D5,0xB4BDC5,
    0xFFC5AC,0xF69C7B,0xC5735A,0x9C5A41,
    0x734A29,0xDE9410,0xC56A00,0x41B483,
    0x204131,0xA44110,0x6A1000,0x391008 };
        DumpGFX_4bit(0x3F0000+0*6*6*32,  6, 6, "character 1 portrait", "face1.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0xA429D5,0xFFEEE6,0xEEBDA4,
    0xE6A48B,0xC59473,0xB48352,0x946A4A,
    0x734139,0x41C55A,0xD5A462,0xD58331,
    0xC56210,0xC53900,0x318341,0x412008, 0x000000 };
        DumpGFX_4bit(0x3F0000+1*6*6*32,  6, 6, "character 2 portrait", "face2.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0x8B8B8B,0xF6F6EE,0xF6D5CD,
    0xF6B4B4,0xD5948B,0xB47329,0xB46A4A,
    0x7B5A20,0x5A3908,0x392008,0x8B3900,
    0xE65241,0x107B8B,0xC55200,0x7B4A18, 0x000000 };
        DumpGFX_4bit(0x3F0000+2*6*6*32,  6, 6, "character 3 portrait", "face3.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0x949494,0xF6E6E6,0xE6D5A4,
    0xDEB49C,0xDE9C62,0xAC7B52,0x834A10,
    0x5A3100,0x412010,0x9C949C,0x7B737B,
    0x62FFB4,0x299C73,0x7B5A39,0x5A4129, 0x000000 };
        DumpGFX_4bit(0x3F0000+3*6*6*32,  6, 6, "character 4 portrait", "face4.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0x949494,0x000000,0xFFFFF6,
    0xEEEEBD,0xD5949C,0xD5DE83,0x9CBD31,
    0xE6A420,0xBD5A41,0x837373,0x9C6A31,
    0x629400,0x622918,0x414120,0x313110, 0x000000 };
        DumpGFX_4bit(0x3F0000+4*6*6*32,  6, 6, "character 5 portrait", "face5.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0x8B8B8B,0xFFF6E6,0xFFD5C5,
    0xE6DE7B,0xC5B462,0xFFB494,0xEE947B,
    0xA49462,0xB48352,0x946241,0x735231,
    0x734141,0x5A2920,0x627B94,0xEE5A7B, 0x000000 };
        DumpGFX_4bit(0x3F0000+5*6*6*32,  6, 6, "character 6 portrait", "face6.tga", pal);
    };

    {   static const unsigned pal[16] =
    {
    0x207362,0x390810,0xE6E6E6,
    0xD5C5B4,0xB4A494,0x8B7B6A,0x625A41,
    0xB494C5,0x6283A4,0x7373C5,0x525294,
    0x203194,0xB4296A,0x5A1020,0x4A3929, 0x000000 };
        DumpGFX_4bit(0x3F0000+6*6*6*32,  6, 6, "character 7 portrait", "face7.tga", pal);
    };

        DumpGFX_4bit(0x3FEB88,  6, 6, "character 8 portrait (b/w)", "face8.tga");
#endif
    }

    void DumpText()
    {
        // 
        BlockComment(";242 items (note: max length = 11 chars, [symbol] takes 1)\n");
        DumpFStrings(0x0C0B5E, "items", 11, 242);
        BlockComment(";242 item descriptions - remember to check for wrapping\n");
        DumpZStrings(0x0C2EB1, "item descs", 242, false);
        BlockComment(";242 item types (only 232 are used though)\n");
        Dump8Strings(0x3FB310, "item types", 242);

        BlockComment(";item classes (weapon, helmet, armor, accessory)\n");
        DumpFStrings(0x02A3BA, "item classes", 10, 4);

        BlockComment(";117 techniques (note: max length = 11 chars, [symbol] takes 1)\n");
        DumpFStrings(0x0C15C4, "techs", 11, 117);
        BlockComment(";117 technique descriptions - remember to check for wrapping\n");
        DumpZStrings(0x0C3A09, "tech descs", 117, false);

        BlockComment(";tech/battle related strings\n");
        DumpZStrings(0x0C3AF3, "bat misc", 4, false);
        
        BlockComment(";treasure box messages (are found elsewhere too)\n");
        DumpZStrings(0x1EFF00, "tres msg", 3, false);
        
        BlockComment(";battle menu label: double member techniques\n");
        DumpFStrings(0x0CFB4C, "bat", 16, 1);
        BlockComment(";battle menu label: triple member techniques\n");
        DumpFStrings(0x0CFB5E, "bat", 16, 1);
        BlockComment(";battle menu labels. each label is 2 lines.\n");
        DumpFStrings(0x0CFA41, "bat", 7, 12);
        
        BlockComment(";252 monster names\n");
        DumpFStrings(0x0C6500, "mons", 11, 252); // monsters

        BlockComment(";place names\n");
        DumpZStrings(0x06F400, "places", 112, false);
        
        BlockComment(";era list\n");
        DumpZStrings(0x3FD396, "eraes", 8, false);
        
        BlockComment(";episode list\n");
        DumpZStrings(0x3FD03E, "eps", 27, false);
        
        BlockComment(";battle messages, part 1 (remember to check for wrapping)\n");
        DumpZStrings(0x0EEF11, "bat", 14, false);
        
        BlockComment(";battle messages, part 2 (remember to check for wrapping)\n");
        DumpZStrings(0x0CCBC9, "bat", 227, false);
        
        BlockComment(";600ad (castle, masa+mune, naga-ette)\n"
                     ";12kbc daltonstuff\n");
        DumpZStrings(0x18D000, "dialog", 78);
        
        BlockComment(";65Mbc\n");
        DumpZStrings(0x18DD80, "dialog", 254);
        
        BlockComment(";2300ad (factory, sewer, belthasar)\n"
                     ";65Mbc azalastuff\n"
                     ";slideshow-ending\n");
        DumpZStrings(0x1EC000, "dialog", 187);
        
        BlockComment(";1000ad (towns, castle)\n"
                     ";600ad (towns)\n"
                     ";2300ad (factory)\n");
        DumpZStrings(0x1EE300, "dialog", 145);

        BlockComment(";1000ad (Lucca's home)\n" 
                     ";2300ad (factory)\n"
                     ";1000ad (rainbow shell trial)\n");
        DumpZStrings(0x36A000, "dialog", 106);

        BlockComment(";no Crono -ending\n"
                     ";happy ending (castle)\n");
        DumpZStrings(0x36B230, "dialog", 144);

        BlockComment(";1000ad (various indoors)\n"
                     ";600ad (various indoors)\n"
                     ";2300ad (various indoors)\n");
        DumpZStrings(0x370000, "dialog", 456);

        BlockComment(";2300ad (various indoors)\n"
                     ";end of time (gaspar's stories, Spekkio etc)\n"
                     ";600ad (Ozzie's scenes, Magus battle, castle)\n"
                     ";1999ad Lavos scenes\n"
                     ";12kbc various scenes\n"
                     ";1000ad castle scenes\n");
        DumpZStrings(0x374900, "dialog", 1203);

        BlockComment(";1000ad jail scenes\n"
                     ";600ad bridge battle stuff\n"
                     ";1000ad (melchior, medina village)\n"
                     ";65Mbc\n"
                     ";12kbc\n"
                     ";600ad (Toma stuff, Marco&Fiona stuff)\n"
                     ";1000ad (Cyrus stuff)\n"
                     ";Black Omen\n");
        DumpZStrings(0x384650, "dialog", 678);

        BlockComment(";600ad (Cathedral, other indoors)\n"
                     ";12kbc (out- and indoors)\n");
        DumpZStrings(0x39B000, "dialog", 444);

        BlockComment(";1000ad (fair, the trial, castle)\n"
                     ";600ad (Frog scenes)\n"
                     ";12kbc\n"
                     ";2300ad (death's peak)\n");
        DumpZStrings(0x3CBA00, "dialog", 399);

        BlockComment(";Dreamteam etc\n"
                     ";Forest scene\n");
        DumpZStrings(0x3F4460, "dialog", 81);
        
        BlockComment(";12kbc cities\n");
        DumpZStrings(0x3F5860, "dialog", 85);

        BlockComment(";Ayla's home (after the defeat of Magus)\n"
                     ";earthbound islands\n");
        DumpZStrings(0x3F6B00, "dialog", 186);

        BlockComment(";battle tutorials, Zeal stuff, fair stuff\n");
        DumpZStrings(0x3F8400, "dialog", 39);
        
        BlockComment(";all kind of screens - be careful when editing.\n" 
                     ";These are the special symbols used here:\n"
                     ";  [next]      = jumps to the next column\n"
                     ";  [goto,w]    = jumps to the specified display address\n"
                     ";  [func1,w,w] = displays a number\n"
                     ";  [substr,p]  = inserts a substring from given address\n"
                     ";  [member,w]  = displays a member name from given address\n"
                     ";  [attrs,w]   = loads display attributes from given addr\n"
                     ";  [out,w]     = displays a symbol from given address\n"
                     ";  [spc,b]     = outputs given amount of spaces\n"
                     ";  [len,b]     = configures the column width\n"
                     ";  [attr,b]    = sets new display attributes\n"
                     ";  [func2,w,w] = unused? not sure\n"
                     ";  [stat,b,w]  = displays a stat from address\n"
                     ";  [gfx,b,...] = raw bytes\n");
        Dump8Strings(0x3FC457, "screens", 62);

        BlockComment(";name input character set\n"
                     ";(note: this string ends with one space - don't erase it)\n"
                    );
        DumpFStrings(0x3FC9AC, "cset", 80, 1);

        BlockComment(";some misc prompts\n");
        DumpZStrings(0x3FCF3B, "prompts", 7);
        
        BlockComment(";configuration screen strings\n");
        DumpZStrings(0x3FD3FE, "cfg", 50, false);
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
        "; Note: There is a one byte sequence for [nl] and three spaces.\n"
        ";       Don't attempt to save space by removing those spaces,\n"
        ";       you will only make things worse...\n"
        ";       Similar for [pause]s, [pausenl]s and [cls]s.\n"
        ";\n"
        "; Note: The character names used in this script are:\n"
        ";       Crono Marle Lucca Frog Robos Ayla Magus Epoch\n"
        ";       \"Robos\" is used instead of \"Robo\" to make it possible\n"
        ";       to use words starting with \"Robo\" (like \"Robot\") without\n"
        ";       them breaking when the player changes the character names.\n"
        ";       This has no effect to the game. Do not try changing all\n"
        ";       instances of \"Robos\" to \"Robo\". It will break the game.\n"
        ";\n"
        "; Note: The order of strings (each starts with $) inside a block\n"
        ";       (each starts with *) can be changed, but they can't be moved\n"
        ";       from block to another.\n"
          );
    
    LoadDict();
    
    fprintf(stderr, "Creating %s (all text content)...\n", scriptoutfile);
    
    BlockComment(";dictionary, used for compression. don't try to translate it.\n");
    DumpDict();
    DumpFonts();
    DumpGFX();
    DumpText();
    
    FindEndSpaces();

    BlockComment(";Next comes the map of free space in the ROM.\n"
                 ";It is automatically generated by ctdump.\n"
                 ";You shouldn't edit it unless you know what you're doing.\n"
                 ";It is required by the insertor. The insertor uses this information\n"
                 ";to know where to put the data.\n");
    ListSpaces();

    PutAscii(";end of free space list\n");
    
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
