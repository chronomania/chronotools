#ifndef bqtO65hh
#define bqtO65hh

/* xa65 object file loader and linker for C++
 * For loading and linking 65816 object files
 * Copyright (C) 1992,2003 Bisqwit (http://iki.fi/bisqwit/)
 *
 * Version 1.2.0 - Aug 18 2003, Sep 4 2003
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
 *    Five type of fixups and relocs: 16bit, 16/hi, 16/lo, long, seg
 *
 * Defects:
 *
 *    Only the TEXT segment is handled.
 *    File validity is not checked carefully.
 *    Undefined symbols may only be defined once.
 *
 */

class O65
{
public:
    O65();
    ~O65();
    
    // Copy constructor, assignment operator
    O65(const O65 &);
    void operator= (const O65 &);
    
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
    
    bool HasSym(const string& name) const;
    
    const vector<string> GetSymbolList() const;
    const vector<string> GetExternList() const;
    
    const string GetByteAt(unsigned addr) const;
    const string GetWordAt(unsigned addr) const;
    const string GetLongAt(unsigned addr) const;
    
    /* Verifies that all symbols have been properly defined */
    void Verify() const;

    class segment;
private:
    // undefined symbols; sym -> defined_flag -> old value
    vector<pair<string, pair<bool, unsigned> > > undefines;
    
    segment *text, *data;
};

#endif
