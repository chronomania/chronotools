#include <cstdio>
#include "o65.hh"

int main(void)
{
    O65 tmp;
    
    FILE *fp = fopen("vwf8.o65", "rb");
    tmp.Load(fp);
    fclose(fp);
    
    const vector<unsigned char>& code = tmp.GetCode();
    
    printf("Code size: %u bytes\n", code.size());
    
    printf(" Write_4bit is at %06X\n",
        tmp.GetSymAddress(AscToWstr("Write_4bit"))
          );
    
    printf(" PrepareEndX is at %06X\n",
        tmp.GetSymAddress(AscToWstr("PrepareEndX"))
          );
    
    tmp.LocateCode(0xFF0000);
    
    printf(" Write_4bit is at %06X\n",
        tmp.GetSymAddress(AscToWstr("Write_4bit"))
          );

    printf(" PrepareEndX is at %06X\n",
        tmp.GetSymAddress(AscToWstr("PrepareEndX"))
          );

    tmp.LinkSym(AscToWstr("WIDTH_SEG"), 0xFF);
    
    tmp.Verify();
}
