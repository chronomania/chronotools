#include <cstdio>
#include <cstdarg>

#include "ctinsert.hh"
#include "config.hh"
#include "o65.hh"

using namespace std;

void insertor::GenerateCrononickCode()
{
    const string codefile = WstrToAsc(GetConf("crononick", "file").SField());

    O65 crononick_code;
    {FILE *fp = fopen(codefile.c_str(), "rb");
    if(!fp) { perror(codefile.c_str()); return; }
    crononick_code.Load(fp);
    fclose(fp);}
    
    const unsigned Code_Size = crononick_code.GetCodeSize();
    
    const unsigned Code_Address = freespace.FindFromAnyPage(Code_Size);
    
    fprintf(stderr,
        "\r> Crononick(%s):"
            " %u(code)@ $%06X\n",
        codefile.c_str(),
        Code_Size, 0xC00000 | Code_Address
           );
    
    crononick_code.LocateCode(Code_Address | 0xC00000);

    LinkCalls("crononick", crononick_code);
    
    crononick_code.Verify();

    PlaceData(crononick_code.GetCode(), Code_Address);
}
