#include <cstdio>
#include <iostream>
#include <string>
using namespace std;

static void ConvPtrTo62(const string &s)
{
    cout << "Pointer \"" << s << "\" = ";
    const char *q = s.c_str();
    unsigned value=0;
    for(; *q; ++q)
    {
        if(*q >= '0' && *q <= '9') value = value*16 + *q - '0';
        else if(*q >= 'a' && *q <= 'f') value = value*16 + *q - ('a'-10);
        else if(*q >= 'A' && *q <= 'F') value = value*16 + *q - ('A'-10);
    }
    for(unsigned k=62*62*62; ; k/=62)
    {
        unsigned dig = (value/k)%62;
        if(dig < 10) cout << (char)('0' + dig);
        else if(dig < 36) cout << (char)('A' + (dig-10));
        else cout << (char)('a' + (dig-36));
        if(k==1)break;
    }
    cout << endl;
}
static void Conv62ToPtr(const string &s)
{
    cout << "Label \"" << s << "\" = ";
    const char *q = s.c_str();
    unsigned value=0;
    for(; *q; ++q)
    {
        if(*q >= '0' && *q <= '9') value = value*62 + *q - '0';
        else if(*q >= 'a' && *q <= 'z') value = value*62 + *q - ('a'-36);
        else if(*q >= 'A' && *q <= 'Z') value = value*62 + *q - ('A'-10);
    }
    char Buf[64];
    sprintf(Buf, "%02X:%04X\n", value>>16, value&0xFFFF);
    cout << Buf << endl;
}

int main(int argc, const char *const *argv)
{
    string s = argv[1];
    
    ConvPtrTo62(s);
    
    Conv62ToPtr(s);
    
    return 0;
}
