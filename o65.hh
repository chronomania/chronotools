#ifndef bqtO65hh
#define bqtO65hh

/* xa65 object file loader and linker for C++
 * For loading and linking 65816 object files
 * Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)
 *
 * Version 1.0.1 - Aug 18 2003
 */

#include <cstdio>
#include <vector>
#include <string>
#include <utility>

using std::FILE;
using std::vector;
using std::string;
using std::pair;

/* An xa65 object file loader */

/*
 * Features:
 *
 *    Loading TEXT segment
 *    Relocating the TEXT segment
 *    Acquiring symbol pointers
 *    Defining the undefined numeric constants
 *    Defining the undefined symbol addresses
 *
 * Defects:
 *
 *    Only the TEXT segment is handled.
 *    Only 16-bit, LOW and HIGH type fixups and relocs are supported.
 *    File validity is not checked.
 *    Undefined symbols may only be defined once.
 *
 */

class O65
{
public:
    O65();
    ~O65();
    
    /* Loads an object file from the specified file */
    void Load(FILE *fp);
    
    /* Relocated the TEXT segment to new address */
    void LocateCode(unsigned newaddress);
    
    /* Defines the value of a symbol the TEXT is referring */
    void LinkSym(const string& name, unsigned value);
    
    /* Returns the TEXT segment */
    const vector<unsigned char>& GetCode() const;
    
    /* Returns the TEXT segment size */
    unsigned GetCodeSize() const;
    
    /* Returns the address of a global defined in TEXT segment */
    unsigned GetSymAddress(const string& name) const;
    
    /* Verifies that all symbols have been properly defined */
    void Verify() const;

    class segment;
private:
    // undefined symbols; sym -> defined_flag -> old value
    vector<pair<string, pair<bool, unsigned> > > undefines;
    
    // No assigning
    O65(const O65 &);
    void operator= (const O65 &);
    
    segment *text, *data;
};

/*
 Example code:

#include <cstdio>
#include "o65.hh"

using namespace std;

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
*/

#endif
