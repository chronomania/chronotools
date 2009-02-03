#include <cstdio>
#include <iostream>
#include <string>
#include <cstdlib>
using namespace std;

#include "base62.hh"

static void ConvPtrTo62(const string &s)
{
    cout << "Pointer \"" << s << "\" = ";
    const char *q = s.c_str();
    long value = std::strtol(q, NULL, 16);
    cout << EncodeBase62(value, 4);
    if(value & 0xC00000)
    {
        cout << " -> ";
        value &= 0x3FFFFF;
        cout << EncodeBase62(value, 4);
    }
    else
    {
        cout << " (low)";
    }
    cout << endl;
}
static void Conv62ToPtr(const string &s)
{
    cout << "Label \"" << s << "\" = ";
    const char *q = s.c_str();
    unsigned value=0;
    while(*q && CumulateBase62(value, *q)) ++q;
    
    char Buf[64];
    sprintf(Buf, "%02X:%04X", value>>16, value&0xFFFF);
    cout << Buf;
    if(value & 0xC00000)
        cout << " (high)";
    else
    {
        value |= 0xC00000;
        sprintf(Buf, " -> %02X:%04X", value>>16, value&0xFFFF);
        cout << Buf;
    }
    cout << endl;
}

int main(int /*argc*/, const char *const *argv)
{
    string s = argv[1];
    
    ConvPtrTo62(s);
    
    Conv62ToPtr(s);
    
    return 0;
}
