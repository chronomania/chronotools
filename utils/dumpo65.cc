#include <cstdio>
#include <vector>
#include <string>

using namespace std;

#define word(x) x=fgetc(fp);x|=fgetc(fp)<<8
#define sword(x) do{word(x);if(use32){x|=fgetc(fp)<<16;x|=fgetc(fp)<<24;}}while(0)

namespace
{
    unsigned mode;
    unsigned tbase, tlen;
    unsigned dbase, dlen;
    unsigned bbase, blen;
    unsigned zbase, zlen;
    unsigned stack;
    
    bool use32;

    vector<unsigned char> text, data;

    unsigned num_undefs;
    unsigned num_extrns;
    
    FILE *fp;
    
    void Relocations(const vector<unsigned char>& segment, unsigned base)
    {
        int addr = -1;
        for(;;)
        {
            int c = fgetc(fp);
            if(!c || c == EOF)break;
            if(c == 255)
            {
                addr += 254;
                continue;
            }
            addr += c;
            c = fgetc(fp);
            unsigned type = c & 0xE0;
            unsigned seg  = c & 0x07;
            
            if(seg == 0)
            {
                unsigned apu;
                word(apu);
                printf("   Extern = %04X:\n", apu);
            }
            
            unsigned actual = addr - base;
            if(actual >= segment.size()-3) actual = 0;

            switch(type)
            {
                case 0x20:
                {
                    unsigned value = segment[actual] << 8;
                    printf("Segid=%u, type 20; 16.LO [%04X]=%04X\n", seg, addr, value);
                    break;
                }
                case 0x40:
                {
                    unsigned apu = fgetc(fp);
                    unsigned value = (segment[actual] << 8) | apu;
                    printf("Segid=%u, type 40; 16.HI [%04X]=%04X\n", seg, addr, value);
                    break;
                }
                case 0x80:
                {
                    unsigned value = (*(unsigned *)&segment[actual])&0xFFFF;
                    printf("Segid=%u, type 80; 16    [%04X]=%04X\n", seg, addr, value);
                    break;
                }
                case 0xA0:
                {
                    unsigned value = (*(unsigned *)&segment[actual])&0xFFFFFF;
                    unsigned apu; word(apu);
                    printf("Segid=%u, type A0; 24.SEG[%04X]=%06X,%04X\n", seg, addr, value,apu);
                    break;
                }
                case 0xC0:
                {
                    unsigned value = (*(unsigned *)&segment[actual])&0xFFFFFF;
                    printf("Segid=%u, type C0; 24    [%04X]=%06X\n", seg, addr, value);
                    break;
                }
                default:
                {
                    printf("Segid=%u, type %02X at %04X\n", seg, type, addr);
                }
            }
        }
    }
}

int main(int /*argc*/, const char *const *argv)
{
    if(!argv[1] || !(fp = fopen(argv[1], "rb")))
    {
        perror("param error");
        return -1;
    }
    
    fseek(fp, 6, SEEK_SET);
    
    word(mode);
    
    use32 = mode & 0x2000;
    
    sword(tbase);  sword(tlen);
    sword(dbase);  sword(dlen);
    sword(bbase);  sword(blen);
    sword(zbase);  sword(zlen);
    sword(stack);
    
    text.resize(tlen);
    data.resize(dlen);
    
    for(;;)
    {
        unsigned len = fgetc(fp);
        if(!len) break;
        
        unsigned type = fgetc(fp);
        vector<unsigned char> value(--len);
        for(unsigned a=0; a<len; ++a)
            value[a] = fgetc(fp);

        printf("value %u: ", type); 
        for(unsigned a=0; a<len; ++a) putchar(value[a]);
        printf("\n");
    }
    
    printf("text is %u @ $%04X\n", tlen, tbase);
    printf("data is %u @ $%04X\n", dlen, dbase);
    printf("bss  is %u @ $%04X%s\n", blen, bbase, blen?" (huh?)":"");
    printf("zero is %u @ $%04X%s\n", zlen, zbase, zlen?" (huh?)":"");
    
    if(tlen) fread(&text[0], tlen, 1, fp);
    if(dlen) fread(&data[0], dlen, 1, fp);
    
    sword(num_undefs);
    printf("%u externs:\n", num_undefs);
    for(unsigned a=0; a<num_undefs; ++a)
    {
        string varname;
        while(int c = fgetc(fp)) varname += (char) c;
        printf("extern %u: %s\n", a, varname.c_str());
    }
    
    printf("--text--\n");
    Relocations(text, tbase);
    printf("--data--\n");
    Relocations(data, dbase);
    
    sword(num_extrns);
    printf("%u globals:\n", num_extrns);
    for(unsigned a=0; a<num_extrns; ++a)
    {
        string varname;
        while(int c = fgetc(fp)) varname += (char) c;
        
        unsigned seg = fgetc(fp);
        unsigned value;
        sword(value);
        
        printf("global: %s (seg %u, value %04X)\n",
            varname.c_str(), seg, value);
    }
    fclose(fp);
}
