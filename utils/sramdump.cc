#include <cstdio>
#include <string>
#include <vector>

using namespace std;

#include "ctcset.hh"

static vector<string> items;
static void LoadDict()
{
    FILE *fp = fopen("ct_eng.txt", "rt");
    if(!fp)return;
    char Buf[600];
    bool ok = false;
    while((fgets(Buf, sizeof Buf, fp)))
    {
        if(Buf[0] == '*' && Buf[1] == 'd') { ok = true; continue; }
        if(!ok) continue;
        if(Buf[0] != '$') { ok = false; continue; }
        char *s = Buf;
        while(*s && *s != ':') ++s;
        string item;
        while(*++s != ';' && *s != '\n' && *s != '\r')
            item += *s;
        items.push_back(item);
    }
    fclose(fp);
}

int main(void)
{
    unsigned base = 0x000000;
    LoadDict();
    
    while(!feof(stdin))
    {
        char Buf[16];
        if(fread(Buf, 1, 16, stdin) < 1) break;
        
        printf("$%02X:%04X  ", base>>16, (base)&65535);
        
        for(unsigned b=0; b<16; ++b)
            printf("%02X%c", (unsigned char)Buf[b], b==7?'-':' ');
        printf("  ");
        for(unsigned b=0; b<16; ++b)
        {
            unsigned char c = Buf[b];
            
            if(c < 0x20) c = '.';
            else if(c >= 0x7F && c <= 0x9F) c = '.';
            
            putchar(c);
        }
        printf("  ");
        for(unsigned b=0; b<16; ++b)
        {
            unsigned char c = Buf[b];
            
            if(c >= 0x21 && c <= 0x7F)
            {
                printf("%s", items[c-0x21].c_str());
            }
            else if(c >= 0xA0 && c <= 0xFF)
            {
                ucs4 ch = getucs4(c, cset_12pix);
                putchar(ch);
            }
            else
            {
                putchar('.');
            }
        }
        printf("\n");
        
        base += 16;
    }
    return 0;
}
