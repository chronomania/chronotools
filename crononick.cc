#include <cstdio>
#include <cstdarg>

#include "snescode.hh"
#include "ctinsert.hh"
#include "config.hh"
#include "o65.hh"

using namespace std;

void insertor::GenerateCrononickCode()
{
    const string codefile = WstrToAsc(GetConf("crononick", "file").SField());

    O65 crononick_code = LoadObject(codefile, "Crononick");
    if(crononick_code.Error()) return;
    
    if(!LinkCalls("crononick"))
    {
        fprintf(stderr, "> > Crononick won't be used\n");
        return;
    }
    
    objects.AddObject(crononick_code, "crononick code");
}
