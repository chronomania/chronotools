#include <cstdio>
#include <string>
#include <vector>

using namespace std;

#include "ctcset.cc"

static vector<string> items;
static void LoadDict()
{
    FILE *fp = fopen("ct_eng.txt", "rt");
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
        while(*++s != ';')
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
            char c = isprint(Buf[b]) ? Buf[b] : '.';
            putchar(c);
        }
        printf("  ");
        for(unsigned b=0; b<16; ++b)
        {
            unsigned char c = Buf[b];
            switch(c)
            {
                case 0x21 ... 0x7F:
                    printf("%s", items[c-0x21].c_str());
                    break;
                case 0xA0 ... 0xFF:
                    putchar(getucs4(c));
                    break;
                default:
                    putchar('.');
            }
        }
        printf("\n");
        
        base += 16;
    }
    return 0;
}
