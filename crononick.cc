#include <cstdio>
#include <cstdarg>

#include "ctinsert.hh"
#include "config.hh"
#include "o65.hh"

using namespace std;

void insertor::GenerateCrononickCode()
{
    const string codefile = WstrToAsc(GetConf("crononick", "file").SField());

    FILE *fp = fopen(codefile.c_str(), "rb");
    if(!fp) return;

    O65 crononick_code;
    
    crononick_code.Load(fp);
    fclose(fp);
    
    const unsigned Code_Size = crononick_code.GetCodeSize();
    
    const unsigned Code_Address = freespace.FindFromAnyPage(Code_Size);
    
    fprintf(stderr, "Writing crononick-handler:"
                    " %u(code)@ $%06X\n",
        Code_Size,       0xC00000 | Code_Address
           );
    
    crononick_code.LocateCode(Code_Address | 0xC00000);

    LinkCalls("crononick", crononick_code);
    
    crononick_code.Verify();

    PlaceData(crononick_code.GetCode(), Code_Address);
}
