#include <string>

#include "settings.hh"
#include "rommap.hh"

struct Tile
{
    unsigned char data[12][12];
    unsigned width;
};

static Tile Tiles[0x300];

static std::string Dict[0x100];

static void DumpFont(unsigned begin,unsigned end, unsigned offs1, unsigned offs2, unsigned sizeoffs)
{
    for(unsigned a=begin; a<=end; ++a)
    {
        unsigned hioffs = offs1 + 24 * a;
        unsigned looffs = offs2 + 24 * (a >> 1);
        
        Tile tmp;
        
        if(GetConst(VWF12_WIDTH_USE))
            tmp.width = ROM[sizeoffs + a];
        else
            tmp.width = 12;
        
        for(unsigned y=0; y<12; ++y)
        {
            unsigned char byte1 = ROM[hioffs];
            unsigned char byte2 = ROM[hioffs+1];
            unsigned char byte3 = ROM[looffs];
            unsigned char byte4 = ROM[looffs+1];
            
            hioffs += 2;
            looffs += 2;
            
            if(a&1) { byte3 = (byte3&15)<<4; byte4 = (byte4&15)<<4; }
            
            for(unsigned x=0; x<8; ++x)
            {
                tmp.data[y][x] =
                      ((byte1 >> (7-(x&7)))&1)  
                   | (((byte2 >> (7-(x&7)))&1) << 1);
            }
            for(unsigned x=0; x<4; ++x)
            {
                tmp.data[y][x+8] =
                      ((byte3 >> (7-(x&7)))&1)  
                   | (((byte4 >> (7-(x&7)))&1) << 1);
            }
        }
        Tiles[a] = tmp;
    }
}

static void Load12Font()
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
    
    DumpFont(0,0x2FF, FontPtr1, FontPtr2, WidthPtr);
}                                   

static void LoadDict(unsigned offs, unsigned len)
{
    for(unsigned a=0; a<len; ++a)
    {
        unsigned ptr = (ROM[offs+a*2] + 256*ROM[offs+a*2+1] + (offs&0xFF0000));
        offs += 2;
        
        std::string result;
        for(unsigned clen = ROM[ptr++]; clen > 0; --clen)
        {
            char c = ROM[ptr++];
            result += c;
            if(c == 1 || c == 2) result += ROM[ptr++];
        }

        fprintf(stderr, "$%02X:", a+0x21);
        for(unsigned a=0; a<result.size(); ++a)
            fprintf(stderr, " %02X", (unsigned char)result[a]);
        fprintf(stderr, "\n");

        Dict[a+0x21] = result;
    }
}
                                    
static void LoadDict()
{   
    unsigned DictPtr = ROM[GetConst(DICT_OFFSET)+0]
                    + (ROM[GetConst(DICT_OFFSET)+1] << 8)
                   + ((ROM[GetConst(DICT_SEGMENT1)] & 0x3F) << 16);

    unsigned char UpperLimit = ROM[GetConst(CSET_BEGINBYTE)];
    
    fprintf(stderr, "Dictionary end byte for this ROM is $%02X...\n", UpperLimit);

    unsigned dictsize = UpperLimit-0x21;  // For A0, that is 127.
    
    LoadDict(DictPtr, dictsize);
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

FILE *scriptout;
int main(int argc, const char* const* argv)
{
    SelectJAPconst();

    LoadROM(argv[1]);
    
    Load12Font();
    LoadDict();
    
    return 0;
}
