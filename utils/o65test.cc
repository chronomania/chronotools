#include <cstdio>
#include "o65.hh"

using std::printf;

int main(void)
{
    O65 tmp;
    
    FILE *fp = fopen("ct-vwf8.o65", "rb");
    tmp.Load(fp);
    fclose(fp);
    
    const vector<unsigned char>& code = tmp.GetCode();
    
    printf("Code size: %u bytes\n", code.size());
    
    printf(" Write_4bit is at %06X\n", tmp.GetSymAddress("Write_4bit"));
    printf(" NextTile is at %06X\n", tmp.GetSymAddress("NextTile"));
    
    printf("After relocating code at FF0000:\n");
    
    tmp.LocateCode(0xFF0000);
    
    printf(" Write_4bit is at %06X\n", tmp.GetSymAddress("Write_4bit"));
    printf(" NextTile is at %06X\n", tmp.GetSymAddress("NextTile"));
    
    tmp.LinkSym("WIDTH_SEG", 0xFF);
    
    tmp.Verify();
    
    return 0;
}
