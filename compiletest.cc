#include "compiler.hh"

int main()
{
    FILE *fp = fopen("taipus.txt", "rt");
    
    FunctionList l = Compile(fp);
    
    l.RequireFunction("IsEs");
    
    fclose(fp);
}
